/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Packager main
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "stratohal/platform.h"

/* Obfuscation Key */
#include "key.h"

#ifdef HAL_TARGET_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Maximum file entries */
#define FILE_ENTRY_SIZE		(8192)

/* File name size */
#define FILE_NAME_SIZE		(256)

/* Max path size */
#define PATH_SIZE		(256)

/* Size of file count which is written at top of an archive */
#define FILE_COUNT_BYTES	(8)

/* Size of file entry */
#define ENTRY_BYTES		(256 + 8 + 8)

/* Size of directory names */
#define DIR_COUNT	((int)(sizeof(dir_names) / sizeof(const char *)))

/* File entry */
struct file_entry {
	char name[FILE_NAME_SIZE];
	uint64_t size;
	uint64_t offset;
};

/* File entry */
struct file_entry entry[FILE_ENTRY_SIZE];

/* File count */
static uint64_t file_count;

/* Current processing file's offset in archive file */
static uint64_t offset;

/* Next random number. */
static uint64_t next_random;

/* forward declaration */
static bool add_file(const char *fname);
static bool get_file_sizes(void);
static bool write_archive_file(const char *pkg_file);
static bool write_file_entries(FILE *fp);
static bool write_file_bodies(FILE *fp);
static void set_random_seed(uint64_t index);
static char get_next_random(void);

/*
 * main
 */
int command_archive(int argc, char *argv[])
{
	int i;

	file_count = 0;
	offset = 0;
	next_random = 0;

	if (argc < 2) {
		printf("Specify input files.\n");
		return 1;
	}

	/* Add scpecified files. */
	for (i = 1; i < argc; i++) {
		if (!add_file(argv[i])) {
			printf("Failed.\n");
			return 1;
		}
	}

	/* Get all file sizes and decide all offsets. */
	if (!get_file_sizes()) {
		printf("Failed.\n");
		return 1;
	}

	/* Write an archive file. */
	if (!write_archive_file(HAL_PACKAGE_FILE)) {
		printf("Failed.\n");
		return 1;
	}

	return 0;
}

#if defined(HAL_TARGET_WINDOWS)

/*
 * For Windows:
 */

#define CONV_MESSAGE_SIZE 65536

static wchar_t wszMessage[CONV_MESSAGE_SIZE];
static char szMessage[CONV_MESSAGE_SIZE];

static const wchar_t *utf8_to_utf16(const char *utf8_message)
{
	assert(utf8_message != NULL);
	MultiByteToWideChar(CP_UTF8, 0, utf8_message, -1, wszMessage, CONV_MESSAGE_SIZE - 1);
	return wszMessage;
}

static const char *utf16_to_utf8(const wchar_t *utf16_message)
{
	assert(utf16_message != NULL);
	WideCharToMultiByte(CP_UTF8, 0, utf16_message, -1, szMessage, CONV_MESSAGE_SIZE - 1, NULL, NULL);
	return szMessage;
}

/* Get file list in directory (for Windows) */
static bool add_file(const char *fname)
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;
	DWORD dwAttr;

	if (file_count >= FILE_ENTRY_SIZE)
	{
		printf("Error: too many files in a package.\n");
		return false;
	}

	dwAttr = GetFileAttributesW(utf8_to_utf16(fname));
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		wchar_t wszFindPath[PATH_SIZE];

		_snwprintf(wszFindPath, PATH_SIZE, L"%s\\*.*", utf8_to_utf16(fname));

		hFind = FindFirstFileW(wszFindPath, &wfd);
		if(hFind == INVALID_HANDLE_VALUE)
			return false;
		do
		{
			wchar_t wszNextPath[PATH_SIZE];

			if (wcscmp(wfd.cFileName, L".") == 0)
				continue;
			if (wcscmp(wfd.cFileName, L"..") == 0)
				continue;

			_snwprintf(wszNextPath, PATH_SIZE, L"%s\\%s", utf8_to_utf16(fname), wfd.cFileName);
			if (!add_file(utf16_to_utf8(wszNextPath)))
				return false;
		}
		while(FindNextFileW(hFind, &wfd));
	}
	else
	{
		char *slash;
		while ((slash = strchr(fname, '\\')) != NULL)
			*slash = '/';
		snprintf(entry[file_count].name, FILE_NAME_SIZE, "%s", fname);
		printf("Adding file %s\n", fname);
		file_count++;
	}

	return true;
}

#else

/*
 * For macOS and Linux:
 */

/* Get directory file list (for Mac and Linux) */
static bool add_file(const char *fname)
{
	struct stat st;

	if (file_count >= FILE_ENTRY_SIZE) {
		printf("Error: too many files in a package.\n");
		return false;
	}

	if (stat(fname, &st) != 0) {
		printf("Cannot find %s\n", fname);
		return false;
	}
	if (S_ISDIR(st.st_mode)) {
		struct dirent **names;
		int count;
		int i;

		/* printf("Searching directory %s\n", fname); */

		/* Get directory content. */
		count = scandir(fname, &names, NULL, alphasort);
		if (count < 0) {
			printf("Skipping empty directory %s\n", fname);
			return false;
		}

		for (i = 0; i < count; i++) {
			char next_path[256 + 1];

			if (strcmp(names[i]->d_name, ".") == 0)
				continue;
			if (strcmp(names[i]->d_name, "..") == 0)
				continue;

			snprintf(next_path, sizeof(next_path), "%s/%s", fname, names[i]->d_name);
			if (!add_file(next_path))
				return false;
		}
	} else if(S_ISREG(st.st_mode)) {
		snprintf(entry[file_count].name, FILE_NAME_SIZE, "%s", fname);
		printf("Adding %s\n", entry[file_count].name);
		file_count++;
	}

	return true;
}

