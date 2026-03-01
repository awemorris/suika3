/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Tag file
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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

#include "tag.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False assertion */
#define NEVER_COME_HERE		0

/* Sizes. */
#define TAG_NAME_MAX		128
#define PROP_NAME_MAX		128
#define PROP_VALUE_MAX		4096
#define TAG_MAX			65536
#define STACK_MAX		128

/* Stack element type. */
#define TYPE_IF			1
#define TYPE_FOR		2
#define TYPE_WHILE		3

/* Current tag file. */
static char cur_file[1024];

/* Current tag index. */
static int cur_index;

/* Tag table. */
static struct pfi_tag tag[TAG_MAX];

/* Tag size. */
static int tag_size;

/* Tag execution stack. */
static struct pfi_tag_stack tag_stack[STACK_MAX];
static int stack_pointer;

/* Forward declaration. */
static bool parse_tag_document(const char *doc, bool (*callback)(const char *, int, const char **, const char **, int), char **error_msg, int *error_line);
static bool parse_tag_callback(const char *name, int props, const char **prop_name, const char **prop_value, int line);

/*
 * Initialize the tag system.
 */
void
pfi_init_tag(void)
{
	pfi_cleanup_tag();
}

/*
 * Cleanup the tag system.
 */
void
pfi_cleanup_tag(void)
{
	struct pfi_tag *t;
	int i, j;

	cur_index = 0;

	strcpy(cur_file, "");

	for (i = 0; i < tag_size; i++) {
		t = &tag[i];
		free(t->tag_name);
		for (j = 0; j < PROP_MAX; j++) {
			if (t->prop_name[i] != NULL) {
				free(t->prop_name[i]);
				t->prop_name[i] = NULL;
			}
			if (t->prop_value[i] != NULL) {
				free(t->prop_value[i]);
				t->prop_value[i] = NULL;
			}
		}
	}

	stack_pointer = 0;
}

/*
 * Load a tag file.
 */
bool
pfi_load_tag_file(
	const char *file)
{
	char *buf;
	char *error_message;
	int error_line;

	/* Get the file content. */
	if (!pfi_load_file(file, &buf, NULL))
		return false;

	/* Destroy the existing commands. */
	pfi_cleanup_tag();

	/* Save the file name. */
	strncpy(cur_file, file, sizeof(cur_file) - 1);

	/* Parse the file content. */
	if (!parse_tag_document(buf, parse_tag_callback, &error_message, &error_line)) {
		hal_log_error(PF_TR("%s:%d: %s\n"),  file, error_line, error_message);
		free(buf);
		return false;
	}

	/* Free the file content. */
	free(buf);

	return true;
}

/*
 * Get the file name of the current tag.
 */
const char *
pfi_get_tag_file_name(void)
{
	return &cur_file[0];
}

/*
 * Get the tag count.
 */
int
pfi_get_tag_count(void)
{
	return tag_size;
}

/*
 * Get the index of the current tag.
 */
int
pfi_get_tag_index(void)
{
	/* If the current tag index is invalid. */
	if (cur_index >= tag_size)
		return -1;
	
	return cur_index;
}

/*
 * Get the line number of the current tag.
 */
int
pfi_get_tag_line(void)
{
	/* If the current tag index is invalid. */
	if (cur_index >= tag_size)
		return -1;
	
	return tag[cur_index].line;
}

/*
 * Get the current tag.
 */
struct pfi_tag *
pfi_get_current_tag(void)
{
	/* If the current command index is invalid. */
	if (cur_index >= tag_size)
		return NULL;
	
	return &tag[cur_index];
}

/*
 * Move to the next tag.
 */
bool
pfi_move_to_next_tag(void)
{
	if (cur_index + 1 >= tag_size) {
		cur_index = tag_size;
		return true;
	}

	cur_index++;

	return true;
}

/*
 * Move to a label tag.
 */
bool
pfi_move_to_label_tag(
	const char *label)
{
	int tag_count, prop_count;
	int i, j;

	/* Search tags. */
	for (i = 0; i < tag_count; i++) {
		if (strcmp(tag[i].tag_name, "label") != 0)
			continue;

		/* Check the "name" propery. */
		for (j = 0; j < tag[i].prop_count; j++) {
			if (strcmp(tag[i].prop_name[j], "name") != 0)
				continue;
			if (strcmp(tag[i].prop_value[j], label) != 0)
				continue;

			/* Found. */
			cur_index = i;
			return true;
		}
	}

	/* Not found. */
	return false;
}

