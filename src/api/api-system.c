/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The System API.
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_WINDOWS)
#include <fcntl.h>
#include <windows.h>
#elif defined(NOCT_TARGET_DOS)
#include <io.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

static const char *system_import_param[NOCT_ARG_MAX] = {
	"file"
};

static const char *system_register_source_param[NOCT_ARG_MAX] = {
	"source"
};

static const char *system_get_env_param[NOCT_ARG_MAX] = {
	"name"
};

static const char *system_shell_param[NOCT_ARG_MAX] = {
	"command"
};

static const char *system_run_command_param[NOCT_ARG_MAX] = {
	"command",
	"workDir",
	"waitForFinish"
};

static const char *system_no_param[NOCT_ARG_MAX] = {
	NULL
};

static const char *system_check_file_param[NOCT_ARG_MAX] = {
	"file"
};

static const char *system_pcall_param[NOCT_ARG_MAX] = {
	"f",
	"a",
	"b"
};

static const char *system_error_param[NOCT_ARG_MAX] = {
	"message"
};

static bool register_system_function(NoctEnv *env, NoctValue *dict, const char *global_name, const char *field_name, size_t param_count, const char *param[], bool (*cfunc)(NoctEnv *env));
static bool cfunc_System_import(NoctEnv *env);
static bool system_load_file(NoctEnv *env, const char *file_name, char **data, size_t *size);
static bool system_report_read_error(NoctEnv *env, const char *file_name, FILE *stream, char **data, size_t *size);
static bool system_finish_file(FILE *stream, char **data, size_t *size, bool succeeded);
static bool cfunc_System_registerSource(NoctEnv *env);
static bool cfunc_System_getEnv(NoctEnv *env);
static bool cfunc_System_shell(NoctEnv *env);
static bool cfunc_System_runCommand(NoctEnv *env);
static bool cfunc_System_getOSName(NoctEnv *env);
static bool cfunc_System_checkFileExists(NoctEnv *env);
static bool cfunc_System_pcall(NoctEnv *env);
static void system_unpin_pcall(NoctEnv *env, NoctValue *function, NoctValue *argument_a, NoctValue *argument_b, NoctValue *result, NoctValue *return_value, NoctValue *temporary);
static bool cfunc_System_error(NoctEnv *env);

/*
 * Registers the System API functions.
 */
NOCT_DLL
bool
noct_register_api_system(
	NoctEnv *env)
{
	NoctValue dict;

	/* Creates the global System dictionary. */
	if (!noct_make_empty_dict(env, &dict))
		return false;

	/* Publishes the empty System dictionary. */
	if (!noct_set_global(env, "System", &dict))
		return false;

	/* Registers System.import(). */
	if (!register_system_function(
		env,
		&dict,
		"System.import",
		"import",
		1,
		system_import_param,
		cfunc_System_import)) {
		return false;
	}

	/* Registers System.registerSource(). */
	if (!register_system_function(
		env,
		&dict,
		"System.registerSource",
		"registerSource",
		1,
		system_register_source_param,
		cfunc_System_registerSource)) {
		return false;
	}

	/* Registers System.getEnv(). */
	if (!register_system_function(
		env,
		&dict,
		"System.getEnv",
		"getEnv",
		1,
		system_get_env_param,
		cfunc_System_getEnv)) {
		return false;
	}

	/* Registers System.shell(). */
	if (!register_system_function(
		env,
		&dict,
		"System.shell",
		"shell",
		1,
		system_shell_param,
		cfunc_System_shell)) {
		return false;
	}

	/* Registers System.runCommand(). */
	if (!register_system_function(
		env,
		&dict,
		"System.runCommand",
		"runCommand",
		3,
		system_run_command_param,
		cfunc_System_runCommand)) {
		return false;
	}

	/* Registers System.getOSName(). */
	if (!register_system_function(
		env,
		&dict,
		"System.getOSName",
		"getOSName",
		0,
		system_no_param,
		cfunc_System_getOSName)) {
		return false;
	}

	/* Registers System.checkFileExists(). */
	if (!register_system_function(
		env,
		&dict,
		"System.checkFileExists",
		"checkFileExists",
		1,
		system_check_file_param,
		cfunc_System_checkFileExists)) {
		return false;
	}

	/* Registers System.pcall(). */
	if (!register_system_function(
		env,
		&dict,
		"System.pcall",
		"pcall",
		3,
		system_pcall_param,
		cfunc_System_pcall)) {
		return false;
	}

	/* Registers System.error(). */
	if (!register_system_function(
		env,
		&dict,
		"System.error",
		"error",
		1,
		system_error_param,
		cfunc_System_error)) {
		return false;
	}

	/* Reports successful System API registration. */
	return true;
}

