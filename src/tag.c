/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Tag Subsystem
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2026 The Suika3 Community
 * Copyright (c) 2025-2026 The Playfield Engine Project
 * Copyright (c) 2025-2026 The NoctVM Project
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 2016-2024 The Suika2 Development Team
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is derived from the codebase of Playfield Engine, NoctLang,
 * Suika2, Suika Studio, Wind Game Lib, and 98/AT Game Lib.
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
#include "tag.h"

#include <playfield/playfield.h>

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

/*
 * Maximum properties in a tag.
 */
#define PROP_MAX	128

/*
 * Command struct.
 */
struct s3i_tag {
	char *tag_name;
	int prop_count;
	char *prop_name[PROP_MAX];
	char *prop_value[PROP_MAX];
	char *prop_value_eval[PROP_MAX];
	int line;
};

/*
 * Tag execution stack element.
 */
struct s3i_tag_stack {
	int type;
	int start;
};

/* Current tag file. */
static char cur_file[1024];

/* Current tag index. */
static int cur_index;

/* Tag table. */
static struct s3i_tag tag[TAG_MAX];

/* Tag size. */
static int tag_size;

/* Tag execution stack. */
static struct s3i_tag_stack tag_stack[STACK_MAX];
static int stack_pointer;

/* Evaluation buffer. */
static char eval_buf[65536];

/* Forward declaration. */
static const char *evaluate_prop_value(const char *prop_value);
static bool parse_tag_document(const char *doc, bool (*callback)(const char *, int, const char **, const char **, int), char **error_msg, int *error_line);
static bool parse_tag_callback(const char *name, int props, const char **prop_name, const char **prop_value, int line);

/*
 * Initialize the tag subsystem.
 */
bool
s3i_init_tag(void)
{
	s3i_cleanup_tag();

	if (!s3_move_to_tag_file(S3_PATH_START_TAG))
		return false;

	return true;
}

/*
 * Cleanup the tag subsystem.
 */
void
s3i_cleanup_tag(void)
{
	struct s3i_tag *t;
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
			if (t->prop_value_eval[i] != NULL) {
				free(t->prop_value_eval[i]);
				t->prop_value_eval[i] = NULL;
			}
		}
	}

	tag_size = 0;
	stack_pointer = 0;

	memset(tag, 0, sizeof(tag));
}

/*
 * Move to a tag file.
 */
bool
s3_move_to_tag_file(const char *file)
{
	char *buf;
	char *error_message;
	int error_line;

	/* Get the file content. */
	if (!s3_read_file_content(file, &buf, NULL))
		return false;

	/* Destroy the existing commands. */
	s3i_cleanup_tag();

	/* Save the file name. */
	strncpy(cur_file, file, sizeof(cur_file) - 1);

	/* Parse the file content. */
	if (!parse_tag_document(buf, parse_tag_callback, &error_message, &error_line)) {
		s3_log_error(S3_TR("%s:%d: %s\n"),  file, error_line, error_message);
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
s3_get_tag_file(void)
{
	return &cur_file[0];
}

/*
 * Get the index of the current tag.
 */
int
s3_get_tag_index(void)
{
	if (cur_index >= tag_size)
		return tag_size;
	
	return cur_index;
}

/*
 * Get the line number of the current tag.
 */
int
s3_get_tag_line(void)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return -1;
	
	return tag[cur_index].line;
}

/*
 * Get the tag count.
 */
int
s3_get_tag_count(void)
{
	return tag_size;
}

/*
 * Move to the next tag.
 */
bool
s3_move_to_next_tag(void)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return false;

	if (cur_index + 1 >= tag_size) {
		cur_index = tag_size;
		return true;
	}

	cur_index++;

	return true;
}

/*
 * Move to a tag index.
 */
bool
s3_move_to_tag_index(
	int index)
{
	assert(index < tag_size);
	if (index >= tag_size)
		return false;

	cur_index = index;

	return true;
}

/*
 * Move to a label.
 */
bool
s3_move_to_label_tag(
	const char *label)
{
	int i, j;

	/* Search tags. */
	for (i = 0; i < tag_size; i++) {
		if (strcmp(tag[i].tag_name, "label") != 0)
			continue;

		/* Check the "name" propery. */
		for (j = 0; j < tag[i].prop_count; j++) {
			if (strcmp(tag[i].prop_name[j], "name") != 0)
				continue;
			if (strcmp(tag[i].prop_value[j], label) != 0)
				continue;

			/* Found. */
			cur_index = i + 1;
			return true;
		}
	}

	/* Not found. */
	s3_log_tag_error(S3_TR("Label \"%s\" not found."), label);
	return false;
}

/*
 * Move to a matched else (or elseif or endif) tag.
 */
