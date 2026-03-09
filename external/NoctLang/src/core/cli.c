/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

#include <noct/noct.h>
#include "runtime.h"
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "cback.h"
#include "elback.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

/* Bytecode File Header */
#define BYTECODE_HEADER		"Noct Bytecode"

/* i18n.c */
#if defined(NOCT_USE_TRANSLATION)
void noct_init_locale(void);
#endif

/* Forward declaration. */
static void show_usage(void);
static int command_compile(int argc, char *argv[]);
static int command_transpile_c(int argc, char *argv[]);
static int command_transpile_elisp(int argc, char *argv[]);
static int command_run(int argc, char *argv[]);
static int command_repl(void);
static bool compile_source(const char *file_name);
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file_hook_c(const char *fname);
static bool do_transpile_elisp(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file_hook_elisp(const char *fname);
static bool add_file(const char *fname, bool (*add_file_hook)(const char *));
static bool is_multiline_start(const char *text, bool *is_func);
static bool accept_multiline(const char *text);
static bool load_file_content(const char *fname, char **data, size_t *size);
static int wide_printf(const char *format, ...);
static bool register_ffi(NoctEnv *env);
static bool serialize_printer(NoctEnv *env, char *buf, size_t size, NoctValue *value, bool is_inside_obj);

/*
 * Main
 */

#include "hash.h"

int main(int argc, char *argv[])
{
	char *first_arg;

#if defined(NOCT_USE_TRANSLATION)
	noct_init_locale();
#endif

	if (argc >= 2 &&
	    (strcmp(argv[1], "--help") == 0 ||
	     strcmp(argv[1], "-h") == 0)) {
		show_usage();
		return 1;
	}

	if (argc == 1)
		return command_repl();

	/* Command */
	first_arg = argv[1];
	if (strcmp(first_arg, "--compile") == 0)
		return command_compile(argc, argv);
	if (strcmp(first_arg, "--ansic") == 0)
		return command_transpile_c(argc, argv);
	if (strcmp(first_arg, "--elisp") == 0)
		return command_transpile_elisp(argc, argv);

	/* Run */
	return command_run(argc, argv);
}

/* Show the usage message. */
static void show_usage(void)
{
	wide_printf(N_TR("Noct Programming Language\n"));
	wide_printf(N_TR("Usage\n"));
	wide_printf(N_TR("  noct <file>                        ... run a program\n"));
	wide_printf(N_TR("  noct --compile <files>             ... convert to bytecode files\n"));
	wide_printf(N_TR("  noct --ansic <out file> <in files> ... convert to a C source file\n"));
	wide_printf(N_TR("  noct --elisp <out file> <in files> ... convert to an Emacs Lisp source file\n"));
}

/*
 * Run
 */

/* Top level function for the run mode. */
static int command_run(int argc, char *argv[])
{
	NoctVM *vm;
	NoctEnv *env;
	NoctValue ret;
	int file_arg;
	int i;
	char *data;
	size_t len;
	int arg_count;
	NoctValue *arg_value;

	/* Parse options. */
	file_arg = 1;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[1], "--disable-jit") == 0) {
			noct_conf_jit_enable = false;
			file_arg++;
			continue;
		}
		if (strcmp(argv[1], "--force-jit") == 0) {
			noct_conf_jit_threshold = 0;
			file_arg++;
			continue;
		}

		wide_printf(N_TR("Unknown option %s.\n"), argv[1]);
		return 1;
	}

	/* Check if a file is specified. */
	if (file_arg == argc) {
		/* No file specified, enter REPL. */
		if (argc == 1)
			return command_repl();
		return 1;
	}

	/* Create a runtime. */
	if (!noct_create_vm(&vm, &env))
		return 1;

	/* Register FFI functions. */
	if (!register_ffi(env))
		return 1;

	/* Load a file content. */
	if (!load_file_content(argv[i], &data, &len))
		return 1;

	/* Check for the bytecode header. */
	if (strncmp(data, BYTECODE_HEADER, strlen(BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, argv[i], data)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return 1;
		}
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, len)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return 1;
		}
	}

	/* Make the arguments for "main()". */
	arg_count = argc - file_arg - 1;
	if (arg_count > 0) {
		arg_value = malloc(sizeof(NoctValue) * arg_count);
		if (arg_value == NULL)
			return 1;
		for (i = 0; i < arg_count; i++) {
			if (!noct_make_string(env, &arg_value[i], argv[file_arg + i + 1]))
				return 1;
		}
	} else {
		arg_value = NULL;
	}

	/* Run the "main()" function. */
	if (!noct_enter_vm(env, "main", arg_count, arg_value, &ret)) {
		const char *file, *msg;
		int line;
		noct_get_error_file(env, &file);
		noct_get_error_line(env, &line);
		noct_get_error_message(env, &msg);
		wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
		return 1;
	}

	/* Destroy the runtime. */
	if (!noct_destroy_vm(vm))
		return 1;

	return 0;
}

