/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * API: Thread.*
 *
 * Threading model:
 *  - Thread.createThread() spawns an OS thread with its own NoctEnv.
 *  - Handles (thread, shared, locked, counter) are dictionaries that
 *    carry a native pointer to a small control block.
 *  - Values that must survive across threads are stored in the handle
 *    dictionary itself ("result", "value" keys), so they are rooted by
 *    whoever holds the handle and are relocated properly by the GC.
 *  - Every potentially blocking operation (mutex wait, join, sleep) is
 *    wrapped in noct_enter_blocking()/noct_leave_blocking() so that a
 *    blocked thread never stalls a stop-the-world GC.
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "atomic.h"

#if defined(NOCT_TARGET_WINDOWS)
#include <windows.h>
#elif defined(NOCT_TARGET_POSIX)
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#else
#error "No thread support for this platform."
#endif

/* Magic numbers for the handle control blocks. */
#define THREAD_MAGIC	0x54687264	/* 'Thrd' */
#define SYNC_MAGIC	0x53796e63	/* 'Sync' */
#define COUNTER_MAGIC	0x436e7472	/* 'Cntr' */

/* Maximum nesting depth of a deep copy. */
#define DEEP_COPY_MAX_DEPTH	8

#if defined(NOCT_TARGET_WINDOWS)
typedef HANDLE thr_thread;
typedef CRITICAL_SECTION thr_mutex;
#else
typedef pthread_t thr_thread;
typedef pthread_mutex_t thr_mutex;
#endif

/* Thread handle control block. */
struct thread_obj {
	int magic;
	int joined;
	thr_thread handle;
};

/* Shared/locked handle control block. */
struct sync_obj {
	int magic;
	thr_mutex mutex;
};

/* Counter handle control block. */
struct counter_obj {
	int magic;
	int value;
};

/*
 * Thread startup block.
 *
 * Created and filled by the parent thread, owned and freed by the
 * child. The three values are pinned in the child's environment by the
 * parent, so they are proper GC roots from the moment the child env
 * exists until the child unpins them.
 */
struct thread_start {
	NoctEnv *env;
	NoctValue func_v;
	NoctValue param_v;
	NoctValue handle_v;
};

/* One native function registration. */
struct ffi_item {
	const char *global_name;
	const char *package_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};

static const struct ffi_item *get_ffi_items(size_t *count);
static bool thr_mutex_init(thr_mutex *m);
static void thr_mutex_destroy(thr_mutex *m);
static void thr_mutex_lock_blocking(NoctEnv *env, thr_mutex *m);
static void thr_mutex_unlock(thr_mutex *m);
static bool make_handle_dict(NoctEnv *env, NoctValue *handle, void *native, void (*finalizer)(void *));
static bool get_handle_native(NoctEnv *env, NoctValue *handle, int magic, void **native);
static size_t packed_elem_size(int type);
static bool deep_copy_value(NoctEnv *env, NoctValue *dst, NoctValue *src, int depth);
#if defined(NOCT_TARGET_WINDOWS)
static DWORD WINAPI thread_entry(LPVOID arg);
#else
static void *thread_entry(void *arg);
#endif
static bool cfunc_Thread_createThread(NoctEnv *env);
static void thread_finalizer(void *native_pointer);
static bool cfunc_Thread_joinThread(NoctEnv *env);
static void sync_finalizer(void *native_pointer);
static bool make_sync_handle(NoctEnv *env, NoctValue *handle);
static bool cfunc_Thread_createShared(NoctEnv *env);
static bool cfunc_Thread_updateShared(NoctEnv *env);
static bool cfunc_Thread_snapshotShared(NoctEnv *env);
static void counter_finalizer(void *native_pointer);
static bool cfunc_Thread_createCounter(NoctEnv *env);
static bool cfunc_Thread_incrementCounter(NoctEnv *env);
static bool cfunc_Thread_getCounter(NoctEnv *env);
static bool cfunc_Thread_createLocked(NoctEnv *env);
static bool cfunc_Thread_withLock(NoctEnv *env);
static bool cfunc_Thread_sleep(NoctEnv *env);

/*
 * Registers the Thread API.
 */
