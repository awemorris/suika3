/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * The "choose" tag implementation
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
#include "text.h"
#include "conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* false assertion */
#define INVALID_BTN_INDEX (0)

/*
 * Buttons.
 */
struct choose_button {
	bool is_enabled;
	const char *text;
	int x;
	int y;
	int w;
	int h;
	struct image *img_idle;
	struct image *img_hover;
};
static struct choose_button button[S3_CHOOSEBOX_COUNT];

/*
 * Button States
 */

/* Index of the pointed button. */
static int pointed_index;

/* Is the pointed index changed by a key operation? */
static bool is_selected_by_key;

/* Mouse position when the pointed index is changed by a key operation. */
static int save_mouse_x, save_mouse_y;

/* Whether to ignore due to no effective option? */
static bool ignore_as_no_option;

/* Is the first frame? (to avoid playing an SE of mouse-over) */
static bool is_first_frame;

/*
 * Centering
 */

/* Is text centered? */
static bool is_centered;

/*
 * Transition
 */

/* Is transition to the system menu required? */
static bool need_sysmenu_mode;

/*
 * Time Limit
 */

/* Is time limited? */
static bool is_timed;

/* Is the timer already bombed? */
static bool is_timer_fired;

/* Timer span. */
static float timer_span;

/* Stopwatch for the timer. */
static uint64_t timer_sw;

/*
 * Result
 */
static const char *result_var_name;

/*
 * Forward Declaration
 */

static bool init(void);
static bool main_process(void);
static void draw_text(int index, bool is_idle);
static void process_main_input(void);
static int get_pointed_index(void);
static void play_se(const char *file);
static void run_anime(int unfocus_index, int focus_index);
static void set_choose_call_arg(int bid);
static bool cleanup(void);

/*
 * The "choose" tag implementation.
 */
bool
s3i_tag_choose(
	void *p)
{
	/* For the first frame, do initialization. */
	if (!s3_is_in_command_repetition()) {
		/* Initialize. */
		if (!init())
			return false;

		/* In case of no options to show. */
		if (ignore_as_no_option)
			return s3_move_to_next_tag();
	}

	/* Process a frame. */
	if (!main_process())
		return false;

	/* For the last frame, do cleanup. */
	if (!s3_is_in_command_repetition())
		if (!cleanup())
			return false;

	return true;
}

/* Initialization. */
static bool
init(void)
{
	int i;
	int actual_option_count;

	/* Clear variables. */
	pointed_index = -1;
	ignore_as_no_option = false;
	need_sysmenu_mode = false;
	is_first_frame = true;
	is_timed = false;
	is_timer_fired = false;
	is_centered = true;
	s3_reset_lap_timer(&timer_sw);

	/* Get the result variable name. */
	result_var_name = s3_get_tag_arg_string("name", false, NULL);

	/* Check for leftification. */
	if (strcmp(s3_get_tag_arg_string("leftify", true, "false"), "true") == 0)
		is_centered = false;
	else
		is_centered = true;

	/* Collect the option infromation. */
	actual_option_count = 0;
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		char text[128];

		snprintf(text, sizeof(text), "text%d", i + 1);

		/* Get the N-th options. */
		button[i].text = s3_get_tag_arg_string(text, true, NULL);
		if (button[i].text == NULL) {
			button[i].is_enabled = false;
			s3_show_choosebox(i, false, false);
			continue;
		} else {
			button[i].is_enabled = true;
		}

		/* Get the coordinates. */
		s3_get_choosebox_rect(i,
				      &button[i].x,
				      &button[i].y,
				      &button[i].w,
				      &button[i].h);

		/* Fill the choose box layers. */
		s3_fill_choosebox_idle_image(i);
		s3_fill_choosebox_hover_image(i);

		/* Draw the text for the choose box layers. */
		draw_text(i, true);
		draw_text(i, false);

		/* Set the layer position. */
		s3_set_layer_position(
			S3_LAYER_CHOOSE1_IDLE + (2 * i),
			button[i].x,
			button[i].y);
		s3_set_layer_position(
			S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1,
			button[i].x,
			button[i].y);

		/* Show the choose box. */
		s3_show_choosebox(i, true, false);

		actual_option_count++;
	}

	/* If there is no option. */
	if (actual_option_count == 0) {
		ignore_as_no_option = true;
		return true;
	}

	/* Start a multi-frame execution. */
	s3_start_command_repetition();

	/* Hide the name and message boxes. */
	if (!conf_msgbox_show_on_choose) {
		s3_show_namebox(false);
		s3_show_msgbox(true);
	}
	s3_show_click(false);

	/* Stop the auto mode. */
	if (s3_is_auto_mode()) {
		s3_stop_auto_mode();
		s3_show_automode_banner(false);
	}

	/* Stop the skip mode. */
	if (s3_is_skip_mode()) {
		s3_stop_skip_mode();
		s3_show_skipmode_banner(false);
	}

	/* Initialize the timer. */
	if (s3_check_tag_arg("time")) {
		is_timed = true;
		timer_span = s3_get_tag_arg_float("time", false, 0);
		s3_reset_lap_timer(&timer_sw);
	} else {
		is_timed = false;
	}

	/* Disable the skip  behavior by continuos swipe. */
	s3_set_continuous_swipe_enabled(false);

	return true;
}