/*
 * Compile
 */

/* The top level function for the compile mode. */
static int command_compile(int argc, char *argv[])
{
	int i;

	/* For each argument file. */
	for (i = 2; i < argc; i++) {
		/* Compile a source to bytecode. */
		if (!compile_source(argv[i]))
			return 1;
	}

	return 1;
}

/* Compile a source file. */
static bool compile_source(const char *file_name)
{
	char bc_fname[1024];
	FILE *fp;
	char *source_data, *dot;
	size_t source_length;
	int func_count, i, j;

	/* Load an argument source file. */
	if (!load_file_content(file_name, &source_data, &source_length))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(file_name, source_data)) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		return false;
	}

	/* Make an output file name. (*.nb) */
	strcpy(bc_fname, file_name);
	dot = strstr(bc_fname, ".");
	if (dot != NULL)
		strcpy(dot, ".nb");
	else
		strcat(bc_fname, ".nb");

	/* Open an output .nb bytecode file. */
	fp = fopen(bc_fname, "wb");
	if (fp == NULL) {
		wide_printf(N_TR("Cannot open file %s.\n"), bc_fname);
		return false;
	}

	/* Put a file header. */
	func_count = hir_get_function_count();
	fprintf(fp, "Noct Bytecode 1.0\n");
	fprintf(fp, "Source\n");
	fprintf(fp, "%s\n", file_name);
	fprintf(fp, "Number Of Functions\n");
	fprintf(fp, "%d\n", func_count);

	/* For each HIR function. */
	for (i = 0; i < func_count; i++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(i);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(N_TR("Error: %s: %d: %s\n"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			return false;
		}

		/* Put a bytcode. */
		fprintf(fp, "Begin Function\n");
		fprintf(fp, "Name\n");
		fprintf(fp, "%s\n", lfunc->func_name);
		fprintf(fp, "Parameters\n");
		fprintf(fp, "%d\n", lfunc->param_count);
		for (j = 0; j < lfunc->param_count; j++)
			fprintf(fp, "%s\n", lfunc->param_name[j]);
		fprintf(fp, "Temporary Size\n");
		fprintf(fp, "%d\n", lfunc->tmpvar_size);
		fprintf(fp, "Bytecode Size\n");
		fprintf(fp, "%d\n", lfunc->bytecode_size);
		fwrite(lfunc->bytecode, (size_t)lfunc->bytecode_size, 1, fp);
		fprintf(fp, "\nEnd Function\n");

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	fclose(fp);

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * C Translation
 */

/* The top level function for the C translation mode. */
static int command_transpile_c(int argc, char *argv[])
{
	if (argc < 4) {
		show_usage();
		return 1;
	}

	if (!do_transpile_c(argv[2], argc - 3, (const char **)&argv[3]))
		return 1;

	return 0;
}

/* Do C translation. */
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!cback_init(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		/* Recursively add files. */
		if (!add_file(in_file[i], add_file_hook_c))
			return false;
	}

	/* Put a epilogue code. */
	if (!cback_finalize_dll())
		return false;

	return true;
}

/* "On file add" callback for the recursive file search. */
static bool add_file_hook_c(const char *fname)
{
	char *data;
	size_t len;
	int func_count, j;

	/* Load an argument file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(fname, data)) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		return false;
	}

	/* For each HIR function. */
	func_count = hir_get_function_count();
	for (j = 0; j < func_count; j++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(j);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(N_TR("Error: %s: %d: %s\n"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			return false;;
		}

		/* Put a C function. */
		if (!cback_translate_func(lfunc))
			return false;

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	/* Free intermediated. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * Emacs Lisp Translation
 */

/* The top level function for the Emacs Lisp translation. */
static int command_transpile_elisp(int argc, char *argv[])
{
	if (argc < 4) {
		show_usage();
		return 1;
	}

	if (!do_transpile_elisp(argv[2], argc - 3, (const char **)&argv[3]))
		return 1;

	return 0;
}

/* Do Emacs Lisp translation. */
static bool do_transpile_elisp(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!elback_init(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		if (!add_file(in_file[i], add_file_hook_elisp))
			return false;
	}

	/* Put a epilogue code. */
	if (!elback_finalize())
		return false;

	return true;
}

/* "On file add" callback for Emacs Lisp translation. */
static bool add_file_hook_elisp(const char *fname)
{
	char *data;
	size_t len;
	int func_count, j;

	/* Load a file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(fname, data)) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		return false;
	}

	/* For each HIR function. */
	func_count = hir_get_function_count();
	for (j = 0; j < func_count; j++) {
		struct hir_block *hfunc;

		/* Put Emacs Lisp. */
		hfunc = hir_get_function(j);
		if (!elback_translate_func(hfunc))
			return false;
	}

	/* Free entire HIR. */
	hir_cleanup();

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * REPL
 */

int command_repl(void)
{
	NoctVM *vm;
	NoctEnv *env;

	/* Turn off JIT. */
	/* noct_conf_use_jit = false; */

	/* Create a runtime. */
	if (!noct_create_vm(&vm, &env))
		return 1;

	/* Register FFI functions. */
	if (!register_ffi(env))
		return 1;

	wide_printf(N_TR("Noct Programming Language\n"));
	wide_printf(N_TR("Entering REPL mode.\n"));
	wide_printf("\n");

	/* Prompt. */
	while (1) {
		char line[4096];
		char entire[32768];
		char *start;
		bool is_func;
		NoctValue ret, zero;

		memset(line, 0, sizeof(line));
		memset(entire, 0, sizeof(entire));

		/* Show the prompt and get an input. */
		wide_printf("> ");
		if (fgets(line, sizeof(line) - 1, stdin) == NULL)
			break;

		/* Check if it is multiple lines. */
		if (!is_multiline_start(line, &is_func)) {
			/* Single line. */

			/* Make a function. */
			strncpy(entire, "func repl() {", sizeof(entire) - 1);
			strncat(entire, line, sizeof(entire) - 1);
			strncat(entire, "; }", sizeof(entire) - 1);

			/* Compile the source. */
			if (!noct_register_source(env, "REPL", entire)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}

			/* Run the "repl()" function. */
			if (!noct_enter_vm(env, "repl", 0, NULL, &ret)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}
		} else {
			/* Multiple lines. */

			/* Make a function if the block is not a function. */
			if (!is_func)
				strncpy(entire, "func repl() {", sizeof(entire) - 1);
			else
				strcpy(entire, "");
			start = &entire[strlen(entire)];

			/* Show the multiline prompt and get inputs until the block ends. */
			strncat(entire, line, sizeof(entire) - 1);
			while (!accept_multiline(start)) {
				wide_printf(". ");
				if (fgets(line, sizeof(line) - 1, stdin) == NULL)
					break;
				strncat(entire, line, sizeof(entire) - 1);
			}

			/* Terminate the synthetic function if the block is not a function. */
			if (!is_func)
				strncat(entire, "}", sizeof(entire) - 1);

			/* Compile the source. */
			if (!noct_register_source(env, "REPL", entire)) {
				const char *file, *msg;
				int line;
				noct_get_error_file(env, &file);
				noct_get_error_line(env, &line);
				noct_get_error_message(env, &msg);
				wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
				continue;
			}

			/* If the block is not a function, run the synthetic function. */
			if (!is_func) {
				/* Run the "repl()" function. */
				if (!noct_enter_vm(env, "repl", 0, NULL, &ret)) {
					const char *file, *msg;
					int line;
					noct_get_error_file(env, &file);
					noct_get_error_line(env, &line);
					noct_get_error_message(env, &msg);
					wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
					continue;
				}
			}
		}

		/* Make the "repl()" function updatable. */
		noct_make_int(env, &zero, 0);
		if (!noct_set_global(env, "repl", &zero))
			return 1;
	}

	/* Destroy the runtime. */
	if (!noct_destroy_vm(vm))
		return 1;

	return 0;
}

/* Check for the start of the multiple line mode. */
static bool is_multiline_start(const char *text, bool *is_func)
{
	const char *s;

	s = text;
	while (*s == ' ')
		s++;

	*is_func = false;
	if (strncmp(s, "func", 4) == 0) {
		if (s[4] == ' ' || s[4] == '\t' || s[4] == '\n' || s[4] == '(') {
			*is_func = true;
			return true;
		}
	}
	if (strncmp(s, "if", 2) == 0) {
		if (s[2] == ' ' || s[2] == '\t' || s[2] == '\n' || s[2] == '(')
			return true;
	}
	if (strncmp(s, "for", 2) == 0) {
		if (s[3] == ' ' || s[3] == '\t' || s[3] == '\n' || s[3] == '(')
			return true;
	}
	if (strncmp(s, "while", 5) == 0) {
		if (s[5] == ' ' || s[5] == '\t' || s[5] == '\n' || s[5] == '(')
			return true;
	}

	return false;
}

/* Check for the end of the multiple line mode. */
static bool accept_multiline(const char *text)
{
	int open, close;
	const char *s;

	open = 0;
	close = 0;
	s = text;
	while(*s) {
		if (*s == '{')
			open++;
		else if (*s == '}')
			close++;
		s++;
	}

	if (open > 0 && open == close)
		return true; /* Matched. */

	return false;
}

/*
 * Helpers
 */

/* Load a file. */
static bool load_file_content(const char *fname, char **data, size_t *size)
{
	FILE *fp;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		wide_printf(N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	*data = malloc(*size + 1);
	if (*data == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}

	/* Read the data. */
	if (fread(*data, 1, *size, fp) != *size) {
		wide_printf(N_TR("Cannot read file %s.\n"), fname);
		return false;
	}

	/* Terminate the string. */
	(*data)[*size] = '\0';

	fclose(fp);

	return true;
}

/* Print to console. (supports wide characters on Windows console.) */
static int wide_printf(const char *format, ...)
{
	static char buf[4096];
	va_list ap;
	int size;

	va_start(ap, format);
	size = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if !defined(_WIN32)
	return printf("%s", buf);
#else
	/* MSVC or MinGW: Use WriteConsoleW(). (Otherwise we can't write CJK.) */
	static wchar_t wbuf[4096];
	DWORD dwWritten;
	memset(wbuf, 0, sizeof(wbuf));
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, sizeof(wbuf) / sizeof(wchar_t));
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wbuf, lstrlenW(wbuf), &dwWritten, NULL);
	return size;
#endif
}

/*
 * For POSIX
 */
#if !defined(_WIN32)

/* Recursively add files. */
static bool add_file(const char *fname, bool (*add_file_hook)(const char *))
{
	struct stat st;

	if (stat(fname, &st) != 0) {
		printf(N_TR("Cannot find %s.\n"), fname);
		return false;
	}
	if (S_ISDIR(st.st_mode)) {
		struct dirent **names;
		int count;
		int i;

		printf(N_TR("Searching directory %s.\n"), fname);

		/* Get directory content. */
		count = scandir(fname, &names, NULL, alphasort);
		if (count < 0) {
			wide_printf(N_TR("Skipping empty directory %s.\n"), fname);
			return false;
		}

		for (i = 0; i < count; i++) {
			char next_path[256 + 1];

			if (strcmp(names[i]->d_name, ".") == 0)
				continue;
			if (strcmp(names[i]->d_name, "..") == 0)
				continue;

			snprintf(next_path, sizeof(next_path), "%s/%s", fname, names[i]->d_name);
			if (!add_file(next_path, add_file_hook))
				return false;
		}
	} else if(S_ISREG(st.st_mode)) {
		wide_printf(N_TR("Adding file %s.\n"), fname);
		if (!add_file_hook(fname))
			return false;
	}

	return true;
}

/*
 * For Windows
 */
#else

#define BUF_SIZE	1024

static wchar_t wszMessage[BUF_SIZE];
static char szMessage[BUF_SIZE];

/* Convert UTF-8 to UTF-16. */
static const wchar_t *win32_utf8_to_utf16(const char *utf8_message)
{
	assert(utf8_message != NULL);
	MultiByteToWideChar(CP_UTF8, 0, utf8_message, -1, wszMessage, BUF_SIZE - 1);
	return wszMessage;
}

/* Convert UTF-16 to UTF-8. */
static const char *win32_utf16_to_utf8(const wchar_t *utf16_message)
{
	assert(utf16_message != NULL);
	WideCharToMultiByte(CP_UTF8, 0, utf16_message, -1, szMessage, BUF_SIZE - 1, NULL, NULL);
	return szMessage;
}

/* Recursively add files. */
static bool add_file(const char *fname, bool (*add_file_hook)(const char *))
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;
	DWORD dwAttr;

	dwAttr = GetFileAttributesW(win32_utf8_to_utf16(fname));
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		wchar_t wszFindPath[BUF_SIZE];

		_snwprintf(wszFindPath, BUF_SIZE, L"%s\\*.*", win32_utf8_to_utf16(fname));

		hFind = FindFirstFileW(wszFindPath, &wfd);
		if(hFind == INVALID_HANDLE_VALUE)
			return false;
		do
		{
			wchar_t wszNextPath[BUF_SIZE];

			if (wcscmp(wfd.cFileName, L".") == 0)
				continue;
			if (wcscmp(wfd.cFileName, L"..") == 0)
				continue;

			_snwprintf(wszNextPath, BUF_SIZE, L"%s\\%s", win32_utf8_to_utf16(fname), wfd.cFileName);
			if (!add_file(win32_utf16_to_utf8(wszNextPath), add_file_hook))
				return false;
		}
		while(FindNextFileW(hFind, &wfd));
	}
	else
	{
		wide_printf(N_TR("Adding file %s\n"), fname);
		if (!add_file_hook(fname))
			return false;
	}

	return true;
}

#endif	/* !defined(_WIN32) */

/*
 * Windows Quirk
 */

#ifdef _WIN32
#include <windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    /* Dispatch to main(). */
    int argc, i;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = malloc(sizeof(char *) * argc);
    for (i = 0; i < argc; i++)
    {
        char tmp[1024];
	WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, tmp, sizeof(tmp) - 1, NULL, NULL);
        argv[i] = strdup(tmp);
    }
    return main(argc, argv);
}
#endif

