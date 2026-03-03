/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Wrappers for APIs of Playfield Engine and NoctLang
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
#include <noct/noct.h>

#include <stdio.h>
#include <stdarg.h>

/*
 * Lap Timer
 */

/*
 * Reset a lap timer and initializes it with a current time.
 */
void
s3_reset_lap_timer(
	uint64_t *origin)
{
	pf_reset_lap_timer(origin);
}

/*
 * Get a lap time in milliseconds.
 */
uint64_t
s3_get_lap_timer_millisec(
	uint64_t *origin)
{
	return pf_get_lap_timer_millisec(origin);
}

/*
 * Raw File I/O
 */

/* Check if a file exists. */
bool
s3_check_file_exists(
	const char *fname)
{
	return pf_check_file_exists(fname);
}

/* Read a file. */
bool
s3_read_file_content(
	const char *fname,
	char **buf,
	size_t *len)
{
	return pf_read_file_content(fname, buf, len);
}

/*
 * Save Data
 */

/*
 * Write save data.
 */
bool
s3_write_save_data(
	const char *key,
	const void *data,
	size_t size)
{
	return pf_write_save_data(key, data, size);
}

/*
 * Read save data.
 */
bool
s3_read_save_data(
	const char *key,
	void *data,
	size_t size,
	size_t *ret)
{
	return pf_read_save_data(key, data, size, ret);
}

/*
 * Check whether save data exist or not.
 */
bool
s3_check_save_data(
	const char *key)
{
	return pf_check_save_data(key);
}

/*
 * Get the size of save data.
 */
size_t
s3_get_save_data_size(
	const char *key)
{
	size_t ret;

	if (!pf_get_save_data_size(key, &ret))
		return 0;

	return ret;
}

/*
 * VM Access
 */

/*
 * Install an API function.
 */
bool
s3_install_api(
	const char *name,
	bool (*func)(void *),
	bool no_args)
{
	const char *params[] = {"param"};
	char full_name[256];
	NoctEnv *env;
	NoctValue dict;
	NoctValue funcval;

	env = pf_get_vm_env();

	/* If the "Suika" variable does not exist. */
	if (!noct_check_global(env, "Suika")) {
		/* Make a global variable "Suika". */
		if (!noct_make_empty_dict(env, &dict))
			return false;
		if (!noct_set_global(env, "Suika", &dict))
			return false;
	} else {
		if (!noct_get_global(env, "Suika", &dict))
			return false;
	}

	/* Register a cfunc. */
	snprintf(full_name, sizeof(full_name), "Suika.%s", name);
	if (!no_args) {
		if (!noct_register_cfunc(env, full_name, 1, params, (void *)func, NULL))
			return false;
	} else {
		if (!noct_register_cfunc(env, full_name, 0, NULL, (void *)func, NULL))
			return false;
	}

	/* Get a function value. */
	if (!noct_get_global(env, full_name, &funcval))
		return false;

	/* Add to the "Suika" API dictionary. */
	if (!noct_set_dict_elem(env, &dict, name, &funcval))
		return false;

	return true;
}

/*
 * Install a tag function.
 */
bool
s3_install_tag(
	const char *name,
	bool (*func)(void *))
{
	const char *params[] = {"param"};
	NoctEnv *env;

	env = pf_get_vm_env();

	/* Register a cfunc. */
	if (!noct_register_cfunc(env, name, 1, params, (void *)func, NULL))
		return false;

	return true;
}

/*
 * Get a VM integer. (Suika.*)
 */
bool
s3_get_vm_int(
	const char *name,
	int *val)
{
	NoctEnv *env;
	NoctValue dict;
	NoctValue value;

	env = pf_get_vm_env();

	if (!noct_get_global(env, "Suika", &dict))
			return false;
	if (!noct_get_dict_elem_check_int(env, &dict, name, &value,val))
		return false;

	return true;
}

/*
 * Set a VM integer. (Suika.*)
 */
bool
s3_set_vm_int(
	const char *name,
	int val)
{
	NoctEnv *env;
	NoctValue dict;
	NoctValue value;

	env = pf_get_vm_env();

	if (!noct_get_global(env, "Suika", &dict))
			return false;
	if (!noct_make_int(env, &value, val))
		return false;
	if (!noct_set_dict_elem(env, &dict, name, &value))
		return false;

	return true;
}

/*
 * Set a VM float variable. (Suika.*)
 */
bool
s3_set_vm_float(
	const char *name,
	float val)
{
	NoctEnv *env;
	NoctValue dict;
	NoctValue value;

	env = pf_get_vm_env();
	if (!noct_get_global(env, "Suika", &dict))
			return false;
	if (!noct_make_float(env, &value, val))
		return false;
	if (!noct_set_dict_elem(env, &dict, name, &value))
		return false;

	return true;
}

/*
 * Set a VM string variable. (Suika.*)
 */
bool
s3_set_vm_string(
	const char *name,
	const char *val)
{
	NoctEnv *env;
	NoctValue dict;
	NoctValue value;

	env = pf_get_vm_env();
	if (!noct_get_global(env, "Suika", &dict))
			return false;
	if (!noct_make_string(env, &value, val))
		return false;
	if (!noct_set_dict_elem(env, &dict, name, &value))
		return false;

	return true;
}

/*
 * Call a VM function.
 */
bool
s3_call_vm_function(
	const char *name)
{
	return pf_call_vm_function(name);
}

/*
 * Call a VM function that corresponds to the current tag.
 */
