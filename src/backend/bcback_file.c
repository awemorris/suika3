/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Transactional bytecode backend output.
 */

#include "bcback_file.h"

#include <noct/noct.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_WINDOWS)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#elif defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define BCBACK_TEMPORARY_ATTEMPTS	1024

static unsigned long bcback_temporary_serial;

static void bcback_output_reset(struct bcback_output *output);
static bool bcback_output_make_temporary_path(const char *final_path, unsigned long serial, char **temporary_path);
static FILE *bcback_output_create_exclusive(const char *path);
#if defined(NOCT_TARGET_WINDOWS)
static bool bcback_output_acp_to_wide(const char *path, wchar_t **wide_path);
#endif
static bool bcback_output_replace(const char *temporary_path, const char *final_path);

/*
 * Opens a unique sibling output without changing the destination file.
 */
bool
bcback_output_open(
	struct bcback_output *output,
	const char *final_path)
{
	unsigned long serial;
	unsigned int attempt;

	assert(output != NULL);

	if (output->stream != NULL ||
	    output->final_path != NULL ||
	    output->temporary_path != NULL) {
		return false;
	}
	if (final_path == NULL || final_path[0] == '\0')
		return false;

	output->final_path = noct_strdup(final_path);
	if (output->final_path == NULL)
		return false;

	/* Try distinct sibling names until one exclusive creation succeeds. */
	for (attempt = 0; attempt < BCBACK_TEMPORARY_ATTEMPTS; attempt++) {
		serial = bcback_temporary_serial;
		bcback_temporary_serial++;

		if (!bcback_output_make_temporary_path(
			final_path,
			serial,
			&output->temporary_path)) {
			bcback_output_abort(output);
			return false;
		}

		output->stream = bcback_output_create_exclusive(
			output->temporary_path);
		if (output->stream != NULL)
			return true;

		noct_free(output->temporary_path);
		output->temporary_path = NULL;
		if (errno != EEXIST) {
			bcback_output_abort(output);
			return false;
		}
	}

	bcback_output_abort(output);

	return false;
}

/*
 * Returns the borrowed stream owned by one open transaction.
 */
FILE *
bcback_output_get_stream(
	struct bcback_output *output)
{
	assert(output != NULL);

	return output->stream;
}

/*
 * Flushes and atomically replaces the destination with one complete output.
 */
bool
bcback_output_commit(
	struct bcback_output *output)
{
	bool succeeded;

	assert(output != NULL);

	if (output->stream == NULL ||
	    output->final_path == NULL ||
	    output->temporary_path == NULL) {
		bcback_output_abort(output);
		return false;
	}

	succeeded = true;
	if (fflush(output->stream) != 0)
		succeeded = false;
	if (ferror(output->stream) != 0)
		succeeded = false;
	if (fclose(output->stream) != 0)
		succeeded = false;
	output->stream = NULL;

	if (succeeded) {
		succeeded = bcback_output_replace(
			output->temporary_path,
			output->final_path);
	}

	if (!succeeded) {
		remove(output->temporary_path);
		bcback_output_reset(output);
		return false;
	}

	bcback_output_reset(output);

	return true;
}

/*
 * Aborts an output transaction and removes its temporary file.
 */
void
bcback_output_abort(
	struct bcback_output *output)
{
	if (output == NULL)
		return;

	if (output->stream != NULL) {
		fclose(output->stream);
		output->stream = NULL;
	}
	if (output->temporary_path != NULL)
		remove(output->temporary_path);

	bcback_output_reset(output);
}

/* Release all owned paths and restore the zero state. */
static void
bcback_output_reset(
	struct bcback_output *output)
{
	noct_free(output->temporary_path);
	noct_free(output->final_path);
	memset(output, 0, sizeof(*output));
}

/* Build one checked sibling temporary path. */
static bool
bcback_output_make_temporary_path(
	const char *final_path,
	unsigned long serial,
	char **temporary_path)
{
	char suffix[64];
	int suffix_size;
	size_t final_size;
	size_t total_size;

	assert(final_path != NULL);
	assert(temporary_path != NULL);

	suffix_size = snprintf(
		suffix,
		sizeof(suffix),
		".tmp.%lu",
		serial);
	if (suffix_size < 0 || (size_t)suffix_size >= sizeof(suffix))
		return false;

	final_size = strlen(final_path);
	if (final_size > SIZE_MAX - (size_t)suffix_size - 1)
		return false;
	total_size = final_size + (size_t)suffix_size + 1;

	*temporary_path = noct_malloc(total_size);
	if (*temporary_path == NULL)
		return false;

	memcpy(*temporary_path, final_path, final_size);
	memcpy(
		*temporary_path + final_size,
		suffix,
		(size_t)suffix_size + 1);

	return true;
}

/* Create one binary stream without replacing an existing temporary file. */
static FILE *
bcback_output_create_exclusive(
	const char *path)
{
#if defined(NOCT_TARGET_WINDOWS)
	int descriptor;
	FILE *stream;

	descriptor = _open(
		path,
		_O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
		_S_IREAD | _S_IWRITE);
	if (descriptor < 0)
		return NULL;

	stream = _fdopen(descriptor, "w+b");
	if (stream == NULL)
		_close(descriptor);

	return stream;
#elif defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
	int descriptor;
	FILE *stream;

	descriptor = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (descriptor < 0)
		return NULL;

	stream = fdopen(descriptor, "w+b");
	if (stream == NULL)
		close(descriptor);

	return stream;
#else
	FILE *probe;

	probe = fopen(path, "rb");
	if (probe != NULL) {
		fclose(probe);
		errno = EEXIST;
		return NULL;
	}

	return fopen(path, "w+b");
#endif
}

#if defined(NOCT_TARGET_WINDOWS)
/* Convert one current-ACP narrow path to a newly allocated wide path. */
static bool
bcback_output_acp_to_wide(
	const char *path,
	wchar_t **wide_path)
{
	int length;

	length = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
	if (length <= 0)
		return false;
	if ((size_t)length > SIZE_MAX / sizeof(**wide_path))
		return false;

	*wide_path = noct_malloc((size_t)length * sizeof(**wide_path));
	if (*wide_path == NULL)
		return false;

	if (MultiByteToWideChar(CP_ACP, 0, path, -1, *wide_path, length) == 0) {
		noct_free(*wide_path);
		*wide_path = NULL;
		return false;
	}

	return true;
}
#endif

/* Atomically replace one destination using the platform path contract. */
static bool
bcback_output_replace(
	const char *temporary_path,
	const char *final_path)
{
#if defined(NOCT_TARGET_WINDOWS)
	wchar_t *wide_temporary;
	wchar_t *wide_final;
	bool succeeded;

	wide_temporary = NULL;
	wide_final = NULL;
	succeeded = false;

	if (!bcback_output_acp_to_wide(temporary_path, &wide_temporary))
		goto cleanup;
	if (!bcback_output_acp_to_wide(final_path, &wide_final))
		goto cleanup;

	if (MoveFileExW(
		wide_temporary,
		wide_final,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
		goto cleanup;
	}

	succeeded = true;

cleanup:
	noct_free(wide_final);
	noct_free(wide_temporary);

	return succeeded;
#else
	return rename(temporary_path, final_path) == 0;
#endif
}