/*
 * Move to a tag by index.
 */
bool
pfi_move_to_tag_index(
	int index)
{
	if (index >= tag_size)
		return false;

	cur_index = index;

	return true;
}

/*
 * Push an "if" to the tag stack.
 */
bool
pfi_push_tag_stack_if(void)
{
	if (stack_pointer >= STACK_MAX) {
		pf_log_error(PF_TR("Too many nests."));
		return false;
	}

	tag_stack[stack_pointer].type = TYPE_IF;
	tag_stack[stack_pointer].start = cur_index;
	return true;
}

/*
 * Pop an "if" from the tag stack.
 */
bool
pfi_pop_tag_stack_if(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_IF) {
		pf_log_error(PF_TR("No correspoinding \"if\" detected."));
		return false;
	}

	stack_pointer--;
	return true;
}

/*
 * Push a "while loop" to the tag stack.
 */
bool
pfi_push_tag_stack_while(void)
{
	if (stack_pointer >= STACK_MAX) {
		pf_log_error(PF_TR("Too many nests."));
		return false;
	}

	tag_stack[stack_pointer].type = TYPE_WHILE;
	tag_stack[stack_pointer].start = cur_index;
	return true;
}

/*
 * Pop a "while loop" from the tag stack.
 */
bool
pfi_pop_tag_stack_while(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_WHILE) {
		pf_log_error(PF_TR("No correspoinding \"while\" detected."));
		return false;
	}

	stack_pointer--;
	return true;
}

/*
 * Push a "for loop" to the tag stack.
 */
bool
pfi_push_tag_stack_for(void)
{
	if (stack_pointer >= STACK_MAX) {
		pf_log_error(PF_TR("Too many nests."));
		return false;
	}

	tag_stack[stack_pointer].type = TYPE_FOR;
	tag_stack[stack_pointer].start = cur_index;
	return true;
}

/*
 * Pop a "for loop" from the tag stack.
 */
bool
pfi_pop_tag_stack_for(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_FOR) {
		pf_log_error(PF_TR("No correspoinding \"for\" detected."));
		return false;
	}

	stack_pointer--;
	return true;
}

