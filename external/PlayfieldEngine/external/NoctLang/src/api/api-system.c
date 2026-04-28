/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * API: System.*
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NOCT_TARGET_WINDOWS
#include <windows.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#endif

/* Forward declaration. */
static bool cfunc_System_import(NoctEnv *env);
static bool cfunc_System_shell(NoctEnv *env);
static bool cfunc_System_runCommand(NoctEnv *env);
static bool cfunc_System_getOSName(NoctEnv *env);
static bool cfunc_System_checkFileExists(NoctEnv *env);
static bool system_load_file(NoctEnv *env, const char *fname, char **data, size_t *size);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *field_name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	{
		"__System_import",
		"import",
		1,
		{"file"},
		cfunc_System_import
	},
	{
		"__System_shell",
		"shell",
		1,
		{"command"},
		cfunc_System_shell
	},
	{
		"__System_runCommand",
		"runCommand",
		3,
		{"command", "workDir", "waitForFinish"},
		cfunc_System_runCommand
	},
	{
		"__System_getOSName",
		"getOSName",
		0,
		{NULL},
		cfunc_System_getOSName
	},
	{
		"__System_checkFileExists",
		"checkFileExists",
		1,
		{"file"},
		cfunc_System_checkFileExists
	},
};

/*
 * Register "System.*" functions.
 */
bool noct_register_api_system(NoctEnv *env)
{
	NoctValue dict;
	int i;

	/* Make a global variable "System". */
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "System", &dict))
		return false;

	/* Register functions. */
	for (i = 0; i < sizeof(ffi_items) / sizeof(struct ffi_item); i++) {
		NoctValue funcval;

		/* Register a cfunc. */
		if (!noct_register_cfunc(
			    env,
			    ffi_items[i].global_name,
			    ffi_items[i].param_count,
			    ffi_items[i].param,
			    ffi_items[i].cfunc,
			    NULL))
			return false;

		/* Get a function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Make a dictionary element. */
		if (!noct_set_dict_elem(
			    env,
			    &dict,
			    ffi_items[i].field_name,
			    &funcval))
			return false;
	}

	return true;
}

/* Implementation of import() */
static bool
cfunc_System_import(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *file;
	char *data;
	size_t len;

	if (!noct_get_arg_check_string(env, 0, &tmp, &file))
		return false;

	/* Load a source file content. */
	if (!system_load_file(env, file, &data, &len))
		return false;

	/* Check for the bytecode header. */
	if (strncmp(data, NOCT_BYTECODE_HEADER, strlen(NOCT_BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, file, data))
			return false;
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, len))
			return false;
	}

	return true;
}

/* Implementation of runCommand() */
static bool
cfunc_System_runCommand(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *command;
	const char *work_dir;
	int wait_for_finish;
	int ret;

	if (!noct_get_arg_check_string(env, 0, &tmp, &command))
		return false;
	if (!noct_get_arg_check_string(env, 1, &tmp, &work_dir))
		return false;
	if (!noct_get_arg_check_int(env, 2, &tmp, &wait_for_finish))
		return false;

#if defined(NOCT_TARGET_WINDOWS)
	{
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;
		wchar_t wszCmdLine[1024];
		wchar_t wszWorkDir[1024];

		MultiByteToWideChar(CP_UTF8, 0, command, -1, wszCmdLine, sizeof(wszCmdLine) / sizeof(wchar_t) - 1);
		MultiByteToWideChar(CP_UTF8, 0, work_dir, -1, wszWorkDir, sizeof(wszWorkDir) / sizeof(wchar_t) - 1);

		ZeroMemory(&si, sizeof(STARTUPINFOW));
		si.cb = sizeof(STARTUPINFOW);
		CreateProcessW(NULL,		/* lpApplication */
			       wszCmdLine,	/* lpCommandLine */
			       NULL,		/* lpProcessAttribute */
			       NULL,		/* lpThreadAttributes */
			       FALSE,		/* bInheritHandles */
			       NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
			       NULL,		/* lpEnvironment */
			       wszWorkDir,
			       &si,
			       &pi);
		if (pi.hProcess != NULL) {
			ret = 0;
			if (wait_for_finish) {
				DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);
				if (waitResult == WAIT_OBJECT_0) {
					DWORD exitCode = 0;
					GetExitCodeProcess(pi.hProcess, &exitCode);
					ret = exitCode;
				}
			}
		} else {
			ret = 1;
		}
		if (pi.hThread != NULL)
			CloseHandle(pi.hThread);
		if (pi.hProcess != NULL)
			CloseHandle(pi.hProcess);
	}
#else
	{
		pid_t pid = fork();
		if (pid < 0) {
			noct_error(env, "fork() failed.");
			return false;
		}
		if (pid == 0) {
			char *cmd_copy;
			char *argv[64];
			char *token;
			int i;
			
			/* Set the working directory. */
			if (chdir(work_dir) != 0) {
				printf("chdir() failed.\n");
				_exit(EXIT_FAILURE);
			}

			/* Parse the command line arguments. TODO: support quotation */
			cmd_copy = strdup(command);
			i = 0;
			token = strtok(cmd_copy, " ");
			while (token != NULL && i < 63) {
				argv[i++] = token;
				token = strtok(NULL, " ");
			}
			argv[i] = NULL;

			/* Run. */
			execvp(argv[0], argv);

			printf("execvp() failed.\n");
			free(cmd_copy);
			_exit(EXIT_FAILURE);
		} else {
			if (wait_for_finish)
				waitpid(pid, &ret, 0);
		}
	}
#endif

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &tmp, ret))
		return false;

	return true;
}

/* Implementation of shell() */
static bool
cfunc_System_shell(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *s;
	int cmd_ret;

	/* Get the "command" parameer. */
	if (!noct_get_arg_check_string(env, 0, &tmp, &s))
		return false;

	/* Run a command. */
	cmd_ret = system(s);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &tmp, cmd_ret))
		return false;

	return true;
}

/* Implementation of getOSName() */
static bool
cfunc_System_getOSName(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *name;

#if defined(NOCT_TARGET_WINDOWS)
	name = "windows";
#elif defined(NOCT_TARGET_MACOS)
	name = "macos";
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

	/* Make a return value. */
	if (!noct_set_return_make_string(env, &tmp, name))
		return false;

	return true;
}

/* Implementation of checkFileExists() */
static bool
cfunc_System_checkFileExists(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *s;
	int ret;

	/* Get the "file" parameer. */
	if (!noct_get_arg_check_string(env, 0, &tmp, &s))
		return false;

	/* Run a command. */
	ret = 0;
	if (access(s, 0) == 0)
		ret = 1;

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &tmp, ret))
		return false;

	return true;
}

/* Load a file. */
static bool system_load_file(NoctEnv *env, const char *fname, char **data, size_t *size)
{
	FILE *fp;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	*data = malloc(*size + 1);
	if (*data == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Read the data. */
	if (fread(*data, 1, *size, fp) != *size) {
		noct_error(env, N_TR("Cannot read file %s.\n"), fname);
		return false;
	}

	/* Terminate the string. */
	(*data)[*size] = '\0';

	fclose(fp);

	return true;
}