NOCT_DLL
bool
noct_register_api_thread(
	NoctEnv *env)
{
	NoctValue thread_dict;
	NoctValue funcval;
	const struct ffi_item *ffi_items;
	const char *param[NOCT_ARG_MAX];
	size_t ffi_item_count;
	size_t i;
	size_t j;

	/* Creates the global Thread dictionary. */
	if (!noct_make_empty_dict(env, &thread_dict))
		return false;

	/* Publishes the global Thread dictionary. */
	if (!noct_set_global(env, "Thread", &thread_dict))
		return false;

	/* Registers every Thread function in declaration order. */
	ffi_items = get_ffi_items(&ffi_item_count);
	for (i = 0; i < ffi_item_count; i++) {
		/* Copies the immutable parameter names for the registration API. */
		for (j = 0; j < NOCT_ARG_MAX; j++)
			param[j] = ffi_items[i].param[j];

		/* Registers the native function globally. */
		if (!noct_register_cfunc(
			env,
			ffi_items[i].global_name,
			ffi_items[i].param_count,
			param,
			ffi_items[i].cfunc,
			NULL)) {
			return false;
		}

		/* Fetches the registered function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Publishes the function in the Thread dictionary. */
		if (!noct_set_dict_elem_cstr(env, &thread_dict, ffi_items[i].field_name, &funcval))
			return false;
	}

	/* Reports successful API registration. */
	return true;
}

/* Returns the immutable native-function table. */
static const struct ffi_item *
get_ffi_items(
	size_t *count)
{
	static const struct ffi_item ffi_items[] = {
		{
			"Thread.createThread", "Thread", "createThread", 2,
			{"func", "param"}, cfunc_Thread_createThread
		},
		{
			"Thread.joinThread", "Thread", "joinThread", 1,
			{"th"}, cfunc_Thread_joinThread
		},
		{
			"Thread.createShared", "Thread", "createShared", 1,
			{"value"}, cfunc_Thread_createShared
		},
		{
			"Thread.updateShared", "Thread", "updateShared", 2,
			{"shared", "value"}, cfunc_Thread_updateShared
		},
		{
			"Thread.snapshotShared", "Thread", "snapshotShared", 1,
			{"shared"}, cfunc_Thread_snapshotShared
		},
		{
			"Thread.createCounter", "Thread", "createCounter", 0,
			{NULL}, cfunc_Thread_createCounter
		},
		{
			"Thread.incrementCounter", "Thread", "incrementCounter", 1,
			{"counter"}, cfunc_Thread_incrementCounter
		},
		{
			"Thread.getCounter", "Thread", "getCounter", 1,
			{"counter"}, cfunc_Thread_getCounter
		},
		{
			"Thread.createLocked", "Thread", "createLocked", 1,
			{"dict"}, cfunc_Thread_createLocked
		},
		{
			"Thread.withLock", "Thread", "withLock", 2,
			{"locked", "func"}, cfunc_Thread_withLock
		},
		{
			"Thread.sleep", "Thread", "sleep", 1,
			{"ms"}, cfunc_Thread_sleep
		}
	};

	assert(count != NULL);

	/* Publishes the immutable table size. */
	*count = sizeof(ffi_items) / sizeof(ffi_items[0]);

	/* Returns the immutable registration table. */
	return ffi_items;
}