/* Parse a tag document. */
static bool
parse_tag_document(
	const char *doc,
	bool (*callback)(const char *, int, const char **, const char **, int),
	char **error_msg,
	int *error_line)
{
	static char tag_name[TAG_NAME_MAX];
	static char prop_name[PROP_MAX][PROP_NAME_MAX];
	static char prop_val[PROP_MAX][PROP_VALUE_MAX];

	/* State machine */
	enum state {
		ST_INIT,
		ST_TAGNAME,
		ST_PROPNAME,
		ST_PROPVALUE_QUOTE,
		ST_PROPVALUE_BODY,
	};

	const char *top;
	char c;
	int state;
	int line;
	int len;
	int prop_count;
	char *prop_name_tbl[PROP_MAX];
	char *prop_val_tbl[PROP_MAX];
	int i;

	for (i = 0; i < PROP_MAX; i++) {
		prop_name_tbl[i] = &prop_name[i][0];
		prop_val_tbl[i] = &prop_val[i][0];
	}

	state = ST_INIT;
	top = doc;
	line = 1;
	len = 0;
	prop_count = 0;
	while (*top != '\0') {
		c = *top++;
		switch (state) {
		case ST_INIT:
			if (c == '[') {
				state = ST_TAGNAME;
				len = 0;
				continue;
			}
			if (c == '\n') {
				line++;
				continue;
			}
			if (c == ' ' || c == '\r' || c == '\t')
				continue;

			*error_msg = strdup(PF_TR("Invalid character."));
			*error_line = line;
			return false;
		case ST_TAGNAME:
			if (len == 0 && (c == ' ' || c == '\r' || c == '\t' || c == '\n'))
				continue;
			if (c == '\n')
				line++;
			if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
				assert(len > 0);
				tag_name[len] = '\0';
				state = ST_PROPNAME;
				len = 0;
				continue;
			}
			if (c == ']') {
				tag_name[len] = '\0';
				if (!callback(tag_name, 0, NULL, NULL, line)) {
					*error_msg = strdup(PF_TR("Too many properties."));
					*error_line = line;
					return false;
				}
				state = ST_INIT;
				prop_count = 0;
				continue;
			}
			if (len >= TAG_NAME_MAX) {
				*error_msg = strdup(PF_TR("Tag name too long."));
				*error_line = line;
				return false;
			}
			tag_name[len++] = c;
			continue;
		case ST_PROPNAME:
			if (prop_count == PROP_MAX) {
				*error_msg = strdup(PF_TR("Too many properties."));
				*error_line = line;
				return false;
			}
			if (len == 0 && c == ' ')
				continue;
			if (len == 0 && c == ']') {
				if (!callback(tag_name, prop_count, (const char **)prop_name_tbl, (const char **)prop_val_tbl, line)) {
					*error_msg = strdup(PF_TR("Internal error."));
					*error_line = line;
					return false;
				}
				state = ST_INIT;
				prop_count = 0;
				continue;
			}
			if (len == 0 && c == '\n')
				line++;
			if (len == 0 && (c == ' ' || c == '\r' || c == '\t' || c == '\n'))
				continue;
			if (len > 0 && c == '=') {
				assert(len > 0);

				/* Terminate the property name. */
				prop_name[prop_count][len] = '\0';

				state = ST_PROPVALUE_QUOTE;
				len = 0;
				continue;
			}
			if (len >= PROP_NAME_MAX) {
				*error_msg = strdup(PF_TR("Property name too long."));
				*error_line = line;
				return false;
			}
			if ((c >= 'a' && c <= 'z') ||
			    (c >= 'A' && c <= 'Z') ||
			    (c >= '0' && c <= '9') ||
			    c == '-' ||
			    c == '_') {
				prop_name[prop_count][len++] = c;
				continue;
			}
			*error_msg = strdup(PF_TR("Invalid character."));
			*error_line = line;
			continue;
		case ST_PROPVALUE_QUOTE:
			if (c == '\n')
				line++;
			if (c == ' ' || c == '\r' || c == '\t' || c == '\n')
				continue;
			if (c == '\"') {
				state = ST_PROPVALUE_BODY;
				len = 0;
				continue;
			}
			continue;
		case ST_PROPVALUE_BODY:
			if (c == '\\') {
				switch (*top) {
				case '\"':
					prop_val[prop_count][len] = '\"';
					len++;
					top++;
					continue;
				case 'n':
					prop_val[prop_count][len] = '\n';
					len++;
					top++;
					continue;
				case '\\':
					prop_val[prop_count][len] = '\\';
					len++;
					top++;
					continue;
				default:
					prop_val[prop_count][len] = '\\';
					len++;
					continue;
				}
			}
			if (c == '\"') {
				prop_val[prop_count][len] = '\0';
				prop_count++;

				state = ST_PROPNAME;
				len = 0;
				continue;
			}
			if (len >= PROP_VALUE_MAX) {
				*error_msg = strdup(PF_TR("Property value too long."));
				*error_line = line;
				return false;
			}
			prop_val[prop_count][len++] = c;
			continue;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	}

	if (state == ST_INIT)
		return true;

	*error_msg = strdup(PF_TR("Unexpected EOF"));
	*error_line = line;
	return false;
}

/* Callback for when a tag is read. */
static bool
parse_tag_callback(
	const char *name,
	int props,
	const char **prop_name,
	const char **prop_value,
	int line)
{
	struct pfi_tag *t;
	int i;

	/* If command table is full. */
	if (tag_size >= TAG_MAX) {
		hal_log_error("Too many tags.");
		return false;
	}

	t = &tag[tag_size++];
	t->prop_count = props;

	/* Copy a tag name. */
	t->tag_name = strdup(name);
	if (t->tag_name == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	/* Copy properties. */
	for (i = 0; i < props; i++) {
		t->prop_name[i] = strdup(prop_name[i]);
		if (t->prop_name[i] == NULL) {
			hal_log_out_of_memory();
			return false;
		}

		t->prop_value[i] = strdup(prop_value[i]);
		if (t->prop_value[i] == NULL) {
			hal_log_out_of_memory();
			return false;
		}
	}

	t->line = line;

	return true;
}
