/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "text" tag implementation
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
#include "conf.h"
#include "text.h"
#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/*
 * [Note]
 *  - When returning from the system GUI, we reuse the already rendered message layer as-is (gui_sys_flag)
 */

/* Message speed (x letters/second) */
#define MESSAGE_SPEED	(80.0f)

/*
 * Auto mode wait time when voice is present
 *  - Base time for how long to wait after display and playback completion
 *  - Actually used by multiplying with the auto mode speed coefficient
 */
#define AUTO_MODE_VOICE_WAIT		(4000)

/*
 * Auto mode wait time coefficient for when voice is absent
 *  - Used when automode.speed=0
 *  - Used by multiplying with character count
 */
#define AUTO_MODE_TEXT_WAIT_SCALE	(0.15f)

/*
 * String buffers
 */

/* Name to be drawn */
static char *name_top;

/*
 * Text to be drawn (start of buffer)
 *  - The content of the string is after variable expansion
 *  - In the case of line continuation, the leading '\' is not included
 *  - In the case of line continuation, the leading consecutive "\n" are not included
 */
static char *msg_top;

/*
 * Inline Concatenation
 */
static bool is_inline;

/*
 * Drawing state
 */

/* Drawing state of the main text */
static struct s3_drawmsg *msgbox_context;

/* Whether to draw all due to skipping (avoid inline wait) */
static bool do_draw_all;

/*
 * Total number of characters to be drawn
 *  - Total number of display characters excluding escape sequences
 *  - Unicode composition is not supported
 *    - Base characters + combining characters are each counted as one character
 */
static int total_chars;

/* Number of characters already drawn (0 <= drawn_chars <= total_chars) */
static int drawn_chars;

/*
 * Drawing position
 */

/*
 * Current drawing position
 *  - Coordinates within the message box
 *  - Retained even when switching to other commands or returning from the GUI
 */
static int pen_x;
static int pen_y;

/* Drawing start position (for overlay) */
static int orig_pen_x;
static int orig_pen_y;

/* Position and size of the message box */
static int msgbox_x;
static int msgbox_y;
static int msgbox_w;
static int msgbox_h;

/*
 * Colors
 */

/* Color of the main text */
static s3_pixel_t body_color;

/* Color of the outline of the main text */
static s3_pixel_t body_outline_color;

/* Color of the name text */
static s3_pixel_t name_color;

/* Color of the outline of the name text */
static s3_pixel_t name_outline_color;

/* Background color */
static s3_pixel_t bg_color;

/*
 * Line continuation mode
 */

/* Whether it is line continuation mode */
static bool is_continue_mode;

/* Whether it is overlaying (to ignore color escape sequences in the future) */
static bool is_dimming;

/* Whether to avoid dimming (at load time) */
static bool avoid_dimming;

/*
 * Voice
 */

/* Whether there is voice */
static bool have_voice;

/* Voice file */
static char *voice_file;

/* Whether the voice has been stopped */
static bool is_voice_stopped;

/*
 * Click animation
 */

/* Whether it is the first drawing of the click animation */
static bool is_click_first;

/* Display state of the click animation */
static bool is_click_visible;

/* Stopwatch representing the elapsed time of the command */
static uint64_t click_sw;

/*
 * Auto mode
 */

/* Whether it is waiting after message display and voice playback completion in auto mode */
static bool is_auto_mode_wait;

/* Stopwatch representing the elapsed time of auto mode */
static uint64_t auto_sw;

/*
 * Inline wait
 */

/* Whether it is in inline wait */
static bool is_inline_wait;

/* Inline wait time */
static float inline_wait_time;

/* Total time waited in inline wait */
static float inline_wait_time_total;

/* Stopwatch representing the elapsed time of inline wait */
static uint64_t inline_sw;

/*
 * State when the command started
 */

/* Whether it was started by loading */
static bool load_flag;

/* Whether it has returned from the system GUI */
static bool gui_sys_flag;

/*
 * Hidden
 */

/* Whether hiding by the space key is in progress */
static bool is_hidden;

/*
 * System transition flags
 *  - TODO: Share with cmd_switch.c
 */

/* Whether to transition to system menu mode */
static bool need_sysmenu_mode;

/* Whether to transition to save screen */
static bool need_save_mode;

/* Whether to transition to load screen */
static bool need_load_mode;

/* Whether to transition to history screen */
static bool need_history_mode;

/* Whether to transition to config screen */
static bool need_config_mode;

/*
 * dimming
 */

static bool need_dimming;

/*
 * Others
 */

/* Page top flag. */
static bool is_page_top;

/* Line spacing. */
static const char *line_spacing;

/* Whether to keep in history but not display */
static bool no_show;

/* Last language. (for config language selection) */
static char *last_lang;

/*
 * Forward declarations
 */

/* Main processing */
static bool preprocess(void);
static bool blit_process(void);
static void postprocess(void);

/* Initialization */
static bool init(bool *cont);
static bool init_special_action(bool *exit);
static void clear_msgbox(void);
static void init_flags_and_vars(void);
static void init_auto_mode(void);
static void init_skip_mode(void);
static void init_colors(void);
static bool init_name_top(void);
static bool init_voice_file(void);
static bool init_msg_top(void);
static bool register_message_for_history(const char *msg);
static bool init_msgbox(void);
static bool init_voice(void);
static bool check_play_voice(void);
static bool play_voice(void);
static void set_character_volume_by_name(const char *name);
static bool blit_namebox(void);
static void focus_character(void);
static void init_click(void);
static void init_repetition(void);
static const char *get_localized_text(void);
static const char *get_localized_name(void);
static const char *get_localized_voice(void);
static void speak(void);
static void init_lip_sync(void);
static void cleanup_lip_sync(void);

/* Frame processing */
static bool frame_auto_mode(void);
static bool check_auto_play_condition(void);
static int get_wait_time(void);
static void action_toggle_hide(void);
static bool frame_sysbtn(void);

/* blit drawing processing */
static void blit_frame(void);
static bool is_end_of_msg(void);
static void blit_msgbox(void);
static void inline_wait_hook(float wait_time);
static int get_frame_chars(void);
static bool is_canceled_by_skip(void);
static bool is_fast_forward_by_click(void);
static int calc_frame_chars_by_lap(void);
static void set_click(void);
static bool check_stop_click_animation(void);

/* dimming */
static void blit_dimming(void);

/* Others */
static void play_se(const char *file);
static bool is_skippable(void);

/* Termination processing */
static void stop(void);
static bool cleanup(void);

/*
 * The "text" tag implementation.
 */
bool
s3i_tag_text(
	void *p)
{
	bool cont;

	UNUSED_PARAMETER(p);

	/* Initialization */
	if (!s3_is_in_command_repetition()) {
		if (!init(&cont))
			return false;
		if (cont) {
			/* Chain to the next tag. */
			s3_set_vm_int("s3Continue", 1);
			return s3_move_to_next_tag();
		}
	}
	if (no_show) {
		/* Chain to the next tag. */
		s3_set_vm_int("s3Continue", 1);
		s3_move_to_next_tag();
		return true;
	}

	if (!preprocess())
		return false;
	if (!blit_process())
		return false;
	postprocess();

	/* Termination processing */
	if (!s3_is_in_command_repetition())
		if (!cleanup())
			return false;

	/* Next frame */
	return true;
}

