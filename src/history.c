/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * History Subsystem
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 1996-2026 Awe Morris / SCHOLA SUIKAE
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

#include <suika3/suika3.h>
#include "history.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HISTORY_SIZE	(100)

static struct history {
	char *name;
	char *text;
	char *voice;
	s3_pixel_t name_color;
	s3_pixel_t name_outline_color;
	s3_pixel_t body_color;
	s3_pixel_t body_outline_color;
} history[HISTORY_SIZE];

static int history_count;
static int history_top;		/* Ring buffer top. */
static int last_history_top;

/* Forward declaration. */
static int get_history_index(int offset);

/*
 * Initialize the history subsystem.
 */
bool
s3i_init_history(void)
{
	s3_clear_history();

	last_history_top = -1;

	return true;
}

/*
 * Cleanup the history subsystem.
 */
void
s3i_cleanup_history(void)
{
	s3_clear_history();
}

/*
 * Clear the history.
 */
void
s3_clear_history(void)
{
	int i;

	for (i = 0; i < HISTORY_SIZE; i++) {
		if (history[i].name != NULL) {
			free(history[i].name);
			history[i].name = NULL;
		}
		if (history[i].text != NULL) {
			free(history[i].text);
			history[i].text = NULL;
		}
		if (history[i].voice != NULL) {
			free(history[i].voice);
			history[i].voice = NULL;
		}
	}

	history_count = 0;
	history_top = 0;
	last_history_top = -1;
}

/*
 * Add a history.
 */
bool
s3_add_history(
	const char *name,
	const char *text,
	const char *voice,
	s3_pixel_t body_color,
	s3_pixel_t body_outline_color,
	s3_pixel_t name_color,
	s3_pixel_t name_outline_color)
{
	struct history *h;

	/* Maybe LF-only? Exclude. */
	if (strcmp(text, "") == 0)
		return true;

	/* Get the position to store. */
	h = &history[history_top];

	/* Clear the previous information. */
	if (h->name != NULL) {
		free(h->name);
		h->name = NULL;
	}
	if (h->text != NULL) {
		free(h->text);
		h->text = NULL;
	}
	if (h->voice != NULL) {
		free(h->voice);
		h->voice = NULL;
	}

	/* Store. */
	if (name != NULL && strcmp(name, "") != 0) {
		h->name = strdup(name);
		if (h->name == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	}
	if (voice != NULL && strcmp(voice, "") != 0) {
		h->voice = strdup(voice);
		if (h->voice == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	}
	h->text = strdup(text);
	if (h->text == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	/* Mask the alpha channel off. */
	h->body_color = body_color & s3_make_pixel(0xff, 0, 0, 0);
	h->body_outline_color = body_outline_color & s3_make_pixel(0xff, 0, 0, 0);
	h->name_color = name_color & s3_make_pixel(0xff, 0, 0, 0);
	h->name_outline_color = name_outline_color & s3_make_pixel(0xff, 0, 0, 0);;

	/* Update the position. */
	last_history_top = history_top;
	history_top = (history_top + 1) % HISTORY_SIZE;
	history_count = (history_count + 1) >= HISTORY_SIZE ? HISTORY_SIZE : (history_count + 1);

	return true;
}

/*
 * Append to the last history.
 */
bool
s3_append_history(
	const char *text,
	const char *spacing)
{
	struct history *h;
	char *s;
	size_t len;

	h = &history[last_history_top];

	len = 0;
	if (h->text != NULL)
		len += strlen(h->text);
	len += strlen(spacing);
	len += strlen(text);

	s = malloc(len + 1);
	if (s == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	strcpy(s, "");
	if (h->text != NULL)
		strcat(s, h->text);
	strcat(s, spacing);
	strcat(s, text);

	free(h->text);

	h->text = s;

	return true;
}

/*
 * Get the number of the history.
 */
int
s3_get_history_count(void)
{
	return history_count;
}

/*
 * Get the name at the history index.
 */
const char *
s3_get_history_name(
	int offset)
{
	int index;

	index = get_history_index(offset);

	return history[index].name;
}

/*
 * Get the text at the history index.
 */
const char *
s3_get_history_text(
	int offset)
{
	int index;

	index = get_history_index(offset);

	return history[index].text;
}

/*
 * Get the voice file at the history index.
 */
const char *
s3_get_history_voice(
	int offset)
{
	int index;

	index = get_history_index(offset);

	return history[index].voice;
}

/* Convert from a history offset to an index of the ring buffer.  */
static int
get_history_index(
	int offset)
{
	int index;

	assert(offset >= 0);
	assert(offset < history_count);

	if (history_top - offset - 1 < 0)
		index = HISTORY_SIZE + (history_top - offset - 1);
	else
		index = history_top - offset - 1;

	assert(index >= 0 && index < history_count);

	return index;
}
