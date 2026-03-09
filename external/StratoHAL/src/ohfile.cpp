/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * File HAL for OpenHarmony
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

// Base
extern "C" {
#include "stratohal/platform.h"
};

// HAL
extern "C" {
#include "ohfile.h"
};

// C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// OpenHarmony
#include <rawfile/raw_file_manager.h>
#include <rawfile/raw_file.h>

// Path Size
#define PATH_SIZE	(1024)

struct hal_rfile {
	// For save files
	FILE *fp;

	// For asset files
	RawFile *rawFile;
	uint64_t size;
};

struct hal_wfile {
	FILE *fp;
};

// Global pointers defined in `napi_init.cpp`
extern NativeResourceManager *nativeResMgr;
extern char *sandboxDir;

// Forward Declaration
static void ungetc_rfile(struct hal_rfile *rf, char c);

//
// Initialization
//

//
// Initialize ndkfile.
//
bool init_file(void)
{
	assert(nativeResMgr != NULL);

	return true;
}

//
// Cleanup ndkfile.
//
void cleanup_file(void)
{
}

//
// Read
//

//
// Check if a file exists.
//
bool
hal_check_file_exist(
	const char *file)
{
	if (nativeResMgr == NULL)
		return false;
    
	RawFile *rf = OH_ResourceManager_OpenRawFile(nativeResMgr, file);
	if (rf != NULL) {
		OH_ResourceManager_CloseRawFile(rf);
		return true;
	}

	return false;
}

//
// Open a file input stream.
//
bool
hal_open_rfile(
	const char *file,
	struct hal_rfile **rf)
{
	*rf = (struct hal_rfile *)malloc(sizeof(struct hal_rfile));
	if (*rf == NULL)
		return false;

	if (strncmp(file, HAL_SAVE_DIR "/", strlen(HAL_SAVE_DIR "/")) == 0) {
		/*
		 * For a save file.
		 */

		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", HAL_SAVE_DIR, file);

		(*rf)->fp = fopen(path, "r");
		if ((*rf)->fp == NULL) {
			free(*rf);
			hal_log_error("Failed to open rawfile \"%s\".", file);
			return false;
		}

		fseek((*rf)->fp, 0, SEEK_END);
		(*rf)->size = ftell((*rf)->fp);
		fseek((*rf)->fp, 0, SEEK_SET);

		(*rf)->rawFile = NULL;

		return true;
	} else {
		/*
		 * For an asset file.
		 */

		RawFile *rawFile = OH_ResourceManager_OpenRawFile(nativeResMgr, file);
		if (rawFile == NULL) {
			hal_log_error("Failed to open rawfile \"%s\".", file);
			return false;
		}

		(*rf)->rawFile = rawFile;
		(*rf)->size = (uint64_t)OH_ResourceManager_GetRawFileSize(rawFile);
		(*rf)->fp = NULL;

		return true;
	}
}

//
// Get a file size.
//
bool
hal_get_rfile_size(
	struct hal_rfile *rf,
	size_t *size)
{
	*size = (size_t)rf->size;
	return true;
}

//
// Enable de-obfuscation on a read file stream.
//
void
hal_decode_rfile(
	struct hal_rfile *f)
{
	UNUSED_PARAMETER(f);
}

//
// Read from a file input stream.
//
bool
hal_read_rfile(
	struct hal_rfile *rf,
	void *buf,
	size_t size,
	size_t *ret)
{
	if (rf->fp != NULL) {
		size_t len = fread(buf, 1, size, rf->fp);
		*ret = len;
		if (len == 0)
			return false;
	} else {
		int readLen = OH_ResourceManager_ReadRawFile(rf->rawFile, buf, size);
		if (readLen <= 0)
			return false;
		*ret = (size_t)readLen;
	}
	return true;
}

// ungetc()
static void
ungetc_rfile(
	struct hal_rfile *rf,
	char c)
{
	if (rf->fp != NULL) {
		// Do nothing.
	} else {
		OH_ResourceManager_SeekRawFile(rf->rawFile, 0, SEEK_SET);
	}
}

//
// Read a line from an input file stream.
//
bool
hal_gets_rfile(
	struct hal_rfile *rf,
	char *buf,
	size_t size)
{
	char *ptr;
	size_t len, ret;
	char c;

	ptr = (char *)buf;

	for (len = 0; len < size - 1; len++) {
		if (!hal_read_rfile(rf, &c, 1, &ret)) {
			*ptr = '\0';
			return len == 0 ? false : true;
		}
		if (c == '\n' || c == '\0') {
			*ptr = '\0';
			return true;
		}
		if (c == '\r') {
			if (!hal_read_rfile(rf, &c, 1, &ret)) {
				*ptr = '\0';
				return true;
			}
			if (c == '\n') {
				*ptr = '\0';
				return true;
			}
			ungetc_rfile(rf, c);
			*ptr = '\0';
			return true;
		}
		*ptr++ = c;
	}
	*ptr = '\0';
	return true;
}

//
// Rewind a file input stream.
//
void
hal_rewind_rfile(
	struct hal_rfile *rf)
{
	if (rf->fp != NULL)
		rewind(rf->fp);
	else
		OH_ResourceManager_SeekRawFile(rf->rawFile, 0, SEEK_SET);
}

//
// Close a file input stream.
//
void
hal_close_rfile(
	struct hal_rfile *rf)
{
	if (rf->fp != NULL) {
		fclose(rf->fp);
		free(rf);
	} else {
		OH_ResourceManager_CloseRawFile(rf->rawFile);
		free(rf);
	}
}

//
// Write
//

//
// Open a file output stream.
//
bool
hal_open_wfile(
	const char *file,
	struct hal_wfile **wf)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s", sandboxDir, file);

	FILE *fp = fopen(file, "wb");
	if (fp == NULL)
		return false;

	*wf = (struct hal_wfile *)malloc(sizeof(struct hal_wfile));
	if (*wf == NULL) {
		fclose(fp);
		return false;
	}

	(*wf)->fp = fp;

	return true;
}

//
// Write to a file output stream.
//
bool
hal_write_wfile(
	struct hal_wfile *wf,
	const void *buf,
	size_t size,
	size_t *ret)
{
	size_t written = fwrite(buf, 1, size, wf->fp);
	if (written == 0 && size > 0)
		return false;

	*ret = written;

	return true;
}

//
// Close a file output stream.
//
void
hal_close_wfile(
	struct hal_wfile *wf)
{
	if (wf != NULL) {
		fclose(wf->fp);
		free(wf);
	}
}

//
// Remove a file.
//
bool
hal_remove_file(
	const char *file)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s", sandboxDir, file);

	return remove(file) == 0;
}
