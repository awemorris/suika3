/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Process API.
 *
 * Process.spawn() starts an interactive child on a pseudo terminal.
 * The other functions exchange data with the child and manage its lifetime.
 * Unsupported platforms expose the same API and report failure cleanly.
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_POSIX)

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(NOCT_TARGET_LINUX) || defined(NOCT_TARGET_ZEDBSD)
#include <pty.h>
#else
#include <util.h>
#endif

#endif

#define PROC_MAX	16
#define PROC_ARG_MAX	64
#define PROC_READ_SIZE	8192

#if !defined(NOCT_TARGET_POSIX)
#define cfunc_Process_spawn	cfunc_Process_unsupported
#define cfunc_Process_read	cfunc_Process_unsupported
#define cfunc_Process_write	cfunc_Process_unsupported
#define cfunc_Process_isAlive	cfunc_Process_unsupported
#define cfunc_Process_kill	cfunc_Process_unsupported
#define cfunc_Process_wait	cfunc_Process_unsupported
#define cfunc_Process_close	cfunc_Process_unsupported
#endif

#if defined(NOCT_TARGET_POSIX)
struct proc_slot {
	int used;
	pid_t pid;
	int fd;
	int exited;
	int status;
};
#endif

static const char *process_spawn_param[NOCT_ARG_MAX] = {
	"argv"
};

static const char *process_read_param[NOCT_ARG_MAX] = {
	"h",
	"timeoutMs"
};

static const char *process_write_param[NOCT_ARG_MAX] = {
	"h",
	"s"
};

static const char *process_handle_param[NOCT_ARG_MAX] = {
	"h"
};

static const char *process_kill_param[NOCT_ARG_MAX] = {
	"h",
	"sig"
};

#if defined(NOCT_TARGET_POSIX)
static struct proc_slot proc_table[PROC_MAX];
#endif

static bool register_process_function(NoctEnv *env, NoctValue *dict, const char *global_name, const char *field_name, size_t param_count, const char *param[], bool (*cfunc)(NoctEnv *env));
#if defined(NOCT_TARGET_POSIX)
static struct proc_slot *proc_get(int handle);
static void proc_poll_exit(struct proc_slot *process);
static void proc_free_argv(char *argv[]);
static bool cfunc_Process_spawn(NoctEnv *env);
static bool cfunc_Process_read(NoctEnv *env);
static bool cfunc_Process_write(NoctEnv *env);
static bool cfunc_Process_isAlive(NoctEnv *env);
static bool cfunc_Process_kill(NoctEnv *env);
static bool cfunc_Process_wait(NoctEnv *env);
static bool cfunc_Process_close(NoctEnv *env);
#else
static bool cfunc_Process_unsupported(NoctEnv *env);
#endif

/*
 * Registers the Process API functions.
 */
NOCT_DLL
bool
noct_register_api_process(
	NoctEnv *env)
{
	NoctValue dict;

	/* Creates the global Process dictionary. */
	if (!noct_make_empty_dict(env, &dict))
		return false;

	/* Publishes the empty Process dictionary. */
	if (!noct_set_global(env, "Process", &dict))
		return false;

	/* Registers Process.spawn(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.spawn",
		"spawn",
		1,
		process_spawn_param,
		cfunc_Process_spawn)) {
		return false;
	}

	/* Registers Process.read(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.read",
		"read",
		2,
		process_read_param,
		cfunc_Process_read)) {
		return false;
	}

	/* Registers Process.write(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.write",
		"write",
		2,
		process_write_param,
		cfunc_Process_write)) {
		return false;
	}

	/* Registers Process.isAlive(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.isAlive",
		"isAlive",
		1,
		process_handle_param,
		cfunc_Process_isAlive)) {
		return false;
	}

	/* Registers Process.kill(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.kill",
		"kill",
		2,
		process_kill_param,
		cfunc_Process_kill)) {
		return false;
	}

	/* Registers Process.wait(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.wait",
		"wait",
		1,
		process_handle_param,
		cfunc_Process_wait)) {
		return false;
	}

	/* Registers Process.close(). */
	if (!register_process_function(
		env,
		&dict,
		"Process.close",
		"close",
		1,
		process_handle_param,
		cfunc_Process_close)) {
		return false;
	}

	/* Reports successful Process API registration. */
	return true;
}