/*
 * FFI Functions
 */

/* FFI function implementation. */
static bool cfunc_import(NoctEnv *env);
static bool cfunc_print(NoctEnv *env);
static bool cfunc_readline(NoctEnv *env);
static bool cfunc_readint(NoctEnv *env);
static bool cfunc_readfilelines(NoctEnv *env);
static bool cfunc_writefilelines(NoctEnv *env);
static bool cfunc_shell(NoctEnv *env);

/* FFI table. */
struct ffi_item {
	const char *name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
} ffi_items[] = {
	{"import", 1, {"file"}, cfunc_import},
	{"print", 1, {"msg"}, cfunc_print},
	{"readLine", 0, {NULL}, cfunc_readline},
	{"readInt", 0, {NULL}, cfunc_readint},
	{"readFileLines", 1, {"file"}, cfunc_readfilelines},
	{"writeFileLines", 2, {"file", "lines"}, cfunc_writefilelines},
	{"shell", 1, {"cmd"}, cfunc_shell},
};

/* Register FFI functions. */
static bool
register_ffi(
	NoctEnv *env)
{
	int i;

	for (i = 0; i < (int)(sizeof(ffi_items) / sizeof(struct ffi_item)); i++) {
		if (!noct_register_cfunc(env,
					 ffi_items[i].name,
					 ffi_items[i].param_count,
					 ffi_items[i].param,
					 ffi_items[i].cfunc,
					 NULL))
			return false;
	}

	/* Add a global variable "App". */
	NoctValue dict;
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "App", &dict))
		return false;

	return true;
}