bool
s3_move_to_else_tag(void)
{
	int depth;
	int i;

	depth = 0;

	for (i = cur_index + 1; i < tag_size; i++) {
		if (strcmp(tag[i].tag_name, "endif") == 0) {
			if (depth == 0) {
				cur_index = i + 1;
				return true;
			}
			depth--;
			continue;
		}
		if (strcmp(tag[i].tag_name, "elseif") == 0) {
			if (depth == 0) {
				cur_index = i;
				return true;
			}
			continue;
		}
		if (strcmp(tag[i].tag_name, "else") == 0) {
			if (depth == 0) {
				cur_index = i + 1;
				return true;
			}
			continue;
		}
		if (strcmp(tag[i].tag_name, "if") == 0) {
			depth++;
			continue;
		}
	}

	s3_log_error(S3_TR("No matching endif found."));
	return false;
}

/*
 * Move to a matched endif tag.
 */
bool
s3_move_to_endif_tag(void)
{
	int depth;
	int i;

	depth = 0;

	for (i = cur_index + 1; i < tag_size; i++) {
		if (strcmp(tag[i].tag_name, "endif") == 0) {
			if (depth == 0) {
				cur_index = i + 1;
				return true;
			}
			depth--;
			continue;
		}
		if (strcmp(tag[i].tag_name, "if") == 0) {
			depth++;
			continue;
		}
	}

	s3_log_error(S3_TR("No matching endif found."));
	return false;
}

/*
 * Arguments
 */

/*
 * Get the name of the current tag.
 */
const char *
s3_get_tag_name(void)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return NULL;

	assert(tag[cur_index].tag_name != NULL);
	return tag[cur_index].tag_name;
}

/*
 * Get the property count of the current tag.
 */
int
s3_get_tag_property_count(void)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return 0;

	assert(tag[cur_index].prop_count >= 0);
	return tag[cur_index].prop_count;
}

/*
 * Get the property name of the current tag.
 */
const char *
s3_get_tag_property_name(
	int index)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return NULL;

	assert(index < tag[cur_index].prop_count);
	if (index >= tag[cur_index].prop_count)
		return NULL;

	return tag[cur_index].prop_name[index];
}

/*
 * Get the property value of the current tag.
 */
const char *
s3_get_tag_property_value(
	int index)
{
	assert(cur_index < tag_size);
	if (cur_index >= tag_size)
		return NULL;

	assert(index < tag[cur_index].prop_count);
	if (index >= tag[cur_index].prop_count)
		return false;

	/* If there is an evaluated value. */
	if (tag[cur_index].prop_value_eval[index] != NULL)
		return tag[cur_index].prop_value_eval[index];

	return tag[cur_index].prop_value[index];
}

/*
 * Check if a tag argument exists.
 */
bool
s3_check_tag_arg(
	const char *name)
{
	int i, count;
	const char *prop_name;

	count = s3_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = s3_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			return true;
	}

	return false;
}

/*
 * Get an integer tag argument.
 */
int
s3_get_tag_arg_int(
	const char *name,
	bool omissible,
	int def_val)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;
	int val;

	count = s3_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = s3_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found. */
		if (omissible)
			return def_val;
		s3_log_error(S3_TR("Argument \"%s\" not specified."), name);
		return def_val;
	}

	prop_value = s3_get_tag_property_value(i);
	val = atoi(prop_value);

	return val;
}

/*
 * Get a float tag argument.
 */
float
s3_get_tag_arg_float(
	const char *name,
	bool omissible,
	float def_val)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;
	float val;

	count = s3_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = s3_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found. */
		if (omissible)
			return def_val;
		s3_log_error(S3_TR("Argument \"%s\" not specified."), name);
		return def_val;
	}

	prop_value = s3_get_tag_property_value(i);
	val = (float)atof(prop_value);

	return val;
}

/*
 * Get a string tag argument.
 */
const char *
s3_get_tag_arg_string(
	const char *name,
	bool omissible,
	const char *def_val)
{
	int i, count;
	const char *prop_name;
	const char *prop_value;

	count = s3_get_tag_property_count();

	for (i = 0; i < count; i++) {
		prop_name = s3_get_tag_property_name(i);
		if (strcmp(name, prop_name) == 0)
			break;
	}
	if (i == count) {
		/* Not found. */
		if (omissible)
			return def_val;
		s3_log_error(S3_TR("Argument \"%s\" not specified."), name);
		return def_val;
	}

	prop_value = s3_get_tag_property_value(i);

	return prop_value;
}

/*
 * Evaluate property values of the current tag.
 */
bool
s3_evaluate_tag(void)
{
	int i;
	const char *e;

	for (i = 0; i < tag[cur_index].prop_count; i++) {
		if (tag[cur_index].prop_value_eval[i] != NULL) {
			free(tag[cur_index].prop_value_eval[i]);
			tag[cur_index].prop_value_eval[i] = NULL;
		}

		e = evaluate_prop_value(tag[cur_index].prop_value[i]);
		if (e == NULL)
			return false;

		if (strcmp(e, tag[cur_index].prop_value[i]) != 0) {
			tag[cur_index].prop_value_eval[i] = strdup(e);
			if (tag[cur_index].prop_value_eval[i] == NULL) {
				s3_log_out_of_memory();
				return false;
			}
		}
	}

	return true;
}