/* Initializes a recursive platform mutex. */
static bool
thr_mutex_init(
	thr_mutex *m)
{
#if defined(NOCT_TARGET_POSIX)
	pthread_mutexattr_t attr;

#endif
#if defined(NOCT_TARGET_WINDOWS)
	/* Initializes the recursive Windows critical section. */
	InitializeCriticalSection(m);

	/* Reports a successfully initialized mutex. */
	return true;
#else
	/* Initializes the POSIX mutex attributes. */
	if (pthread_mutexattr_init(&attr) != 0)
		return false;

	/* Selects recursive mutex behavior. */
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	/* Initializes the POSIX mutex from the configured attributes. */
	if (pthread_mutex_init(m, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return false;
	}

	/* Releases the temporary POSIX mutex attributes. */
	pthread_mutexattr_destroy(&attr);

	/* Reports a successfully initialized mutex. */
	return true;
#endif
}

/* Destroys a platform mutex. */
static void
thr_mutex_destroy(
	thr_mutex *m)
{
#if defined(NOCT_TARGET_WINDOWS)
	/* Destroys the Windows critical section. */
	DeleteCriticalSection(m);
#else
	/* Destroys the POSIX mutex. */
	pthread_mutex_destroy(m);
#endif
}

/* Locks a mutex without blocking the VM. */
static void
thr_mutex_lock_blocking(
	NoctEnv *env,
	thr_mutex *m)
{
	/* Leaves VM execution before waiting for the mutex. */
	noct_enter_blocking(env);
#if defined(NOCT_TARGET_WINDOWS)
	/* Acquires the Windows critical section. */
	EnterCriticalSection(m);
#else
	/* Acquires the POSIX mutex. */
	pthread_mutex_lock(m);
#endif

	/* Returns the owning thread to VM execution. */
	noct_leave_blocking(env);
}

/* Unlocks a platform mutex. */
static void
thr_mutex_unlock(
	thr_mutex *m)
{
#if defined(NOCT_TARGET_WINDOWS)
	/* Releases the Windows critical section. */
	LeaveCriticalSection(m);
#else
	/* Releases the POSIX mutex. */
	pthread_mutex_unlock(m);
#endif
}

/* Makes a handle dictionary with a native control block. */
static bool
make_handle_dict(
	NoctEnv *env,
	NoctValue *handle,
	void *native,
	void (*finalizer)(void *))
{
	/* Creates the handle dictionary. */
	if (!noct_make_empty_dict(env, handle))
		return false;

	/* Attaches the native control block to the dictionary. */
	if (!noct_set_dict_native_pointer(env, handle, native, finalizer))
		return false;

	/* Reports a completed native handle. */
	return true;
}

/* Gets a control block from a handle dictionary with a magic check. */
static bool
get_handle_native(
	NoctEnv *env,
	NoctValue *handle,
	int magic,
	void **native)
{
	void (*finalizer)(void *);

	/* Retrieves the native control block from the handle. */
	if (!noct_get_dict_native_pointer(env, handle, native, &finalizer))
		return false;

	/* Rejects a missing control block or a handle of another kind. */
	if (*native == NULL || *(int *)*native != magic) {
		noct_error(env, N_TR("Invalid handle."));
		return false;
	}

	/* Reports a valid native handle. */
	return true;
}

/* Gets a packed element size by type. */
static size_t
packed_elem_size(
	int type)
{
	/* Selects the storage width for the packed element type. */
	switch (type) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
		/* Reports the one-byte element width. */
		return 1;
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
		/* Reports the two-byte element width. */
		return 2;
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
	case NOCT_PACKED_FLOAT32:
		/* Reports the four-byte element width. */
		return 4;
	default:
		/* Reports the width of every remaining packed element. */
		return 8;
	}
}

/* Deep-copies a value into a pinned destination. */
static bool
deep_copy_value(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src,
	int depth)
{
	int type;
	int ptype;
	NoctValue key;
	NoctValue elem;
	NoctValue copy;
	size_t size;
	size_t i;
	size_t esize;
	void *src_buf;
	void *buf;

	/* Rejects a value nested beyond the supported copy depth. */
	if (depth > DEEP_COPY_MAX_DEPTH) {
		noct_error(env, N_TR("Shared value is nested too deeply."));
		return false;
	}

	/* Retrieves the source value type. */
	if (!noct_get_value_type(env, src, &type))
		return false;

	/* Selects the structural copy operation for the source type. */
	switch (type) {
	case NOCT_VALUE_INT:
	case NOCT_VALUE_FLOAT:
	case NOCT_VALUE_LONG:
	case NOCT_VALUE_DOUBLE:
	case NOCT_VALUE_STRING:
	case NOCT_VALUE_FUNC:
		/* Copies an immutable value by value or immutable reference. */
		*dst = *src;

		/* Reports a completed immutable copy. */
		return true;
	case NOCT_VALUE_ARRAY:
		/* Pins the array element and its recursive copy. */
		if (!noct_pin_local(env, 2, &elem, &copy))
			return false;

		/* Retrieves the source array size. */
		if (!noct_get_array_size(env, src, &size))
			return false;

		/* Creates the destination array. */
		if (!noct_make_empty_array(env, dst))
			return false;

		/* Resizes a nonempty destination array. */
		if (size > 0) {
			/* Allocates every destination element. */
			if (!noct_resize_array(env, dst, size))
				return false;
		}

		/* Deep-copies every source array element in order. */
		for (i = 0; i < size; i++) {
			/* Retrieves the next source element. */
			if (!noct_get_array_elem(env, src, i, &elem))
				return false;

			/* Deep-copies the source element. */
			if (!deep_copy_value(env, &copy, &elem, depth + 1))
				return false;

			/* Publishes the copied destination element. */
			if (!noct_set_array_elem(env, dst, i, &copy))
				return false;
		}

		/* Releases the array-copy roots. */
		noct_unpin_local(env, 2, &elem, &copy);

		/* Reports a completed array copy. */
		return true;
	case NOCT_VALUE_DICT:
		/* Pins the dictionary key, element, and recursive copy. */
		if (!noct_pin_local(env, 3, &key, &elem, &copy))
			return false;

		/* Retrieves the source dictionary size. */
		if (!noct_get_dict_size(env, src, &size))
			return false;

		/* Creates the destination dictionary. */
		if (!noct_make_empty_dict(env, dst))
			return false;

		/* Deep-copies every source dictionary value in index order. */
		for (i = 0; i < size; i++) {
			/* Retrieves the next source dictionary entry. */
			if (!noct_get_dict_by_index(env, src, i, &key, &elem))
				return false;

			/* Deep-copies the source dictionary value. */
			if (!deep_copy_value(env, &copy, &elem, depth + 1))
				return false;

			/* Publishes the copied destination entry. */
			if (!noct_set_dict_elem(env, dst, &key, &copy))
				return false;
		}

		/* Releases the dictionary-copy roots. */
		noct_unpin_local(env, 3, &key, &elem, &copy);

		/* Reports a completed dictionary copy. */
		return true;
	case NOCT_VALUE_PACKED:
		/* Retrieves the packed element type. */
		if (!noct_get_packed_type(env, src, &ptype))
			return false;

		/* Retrieves the packed element count. */
		if (!noct_get_packed_size(env, src, &size))
			return false;

		/* Retrieves the packed source storage. */
		if (!noct_get_packed_pointer(env, src, &src_buf))
			return false;

		/* Computes and allocates the packed byte storage. */
		esize = packed_elem_size(ptype);
		buf = noct_malloc(size * esize);
		if (buf == NULL) {
			noct_out_of_memory(env);
			return false;
		}

		/* Copies the packed bytes into the owned storage. */
		memcpy(buf, src_buf, size * esize);

		/* Publishes the copied packed value with its native finalizer. */
		if (!noct_make_packed(
			env,
			dst,
			ptype,
			size * esize,
			size,
			buf,
			buf,
			noct_free)) {
			noct_free(buf);
			return false;
		}

		/* Reports a completed packed copy. */
		return true;
	default:
		/* Rejects a mutable value type without copy support. */
		noct_error(env, N_TR("Cannot copy this value type."));
		return false;
	}
}

