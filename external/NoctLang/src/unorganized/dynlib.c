/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Restricted native-library resolver and process/VM lifetime management. */

#include "dynlib.h"
#include "noct-api.h"
#include "atomic.h"

#if defined(NOCT_USE_DYNLIB)
#include "dynlib-os.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum process_library_state {
	PROCESS_LIBRARY_OPENING,
	PROCESS_LIBRARY_OPEN,
	PROCESS_LIBRARY_FAILED
};

enum vm_library_state {
	VM_LIBRARY_LOADING,
	VM_LIBRARY_LOADED,
	VM_LIBRARY_FAILED
};

struct process_library {
	char *path;
	int state;
	void *handle;
	bool (CDECL *init)(const NoctAPI *api, NoctEnv *env);
	char error[512];
	struct process_library *next;
};

struct rt_library_state {
	char *path;
	int state;
	struct rt_env *loading_env;
	char error[512];
	struct rt_library_state *next;
};

#if defined(NOCT_USE_DYNLIB)
static struct process_library *process_library_list;
static int process_library_lock;
#endif

static bool
dynlib_valid_name(const char *name)
{
	const unsigned char *p;
	unsigned char c;

	if (name == NULL)
		return false;
	c = (unsigned char)name[0];
	if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	      c == '_'))
		return false;
	for (p = (const unsigned char *)name + 1; *p != '\0'; p++)
		if (!((*p >= 'A' && *p <= 'Z') ||
		      (*p >= 'a' && *p <= 'z') ||
		      (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
			return false;
	return true;
}

static void
dynlib_copy_error(char *dst, size_t size, const char *src)
{
	if (size == 0)
		return;
	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';
}

#if defined(NOCT_USE_DYNLIB)
static char *
dynlib_join(const char *dir, const char *name)
{
	size_t size;
	char *path;

	size = strlen(dir) + 1 + strlen(name) + 1;
	path = malloc(size);
	if (path != NULL)
		snprintf(path, size, "%s/%s", dir, name);
	return path;
}

static char *
dynlib_mapped_name(const char *name)
{
	const char *prefix;
	const char *suffix;
	size_t size;
	char *mapped;

#if defined(NOCT_TARGET_WINDOWS)
	prefix = "";
	suffix = ".dll";
#elif defined(__APPLE__)
	prefix = "lib";
	suffix = ".dylib";
#else
	prefix = "lib";
	suffix = ".so";
#endif
	size = strlen(prefix) + strlen(name) + strlen(suffix) + 1;
	mapped = malloc(size);
	if (mapped != NULL)
		snprintf(mapped, size, "%s%s%s", prefix, name, suffix);
	return mapped;
}

static const char *
dynlib_target_triple(void)
{
#if defined(NOCT_ARCH_X86_64)
#define NOCT_DYNLIB_ARCH "x86_64"
#elif defined(NOCT_ARCH_X86)
#define NOCT_DYNLIB_ARCH "x86"
#elif defined(NOCT_ARCH_ARM64)
#define NOCT_DYNLIB_ARCH "arm64"
#elif defined(NOCT_ARCH_ARM32)
#define NOCT_DYNLIB_ARCH "armv7"
#elif defined(NOCT_ARCH_PPC64)
#define NOCT_DYNLIB_ARCH "ppc64"
#elif defined(NOCT_ARCH_PPC32)
#define NOCT_DYNLIB_ARCH "ppc32"
#else
#define NOCT_DYNLIB_ARCH "unknown"
#endif
#if defined(NOCT_TARGET_WINDOWS)
	return NOCT_DYNLIB_ARCH "-windows";
#elif defined(__APPLE__)
	return NOCT_DYNLIB_ARCH "-macos";
#else
	return NOCT_DYNLIB_ARCH "-linux";
#endif
#undef NOCT_DYNLIB_ARCH
}

static int
dynlib_try_directory(
	const char *dir,
	const char *mapped,
	char **canonical,
	char *error,
	size_t error_size)
{
	char *candidate;
	int result;

	candidate = dynlib_join(dir, mapped);
	if (candidate == NULL) {
		snprintf(error, error_size, "out of memory");
		return -1;
	}
	result = dynlib_os_resolve(candidate, canonical, error, error_size);
	free(candidate);
	return result;
}

static int
dynlib_resolve(
	struct rt_env *env,
	const char *name,
	char **canonical,
	char *error,
	size_t error_size)
{
	char *mapped;
	const char *home;
	char *user_dir;
	char *native_dir;
	char *triple_dir;
	const char *package_dir;
	int frame_index;
	int result;

	*canonical = NULL;
	mapped = dynlib_mapped_name(name);
	if (mapped == NULL) {
		snprintf(error, error_size, "out of memory");
		return -1;
	}
	package_dir = NULL;
	for (frame_index = env->cur_frame_index; frame_index >= 0;
	     frame_index--) {
		struct rt_func *func = env->frame_alloc[frame_index].func;
		if (func != NULL && func->owner_module != NULL &&
		    func->owner_module->package_dir != NULL) {
			package_dir = func->owner_module->package_dir;
			break;
		}
	}
	if (package_dir != NULL) {
		native_dir = dynlib_join(package_dir, "native");
		if (native_dir == NULL) {
			snprintf(error, error_size, "out of memory");
			result = -1;
			goto out;
		}
		triple_dir = dynlib_join(native_dir, dynlib_target_triple());
		free(native_dir);
		if (triple_dir == NULL) {
			snprintf(error, error_size, "out of memory");
			result = -1;
			goto out;
		}
		result = dynlib_try_directory(triple_dir, mapped, canonical,
					      error, error_size);
		free(triple_dir);
		if (result != 0)
			goto out;
		result = dynlib_try_directory(package_dir, mapped, canonical,
					      error, error_size);
		if (result != 0)
			goto out;
	}
	result = dynlib_try_directory(".", mapped, canonical, error, error_size);
	if (result != 0)
		goto out;
#if defined(NOCT_TARGET_WINDOWS)
	home = getenv("USERPROFILE");
#else
	home = getenv("HOME");
#endif
	if (home != NULL && home[0] != '\0') {
		user_dir = dynlib_join(home, ".noct/libraries");
		if (user_dir == NULL) {
			snprintf(error, error_size, "out of memory");
			result = -1;
			goto out;
		}
		result = dynlib_try_directory(user_dir, mapped, canonical,
					       error, error_size);
		free(user_dir);
		if (result != 0)
			goto out;
	}
#if !defined(NOCT_TARGET_WINDOWS)
	result = dynlib_try_directory("/usr/local/share/noct/libraries",
				       mapped, canonical, error, error_size);
	if (result == 0)
		result = dynlib_try_directory("/usr/share/noct/libraries",
					       mapped, canonical, error, error_size);
#endif
out:
	free(mapped);
	return result;
}

static struct process_library *
dynlib_get_process_library(
	const char *path,
	char *error,
	size_t error_size)
{
	struct process_library *library;
	void *handle;
	void *symbol;
	uint64_t relax;

	relax = CPU_RELAX_BASE_INITIALIZER;
retry:
	atomic_spin_lock(&process_library_lock);
	for (library = process_library_list; library != NULL;
	     library = library->next) {
		if (strcmp(library->path, path) != 0)
			continue;
		if (library->state == PROCESS_LIBRARY_OPENING) {
			atomic_spin_unlock(&process_library_lock);
			cpu_relax(&relax);
			goto retry;
		}
		if (library->state == PROCESS_LIBRARY_FAILED)
			snprintf(error, error_size, "%s", library->error);
		atomic_spin_unlock(&process_library_lock);
		return library->state == PROCESS_LIBRARY_OPEN ? library : NULL;
	}
	library = calloc(1, sizeof(*library));
	if (library == NULL) {
		atomic_spin_unlock(&process_library_lock);
		snprintf(error, error_size, "out of memory");
		return NULL;
	}
	library->path = malloc(strlen(path) + 1);
	if (library->path == NULL) {
		free(library);
		atomic_spin_unlock(&process_library_lock);
		snprintf(error, error_size, "out of memory");
		return NULL;
	}
	strcpy(library->path, path);
	library->state = PROCESS_LIBRARY_OPENING;
	library->next = process_library_list;
	process_library_list = library;
	atomic_spin_unlock(&process_library_lock);

	handle = NULL;
	if (!dynlib_os_open(path, &handle, library->error,
			    sizeof(library->error))) {
		atomic_spin_lock(&process_library_lock);
		library->state = PROCESS_LIBRARY_FAILED;
		atomic_spin_unlock(&process_library_lock);
		snprintf(error, error_size, "%s", library->error);
		return NULL;
	}
	symbol = dynlib_os_symbol(handle, "noct_library_init", library->error,
				  sizeof(library->error));
	if (symbol == NULL) {
		/* The handle intentionally remains loaded even on failure. */
		atomic_spin_lock(&process_library_lock);
		library->handle = handle;
		library->state = PROCESS_LIBRARY_FAILED;
		atomic_spin_unlock(&process_library_lock);
		snprintf(error, error_size, "%s", library->error);
		return NULL;
	}
	/* ISO C has no data/function pointer conversion; platform loaders define it. */
	memcpy(&library->init, &symbol, sizeof(library->init));
	atomic_spin_lock(&process_library_lock);
	library->handle = handle;
	library->state = PROCESS_LIBRARY_OPEN;
	atomic_spin_unlock(&process_library_lock);
	return library;
}

static void
dynlib_vm_lock(struct rt_vm *vm)
{
#if defined(NOCT_USE_MULTITHREAD)
	if (vm->config.object_model == NOCT_OBJECT_MODEL_MULTI)
		atomic_spin_lock(&vm->library_state_lock);
#else
	UNUSED_PARAMETER(vm);
#endif
}

static void
dynlib_vm_unlock(struct rt_vm *vm)
{
#if defined(NOCT_USE_MULTITHREAD)
	if (vm->config.object_model == NOCT_OBJECT_MODEL_MULTI)
		atomic_spin_unlock(&vm->library_state_lock);
#else
	UNUSED_PARAMETER(vm);
#endif
}
#endif

bool
rt_load_library(
	struct rt_env *env,
	const char *name,
	bool optional,
	bool *loaded)
{
#if !defined(NOCT_USE_DYNLIB)
	/*
	 * For when dynlib is disabled by the build config.
	 */
	*loaded = false;
	if (!dynlib_valid_name(name)) {
		rt_error(env, N_TR("Invalid native library name \"%s\"."),
			 name != NULL ? name : "");
		return false;
	}
	if (optional)
		return true;
	rt_error(env, N_TR("Native library loading is unavailable on this target."));
	return false;
#else
	/*
	 * For when dynlib is enabled by the build config.
	 */

	char *canonical;
	char error[512];
	int resolve_result;
	struct rt_library_state *state;
	struct process_library *process;
	bool init_result;
	uint64_t relax;

	*loaded = false;
	if (!dynlib_valid_name(name)) {
		rt_error(env, N_TR("Invalid native library name \"%s\"."),
			 name != NULL ? name : "");
		return false;
	}

	if (env->library_transaction != NULL) {
		rt_error(env, N_TR("A native library cannot load another library during noct_library_init()."));
		return false;
	}
	error[0] = '\0';
	resolve_result = dynlib_resolve(env, name, &canonical, error,
					sizeof(error));
	if (resolve_result == 0) {
		if (optional)
			return true;
		rt_error(env, N_TR("Native library \"%s\" was not found."), name);
		return false;
	}
	if (resolve_result < 0) {
		rt_error(env, N_TR("Cannot resolve native library \"%s\": %s"),
			 name, error);
		return false;
	}

	relax = CPU_RELAX_BASE_INITIALIZER;
retry_vm:
	dynlib_vm_lock(env->vm);
	for (state = env->vm->library_list; state != NULL;
	     state = state->next) {
		if (strcmp(state->path, canonical) == 0)
			break;
	}
	if (state != NULL) {
		int vm_state = state->state;
		struct rt_env *loading_env = state->loading_env;
		snprintf(error, sizeof(error), "%s", state->error);
		dynlib_vm_unlock(env->vm);
		if (vm_state == VM_LIBRARY_LOADED) {
			free(canonical);
			*loaded = true;
			return true;
		}
		if (vm_state == VM_LIBRARY_LOADING && loading_env != env) {
			cpu_relax(&relax);
			goto retry_vm;
		}
		free(canonical);
		if (vm_state == VM_LIBRARY_LOADING)
			rt_error(env, N_TR("Recursive native library initialization is not supported."));
		else
			rt_error(env, N_TR("Native library \"%s\" previously failed: %s"),
				 name, error);
		return false;
	}
	state = noct_calloc(1, sizeof(*state));
	if (state == NULL) {
		dynlib_vm_unlock(env->vm);
		free(canonical);
		rt_out_of_memory(env);
		return false;
	}
	state->path = noct_strdup(canonical);
	if (state->path == NULL) {
		dynlib_vm_unlock(env->vm);
		free(canonical);
		noct_free(state);
		rt_out_of_memory(env);
		return false;
	}
	state->state = VM_LIBRARY_LOADING;
	state->loading_env = env;
	state->next = env->vm->library_list;
	env->vm->library_list = state;
	dynlib_vm_unlock(env->vm);

	process = dynlib_get_process_library(canonical, error, sizeof(error));
	free(canonical);
	if (process == NULL) {
		dynlib_vm_lock(env->vm);
		state->state = VM_LIBRARY_FAILED;
		state->loading_env = NULL;
		snprintf(state->error, sizeof(state->error), "%s", error);
		dynlib_vm_unlock(env->vm);
		rt_error(env, N_TR("Cannot load native library \"%s\": %s"),
			 name, error);
		return false;
	}
	if (!rt_library_transaction_begin(env)) {
		dynlib_vm_lock(env->vm);
		state->state = VM_LIBRARY_FAILED;
		state->loading_env = NULL;
		snprintf(state->error, sizeof(state->error),
			 "cannot begin registration");
		dynlib_vm_unlock(env->vm);
		return false;
	}
	init_result = process->init(noct_get_api_table(), env);
	if (!init_result) {
		rt_library_transaction_abort(env);
		dynlib_vm_lock(env->vm);
		state->state = VM_LIBRARY_FAILED;
		state->loading_env = NULL;
		if (env->error_message[0] == '\0')
			rt_error(env, N_TR("Native library \"%s\" initialization failed."), name);
		dynlib_copy_error(state->error, sizeof(state->error),
				  env->error_message);
		dynlib_vm_unlock(env->vm);
		return false;
	}
	if (!rt_library_transaction_commit(env)) {
		dynlib_vm_lock(env->vm);
		state->state = VM_LIBRARY_FAILED;
		state->loading_env = NULL;
		dynlib_copy_error(state->error, sizeof(state->error),
				  env->error_message);
		dynlib_vm_unlock(env->vm);
		return false;
	}
	dynlib_vm_lock(env->vm);
	state->state = VM_LIBRARY_LOADED;
	state->loading_env = NULL;
	dynlib_vm_unlock(env->vm);
	*loaded = true;
	return true;
#endif
}

void
rt_cleanup_libraries(
	struct rt_vm *vm)
{
	struct rt_library_state *state;
	struct rt_library_state *next;

	for (state = vm->library_list; state != NULL; state = next) {
		next = state->next;
		noct_free(state->path);
		noct_free(state);
	}
	vm->library_list = NULL;

	/*
	 * Note: Process-global handles intentionally remain loaded.
	 */
}
