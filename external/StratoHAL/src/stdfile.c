/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * File HAL for standard C I/O
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
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * [Archive File Format]
 *
 * struct header {
 *     u64 file_count;
 *     struct file_entry {
 *         u8  file_name[256]; // Obfuscated
 *         u64 file_size;
 *         u64 file_offset;
 *     } [file_count];
 * };
 * u8 file_body[file_count][file_length]; // Obfuscated
 */

#include "stratohal/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Win32 */
#ifdef HAL_TARGET_WINDOWS
#include <fcntl.h>
#endif

/*
 * The "key" of obfuscation
 */

/* The key. */
#define OBFUSCATION_KEY	(0xabadcafedeadbeef)

/* The key is shuffled so that decompilers cannot read it directly. */
static volatile uint64_t key_obfuscated =
	~((((OBFUSCATION_KEY >> 56) & 0xff) << 0) |
	  (((OBFUSCATION_KEY >> 48) & 0xff) << 8) |
	  (((OBFUSCATION_KEY >> 40) & 0xff) << 16) |
	  (((OBFUSCATION_KEY >> 32) & 0xff) << 24) |
	  (((OBFUSCATION_KEY >> 24) & 0xff) << 32) |
	  (((OBFUSCATION_KEY >> 16) & 0xff) << 40) |
	  (((OBFUSCATION_KEY >> 8)  & 0xff) << 48) |
	  (((OBFUSCATION_KEY >> 0)  & 0xff) << 56));

/* Shuffled key. */
static volatile uint64_t key_reversed;

/* Use indirect addressing to avoid storing key_reserved to a register on x86. (no effect on Arm.) */
static volatile uint64_t *key_ref = &key_reversed;

/* These keys are not secret. */
static const uint64_t NEXT_MASK1 = 0xafcb8f2ff4fff33f;
static const uint64_t NEXT_MASK2 = 0xfcbfaff8f2f4f3f0;

/*
 * Package
 */

/* Maximum entries in a package. */
#define ENTRY_SIZE		(65536)

/* File name length for an entry. */
#define FILE_NAME_SIZE		(256)

/* Package file entry. */
struct file_entry {
	/* File name. */
	char name[FILE_NAME_SIZE];

	/* File size. */
	uint64_t size;

	/* Offset in the package file. */
	uint64_t offset;
};

/* Package file path. */
static char *package_path;

/* File entry count. */
static uint64_t entry_count;

/* File entry table. */
static struct file_entry entry[ENTRY_SIZE];

/*
 * File read stream.
 */
struct hal_rfile {
	/* Is a packaged file? */
	bool is_packaged;

	/* Is obfuscated? */
	bool is_obfuscated;

	/* stdio FILE pointer */
	FILE *fp;

	/* Obfuscation parameters */
	uint64_t next_random;
	uint64_t prev_random;

	/* Effective for a packaged file: */
	uint64_t index;
	uint64_t size;
	uint64_t offset;
	uint64_t pos;
};

/*
 * File write stream.
 */
struct hal_wfile {
	FILE *fp;
	uint64_t next_random;
};

/*
 * Forward declarations.
 */
static bool open_package(struct hal_rfile *rf, const char *path);
#if !defined(HAL_TARGET_IOS) && !defined(HAL_TARGET_WASM)
static bool open_real(struct hal_rfile *rf, const char *path);
#endif
static void ungetc_rfile(struct hal_rfile *rf, char c);
static void set_random_seed(uint64_t index, uint64_t *next_random);
static char get_next_random(uint64_t *next_random, uint64_t *prev_random);
static void rewind_random(uint64_t *next_random, uint64_t *prev_random);

/*
 * Initialize the stdfile module.
 */
