/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Game Subsystem
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
#include "api.h"
#include "cmd.h"
#include "conf.h"
#include "gui.h"
#include "mixer.h"
#include "stage.h"
#include "sysbtn.h"
#include "tag.h"
#include "text.h"
#include "vars.h"

#include <playfield/playfield.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#define FREE(x) \
	do { \
		if (x != NULL) { \
			free(x); \
			x = NULL; \
		} \
	} while (0)

#define STRDUP(x, y) \
	do { \
		x = strdup(y); \
		if (x == NULL) { \
			s3_log_out_of_memory(); \
			return false;\
		} \
	} while (0)

/*
 * Input States
 */

static int mouse_pos_x;
static int mouse_pos_y;
static bool is_mouse_left_pressed;
static bool is_mouse_right_pressed;
static bool is_mouse_left_clicked;
static bool is_mouse_right_clicked;
static bool is_mouse_dragging;
static bool is_return_key_pressed;
static bool is_space_key_pressed;
static bool is_escape_key_pressed;
static bool is_up_key_pressed;
static bool is_down_key_pressed;
static bool is_left_key_pressed;
static bool is_right_key_pressed;
static bool is_pageup_key_pressed;
static bool is_pagedown_key_pressed;
static bool is_control_key_pressed;
static bool is_s_key_pressed;
static bool is_l_key_pressed;
static bool is_h_key_pressed;
static bool is_touch_canceled;
static bool is_swiped;

/*
 * Internal Game States
 */

/* Is in a multiple-frame command execution? */
static bool is_in_repetition;

/* Is message showing? */
static bool flag_message_active;

/* Is executing auto-mode? */
static bool flag_auto_mode;

/* Is executing skip-mode? */
static bool flag_skip_mode;

/* Is save/load allowed? */
static bool flag_save_load_enabled = true;

/* Is non-interruptible mode? */
static bool flag_non_interruptible;

/* Pen position. */
static int saved_pen_x;
static int saved_pen_y;

/* Calling arguments. */
static char *call_arg[S3_CALL_ARGS];

/* Stored message for the page mode. */
static char *buffered_message;

/* Lines in a page. */
static int page_line;

/* BGVoice */
static char *bgvoice;
static bool flag_bgvoice_playing;

/* Chapter title. */
static char *chapter_name;

/* Last message. */
static char *last_message;

/* Previous last message. (last_message minus the latest continued message) */
static char *prev_last_message;

/* Speed of showing text. */
static float msg_text_speed;

/* Speed of the auto mode. */
static float msg_auto_speed;

/* Index of the last English tag. */
static int last_en_index;

/* Tag call stack. */
static int stack_pointer;
static char *stack_file[S3_CALL_STACK_MAX];
static int stack_index[S3_CALL_STACK_MAX];

/* Last tag name. */
static const char *last_tag_name;

/*
 * Forward declaration.
 */
static bool dispatch_update(bool *should_continue);
static bool dispatch_render(void);

/*
 * Called before the game starts.
 */
bool
pf_init_hook(
	int width,
	int height)
{
	conf_game_width = width;
	conf_game_height = height;

	/* Install the default API. */
	if (!s3i_install_default_api())
		return false;

	/* Install the tag functions. */
	if (!s3i_install_tag_funcs())
		return false;

	return true;
}

/*
 * Called when the game starts.
 */
