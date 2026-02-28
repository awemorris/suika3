/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Tag "choose" tag implementation
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
#include "command.h"
#include "conf.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* false assertion */
#define INVALID_BTN_INDEX (0)

/*
 * Buttons.
 */
struct choose_button {
	const char *msg;
	const char *label;
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
static int save_mouse_pos_x, save_mouse_pos_y;

/* Whether to ignore due to no effective option? */
static bool ignore_as_no_option;

/* Is the first frame? (to avoid playing an SE of mouse-over) */
static bool is_first_frame;

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
static bool is_bombed;

/* Stopwatch for the timer. */
static uint64_t bomb_sw;

/*
 * Forward Declaration
 */

/* Main processing. */
static void pre_process(void);
static bool blit_process(void);
static void render_process(void);
static bool post_process(void);

/* Initiaializatoin. */
static bool init(void);
static void draw_text(struct image *target, const char *text, int w, int h, bool is_bg);

/* Input processing. */
static void process_main_input(void);
static int get_pointed_index(void);

/* Misc. */
static void play_se(const char *file);
static void run_anime(int unfocus_index, int focus_index);

/* Cleanup.*/
static bool cleanup(void);

/*
 * The "choose" tag implementation.
 */
bool
s3i_command_choose(
	void *p)
{
	/* For the first frame, do initialization. */
	if (!is_in_command_repetition())
		if (!init())
			return false;

	/* In case of no options to show. */
	if (ignore_as_no_options)
		return move_to_next_command();

	main_process();

	/* For the last frame, do cleanup. */
	if (!is_in_command_repetition())
		if (!cleanup())
			return false;

	return true;
}

/* Initialization. */
bool init(void)
{
	int type;
	int actual_option_count;

	/* For when the DLL is reused, cleanup first. */
	cleanup();

	/* Clear variables. */
	pointed_index = -1;
	ignore_as_no_options = false;
	need_sysmenu_mode = false;
	is_first_frame = true;
	is_timed = false;
	is_bombed = false;
	reset_lap_timer(&bomb_sw);

	/* Collect the option infromation. */
	actual_option_count = 0;
	for (i = 0; i < CHOOSE_COUNT; i++) {
		bool has_label;
		bool has_text;
		char label[128];
		char text[128];

		snprintf(label, sizeof(label), "label%d", i + 1);
		snprintf(text, sizeof(text), "text%d", i + 1);

		/* Get the N-th options. */
		button[i].label = s3_get_tag_arg_string(label);
		button[i].text = s3_get_tag_arg_string(text);
		
		/* Get the coordinates. */
		s3_get_choose_rect(i,
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

		actual_option_count++;
	}

	/* If there is no option. */
	if (actual_option_count == 0) {
		ignore_as_no_options = true;
		return true;
	}

	/* Start a multi-frame execution. */
	start_command_repetition();

	/* Hide the name and message boxes. */
	if (!conf_msgbox_show_on_choose) {
		show_namebox(false);
		show_msgbox(true);
	}
	show_click(false);

	/* Stop the auto mode. */
	if (is_auto_mode()) {
		stop_auto_mode();
		show_automode_banner(false);
	}

	/* Stop the skip mode. */
	if (is_skip_mode()) {
		stop_skip_mode();
		show_skipmode_banner(false);
	}

	/* Initialize the timer. */
	if (s3_check_tag_arg("time")) {
		is_timed = true;
		timer_span = s3_get_tag_arg_float("time");
		s3_reset_laptime(&bomb_sw);
	} else {
		is_timed = false;
	}

	/* Disable the skip  behavior by continuos swipe. */
	set_continuous_swipe_enabled(false);

	return true;
}

/* Draw an option text to a choose box layer. */
static void
draw_text(
	int index,
	bool is_bg)
{
	struct draw_msg_context context;
	pixel_t color, outline_color;
	int layer;
	struct s3_image *img;
	int char_count;
	int x, y;

	x = 0;
	y = 0;

	/* Decide the color to draw. */
	if (is_bg) {
		color = s3_make_pixel(255,
				      conf_choose_font_idle_r,
				      conf_choose_font_idle_g,
				      conf_choose_font_idle_b);
		outline_color = s3_make_pixel(255,
					      conf_choose_font_hover_r,
					      conf_choose_font_hover_g,
					      conf_choose_font_hover_b);
	}

	/* Decide the position to draw. */
	if (is_centered) {
		if (!conf_choose_font_tategaki) {
			x = x + (w - get_string_width(conf_choose_font, conf_choose_font_size, text)) / 2;
			y += conf_choose_text_margin_top;
		} else {
			x = x + (w - conf_choose_font_size) / 2;
			y = y + (h - get_string_height(conf_choose_font, conf_choose_font_size, text)) / 2;
		}
	} else {
		y += conf_choose_text_margin_top;
	}

	/* Get the layer index. */
	layer = S3_LAYER_CHOOSE1_IDLE + (2 * index);
	if (!is_bg)
		layer++;

	/* Get the layer image. */
	img = s3_get_layer_image(layer);

	/* Draw a text. */
	construct_draw_msg_context(
		&context,
		img,
		text,
		conf_choose_font,
		conf_choose_font_size,
		conf_choose_font_size,	/* base_font_size */
		conf_choose_font_ruby,	/* FIXME: namebox.ruby.sizeの導入 */
		conf_choose_font_outline,
		x,
		y,
		conf_game_width,
		conf_game_height,
		x,			/* left_margin */
		0,			/* right_margin */
		conf_choose_text_margin_top,
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
	char_count = count_chars_common(&context, NULL);
	draw_msg_common(&context, char_count);
}

/* Main processing. */
static void
main_process(void)
{
	if (is_timed) {
		if ((float)get_lap_timer_millisec(&bomb_sw) >= conf_choose_timed * 1000.0f) {
			is_bombed = true;
			return;
		}
	}

	process_main_input();

	int i;

	if (is_bombed)
		return true;

	/*
	 * 必要な場合はステージのサムネイルを作成する
	 *  - クイックセーブされるとき
	 *  - システムGUIに遷移するとき
	 */
	if (need_sysmenu_mode) {
		draw_stage_to_thumb();
		for (i = 0; i < CHOOSE_COUNT; i++) {
			if (choose_button[i].img_idle == NULL)
				continue;
			draw_choose_to_thumb(choose_button[i].img_idle,
					     choose_button[i].x,
					     choose_button[i].y);
		}
	}

	if (need_sysmenu_mode) {
		if (!prepare_gui_mode(SYSMENU_GUI_FILE, true))
			return false;
		start_gui_mode();
	}

	/*
	 * 必要な場合は繰り返し動作を停止する
	 *  - 時間制限に達したとき
	 *  - システムGUIに遷移するとき
	 */
	if (is_bombed || need_sysmenu_mode)
		stop_command_repetition();

	is_first_frame = false;

	return true;
}

/* Process inputs. */ 
static void
process_main_input(void)
{
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
	    conf_tts &&
	    is_selected_by_key &&
	    button[new_pointed_index].msg != NULL) {
		speak_text(NULL);
		speak_text(choose_button[pointed_index].msg);
	}

	/* If a pointed index is changed and an item is pointed. */
	if (new_pointed_index != -1 &&
	    new_pointed_index != old_pointed_index) {
		/* Avoid the first frame. */
		if (!is_first_frame) {
			play_se(is_left_clicked ? conf_choose_click_se : conf_choose_change_se);
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
		if (i == new_pointed_index) {
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
	if (pointed_index != -1 && (is_left_clicked || is_return_pressed)) {
		play_se(conf_choose_click_se);
		stop_command_repetition();
		run_anime(pointed_index, -1);
	}

	/* If the system button is enabled. */
	if (conf_sysbtn_enable) {
		/* Start here and decide whether transit to the system menu or not. */
		enter_sysmenu = false;

		/* If right-clicked. */
		if (is_right_clicked)
			enter_sysmenu = true;

		/* If the escape key is pressed. */
		if (is_escape_pressed)
			enter_sysmenu = true;

		/* If the system button is clicked. */
		if (is_left_clicked && is_sysbtn_pointed())
			enter_sysmenu = true;

		/* If the system menu will be run. */
		if (enter_sysmenu) {
			run_anime(pointed_index, -1);
			need_sysmenu_mode = true;
		}
	}
}

/* Get the pointed index. */
static int get_pointed_index(void)
{
	int i;

	/* Process the right arrow key. */
	if (is_right_arrow_pressed) {
		is_selected_by_key = true;
		save_mouse_pos_x = mouse_pos_x;
		save_mouse_pos_y = mouse_pos_y;
		if (pointed_index == -1)
			return 0;
		if (pointed_index == CHOOSE_COUNT - 1)
			return 0;
		if (choose_button[pointed_index + 1].msg != NULL)
			return pointed_index + 1;
		else
			return 0;
	}

	/* Process the left arrow key. */
	if (is_left_arrow_pressed) {
		is_selected_by_key = true;
		save_mouse_pos_x = mouse_pos_x;
		save_mouse_pos_y = mouse_pos_y;
		if (pointed_index == -1 ||
		    pointed_index == 0) {
			for (i = CHOOSE_COUNT - 1; i >= 0; i--)
				if (choose_button[i].msg != NULL)
					return i;
		}
		return pointed_index - 1;
	}

	/* Process the mouse cursor. */
	for (i = 0; i < CHOOSE_COUNT; i++) {
		if (choose_button[i].msg == NULL)
			continue;

		if (mouse_pos_x >= choose_button[i].x &&
		    mouse_pos_x < choose_button[i].x + choose_button[i].w &&
		    mouse_pos_y >= choose_button[i].y &&
		    mouse_pos_y < choose_button[i].y + choose_button[i].h) {
			/* キーで選択済みの項目があり、マウスが移動していない場合 */
			if (is_selected_by_key &&
			    mouse_pos_x == save_mouse_pos_x &&
			    mouse_pos_y == save_mouse_pos_y)
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

	set_mixer_input_file(S3_TRACK_SYS, file);
}

/* Run an anime. */
static void
run_anime(
	int unfocus_index,
	int focus_index)
{
	/* Anime for a choose box to be unfocused. */
	if (unfocus_index != -1 && conf_choose_anime_unfocus[unfocus_index] != NULL)
		load_anime_from_file(conf_choose_anime_unfocus[unfocus_index], -1, NULL);

	/* Anime for a choose box to be focused. */
	if (focus_index != -1 && conf_choose_anime_focus[focus_index] != NULL)
		load_anime_from_file(conf_choose_anime_focus[focus_index], -1, NULL);
}

/* Cleanup. */
static bool
cleanup(void)
{
	int i;

	/* Stop anime sequences. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++)
		run_anime(i, -1);

	/* If we need to transition to a system GUI. */
	if (need_sysmenu_mode) {
		/*
		 * Do not move to the next tag.
		 * We may visit this tag again if the GUI is canceled.
		 */
		return true;
	}

	/* If the timer bombed. */
	if (is_bombed) {
		/* Move to the next tag. */
		if (!move_to_next_command())
			return false;
		return true;
	}

	/*
	 * Now an option is chosen.
	 * Move to the label of the chosen item.
	 */
	assert(pointed_index != -1);
	return move_to_label(button[pointed_index].label);
}