/* Registers one System function and publishes it in the package. */
static bool
register_system_function(
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

	/* Publishes the function in the System dictionary. */
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

/* Implements System.import(). */
static bool
cfunc_System_import(
	NoctEnv *env)
{
	NoctValue temporary;
	const char *file_name;
	char *data;
	size_t size;

	/* Initializes the owned source buffer. */
	data = NULL;

	/* Reads the source file name. */
	if (!noct_get_arg_check_string(env, 0, &temporary, &file_name))
		return false;

	/* Loads the complete source file. */
	if (!system_load_file(env, file_name, &data, &size))
		return false;

	/* Rejects byte-oriented files containing embedded NUL bytes. */
	if (memchr(data, '\0', size) != NULL) {
		noct_error(env, N_TR("System.import() accepts source files only."));
		noct_free(data);
		return false;
	}

	/* Compiles and registers the loaded source. */
	if (!noct_register_source(env, file_name, data)) {
		noct_free(data);
		return false;
	}

	/* Releases the source buffer after registration. */
	noct_free(data);

	/* Reports successful source registration. */
	return true;
}

/* Loads one complete file into an owned buffer. */
static bool
system_load_file(
	NoctEnv *env,
	const char *file_name,
	char **data,
	size_t *size)
{
	FILE *stream;
	long file_size;
	size_t read_size;
	bool succeeded;

	/* Initializes the output ownership state. */
	*data = NULL;
	*size = 0;

	/* Opens the requested file. */
	stream = fopen(file_name, "rb");
	if (stream == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), file_name);
		return false;
	}

	/* Seeks to the end of the file. */
	if (fseek(stream, 0, SEEK_END) != 0) {
		succeeded = system_report_read_error(
			env,
			file_name,
			stream,
			data,
			size);
		return succeeded;
	}

	/* Reads the file extent. */
	file_size = ftell(stream);
	if (file_size < 0) {
		succeeded = system_report_read_error(
			env,
			file_name,
			stream,
			data,
			size);
		return succeeded;
	}

	/* Returns the stream to the beginning of the file. */
	if (fseek(stream, 0, SEEK_SET) != 0) {
		succeeded = system_report_read_error(
			env,
			file_name,
			stream,
			data,
			size);
		return succeeded;
	}

	/* Validates the file extent in the host size type. */
	read_size = (size_t)file_size;
	if ((long)read_size != file_size || read_size == SIZE_MAX) {
		succeeded = system_report_read_error(
			env,
			file_name,
			stream,
			data,
			size);
		return succeeded;
	}

	/* Allocates the file buffer and its string terminator. */
	*data = noct_malloc(read_size + 1);
	if (*data == NULL) {
		noct_out_of_memory(env);
		succeeded = system_finish_file(
			stream,
			data,
			size,
			false);
		return succeeded;
	}

	/* Reads the complete file contents. */
	if (fread(*data, 1, read_size, stream) != read_size) {
		succeeded = system_report_read_error(
			env,
			file_name,
			stream,
			data,
			size);
		return succeeded;
	}

	/* Terminates and publishes the loaded bytes. */
	(*data)[read_size] = '\0';
	*size = read_size;

	/* Closes the successfully consumed stream. */
	succeeded = system_finish_file(stream, data, size, true);

	/* Reports whether loading and closing succeeded. */
	return succeeded;
}

/* Reports a file read error and releases the partial result. */
static bool
system_report_read_error(
	NoctEnv *env,
	const char *file_name,
	FILE *stream,
	char **data,
	size_t *size)
{
	bool succeeded;

	/* Records the failed file read. */
	noct_error(env, N_TR("Cannot read file %s.\n"), file_name);

	/* Closes the stream and discards any partial bytes. */
	succeeded = system_finish_file(stream, data, size, false);

	/* Reports the failed read operation. */
	return succeeded;
}

/* Closes a file and normalizes its output ownership. */
static bool
system_finish_file(
	FILE *stream,
	char **data,
	size_t *size,
	bool succeeded)
{
	int close_status;

	/* Closes the file exactly once. */
	close_status = fclose(stream);

	/* Converts a close failure into an overall failure. */
	if (close_status != 0)
		succeeded = false;

	/* Discards the output unless the complete operation succeeded. */
	if (!succeeded) {
		noct_free(*data);
		*data = NULL;
		*size = 0;
	}

	/* Reports the normalized operation status. */
	return succeeded;
}