bool
s3i_on_game_start(void)
{
	int i;

	/* Initialize the pseudo random number. */
	srand((unsigned int)time(NULL));

	/* Initialize the config subsystem. */
	if (!s3i_init_conf())
		return false;

	/* Initialize the variable subsystem. */
	if (!s3i_init_vars())
		return false;

	/* Initialize the stage subsystem. */
	if (!s3i_init_stage())
		return false;

	/* Initialize the sysbtn subsystem. */
	if (!s3i_init_sysbtn())
		return false;

	/* Initialize the mixer subsystem. */
	if (!s3i_init_mixer())
		return false;

	/* Initialize the tag subsystem. */
	if (!s3i_init_tag())
		return false;

	/* Initialize the text subsystem. */
	if (!s3i_init_text())
		return false;

	/* Initialize the game states. */
	is_in_repetition = false;
	flag_message_active = false;
	flag_auto_mode = false;
	flag_save_load_enabled = true;
	flag_non_interruptible = false;
	saved_pen_x = false;
	saved_pen_y = false;
	page_line = 0;
	flag_bgvoice_playing = false;
	msg_text_speed = 0.5f;
	msg_auto_speed = 0.5f;
	last_en_index = -1;
	FREE(buffered_message);
	FREE(bgvoice);
	FREE(chapter_name);
	FREE(last_message);
	FREE(prev_last_message);
	for (i = 0; i < S3_CALL_ARGS; i++)
		FREE(call_arg[i]);

	/* Clear the input states. */
	mouse_pos_x = 0;
	mouse_pos_y = 0;
	is_mouse_left_pressed = false;
	is_mouse_right_pressed = false;
	is_mouse_left_clicked = false;
	is_mouse_right_clicked = false;
	is_mouse_dragging = false;
	is_return_key_pressed = false;
	is_space_key_pressed = false;
	is_escape_key_pressed = false;
	is_up_key_pressed = false;
	is_down_key_pressed = false;
	is_pageup_key_pressed = false;
	is_pagedown_key_pressed = false;
	is_control_key_pressed = false;
	is_s_key_pressed = false;
	is_l_key_pressed = false;
	is_h_key_pressed = false;

	return true;
}

/*
 * Called when a frame is updated.
 */
bool
s3i_on_game_update(void)
{
	int s3_continue;
	bool tag_end;
	const char *tag_name;

	mouse_pos_x = pf_mouse_pos_x;
	mouse_pos_y = pf_mouse_pos_y;
	is_mouse_left_clicked = pf_is_mouse_left_clicked;
	is_mouse_right_clicked = pf_is_mouse_right_clicked;
	is_mouse_dragging = pf_is_mouse_dragging;
	is_space_key_pressed = pf_is_space_key_pressed;
	is_return_key_pressed = pf_is_return_key_pressed;
	is_escape_key_pressed = pf_is_escape_key_pressed;
	is_up_key_pressed = pf_is_up_key_pressed;
	is_down_key_pressed = pf_is_down_key_pressed;
	is_left_key_pressed = pf_is_left_key_pressed;
	is_right_key_pressed = pf_is_right_key_pressed;
	is_s_key_pressed = pf_is_s_key_pressed;
	is_l_key_pressed = pf_is_l_key_pressed;
	is_h_key_pressed = pf_is_h_key_pressed;
	is_touch_canceled = pf_is_touch_canceled;
	is_swiped = pf_is_swiped;

	/* Start Kirakira effect. */
	if (is_mouse_left_clicked || is_mouse_right_clicked)
		s3i_start_kirakira(mouse_pos_x, mouse_pos_y);

	/* Call tags. */
	while (1) {
		/* If a GUI is running, do not run a tag. */
		if (s3_is_gui_running())
			break;

		/* Clear the continue flag. */
		s3_set_vm_int("s3Continue", 0);

		/* Save the last tag name. */
		if (s3_get_tag_index() < s3_get_tag_count())
			tag_name = s3_get_tag_name();
		else
			tag_name = NULL;

		/* Call the tag function. */
		if (!s3_call_vm_tag_function(&tag_end)) {
			/* Error: will stop the game after the next rendering. */
			pf_set_vm_int("exitFlag", 1);
			break;
		}
		if (tag_end) {
			/* Finished: stop the game after the next rendering. */
			pf_set_vm_int("exitFlag", 1);
			break;
		}

		/* Save the last tag name. */
		last_tag_name = tag_name;

		/* Check the continue flag. */
		s3_get_vm_int("s3Continue", &s3_continue);
		if (s3_continue) {
			/* Process the next tag. */
			continue;
		}

		break;
	} 

	/* Update the GUI if running. */
	if (s3_is_gui_running()) {
		bool is_gui_tag;

		/* Check if this is a gui tag execution. (if not, it's a system GUI!) */
		is_gui_tag = s3_is_in_command_repetition();

		if (!s3i_run_gui_update()) {
			/* Error: will stop the game after the next rendering. */
			pf_set_vm_int("exitFlag", 1);
		}

		if (s3_is_gui_finished()) {
			if (is_gui_tag) {
				/*
				 * If the GUI is triggered by the "gui" tag, do a cleanup.
				 * Call the tag function with no parameter.
				 */
				s3i_tag_gui(NULL);
			} else {
				/* Otherwise, stop the system GUI. */
				s3_stop_gui();
			}
		}
	}

	/* Do a sound fading. */
	process_sound_fading();

	/* Reset input states to avoid keyboard repeat. */
	pf_is_return_key_pressed = false;
	pf_is_escape_key_pressed = false;
	pf_is_up_key_pressed = false;
	pf_is_down_key_pressed = false;
	pf_is_left_key_pressed = false;
	pf_is_right_key_pressed = false;
	pf_is_s_key_pressed = false;
	pf_is_l_key_pressed = false;
	pf_is_h_key_pressed = false;

	return true;
}