#endif

/* Get sizes of each files. */
static bool get_file_sizes(void)
{
	FILE *fp;
	uint64_t i;

	/* Get each file size, and calc offsets. */
	offset = FILE_COUNT_BYTES + ENTRY_BYTES * file_count;
	for (i = 0; i < file_count; i++) {
		/*
		 * Make a path and open the file.
		 */
#if defined(HAL_TARGET_WINDOWS)
		/* Make a path on Windows. */
		char *path = strdup(entry[i].name);
		char *slash;
		if (path == NULL) {
			printf("Out of memory.\n");
			return false;
		}
		while ((slash = strchr(path, '/')) != NULL)
			*slash = '\\';
		fp = fopen(path, "rb");
		free(path);
#else
		/* Make path on Mac/Linux. */
		fp = fopen(entry[i].name, "r");
#endif

		/* Check the fp. */
		if (fp == NULL) {
			printf("Failed to open %s.\n", entry[i].name);
			return false;
		}

		/* Get the file size. */
		fseek(fp, 0, SEEK_END);
		entry[i].size = (uint64_t)ftell(fp);
		entry[i].offset = offset;
		fclose(fp);

		/* Increment the offset. */
		offset += entry[i].size;

		/*printf("Added %s : size %lld, offset %lld\n", entry[i].name, entry[i].size, entry[i].offset);*/
	}
	return true;
}

/* Write archive file. */
static bool write_archive_file(const char *pkg_file)
{
	FILE *fp;
	bool success;

	fp = fopen(pkg_file, "wb");
	if (fp == NULL) {
		printf("Failed to open %s.\n", pkg_file);
		return false;
	}

	success = false;
	do {
		if (fwrite(&file_count, sizeof(uint64_t), 1, fp) < 1)
			break;
		if (!write_file_entries(fp))
			break;
		if (!write_file_bodies(fp))
			break;
		fclose(fp);
		success = true;
	} while (0);

	if (!success) {
		printf("Failed to write to %s.\n", pkg_file);
		return false;
	}

	printf("Wrote %s.\n", pkg_file);

	return true;
}

/* Write file entries. */
static bool write_file_entries(FILE *fp)
{
	char xor[FILE_NAME_SIZE];
	uint64_t i;
	int j;

	for (i = 0; i < file_count; i++) {
		set_random_seed(i);
		for (j = 0; j < FILE_NAME_SIZE; j++)
			xor[j] = entry[i].name[j] ^ get_next_random();

		if (fwrite(xor, FILE_NAME_SIZE, 1, fp) < 1)
			return false;
		if (fwrite(&entry[i].size, sizeof(uint64_t), 1, fp) < 1)
			return false;
		if (fwrite(&entry[i].offset, sizeof(uint64_t), 1, fp) < 1)
			return false;
	}
	return true;
}

/* Write file bodies. */
static bool write_file_bodies(FILE *fp)
{
	char buf[8192];
	FILE *fpin;
	uint64_t i;
	size_t len, obf;

	for (i = 0; i < file_count; i++) {
#ifdef HAL_TARGET_WINDOWS
		char *path = strdup(entry[i].name);
		char *slash;
		if (path == NULL) {
			printf("Out of memory.\n");
			return false;
		}
		while ((slash = strchr(path, '/')) != NULL)
			*slash = '\\';
		fpin = fopen(path, "rb");
#else
		fpin = fopen(entry[i].name, "r");
#endif
		if (fpin == NULL) {
			printf("Failed to open %s.\n", entry[i].name);
			return false;
		}
		set_random_seed(i);
		do  {
			len = fread(buf, 1, sizeof(buf), fpin);
			if (len > 0) {
				for (obf = 0; obf < len; obf++)
					buf[obf] ^= get_next_random();
				if (fwrite(buf, len, 1, fp) < 1) {
					printf("Failed to write to the package file.\n");
					return false;
				}
			}
		} while (len == sizeof(buf));
#ifdef HAL_TARGET_WINDOWS
		free(path);
#endif
		fclose(fpin);
	}
	return true;
}

/* Set random seed. */
static void set_random_seed(uint64_t index)
{
	uint64_t i, lsb;

	next_random = OBFUSCATION_KEY;
	for (i = 0; i < index; i++) {
		next_random ^= 0xafcb8f2ff4fff33fULL;
		lsb = next_random >> 63;
		next_random = (next_random << 1) | lsb;
	}
}

/* Get next random number. */
char get_next_random(void)
{
	char ret;

	ret = (char)next_random;

	next_random = (((OBFUSCATION_KEY & 0xff00) * next_random +
			(OBFUSCATION_KEY & 0xff)) % OBFUSCATION_KEY) ^
		      0xfcbfaff8f2f4f3f0;

	return ret;
}