/* Registers one Process function and publishes it in the package. */
static bool
register_process_function(
	NoctEnv *env,
	NoctValue *dict,
	const char *global_name,
	const char *field_name,
	size_t param_count,
	const char *param[],
	bool (*cfunc)(NoctEnv *env))
{
	NoctValue function;

	/* Registers the native global function. */
	if (!noct_register_cfunc(
		env,
		global_name,
		param_count,
		param,
		cfunc,
		NULL)) {
		return false;
	}

	/* Reads the newly registered function value. */
	if (!noct_get_global(env, global_name, &function))
		return false;

	/* Publishes the function in the Process dictionary. */
	if (!noct_set_dict_elem_cstr(
		env,
		dict,
		field_name,
		&function)) {
		return false;
	}

	/* Reports successful function registration. */
	return true;
}

#if defined(NOCT_TARGET_POSIX)

/* Resolves a live process handle. */
static struct proc_slot *
proc_get(
	int handle)
{
	/* Rejects handles outside the process table. */
	if (handle < 0 || handle >= PROC_MAX)
		return NULL;

	/* Rejects unused process slots. */
	if (!proc_table[handle].used)
		return NULL;

	/* Returns the live process slot. */
	return &proc_table[handle];
}

/* Records a child process that has exited. */
static void
proc_poll_exit(
	struct proc_slot *process)
{
	int status;
	pid_t result;

	/* Leaves an already reaped process unchanged. */
	if (process->exited)
		return;

	/* Polls the child without blocking the caller. */
	result = waitpid(process->pid, &status, WNOHANG);

	/* Records the status when the child was reaped. */
	if (result == process->pid) {
		process->exited = 1;
		process->status = status;
	}
}

/* Releases every copied child argument. */
static void
proc_free_argv(
	char *argv[])
{
	int i;

	/* Releases each possibly initialized argument slot. */
	for (i = 0; i < PROC_ARG_MAX; i++)
		free(argv[i]);
}

/* Implements Process.spawn(). */
static bool
cfunc_Process_spawn(
	NoctEnv *env)
{
	NoctValue array;
	NoctValue element;
	NoctValue result;
	char *argv[PROC_ARG_MAX];
	const char *argument;
	size_t argument_count;
	pid_t child;
	int argc;
	int flags;
	int handle;
	int i;
	int master;
	bool success;

	/* Initializes the copied argument array. */
	memset(argv, 0, sizeof(argv));

	/* Pins the VM values used while the child is created. */
	if (!noct_pin_local(env, 3, &array, &element, &result))
		return false;

	/* Reads the child argument array. */
	if (!noct_get_arg_check_array(env, 0, &array)) {
		(void)noct_unpin_local(env, 3, &array, &element, &result);
		return false;
	}

	/* Reads the number of child arguments. */
	if (!noct_get_array_size(env, &array, &argument_count)) {
		(void)noct_unpin_local(env, 3, &array, &element, &result);
		return false;
	}

	/* Rejects empty and overlong child commands. */
	if (argument_count == 0 || argument_count >= PROC_ARG_MAX) {
		(void)noct_unpin_local(env, 3, &array, &element, &result);
		return false;
	}

	/* Copies every argument into child-owned C storage. */
	argc = (int)argument_count;
	for (i = 0; i < argc; i++) {
		/* Reads the next string argument. */
		if (!noct_get_array_elem_check_string(
			env,
			&array,
			(size_t)i,
			&element,
			&argument)) {
			proc_free_argv(argv);
			(void)noct_unpin_local(
				env,
				3,
				&array,
				&element,
				&result);
			return false;
		}

		/* Copies the argument for execvp(). */
		argv[i] = strdup(argument);
		if (argv[i] == NULL) {
			proc_free_argv(argv);
			(void)noct_unpin_local(
				env,
				3,
				&array,
				&element,
				&result);
			return false;
		}
	}
	argv[argc] = NULL;

	/* Finds the first available process slot. */
	handle = -1;
	for (i = 0; i < PROC_MAX; i++) {
		/* Selects the first unused slot. */
		if (!proc_table[i].used) {
			handle = i;
			break;
		}
	}

	/* Reports a full process table. */
	if (handle < 0) {
		success = noct_set_return_make_int(env, &result, -1);
		proc_free_argv(argv);
		(void)noct_unpin_local(env, 3, &array, &element, &result);
		return success;
	}

	/* Creates the child process and its pseudo terminal. */
	child = forkpty(&master, NULL, NULL, NULL);

	/* Reports a failed child creation. */
	if (child < 0) {
		success = noct_set_return_make_int(env, &result, -1);
		proc_free_argv(argv);
		(void)noct_unpin_local(env, 3, &array, &element, &result);
		return success;
	}

	/* Replaces the child image with the requested program. */
	if (child == 0) {
		execvp(argv[0], argv);
		_exit(127);
	}

	/* Enables non-blocking reads from the parent pseudo terminal. */
	flags = fcntl(master, F_GETFL, 0);
	(void)fcntl(master, F_SETFL, flags | O_NONBLOCK);

	/* Publishes the new process in its reserved slot. */
	proc_table[handle].used = 1;
	proc_table[handle].pid = child;
	proc_table[handle].fd = master;
	proc_table[handle].exited = 0;
	proc_table[handle].status = 0;

	/* Returns the new process handle. */
	success = noct_set_return_make_int(env, &result, handle);

	/* Releases the temporary command storage and VM roots. */
	proc_free_argv(argv);
	(void)noct_unpin_local(env, 3, &array, &element, &result);

	/* Reports whether the process handle was returned. */
	return success;
}