/*
 * Called when a frame is rendering.
 */
bool
s3i_on_game_render(void)
{
	s3_render_stage();

	if (conf_kirakira_enable)
		s3i_render_kirakira();

	return true;
}

/*
 * Input State
 */

/*
 * Get the mouse X position.
 */
int
s3_get_mouse_pos_x(void)
{
	return mouse_pos_x;
}

/*
 * Get the mouse Y position.
 */
int
s3_get_mouse_pos_y(void)
{
	return mouse_pos_y;
}

/*
 * Check if mouse left button is pressed.
 */
bool
s3_is_mouse_left_pressed(void)
{
	return is_mouse_left_pressed;
}

/*
 * Check if mouse right button is pressed.
 */
bool
s3_is_mouse_right_pressed(void)
{
	return is_mouse_right_pressed;
}

/*
 * Check if mouse left button is pressed then released.
 */
bool
s3_is_mouse_left_clicked(void)
{
	return is_mouse_left_clicked;
}

/*
 * Check if mouse right button is pressed then released.
 */
bool
s3_is_mouse_right_clicked(void)
{
	return is_mouse_right_clicked;
}

/*
 * Check if mouse is dragging.
 */
bool
s3_is_mouse_dragging(void)
{
	return is_mouse_dragging;
}

/*
 * Check if return key is pressed.
 */
bool
s3_is_return_key_pressed(void)
{
	return is_return_key_pressed;
}

/*
 * Check if space key is pressed.
 */
bool
s3_is_space_key_pressed(void)
{
	return is_space_key_pressed;
}

/*
 * Check if escape key is pressed.
 */
bool
s3_is_escape_key_pressed(void)
{
	return is_escape_key_pressed;
}

/*
 * Check if up key is pressed.
 */
bool
s3_is_up_key_pressed(void)
{
	return is_up_key_pressed;
}

/*
 * Check if down key is pressed.
 */
bool
s3_is_down_key_pressed(void)
{
	return is_down_key_pressed;
}

/*
 * Check if left key is pressed.
 */
bool
s3_is_left_key_pressed(void)
{
	return is_left_key_pressed;
}

/*
 * Check if right key is pressed.
 */
bool
s3_is_right_key_pressed(void)
{
	return is_right_key_pressed;
}

/*
 * Check if pageup key is pressed.
 */
bool
s3_is_pageup_key_pressed(void)
{
	return is_pageup_key_pressed;
}

/*
 * Check if pagedown key is pressed.
 */
bool
s3_is_pagedown_key_pressed(void)
{
	return is_pagedown_key_pressed;
}

/*
 * Check if control key is pressed.
 */
bool
s3_is_control_key_pressed(void)
{
	return is_control_key_pressed;
}

/*
 * Check if S key is pressed.
 */
bool
s3_is_s_key_pressed(void)
{
	return is_s_key_pressed;
}

/*
 * Check if L key is pressed.
 */
bool
s3_is_l_key_pressed(void)
{
	return is_l_key_pressed;
}

/*
 * Check if H key is pressed.
 */