bool
init_file(void)
{
	FILE *fp;
	uint64_t i, next_random;
	int j;

	/* Get a real path to a package file. */
	package_path = hal_make_real_path(HAL_PACKAGE_FILE);
	if (package_path == NULL)
		return false;

	/* Try opening a package file. */
#ifdef HAL_TARGET_WINDOWS
	_fmode = _O_BINARY;
#ifdef _UNICODE
	fp = _wfopen(win32_utf8_to_utf16(package_path), L"rb");
#else
	fp = fopen(package_path, "rb");
#endif
#else
	fp = fopen(package_path, "r");
#endif
	if (fp == NULL) {
		/* Failed to open. */
#if defined(HAL_TARGET_IOS) || defined(HAL_TARGET_WASM)
		/* Fail: On iOS and Emscripten, we need a package file. */
		return false;
#else
		/* On other platforms, we won't use a package file. */
		free(package_path);
		package_path = NULL;
		return true;
#endif
	}

	/* Read the number of the file entries. */
	if (fread(&entry_count, sizeof(uint64_t), 1, fp) < 1) {
		hal_log_error("Corrupted package file.");
		fclose(fp);
		return false;
	}
	if (entry_count > ENTRY_SIZE) {
		hal_log_error("Corrupted package file.");
		fclose(fp);
		return false;
	}

	/* Read the file entries. */
	for (i = 0; i < entry_count; i++) {
		if (fread(&entry[i].name, FILE_NAME_SIZE, 1, fp) < 1)
			break;
		set_random_seed(i, &next_random);
		for (j = 0; j < FILE_NAME_SIZE; j++)
			entry[i].name[j] ^= get_next_random(&next_random, NULL);
		if (fread(&entry[i].size, sizeof(uint64_t), 1, fp) < 1)
			break;
		if (fread(&entry[i].offset, sizeof(uint64_t), 1, fp) < 1)
			break;
	}
	if (i != entry_count) {
		hal_log_error("Package file corrupted.");
		fclose(fp);
		return false;
	}

	/*
	 * Close the package for now;
	 * we will reopen a FILE pointer per an input stream.
	 */
	fclose(fp);

	return true;
}

/*
 * Cleanup the stdfile module.
 */
void
cleanup_file(void)
{
	if (package_path != NULL) {
		free(package_path);
		package_path = NULL;
	}
}

/*
 * Check whether a file exists.
 */
bool
hal_check_file_exist(
	const char *file)
{
	FILE *fp;
	uint64_t i;

	/* If we're using a package file. */
	if (package_path != NULL) {
		/* Check whether a file entry exists in the package. */
		for (i = 0; i < entry_count; i++) {
			if (strcasecmp(entry[i].name, file) == 0) {
				/* Entry exists. */
				return true;
			}
		}
	}

#if defined(HAL_TARGET_IOS) || defined(HAL_TARGET_WASM)
	(void)fp;
	return false;
#else
	/* Make a real file path. */
	char *real_path;
	real_path = hal_make_real_path(file);
	if (real_path == NULL)
		return false;

	/* Open a FILE pointer. */
#ifdef HAL_TARGET_WINDOWS
	_fmode = _O_BINARY;
#ifdef _UNICODE
	fp = _wfopen(win32_utf8_to_utf16(real_path), L"rb");
#else
	fp = fopen(real_path, "rb");
#endif
#else
	fp = fopen(real_path, "r");
#endif

	/* Check if file exists. */
	free(real_path);
	if (fp == NULL) {
		/* Doesn't exist. */
		return false;
	}

	/* File exists. */
	fclose(fp);
	return true;
#endif
}

/*
 * Open a read file stream.
 */
bool
hal_open_rfile(
	const char *path,
	struct hal_rfile **f)
{
	struct hal_rfile *fs;