/* Implements Process.read(). */
static bool
cfunc_Process_read(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue timeout_value;
	NoctValue result;
	struct proc_slot *process;
	struct pollfd descriptor;
	char buffer[PROC_READ_SIZE];
	ssize_t read_size;
	int handle;
	int timeout_ms;
	bool success;

	/* Pins the arguments and return value during the read. */
	if (!noct_pin_local(
		env,
		3,
		&handle_value,
		&timeout_value,
		&result)) {
		return false;
	}

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&timeout_value,
			&result);
		return false;
	}

	/* Reads the timeout in milliseconds. */
	if (!noct_get_arg_check_int(env, 1, &timeout_value, &timeout_ms)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&timeout_value,
			&result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Returns an empty string for an invalid handle. */
	if (process == NULL) {
		success = noct_set_return_make_string(env, &result, "");
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&timeout_value,
			&result);
		return success;
	}

	/* Tries to read immediately from the pseudo terminal. */
	read_size = read(process->fd, buffer, sizeof(buffer) - 1);

	/* Waits once when a non-blocking read found no available data. */
	if (read_size < 0 &&
	    (errno == EAGAIN || errno == EWOULDBLOCK) &&
	    timeout_ms > 0) {
		descriptor.fd = process->fd;
		descriptor.events = POLLIN;
		noct_enter_blocking(env);
		(void)poll(&descriptor, 1, timeout_ms);
		noct_leave_blocking(env);
		read_size = read(process->fd, buffer, sizeof(buffer) - 1);
	}

	/* Returns an empty string when no output was read. */
	if (read_size <= 0) {
		proc_poll_exit(process);
		success = noct_set_return_make_string(env, &result, "");
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&timeout_value,
			&result);
		return success;
	}

	/* Terminates the bytes as a C string for the Noct string API. */
	buffer[read_size] = '\0';

	/* Returns the terminal bytes through the existing UTF-8 path. */
	success = noct_set_return_make_string(env, &result, buffer);

	/* Releases the rooted arguments and return value. */
	(void)noct_unpin_local(
		env,
		3,
		&handle_value,
		&timeout_value,
		&result);

	/* Reports whether the terminal string was returned. */
	return success;
}

/* Implements Process.write(). */
static bool
cfunc_Process_write(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue string_value;
	NoctValue result;
	struct proc_slot *process;
	const char *string;
	int handle;
	bool success;

	/* Pins the arguments and return value during the write. */
	if (!noct_pin_local(
		env,
		3,
		&handle_value,
		&string_value,
		&result)) {
		return false;
	}

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&string_value,
			&result);
		return false;
	}

	/* Reads the string to write. */
	if (!noct_get_arg_check_string(env, 1, &string_value, &string)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&string_value,
			&result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Reports an invalid process handle. */
	if (process == NULL) {
		success = noct_set_return_make_int(env, &result, 0);
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&string_value,
			&result);
		return success;
	}

	/* Writes the complete string through the pseudo terminal. */
	if (write(process->fd, string, strlen(string)) < 0) {
		success = noct_set_return_make_int(env, &result, 0);
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&string_value,
			&result);
		return success;
	}

	/* Reports a successful terminal write. */
	success = noct_set_return_make_int(env, &result, 1);

	/* Releases the rooted arguments and return value. */
	(void)noct_unpin_local(
		env,
		3,
		&handle_value,
		&string_value,
		&result);

	/* Reports whether the write result was returned. */
	return success;
}

