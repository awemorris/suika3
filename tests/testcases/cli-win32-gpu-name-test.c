/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "cli-win32.h"

#include <windows.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define TEST_GPU_ARGUMENT L" --gpu=\u65e5\u672c\u8a9e --child"
#define TEST_GPU_UTF8 "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e"

static bool test_child(int argc, char *argv[]);
static bool test_parent(void);
static wchar_t *get_executable_path(void);
static wchar_t *make_child_command(const wchar_t *executable_path);

/*
 * Tests lossless Windows GPU-name conversion.
 */
int
main(
	int argc,
	char *argv[])
{
	if (argc == 3 && strcmp(argv[2], "--child") == 0) {
		if (!test_child(argc, argv))
			return 1;

		return 0;
	}

	if (argc != 1) {
		fprintf(stderr, "Unexpected test arguments.\n");
		return 1;
	}

	if (!test_parent())
		return 1;

	return 0;
}

/* Run the conversion inside the child process. */
static bool
test_child(
	int argc,
	char *argv[])
{
	char *name;
	bool succeeded;

	UNUSED_PARAMETER(argv);

	name = NULL;
	succeeded = cli_windows_gpu_name_utf8(argc, 1, &name);
	if (!succeeded) {
		fprintf(stderr, "GPU-name conversion failed.\n");
		return false;
	}

	if (strcmp(name, TEST_GPU_UTF8) != 0) {
		fprintf(stderr, "GPU-name UTF-8 bytes differ.\n");
		free(name);
		return false;
	}

	free(name);

	return true;
}

/* Start a child with a non-ASCII wide command line. */
static bool
test_parent(
	void)
{
	STARTUPINFOW startup;
	PROCESS_INFORMATION process;
	wchar_t *executable_path;
	wchar_t *command;
	DWORD wait_result;
	DWORD exit_code;
	BOOL created;
	BOOL queried;
	bool succeeded;

	executable_path = get_executable_path();
	if (executable_path == NULL)
		return false;

	command = make_child_command(executable_path);
	free(executable_path);
	if (command == NULL)
		return false;

	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	created = CreateProcessW(
		NULL,
		command,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&startup,
		&process);
	free(command);
	if (!created) {
		fprintf(stderr, "CreateProcessW failed.\n");
		return false;
	}

	succeeded = false;
	wait_result = WaitForSingleObject(process.hProcess, INFINITE);
	if (wait_result != WAIT_OBJECT_0) {
		fprintf(stderr, "Waiting for child failed.\n");
		goto cleanup;
	}

	exit_code = 1;
	queried = GetExitCodeProcess(process.hProcess, &exit_code);
	if (!queried) {
		fprintf(stderr, "Getting child exit code failed.\n");
		goto cleanup;
	}
	if (exit_code != 0) {
		fprintf(stderr, "Child conversion test failed.\n");
		goto cleanup;
	}

	succeeded = true;

cleanup:
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	return succeeded;
}

/* Get an owned absolute path to this executable. */
static wchar_t *
get_executable_path(
	void)
{
	wchar_t *path;
	wchar_t *new_path;
	size_t allocation_size;
	DWORD capacity;
	DWORD length;

	path = NULL;
	capacity = 256;

	/* Grow the buffer until the complete path fits. */
	for (;;) {
		allocation_size = (size_t)capacity * sizeof(*path);
		if (capacity != 0 &&
		    allocation_size / sizeof(*path) != (size_t)capacity) {
			free(path);
			return NULL;
		}
		new_path = realloc(path, allocation_size);
		if (new_path == NULL) {
			free(path);
			return NULL;
		}
		path = new_path;

		length = GetModuleFileNameW(NULL, path, capacity);
		if (length == 0) {
			free(path);
			return NULL;
		}
		if (length < capacity - 1)
			break;
		if (capacity > (MAXDWORD / 2)) {
			free(path);
			return NULL;
		}
		capacity *= 2;
	}

	return path;
}

/* Build an owned wide child-process command line. */
static wchar_t *
make_child_command(
	const wchar_t *executable_path)
{
	wchar_t *command;
	size_t path_length;
	size_t argument_length;
	size_t command_length;
	size_t offset;

	path_length = wcslen(executable_path);
	argument_length = wcslen(TEST_GPU_ARGUMENT);
	if (path_length > SIZE_MAX - argument_length - 4)
		return NULL;
	command_length = path_length + argument_length + 3;
	if (command_length > SIZE_MAX / sizeof(*command))
		return NULL;

	command = malloc(command_length * sizeof(*command));
	if (command == NULL)
		return NULL;

	offset = 0;
	command[offset++] = L'"';
	memcpy(&command[offset], executable_path,
	       path_length * sizeof(*command));
	offset += path_length;
	command[offset++] = L'"';
	memcpy(&command[offset], TEST_GPU_ARGUMENT,
	       (argument_length + 1) * sizeof(*command));

	return command;
}