bool
s3_is_h_key_pressed(void)
{
	return is_h_key_pressed;
}

/*
 * Check if touch is canceled.
 */
bool
s3_is_touch_canceled(void)
{
	return is_touch_canceled;
}

/*
 * Check if swiped.
 */
bool
s3_is_swiped(void)
{
	return is_swiped;
}

/*
 * Clear input states to avoid further input processing in the current frame.
 */
void
s3_clear_input_state(void)
{
	is_mouse_left_pressed = false;
	is_mouse_right_pressed = false;
	is_mouse_left_clicked = false;
	is_mouse_right_clicked = false;
	is_return_key_pressed = false;
	is_up_key_pressed = false;
	is_down_key_pressed = false;
	is_left_key_pressed = false;
	is_right_key_pressed = false;
	is_escape_key_pressed = false;
}

/*
 * Start a multiple-frame command execution.
 */
void
s3_start_command_repetition(void)
{
	assert(!is_in_repetition);
	is_in_repetition = true;
}

/*
 * Stop a multiple-frame command execution.
 */
void
s3_stop_command_repetition(void)
{
	assert(is_in_repetition);
	is_in_repetition = false;
}

/*
 * Check whether we are in a multiple-frame command execution or not.
 */
bool
s3_is_in_command_repetition(void)
{
	return is_in_repetition;
}

/*
 * Set the message showing state.
 *  - Set active when a message shown.
 *  - Keep active when move to a GUI.
 *  - Cleared when move to a next command or loaded a save file.
 */
void
s3_set_message_active(void)
{
	assert(!flag_message_active);
	flag_message_active = true;
}

/*
 * Reset the message showing state.
 */
void
s3_clear_message_active(void)
{
	assert(flag_message_active);
	flag_message_active = false;
}

/*
 * Check whether the message showing state is set or not.
 */
bool
s3_is_message_active(void)
{
	return flag_message_active;
}

/*
 * Start the auto-mode.
 */
void
s3_start_auto_mode(void)
{
	assert(!flag_auto_mode);
	assert(!flag_skip_mode);
	flag_auto_mode = true;
}

/*
 * Stop the auto-mode.
 */
void
s3_stop_auto_mode(void)
{
	assert(flag_auto_mode);
	assert(!flag_skip_mode);
	flag_auto_mode = false;
}

/*
 * Check whether we are in the auto-mode or not.
 */
bool
s3_is_auto_mode(void)
{
	return flag_auto_mode;
}

/*
 * Start the skip-mode.
 */
void
s3_start_skip_mode(void)
{
	assert(!flag_skip_mode);
	assert(!flag_auto_mode);
	flag_skip_mode = true;
}

/*
 * Stop the skip-mode.
 */
void
s3_stop_skip_mode(void)
{
	assert(flag_skip_mode);
	assert(!flag_auto_mode);
	flag_skip_mode = false;
}

/*
 * Check whether we are in the skip-mode or not.
 */
bool
s3_is_skip_mode(void)
{
	return flag_skip_mode;
}

/*
 * Set the save/load enable setting.
 */
void
s3_set_save_load(
	bool enable)
{
	flag_save_load_enabled = enable;
}

/*
 * Get the save/load enable setting.
 */
bool
s3_is_save_load_enabled(void)
{
	return flag_save_load_enabled;
}

/*
 * Set the non-interruptible mode setting.
 */
void
s3_set_non_interruptible(
	bool mode)
{
	flag_non_interruptible = mode;
}

/*
 * Get the non-interruptible mode setting.
 */
bool
s3_is_non_interruptible(void)
{
	return flag_non_interruptible;
}

/*
 * Set the pen position.
 */
void
s3_set_pen_position(
	int x,
	int y)
{
	saved_pen_x = x;
	saved_pen_y = y;
}

/*
 * Get the pen X position.
 */
int
s3_get_pen_position_x(void)
{
	return saved_pen_x;
}

/*
 * Get the pen Y position.
 */
int
s3_get_pen_position_y(void)
{
	return saved_pen_y;
}

/*
 * Push the return point.
 */