/* Implementation of import() */
static bool
cfunc_import(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *file;
	char *data;
	size_t len;

	if (!noct_get_arg_check_string(env, 0, &tmp, &file))
		return false;

	/* Load a source file content. */
	if (!load_file_content(file, &data, &len))
		return false;

	/* Check for the bytecode header. */
	if (strncmp(data, BYTECODE_HEADER, strlen(BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, file, data)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return false;
		}
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, len)) {
			const char *file, *msg;
			int line;
			noct_get_error_file(env, &file);
			noct_get_error_line(env, &line);
			noct_get_error_message(env, &msg);
			wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, msg);
			return false;
		}
	}

	return true;
}

/* Implementation of print() */
static bool
cfunc_print(
	NoctEnv *env)
{
	char buf[8192];
	NoctValue value;

	if (!noct_pin_local(env, 1, &value))
		return false;

	if (!noct_get_arg(env, 0, &value)) {
		noct_unpin_local(env, 1, &value);
		return false;
	}

	memset(buf, 0, sizeof(buf));
	serialize_printer(env, buf, sizeof(buf), &value, false);

	wide_printf("%s\n", buf);

	noct_unpin_local(env, 1, &value);

	return true;
}
	
static bool serialize_printer(
	NoctEnv *env,
	char *buf,
	size_t size,
	NoctValue *value,
	bool is_inside_obj)
{
	int type;
	int ival;
	float fval;
	const char *sval;
	int items;
	int i;
	char digits[1024];

	if (!noct_get_value_type(env, value, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, value, &ival))
			return false;
		snprintf(digits, sizeof(digits), "%d", ival);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, value, &fval))
			return false;
		snprintf(digits, sizeof(digits), "%f", fval);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_STRING:
		if (!noct_get_string(env, value, &sval))
			return false;
		if (is_inside_obj)
			strncat(buf, "\"", size);
		strncat(buf, sval, size);
		if (is_inside_obj)
			strncat(buf, "\"", size);
		break;
	case NOCT_VALUE_ARRAY:
		if (!noct_get_array_size(env, value, &items))
			return false;
		strncat(buf, "[", size);
		for (i = 0; i < items; i++) {
			NoctValue elem;
			if (!noct_get_array_elem(env, value, i, &elem))
				return false;
			if (!serialize_printer(env, buf, size, &elem, true))
				return false;
			if (i != items - 1)
				strncat(buf, ", ", size);
		}
		strncat(buf, "]", size);
		break;
	case NOCT_VALUE_DICT:
		if (!noct_get_dict_size(env, value, &items))
			return false;
		strncat(buf, "{", size);
		for (i = 0; i < items; i++) {
			NoctValue k, v;
			if (!noct_get_dict_key_by_index(env, value, i, &k))
				return false;
			if (!noct_get_string(env, &k, &sval))
				return false;
			strncat(buf, sval, size);
			strncat(buf, ": ", size);
			if (!noct_get_dict_value_by_index(env, value, i, &v))
				return false;
			serialize_printer(env, buf, size, &v, true);
			if (i != items - 1)
				strncat(buf, ", ", size);
		}
		strncat(buf, "}", size);
		break;
	case NOCT_VALUE_FUNC:
		strncat(buf, "<func>", size);
		break;
	default:
		assert(0);
		break;
	}

	return true;
}