/* Draw an option text to a choose box layer. */
static void
draw_text(
	int index,
	bool is_idle)
{
	struct s3_drawmsg *context;
	s3_pixel_t color, outline_color;
	int outline_width;
	int layer;
	struct s3_image *img;
	int char_count;
	int x, y;

	x = 0;
	y = 0;

	/* Decide the color to draw. */
	if (is_idle) {
		color = s3_make_pixel(255,
				      conf_choose_font_idle_r,
				      conf_choose_font_idle_g,
				      conf_choose_font_idle_b);
		outline_color = s3_make_pixel(255,
					      conf_choose_font_idle_r,
					      conf_choose_font_idle_g,
					      conf_choose_font_idle_b);
		outline_width = conf_choose_font_idle_outline_width;
	} else {
		color = s3_make_pixel(255,
				      conf_choose_font_hover_r,
				      conf_choose_font_hover_g,
				      conf_choose_font_hover_b);
		outline_color = s3_make_pixel(255,
					      conf_choose_font_hover_r,
					      conf_choose_font_hover_g,
					      conf_choose_font_hover_b);
		outline_width = conf_choose_font_hover_outline_width;
	}

	/* Decide the position to draw. */
	if (is_centered) {
		if (!conf_choose_font_tategaki) {
			int vw = s3_get_string_width(conf_choose_font_select,
						     conf_choose_font_size,
						     button[index].text);
			x = x + (button[index].w - vw) / 2;
			y += conf_choose_margin_top[index];
		} else {
			int vh = s3_get_string_height(conf_choose_font_select,
						      conf_choose_font_size,
						      button[index].text);
			x = x + (button[index].w - conf_choose_font_size) / 2;
			y = y + (button[index].h - vh) / 2;
		}
	} else {
		y += conf_choose_margin_top[index];
	}

	/* Get the layer index. */
	layer = S3_LAYER_CHOOSE1_IDLE + (2 * index);
	if (!is_idle)
		layer++;

	/* Get the layer image. */
	img = s3_get_layer_image(layer);

	/* Draw a text. */
	context = s3_create_drawmsg(
		img,
		button[index].text,
		conf_choose_font_select,
		conf_choose_font_size,
		conf_choose_font_size,	/* base_font_size */
		conf_choose_font_ruby,	/* FIXME: namebox.ruby.sizeの導入 */
		outline_width,
		x,
		y,
		conf_game_width,
		conf_game_height,
		x,			/* left_margin */
		0,			/* right_margin */
		conf_choose_margin_top[index],
		0,			/* bottom_margin */
		0,			/* line_margin */
		conf_msgbox_margin_char,
		color,
		outline_color,
		0,			/* bg_color */
		false,			/* fill_bg */
		false,			/* is_dimming */
		true,			/* ignore_linefeed */
		false,			/* ignore_font */
		false,			/* ignore_size */
		false,			/* ignore_color */
		false,			/* ignore_size */
		false,			/* ignore_position */
		false,			/* ignore_ruby */
		true,			/* ignore_wait */
		NULL,			/* inline_wait_hook */
		conf_choose_font_tategaki);
	if (context == NULL)
		return;
	char_count = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, char_count);
	s3_destroy_drawmsg(context);
}