/* Implements System.registerSource(). */
static bool
cfunc_System_registerSource(
	NoctEnv *env)
{
	NoctValue source;
	const char *source_string;

	/* Initializes the source root. */
	memset(&source, 0, sizeof(source));

	/* Pins the source value during compilation. */
	if (!noct_pin_local(env, 1, &source))
		return false;

	/* Reads the source text. */
	if (!noct_get_arg_check_string(env, 0, &source, &source_string)) {
		(void)noct_unpin_local(env, 1, &source);
		return false;
	}

	/* Compiles and registers the in-memory source. */
	if (!noct_register_source(env, "<registerSource>", source_string)) {
		(void)noct_unpin_local(env, 1, &source);
		return false;
	}

	/* Releases the source root after compilation. */
	(void)noct_unpin_local(env, 1, &source);

	/* Reports successful source registration. */
	return true;
}

/* Implements System.getEnv(). */
static bool
cfunc_System_getEnv(
	NoctEnv *env)
{
	NoctValue name;
	NoctValue result;
	const char *name_string;
	const char *value;
	bool succeeded;

	/* Pins the environment name and return value. */
	if (!noct_pin_local(env, 2, &name, &result))
		return false;

	/* Reads the environment variable name. */
	if (!noct_get_arg_check_string(env, 0, &name, &name_string)) {
		(void)noct_unpin_local(env, 2, &name, &result);
		return false;
	}

	/* Reads the process environment. */
	value = getenv(name_string);

	/* Substitutes the documented empty value when the name is unset. */
	if (value == NULL)
		value = "";

	/* Returns the selected environment value. */
	succeeded = noct_set_return_make_string(env, &result, value);

	/* Releases the rooted environment name and result. */
	(void)noct_unpin_local(env, 2, &name, &result);

	/* Reports whether the environment value was returned. */
	return succeeded;
}

/* Implements System.shell(). */
static bool
cfunc_System_shell(
	NoctEnv *env)
{
	NoctValue temporary;
	const char *command;
	int command_status;

	/* Reads the shell command. */
	if (!noct_get_arg_check_string(env, 0, &temporary, &command))
		return false;

	/* Runs the command through the host shell. */
	command_status = system(command);

	/* Returns the host command status. */
	if (!noct_set_return_make_int(env, &temporary, command_status))
		return false;

	/* Reports successful status conversion. */
	return true;
}

