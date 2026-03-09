/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * File HAL for Chrome's File System Access API
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

#include <emscripten.h>

#include <string.h>
#include <assert.h>

/* Path length. */
#define PATH_SIZE	(1024)

/* File read stream. */
struct hal_rfile {
	char *data;
	uint64_t size;
	uint64_t pos;
};

/* File write stream. */
struct hal_wfile {
	char *buf;
	uint64_t buf_size;
	uint64_t size;
};

/* Forward declaration.
 */
static void ungetc_rfile(struct hal_rfile *rf, char c);

/* JS: Get a file size. */
EM_ASYNC_JS(int, jsGetFileSize, (const char *file_name),
{
	const fileName = UTF8ToString(file_name);
	try{
		const fileHandle = await dirHandle.getFileHandle(fileName, { create: false });
		const file = await fileHandle.getFile();
		const fileSize = file.size;
		return fileSize;
	} catch(e) {
		alert('s2GetFileSize(): error '+ e);
	}
	return -1;
});

/* JS: Get a file content. */
EM_ASYNC_JS(int, jsReadFile, (const char *file_name, char *data),
{
	const fileName = UTF8ToString(file_name);
	try{
		const fileHandle = await dirHandle.getFileHandle(fileName, { create: false });
		const file = await fileHandle.getFile();
		const fileSize = file.size;
		const fileReader = new FileReader();
		const arrayBuffer = await new Promise((resolve, reject) => {
			fileReader.addEventListener('load', () => resolve(fileReader.result));
			fileReader.readAsArrayBuffer(file);
		});
		const u8Array = new Uint8Array(arrayBuffer);
		writeArrayToMemory(u8Array, data);
		return 0;
	} catch(e) {
		alert('s2ReadFile(): error ' + e);
	}
	return -1;
});

/*
 * Check whether a file exists.
 */
bool
hal_check_file_exist(
	const char *file)
{
	uint64_t size;

	/* Get the file size. */
	size = jsGetFileSize(file);
	if (size == -1) {
		/* File does not exist. */
		return false;
	}

	/* File exists. */
	return true;
}

/*
 * Open a file read stream.
 */
bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **f)
{
	struct hal_rfile *rf;
	uint64_t size;
	int result;

	/* Get the file size. */
	size = jsGetFileSize(file);
	if (size == -1)
		return false;

	/* Allocate a rfile structure. */
	rf = malloc(sizeof(struct hal_rfile));
	if (rf == NULL) {
		hal_log_out_of_memory();
		return false;
	}
	rf->size = size;
	rf->pos = 0;
	if (size == 0) {
		rf->data = NULL;
		*f = rf;
		return true;
	}

	/* Allocate a file buffer. */
	rf->data = malloc(size);
	if (rf->data == NULL) {
		hal_log_out_of_memory();
		return false;
	}
	memset(rf->data, 0, size);

	/* Get the file content. */
	result = jsReadFile(file, rf->data);
	if (result == -1) {
		hal_log_error("ASM ERROR.");
		free(rf);
		return false;
	}

	*f = rf;
	return true;
}

/*
 * Get a file size.
 */
bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *ret)
{
	*ret = rf->size;
	return true;
}

/*
 * Enable de-obfuscation on a read file stream.
 */
void
hal_decode_rfile(
	struct hal_rfile *f)
{
	UNUSED_PARAMETER(f);
}

/*
 * Read from a file read stream.
 */
bool
hal_read_rfile(
	struct hal_rfile *rf,
	void *buf,
	size_t size,
	size_t *ret)
{
	if (rf->pos + size > rf->size)
		size = (size_t)(rf->size - rf->pos);
	if (size == 0)
		return false;
	memcpy(buf, rf->data + rf->pos, size);
	rf->pos += size;
	*ret = size;
	return true;
}

/*
 * Read a u64 from a file read stream.
 */
bool
hal_get_rfile_u64(
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

/* Read a u32 from a file read stream. */
bool
get_rfile_u32(
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

/* Read a u16 from a file read stream. */
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

/* Read a u8 from a file read stream. */
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
	struct hal_rfile *rf,
	char c)
{
	rf->pos--;
}

/*
 * Rewind a read stream.
 */
void
hal_rewind_rfile(
	struct hal_rfile *rf)
{
	rf->pos = 0;
}

/*
 * Close a file read stream.
 */
void
hal_close_rfile(
	struct hal_rfile *rf)
{
	assert(rf != NULL);

	free(rf->data);
	free(rf);
}

/*
 * Open a file write stream.
 */
bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **f)
{
	/* TODO */
	return false;
}

/*
 * Write to a file write stream.
 */
bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret)
{
	/* TODO */
	return false;
}

/*
 * Close a file write stream.
 */
void
hal_close_wfile(
	struct hal_wfile *wf)
{
	/* TODO */
}

/*
 * Remove a real file.
 */
void
remove_file(
	const char *file)
{
	/* TODO */
}