	/* Allocate a file struct. */
	fs = malloc(sizeof(struct hal_rfile));
	if (fs == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	/* If we're using a package file. */
	if (package_path != NULL) {
		/* Open a package file. */
		if (!open_package(fs, path)) {
			free(fs);
			return false;
		}

		/* Succeeded. */
		*f = fs;
		return true;
	}

#if defined(HAL_TARGET_IOS) || defined(HAL_TARGET_WASM)
	return false;
#else
	/* Open a real file. */
	if (!open_real(fs, path)) {
		free(fs);
		return false;
	}

	/* Succeeded. */
	*f = fs;
	return true;
#endif
}

/* Open a file in the package. */
static bool
open_package(
	struct hal_rfile *f,
	const char *path)
{
	uint64_t i;
		
	/* Search a file entry on the package. */
	for (i = 0; i < entry_count; i++) {
		if (strcasecmp(entry[i].name, path) == 0)
			break;
	}
	if (i == entry_count) {
		/* Not found. */
		//hal_log_error("Cannot open file \"%s\".", path);
		return false;
	}

	/* Open a new FILE pointer to the package file. */
#ifdef HAL_TARGET_WINDOWS
	_fmode = _O_BINARY;
#ifdef _UNICODE
	f->fp = _wfopen(win32_utf8_to_utf16(package_path), L"rb");
#else
	f->fp = fopen(package_path, "rb");
#endif
#else
	f->fp = fopen(package_path, "r");
#endif
	if (f->fp == NULL) {
		//hal_log_error("Cannot open file \"%s\".", package_path);
		return false;
	}

	/* Seek to the offset. */
	if (fseek(f->fp, (long)entry[i].offset, SEEK_SET) != 0) {
		//hal_log_error("Cannot read file \"%s\".", HAL_PACKAGE_FILE);
		fclose(f->fp);
		return false;
	}

	/* Setup the file struct. */
	f->is_packaged = true;
	f->is_obfuscated = true;
	f->index = i;
	f->size = entry[i].size;
	f->offset = entry[i].offset;
	f->pos = 0;
	set_random_seed(i, &f->next_random);
	f->prev_random = 0;

	return true;
}

#if !defined(HAL_TARGET_IOS) && !defined(HAL_TARGET_WASM)
/* Open a real file on a file system. */
static bool
open_real(
	struct hal_rfile *f,
	const char *path)
{
	char *real_path;

	/* Make a real path on the OS's file system. */
	real_path = hal_make_real_path(path);
	if (real_path == NULL)
		return false;

	/* Open a real file. */
#ifdef HAL_TARGET_WINDOWS
	_fmode = _O_BINARY;
#ifdef _UNICODE
	f->fp = _wfopen(win32_utf8_to_utf16(real_path), L"rb");
#else
	f->fp = fopen(real_path, "rb");
#endif
#else
	f->fp = fopen(real_path, "r");
#endif
	free(real_path);
	if (f->fp == NULL)
		return false;

	f->is_packaged = false;
	f->is_obfuscated = false;

	return true;
}
#endif

/*
 * Get a file size.
 */
bool
hal_get_rfile_size(
	struct hal_rfile *f,
	size_t *ret)
{
	long pos, len;

	/* If f points to a package entry. */
	if (f->is_packaged) {
		*ret = (size_t)f->size;
		return true;
	}

	/* Return a real file size. */
	pos = ftell(f->fp);
	fseek(f->fp, 0, SEEK_END);
	len = ftell(f->fp);
	fseek(f->fp, pos, SEEK_SET);
	*ret = (size_t)len;
	return true;
}

/*
 * Enable de-obfuscation on a read file stream.
 */
void
hal_decode_rfile(
	struct hal_rfile *f)
{
	/* Setup the file struct. */
	f->is_obfuscated = true;
	set_random_seed(0, &f->next_random);
}

/*
 * Read bytes from a read file stream.
 */
bool
hal_read_rfile(
	struct hal_rfile *f,
	void *buf,
	size_t size,
	size_t *ret)
{
	size_t len, obf;

	assert(f != NULL);
	assert(f->fp != NULL);

	if (f->is_packaged) {
		/*
		 * For the case f points to a package entry.
		 */

		/* Read. */
		if (f->pos + size > f->size)
			size = (size_t)(f->size - f->pos);
		if (size == 0)
			return false;
		len = fread(buf, 1, size, f->fp);
		f->pos += len;

		/* Do obfuscation decode. */
		for (obf = 0; obf < len; obf++)
			*(((char *)buf) + obf) ^= get_next_random(&f->next_random, &f->prev_random);
	} else {
		/*
		 * For the case f points to a real file.
		 */

		/* Read. */
		len = fread(buf, 1, size, f->fp);

		/* Do obfuscation decode. */
		if (f->is_obfuscated) {
			for (obf = 0; obf < len; obf++)
				*(((char *)buf) + obf) ^= get_next_random(&f->next_random, &f->prev_random);
		}

	}

	*ret = len;
	if (len == 0)
		return false;

	return true;
}

/*
 * Read a u64 from a file stream.
 */
bool
get_rfile_u64(
	struct hal_rfile *f,
	uint64_t *data)
{
	uint64_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 8, &ret))
		return false;
	if (ret != 8)
		return false;

	*data = hal_le_to_host_64(val);
	return true;
}