/* Runs a Noct function in a child thread environment. */
#if defined(NOCT_TARGET_WINDOWS)
static DWORD WINAPI
thread_entry(
	LPVOID arg)
#else
static void *
thread_entry(
	void *arg)
#endif
{
	struct thread_start *start;
	NoctEnv *env;
	NoctValue func_v;
	NoctValue param_v;
	NoctValue handle_v;
	NoctValue ret_v;
	NoctValue tmp;
	NoctFunc *f;
	const char *msg;

	/* Recovers the child environment from the startup block. */
	start = (struct thread_start *)arg;
	env = start->env;

	/* Adopts the environment that the parent created. */
	noct_attach_thread_env(env);

	/* Pins the child-owned arguments, handle, result, and temporary value. */
	memset(&func_v, 0, sizeof(NoctValue));
	memset(&param_v, 0, sizeof(NoctValue));
	memset(&handle_v, 0, sizeof(NoctValue));
	memset(&ret_v, 0, sizeof(NoctValue));
	memset(&tmp, 0, sizeof(NoctValue));
	noct_pin_local(env, 5, &func_v, &param_v, &handle_v, &ret_v, &tmp);

	/*
	 * Takes over the arguments. Both the source and the destination
	 * slots are pinned, so this is safe against a concurrent GC.
	 */
	func_v = start->func_v;
	param_v = start->param_v;
	handle_v = start->handle_v;

	/* Releases the parent-created roots and startup block. */
	noct_unpin_local(env, 3, &start->func_v, &start->param_v, &start->handle_v);
	noct_free(start);
	start = NULL;

	/* Resolves the child function value. */
	if (!noct_get_func(env, &func_v, &f)) {
		/* Parks the unusable child environment. */
		noct_detach_thread_env(env);
#if defined(NOCT_TARGET_WINDOWS)
		/* Terminates the Windows thread entry routine. */
		return 0;
#else
		/* Terminates the POSIX thread entry routine. */
		return NULL;
#endif
	}

	/* Calls the child function with its transferred argument. */
	if (noct_call(env, f, 1, &param_v, &ret_v)) {
		/* Stores the result into the shared handle dictionary. */
		noct_set_dict_elem_cstr(env, &handle_v, "result", &ret_v);
	} else {
		/* Retrieves the child error message. */
		msg = NULL;
		noct_get_error_message(env, &msg);

		/* Stores the error message into the shared handle dictionary. */
		noct_set_dict_elem_make_string(
			env,
			&handle_v,
			"error",
			&tmp,
			msg != NULL ? msg : "unknown error");
	}

	/* Parks the completed child environment for recycling. */
	noct_detach_thread_env(env);

#if defined(NOCT_TARGET_WINDOWS)
	/* Terminates the Windows thread entry routine. */
	return 0;
#else
	/* Terminates the POSIX thread entry routine. */
	return NULL;
#endif
}