bool
s3_call_vm_tag_function(
	bool *tag_end)
{
	NoctEnv *env;
	struct pfi_tag *t;
	NoctValue dict;
	int i;
	int prop_count;
	char func_name[256];
	const char *tag_name;
	NoctValue func_val;
	NoctFunc *func;
	NoctValue ret;

	env = pf_get_vm_env();

	*tag_end = false;

	/* Get a current command. */
	if (s3_get_tag_index() == s3_get_tag_count()) {
		/* Reached to the end. Finish the game loop. */
		*tag_end = true;
		return true;
	}

	/* Make a parameter dictionary. */
	if (!noct_make_empty_dict(env, &dict)) {
		s3_log_error(S3_TR("%s:%d: Runtime error."),
			     s3_get_tag_file(),
			     s3_get_tag_line());
		return false;
	}

	/* Setup properties as dictionary items. */
	prop_count = s3_get_tag_property_count();
	for (i = 0; i < prop_count; i++) {
		NoctValue str;
		const char *prop_name;
		const char *prop_value;

		prop_name = s3_get_tag_property_name(i);
		prop_value = s3_get_tag_property_value(i);

		if (!noct_make_string(env, &str, prop_value)) {
			s3_log_error(S3_TR("%s:%d: Runtime error."),
				     s3_get_tag_file(),
				     s3_get_tag_line());
			return false;
		}
		if (!noct_set_dict_elem(env, &dict, prop_name, &str)) {
			s3_log_error(S3_TR("%s:%d: Runtime error."),
				     s3_get_tag_file(),
				     s3_get_tag_line());
			return false;
		}
	}

	/* Make a tag function name. */
	tag_name = s3_get_tag_name();
	snprintf(func_name, sizeof(func_name), "Tag_%s", tag_name);

	/* Get a corresponding function.  */
	if (!noct_get_global(env, func_name, &func_val)) {
		s3_log_error(S3_TR("%s:%d: Tag \"%s\" not found."),
			     s3_get_tag_file(),
			     s3_get_tag_line(),
			     tag_name);
		return false;
	}
	if (!noct_get_func(env, &func_val, &func)) {
		s3_log_error(S3_TR("%s:%d: \"Tag_%s\" is not a function."),
			     s3_get_tag_file(),
			     s3_get_tag_line(),
			     tag_name);
		return false;
	}

	/* Call the function. */
	if (!noct_enter_vm(env, func_name, 1, &dict, &ret)) {
		const char *file;
		int line;
		const char *msg;

		noct_get_error_file(env, &file);
		noct_get_error_line(env, &line);
		noct_get_error_message(env, &msg);
		return false;
	}

	return true;
}

/*
 * Touch
 */

/*
 * Enable/disable message skip by touch move.
 */
void
s3_set_continuous_swipe_enabled(
	bool is_enabled)
{
	pf_set_continuous_swipe_enabled(is_enabled);
}

/*
 * Full Screen Mode
 */

/*
 * Return whether we support the full screen mode.
 */
bool
s3_is_full_screen_supported(void)
{
	return pf_is_full_screen_supported();
}

/*
 * Return whether the we are in the full screen mode.
 */
bool
s3_is_full_screen_mode(void)
{
	return pf_is_full_screen_mode();
}

/*
 * Enter the full screen mode.
 */
void
s3_enter_full_screen_mode(void)
{
	pf_enter_full_screen_mode();
}

/*
 * Leave the full screen mode.
 */
void
s3_leave_full_screen_mode(void)
{
	pf_leave_full_screen_mode();
}

/*
 * Video
 */

/*
 * Play a video.
 */
bool
s3_play_video(
	const char *file,
	bool is_skippable)
{
	return pf_play_video(file, is_skippable);
}

/*
 * Stop the video.
 */
void
s3_stop_video(void)
{
	pf_stop_video();
}

/*
 * Check if a video is playing back.
 */
bool
s3_is_video_playing(void)
{
	return pf_is_video_playing();
}

/*
 * Locale
 */

/*
 * Get the system language.
 */
const char *
s3_get_system_language(void)
{
	return pf_get_system_language();
}

/*
 * Logging
 */

/*
 * Print a debug message.
 */
void
s3_log_info(
	const char *msg,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, msg);
	vsnprintf(buf, sizeof(buf), msg, ap);
	va_end(ap);

	pf_log_info(buf);
}

/*
 * Print a warning message.
 */
void
s3_log_warn(
	const char *msg,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, msg);
	vsnprintf(buf, sizeof(buf), msg, ap);
	va_end(ap);

	pf_log_warn(buf);
}

/*
 * Print an error message.
 */
void
s3_log_error(
	const char *msg,
	...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, msg);
	vsnprintf(buf, sizeof(buf), msg, ap);
	va_end(ap);

	pf_log_error(buf);
}

/*
 * Print an out-of-memory error message.
 */
void
s3_log_out_of_memory(void)
{
	pf_log_error("Out-of-memory.");
}

/*
 * Print a log footer for execution error.
 */
void
s3_log_tag_error(
	const char *msg,
	...)
{
	char buf[4096];
	va_list ap;
	int n;

	n = snprintf(buf,
		     sizeof(buf),
		     S3_TR("%s:%d:%s: "),
		     s3_get_tag_file(),
		     s3_get_tag_line(),
		     s3_get_tag_name());
		     
	va_start(ap, msg);
	vsnprintf(buf + n, sizeof(buf) - n, msg, ap);
	va_end(ap);

	pf_log_error(buf);
}

/*
 * Misc.
 */

/*
 * Text-to-speech.
 */
void
s3_speak_text(
	const char *msg)
{
	UNUSED_PARAMETER(msg);
}