static bool
preprocess(void)
{
	/* Cancel inline wait */
	if (is_inline_wait) {
		if (s3_is_mouse_left_clicked() || s3_is_mouse_right_clicked() ||
		    s3_is_control_key_pressed() || s3_is_space_key_pressed() ||
		    s3_is_return_key_pressed() || s3_is_up_key_pressed() || s3_is_down_key_pressed() ||
		    s3_is_pageup_key_pressed() || s3_is_pagedown_key_pressed() ||
		    s3_is_escape_key_pressed()) {
			is_inline_wait = false;
			s3_clear_input_state();
			return true;
		}
	}

	/* Process auto mode */
	if (frame_auto_mode()) {
		/* Input was captured, so return here */
		return true;
	}

	/* Process system buttons */
	if (frame_sysbtn()) {
		/* Input was captured, so return here */
		return true;
	}

	/* Input was not captured, but there is no error */
	return true;
}

static bool
blit_process(void)
{
	/* If dimming is needed */
	if (need_dimming) {
		blit_dimming();
		/* If not in auto/skip mode */
		if (s3_is_in_command_repetition())
			s3_stop_command_repetition();
		return true;
	}

	/* Draw characters / control click animation */
	blit_frame();

	/* Create a thumbnail of the stage if necessary (system GUI transition) */
	if (need_sysmenu_mode)
		s3_draw_stage_to_thumb();

	/* If system button was pressed */
	if (need_sysmenu_mode) {
		if (!s3_load_gui_file(S3_PATH_SYSMENU_GUI, true))
			return false;
		s3_start_gui();
		return true;
	}

	/* If save/load/history/config buttons were pressed */
	if (need_save_mode) {
		if (!s3_load_gui_file(S3_PATH_SAVE_GUI, true))
			return false;
		s3_start_gui();
		need_save_mode = false;
		return true;
	}
	if (need_load_mode) {
		if (!s3_load_gui_file(S3_PATH_LOAD_GUI, true))
			return false;
		s3_start_gui();
		need_load_mode = false;
		return true;
	}
	if (need_history_mode) {
		if (!s3_load_gui_file(S3_PATH_HISTORY_GUI, true))
			return false;
		s3_start_gui();
		need_history_mode = false;
		return true;
	}
	if (need_config_mode) {
		if (!s3_load_gui_file(S3_PATH_CONFIG_GUI, true))
			return false;
		s3_start_gui();
		need_config_mode = false;
		return true;
	}

	return true;
}

static void
postprocess(void)
{
	/* Clear the flag if loaded */
	if (load_flag)
		load_flag = false;

	/* If transitioning to system GUI */
	if (need_sysmenu_mode)
		stop();
}

/*
 * Initialization
 */

/* Perform initialization */
static bool
init(
     bool *cont)
{
	bool exit_special;

	*cont = false;

	/* Apply inline variable references. */
	s3_evaluate_tag();

	/* Initialize flags */
	init_flags_and_vars();

	/* Restore for load. */
	if (!s3_is_page_mode()) {
		/* If not page mode, fill the msgbox layer by the msgbox image. */
		if (!gui_sys_flag)
			clear_msgbox();
	} else {
		if (load_flag) {
			/* If loaded, draw the last page. */
			clear_msgbox();
			s3i_blit_load_message();
		}
	}

	/* If page mode */
	if (!init_special_action(&exit_special)) {
		return false;
	} else {
		if (exit_special)
			return true;
	}

	/* Initialize if in auto mode */
	init_auto_mode();

	/* Initialize if in skip mode */
	init_skip_mode();

	/* Initialize the colors. */
	init_colors();

	/* Get the name */
	if (!init_name_top())
		return false;

	/* Get the voice file */
	if (!init_voice_file())
		return false;

	/* Get the message */
	if (!init_msg_top())
		return false;

	/* If register to the history but not displayed. */
	if (strcmp(s3_get_tag_arg_string("only-history", true, ""), "true") == 0) {
		no_show = true;
		return true;
	}

	/* Initialize the message box */
	if (!init_msgbox())
		return false;

	/* Initialize the voice. */
	if (!init_voice())
		return false;

	/*
	 * Set the message active state
	 *  - Retained even when entering system GUI
	 *  - Cleared when transitioning from message to next command
	 *  - Cleared when loaded
	 *  - Cleared when returning to title
	 */
	if (!gui_sys_flag) {
		if (!s3_is_message_active())
			s3_set_message_active();
	}

	/* Hide the click animation */
	init_click();

	/* Show the sysbtn if enabled. */
	if (conf_sysbtn_enable)
		s3_enable_sysbtn(true);

	/* Set repetition behavior */
	init_repetition();

	/* Perform text-to-speech */
	speak();

	/* Perform lip sync */
	init_lip_sync();

	/* Enable skip behavior by continuous swipe */
	s3_set_continuous_swipe_enabled(true);

	return true;
}

/* Initialize flags */
static void
init_flags_and_vars(void)
{
	/* Check if just returned from GUI */
	if (s3_check_right_after_sys_gui()) {
		if (last_lang != NULL && strcmp(last_lang, s3_get_system_language()) != 0) {
			/* The language has been changed. */
			gui_sys_flag = false;
		} else if (s3_is_message_active()) {
			/*
			 * In case of returning from system GUI while message is active
			 *  - Set the flag to perform instant display
			 */
			gui_sys_flag = true;
		} else {
			/*
			 * In case of returning from GUI command
			 */
			gui_sys_flag = false;
		}
	} else {
		gui_sys_flag = false;
	}

	/* Check if just loaded */
	load_flag = s3_check_right_after_load();

	/* Set to not hidden by space key */
	is_hidden = false;

	/* Set the state of auto mode */
	is_auto_mode_wait = false;

	/* Clear the state of menu transition */
	need_sysmenu_mode = false;
	need_save_mode = false;
	need_load_mode = false;
	need_history_mode = false;
	need_config_mode = false;

	/* Set to not dimmed (not overlapping paint) */
	need_dimming = false;
	is_dimming = false;
	avoid_dimming = false;

	/* Set to not inline wait state */
	is_inline_wait = false;
	inline_wait_time_total = 0;
	do_draw_all = false;

	/* Set to state before voice stopped */
	is_voice_stopped = false;

	/* If registered in history but not displayed */
	no_show = false;

	/* Get the pen position. */
	pen_x = s3_get_pen_position_x();
	pen_y = s3_get_pen_position_y();

	/* Page top flag. */
	is_page_top = s3_is_page_top();

	/* Others. */
	is_inline = false;
}

/* Initialize for the page mode and special actions. */
static bool
init_special_action(bool *exit)
{
	const char *action;

	action = s3_get_tag_arg_string("action", true, NULL);

	/* If no action specified. */
	if (action == NULL) {
		if (!s3_is_page_mode()) {
			*exit = false;
			return true;
		}

		/* Mark that the first line was processed. */
		s3_inc_page_line();

		/* Continue execution. */
		*exit = false;
		return true;
	}

	if (strcmp(action, "clear") == 0) {
		/* Clear the message box and the name box. */
		clear_msgbox();
		s3_fill_namebox();
		s3_reset_page_line();

		/* Exit. */
		*exit = true;
		return true;
	}

	if (strcmp(action, "new") == 0) {
		/* Clear the message box and the name box. */
		clear_msgbox();
		s3_fill_namebox();
		s3_reset_page_line();

		/* Show the message box. */
		s3_show_msgbox(true);

		/* Exit. */
		*exit = true;
		return true;
	}

	if (strcmp(action, "hide") == 0) {
		/* Show the message box. */
		s3_show_msgbox(false);
		s3_show_namebox(false);

		/* Exit. */
		*exit = true;
		return true;
	}
	
	if (strcmp(action, "show") == 0) {
		/* Hide the message box. */
		s3_show_msgbox(false);

		/* Exit. */
		*exit = true;
		return true;
	}

	if (strcmp(action, "inline") == 0) {
		is_inline = true;

		/* Continue execution. */
		*exit = false;
		return true;
	}

	s3_log_tag_error(S3_TR("Unknown action \"%s\" is specified on the page mode."), action);
	*exit = true;
	return false;
}

