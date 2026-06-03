/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Main
 */

#include "cli-main.h"

#if defined(NOCT_TARGET_WINDOWS)
#include <windows.h>
#elif defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#elif defined(NOCT_TARGET_DOS4G)
#include <dos.h>
#include <sys/stat.h>
#endif

/* i18n.c */
#if defined(NOCT_USE_TRANSLATION)
void noct_init_locale(void);
#endif

/*
 * Main
 */
int main(int argc, char *argv[])
{
#if defined(NOCT_USE_TRANSLATION)
	noct_init_locale();
#endif

	if (argc <= 1) {
#if defined(NOCT_USE_REPL)
		return command_repl();
#else
		show_usage();
		return 1;
#endif
	}

	if (strcmp(argv[1], "--version") == 0) {
		wide_printf("Noct " NOCT_VERSION "\n");
		return 0;
	}

	if (strcmp(argv[1], "--help") == 0) {
		show_usage();
		return 1;
	}

#if defined(NOCT_USE_BCBACKEND)
	if (strcmp(argv[1], "--compile") == 0)
		return command_compile(argc, argv);
#endif

#if defined(NOCT_USE_CBACKEND)
	if (strcmp(argv[1], "--ansic") == 0)
		return command_transpile_c(argc, argv);
#endif

#if defined(NOCT_USE_ELBACKEND)
	if (strcmp(argv[1], "--elisp") == 0)
		return command_transpile_elisp(argc, argv);
#endif

#if defined(NOCT_USE_SCMBACKEND)
	if (strcmp(argv[1], "--scheme") == 0)
		return command_transpile_scheme(argc, argv);
#endif

	return command_run(argc, argv);
}

/*
 * Windows Main
 */
#ifdef _WIN32
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
 * Show the usage message.
 */
void show_usage(void)
{
	wide_printf(N_TR("Noct Programming Language\n"));
	wide_printf(N_TR("Version %s\n"), NOCT_VERSION);
	wide_printf("\n");
	wide_printf(N_TR("Usage\n"));
	wide_printf(N_TR("  noct <vm-options> <files>           ... run a program\n"));
#if defined(NOCT_USE_BCBACKEND)
	wide_printf(N_TR("  noct --compile <in-files>           ... convert to bytecode files\n"));
#endif
#if defined(NOCT_USE_CBACKEND)
	wide_printf(N_TR("  noct --ansic <out-file> <in-files>  ... convert to a C source file\n"));
#endif
#if defined(NOCT_USE_ELBACKEND)
	wide_printf(N_TR("  noct --elisp <out-file> <in-files>  ... convert to an Emacs Lisp source file\n"));
#endif
#if defined(NOCT_USE_SCMBACKEND)
	wide_printf(N_TR("  noct --scheme <out-file> <in-files> ... convert to an Scheme source file\n"));
#endif
	wide_printf("\n");
	wide_printf(N_TR("vm-options:\n"));
	wide_printf(N_TR("  --disable-jit        ... disable JIT\n"));
	wide_printf(N_TR("  --force-jit          ... equivalent to --jit-threshold=0\n"));
	wide_printf(N_TR("  --jit-threshold=N    ... call-count threshold for compilation\n"));
	wide_printf(N_TR("  --optimize-level=N   ... optimize level (0/1)\n"));
	wide_printf(N_TR("  --gc-nursery-size=N  ... first GC space size in bytes\n"));
	wide_printf(N_TR("  --gc-graduate-size=N ... second GC space size in bytes\n"));
	wide_printf(N_TR("  --gc-tenure-size=N   ... final GC space size in bytes\n"));
	wide_printf(N_TR("  --gc-lop-threshold=N ... move objects larger than N-bytes to final GC space\n"));
	wide_printf("\n");
}

/*
 * Load a file.
 */
bool load_file_content(const char *fname, char **data, size_t *size)
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
	*size = (size_t)ftell(fp);
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

/*
 * Print to console. (supports wide characters on Windows console.)
 */
int wide_printf(const char *format, ...)
{
	static char buf[4096];
	va_list ap;
	int size;

	va_start(ap, format);
	size = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if !defined(_WIN32)
	printf("%s", buf);
	return size;
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
#if defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)

/*
 * Recursively add files.
 */
bool add_file(const char *fname, bool (*add_file_hook)(const char *))
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

#endif

/*
 * For Windows
 */
#if defined(NOCT_TARGET_WINDOWS)

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

/*
 * Recursively add files.
 */
bool add_file(const char *fname, bool (*add_file_hook)(const char *))
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

#endif

#if defined(NOCT_TARGET_DOS4G)

#ifndef _A_SUBDIR
#define _A_SUBDIR 0x10
#endif

/*
 * Recursively add files.
 */
bool add_file(const char *fname, bool (*add_file_hook)(const char *))
{
        struct stat st;

        if (stat(fname, &st) != 0) {
                printf(N_TR("Cannot find %s.\n"), fname);
                return false;
        }

        if (st.st_mode & S_IFDIR) {
                struct find_t find;
                char pattern[256 + 1];
                unsigned rc;

                printf(N_TR("Searching directory %s.\n"), fname);

                snprintf(pattern, sizeof(pattern), "%s\\*.*", fname);

                rc = _dos_findfirst(pattern, _A_NORMAL | _A_RDONLY | _A_HIDDEN |
                                             _A_SYSTEM | _A_SUBDIR | _A_ARCH,
                                    &find);
                if (rc != 0) {
                        wide_printf(N_TR("Skipping empty directory %s.\n"), fname);
                        return false;
                }

                do {
                        char next_path[256 + 1];

                        if (strcmp(find.name, ".") == 0)
                                continue;
                        if (strcmp(find.name, "..") == 0)
                                continue;

                        snprintf(next_path, sizeof(next_path), "%s\\%s", fname, find.name);

                        if (!add_file(next_path, add_file_hook))
                                return false;

                } while (_dos_findnext(&find) == 0);

        } else if (st.st_mode & S_IFREG) {
                wide_printf(N_TR("Adding file %s.\n"), fname);

                if (!add_file_hook(fname))
                        return false;
        }

        return true;
}

#endif