/*
 * Read a u32 from a file stream.
 */
bool
hal_get_rfile_u32(
	struct hal_rfile *f,
	uint32_t *data)
{
	uint32_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 4, &ret))
		return false;
	if (ret != 4)
		return false;

	*data = hal_le_to_host_32(val);
	return true;
}

/*
 * Read a u16 from a file stream.
 */
bool
hal_get_rfile_u16(
	struct hal_rfile *f,
	uint16_t *data)
{
	uint16_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 2, &ret))
		return false;
	if (ret != 2)
		return false;

	*data = hal_le_to_host_16(val);
	return true;
}

/*
 * Read a u8 from a file stream.
 */
bool
hal_get_rfile_u8(
	struct hal_rfile *f,
	uint8_t *data)
{
	uint8_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 1, &ret))
		return false;
	if (ret != 1)
		return false;

	*data = val;
	return true;
}

/*
 * Read a line from a read file stream.
 */
bool
hal_get_rfile_string(
	struct hal_rfile *f,
	char *buf,
	size_t size)
{
	char *ptr;
	size_t len, read_size;
	char c;

	assert(f != NULL);
	assert(f->fp != NULL);
	assert(buf != NULL);
	assert(size > 0);

	ptr = buf;
	for (len = 0; len < size - 1; len++) {
		if (!hal_read_rfile(f, &c, 1, &read_size)) {
			*ptr = '\0';
			if (len == 0)
				return false;
			return true;
		}
		if (c == '\n' || c == '\0') {
			*ptr = '\0';
			return true;
		}
		if (c == '\r') {
			if (!hal_read_rfile(f, &c, 1, &read_size)) {
				*ptr = '\0';
				return true;
			}
			if (c == '\n') {
				*ptr = '\0';
				return true;
			}
			ungetc_rfile(f, c);
			*ptr = '\0';
			return true;
		}
		*ptr++ = c;
	}
	*ptr = '\0';

	if (len == 0)
		return false;

	return true;
}

/* Push back a character to a read file stream. */
static void
ungetc_rfile(
	struct hal_rfile *f,
	char c)
{
	assert(f != NULL);
	assert(f->fp != NULL);

	if (f->is_packaged) {
		/* If f points to a package entry. */
		assert(f->pos != 0);
		ungetc(c, f->fp);
		f->pos--;
		rewind_random(&f->next_random, &f->prev_random);
	} else {
		/* If f points to a real file. */
		ungetc(c, f->fp);
	}
}

/*
 * Close a read file stream.
 */
void
hal_close_rfile(
	struct hal_rfile *f)
{
	assert(f != NULL);
	assert(f->fp != NULL);

	fclose(f->fp);
	free(f);
}

/*
 * Rewind a read file stream.
 */
void
hal_rewind_rfile(
	struct hal_rfile *f)
{
	assert(f != NULL);
	assert(f->fp != NULL);

	if (f->is_packaged) {
		/* If f points to a package entry. */
		fseek(f->fp, (long)f->offset, SEEK_SET);
		f->pos = 0;
		set_random_seed(f->index, &f->next_random);
		f->prev_random = 0;
	} else {
		/* If f points to a real file. */
		rewind(f->fp);
		f->pos = 0;
	}
}