static void
clear_msgbox(void)
{
	/* Clear the message box image. */
	s3_fill_msgbox();

	/* Clear the pen position. */
	s3_get_msgbox_rect(&msgbox_x, &msgbox_y, &msgbox_w, &msgbox_h);
	if (!conf_msgbox_font_tategaki) {
		pen_x = conf_msgbox_margin_left;
		pen_y = conf_msgbox_margin_top;
	} else {
		pen_x = msgbox_w - conf_msgbox_margin_right - conf_msgbox_font_size;
		pen_y = conf_msgbox_margin_top;
	}
}

/* Initialize if in auto mode */
static void
init_auto_mode(void)
{
	/* Do not process if returned from system GUI */
	if (gui_sys_flag)
		return;

	/* Do not process if registered in history but not displayed */
	/*
	 * if (conf_msgbox_history_control != NULL &&
	 *     strcmp(conf_msgbox_history_control, "only-history") == 0)
	 * 	return;
	 */

	/* If in auto mode */
	if (s3_is_auto_mode()) {
		/* Disable input for return key and down key */
		s3_clear_input_state();
	}
}

/* Initialize if in skip mode */
static void
init_skip_mode(void)
{
	/* Do not process if returned from system GUI */
	if (gui_sys_flag)
		return;

	/* Do not process if registered in history but not displayed */
	/*
	 * if (conf_msgbox_history_control != NULL &&
	 *     strcmp(conf_msgbox_history_control, "only-history") == 0)
	 * 	return;
	 */

	/* If in skip mode */
	if (s3_is_skip_mode()) {
		/* If reached unread, stop skip mode */
		if (!is_skippable()) {
			s3_stop_skip_mode();
			s3_show_skipmode_banner(false);
			return;
		}

		/* If clicked */
		if (s3_is_mouse_right_clicked() || s3_is_mouse_left_clicked() ||
		    s3_is_up_key_pressed() || s3_is_down_key_pressed() ||
		    s3_is_escape_key_pressed()) {
			/* Play SE */
			play_se(conf_skipmode_leave_se);

			/* Stop skip mode */
			s3_stop_skip_mode();
			s3_show_skipmode_banner(false);

			/* Do not process subsequent click */
			s3_clear_input_state();
		}
	}
}

/* Initialize the colors. */
static void
init_colors(void)
{
	body_color = s3_make_pixel(0xff,
				   (uint32_t)conf_msgbox_font_r,
				   (uint32_t)conf_msgbox_font_g,
				   (uint32_t)conf_msgbox_font_b);

	body_outline_color = s3_make_pixel(0xff,
					   (uint32_t)conf_msgbox_font_outline_r,
					   (uint32_t)conf_msgbox_font_outline_g,
					   (uint32_t)conf_msgbox_font_outline_b);

	name_color = s3_make_pixel(0xff,
				   (uint32_t)conf_namebox_font_r,
				   (uint32_t)conf_namebox_font_g,
				   (uint32_t)conf_namebox_font_b);

	name_outline_color = s3_make_pixel(0xff,
					   (uint32_t)conf_namebox_font_outline_r,
					   (uint32_t)conf_namebox_font_outline_g,
					   (uint32_t)conf_namebox_font_outline_b);

	bg_color = s3_make_pixel(0xff,
				 (uint32_t)conf_msgbox_fill_r,
				 (uint32_t)conf_msgbox_fill_g,
				 (uint32_t)conf_msgbox_fill_b);
}