/* Main processing. */
static bool
main_process(void)
{
	int i;

	if (is_timed) {
		if ((float)s3_get_lap_timer_millisec(&timer_sw) >= timer_span * 1000.0f) {
			is_timer_fired = true;
			return true;
		}
	}

	process_main_input();

	if (is_timer_fired)
		return true;

	/* Create the thumbnail if move to system menu. */
	if (need_sysmenu_mode)
		s3_draw_stage_to_thumb();

	if (need_sysmenu_mode) {
		if (!s3_load_gui_file(S3_PATH_SYSMENU_GUI, true))
			return false;
		s3_start_gui();
	}

	/* Stop the repetition if needed. */
	if (is_timer_fired || need_sysmenu_mode)
		s3_stop_command_repetition();

	is_first_frame = false;

	return true;
}

/* Process inputs. */ 
static void
process_main_input(void)
{
	int i;
	int old_pointed_index, new_pointed_index;
	bool enter_sysmenu;

	/* Get the pointed item. */
	old_pointed_index = pointed_index;
	new_pointed_index = get_pointed_index();

	/*
	 * If the pointed index is changed, and an item is pointed,
	 * and TTS is enabled, and the pointed index is changed by a
	 * key.
	 */
	if (new_pointed_index != old_pointed_index &&
	    new_pointed_index != -1 &&
	    conf_tts_enable &&
	    is_selected_by_key &&
	    button[new_pointed_index].text != NULL) {
		s3_speak_text(NULL);
		s3_speak_text(button[pointed_index].text);
	}

	/* If a pointed index is changed and an item is pointed. */
	if (new_pointed_index != -1 &&
	    new_pointed_index != old_pointed_index) {
		/* Avoid the first frame. */
		if (!is_first_frame) {
			play_se(s3_is_mouse_left_clicked() ? conf_choose_click_se : conf_choose_change_se);
			run_anime(old_pointed_index, new_pointed_index);
		}
	}

	/* If previously an item was pointed, and currently no item is pointed. */
	if (old_pointed_index != -1 && new_pointed_index == -1) {
		/* Run an anime. */
		run_anime(old_pointed_index, -1);
	}

	/* Show or hide the choose boxes. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		if (!button[i].is_enabled)
			continue;

		if (i != new_pointed_index) {
			/* Non-pointed item: show idle, hide hover. */
			s3_show_choosebox(i, true, false);
		} else {
			/* Pointed item: hide idle, show hover. */
			s3_show_choosebox(i, false, true);
		}
	}

	/* Commit the change of the pointed index. */
	pointed_index = new_pointed_index;

	/* If clicked by the mouse left button. */
	if (pointed_index != -1 &&
	    (s3_is_mouse_left_clicked() || s3_is_return_key_pressed())) {
		play_se(conf_choose_click_se);
		s3_stop_command_repetition();
		run_anime(pointed_index, -1);
	}

	/* If the system button is enabled. */
	if (conf_sysbtn_enable) {
		/* Start here and decide whether transit to the system menu or not. */
		enter_sysmenu = false;

		/* If right-clicked. */
		if (s3_is_mouse_right_clicked())
			enter_sysmenu = true;

		/* If the escape key is pressed. */
		if (s3_is_escape_key_pressed())
			enter_sysmenu = true;

		/* If the system button is clicked. */
		if (s3_is_mouse_left_clicked() && s3_is_sysbtn_pointed())
			enter_sysmenu = true;

		/* If the system menu will be run. */
		if (enter_sysmenu) {
			run_anime(pointed_index, -1);
			need_sysmenu_mode = true;
		}
	}
}