/* Implements Process.isAlive(). */
static bool
cfunc_Process_isAlive(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue result;
	struct proc_slot *process;
	int handle;
	bool success;

	/* Pins the argument and return value during the liveness check. */
	if (!noct_pin_local(env, 2, &handle_value, &result))
		return false;

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(env, 2, &handle_value, &result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Reports an invalid process as not alive. */
	if (process == NULL) {
		success = noct_set_return_make_int(env, &result, 0);
		(void)noct_unpin_local(env, 2, &handle_value, &result);
		return success;
	}

	/* Refreshes the child exit status. */
	proc_poll_exit(process);

	/* Returns the current liveness flag. */
	success = noct_set_return_make_int(
		env,
		&result,
		process->exited ? 0 : 1);

	/* Releases the rooted argument and return value. */
	(void)noct_unpin_local(env, 2, &handle_value, &result);

	/* Reports whether the liveness flag was returned. */
	return success;
}

/* Implements Process.kill(). */
static bool
cfunc_Process_kill(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue signal_value;
	NoctValue result;
	struct proc_slot *process;
	int handle;
	int signal_number;
	bool success;

	/* Pins the arguments and return value during signal delivery. */
	if (!noct_pin_local(
		env,
		3,
		&handle_value,
		&signal_value,
		&result)) {
		return false;
	}

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&signal_value,
			&result);
		return false;
	}

	/* Reads the signal number. */
	if (!noct_get_arg_check_int(env, 1, &signal_value, &signal_number)) {
		(void)noct_unpin_local(
			env,
			3,
			&handle_value,
			&signal_value,
			&result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Delivers the signal to a live child. */
	if (process != NULL && !process->exited)
		(void)kill(process->pid, signal_number);

	/* Returns the existing zero status. */
	success = noct_set_return_make_int(env, &result, 0);

	/* Releases the rooted arguments and return value. */
	(void)noct_unpin_local(
		env,
		3,
		&handle_value,
		&signal_value,
		&result);

	/* Reports whether the status was returned. */
	return success;
}

/* Implements Process.wait(). */
static bool
cfunc_Process_wait(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue result;
	struct proc_slot *process;
	int handle;
	int status;
	int exit_status;
	bool success;

	/* Pins the argument and return value while waiting. */
	if (!noct_pin_local(env, 2, &handle_value, &result))
		return false;

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(env, 2, &handle_value, &result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Reports an invalid process handle. */
	if (process == NULL) {
		success = noct_set_return_make_int(env, &result, -1);
		(void)noct_unpin_local(env, 2, &handle_value, &result);
		return success;
	}

	/* Waits for a child whose status has not been collected. */
	if (!process->exited) {
		noct_enter_blocking(env);
		(void)waitpid(process->pid, &status, 0);
		noct_leave_blocking(env);
		process->exited = 1;
		process->status = status;
	}

	/* Converts the collected wait status into the API result. */
	if (WIFEXITED(process->status))
		exit_status = WEXITSTATUS(process->status);
	else
		exit_status = -1;

	/* Returns the child exit status. */
	success = noct_set_return_make_int(env, &result, exit_status);

	/* Releases the rooted argument and return value. */
	(void)noct_unpin_local(env, 2, &handle_value, &result);

	/* Reports whether the child status was returned. */
	return success;
}

/* Implements Process.close(). */
static bool
cfunc_Process_close(
	NoctEnv *env)
{
	NoctValue handle_value;
	NoctValue result;
	struct proc_slot *process;
	int handle;
	bool success;

	/* Pins the argument and return value while closing the handle. */
	if (!noct_pin_local(env, 2, &handle_value, &result))
		return false;

	/* Reads the process handle. */
	if (!noct_get_arg_check_int(env, 0, &handle_value, &handle)) {
		(void)noct_unpin_local(env, 2, &handle_value, &result);
		return false;
	}

	/* Resolves the requested process. */
	process = proc_get(handle);

	/* Closes and releases a valid process slot. */
	if (process != NULL) {
		(void)close(process->fd);
		process->used = 0;
	}

	/* Returns the existing zero status. */
	success = noct_set_return_make_int(env, &result, 0);

	/* Releases the rooted argument and return value. */
	(void)noct_unpin_local(env, 2, &handle_value, &result);

	/* Reports whether the status was returned. */
	return success;
}

#else

/* Reports that the Process API is unsupported. */
static bool
cfunc_Process_unsupported(
	NoctEnv *env)
{
	NoctValue result;
	bool success;

	/* Preserves the existing best-effort root registration. */
	(void)noct_pin_local(env, 1, &result);

	/* Returns the unsupported-operation status. */
	success = noct_set_return_make_int(env, &result, -1);

	/* Releases the temporary return root. */
	(void)noct_unpin_local(env, 1, &result);

	/* Reports whether the status was returned. */
	return success;
}

#endif