/* Set a random seed. */
static void
set_random_seed(
	uint64_t index,
	uint64_t *next_random)
{
	uint64_t i, next, lsb;

	/* The key is shuffled so that decompilers cannot read it directly. */
	key_reversed = ((((key_obfuscated >> 56) & 0xff) << 0) |
			(((key_obfuscated >> 48) & 0xff) << 8) |
			(((key_obfuscated >> 40) & 0xff) << 16) |
			(((key_obfuscated >> 32) & 0xff) << 24) |
			(((key_obfuscated >> 24) & 0xff) << 32) |
			(((key_obfuscated >> 16) & 0xff) << 40) |
			(((key_obfuscated >> 8)  & 0xff) << 48) |
			(((key_obfuscated >> 0)  & 0xff) << 56));
	next = ~(*key_ref);
	for (i = 0; i < index; i++) {
		/* This XOR mask is not a secret. */
		next ^= NEXT_MASK1;
		lsb = next >> 63;
		next = (next << 1) | lsb;
	}

	*next_random = next;
}

/* Get a next random mask. */
static char
get_next_random(
	uint64_t *next_random,
	uint64_t *prev_random)
{
	uint64_t next;
	char ret;

	/* For ungetc(). */
	if (prev_random != NULL)
		*prev_random = *next_random;

	ret = (char)(*next_random);
	next = *next_random;
	next = (((~(*key_ref) & 0xff00) * next + (~(*key_ref) & 0xff)) %
		~(*key_ref)) ^ NEXT_MASK2;
	*next_random = next;

	return ret;
}

/* Go back to the previous random mask. */
static void
rewind_random(
	uint64_t *next_random,
	uint64_t *prev_random)
{
	*next_random = *prev_random;
	*prev_random = 0;
}

/*
 * Write
 */

/*
 * Open a write file stream.
 */
bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **wf)
{
	char *path;

	/* Allocate wfile struct. */
	*wf = malloc(sizeof(struct hal_wfile));
	if (*wf == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	/* Make a real file path. */
	path = hal_make_real_path(file);
	if (path == NULL) {
		hal_log_out_of_memory();
		free(*wf);
		*wf = NULL;
		return false;
	}

	/* Open a real file. */
#ifdef HAL_TARGET_WINDOWS
	_fmode = _O_BINARY;
#ifdef _UNICODE
	(*wf)->fp = _wfopen(win32_utf8_to_utf16(path), L"wb");
#else
	(*wf)->fp = fopen(path, "wb");
#endif
#else
	(*wf)->fp = fopen(path, "w");
#endif
	if ((*wf)->fp == NULL) {
		//log_error("Failed to open the file \"%s\".", path);
		free(path);
		free(*wf);
		*wf = NULL;
		return false;
	}
	free(path);

	/* Initialize the random seed. */
	set_random_seed(0, &(*wf)->next_random);

	return true;
}

/*
 * Write bytes to a write file stream.
 */
bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret)
{
	char obf[1024];
	const char *src;
	size_t block_size, out, total, i;

	assert(wf != NULL);
	assert(wf->fp != NULL);

	src = buf;
	total = 0;
	while (size > 0) {
		/* Get the block size. (Max. 1024 bytes) */
		if (size > sizeof(obf))
			block_size = sizeof(obf);
		else
			block_size = size;

		/* Obfuscate the block. */
		for (i = 0; i < block_size; i++)
			obf[i] = *src++ ^ get_next_random(&wf->next_random, NULL);

		/* Write the block to the stream. */
		out = fwrite(obf, 1, block_size, wf->fp);
		if (out != block_size) {
			*ret = total + out;
			return true;
		}
		total += out;
		size -= out;
	}

	*ret = total;
	return true;
}

/*
 * Close a write file stream.
 */
void
hal_close_wfile(
	struct hal_wfile *wf)
{
	assert(wf != NULL);
	assert(wf->fp != NULL);

	fflush(wf->fp);
	fclose(wf->fp);
	free(wf);
}

/*
 * Remove a real file.
 */
bool
hal_remove_file(
	const char *file)
{
	char *path;

	/* Make a real path. */
	path = hal_make_real_path(file);
	if (path == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	/* Remove the file from the file system. */
	remove(path);

	/* Free the memory of the path. */
	free(path);

	return true;
}