/* Get the pointed index. */
static int
get_pointed_index(void)
{
	int i;
	int mouse_x;
	int mouse_y;

	mouse_x = s3_get_mouse_pos_x();
	mouse_y = s3_get_mouse_pos_y();

	/* Process the right arrow key. */
	if (s3_is_right_key_pressed()) {
		is_selected_by_key = true;
		save_mouse_x = mouse_x;
		save_mouse_y = mouse_y;
		if (pointed_index == -1)
			return 0;
		if (pointed_index == S3_CHOOSEBOX_COUNT - 1)
			return 0;
		if (button[pointed_index + 1].text != NULL)
			return pointed_index + 1;
		else
			return 0;
	}

	/* Process the left arrow key. */
	if (s3_is_left_key_pressed()) {
		is_selected_by_key = true;
		save_mouse_x = mouse_x;
		save_mouse_y = mouse_y;
		if (pointed_index == -1 ||
		    pointed_index == 0) {
			for (i = S3_CHOOSEBOX_COUNT - 1; i >= 0; i--)
				if (button[i].text != NULL)
					return i;
		}
		return pointed_index - 1;
	}

	/* Process the mouse cursor. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		if (!button[i].is_enabled)
			continue;

		if (mouse_x >= button[i].x &&
		    mouse_x < button[i].x + button[i].w &&
		    mouse_y >= button[i].y &&
		    mouse_y < button[i].y + button[i].h) {
			/* If there is a pointed item by key, and the mouse cursor has not moved. */
			if (is_selected_by_key &&
			    mouse_x == save_mouse_x &&
			    mouse_y == save_mouse_y)
				continue;
			is_selected_by_key = false;
			return i;
		}
	}

	/* If not selected by key, keep the pointing index as is. */
	if (is_selected_by_key)
		return pointed_index;

	/* For other cases, no selection. */
	return -1;
}

/* Play a sound effect. */
static void
play_se(
	const char *file)
{
	if (file == NULL || strcmp(file, "") == 0)
		return;

	s3_set_mixer_input_file(S3_TRACK_SYS, file, false);
}

/* Run an anime. */
static void
run_anime(
	int unfocus_index,
	int focus_index)
{
	/* Anime for a choose box to be unfocused. */
	if (unfocus_index != -1 && conf_choose_idle_anime[unfocus_index] != NULL) {
		set_choose_call_arg(unfocus_index);
		s3_load_anime_from_file(conf_choose_idle_anime[unfocus_index], NULL, NULL);
	}

	/* Anime for a choose box to be focused. */
	if (focus_index != -1 && conf_choose_hover_anime[focus_index] != NULL) {
		set_choose_call_arg(focus_index);
		s3_load_anime_from_file(conf_choose_hover_anime[focus_index], NULL, NULL);
	}
}

/* Set call arguments for an anime. */
static void
set_choose_call_arg(
	int index)
{
	char layer_idle[32], layer_hover[32];
	int i;

	snprintf(layer_idle, sizeof(layer_idle), "choose%d-idle", index + 1);
	snprintf(layer_hover, sizeof(layer_hover), "choose%d-hover", index + 1);

	s3_set_call_argument(0, layer_idle);
	s3_set_call_argument(1, layer_hover);

 	for (i = 2; i < S3_CALL_ARGS; i++)
		s3_set_call_argument(i, NULL);
}

/* Cleanup. */
static bool
cleanup(void)
{
	int i;

	/* Stop anime sequences. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++)
		run_anime(i, -1);

	/* Hide the choose boxes. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++)
		s3_show_choosebox(i, false, false);

	/* If we need to transition to a system GUI. */
	if (need_sysmenu_mode) {
		/*
		 * Do not move to the next tag.
		 * We may visit this tag again if the GUI is canceled.
		 */
		return true;
	}

	/* If the timer bombed. */
	if (is_timer_fired) {
		/* Set the result empty. */
		s3_set_variable_string(result_var_name, "");

		/* Move to the next tag. */
		if (!s3_move_to_next_tag())
			return false;
		return true;
	}

	/* Set the result. */
	s3_set_variable_string(result_var_name, button[pointed_index].text);

	/*
	 * Now an option is chosen.
	 * Move to the label of the chosen item.
	 */
	assert(pointed_index != -1);
	return s3_move_to_next_tag();
}