bool
s3_push_for_call(
	const char *file,
	int index)
{
	if (stack_pointer == S3_CALL_STACK_MAX) {
		s3_log_tag_error(S3_TR("Too many macro calls."));
		return false;
	}

	stack_file[stack_pointer] = strdup(file);
	if (stack_file[stack_pointer] == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	stack_index[stack_pointer] = index;
	stack_pointer++;

	return true;
}

/*
 * Pop the return point.
 */
bool
s3_pop_for_return(
	char **file,
	int *index)
{
	if (stack_pointer == 0) {
		s3_log_tag_error(S3_TR("Too many macro calls."));
		return false;
	}
	stack_pointer--;

	*file = strdup(stack_file[stack_pointer]);
	if (file == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	free(stack_file[stack_pointer]);
	*index = stack_index[stack_pointer];

	return true;
}

/*
 * Read the call stack.
 */
bool
s3_read_call_stack(
	int sp,
	char **file,
	int *index)
{
	*file = strdup(stack_file[stack_pointer]);
	if (*file == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	*index = stack_index[stack_pointer];
	return true;
}

/*
 * Write the call stack.
 */
bool
s3_write_call_stack(
	int sp,
	const char *file,
	int index)
{
	assert(sp < S3_CALL_STACK_MAX);

	if (stack_file[stack_pointer] != NULL)
		free(stack_file[stack_pointer]);
	stack_file[stack_pointer] = strdup(file);
	if (stack_file[stack_pointer] == NULL) {
		s3_log_out_of_memory();
		return false;
	}
	stack_index[stack_pointer] = index;

	return true;
}

/*
 * Set a calling argument.
 */
bool
s3_set_call_argument(
	int index,
	const char *val)
{
	assert(index < S3_CALL_ARGS);

	FREE(call_arg[index]);
	if (val != NULL)
		STRDUP(call_arg[index], val);

	return true;
}

/*
 * Get a calling argument.
 */
const char *
s3_get_call_argument(
	int index)
{
	assert(index < S3_CALL_ARGS);

	if (call_arg[index] == NULL)
		return "";

	return call_arg[index];
}

/*
 * Check if the script page mode is enabled.
 */
bool
s3_is_page_mode(void)
{
	return conf_game_novel;
}

/*
 * Append a string to the page mode buffer string.
 */
bool
s3_append_buffered_message(
	const char *msg)
{
	char *s;
	size_t len;

	len = 0;
	if (buffered_message != NULL)
		len += strlen(buffered_message);
	len += strlen(msg);

	s = malloc(len + 1);
	if (s == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	strcpy(s, "");
	if (buffered_message != NULL)
		strcat(s, buffered_message);
	strcat(s, msg);

	FREE(buffered_message);

	buffered_message = s;

	return true;
}

/*
 * Get the page mode buffer string.
 */
const char *
s3_get_buffered_message(void)
{
	if (buffered_message == NULL)
		return "";

	return buffered_message;
}

/*
 * Clear the page mode buffer string.
 */
void
s3_clear_buffered_message(void)
{
	FREE(buffered_message);
}

/*
 * Reset the message line count in a page.
 */
void
s3_reset_page_line(void)
{
	page_line = 0;
}

/*
 * Increment the line count in a page.
 */
void
s3_inc_page_line(void)
{
	page_line++;
}

/*
 * Check if we are at the first line in a page.
 */
bool
s3_is_page_top(void)
{
	if (page_line == 0)
		return true;

	return false;
}

/*
 * Register a BGVoice.
 */
bool
s3_register_bgvoice(
	const char *file)
{
	FREE(bgvoice);

	if (file != NULL)
		STRDUP(bgvoice, file);

	return true;
}

/*
 * Get the BGVoice.
 */
const char *
s3_get_bgvoice(void)
{
	return bgvoice;
}

/*
 * Set the BGVoice state playing.
 */
void
s3_set_bgvoice_playing(
	bool is_playing)
{
	flag_bgvoice_playing = is_playing;
}

/*
 * Check if the BGVoice is playing.
 */
bool
s3_is_bgvoice_playing(void)
{
	return flag_bgvoice_playing;
}

/*
 * Set the chapter name.
 */
bool
s3_set_chapter_name(
	const char *name)
{
	free(chapter_name);

	STRDUP(chapter_name, name);

	return true;
}

/*
 * Get the chapter name.
 */
const char *
s3_get_chapter_name(void)
{
	if (chapter_name == NULL)
		return "";

	return chapter_name;
}

/*
 * Set the last message.
 */
bool
s3_set_last_message(
	const char *msg,
	bool is_append)
{
	FREE(prev_last_message);

	/* If a continued line. */
	if (is_append && last_message != NULL) {
		char *new_text;
		size_t new_len = 0;
		size_t next_len = 0;

		new_len = strlen(last_message) + strlen(msg);
		new_text = malloc(new_len + 1);
		if (new_text == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		strcpy(new_text, last_message);
		strcpy(new_text, msg);

		prev_last_message = last_message;
		last_message = new_text;
		return true;
	}

	/* Otherwise. */
	FREE(last_message);
	STRDUP(last_message, msg);

	return true;
}

/*
 * Set the previous last message.
 */
bool
s3_set_prev_last_message(
	const char *msg)
{
	FREE(prev_last_message);
	STRDUP(prev_last_message, msg);
	return true;
}

/*
 * Get the last message.
 */
const char *
s3_get_last_message(void)
{
	if (last_message == NULL)
		return "";
	return last_message;
}

/*
 * Get the previous last message.
 */
const char *
s3_get_prev_last_message(void)
{
	if (prev_last_message == NULL)
		return "";
	return prev_last_message;
}

/*
 * Set the text speed.
 */
void
s3_set_text_speed(
	float val)
{
	assert(val >= 0 && val <= 1.0f);

	msg_text_speed = val;
}

/*
 * Get the text speed.
 */
float
s3_get_text_speed(void)
{
	return msg_text_speed;
}

/*
 * Set the auto speed.
 */
void
s3_set_auto_speed(
	float val)
{
	assert(val >= 0 && val <= 1.0f);

	msg_auto_speed = val;
}

/*
 * Get the auto speed.
 */
float
s3_get_auto_speed(void)
{
	return msg_auto_speed;
}

/*
 * Mark the last English index.
 */
void
s3_mark_last_english_tag_index(void)
{
	last_en_index = s3_get_tag_index();
}

/*
 * Clear the last English index.
 */
void
s3_clear_last_english_tag_index(void)
{
	last_en_index = -1;
}

/*
 * Get the last English index.
 */
int
s3_get_last_english_tag_index(void)
{
	return last_en_index;
}

/*
 * Clear the last English index.
 */
void
s3_clear_last_en_index(void)
{
	last_en_index = -1;
}

/*
 * Get the last tag name.
 */
const char *
s3_get_last_tag_name(void)
{
	return last_tag_name;
}

/*
 * Embedded License String
 */

const char license_info[]
#ifdef __GNUC__
 /* Don't remove this string even if it's not referenced. */
 __attribute__((used))
#endif
 = "Suika3: Copyright (C) 2001-2026 The Suika3 Authors\n"
   "zlib: Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler. All rights reserved.\n"
   "libpng: Copyright (C) 2000-2002, 2004, 2006-2016, Glenn Randers-Pehrson and the original authors. All rights reserved.\n"
   "jpeg: copyright (C) 1991-2022, Thomas G. Lane, Guido Vollbeding. All rights reserved.\n"
   "bzip2: Copyright (C) 1996-2010 Julian R Seward. All rights reserved.\n"
   "libwebp: Copyright (C) 2010, Google Inc. All rights reserved.\n"
   "libogg: Copyright (C) 2002, Xiph.org Foundation. All rights reserved.\n"
   "libvorbis: Copyright (C) 2002-2015, Xiph.org Foundation. All rights reserved.\n"
   "brotli: Copyright (c) 2009, 2010, 2013-2016 by the Brotli Authors. All rights reserved.\n"
   "FreeType2: Copyright (C) 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg. All rights reserved.\n";