/* Implementation of readline() */
static bool
cfunc_readline(
	NoctEnv *env)
{
	struct rt_value ret;
	char buf[1024];
	int len;

	memset(buf, 0, sizeof(buf));

	if (fgets(buf, sizeof(buf) - 1, stdin) == NULL) {
		if (!noct_set_return_make_int(env, &ret, 0))
			return false;
		return true;
	}

	len = (int)strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';
	
	if (!noct_make_string(env, &ret, buf))
		return false;
	if (!noct_set_return(env, &ret))
		return false;

	return true;
}

/* Implementation of readint() */
static bool
cfunc_readint(
	NoctEnv *env)
{
	NoctValue tmp;
	char buf[1024];

	memset(buf, 0, sizeof(buf));

	if (fgets(buf, sizeof(buf) - 1, stdin) == NULL)
		strcpy(buf, "");
	
	if (!noct_set_return_make_int(env, &tmp, atoi(buf)))
		return false;

	return true;
}

/* Implementation of readfilelines() */
static bool
cfunc_readfilelines(
	NoctEnv *env)
{
	const char *file;
	FILE *fp;
	char buf[4096];
	struct rt_value array;
	struct rt_value line;
	int index;
	NoctValue tmp;

	if (!noct_get_arg_check_string(env, 0, &tmp, &file))
		return false;

	fp = fopen(file, "r");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannon open file %s."), file);
		return false;
	}

	if (!noct_make_empty_array(env, &array))
		return false;

	index = 0;
	while (fgets(buf, sizeof(buf) - 2, fp) != NULL) {
		int len;

		len = strlen(buf);
		if (len > 0) {
			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';
		}

		if (!noct_make_string(env, &line, buf))
			return false;
		if (!noct_set_array_elem(env, &array, index, &line))
			return false;
		index++;
	}

	fclose(fp);

	if (!noct_set_return(env, &array))
		return false;

	return true;
}

/* Implementation of writefilelines() */
static bool
cfunc_writefilelines(
	NoctEnv *env)
{
	const char *file;
	NoctValue array, tmp;
	FILE *fp;
	int i, size;

	if (!noct_get_arg_check_string(env, 0, &tmp, &file))
		return false;
	if (!noct_get_arg_check_array(env, 1, &array))
		return false;

	fp = fopen(file, "wb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannon open file %s."), file);
		return false;
	}

	if (!noct_get_array_size(env, &array, &size))
		return false;

	for (i = 0; i < size; i++) {
		const char *line;

		if (!noct_get_array_elem_check_string(env, &array, i, &tmp, &line))
			return false;

		fprintf(fp, "%s\n", line);
	}

	fclose(fp);

	if (!noct_set_return_make_int(env, &tmp, 1))
		return false;

	return true;
}

/* Implementation of shell() */
static bool
cfunc_shell(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *s;
	int cmd_ret;

	/* Get a "cmd" parameer. */
	if (!noct_get_arg_check_string(env, 0, &tmp, &s))
		return false;

	/* Run a command. */
	cmd_ret = system(s);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &tmp, cmd_ret))
		return false;

	return true;
}