/* Get the name */
static bool
init_name_top(void)
{
	const char *name;

	/* Do not process if returned from system GUI */
	if (gui_sys_flag && !load_flag)
		return true;

	/* If returned from load/title, name_top remains, so free it */
	if (name_top != NULL) {
		free(name_top);
		name_top = NULL;
	}

	/* Get the name */
	name = get_localized_name();
	if (name != NULL) {
		name_top = strdup(name);
		if (name_top == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	} else {
		name_top = NULL;
	}

	s3_set_last_name(name);

	if (name_top == NULL) {
		/* No name. */
		s3_show_namebox(false);
	} else {
		if (conf_namebox_enable) {
			/* Draw the name */
			if (!blit_namebox())
				return false;

			/* Show the name box */
			s3_show_namebox(true);
		}

		/* Set the focus */
		focus_character();
	}

	return true;
}

/* Get the voice file name */
static bool
init_voice_file(void)
{
	const char *voice;

	/* If returned from load/title, voice_file remains, so free it */
	if (voice_file != NULL) {
		free(voice_file);
		voice_file = NULL;
	}

	/* Get the voice file name */
	voice = get_localized_voice();
	if (voice == NULL)
		return true;

	/* Copy the voice file name. */
	voice_file = strdup(voice);
	if (voice_file == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	return true;
}

/* Get the message */
static bool
init_msg_top(void)
{
	const char *msg;

	/* Do not process if returned from system GUI */
	if (gui_sys_flag && !load_flag)
		return true;

	/* If a load or a back-to-title executed, msg_top still remains, so free it. */
	if (msg_top != NULL) {
		free(msg_top);
		msg_top = NULL;
	}

	/* If we are in the page mode. */
	msg = get_localized_text();
	if (msg == NULL)
		return false;

	/* Register message history for history screen */
	if (!register_message_for_history(msg))
		return false;

	/* Save the message. (for save. do not save if this is a first tag for a load.) */
	if (!load_flag) {
		if (!s3_is_page_mode() || is_page_top) {
			if (!s3_set_last_message(msg))
				return false;
		} else {
			if (!s3_append_last_message(line_spacing))
				return false;
			if (!s3_append_last_message(msg))
				return false;
		}
	}

	/*
	 * Duplicate the text because a tag parameter may be freed by
	 * s3_evaluate_tag(). If we don't duplicate the text, it will
	 * be a use-after-free in case we returned from the system GUI.
	 */
	msg_top = strdup(msg);
	if (msg_top == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	if (last_lang != NULL) {
		free(last_lang);
		last_lang = NULL;
	}
	last_lang = strdup(s3_get_system_language());

	return true;
}

/* Register message history for history screen */
static bool
register_message_for_history(
	const char *msg)
{
	const char *voice;

	assert(msg != NULL);
	assert(!gui_sys_flag);

	/* Do not play beep sound in history screen */
	voice = voice_file;
	if (voice != NULL && voice[0] == '@')
		voice = NULL;

	if (!s3_is_page_mode() || is_page_top) {
		if (!s3_add_history(name_top,
				    msg,
				    voice,
				    body_color,
				    body_outline_color,
				    name_color,
				    name_outline_color))
			return false;
	} else {
		if (!s3_append_history(msg,
				       line_spacing))
			return false;
	}

	/* Success */
	return true;
}

/* Initialize the message box */
static bool
init_msgbox(void)
{
	/* If returning from system GUI */
	if (gui_sys_flag && !load_flag)
		return true;

	/* Do LF for page mode if `action="inline"` is not specified. */
	if (s3_is_page_mode() & !is_page_top && !is_inline) {
		if (!conf_msgbox_font_tategaki) {
			pen_x = conf_msgbox_margin_left;
			pen_y += conf_msgbox_margin_line;
		} else {
			pen_x -= conf_msgbox_margin_line;
			pen_y = conf_msgbox_margin_top;
		}
	}

	/* Save the pen position for overlay painting */
	orig_pen_x = pen_x;
	orig_pen_y = pen_y;

	/* Make the message layer visible */
	s3_show_msgbox(true);

	/* Create a context for message drawing */
	msgbox_context = s3_create_drawmsg(
		s3_get_layer_image(S3_LAYER_MSGBOX),
		msg_top,
		conf_msgbox_font_select,
		conf_msgbox_font_size,
		conf_msgbox_font_size,
		conf_msgbox_font_ruby,
		conf_msgbox_font_outline_width,
		pen_x,
		pen_y,
		msgbox_w,
		msgbox_h,
		conf_msgbox_margin_left,
		conf_msgbox_margin_right,
		conf_msgbox_margin_top,
		conf_msgbox_margin_bottom,
		conf_msgbox_margin_line,
		conf_msgbox_margin_char,
		body_color,
		body_outline_color,
		bg_color,
		conf_msgbox_fill_enable,
		false,	/* is_dimming */
		false,	/* ignore_linefeed */
		false,	/* ignore_font */
		false,	/* ignore_outline */
		false,	/* ignore_color */
		false,	/* ignore_size */
		false,	/* ignore_position */
		false,	/* ignore_ruby */
		false,	/* ignore_wait */
		inline_wait_hook,
		conf_msgbox_font_tategaki);
	if (msgbox_context == NULL)
		return false;

	/* Count the number of characters */
	total_chars = s3_count_drawmsg_chars(msgbox_context, NULL);
	drawn_chars = 0;

	return true;
}

/* Process the voice. */
static bool
init_voice(void)
{
	have_voice = voice_file != NULL;

	/* Play the voice */
	if (check_play_voice()) {
		/* Play the voice */
		if (!play_voice())
			return false;
	}

	return true;
}

/* Determine whether to play the voice */
static bool
check_play_voice(void)
{
	/* If returning from system GUI */
	if (gui_sys_flag && !load_flag)
		return false;

	/* If in non-interruptible mode, play without considering other conditions */
	if (s3_is_non_interruptible())
		return true;

	/* If in skip mode and already read, do not play */
	if (s3_is_skip_mode() && is_skippable())
		return false;

	/* If in auto mode, play */
	if (s3_is_auto_mode())
		return true;

	/* If not read, play */
	if (!is_skippable())
		return true;

	/* If the control key is pressed, do not play. */
	if (s3_is_control_key_pressed())
		return false;

	/* Otherwise, play */
	return true;
}

/* Play the voice */
static bool
play_voice(void)
{
	if (voice_file == NULL || strcmp(voice_file, "") == 0)
		return true;

	/* Set the character volume */
	set_character_volume_by_name(name_top);

	/* Play the PCM stream */
	s3_set_mixer_input_file(S3_TRACK_VOICE, voice_file, false);

	/* Indicate that there is a voice */
	have_voice = true;

	return true;
}

/* Set the character volume */
static void
set_character_volume_by_name(
	const char *name)
{
	int i;

	for (i = 1; i < S3_CHARACTER_MAP_COUNT; i++) {
		/* Search for the character name */
		if (conf_character_name[i] == NULL)
			continue;
		if (strcmp(conf_character_name[i], name) == 0) {
			/* If found */
			s3_apply_character_volume(i);
			return;
		}
	}

	/* Apply the volume for other characters */
	s3_apply_character_volume(S3_CH_VOL_SLOT_DEFAULT);
}

/* Draw the name box */
static bool
blit_namebox(void)
{
	struct s3_drawmsg *context;
	int pen_x, pen_y, char_count;
	int namebox_x, namebox_y, namebox_width, namebox_height;

	/* Get the size of the name box */
	s3_get_namebox_rect(&namebox_x, &namebox_y, &namebox_width, &namebox_height);

	/* Determine the drawing position */
	if (conf_namebox_centering) {
		if (!conf_namebox_font_tategaki) {
			pen_x = (namebox_width - s3_get_string_width(conf_namebox_font_select, conf_namebox_font_size, name_top)) / 2;
			pen_y = conf_namebox_margin_top;
		} else {
			pen_x = conf_namebox_margin_left;
			pen_y = (namebox_height - s3_get_string_height(conf_namebox_font_select, conf_namebox_font_size, name_top)) / 2;
		}
	} else {
		pen_x = conf_namebox_margin_left;
		pen_y = conf_namebox_margin_top;
	}

	/* Fill the name box layer with the name box image */
	s3_fill_namebox();

	/* Create the character drawing context */
	context = s3_create_drawmsg(
		s3_get_layer_image(S3_LAYER_NAMEBOX),
		name_top,
		conf_namebox_font_select,
		conf_namebox_font_size,
		conf_namebox_font_size,		/* base_font_size */
		conf_namebox_font_ruby,
		conf_namebox_font_outline_width,
		pen_x,
		pen_y,
		namebox_width,
		namebox_height,
		conf_namebox_margin_left,
		0,			/* right_margin */
		conf_namebox_margin_top,
		0,			/* bottom_margin */
		0,			/* line_margin */
		conf_msgbox_margin_char,
		name_color,
		name_outline_color,
		0,			/* bg_color */
		false,			/* fill_bg */
		false,			/* is_dimming */
		false,			/* ignore_linefeed */
		false,			/* ignore_font*/
		false,			/* ignore_outline */
		false,			/* ignore_color */
		false,			/* ignore_size */
		false,			/* ignore_position */
		false,			/* ignore_ruby */
		true,			/* ignore_wait */
		NULL,			/* inline_wait_hook */
		conf_namebox_font_tategaki);
	if (context == NULL)
		return false;

	/* Get the number of characters in the name */
	char_count = s3_count_drawmsg_chars(context, NULL);

	/* Draw the characters */
	s3_draw_message(context, char_count);

	s3_destroy_drawmsg(context);

	return true;
}

bool
s3i_blit_load_name(void)
{
	const char *name;

	name = s3_get_last_name();
	if (name == NULL)
		return true;

	name_top = strdup(name);
	if (name_top != NULL) {
		blit_namebox();

		free(name_top);
		name_top = NULL;
	}

	return true;
}

/* Focus the character */
static void
focus_character(void)
{
	const char *name;
	int i, index;

	/* Get the talking character. */
	name = s3_get_tag_arg_string("name", true, NULL);
	if (name == NULL) {
		/* Update the talking character. */
		s3_set_ch_talking(-1);

		/* Do autofocus. */
		if (conf_autofocus_on_text_no_name) {
			/* Update dimming based on the talking character. */
			s3_update_ch_dim_by_talking_ch();
		}
	} else {
		/* Check if the character is registered with a name */
		for (i = 0; i < S3_CHARACTER_MAP_COUNT; i++) {
			if (conf_character_name[i] == NULL)
				continue;
			if (conf_character_folder[i] == NULL)
				continue;

			if (strcmp(conf_character_name[i], name) == 0) {
				/* Set the current talking character. */
				s3_set_ch_talking(i < S3_CHARACTER_MAP_COUNT ? i : -1);
				break;
			}
		}
		if (i == S3_CHARACTER_MAP_COUNT)
			index = -1;
		else
			index = i;

		/* Update the talking character. */
		s3_set_ch_talking(index);

		/* Do autofocus. */
		if (conf_autofocus_on_text_name) {
			/* Update dimming based on the talking character. */
			s3_update_ch_dim_by_talking_ch();
		}
	}
}

/* Initialize the click animation */
static void
init_click(void)
{
	/* Hide the click layer */
	s3_show_click(false);

	/* Set the first display flag for the click animation */
	is_click_first = true;

	/* Save the display state of the click animation */
	is_click_visible = false;
}

/* Set repetition behavior during initialization */
static void
init_repetition(void)
{
	if (is_skippable() && !s3_is_non_interruptible() &&
	    (s3_is_skip_mode() || (!s3_is_auto_mode() && s3_is_control_key_pressed()))) {
		/* Do not repeat, display immediately */
	} else {
		/* Make the command be called repeatedly */
		s3_start_command_repetition();

		/* Start timing */
		s3_reset_lap_timer(&click_sw);
	}
}

/*
 * Frame processing
 */

/* Process auto mode control */
static bool
frame_auto_mode(void)
{
	uint64_t lap;

	/* If not in auto mode, do nothing */
	if (!s3_is_auto_mode())
		return false;

	/* Cannot enter skip mode during auto mode */
	assert(!s3_is_skip_mode());

	/* Cannot hide the message box during auto mode */
	assert(!is_hidden);

#ifdef USE_EDITOR
	/* If stop is requested during auto mode */
	if (dbg_is_stop_requested()) {
		/* Process auto mode stop action */
		stop_auto_mode();
		show_automode_banner(false);

		/* Treat as clicked */
		return true;
	}
#endif

	/* If clicked */
	if (s3_is_mouse_left_clicked() || s3_is_mouse_right_clicked() || s3_is_escape_key_pressed() ||
	    s3_is_return_key_pressed() || s3_is_down_key_pressed()) {
		/* Play SE */
		play_se(conf_automode_leave_se);

		/* Stop auto mode */
		s3_stop_auto_mode();

		/* Hide auto mode banner */
		s3_show_automode_banner(false);

		/* Message display and voice playback are incomplete */
		is_auto_mode_wait = false;

		/* Do not process further clicks */
		s3_clear_input_state();

		/* Clicked */
		return true;
	} else if (!is_auto_mode_wait) {
		/* Message display and voice playback are incomplete */

		/* If already displayed */
		if (check_auto_play_condition()) {
			/* Enter waiting time */
			is_auto_mode_wait = true;

			/* Start timing */
			s3_reset_lap_timer(&auto_sw);
		}
	} else {
		/* Waiting time */

		/* Measure time */
		lap = s3_get_lap_timer_millisec(&auto_sw);

		/* If time has elapsed, proceed to command termination processing */
		if (lap >= (uint64_t)get_wait_time()) {
			if (conf_msgbox_dim_enable)
				need_dimming = true;

			stop();

			/* End the command */
			return true;
		}
	}

	/* Not clicked and command not ended */
	return false;
}

/* Process auto play completion check */
static bool
check_auto_play_condition(void)
{
	/*
	 * If the command returned from the system GUI
	 *  - Already displayed
	 */
	if (gui_sys_flag)
		return true;

	/*
	 * If there is voice
	 *  - Check if voice playback and text display are complete
	 */
	if (have_voice) {
		/* Check if display and playback are complete */
		if (s3_is_mixer_sound_finished(S3_TRACK_VOICE) && is_end_of_msg())
			return true;

		return false;
	}

	/*
	 * If there is no voice
	 *  - Check if text display is complete
	 */
	if (is_end_of_msg())
		return true;

	return false;
}

/* Get auto mode wait time */
static int
get_wait_time(void)
{
	float scale;

	/* If there is voice */
	if (have_voice)
		return (int)(AUTO_MODE_VOICE_WAIT * s3_get_auto_speed());

	/* If there is no voice, calculate scale */
	scale = AUTO_MODE_TEXT_WAIT_SCALE;

	return (int)((float)total_chars * scale * s3_get_auto_speed() * 1000.0f);
}

/* Start hiding/showing the message box */
static void
action_toggle_hide(void)
{
	if (!is_hidden) {
		/* Hide the message box */
		is_hidden = true;
		if (name_top != NULL)
			s3_show_namebox(false);
		s3_show_msgbox(false);
		s3_show_click(false);

		// FIXME
		//play_se(conf_msgbox_btn_hide_enter_se);
	} else {
		/* Show the message box */
		is_hidden = false;
		if (name_top != NULL)
			s3_show_namebox(true);

		s3_show_msgbox(true);
		s3_show_click(is_click_visible);

		// FIXME
		//play_se(conf_msgbox_btn_hide_leave_se);
	}
}

/* Process system buttons */
static bool
frame_sysbtn(void)
{
	bool enter_sysmenu;

#ifdef USE_EDITOR
	/* If single step or stop is requested */
	if (dbg_is_stop_requested()) {
		return false;
	}
#endif

	/* Accept key operations */
	if (!is_hidden) {
		if (s3_is_mouse_right_clicked()) {
			action_toggle_hide();
			s3_clear_input_state();
			return true;
		}
	} else {
		if (s3_is_mouse_left_clicked() ||
		    s3_is_mouse_right_clicked()) {
			action_toggle_hide();
			s3_clear_input_state();
			return true;
		}
	}

	/* If system buttons are not displayed in the config */
	if (!conf_sysbtn_enable)
		return false;

	/* Do not process when the message box is hidden */
	if (is_hidden)
		return false;

	/* Do not process during auto mode */
	if (s3_is_auto_mode())
		return false;

	/* Do not process during skip mode */
	if (s3_is_skip_mode())
		return false;

	/* Determine whether to enter the system menu */
	enter_sysmenu = false;
	if (s3_is_mouse_right_clicked() || s3_is_escape_key_pressed()) {
		/* If right-clicked or escape key is pressed */
		enter_sysmenu = true;
		s3_clear_input_state();
	} else if (s3_is_mouse_left_clicked() && s3_is_sysbtn_pointed()) {
		/* If system button is clicked */
		enter_sysmenu = true;
		s3_clear_input_state();
	}

	/* If not entering the system menu */
	if (!enter_sysmenu)
		return false;

	/* Enter the system menu */
	play_se(conf_sysbtn_enter_se);

	/* Show the system menu */
	need_sysmenu_mode = true;

	return true;
}

/*
 * Main drawing process
 */

/* Draw the frame */
static void
blit_frame(void)
{
	/* Do not process when the message box is hidden */
	if (is_hidden)
		return;

	/* Below, perform the main display processing */

	/* If there was input. */
	if (is_canceled_by_skip()) {
		/* And it is not a sysmenu transition. */
		if (!need_sysmenu_mode) {
			/* Stop the voice. */
			s3_set_mixer_input_file(S3_TRACK_VOICE, NULL, false);
		}
	}

	/* Whether the main text is being drawn */
	if (!is_end_of_msg()) {
		/* Process inline wait */
		if (is_inline_wait) {
			if ((float)s3_get_lap_timer_millisec(&inline_sw) / 1000.0f >
			    inline_wait_time)
				is_inline_wait = false;
		}
		if (!is_inline_wait) {
			/* Draw the main text */
			blit_msgbox();
		}
		if (is_end_of_msg()) {
			/* Stop lip sync */
			/*cleanup_lip_sync();*/
		}
	} else {
		/*
		 * The main text has finished drawing and is trying to draw further,
		 * so draw the click animation
		 */
		set_click();
	}
}

/* Whether the main text has finished drawing */
static bool
is_end_of_msg(void)
{
	/* If returned from system GUI */
	if (gui_sys_flag)
		return true;

	/* If finished */
	if (drawn_chars == total_chars)
		return true;

	/* If not finished */
	return false;
}

bool
s3i_blit_load_message(void)
{
	struct s3_drawmsg *context;
	const char *text;
	s3_pixel_t save_body_color, save_body_outline_color;
	int len;

	text = s3_get_prev_last_message();
	if (text == NULL || strcmp(text, "") == 0)
		return true;

	clear_msgbox();

	orig_pen_x = pen_x;
	orig_pen_y = pen_y;

	if (!conf_msgbox_dim_enable) {
		save_body_color = body_color;
		save_body_outline_color = body_outline_color;
		body_color = s3_make_pixel(0xff,
					   (uint32_t)conf_msgbox_font_r,
					   (uint32_t)conf_msgbox_font_g,
					   (uint32_t)conf_msgbox_font_b);
		body_outline_color = s3_make_pixel(0xff,
						   (uint32_t)conf_msgbox_font_outline_r,
						   (uint32_t)conf_msgbox_font_outline_g,
						   (uint32_t)conf_msgbox_font_outline_b);
	} else {
		/* Determine the text color for dimming */
		body_color = s3_make_pixel(0xff,
					   (uint32_t)conf_msgbox_dim_r,
					   (uint32_t)conf_msgbox_dim_g,
					   (uint32_t)conf_msgbox_dim_b);
		body_outline_color = s3_make_pixel(0xff,
						   (uint32_t)conf_msgbox_dim_outline_r,
						   (uint32_t)conf_msgbox_dim_outline_g,
						   (uint32_t)conf_msgbox_dim_outline_b);
	}

	context = s3_create_drawmsg(
		s3_get_layer_image(S3_LAYER_MSGBOX),
		text,
		conf_msgbox_font_select,
		conf_msgbox_font_size,
		conf_msgbox_font_size,
		conf_msgbox_font_ruby,
		conf_msgbox_font_outline_width,
		pen_x,
		pen_y,
		msgbox_w,
		msgbox_h,
		conf_msgbox_margin_left,
		conf_msgbox_margin_right,
		conf_msgbox_margin_top,
		conf_msgbox_margin_bottom,
		conf_msgbox_margin_line,
		conf_msgbox_margin_char,
		body_color,
		body_outline_color,
		0,	/* bg_color */
		false,	/* fill_bg */
		conf_msgbox_dim_enable,
		false,	/* ignore_linefeed */
		false,	/* ignore_font */
		false,	/* ignore_outline */
		false,	/* ignore_color */
		false,	/* ignore_size */
		false,	/* ignore_position */
		false,	/* ignore_ruby */
		false,	/* ignore_wait */
		inline_wait_hook,
		conf_msgbox_font_tategaki);
	if (context == NULL)
		return false;
	len = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, len);
	s3_get_drawmsg_pen_position(context, &pen_x, &pen_y);
	s3_destroy_drawmsg(context);

	body_color = save_body_color;
	body_outline_color = save_body_outline_color;
	avoid_dimming = true;

	s3_set_pen_position(pen_x, pen_y);

	return true;
}

/* Draw the message box */
static void
blit_msgbox(void)
{
	int char_count, ret;

	/*
	 * This should not be drawing when returning from system GUI
	 *  - However, in the case of dimming, it should draw even when returning from system GUI
	 */
	if (!is_dimming)
		assert(!gui_sys_flag);

	/* Get the number of characters to draw in this frame */
	char_count = get_frame_chars();
	if (char_count == 0)
		return;

	/* Perform drawing */
	ret = s3_draw_message(msgbox_context, char_count);
	if (is_inline_wait) {
		/* If inline wait appeared */
		drawn_chars += ret;
		return;
	}

	/* Record the number of characters drawn */
	drawn_chars += char_count;

	/* Get the pen's drawing end position */
	if (drawn_chars == total_chars)
		s3_get_drawmsg_pen_position(msgbox_context, &pen_x, &pen_y);
}

/* Callback when inline wait escape sequence appears */
static void
inline_wait_hook(
	float wait_time)
{
	/* Start inline wait */
	if (wait_time > 0 && !do_draw_all) {
		is_inline_wait = true;
		inline_wait_time = wait_time;
		inline_wait_time_total += inline_wait_time;
		s3_reset_lap_timer(&inline_sw);
	}
}

/*
 * Get the number of characters to draw in this frame
 *  - Consider clicks, key presses, transitions to system GUI, and all other factors
 */
static int
get_frame_chars(void)
{
	if (!is_dimming)
		assert(!gui_sys_flag);

	/* If not repeating (including dimming) */
	if (!s3_is_in_command_repetition()) {
		/* Draw all characters */
		do_draw_all = true;
		return total_chars;
	}

	/* If instant display is set in config */
	/* FIXME */
	/*
	 * if (conf_msgbox_nowait) {
	 * 	do_draw_all = true;
	 * 	return total_chars;
	 * }
	 */

	/* If drawing all characters to create a save thumbnail */
	if (need_sysmenu_mode || need_save_mode || need_load_mode || need_history_mode || need_config_mode) {
		/* Draw all remaining characters */
		do_draw_all = true;
		return total_chars - drawn_chars;
	}

	/* If canceled by skip */
	if (is_canceled_by_skip()) {
		if (conf_msgbox_dim_enable)
			need_dimming = true;

		/* Stop repetition */
		stop();

		/* Stop lip sync */
		cleanup_lip_sync();

		/* Draw all remaining characters */
		do_draw_all = true;
		return total_chars - drawn_chars;
	}

	/* If fast forwarding by click */
	if (is_fast_forward_by_click()) {
		/* Stop lip sync */
		cleanup_lip_sync();

#ifdef USE_EDITOR
		/*
		 * If the debugger's stop button is pressed,
		 * transition to command end without moving to click wait.
		 */
		if (dbg_is_stop_requested())
			s3_stop_command_repetition();
#endif

		/* Draw all remaining characters. */
		do_draw_all = true;
		s3_set_drawmsg_ignore_inline_wait(msgbox_context);
		return total_chars - drawn_chars;
	}

	/* Calculate the number of characters to draw this frame based on elapsed time */
	return calc_frame_chars_by_lap();
}

/* Check if canceled by skip */
static bool
is_canceled_by_skip(void)
{
	/* If unread, skipping is not possible */
	if (!is_skippable())
		return false;

	/* If non-interruptible, do not skip */
	if (s3_is_non_interruptible())
		return false;

	/* If skip mode, skip */
	if (s3_is_skip_mode())
		return true;

	/* If Control key is pressed, skip */
	if (s3_is_control_key_pressed()) {
		/* However, if auto mode, do not skip */
		if (s3_is_auto_mode())
			return false;
		else
			return true;
	}

	/* Do not skip */
	return false;
}

/* If fast forwarding by click */
static bool
is_fast_forward_by_click(void)
{
	/* If non-interruptible, do not move to click wait */
	if (s3_is_non_interruptible())
		return false;

	/* If Return key is pressed, move to click wait */
	if (s3_is_return_key_pressed())
		return true;

	/* If Down key is pressed, move to click wait */
	if (s3_is_down_key_pressed())
		return true;

	/* If clicked, move to click wait */
	if (s3_is_mouse_left_clicked()) {
		/* Move to click wait */
		return true;
	}

	/* Do not move to click wait */
	return false;
}

/* Calculate the number of characters to draw this frame based on elapsed time */
static int
calc_frame_chars_by_lap(void)
{
	float lap;
	float progress;
	int char_count;

	/*
	 * If the text speed is maximum in the system GUI config,
	 * draw all characters with no wait.
	 */
	if (s3_get_text_speed() == 1.0f)
		return total_chars - drawn_chars;

	/*
	 * Get the elapsed time (seconds)
	 *  - Subtract the inline wait time
	 */
	lap = (float)s3_get_lap_timer_millisec(&click_sw) / 1000.0f -
		inline_wait_time_total;

	/* Calculate progress (number of characters) */
	progress = MESSAGE_SPEED * lap;

	/* Multiply by user-configured text speed (add 0.1 to avoid zero) */
	progress *= s3_get_text_speed() + 0.1f;

	/* Convert to integer */
	char_count = (int)ceilf(progress);

	/* Convert to remaining number of characters */
	char_count -= drawn_chars;

	/* If it exceeds the remaining number of characters, set to remaining number of characters */
	if (char_count > total_chars - drawn_chars)
		char_count = total_chars - drawn_chars;

	return char_count;
}

/* Set click animation */
static void
set_click(void)
{
	uint64_t lap;
	int click_x, click_y, click_w, click_h;
	int index;

	/* If continue mode and only newline, do not wait for click */
	if (is_continue_mode && total_chars == 0) {
		if (s3_is_in_command_repetition())
			stop();
		return;
	}

	/* If input is received, stop repetition */
	if (check_stop_click_animation()) {
		if (conf_msgbox_dim_enable && !avoid_dimming)
			need_dimming = true;
		stop();
	}

	/* When the click animation is displayed for the first time */
	if (is_click_first) {
		is_click_first = false;

		/* Set display position */
		if (conf_click_move) {
			if (!gui_sys_flag) {
				int cur_pen_x, cur_pen_y;
				s3_set_click_index(0);
				s3_get_click_rect(&click_x, &click_y, &click_w, &click_h);
				s3_get_drawmsg_pen_position(msgbox_context, &cur_pen_x, &cur_pen_y);
				s3_set_click_position(cur_pen_x + conf_msgbox_x,
						      cur_pen_y + conf_msgbox_y);
			} else {
				s3_set_click_position(pen_x, pen_y);
			}
		} else {
			s3_set_click_position(conf_click_x, conf_click_y);
		}

		/* Start timing */
		s3_reset_lap_timer(&click_sw);
	}

	/* Get elapsed time */
	lap = s3_get_lap_timer_millisec(&click_sw);

	/* Display click animation */
	index = (int)((lap % (uint64_t)(conf_click_interval * 1000)) /
		((uint64_t)(conf_click_interval * 1000) / (uint64_t)conf_click_frames) %
		      (uint64_t)conf_click_frames);
	index = index < 0 ? 0 : index;
	index = index >= conf_click_frames ? 0 : index;
	s3_set_click_index(index);
	s3_show_click(true);
	is_click_visible = true;
}

/* If input is received during click animation, stop repetition */
static bool
check_stop_click_animation(void)
{
	/* Do not come here if the main text drawing is not complete */
	assert(is_end_of_msg());

#ifdef USE_EDITOR
	/* If a stop request is received from the debugger */
	if (dbg_is_stop_requested()) {
		/* Stop if there is no voice */
		if (!have_voice)
			return true;

		/* Stop on the frame when voice playback is complete */
		if (is_mixer_sound_finished(VOICE_STREAM))
			return true;

		/* Stop if clicked on an empty area */
		if (s3_is_mouse_left_clicked() && !is_sysbtn_pointed())
			return true;

		/* Stop if a key is pressed */
		if (s3_fis_down_key_pressed() || s3_is_return_key_pressed())
			return true;
	}
#endif

	/* If skip mode */
	if (s3_is_skip_mode()) {
		assert(!s3_is_auto_mode());

		/* Do not stop if not read */
		if (!is_skippable())
			return false;

		/* If not non-interruptible, stop due to skip */
		if (!s3_is_non_interruptible())
			return true;

		/* Stop due to skip if there is no voice */
		if (!have_voice)
			return true;

		/* Stop due to skip if voice playback is complete */
		if (s3_is_mixer_sound_finished(S3_TRACK_VOICE))
			return true;

		/* Do not stop */
		return false;
	}

	/* If the control key is pressed */
	if (s3_is_control_key_pressed()) {
		/* Ignore the control key in auto mode */
		if (s3_is_auto_mode())
			return false;

		/* Do not stop if not read */
		if (!is_skippable())
			return false;

		/* If not non-interruptible, stop due to skip */
		if (!s3_is_non_interruptible())
			return true;

		/* Stop due to skip if there is no voice */
		if (!have_voice)
			return true;

		/* Stop due to skip if voice playback is ongoing */
		if (s3_is_mixer_sound_finished(S3_TRACK_VOICE))
			return true;

		/* Do not stop */
		return false;
	}

	/* If returning from system GUI and command has started */
	if (gui_sys_flag) {
		/* FYI: Voice is not playing */

		/* Stop if clicked on an empty area */
		if (s3_is_mouse_left_clicked() && !s3_is_sysbtn_pointed())
			return true;

		/* Stop if a key is pressed */
		if (s3_is_return_key_pressed() || s3_is_down_key_pressed())
			return true;

		/* Do not stop */
		return false;
	}

	/* If clicked on an empty area or a key is pressed */
	if ((s3_is_mouse_left_clicked() && !s3_is_sysbtn_pointed()) ||
	    (s3_is_return_key_pressed() || s3_is_down_key_pressed())) {
		/*
		 * If not non-interruptible, stop
		 *  - If there is a voice playing, it will be stopped by cleanup()
		 */
		if (!s3_is_non_interruptible())
			return true;

		/*
		 * Even in non-interruptible mode, stop if there is no voice or if playback is complete
		 */
		if (!have_voice || s3_is_mixer_sound_finished(S3_TRACK_VOICE))
			return true;

		/* Do not stop */
		return false;
	}

	/* Do not stop */
	return false;
}

/*
 * Do dimming (overlay painting)
 *  - In full-screen style, draw text to darken the parts that have already been read
 */
static void
blit_dimming(void)
{
	struct s3_drawmsg *context;

	/* Dimming is enabled in the config */
	assert(conf_msgbox_dim_enable);

	/* Do not dim when transitioning to system GUI */
	assert(!need_sysmenu_mode);

	/* Determine the text color for dimming */
	body_color = s3_make_pixel(0xff,
				   (uint32_t)conf_msgbox_dim_r,
				   (uint32_t)conf_msgbox_dim_g,
				   (uint32_t)conf_msgbox_dim_b);
	body_outline_color = s3_make_pixel(0xff,
					   (uint32_t)conf_msgbox_dim_outline_r,
					   (uint32_t)conf_msgbox_dim_outline_g,
					   (uint32_t)conf_msgbox_dim_outline_b);

	/* Overwrite the main text */
	context = s3_create_drawmsg(
		s3_get_layer_image(S3_LAYER_MSGBOX),
		msg_top,
		conf_msgbox_font_select,
		conf_msgbox_font_size,
		conf_msgbox_font_size,
		conf_msgbox_font_ruby,
		conf_msgbox_font_outline_width,
		orig_pen_x,
		orig_pen_y,
		msgbox_w,
		msgbox_h,
		conf_msgbox_margin_left,
		conf_msgbox_margin_right,
		conf_msgbox_margin_top,
		conf_msgbox_margin_bottom,
		conf_msgbox_margin_line,
		conf_msgbox_margin_char,
		body_color,
		body_outline_color,
		0,	/* bg_color */
		false,	/* fill_bg */
		true,	/* is_dimming */
		false,	/* ignore_linefeed */
		false,	/* ignore_font */
		false,	/* ignore_outline */
		true,	/* ignore_color */
		false,	/* ignore_size */
		false,	/* ignore_position */
		false,	/* ignore_ruby */
		true,	/* ignore_wait */
		NULL,	/* inline_wait_hook */
		conf_msgbox_font_tategaki);
	s3_draw_message(context, total_chars);
	s3_destroy_drawmsg(context);
}

/*
 * Others
 */

/* Get the text for the current locale. */
static const char *
get_localized_text(void)
{
	char name[128];
	const char *locale;
	const char *text;

	locale = s3_get_locale();
	assert(locale != NULL);

	/*
	 * Try a localized text.
	 */

	/* Try a full locale such as "en-us" or "ja". */
	snprintf(name, sizeof(name), "text-%s", locale);
	text = s3_get_tag_arg_string(name, true, NULL);
	if (text == NULL) {
		/* Fallback to a major locale such as "en" for "es-us". */
		locale = s3i_get_major_locale();
		if (locale != NULL) {
			snprintf(name, sizeof(name), "text-%s", locale);
			text = s3_get_tag_arg_string(name, true, NULL);
		}

		/* Fallback to a non-localized text. */
		if (text == NULL)
			text = s3_get_tag_arg_string("text", true, NULL);

		/* Fallback to a "en" text. */
		if (text == NULL)
			text = s3_get_tag_arg_string("text-en", true, NULL);
	}

	if (text == NULL) {
		s3_log_tag_error(S3_TR("No matching localized text."));
		return NULL;
	}

	return text;
}

/* Get the name for the current locale. */
static const char *
get_localized_name(void)
{
	const char *locale, *major_locale, *name;
	int i;

	name = s3_get_tag_arg_string("name", true, NULL);
	if (name == NULL)
		return NULL;

	/* Look up the name map index. */
	for (i = 0; i < S3_CHARACTER_MAP_COUNT; i++) {
		if (conf_character_name[i] == NULL)
			continue;
		if (strcmp(conf_character_name[i], name) == 0)
			break;
	}
	if (i == S3_CHARACTER_MAP_COUNT) {
		/* Not found, no translation. */
		return name;
	}

	/* Get the locale. */
	locale = s3_get_locale();
	major_locale = s3i_get_major_locale();
	if (major_locale != NULL)
		locale = major_locale;

	/* Translate. */
	if (strcmp(locale, "en") == 0) {
		if (conf_character_name_en[i] != NULL)
			return conf_character_name_en[i];
		return name;
	}
	if (strcmp(locale, "fr") == 0) {
		if (conf_character_name_fr[i] != NULL)
			return conf_character_name_fr[i];
		return name;
	}
	if (strcmp(locale, "es") == 0) {
		if (conf_character_name_es[i] != NULL)
			return conf_character_name_es[i];
		return name;
	}
	if (strcmp(locale, "de") == 0) {
		if (conf_character_name_de[i] != NULL)
			return conf_character_name_de[i];
		return name;
	}
	if (strcmp(locale, "it") == 0) {
		if (conf_character_name_it[i] != NULL)
			return conf_character_name_it[i];
		return name;
	}
	if (strcmp(locale, "ru") == 0) {
		if (conf_character_name_ru[i] != NULL)
			return conf_character_name_ru[i];
		return name;
	}
	if (strcmp(locale, "el") == 0) {
		if (conf_character_name_el[i] != NULL)
			return conf_character_name_el[i];
		return name;
	}
	if (strcmp(locale, "zh-cn") == 0) {
		if (conf_character_name_zh_cn[i] != NULL)
			return conf_character_name_zh_cn[i];
		return name;
	}
	if (strcmp(locale, "zh-tw") == 0) {
		if (conf_character_name_zh_tw[i] != NULL)
			return conf_character_name_zh_tw[i];
		return name;
	}
	if (strcmp(locale, "ja") == 0) {
		if (conf_character_name_ja[i] != NULL)
			return conf_character_name_ja[i];
		return name;
	}

	/* No translation. */
	return name;
}

/* Get the voice file for the current locale. */
static const char *
get_localized_voice(void)
{
	char name[128];
	const char *locale, *major_locale;
	const char *voice_base;
	const char *voice;
	static char file[256];

	locale = s3_get_locale();
	major_locale = s3i_get_major_locale();
	voice_base = s3_get_tag_arg_string("voice", true, NULL);

	/*
	 * Try a full locale such as "en-us" or "ja".
	 */

	/* Try the argument. */
	snprintf(name, sizeof(name), "voice-%s", locale);
	voice = s3_get_tag_arg_string(name, true, NULL);
	if (voice != NULL)
		return voice;

	/* Try the file. */
	if (voice_base != NULL) {
		/* Try the detailed locale such as "en-us". */
		snprintf(file, sizeof(file), "voice/%s/%s", locale, voice_base);
		if (s3_check_file_exists(file))
			return file;
	}

	/*
	 * Fallback to a major locale such as "en".
	 */

	if (major_locale != NULL) {
		/* Try the argument. */
		snprintf(name, sizeof(name), "voice-%s", major_locale);
		voice = s3_get_tag_arg_string(name, true, NULL);
		if (voice != NULL)
			return voice;

		/* Try the file. */
		if (voice_base != NULL) {
			snprintf(file, sizeof(file), "voice/%s/%s", major_locale, voice_base);
			if (s3_check_file_exists(file))
				return file;
		}
	}

	/*
	 * Fallback to the "voice" argument.
	 */
	if (voice_base != NULL)
		return voice_base;

	/*
	 * Fallback to the "voice-en" argument.
	 */
	voice = s3_get_tag_arg_string("voice-en", true, NULL);
	if (voice != NULL)
		return voice;

	/* No voice. */
	return NULL;
}

/* Play SE */
static void
play_se(
	const char *file)
{
	if (file == NULL || strcmp(file, "") == 0)
		return;

	s3_set_mixer_input_file(S3_TRACK_SYS, file, false);
}

/* Check if skippable */
static bool
is_skippable(void)
{
	bool is_seen;

	is_seen = s3_get_seen_flags() != 0;
	if (is_seen) {
		return true;
	} else {
		if (conf_msgbox_skip_unseen)
			return true;
		return false;
	}
}

/* Perform text-to-speech */
static void
speak(void)
{
	if (!conf_tts_enable)
		return;
	if (have_voice)
		return;

	s3_speak_text(NULL);
	s3_speak_text(msg_top);
}

/* Perform lip sync */
static void
init_lip_sync(void)
{
	int chpos;

	if (name_top == NULL)
		return;
	if (gui_sys_flag)
		return;

	chpos = s3_get_talking_chpos();
	if (chpos == -1)
		return;

	s3_run_lip_anime(chpos, msg_top);
}

/* End lip sync */
static void
cleanup_lip_sync(void)
{
	int chpos;

	chpos = s3_get_talking_chpos();
	if (chpos == -1)
		return;

	s3_stop_lip_anime(chpos);
}

/*
 * Cleanup
 */

/* A wrapper of s3_stop_command_repetition() */
static void
stop(void)
{
	if (need_sysmenu_mode || need_save_mode || need_load_mode || need_history_mode || need_config_mode) {
		/* If not in auto/skip mode */
		if (s3_is_in_command_repetition())
			s3_stop_command_repetition();
		return;
	}

	if (conf_msgbox_dim_enable && !avoid_dimming) {
		need_dimming = true;
	} else {
		/* If not in auto/skip mode */
		if (s3_is_in_command_repetition())
			s3_stop_command_repetition();
	}
}

/* Perform cleanup */
static bool
cleanup(void)
{
	int chpos;

	/* Destroy the text drawing context. */
	if (msgbox_context) {
		s3_destroy_drawmsg(msgbox_context);
		msgbox_context = NULL;
	}

	/* Save the pen position for inline choose. */
	s3_set_pen_position(pen_x, pen_y);

	/* Hide the click animation */
	s3_show_click(false);

	/*
	 * When moving to the next command, set the active message to none
	 *  - Keep it active when transitioning to system GUI
	 */
	if (!need_sysmenu_mode&& s3_is_message_active())
		s3_clear_message_active();

	/* Mark as seen */
	s3_set_seen_flags(1);

	/* Free the name and main text */
	if (!need_sysmenu_mode && !need_save_mode && !need_load_mode && !need_history_mode && !need_config_mode) {
		if (name_top != NULL) {
			free(name_top);
			name_top = NULL;
		}
		if (msg_top != NULL) {
			free(msg_top);
			msg_top = NULL;
		}
		if (voice_file != NULL) {
			free(voice_file);
			voice_file = NULL;
		}
	}

	/* Move to the next command */
	if (!need_sysmenu_mode && !need_save_mode && !need_load_mode && !need_history_mode && !need_config_mode) {
		if (!s3_move_to_next_tag())
			return false;
	}

	/* Stop the lip sync animation */
	chpos = s3_get_talking_chpos();
	if (chpos != -1)
		s3_stop_lip_anime(chpos);

	return true;
}