/* Implements System.runCommand(). */
static bool
cfunc_System_runCommand(
	NoctEnv *env)
{
	NoctValue temporary;
	const char *command;
	const char *work_directory;
	int wait_for_finish;
	int command_status;
#if defined(NOCT_TARGET_WINDOWS)
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;
	wchar_t command_line[1024];
	wchar_t wide_work_directory[1024];
	DWORD wait_result;
	DWORD exit_code;
	BOOL created;
#elif defined(NOCT_TARGET_POSIX)
	pid_t child;
	char *command_copy;
	char *argv[64];
	char *token;
	int i;
#endif

	/* Reads the command string. */
	if (!noct_get_arg_check_string(env, 0, &temporary, &command))
		return false;

	/* Reads the requested working directory. */
	if (!noct_get_arg_check_string(
		env,
		1,
		&temporary,
		&work_directory)) {
		return false;
	}

	/* Reads the wait policy. */
	if (!noct_get_arg_check_int(
		env,
		2,
		&temporary,
		&wait_for_finish)) {
		return false;
	}

	/* Initializes the cross-platform command status. */
	command_status = 0;

#if defined(NOCT_TARGET_WINDOWS)
	/* Converts the command and working directory to UTF-16. */
	(void)MultiByteToWideChar(
		CP_UTF8,
		0,
		command,
		-1,
		command_line,
		(int)(sizeof(command_line) / sizeof(wchar_t) - 1));
	(void)MultiByteToWideChar(
		CP_UTF8,
		0,
		work_directory,
		-1,
		wide_work_directory,
		(int)(sizeof(wide_work_directory) / sizeof(wchar_t) - 1));

	/* Initializes the Windows process descriptors. */
	ZeroMemory(&startup_info, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);
	ZeroMemory(&process_info, sizeof(process_info));

	/* Creates the detached command process. */
	created = CreateProcessW(
		NULL,
		command_line,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS |
			CREATE_NEW_PROCESS_GROUP |
			CREATE_NO_WINDOW,
		NULL,
		wide_work_directory,
		&startup_info,
		&process_info);

	/* Records a Windows process creation error. */
	if (!created)
		noct_error(env, N_TR("CreateProcess() failed."));

	/* Collects the requested process status. */
	if (process_info.hProcess != NULL) {
		command_status = 0;

		/* Waits for completion when requested. */
		if (wait_for_finish) {
			wait_result = WaitForSingleObject(
				process_info.hProcess,
				INFINITE);

			/* Reads a normally completed process status. */
			if (wait_result == WAIT_OBJECT_0) {
				exit_code = 0;
				(void)GetExitCodeProcess(
					process_info.hProcess,
					&exit_code);
				command_status = (int)exit_code;
			}
		}
	} else {
		command_status = 1;
	}

	/* Releases the created thread handle. */
	if (process_info.hThread != NULL)
		(void)CloseHandle(process_info.hThread);

	/* Releases the created process handle. */
	if (process_info.hProcess != NULL)
		(void)CloseHandle(process_info.hProcess);
#elif defined(NOCT_TARGET_POSIX)
	/* Creates the command process. */
	child = fork();

	/* Reports a failed process creation. */
	if (child < 0) {
		noct_error(env, N_TR("fork() failed."));
		return false;
	}

	/* Prepares and executes the command in the child. */
	if (child == 0) {
		/* Changes to a non-empty requested working directory. */
		if (strcmp(work_directory, "") != 0) {
			/* Reports a failed directory change in the child. */
			if (chdir(work_directory) != 0) {
				printf(N_TR("chdir() failed.\n"));
				return EXIT_FAILURE;
			}
		}

		/* Copies the command for the destructive token parser. */
		command_copy = noct_strdup(command);

		/* Splits the command on spaces into the fixed argument vector. */
		i = 0;
		token = strtok(command_copy, " ");
		while (token != NULL && i < 63) {
			argv[i] = token;
			i++;
			token = strtok(NULL, " ");
		}
		argv[i] = NULL;

		/* Replaces the child image with the requested command. */
		execvp(argv[0], argv);

		/* Reports an exec failure in the child process. */
		printf(N_TR("execvp() failed for %s.\n"), argv[0]);
		noct_free(command_copy);

		/* Preserves the existing child failure result. */
		return EXIT_FAILURE;
	}

	/* Waits for the child when requested. */
	if (wait_for_finish)
		(void)waitpid(child, &command_status, 0);
#endif

	/* Returns the platform command status. */
	if (!noct_set_return_make_int(env, &temporary, command_status))
		return false;

	/* Reports successful command execution. */
	return true;
}

/* Implements System.getOSName(). */
static bool
cfunc_System_getOSName(
	NoctEnv *env)
{
	NoctValue temporary;
	const char *name;

	/* Selects the configured operating-system name. */
#if defined(NOCT_TARGET_WINDOWS)
	name = "windows";
#elif defined(NOCT_TARGET_MACOS)
	name = "macos";
#elif defined(NOCT_TARGET_ZEDBSD)
	name = "zedbsd";
#elif defined(NOCT_TARGET_LINUX)
	name = "linux";
#elif defined(NOCT_TARGET_IOS)
	name = "ios";
#elif defined(NOCT_TARGET_ANDROID)
	name = "android";
#elif defined(NOCT_TARGET_WASM)
	name = "wasm";
#elif defined(NOCT_TARGET_UNITY)
	name = "unity";
#elif defined(NOCT_TARGET_FREEBSD)
	name = "freebsd";
#elif defined(NOCT_TARGET_NETBSD)
	name = "netbsd";
#elif defined(NOCT_TARGET_OPENBSD)
	name = "openbsd";
#elif defined(NOCT_TARGET_SOLARIS10)
	name = "sunos";
#elif defined(NOCT_TARGET_SOLARIS11)
	name = "sunos";
#elif defined(NOCT_TARGET_BEOS)
	name = "haiku";
#else
	name = "unknown";
#endif

	/* Returns the selected operating-system name. */
	if (!noct_set_return_make_string(env, &temporary, name))
		return false;

	/* Reports successful name conversion. */
	return true;
}

/* Implements System.checkFileExists(). */
static bool
cfunc_System_checkFileExists(
	NoctEnv *env)
{
	NoctValue temporary;
	const char *file_name;
	int exists;

	/* Reads the requested file name. */
	if (!noct_get_arg_check_string(env, 0, &temporary, &file_name))
		return false;

	/* Tests the file through the platform access interface. */
	exists = 0;
	if (access(file_name, 0) == 0)
		exists = 1;

	/* Returns the normalized existence flag. */
	if (!noct_set_return_make_int(env, &temporary, exists))
		return false;

	/* Reports successful existence testing. */
	return true;
}