/* Implements Thread.createThread(). */
static bool
cfunc_Thread_createThread(
	NoctEnv *env)
{
	NoctValue func;
	NoctValue param;
	NoctValue handle;
	NoctFunc *f;
	struct thread_obj *obj;
	struct thread_start *start;
	bool created;

	/* Pins the child function, argument, and returned handle. */
	memset(&func, 0, sizeof(NoctValue));
	memset(&param, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &func, &param, &handle);

	/* Reads the child function argument. */
	if (!noct_get_arg_check_func(env, 0, &func, &f))
		return false;

	/* Reads the value passed to the child function. */
	if (!noct_get_arg(env, 1, &param))
		return false;

	/* Allocates the thread control block. */
	obj = noct_malloc(sizeof(struct thread_obj));
	if (obj == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the thread control block. */
	memset(obj, 0, sizeof(struct thread_obj));
	obj->magic = THREAD_MAGIC;

	/* Creates the managed thread handle. */
	if (!make_handle_dict(env, &handle, obj, thread_finalizer)) {
		noct_free(obj);
		return false;
	}

	/* Allocates the child startup block. */
	start = noct_malloc(sizeof(struct thread_start));
	if (start == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the child startup block. */
	memset(start, 0, sizeof(struct thread_start));

	/*
	 * Creates the child's environment here, while this thread is
	 * in-flight: no stop-the-world section can be running, so
	 * linking the new env into the VM's env list is race-free.
	 */
	if (!noct_create_thread_env(env, &start->env)) {
		noct_free(start);
		return false;
	}

	/*
	 * Hands over the arguments. Pinning them in the child's
	 * environment makes them GC roots before the child runs.
	 */
	noct_pin_local(
		start->env,
		3,
		&start->func_v,
		&start->param_v,
		&start->handle_v);
	start->func_v = func;
	start->param_v = param;
	start->handle_v = handle;

	/* Starts the platform thread with the transferred startup block. */
#if defined(NOCT_TARGET_WINDOWS)
	obj->handle = CreateThread(
		NULL,
		0,
		thread_entry,
		start,
		0,
		NULL);
	created = obj->handle != NULL;
#else
	created = pthread_create(
		&obj->handle,
		NULL,
		thread_entry,
		start) == 0;
#endif

	/* Releases child resources after a platform thread creation failure. */
	if (!created) {
		noct_unpin_local(
			start->env,
			3,
			&start->func_v,
			&start->param_v,
			&start->handle_v);
		noct_release_thread_env(start->env);
		noct_free(start);
		noct_error(env, N_TR("Cannot create a thread."));
		return false;
	}

	/* Publishes the running thread handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the parent-owned argument and handle roots. */
	noct_unpin_local(env, 3, &func, &param, &handle);

	/* Reports a successfully created thread. */
	return true;
}

/* Finalizes a thread control block. */
static void
thread_finalizer(
	void *native_pointer)
{
	struct thread_obj *obj;

	/* Recovers the thread control block. */
	obj = (struct thread_obj *)native_pointer;

	/* Ignores an empty native pointer. */
	if (obj == NULL)
		return;

	/* Releases an unjoined thread's platform resources on exit. */
	if (!obj->joined) {
#if defined(NOCT_TARGET_WINDOWS)
		/* Releases the Windows thread handle. */
		CloseHandle(obj->handle);
#else
		/* Detaches the unjoined POSIX thread. */
		pthread_detach(obj->handle);
#endif
	}

	/* Releases the thread control block. */
	noct_free(obj);
}

/* Implements Thread.joinThread(). */
static bool
cfunc_Thread_joinThread(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	NoctValue err;
	struct thread_obj *obj;
	const char *msg;
	bool has_result;
	bool has_error;

	/* Pins the thread handle, result, and error value. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	memset(&err, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &handle, &ret, &err);

	/* Reads the thread handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the thread control block. */
	if (!get_handle_native(env, &handle, THREAD_MAGIC, (void **)&obj))
		return false;

	/* Rejects a thread whose platform resources were already joined. */
	if (obj->joined) {
		noct_error(env, N_TR("Thread is already joined."));
		return false;
	}

	/* Leaves VM execution before waiting for the child thread. */
	noct_enter_blocking(env);
#if defined(NOCT_TARGET_WINDOWS)
	/* Waits for and releases the Windows thread handle. */
	WaitForSingleObject(obj->handle, INFINITE);
	CloseHandle(obj->handle);
#else
	/* Waits for the POSIX child thread. */
	pthread_join(obj->handle, NULL);
#endif

	/* Returns the joining thread to VM execution. */
	noct_leave_blocking(env);

	/* Records that the platform thread resources were joined. */
	obj->joined = 1;

	/* Checks whether the child published an error. */
	if (!noct_check_dict_key_cstr(env, &handle, "error", &has_error))
		return false;

	/* Propagates an error published by the child thread. */
	if (has_error) {
		msg = NULL;

		/* Retrieves the child error string. */
		if (!noct_get_dict_elem_check_string(env, &handle, "error", &err, &msg))
			return false;

		/* Reports the child error in the joining environment. */
		noct_error(env, N_TR("Thread error: %s"), msg);
		return false;
	}

	/* Checks whether the child published a result. */
	if (!noct_check_dict_key_cstr(env, &handle, "result", &has_result))
		return false;

	/* Retrieves a result published by the child thread. */
	if (has_result) {
		/* Fetches the published result value. */
		if (!noct_get_dict_elem_cstr(env, &handle, "result", &ret))
			return false;
	}

	/* Publishes the child result in the joining environment. */
	if (!noct_set_return(env, &ret))
		return false;

	/* Releases the thread handle, result, and error roots. */
	noct_unpin_local(env, 3, &handle, &ret, &err);

	/* Reports a successfully joined thread. */
	return true;
}

/* Finalizes a synchronized handle control block. */
static void
sync_finalizer(
	void *native_pointer)
{
	struct sync_obj *obj;

	/* Recovers the synchronized control block. */
	obj = (struct sync_obj *)native_pointer;

	/* Ignores an empty native pointer. */
	if (obj == NULL)
		return;

	/* Destroys the owned mutex and releases the control block. */
	thr_mutex_destroy(&obj->mutex);
	noct_free(obj);
}

/* Makes a shared or locked handle. */
static bool
make_sync_handle(
	NoctEnv *env,
	NoctValue *handle)
{
	struct sync_obj *obj;

	/* Allocates the synchronized control block. */
	obj = noct_malloc(sizeof(struct sync_obj));
	if (obj == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the synchronized control block. */
	memset(obj, 0, sizeof(struct sync_obj));
	obj->magic = SYNC_MAGIC;

	/* Initializes the owned recursive mutex. */
	if (!thr_mutex_init(&obj->mutex)) {
		noct_free(obj);
		noct_error(env, N_TR("Cannot create a mutex."));
		return false;
	}

	/* Creates the managed synchronized handle. */
	if (!make_handle_dict(env, handle, obj, sync_finalizer)) {
		thr_mutex_destroy(&obj->mutex);
		noct_free(obj);
		return false;
	}

	/* Reports a completed synchronized handle. */
	return true;
}

/* Implements Thread.createShared(). */
static bool
cfunc_Thread_createShared(
	NoctEnv *env)
{
	NoctValue value;
	NoctValue copy;
	NoctValue handle;

	/* Pins the source value, copied value, and returned handle. */
	memset(&value, 0, sizeof(NoctValue));
	memset(&copy, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &value, &copy, &handle);

	/* Reads the initial shared value. */
	if (!noct_get_arg(env, 0, &value))
		return false;

	/* Creates the synchronized shared handle. */
	if (!make_sync_handle(env, &handle))
		return false;

	/* Deep-copies the initial shared value. */
	if (!deep_copy_value(env, &copy, &value, 0))
		return false;

	/* Stores the copied value in the shared handle. */
	if (!noct_set_dict_elem_cstr(env, &handle, "value", &copy))
		return false;

	/* Publishes the completed shared handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the source, copy, and handle roots. */
	noct_unpin_local(env, 3, &value, &copy, &handle);

	/* Reports a successfully created shared value. */
	return true;
}

/* Implements Thread.updateShared(). */
static bool
cfunc_Thread_updateShared(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue value;
	NoctValue copy;
	NoctValue ret;
	struct sync_obj *obj;
	bool ok;

	/* Pins the shared handle, source, copy, and result. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&value, 0, sizeof(NoctValue));
	memset(&copy, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 4, &handle, &value, &copy, &ret);

	/* Reads the shared handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Reads the replacement shared value. */
	if (!noct_get_arg(env, 1, &value))
		return false;

	/* Retrieves the synchronized control block. */
	if (!get_handle_native(env, &handle, SYNC_MAGIC, (void **)&obj))
		return false;

	/* Deep-copies the replacement value while holding the mutex. */
	thr_mutex_lock_blocking(env, &obj->mutex);
	ok = deep_copy_value(env, &copy, &value, 0);

	/* Stores the copy only after a successful deep copy. */
	if (ok)
		ok = noct_set_dict_elem_cstr(env, &handle, "value", &copy);

	/* Releases the mutex after either copy outcome. */
	thr_mutex_unlock(&obj->mutex);

	/* Propagates a failed copy or dictionary update. */
	if (!ok)
		return false;

	/* Publishes a successful update result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the handle, source, copy, and result roots. */
	noct_unpin_local(env, 4, &handle, &value, &copy, &ret);

	/* Reports a successfully updated shared value. */
	return true;
}

/* Implements Thread.snapshotShared(). */
static bool
cfunc_Thread_snapshotShared(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue value;
	NoctValue copy;
	struct sync_obj *obj;
	bool ok;

	/* Pins the shared handle, stored value, and copied result. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&value, 0, sizeof(NoctValue));
	memset(&copy, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &handle, &value, &copy);

	/* Reads the shared handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the synchronized control block. */
	if (!get_handle_native(env, &handle, SYNC_MAGIC, (void **)&obj))
		return false;

	/* Retrieves the stored value while holding the mutex. */
	thr_mutex_lock_blocking(env, &obj->mutex);
	ok = noct_get_dict_elem_cstr(env, &handle, "value", &value);

	/* Deep-copies only a successfully retrieved value. */
	if (ok)
		ok = deep_copy_value(env, &copy, &value, 0);

	/* Releases the mutex after either copy outcome. */
	thr_mutex_unlock(&obj->mutex);

	/* Propagates a failed retrieval or deep copy. */
	if (!ok)
		return false;

	/* Publishes the copied snapshot. */
	if (!noct_set_return(env, &copy))
		return false;

	/* Releases the shared handle, stored value, and copied result roots. */
	noct_unpin_local(env, 3, &handle, &value, &copy);

	/* Reports a successfully copied shared snapshot. */
	return true;
}

/* Finalizes a counter control block. */
static void
counter_finalizer(
	void *native_pointer)
{
	/* Releases a present counter control block. */
	if (native_pointer != NULL)
		noct_free(native_pointer);
}

/* Implements Thread.createCounter(). */
static bool
cfunc_Thread_createCounter(
	NoctEnv *env)
{
	NoctValue handle;
	struct counter_obj *obj;

	/* Pins the returned counter handle. */
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 1, &handle);

	/* Allocates the counter control block. */
	obj = noct_malloc(sizeof(struct counter_obj));
	if (obj == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the counter control block. */
	memset(obj, 0, sizeof(struct counter_obj));
	obj->magic = COUNTER_MAGIC;

	/* Creates the managed counter handle. */
	if (!make_handle_dict(env, &handle, obj, counter_finalizer)) {
		noct_free(obj);
		return false;
	}

	/* Publishes the completed counter handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the counter handle root. */
	noct_unpin_local(env, 1, &handle);

	/* Reports a successfully created counter. */
	return true;
}

/* Implements Thread.incrementCounter(). */
static bool
cfunc_Thread_incrementCounter(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	struct counter_obj *obj;
	int new_value;

	/* Pins the counter handle and result. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &handle, &ret);

	/* Reads the counter handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the counter control block. */
	if (!get_handle_native(env, &handle, COUNTER_MAGIC, (void **)&obj))
		return false;

	/* Atomically increments and fetches the new counter value. */
	new_value = atomic_fetch_add_release_int(&obj->value, 1) + 1;

	/* Publishes the incremented counter value. */
	if (!noct_set_return_make_int(env, &ret, new_value))
		return false;

	/* Releases the counter handle and result roots. */
	noct_unpin_local(env, 2, &handle, &ret);

	/* Reports a successful atomic increment. */
	return true;
}

/* Implements Thread.getCounter(). */
static bool
cfunc_Thread_getCounter(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	struct counter_obj *obj;
	int value;

	/* Pins the counter handle and result. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &handle, &ret);

	/* Reads the counter handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the counter control block. */
	if (!get_handle_native(env, &handle, COUNTER_MAGIC, (void **)&obj))
		return false;

	/* Atomically loads the current counter value. */
	value = atomic_load_acquire_int(&obj->value);

	/* Publishes the current counter value. */
	if (!noct_set_return_make_int(env, &ret, value))
		return false;

	/* Releases the counter handle and result roots. */
	noct_unpin_local(env, 2, &handle, &ret);

	/* Reports a successful atomic load. */
	return true;
}

/* Implements Thread.createLocked(). */
static bool
cfunc_Thread_createLocked(
	NoctEnv *env)
{
	NoctValue value;
	NoctValue handle;

	/* Pins the shared dictionary and returned handle. */
	memset(&value, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &value, &handle);

	/* Reads the dictionary that will be shared under the lock. */
	if (!noct_get_arg_check_dict(env, 0, &value))
		return false;

	/* Creates the synchronized locked handle. */
	if (!make_sync_handle(env, &handle))
		return false;

	/*
	 * Store the dictionary as-is: it is intentionally shared and
	 * must only be touched under Thread.withLock().
	 */
	if (!noct_set_dict_elem_cstr(env, &handle, "value", &value))
		return false;

	/* Publishes the completed locked handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the shared dictionary and handle roots. */
	noct_unpin_local(env, 2, &value, &handle);

	/* Reports a successfully created locked dictionary. */
	return true;
}

/* Implements Thread.withLock(). */
static bool
cfunc_Thread_withLock(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue func;
	NoctValue value;
	NoctValue ret;
	NoctFunc *f;
	struct sync_obj *obj;
	bool ok;

	/* Pins the locked handle, callback, stored value, and result. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&func, 0, sizeof(NoctValue));
	memset(&value, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 4, &handle, &func, &value, &ret);

	/* Reads the locked handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Reads the callback function argument. */
	if (!noct_get_arg_check_func(env, 1, &func, &f))
		return false;

	/* Retrieves the synchronized control block. */
	if (!get_handle_native(env, &handle, SYNC_MAGIC, (void **)&obj))
		return false;

	/* Retrieves the shared dictionary while holding the mutex. */
	thr_mutex_lock_blocking(env, &obj->mutex);
	ok = noct_get_dict_elem_cstr(env, &handle, "value", &value);

	/* Calls the callback only after retrieving the shared dictionary. */
	if (ok)
		ok = noct_call(env, f, 1, &value, &ret);

	/* Releases the mutex after either callback outcome. */
	thr_mutex_unlock(&obj->mutex);

	/* Propagates a failed retrieval or callback. */
	if (!ok)
		return false;

	/* Publishes the callback result. */
	if (!noct_set_return(env, &ret))
		return false;

	/* Releases the handle, callback, stored value, and result roots. */
	noct_unpin_local(env, 4, &handle, &func, &value, &ret);

	/* Reports a successfully completed locked callback. */
	return true;
}

/* Implements Thread.sleep(). */
static bool
cfunc_Thread_sleep(
	NoctEnv *env)
{
	NoctValue ms;
	NoctValue ret;
	size_t ms_n;
#if defined(NOCT_TARGET_POSIX)
	struct timespec ts;
	int sleep_result;
#endif

	/* Pins the duration argument and result. */
	memset(&ms, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &ms, &ret);

	/* Reads the sleep duration in milliseconds. */
	if (!noct_get_arg_check_int_long(env, 0, &ms, &ms_n))
		return false;

	/* Leaves VM execution before sleeping the current thread. */
	noct_enter_blocking(env);
#if defined(NOCT_TARGET_WINDOWS)
	/* Sleeps through the Windows thread API. */
	Sleep((DWORD)ms_n);
#else
	/* Converts the millisecond duration to a POSIX timespec. */
	ts.tv_sec = (time_t)(ms_n / 1000);
	ts.tv_nsec = (long)(ms_n % 1000) * 1000000L;

	/* Retries the remaining duration after an interrupted POSIX sleep. */
	for (;;) {
		sleep_result = nanosleep(&ts, &ts);

		/* Stops after a completed or nonstandard sleep result. */
		if (sleep_result != -1)
			break;

		/* Stops after a failure other than interruption. */
		if (errno != EINTR)
			break;
	}
#endif

	/* Returns the sleeping thread to VM execution. */
	noct_leave_blocking(env);

	/* Publishes a successful sleep result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the duration argument and result roots. */
	noct_unpin_local(env, 2, &ms, &ret);

	/* Reports a completed sleep operation. */
	return true;
}