/* Parser for inline variables. (`${var}` format) */
static const char *
evaluate_prop_value(
	const char *prop_value)
{
	const char *src;
	char *dst;
	bool is_escape1;
	bool is_escape2;
	int empty;

	memset(eval_buf, 0, sizeof(eval_buf));

	src = prop_value;
	dst = eval_buf;
	is_escape1 = false;
	is_escape2 = false;
	empty = sizeof(eval_buf) - 1;

	while (*src != '\0') {
		/* '$' */
		if (!is_escape1 && !is_escape2) {
			if (*src == '$') {
				is_escape1 = true;
				src++;
				continue;
			} else {
				*dst++ = *src++;
				if (--empty == 0)
					return eval_buf;
				continue;
			}
		}

		/* '{' */
		if (is_escape1 && !is_escape2) {
			if (*src == '{') {
				is_escape1 = false;
				is_escape2 = true;
				src++;
				continue;
			} else {
				is_escape1 = false;
				is_escape2 = false;
				*dst++ = '$';
				if (--empty == 0)
					return eval_buf;
				*dst++ = *src++;
				if (--empty == 0)
					return eval_buf;
			}
		}

		/* var */
		if (!is_escape1 && is_escape2) {
			char *v;
			int vempty;
			char var_name[1024];
			const char *var_value;

			memset(var_name, 0, sizeof(var_name));

			v = var_name;
			vempty = sizeof(var_name) - 1;

			while (*src != '\0') {
				if (*src == '}')
					break;
				*v++ = *src++;
				if (--vempty == 0)
					return eval_buf;
			}
			if (*src == '\0')
				return eval_buf;

			var_value = s3_get_variable_string(var_name);
			if (var_value != NULL) {
				while(*var_value != '\0') {
					*dst++ = *var_value++;
					if (--empty == 0) {
						*dst = '\0';
						return eval_buf;
					}
				}
			}

			is_escape1 = false;
			is_escape2 = false;
			src++;
			continue;
		}

		*dst++ = *src++;
		if (--empty == 0)
			return eval_buf;
	}

	return eval_buf;
}

/*
 * Push an "if" to the tag stack.
 */
bool
s3_push_tag_stack_if(void)
{
	if (stack_pointer >= STACK_MAX) {
		s3_log_error(S3_TR("Too many nests."));
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
s3_pop_tag_stack_if(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_IF) {
		s3_log_error(S3_TR("No correspoinding \"if\" detected."));
		return false;
	}

	stack_pointer--;
	return true;
}

/*
 * Push a "while loop" to the tag stack.
 */
bool
s3_push_tag_stack_while(void)
{
	if (stack_pointer >= STACK_MAX) {
		s3_log_error(S3_TR("Too many nests."));
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
s3_pop_tag_stack_while(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_WHILE) {
		s3_log_error(S3_TR("No correspoinding \"while\" detected."));
		return false;
	}

	stack_pointer--;
	return true;
}

/*
 * Push a "for loop" to the tag stack.
 */
bool
s3_push_tag_stack_for(void)
{
	if (stack_pointer >= STACK_MAX) {
		s3_log_error(S3_TR("Too many nests."));
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
s3_pop_tag_stack_for(void)
{
	if (stack_pointer == 0 ||
	    tag_stack[stack_pointer].type != TYPE_FOR) {
		s3_log_error(S3_TR("No correspoinding \"for\" detected."));
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
		ST_COMMENT,
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
			if (c == '#') {
				state = ST_COMMENT;
				continue;
			}

			*error_msg = strdup(S3_TR("Invalid character."));
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
					*error_msg = strdup(S3_TR("Too many properties."));
					*error_line = line;
					return false;
				}
				state = ST_INIT;
				prop_count = 0;
				continue;
			}
			if (len >= TAG_NAME_MAX) {
				*error_msg = strdup(S3_TR("Tag name too long."));
				*error_line = line;
				return false;
			}
			tag_name[len++] = c;
			continue;
		case ST_PROPNAME:
			if (prop_count == PROP_MAX) {
				*error_msg = strdup(S3_TR("Too many properties."));
				*error_line = line;
				return false;
			}
			if (len == 0 && c == ' ')
				continue;
			if (len == 0 && c == ']') {
				if (!callback(tag_name, prop_count, (const char **)prop_name_tbl, (const char **)prop_val_tbl, line)) {
					*error_msg = strdup(S3_TR("Internal error."));
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
				*error_msg = strdup(S3_TR("Property name too long."));
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
			*error_msg = strdup(S3_TR("Invalid character."));
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
				*error_msg = strdup(S3_TR("Property value too long."));
				*error_line = line;
				return false;
			}
			prop_val[prop_count][len++] = c;
			continue;
		case ST_COMMENT:
			if (c == '\n') {
				state = ST_INIT;
				line++;
				continue;
			}
			continue;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	}

	if (state == ST_INIT)
		return true;

	*error_msg = strdup(S3_TR("Unexpected EOF"));
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
	struct s3i_tag *t;
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