/* Implements System.pcall(). */
static bool
cfunc_System_pcall(
	NoctEnv *env)
{
	NoctValue function_value;
	NoctValue argument_a;
	NoctValue argument_b;
	NoctValue result;
	NoctValue return_value;
	NoctValue temporary;
	NoctFunc *function;
	NoctValue arguments[2];
	const char *message;
	bool call_succeeded;

	/* Initializes every value before it becomes a GC root. */
	memset(&function_value, 0, sizeof(function_value));
	memset(&argument_a, 0, sizeof(argument_a));
	memset(&argument_b, 0, sizeof(argument_b));
	memset(&result, 0, sizeof(result));
	memset(&return_value, 0, sizeof(return_value));
	memset(&temporary, 0, sizeof(temporary));

	/* Pins the call arguments, result dictionary, and temporaries. */
	if (!noct_pin_local(
		env,
		6,
		&function_value,
		&argument_a,
		&argument_b,
		&result,
		&return_value,
		&temporary)) {
		return false;
	}

	/* Reads the protected function. */
	if (!noct_get_arg_check_func(env, 0, &function_value, &function)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Reads the first protected argument. */
	if (!noct_get_arg(env, 1, &argument_a)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Reads the second protected argument. */
	if (!noct_get_arg(env, 2, &argument_b)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Prepares the protected argument vector. */
	arguments[0] = argument_a;
	arguments[1] = argument_b;

	/* Calls the function while retaining either outcome. */
	call_succeeded = noct_call(
		env,
		function,
		2,
		arguments,
		&return_value);

	/* Creates the protected-call result dictionary. */
	if (!noct_make_empty_dict(env, &result)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Publishes the protected-call status. */
	if (!noct_set_dict_elem_make_int(
		env,
		&result,
		"ok",
		&temporary,
		call_succeeded ? 1 : 0)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Publishes either the returned value or the captured error. */
	if (call_succeeded) {
		/* Stores the successful function result. */
		if (!noct_set_dict_elem_cstr(
			env,
			&result,
			"value",
			&return_value)) {
			system_unpin_pcall(
				env,
				&function_value,
				&argument_a,
				&argument_b,
				&result,
				&return_value,
				&temporary);
			return false;
		}
	} else {
		/* Reads the captured runtime error message. */
		if (!noct_get_error_message(env, &message)) {
			system_unpin_pcall(
				env,
				&function_value,
				&argument_a,
				&argument_b,
				&result,
				&return_value,
				&temporary);
			return false;
		}

		/* Substitutes a defensive fallback message. */
		if (message == NULL)
			message = "?";

		/* Stores the captured runtime error. */
		if (!noct_set_dict_elem_make_string(
			env,
			&result,
			"message",
			&temporary,
			message)) {
			system_unpin_pcall(
				env,
				&function_value,
				&argument_a,
				&argument_b,
				&result,
				&return_value,
				&temporary);
			return false;
		}
	}

	/* Returns the completed protected-call dictionary. */
	if (!noct_set_return(env, &result)) {
		system_unpin_pcall(
			env,
			&function_value,
			&argument_a,
			&argument_b,
			&result,
			&return_value,
			&temporary);
		return false;
	}

	/* Releases all protected-call roots. */
	system_unpin_pcall(
		env,
		&function_value,
		&argument_a,
		&argument_b,
		&result,
		&return_value,
		&temporary);

	/* Reports successful protected-call packaging. */
	return true;
}

/* Releases every local root used by System.pcall(). */
static void
system_unpin_pcall(
	NoctEnv *env,
	NoctValue *function,
	NoctValue *argument_a,
	NoctValue *argument_b,
	NoctValue *result,
	NoctValue *return_value,
	NoctValue *temporary)
{
	/* Releases the roots in their original argument order. */
	(void)noct_unpin_local(
		env,
		6,
		function,
		argument_a,
		argument_b,
		result,
		return_value,
		temporary);
}

/* Implements System.error(). */
static bool
cfunc_System_error(
	NoctEnv *env)
{
	NoctValue message;
	const char *text;

	/* Pins the caller-supplied message. */
	if (!noct_pin_local(env, 1, &message))
		return false;

	/* Reads the error message text. */
	if (!noct_get_arg_check_string(env, 0, &message, &text)) {
		(void)noct_unpin_local(env, 1, &message);
		return false;
	}

	/* Raises the requested runtime error. */
	noct_error(env, N_TR("%s"), text);

	/* Releases the message root after copying its text. */
	(void)noct_unpin_local(env, 1, &message);

	/* Propagates the requested runtime error. */
	return false;
}
