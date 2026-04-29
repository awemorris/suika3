/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * GUI Subsystem
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
#include "gui.h"
#include "text.h"
#include "conf.h"
#include "stage.h"
#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

/* Button types. */
enum {
	/* Invalid button. */
	TYPE_INVALID = -1,

	/* No action. */
	TYPE_NOACTION = 0,

	/* Jump to a label when clicked. */
	TYPE_LABEL,

	/* Jump to another GUI. */
	TYPE_GUI,

	/* Script run button. */
	TYPE_SCRIPT,

	/* For the volumes. */
	TYPE_MASTERVOL,
	TYPE_BGMVOL,
	TYPE_VOICEVOL,
	TYPE_SEVOL,
	TYPE_CHARACTERVOL,

	/* Text speed slider. */
	TYPE_TEXTSPEED,

	/* Auto mode speed slider. */
	TYPE_AUTOSPEED,

	/* Text preview area. */
	TYPE_PREVIEW,

	/* Full screen mode button. */
	TYPE_FULLSCREEN,

	/* For the window mode. */
	TYPE_WINDOW,

	/* Default reset button. */
	TYPE_DEFAULT,

	/* Save/load page number button. */
	TYPE_SAVEPAGE,

	/* Save button. */
	TYPE_SAVE,

	/* Load button. */
	TYPE_LOAD,

	/* Auto mode button. */
	TYPE_AUTO,

	/* Skip mode button. */
	TYPE_SKIP,

	/* History button. */
	TYPE_HISTORY,

	/* History scroll bar. */
	TYPE_HISTORYSCROLL,

	/* History scroll bar (horizontal). */
	TYPE_HISTORYSCROLL_HORIZONTAL,

	/* Title button. */
	TYPE_TITLE,

	/* Cancel button. */
	TYPE_CANCEL,

	/* Variable view area. */
	TYPE_VAR,

	/* Variable input button. */
	TYPE_CHAR,

	/* Language button. */
	TYPE_LANGUAGE,
};

/* Save item entries. */
enum save_text_type {
	SAVE_TEXT_INDEX,
	SAVE_TEXT_DATE,
	SAVE_TEXT_CHAPTER,
	SAVE_TEXT_MSG,
};

/* Frame interval of dragging to mitigate high load. */
#if defined(HAL_TARGET_IOS) || \
    defined(HAL_TARGET_ANDROID) || \
    defined(HAL_TARGET_WASM)
#define DRAG_TICK_INTERVAL (30)
#else
#define DRAG_TICK_INTERVAL (20)
#endif

/* Button. */
struct gui_button {
	bool is_initialized;

	/*
	 * Properties written in GUI files.
	 */

	/* Button ID */
	int bid;

	/* Button type. */
	int type;

	/* Position and size. */
	int x;
	int y;
	int width;
	int height;

	/* Common. */
	char *alt;
	int order;
	char *clickse;
	char *pointse;

	/* TYPE_LABEL */
	char *label;

	/* TYPE_GUI, TYPE_MASTERVOL, TYPE_BGMVOL, TYPE_VOICEVOL, TYPE_SEVOL */
	char *file;

	/* TYPE_SAVEPAGE, TYPE_LOADPAGE, TYPE_SAVE, TYPE_LOAD, TYPE_CHARACTERVOL */
	int index;

	/* TYPE_PREVIEW, TYPE_CHAR */
	char *msg;

	/* TYPE_PREVIEW */
	char *msg_en;
	char *msg_fr;
	char *msg_es;
	char *msg_it;
	char *msg_de;
	char *msg_ru;
	char *msg_el;
	char *msg_zh_cn;
	char *msg_zh_tw;
	char *msg_ja;

	/* TYPE_VAR, TYPE_CHAR */
	char *var;

	/* TYPE_LANGUAGE */
	char *lang;

	/* TYPE_CHAR */
	int max;

	/* TYPE_HISTORY, TYPE_SAVE, TYPE_LOAD, TYPE_PREVIEW */
	int clear_r, clear_g, clear_b;

	/* Anime */
	char *anime_idle;
	char *anime_hover;
	char *anime_press;
	char *anime_active;

	/* TYPE_SAVE, TYPE_LOAD */
	int index_x;
	int index_y;
	int date_x;
	int date_y;
	int thumb_x;
	int thumb_y;
	int thumb_width;
	int thumb_height;
	int chapter_x;
	int chapter_y;
	int msg_x;
	int msg_y;
	int new_x;
	int new_y;

	/* TYPE_HISTORY */
	int name_x;
	int name_y;
	int text_x;
	int text_y;

	/* TYPE_PREVIEW/TYPE_SAVE/TYPE_LOAD */
	int margin_left;
	int margin_top;
	int margin_right;
	int margin_bottom;

	/*
	 * Runtime information.
	 */
	struct {
		bool is_disabled;

		/* Is pointed by keyboard input? */
		bool is_selected_by_key;

		/* Is dragging? */
		bool is_dragging;

		/* Is pressed? */
		bool is_pressed;

		/* Slider value. */
		float slider;

		/* Images. */
		struct s3_image *img_idle;
		struct s3_image *img_hover;
		struct s3_image *img_press;
		struct s3_image *img_active;
		struct s3_image *img_disable;
		struct s3_image *img_canvas_idle;
		struct s3_image *img_canvas_hover;
		struct s3_image *img_canvas_press;

		/* Offset for hitstory. */
		int history_offset;

		/* Whether to show NEW button for when TYPE_SAVE/TYPE_LOAD. */
		bool is_new_enabled;

		/* Anime used layers */
		bool used_layers[S3_STAGE_LAYERS];

		/*
		 * Text preview information.
		 */

		/* Is in auto mode wait. */
		bool is_waiting;

		/* Drawing context. */
		struct s3_drawmsg *msg_context;

		/* Total character to draw. */
		int total_chars;

		/* Already drawn characters. */
		int drawn_chars;

		/* Stopwatch for message showing and auto mode wait. */
		uint64_t sw;
	} rt;
};
static struct gui_button button[S3_BUTTON_LAYERS];

/* Is GUI running? */
static bool is_gui_running;

/* Is a system GUI? (Called by the system button) */
static bool is_sys_gui;

/* Is cancel by clicking on the margin? */
static bool is_click_cancel;

/* Is cancel by pressing the escape key? */
static bool is_escape_cancel;

/* GUI file being processed. */
static const char *gui_file;

/* Cancel sound effect. */
static char *cancel_se;

/* Fade-in time */
static float fade_in_time;

/* Fade-out time */
static float fade_out_time;

/* Is finished */
static bool is_finished;

/* Index of the pointed button */
static int pointed_index;

/* Index of the button that was pointed in the previous frame. */
static int prev_pointed_index;

/* Is pointed by keyboard input? */
static bool is_pointed_by_key;

/* Is dragging finished? */
static bool is_drag_finished;

/* Is mouse pointing of a button detected? */
static bool is_mouse_point_detected;

/* Mouse coordinates when pointed by keyboard input. */
static int save_mouse_pos_x, save_mouse_pos_y;

/* Index of the button where dragging started. */
static int dragging_index;

/* Index of the button that was selected as a result. */
static int result_index;

/* Text speed during dragging */
static float transient_text_speed;

/* Auto mode speed during dragging */
static float transient_auto_speed;

/* Page index of the save/load screen. */
static int save_page;

/* Slot count per page in the save/load screen. */
static int save_slots;

/* Is saved in this frame? */
static bool is_saved_in_this_frame;

/* Is a frame to suppress SE playback? */
static bool suppress_se;

/* Is SE already played, so suppress further playback? */
static bool suppress_se_forever;

/* Slot count per page in the history. */
static int history_slots;

/* Index of the top history item to display (history offset number for index=0) */
static int history_top;

/* Value of the history slider during dragging. */
static float transient_history_slider;

/* Is update of history buttons needed? */
static bool need_update_history_buttons;

/* Is update of variable buttons needed? */
static bool need_update_var_buttons;

/* Is fading in? */
static bool is_fading_in;

/* Is fading out? */
static bool is_fading_out;

/* Stopwatch for fade. */
static uint64_t fade_sw;

/* Current alpha value of the fade. */
static int cur_alpha;

/* Is first frame? */
static bool is_first_frame;

/* Is load failed? */
static bool is_load_failed;

/* Did save? */
static bool did_save;

/* Did load? */
static bool did_load;

/* Did chain? */
static bool did_chain;

/* Tick management. */
static uint64_t cur_tick;
static uint64_t last_drag_tick;

/* Time limit. */
static float bomb_time;
static uint64_t bomb_sw;
static bool is_bombed;

/*
 * Forward declarations.
 */
static void process_input(void);
static void process_timeout(void);
static void process_blit(void);
static void process_render(void);
static bool process_action(void);
static void process_se(void);
static bool process_left_right_arrow_keys(void);
static bool add_button(int index, const char *file);
static int get_unused_bid(void);
static int is_bid_unused(int bid);
static bool set_global_key_value(const char *key, const char *val, const char *file, int line);
static bool set_button_key_value(const int index, const char *key, const char *val, const char *file, int line);
static int get_type_for_name(const char *name, const char *file, int line);
static void update_runtime_props(bool is_first_time);
static bool move_to_other_gui(void);
static bool move_to_title(void);
static bool process_button_point(int index, bool key);
static bool process_button_drag(int index);
static float calc_slider_value(int index);
static bool process_button_click(int index);
static void process_button_render(int index);
static void process_button_render_slider(int index);
static void process_button_render_slider_vertical(int index);
static void process_button_render_generic(int index);
static void process_button_render_language(int index);
static void process_button_render_var(int index);
static void process_button_render_preview(int index);
static void process_button_render_save(int index);
static void process_button_render_page(int index);
static bool init_save_buttons(void);
static void update_save_buttons(void);
static void draw_save_button(int button_index);
static int draw_save_text_item(struct s3_image *target, int index, int x, int y, const char *text, int save_text_type);
static void process_save(int b_index);
static void process_load(int button_index);
static void process_auto_mode(void);
static void process_skip_mode(void);
static bool init_history_buttons(void);
static void draw_history_buttons(void);
static void draw_history_button(struct s3_image *target, struct s3_image *source, int index);
static void draw_history_name_item(struct s3_image *target, int index, const char *name);
static void draw_history_text_item(struct s3_image *target, int index, const char *text, bool has_name);
static void process_button_render_history(int button_index);
static void process_history_scroll_up(void);
static void process_history_scroll_down(void);
static void process_history_scroll_at(float pos, bool force);
static float adjust_history_slider_value(float pos);
static void process_history_voice(int button_index);
static bool init_preview_buttons(void);
static void reset_preview_buttons(void);
static void reset_preview_button(int index);
static const char *get_localized_preview_message(int index);
static void draw_preview_buttons(void);
static void draw_preview_button(int index);
static void draw_preview_message(int index);
static int get_frame_chars(int index);
static int get_wait_time(int index);
static bool init_var_buttons(void);
static void draw_var_buttons(void);
static void draw_var_button(int index);
static void draw_var_value(int index);
static void process_char(int index);
static void truncate_variable(const char *var);
static void process_language(int index);
static void render_image_helper(struct s3_image *img, int bid);
static void play_se(const char *file);
static void play_sys_se(const char *file);
static void speak(const char *text);
static bool load_gui_file(const char *file);
static void set_gui_call_arg(int bid);
static void stop_button_anime(int index);
static bool start_button_anime(int index, const char *anime_file);
static void change_save_page(int index);
static bool is_current_language(const char *lang);

/*
 * Initialize the GUI subsystem.
 */
bool
s3i_init_gui(void)
{
	/* Cleanup in case the DLL is reused. */
	s3i_cleanup_gui();

	return true;
}

/*
 * Cleanup the GUI subsystem.
 */
void
s3i_cleanup_gui(void)
{
	int i, j;

	/* Initialize the flags. */
	is_gui_running = false;

	/* Free button resources. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].alt != NULL)
			free(button[i].alt);
		if (button[i].clickse != NULL)
			free(button[i].clickse);
		if (button[i].pointse != NULL)
			free(button[i].pointse);
		if (button[i].label != NULL)
			free(button[i].label);
		if (button[i].file != NULL)
			free(button[i].file);
		if (button[i].msg != NULL)
			free(button[i].msg);
		if (button[i].msg_en != NULL)
			free(button[i].msg_en);
		if (button[i].msg_fr != NULL)
			free(button[i].msg_fr);
		if (button[i].msg_es != NULL)
			free(button[i].msg_es);
		if (button[i].msg_de != NULL)
			free(button[i].msg_de);
		if (button[i].msg_it != NULL)
			free(button[i].msg_it);
		if (button[i].msg_ru != NULL)
			free(button[i].msg_ru);
		if (button[i].msg_el != NULL)
			free(button[i].msg_el);
		if (button[i].msg_zh_cn != NULL)
			free(button[i].msg_zh_cn);
		if (button[i].msg_zh_tw != NULL)
			free(button[i].msg_zh_tw);
		if (button[i].msg_ja != NULL)
			free(button[i].msg_ja);
		if (button[i].var != NULL)
			free(button[i].var);
		if (button[i].lang != NULL)
			free(button[i].lang);
		if (button[i].anime_idle != NULL)
			free(button[i].anime_idle);
		if (button[i].anime_hover != NULL)
			free(button[i].anime_hover);
		if (button[i].anime_active != NULL)
			free(button[i].anime_active);
		if (button[i].rt.img_idle != NULL)
			s3_destroy_image(button[i].rt.img_idle);
		if (button[i].rt.img_hover != NULL)
			s3_destroy_image(button[i].rt.img_hover);
		if (button[i].rt.img_press != NULL)
			s3_destroy_image(button[i].rt.img_press);
		if (button[i].rt.img_active != NULL)
			s3_destroy_image(button[i].rt.img_active);
		if (button[i].rt.img_disable != NULL)
			s3_destroy_image(button[i].rt.img_disable);
		if (button[i].rt.img_canvas_idle != NULL)
			s3_destroy_image(button[i].rt.img_canvas_idle);
		if (button[i].rt.img_canvas_hover != NULL)
			s3_destroy_image(button[i].rt.img_canvas_hover);
		if (button[i].rt.img_canvas_press != NULL)
			s3_destroy_image(button[i].rt.img_canvas_press);
		if (button[i].rt.msg_context != NULL)
			s3_destroy_drawmsg(button[i].rt.msg_context);
	}

	/* Stop all animations. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type == TYPE_INVALID)
			continue;
		for (j = 0; j < S3_STAGE_LAYERS; j++) {
			if (button[i].rt.used_layers[j]) {
				s3_clear_layer_anime_sequence(j);
				button[i].rt.used_layers[j] = false;
			}
		}
	}

	/* Zero out buttons. */
	memset(button, 0, sizeof(button));
	for (i = 0; i < S3_BUTTON_LAYERS; i++)
		button[i].bid = -1;

	/* Free strings. */
	if (cancel_se != NULL) {
		free(cancel_se);
		cancel_se = NULL;
	}
}

/*
 * Check if right after returned from a GUI.
 */
bool
s3_check_right_after_sys_gui(void)
{
	if (is_sys_gui) {
		is_sys_gui = false;
		return true;
	}

	return false;
}

/*
 * Load a GUI file and prepare for a start.
 */
bool
s3_load_gui_file(
	const char *file,
	bool sys)
{
	bool success;
	int i;

	assert(!is_gui_running);

   	/* Cleanup previous GUI if any. */
	s3i_cleanup_gui();

	/* Store properties. */
	gui_file = file;
	is_sys_gui = sys;

	/* Zero out buttons. */
	memset(button, 0, sizeof(button));
	for (i = 0; i < S3_BUTTON_LAYERS; i++)
		button[i].bid = -1;

	/* Set initial values for global properties. */
	save_page = conf_gui_save_last_page;
	save_slots = 0;
	history_slots = 0;
	history_top = -1;
	fade_in_time = 0;
	fade_out_time = 0;
	did_save = false;
	did_load = false;
	did_chain = false;
	is_finished = false;
	dragging_index = -1;
	is_drag_finished = false;
	bomb_time = 0;
	is_click_cancel = false;
	is_escape_cancel = false;
	is_load_failed = false;

	success = false;
	do {
		/* Open the GUI file */
		if (!load_gui_file(gui_file))
			break;

		/* Initialize TYPE_PREVIEW buttons. */
		if (!init_preview_buttons())
			break;

		/* Initialize TYPE_SAVE/TYPE_LOAD buttons. */
		if (!init_save_buttons())
			break;

		update_save_buttons();

		/* Initialize TYPE_HISTORY buttons. */
		if (!init_history_buttons())
			break;

		draw_history_buttons();

		/* Initialize TYPE_VAR buttons. */
		if (!init_var_buttons())
			break;

		draw_var_buttons();

		success = true;
	} while (0);
	if (!success) {
		s3i_cleanup_gui();
		return false;
	}

	/* Prepare the state of buttons. */
	update_runtime_props(true);

	/* Set fade-in if needed. */
	is_fading_in = fade_in_time > 0;
	is_fading_out = false;

	return true;
}

/*
 * Start the loaded GUI.
 */
void
s3_start_gui(void)
{
	int i;

	assert(!is_gui_running);

	/* Enable GUI mode. */
	is_gui_running = true;
	is_first_frame = true;
	pointed_index = -1;
	result_index = -1;
	is_pointed_by_key = false;
	if (is_fading_in)
		s3_reset_lap_timer(&fade_sw);
	is_fading_out = false;
	is_saved_in_this_frame = false;
	suppress_se = false;
	suppress_se_forever = false;

	/* Copy positions to stage layers using anime reset. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++)
		stop_button_anime(i);

	/* Start the idle animations. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++)
		start_button_anime(i, button[i].anime_idle);

	/* Hide the sysbtn. */
	s3_enable_sysbtn(false);

	/* Disable skip action by continuous swipe. */
	s3_set_continuous_swipe_enabled(false);

	/* Restart save page animations. */
	save_page = -1; /* Force change. */
	change_save_page(conf_gui_save_last_page);

	update_runtime_props(false);
}

/*
 * Stop the running GUI.
 */
void
s3_stop_gui(void)
{
	int i, j;

	assert(is_gui_running);

	/* Stop all animations. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type == TYPE_INVALID)
			continue;
		for (j = 0; j < S3_STAGE_LAYERS; j++)
			if (button[i].rt.used_layers[j]) {
				s3_clear_layer_anime_sequence(j);
				button[i].rt.used_layers[j] = false;
			}
	}

	/* Disable GUI mode. */
	is_gui_running = false;
}

/*
 * Check if a GUI is running.
 */
bool
s3_is_gui_running(void)
{
	if (is_gui_running) {
		if (is_finished)
			return false;
		return true;
	}
	return false;
}

/*
 * Chech if a GUI is finished.
 */
bool
s3_is_gui_finished(void)
{
	if (!is_gui_running) {
		return true;
	} else {
		if (!is_finished)
			return false;
		return true;
	}
}

/*
 * Run a GUI frame update.
 */
bool
s3i_run_gui_update(void)
{
	float progress;

	/* If not in GUI mode. */
	if (!is_gui_running)
		return true;

	/* If first frame, reset the first frame flag. */
	if (is_first_frame) {
		is_first_frame = false;
	}

	/* Calculate alpha value during fade-in and fade-out */
	cur_alpha = 255;
	if (is_fading_in) {
		/* Process fade-in. */
		progress = (float)s3_get_lap_timer_millisec(&fade_sw) / 1000.0f / fade_in_time;
		if (progress < 1.0f) {
			/* Continue fade-in. */
			cur_alpha = (int)(progress * 255.0f);
		} else {
			/* End fade-in. */
			is_fading_in = false;
			cur_alpha = 255;
		}
	} else if (is_fading_out) {
		/* Process fade-out. */
		progress = (float)s3_get_lap_timer_millisec(&fade_sw) / 1000.0f / fade_out_time;
		if (progress < 1.0f) {
			/* Continue fade-out. */
			cur_alpha = 255 - (int)(progress * 255.0f);
		} else {
			/* End fade-out. */
			is_fading_out = false;
			is_finished = true;
			cur_alpha = 0;
		}
	}

	/* Handle time limit. */
	process_timeout();

	/* Process input. */
	process_input();
	if (did_load)
		return true;

	/* If returning to the title. */
	if (!is_bombed &&
	    result_index != -1 &&
	    button[result_index].type == TYPE_TITLE)
		return move_to_title();

	/* Process image updates. */
	process_blit();

	/* Process SE feedback. */
	process_se();

	/* Process actions. */
	if (!process_action())
		return false;

	is_first_frame = false;

	/* Exit if failed to load. */
	if (is_load_failed)
		return false;

	/* Increment the tick count. */
	cur_tick++;

	return true;
}

/*
 * Run a GUI frame rendering.
 */
bool
s3i_run_gui_render(void)
{
	assert(is_gui_running);

	if (did_load)
		return true;

	process_render();

	return true;
}

/*
 * Get the label of the selected button.
 */
const char *
s3_get_gui_result_label(void)
{
	struct gui_button *b;

	if (is_fading_in)
		return NULL;

	if (result_index == -1)
		return NULL;

	b = &button[result_index];

	if (b->type != TYPE_LABEL)
		return NULL;

	return b->label;
}

/*
 * Check if the selected GUI button is "back to title".
 */
bool
s3_is_gui_result_title(void)
{
	struct gui_button *b;

	if (is_fading_in)
		return false;

	if (result_index == -1)
		return false;

	b = &button[result_index];

	if (b->type != TYPE_TITLE)
		return false;

	return true;
}

/*
 * Check if any save is issued in the current GUI.
 */
bool
s3_check_if_saved_in_gui(void)
{
	if (did_save)
		return true;

	return false;
}

/*
 * Check if a load is issued in the current GUI.
 */
bool
s3_check_if_loaded_in_gui(void)
{
	if (did_load)
		return true;

	return false;
}

/*
 * Update
 */

/* Process timeout. */
static void
process_timeout(void)
{
	if (bomb_time == 0)
		return;

	if ((float)s3_get_lap_timer_millisec(&bomb_sw) >= bomb_time * 1000.0f) {
		is_bombed = true;
		result_index = -1;
		if (fade_out_time > 0) {
			is_fading_out= true;
			s3_reset_lap_timer(&fade_sw);
		} else {
			is_finished = true;
			s3_clear_input_state();
		}
	}
}

/* Process input. */
static void process_input(void)
{
	int i;
	bool is_key_detected;
	bool is_drag_processed;

	/* Do not accept input during fade. */
	if (is_fading_in || is_fading_out)
		return;

	/* Do not accept input when time limit is reached. */
	if (is_bombed)
		return;

	prev_pointed_index = pointed_index;
	need_update_history_buttons = false;

	/* Process scrolling by mouse wheel, up/down keys, or swipe. */
	if (s3_is_down_key_pressed()) {
		if (s3_is_swiped())
			process_history_scroll_up();
		else
			process_history_scroll_down();
		update_runtime_props(false);
	} else if (s3_is_up_key_pressed()) {
		if (s3_is_swiped())
			process_history_scroll_down();
		else
			process_history_scroll_up();
		update_runtime_props(false);
	}

	/* Process left and right arrow keys. */
	is_key_detected = false;
	if (!is_fading_in && !is_fading_out)
		is_key_detected = process_left_right_arrow_keys();

	/* Update the state of mouse pointing. */
	if (!is_key_detected && !is_pointed_by_key && !is_fading_in && !is_fading_out) {
		is_mouse_point_detected = false;
		for (i = S3_BUTTON_LAYERS - 1; i >= 0; i--) {
			if (!button[i].is_initialized)
				continue;

			/* Update pointed state. */
			if (process_button_point(i, false))
				break;
		}
		if (i == -1) {
			/* No pointed button. */
			pointed_index = -1;
		}
	}

	/* Update the state of mouse dragging. */
	is_drag_processed = false;
	if (!is_key_detected && !is_pointed_by_key && !is_fading_in && !is_fading_out) {
		is_drag_finished = false;
		for (i = S3_BUTTON_LAYERS - 1; i >= 0; i--) {
			if (!button[i].is_initialized)
				continue;

			/* Update drag state. */
			if (process_button_drag(i)) {
				is_drag_processed = true;
				break;
			}
		}
	}

	/* Update the state of mouse click. */
	if (!is_drag_processed && !is_fading_in && !is_fading_out) {
		for (i = S3_BUTTON_LAYERS - 1; i >= 0; i--) {
			/* Get click result. */
			if(process_button_click(i))
				break;
		}
	}

	/* Adjust pointing after dragging is finished. */
	if (is_drag_finished) {
		is_mouse_point_detected = false;
		for (i = S3_BUTTON_LAYERS - 1; i >= 0; i--) {
			if (!button[i].is_initialized)
				continue;
			if (process_button_point(i, false))
				break;
		}
		is_drag_finished = false;
	}

	/* If the pointed button is changed. */
	if (prev_pointed_index != pointed_index && prev_pointed_index != -1) {
		/* Reload the anime. */
		if (button[prev_pointed_index].type == TYPE_SAVEPAGE &&
		    !button[prev_pointed_index].rt.is_disabled) {
			/* Active --> Active (Stay as is.) */
		} else {
			/* Hover --> Inactive */
			stop_button_anime(prev_pointed_index);
			start_button_anime(prev_pointed_index, button[prev_pointed_index].anime_idle);
		}
	}
	if (prev_pointed_index != pointed_index && pointed_index != -1) {
		/* Reload the anime. */
		if (button[pointed_index].type == TYPE_SAVEPAGE &&
		    !button[pointed_index].rt.is_disabled) {
			/* Active --> Active (Stay as is.) */
		} else {
			/* Inactive --> Hover */
			stop_button_anime(pointed_index);
			start_button_anime(pointed_index, button[pointed_index].anime_hover);
		}
	}

	/* If a button is chosen, exit. */
	if (result_index != -1) {
		if (fade_out_time > 0 &&
		    (button[result_index].type != TYPE_GUI &&
		     button[result_index].type != TYPE_TITLE)) {
			is_fading_out = true;
			s3_reset_lap_timer(&fade_sw);
		} else {
			is_finished = true;
		}
	}

	/* If cancel is possible by clicking on the margin. */
	if (is_click_cancel && pointed_index == -1 && s3_is_mouse_left_clicked() && !is_first_frame) {
		/* Perform fade out or finish processing. */
		if (fade_out_time > 0) {
			is_fading_out = true;
			s3_reset_lap_timer(&fade_sw);
			s3_clear_input_state();
		} else {
			is_finished = true;
			s3_clear_input_state();
		}
	}

	/* If cancel is possible by pressing the escape key. */
	if (is_escape_cancel && s3_is_escape_key_pressed()) {
		/* Perform fade out or finish processing. */
		if (fade_out_time > 0) {
			is_fading_out = true;
			s3_reset_lap_timer(&fade_sw);
		} else {
			is_finished = true;
			s3_clear_input_state();
		}
	}

	/* Avoid affecting subsequent command execution. */
	s3_clear_input_state();
}

/* Process image drawing. */
static void
process_blit(void)
{
	/* Update history buttons if needed. */
	if (need_update_history_buttons) {
		draw_history_buttons();
		need_update_history_buttons = false;
	}

	/* Update name variable buttons if needed. */
	if (need_update_var_buttons) {
		draw_var_buttons();
		need_update_var_buttons = false;
	}

	/* Update preview buttons if needed. */
	draw_preview_buttons();
}

/* Update the state of buttons. */
static void
update_runtime_props(bool is_first_time)
{
	int i;

	/* If this is the first call. */
	if (is_first_time) {
		transient_text_speed = s3_get_text_speed();
		transient_auto_speed = 1.0f - s3_get_auto_speed();
	}

	/* Update each button. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		switch (button[i].type) {
		case TYPE_MASTERVOL:
			button[i].rt.slider = s3_get_master_volume();
			break;
		case TYPE_BGMVOL:
			button[i].rt.slider = s3_get_mixer_global_volume(S3_TRACK_BGM);
			break;
		case TYPE_VOICEVOL:
			button[i].rt.slider = s3_get_mixer_global_volume(S3_TRACK_VOICE);
			break;
		case TYPE_SEVOL:
			button[i].rt.slider = s3_get_mixer_global_volume(S3_TRACK_SE);
			break;
		case TYPE_CHARACTERVOL:
			button[i].rt.slider = s3_get_character_volume(button[i].index);
			break;
		case TYPE_TEXTSPEED:
			button[i].rt.slider = transient_text_speed;
			break;
		case TYPE_AUTOSPEED:
			button[i].rt.slider = transient_auto_speed;
			break;
		case TYPE_FULLSCREEN:
			if (!s3_is_full_screen_supported())
				break;
			button[i].rt.is_disabled = s3_is_full_screen_mode();
			break;
		case TYPE_WINDOW:
			if (!s3_is_full_screen_supported())
				break;
			button[i].rt.is_disabled = !s3_is_full_screen_mode();
			break;
		case TYPE_SAVEPAGE:
			if (button[i].index == save_page)
				button[i].rt.is_disabled = false;
			else
				button[i].rt.is_disabled = true;
			break;
#if 0
		case TYPE_HISTORYSCROLL:
		case TYPE_HISTORYSCROLL_HORIZONTAL:
			button[i].rt.slider = adjust_history_slider_value(transient_history_slider);
			process_history_scroll_at(button[i].rt.slider, false);
			break;
#endif
		case TYPE_LANGUAGE:
			button[i].rt.is_disabled = is_current_language(button[i].lang);
			break;
		default:
			break;
		}
	}
}

/* Process actions. */
static bool
process_action(void)
{
	if (!is_finished)
		return true;
	if (is_fading_in || is_fading_out)
		return true;

	/* If moving to another GUI */
	if (result_index != -1 &&
	    button[result_index].type == TYPE_GUI) {
		did_chain = false;
		return move_to_other_gui();
	}

	/* If cancel. */
	if (result_index != -1 &&
	    button[result_index].type == TYPE_CANCEL) {
		/* Do nothing here. Processed by cmd_gui.c */
	}

	/* If starting auto mode. */
	if (result_index != -1 && button[result_index].type == TYPE_AUTO) {
		if (s3_is_skip_mode())
			s3_stop_skip_mode();
		s3_show_automode_banner(true);
		s3_start_auto_mode();
	}

	/* If starting skip mode. */
	if (result_index != -1 && button[result_index].type == TYPE_SKIP) {
		if (s3_is_auto_mode())
			s3_stop_auto_mode();
		s3_show_skipmode_banner(true);
		s3_start_skip_mode();
	}

	return true;
}

/* Process SE playback. */
static void
process_se(void)
{
	/* Do not play on the first frame. */
	if (is_first_frame)
		return;

	/* Do not play on frames where SE is suppressed. */
	if (suppress_se) {
		suppress_se = false;
		return;
	}

	/* Do not play on frames that have been saved. */
	if (is_saved_in_this_frame) {
		suppress_se = true;
		is_saved_in_this_frame = false;
		return;
	}

	/* If a button is selected. */
	if (!suppress_se_forever) {
		if (result_index == -1 && (is_fading_out || is_finished)) {
			/* If cancel. */
			play_sys_se(cancel_se);

			/* Do not play SE anymore. */
			suppress_se_forever = true;
			return;
		} else if (result_index != -1) {
			/* Do not play SE for volume-related buttons. */
			if (button[result_index].type == TYPE_MASTERVOL ||
			    button[result_index].type == TYPE_BGMVOL ||
			    button[result_index].type == TYPE_VOICEVOL ||
			    button[result_index].type == TYPE_SEVOL ||
			    button[result_index].type == TYPE_CHARACTERVOL)
				return;

			/* Play click sound. */
			play_sys_se(button[result_index].clickse);

			/* Do not play SE anymore. */
			if (is_fading_out || is_finished)
				suppress_se_forever = true;
			return;
		}
	}

	/* If a different button was pointed at compared to the previous frame. */
	if (pointed_index != prev_pointed_index && pointed_index != -1)
		play_sys_se(button[pointed_index].pointse);
}

/* Process left and right arrow keys. */
static bool
process_left_right_arrow_keys(void)
{
	int i, search_start;

	search_start = -1;

	/* Process right arrow key. */
	if (s3_is_right_key_pressed()) {
		/* Find the selected button. */
		search_start = pointed_index + 1;
		if (search_start == S3_BUTTON_LAYERS)
			search_start = 0;

		/* Search for a button to select. */
		for (i = search_start; i < S3_BUTTON_LAYERS; i++) {
			if (process_button_point(i, true))
				return true;
		}

		/* Wrap the search. */
		if (search_start == 0)
			return false;
		for (i = 0; i < search_start; i++)
			if (process_button_point(i, true))
				return true;
		return false;
	}

	/* Process left arrow key. */
	if (s3_is_left_key_pressed()) {
		/* Find the selected button. */
		if (pointed_index == -1 || pointed_index == 0)
			search_start = S3_BUTTON_LAYERS - 1;
		else
			search_start = pointed_index - 1;

		/* Search for a button to select. */
		for (i = search_start; i >= 0; i--)
			if (process_button_point(i, true))
				return true;

		/* Wrap the search. */
		if (search_start == S3_BUTTON_LAYERS -1)
			return false;
		for (i = S3_BUTTON_LAYERS - 1; i > search_start; i--)
			if (process_button_point(i, true))
				return true;
	}

	return false;
}

/* Move to another GUI. */
static bool
move_to_other_gui(void)
{
	char *file;

	/* Copy the file name. (it becomes inaccessible due to cleanup_gui()) */
	file = strdup(button[result_index].file);
	if (file == NULL) {
		s3_log_out_of_memory();
		s3i_cleanup_gui();
		return false;
	}

	/* Stop GUI mode. */
	s3_stop_gui();

	/* Destroy the current GUI. */
	s3i_cleanup_gui();

	/* Load the GUI. */
	if (!s3_load_gui_file(file, is_sys_gui)) {
		free(file);
		return false;
	}
	free(file);

	/* Start the GUI. */
	s3_start_gui();

	/* Process fade-in. */
	if (is_fading_in)
		cur_alpha = 0;

	return true;
}

/* Move to title. */
static bool
move_to_title(void)
{
	const char *file;
	int i;

	file = button[result_index].file;

	/* If the file is not specified. */
	if (file == NULL)
		return true;

	/* Save the read flag corresponding to the current script. */
	if (!s3_save_seen())
		return false;

	/* Clear the stage. */
	s3_clear_stage();

	/* Clear the state of Ciel. */
	//ciel_clear_hook();

	/* Unset local variables. */
	s3_unset_local_variables();

	/* Load the script. */
	if (!s3_move_to_tag_file(file))
		return false;

	/* Cancel if the @gui command is being executed. */
	if (s3_is_in_command_repetition())
		s3_stop_command_repetition();

	/*
	 * If called during a message or choice
	 *  - In this case, it is not treated as returning from the system GUI.
	 */
	if (is_sys_gui)
		is_sys_gui = false;

	/* Deactivate the message. */
	if (s3_is_message_active())
		s3_clear_message_active();

	/* Stop the GUI. */
	s3_stop_gui();

	/* Destroy the current GUI. */
	s3i_cleanup_gui();

	/* Clear the stage. */
	s3_clear_stage();

	/* Stop the audio. */
	for (i = 0; i < S3_MIXER_TRACKS; i++) {
		if (i != S3_TRACK_SYS)
			s3_set_mixer_input_file(i, NULL, false);
	}

	did_load = true;

	return true;
}

/* Process changes in button pointing state. */
static bool
process_button_point(int index, bool key)
{
	struct gui_button *b;
	int mouse_pos_x, mouse_pos_y;

	b = &button[index];

	/* If the button is TYPE_NOACTION, it cannot be pointed at. (no button) */
	if (b->type == TYPE_NOACTION)
		return false;

	/* If the button is TYPE_LOAD and there is no save data, it cannot be pointed at. */
	if (b->type == TYPE_LOAD && b->rt.is_disabled)
		return false;

	/* If the button is TYPE_LABEL and the button is disabled, it cannot be pointed at. */
	if (b->type == TYPE_LABEL && b->rt.is_disabled)
		return false;

	/* If the button is TYPE_FULLSCREEN and the button is active, it cannot be pointed at. */
	if (b->type == TYPE_FULLSCREEN && b->rt.is_disabled)
		return false;

	/* If the button is TYPE_WINDOW and the button is active, it cannot be pointed at. */
	if (b->type == TYPE_WINDOW && b->rt.is_disabled)
		return false;

	/* If the button is TYPE_HISTORY and the button is inactive, it cannot be pointed at. */
	if (b->type == TYPE_HISTORY && b->rt.is_disabled)
		return false;

	/* If the button is TYPE_PREVIEW, it cannot be pointed at. */
	if (b->type == TYPE_PREVIEW)
		return false;

	/* If the button is TYPE_VAR, it cannot be pointed at. */
	if (b->type == TYPE_VAR)
		return false;

	/* If another item is being dragged. */
	if (dragging_index != -1 && dragging_index != index)
		return false;

	mouse_pos_x = s3_get_mouse_pos_x();
	mouse_pos_y = s3_get_mouse_pos_y();

	/* If it is a key operation. */
	if (key) {
		printf("key: %d\n", index);
		if (index == pointed_index)
			return false;

		/* Set it to the pointed state. */
		pointed_index = index;
		is_pointed_by_key = true;
		save_mouse_pos_x = mouse_pos_x;
		save_mouse_pos_y = mouse_pos_y;
		is_mouse_point_detected = true;

		/* Speak. */
		if (conf_tts_enable)
			speak(button[index].alt);
		return true;
	}


	/* If the mouse is within the button area. */
	if (mouse_pos_x >= b->x && mouse_pos_x <= b->x + b->width &&
	    mouse_pos_y >= b->y && mouse_pos_y <= b->y + b->height) {
		/* If the item is already selected by key. */
		if (is_pointed_by_key && index == pointed_index)
			return false;

		/* If an item is already selected by key and the mouse has not moved. */
		if (is_pointed_by_key &&
		    mouse_pos_x == save_mouse_pos_x &&
		    mouse_pos_y == save_mouse_pos_y)
			return false;

		/* If it is not yet pointed at. */
		if (index != prev_pointed_index) {
			/* Set it to the pointed state. */
			pointed_index = index;
			is_pointed_by_key = false;
			is_mouse_point_detected = true;
		}
		return true;
	}

	return false;
}

/* Process dragging of volume buttons. */
static bool
process_button_drag(int index)
{
	struct gui_button *b;

	b = &button[index];

	/* If the button is not draggable. */
	if (b->type != TYPE_NOACTION &&
	    b->type != TYPE_MASTERVOL &&
	    b->type != TYPE_BGMVOL &&
	    b->type != TYPE_VOICEVOL &&
	    b->type != TYPE_SEVOL &&
	    b->type != TYPE_CHARACTERVOL &&
	    b->type != TYPE_TEXTSPEED &&
	    b->type != TYPE_AUTOSPEED &&
	    b->type != TYPE_HISTORYSCROLL &&
	    b->type != TYPE_HISTORYSCROLL_HORIZONTAL)
		return false;

	/* If the item is not dragging, set dragging state. */
	if (!b->rt.is_dragging) {
		/* If not dragging. */
		if (!s3_is_mouse_dragging())
			return false;

		/* If not pointed at. */
		if (pointed_index != index)
			return false;

		/* If already dragging. */
		if (dragging_index != -1)
			return true;

		/* Start dragging. */
		dragging_index = index;
		b->rt.is_dragging = true;
		b->rt.slider = calc_slider_value(index);
		last_drag_tick = cur_tick - DRAG_TICK_INTERVAL;

		switch (b->type) {
		case TYPE_MASTERVOL:
			s3_set_master_volume(b->rt.slider);
			break;
		case TYPE_BGMVOL:
			s3_set_mixer_global_volume(S3_TRACK_BGM, b->rt.slider);
			break;
		case TYPE_HISTORYSCROLL:
		case TYPE_HISTORYSCROLL_HORIZONTAL:
			transient_history_slider = adjust_history_slider_value(b->rt.slider);
			process_history_scroll_at(transient_history_slider, true);
			break;
		}

		/* Update other buttons in case there are multiple buttons of the same type. */
		update_runtime_props(true);
		return true;
	}

	/*
	 * If still dragging.
	 */
	if (s3_is_mouse_dragging()) {
		/* Ignore continued frame dragging.  */
		if (cur_tick - last_drag_tick >= DRAG_TICK_INTERVAL) {
			last_drag_tick = cur_tick;

			/* Reflect the slider amount in the settings. */
			b->rt.slider = calc_slider_value(index);
			switch (b->type) {
			case TYPE_MASTERVOL:
				s3_set_master_volume(b->rt.slider);
				break;
			case TYPE_BGMVOL:
				s3_set_mixer_global_volume(S3_TRACK_BGM, b->rt.slider);
				break;
			case TYPE_VOICEVOL:
				s3_set_mixer_global_volume(S3_TRACK_VOICE, b->rt.slider);
				break;
			case TYPE_SEVOL:
				s3_set_mixer_global_volume(S3_TRACK_SE, b->rt.slider);
				break;
			case TYPE_CHARACTERVOL:
				s3_set_character_volume(b->index, b->rt.slider);
				break;
			case TYPE_TEXTSPEED:
				transient_text_speed = b->rt.slider;
				break;
			case TYPE_AUTOSPEED:
				transient_auto_speed = b->rt.slider;
				break;
			case TYPE_HISTORYSCROLL:
			case TYPE_HISTORYSCROLL_HORIZONTAL:
				transient_history_slider = adjust_history_slider_value(b->rt.slider);
				process_history_scroll_at(transient_history_slider, true);
				break;
			default:
				break;
			}

			/* Update other buttons in case there are multiple buttons of the same type. */
			update_runtime_props(false);
		}
	}

	/*
	 * If dragging is finished.
	 */
	else {
		b->rt.is_dragging = false;
		dragging_index = -1;
		is_drag_finished = true;
		s3_clear_input_state();

		/* Execute actions after adjustment is completed. */
		switch (b->type) {
		case TYPE_MASTERVOL:
			break;
		case TYPE_BGMVOL:
			break;
		case TYPE_VOICEVOL:
			/* Provide feedback with the default character volume. */
			play_se(b->file);
			break;
		case TYPE_SEVOL:
			/* Provide feedback. */
			play_se(b->file);
			break;
		case TYPE_CHARACTERVOL:
			/* Provide feedback with the specified character volume. */
			play_se(b->file);
			break;
		case TYPE_TEXTSPEED:
			/* Redisplay the text. */
			s3_set_text_speed(transient_text_speed);
			reset_preview_buttons();
			break;
		case TYPE_AUTOSPEED:
			/* Redisplay the text. */
			s3_set_auto_speed(1.0f - transient_auto_speed);
			reset_preview_buttons();
			break;
		case TYPE_HISTORYSCROLL:
		case TYPE_HISTORYSCROLL_HORIZONTAL:
			process_history_scroll_at(transient_history_slider, true);
			break;
		default:
			break;
		}

		/* Update other buttons in case there are multiple buttons of the same type. */
		update_runtime_props(false);
	}

	return true;
}

/* Calculate the value of the slider. */
static float
calc_slider_value(int index)
{
	float val;

	if (button[index].type != TYPE_HISTORYSCROLL) {
		float x1, x2;

		/* Calculate the left end of the slider. */
		x1 = (float)(button[index].x + button[index].height / 2);

		/* Calculate the right end of the slider. */
		x2 = (float)(button[index].x + button[index].width -
			     button[index].height / 2);

		/* Calculate the value at the pointed coordinate. */
		val = ((float)s3_get_mouse_pos_x() - x1) / (x2 - x1);
	} else {
		float y1, y2;

		/* Calculate the top end of the slider. */
		y1 = (float)(button[index].y + button[index].width / 2);

		/* Calculate the bottom end of the slider. */
		y2 = (float)(button[index].y + button[index].height -
			     button[index].width / 2);

		/* Calculate the value at the pointed coordinate. */
		val = ((float)s3_get_mouse_pos_y() - y1) / (y2 - y1);
	}

	/* Handle values less than 0. */
	if (val < 0)
		val = 0;

	/* Handle values greater than 1. */
	if (val > 1.0f)
		val = 1.0f;

	return val;
}

/* Process button click. */
static bool
process_button_click(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	/* Run a press anime. */
	if (index == pointed_index &&
	    (s3_is_mouse_left_pressed() ||
	     s3_is_return_key_pressed())) {
		b->rt.is_pressed = true;
		if (b->anime_press != NULL)
			start_button_anime(index, b->anime_press);
	} else {
		b->rt.is_pressed = false;
	}

	/* If the button cannot be clicked. */
	if (b->type == TYPE_NOACTION ||
	    b->type == TYPE_MASTERVOL ||
	    b->type == TYPE_BGMVOL ||
	    b->type == TYPE_VOICEVOL ||
	    b->type == TYPE_SEVOL ||
	    b->type == TYPE_CHARACTERVOL ||
	    b->type == TYPE_TEXTSPEED ||
	    b->type == TYPE_AUTOSPEED ||
	    b->type == TYPE_PREVIEW)
		return false;

	/* If this button is not pointed. */
	if (index != pointed_index)
		return false;

	/* If the pointed button is not clicked. */
	if (!s3_is_mouse_left_clicked() && !s3_is_return_key_pressed())
		return false;

	/* If other buttons are being dragged. */
	if (dragging_index != -1 && dragging_index != index)
		return false;

	/* Process click according to button type. */
	switch (b->type) {
	case TYPE_FULLSCREEN:
		play_sys_se(b->clickse);
		s3_enter_full_screen_mode();
		update_runtime_props(false);
		break;
	case TYPE_WINDOW:
		play_sys_se(b->clickse);
		s3_leave_full_screen_mode();
		update_runtime_props(false);
		break;
	case TYPE_DEFAULT:
		play_sys_se(b->clickse);
		s3_set_text_speed(0.5f);
		s3_set_auto_speed(0.5f);
		update_runtime_props(true);
		reset_preview_buttons();
		break;
	case TYPE_SAVEPAGE:
		play_sys_se(b->clickse);
		change_save_page(b->index);
		update_runtime_props(false);
		update_save_buttons();
		break;
	case TYPE_SAVE:
		process_save(index);
		break;
	case TYPE_LOAD:
		process_load(index);
		result_index = index;
		break;
	case TYPE_AUTO:
		process_auto_mode();
		result_index = index;
		break;
	case TYPE_SKIP:
		process_skip_mode();
		result_index = index;
		break;
	case TYPE_HISTORY:
		process_history_voice(index);
		break;
	case TYPE_TITLE:
		play_sys_se(b->clickse);
		result_index = index;
		break;
	case TYPE_CHAR:
		play_sys_se(b->clickse);
		process_char(index);
		need_update_var_buttons = true;
		break;
	case TYPE_LANGUAGE:
		process_language(index);
		reset_preview_buttons();
		update_runtime_props(false);
		break;
	default:
		result_index = index;
		break;
	}

	s3_clear_input_state();

	return true;
}

/*
 * Rendering
 */

/* Process rendering. */
static void
process_render(void)
{
	int i;

	/* Render each button according to its state. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (!button[i].is_initialized)
			continue;

		process_button_render(i);
	}
}

/* Draw a button. */
static void
process_button_render(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	switch (b->type) {
	case TYPE_MASTERVOL:
	case TYPE_BGMVOL:
	case TYPE_VOICEVOL:
	case TYPE_SEVOL:
	case TYPE_CHARACTERVOL:
	case TYPE_TEXTSPEED:
	case TYPE_AUTOSPEED:
		/* Render a slider. */
		process_button_render_slider(index);
		break;
	case TYPE_SAVE:
	case TYPE_LOAD:
		/* Render a save/load button. */
		process_button_render_save(index);
		break;
	case TYPE_HISTORY:
		/* Render a history button. */
		process_button_render_history(index);
		break;
	case TYPE_HISTORYSCROLL:
		/* Render a vertical slider. */
		process_button_render_slider_vertical(index);
		break;
	case TYPE_HISTORYSCROLL_HORIZONTAL:
		/* Render a horizontal slider. */
		process_button_render_slider(index);
		break;
	case TYPE_PREVIEW:
		/* Render a preview button. */
		process_button_render_preview(index);
		break;
	case TYPE_VAR:
		/* Render a variable button. */
		process_button_render_var(index);
		break;
	case TYPE_SAVEPAGE:
		/* Render a page button. */
		process_button_render_page(index);
		break;
	case TYPE_LANGUAGE:
		/* Render a language button. */
		process_button_render_language(index);
		break;
	default:
		/* Render a generic button. */
		process_button_render_generic(index);
		break;
	}
}

/* Render a slider button. */
static void
process_button_render_slider(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	/* If the button is not pointed, draw the bar part with the idle image. */
	if (index != pointed_index || is_fading_in || is_fading_out) {
		if (b->rt.img_idle != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					b->rt.img_idle,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* If the button is pointed, draw the bar part with the hover image. */
	if (index == pointed_index && !b->rt.is_pressed && !is_fading_in && !is_fading_out) {
		struct s3_image *img;
		img = b->rt.img_hover != NULL ? b->rt.img_hover : b->rt.img_idle;
		if (img != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					img,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* If the button is pressed, draw the bar part with the press image. */
	if (index == pointed_index && b->rt.is_pressed && !is_fading_in && !is_fading_out) {
		struct s3_image *img;
		img = b->rt.img_press != NULL ? b->rt.img_press : b->rt.img_idle;
		if (img != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					img,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* Draw the knob with the "disable" image. */
	if (b->rt.img_disable != NULL) {
		if (b->rt.img_disable != NULL) {
			int x, y, knob_w, knob_h;

			knob_w = s3_get_image_width(b->rt.img_disable);
			knob_h = s3_get_image_height(b->rt.img_disable);

			/* Calculate the drawing position. */
			x = b->x + (int)((float)(b->width - knob_w) * b->rt.slider);
			y = b->y + b->height / 2 - knob_h / 2;

			s3_render_image(x,
					y,
					knob_w,
					knob_h,
					b->rt.img_disable,
					0,
					0,
					knob_w,
					knob_h,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}
}

/* Render a vertical slider button. */
static void
process_button_render_slider_vertical(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	/* If the button is not pointed, draw the bar part with the idle image. */
	if (index != pointed_index || is_fading_in || is_fading_out) {
		if (b->rt.img_idle != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					b->rt.img_idle,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* If the button is pointed, draw the bar part with the hover image. */
	if (index == pointed_index && !b->rt.is_pressed && !is_fading_in && !is_fading_out) {
		struct s3_image *img;
		img = b->rt.img_hover != NULL ? b->rt.img_hover : b->rt.img_idle;
		if (img != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					img,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* If the button is pressed, draw the bar part with the hover image. */
	if (index == pointed_index && b->rt.is_pressed && !is_fading_in && !is_fading_out) {
		struct s3_image *img;
		img = b->rt.img_press != NULL ? b->rt.img_press : b->rt.img_idle;
		if (img != NULL) {
			s3_render_image(b->x,
					b->y,
					b->width,
					b->height,
					img,
					0,
					0,
					b->width,
					b->height,
					cur_alpha,
					S3_BLEND_ALPHA);
		}
	}

	/* Draw the knob with the "disable" image. */
	if (b->rt.img_disable != NULL) {
		int x, y, knob_w, knob_h;

		knob_w = s3_get_image_width(b->rt.img_disable);
		knob_h = s3_get_image_height(b->rt.img_disable);

		/* Calculate the drawing position. */
		y = b->y + (int)((float)(b->height - knob_h) * b->rt.slider);
		x = b->x + b->width / 2 - knob_w / 2;

		s3_render_image(x,
				y,
				knob_w,
				knob_h,
				b->rt.img_disable,
				0,
				0,
				knob_w,
				knob_h,
				cur_alpha,
				S3_BLEND_ALPHA);
	}
}

/* Render a preview button. */
static void
process_button_render_preview(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	render_image_helper(b->rt.img_canvas_idle, b->bid);
}

/* Render a generic button. */
static void
process_button_render_generic(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	if (b->bid == -1)
		return;

	if (index != pointed_index) {
		if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	} else if (b->rt.is_pressed) {
		if (b->rt.img_press != NULL)
			render_image_helper(b->rt.img_press, b->bid);
		else if (b->rt.img_hover != NULL)
			render_image_helper(b->rt.img_hover, b->bid);
		else if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	} else {
		if (b->rt.img_hover != NULL)
			render_image_helper(b->rt.img_hover, b->bid);
		else if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	}
}

/* Render a generic button. */
static void
process_button_render_language(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	if (b->bid == -1)
		return;

	if (!b->rt.is_disabled) {
		if (index != pointed_index) {
			if (b->rt.img_idle != NULL)
				render_image_helper(b->rt.img_idle, b->bid);
		} else {
			if (b->rt.is_pressed) {
				if (b->rt.img_press != NULL)
					render_image_helper(b->rt.img_press, b->bid);
				else if (b->rt.img_hover != NULL)
					render_image_helper(b->rt.img_hover, b->bid);
				else if (b->rt.img_idle != NULL)
					render_image_helper(b->rt.img_idle, b->bid);
			} else {
				if (b->rt.img_hover != NULL)
					render_image_helper(b->rt.img_hover, b->bid);
				else if (b->rt.img_idle != NULL)
					render_image_helper(b->rt.img_idle, b->bid);
			}
		}
	} else {
		if (b->rt.img_active != NULL)
			render_image_helper(b->rt.img_active, b->bid);
		else if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	}
}

/* Render a page button. */
static void
process_button_render_page(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	if (b->bid == -1)
		return;

	if (index == pointed_index) {
		if (b->rt.is_pressed) {
			if (b->rt.img_press != NULL)
				render_image_helper(b->rt.img_press, b->bid);
			else if (b->rt.img_hover != NULL)
				render_image_helper(b->rt.img_hover, b->bid);
			else if (b->rt.img_idle != NULL)
				render_image_helper(b->rt.img_idle, b->bid);
		} else {
			if (b->rt.img_hover != NULL)
				render_image_helper(b->rt.img_hover, b->bid);
			else if (b->rt.img_idle != NULL)
				render_image_helper(b->rt.img_idle, b->bid);
		}
	} else if (!b->rt.is_disabled) {
		if (b->rt.img_active != NULL)
			render_image_helper(b->rt.img_active, b->bid);
		else if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	} else {
		if (b->rt.img_idle != NULL)
			render_image_helper(b->rt.img_idle, b->bid);
	}
}

/* Render a variable button. */
static void
process_button_render_var(
	int index)
{
	struct gui_button *b;

	b = &button[index];
	assert(b->type == TYPE_VAR);

	render_image_helper(b->rt.img_canvas_idle, b->bid);
}

/*
 * Save/Load Button
 */

/* Initialize buttons of type TYPE_SAVE/TYPE_LOAD. */
static bool
init_save_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_SAVE && button[i].type != TYPE_LOAD)
			continue;

		if (button[i].rt.img_idle != NULL) {
			button[i].rt.img_canvas_idle = s3_create_image(
				s3_get_image_width(button[i].rt.img_idle),
				s3_get_image_height(button[i].rt.img_idle));
			if (button[i].rt.img_canvas_idle == NULL)
				return false;
		}

		if (button[i].rt.img_hover != NULL) {
			button[i].rt.img_canvas_hover = s3_create_image(
				s3_get_image_width(button[i].rt.img_hover),
				s3_get_image_height(button[i].rt.img_hover));
			if (button[i].rt.img_canvas_hover == NULL)
				return false;
		}

		if (button[i].rt.img_press != NULL) {
			button[i].rt.img_canvas_press = s3_create_image(
				s3_get_image_width(button[i].rt.img_press),
				s3_get_image_height(button[i].rt.img_press));
			if (button[i].rt.img_canvas_press == NULL)
				return false;
		}
	}

	return true;
}

/* Render save and load slots. */
static void
update_save_buttons(void)
{
	int i, base, save_index;

	base = save_slots * save_page;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_SAVE && button[i].type != TYPE_LOAD)
			continue;

		save_index = base + button[i].index;

		if (button[i].type == TYPE_LOAD) {
			if (s3_get_save_timestamp(save_index) != 0)
				button[i].rt.is_disabled = false;
			else
				button[i].rt.is_disabled = true;
		}

		draw_save_button(i);
	}
}

/* Render a save/load button. */
static void
draw_save_button(
	int button_index)
{
	char text[128];
	struct gui_button *b;
	struct s3_image *thumb;
	struct tm *timeptr;
	const char *chapter, *msg;
	time_t save_time;
	int save_index;

	b = &button[button_index];

	/* Calculate the save data number. */
	save_index = save_slots * save_page + b->index;

	/* Get the save time. */
	save_time = (time_t)s3_get_save_timestamp(save_index);

	/* Clear the canvas images. */
	if (b->rt.img_canvas_idle != NULL) {
		s3_draw_image(b->rt.img_canvas_idle,
			      0,
			      0,
			      b->rt.img_idle,
			      0,
			      0,
			      s3_get_image_width(b->rt.img_idle),
			      s3_get_image_height(b->rt.img_idle),
			      255,
			      S3_BLEND_COPY);
	}
	if (b->rt.img_canvas_hover != NULL) {
		s3_draw_image(b->rt.img_canvas_hover,
			      0,
			      0,
			      b->rt.img_hover,
			      0,
			      0,
			      s3_get_image_width(b->rt.img_hover),
			      s3_get_image_height(b->rt.img_hover),
			      255,
			      S3_BLEND_COPY);
	}
	if (b->rt.img_canvas_press != NULL) {
		s3_draw_image(b->rt.img_canvas_press,
			      0,
			      0,
			      b->rt.img_press,
			      0,
			      0,
			      s3_get_image_width(b->rt.img_press),
			      s3_get_image_height(b->rt.img_press),
			      255,
			      S3_BLEND_COPY);
	}

	thumb = s3_get_save_thumbnail(save_index);
	if (b->thumb_width == 0)
		b->thumb_width = s3_get_image_width(thumb);
	if (b->thumb_height == 0)
		b->thumb_height = s3_get_image_height(thumb);

	/* Draw the thumbnail. */
	if (thumb != NULL && save_time != 0) {
		if (b->rt.img_canvas_idle != NULL) {
			s3_draw_image_3d(
				b->rt.img_canvas_idle,
				b->thumb_x,
				b->thumb_y,
				b->thumb_x + b->thumb_width,
				b->thumb_y,
				b->thumb_x,
				b->thumb_y + b->thumb_height,
				b->thumb_x + b->thumb_width,
				b->thumb_y + b->thumb_height,
				thumb,
				0,
				0,
				thumb->width,
				thumb->height,
				255,
				S3_BLEND_ALPHA);
		}
		if (b->rt.img_canvas_hover != NULL) {
			s3_draw_image_3d(
				b->rt.img_canvas_hover,
				b->thumb_x,
				b->thumb_y,
				b->thumb_x + b->thumb_width,
				b->thumb_y,
				b->thumb_x,
				b->thumb_y + b->thumb_height,
				b->thumb_x + b->thumb_width,
				b->thumb_y + b->thumb_height,
				thumb,
				0,
				0,
				thumb->width,
				thumb->height,
				255,
				S3_BLEND_ALPHA);
		}
		if (b->rt.img_canvas_press != NULL) {
			s3_draw_image_3d(
				b->rt.img_canvas_press,
				b->thumb_x,
				b->thumb_y,
				b->thumb_x + b->thumb_width,
				b->thumb_y,
				b->thumb_x,
				b->thumb_y + b->thumb_height,
				b->thumb_x + b->thumb_width,
				b->thumb_y + b->thumb_height,
				thumb,
				0,
				0,
				thumb->width,
				thumb->height,
				255,
				S3_BLEND_ALPHA);
		}
	}

	/* Draw the index. */
	snprintf(text, sizeof(text), "%02d", save_index + 1);
	if (b->rt.img_canvas_idle != NULL) {
		draw_save_text_item(b->rt.img_canvas_idle,
				    button_index,
				    b->index_x,
				    b->index_y,
				    text,
				    SAVE_TEXT_INDEX);
	}
	if (b->rt.img_canvas_hover != NULL) {
		draw_save_text_item(b->rt.img_canvas_hover,
				    button_index,
				    b->index_x,
				    b->index_y,
				    text,
				    SAVE_TEXT_INDEX);
	}
	if (b->rt.img_canvas_press != NULL) {
		draw_save_text_item(b->rt.img_canvas_press,
				    button_index,
				    b->index_x,
				    b->index_y,
				    text,
				    SAVE_TEXT_INDEX);
	}

	/* Draw the date. */
	if (save_time == 0) {
		snprintf(text, sizeof(text), "---");
	} else {
		timeptr = localtime(&save_time);
		strftime(text, sizeof(text), "%y/%m/%d %H:%M", timeptr);
	}
	if (b->rt.img_canvas_idle != NULL) {
		draw_save_text_item(b->rt.img_canvas_idle,
				    button_index,
				    b->date_x,
				    b->date_y,
				    text,
				    SAVE_TEXT_DATE);
	}
	if (b->rt.img_canvas_hover != NULL) {
		draw_save_text_item(b->rt.img_canvas_hover,
				    button_index,
				    b->date_x,
				    b->date_y,
				    text,
				    SAVE_TEXT_DATE);
	}
	if (b->rt.img_canvas_press != NULL) {
		draw_save_text_item(b->rt.img_canvas_press,
				    button_index,
				    b->date_x,
				    b->date_y,
				    text,
				    SAVE_TEXT_DATE);
	}
	
	/* Draw the chapter title. */
	chapter = s3_get_save_chapter_name(save_index);
	if (chapter != NULL) {
		if (b->rt.img_canvas_idle != NULL) {
			draw_save_text_item(b->rt.img_canvas_idle,
					    button_index,
					    b->chapter_x,
					    b->chapter_y,
					    chapter,
					    SAVE_TEXT_CHAPTER);
		}
		if (b->rt.img_canvas_hover != NULL) {
			draw_save_text_item(b->rt.img_canvas_hover,
					    button_index,
					    b->chapter_x,
					    b->chapter_y,
					    chapter,
					    SAVE_TEXT_CHAPTER);
		}
		if (b->rt.img_canvas_press != NULL) {
			draw_save_text_item(b->rt.img_canvas_press,
					    button_index,
					    b->chapter_x,
					    b->chapter_y,
					    chapter,
					    SAVE_TEXT_CHAPTER);
		}
	}

	/* Draw the last message. */
	msg = s3_get_save_last_message(save_index);
	if (msg != NULL) {
		if (b->rt.img_canvas_idle != NULL) {
			draw_save_text_item(b->rt.img_canvas_idle,
					    button_index,
					    b->msg_x,
					    b->msg_y,
					    msg,
					    SAVE_TEXT_MSG);
		}
		if (b->rt.img_canvas_hover != NULL) {
			draw_save_text_item(b->rt.img_canvas_hover,
					    button_index,
					    b->msg_x,
					    b->msg_y,
					    msg,
					    SAVE_TEXT_MSG);
		}
		if (b->rt.img_canvas_press != NULL) {
			draw_save_text_item(b->rt.img_canvas_press,
					    button_index,
					    b->msg_x,
					    b->msg_y,
					    msg,
					    SAVE_TEXT_MSG);
		}
	}

	if (b->rt.img_canvas_idle != NULL)
		s3_notify_image_update(b->rt.img_canvas_idle);
	if (b->rt.img_canvas_hover != NULL)
		s3_notify_image_update(b->rt.img_canvas_hover);
	if (b->rt.img_canvas_press != NULL)
		s3_notify_image_update(b->rt.img_canvas_press);
}

/* Render save data text. */
static int
draw_save_text_item(
	struct s3_image *target,
	int button_index,
	int x,
	int y,
	const char *text,
	int save_text_type)
{
	struct s3_drawmsg *context;
	struct gui_button *b;
	s3_pixel_t color, outline_color;
	int width, total_chars;
	int cfg_sel, cfg_size, cfg_r, cfg_g, cfg_b, cfg_o_w, cfg_o_r, cfg_o_g, cfg_o_b, cfg_ruby;
	int cfg_margin_line, cfg_margin_char;
	bool cfg_tategaki;
	bool multiline;

	b = &button[button_index];

	switch (save_text_type) {
	case SAVE_TEXT_INDEX:
		cfg_sel = conf_gui_save_index_font_select;
		cfg_size = conf_gui_save_index_font_size;
		cfg_r = conf_gui_save_index_font_r;
		cfg_g = conf_gui_save_index_font_g;
		cfg_b = conf_gui_save_index_font_b;
		cfg_o_w = conf_gui_save_index_font_outline_width;
		cfg_o_r = conf_gui_save_index_font_outline_r;
		cfg_o_g = conf_gui_save_index_font_outline_g;
		cfg_o_b = conf_gui_save_index_font_outline_b;
		cfg_ruby = conf_gui_save_index_font_ruby;
		cfg_tategaki = conf_gui_save_index_font_tategaki;
		cfg_margin_char = conf_gui_save_index_margin_char;
		cfg_margin_line = 0;
		multiline = false;
		break;
	case SAVE_TEXT_DATE:
		cfg_sel = conf_gui_save_date_font_select;
		cfg_size = conf_gui_save_date_font_size;
		cfg_r = conf_gui_save_date_font_r;
		cfg_g = conf_gui_save_date_font_g;
		cfg_b = conf_gui_save_date_font_b;
		cfg_o_w = conf_gui_save_date_font_outline_width;
		cfg_o_r = conf_gui_save_date_font_outline_r;
		cfg_o_g = conf_gui_save_date_font_outline_g;
		cfg_o_b = conf_gui_save_date_font_outline_b;
		cfg_ruby = conf_gui_save_date_font_ruby;
		cfg_tategaki = conf_gui_save_date_font_tategaki;
		cfg_margin_char = conf_gui_save_date_margin_char;
		cfg_margin_line = 0;
		multiline = false;
		break;
	case SAVE_TEXT_CHAPTER:
		cfg_sel = conf_gui_save_chapter_font_select;
		cfg_size = conf_gui_save_chapter_font_size;
		cfg_r = conf_gui_save_chapter_font_r;
		cfg_g = conf_gui_save_chapter_font_g;
		cfg_b = conf_gui_save_chapter_font_b;
		cfg_o_w = conf_gui_save_chapter_font_outline_width;
		cfg_o_r = conf_gui_save_chapter_font_outline_r;
		cfg_o_g = conf_gui_save_chapter_font_outline_g;
		cfg_o_b = conf_gui_save_chapter_font_outline_b;
		cfg_ruby = conf_gui_save_chapter_font_ruby;
		cfg_tategaki = conf_gui_save_chapter_font_tategaki;
		cfg_margin_char = conf_gui_save_chapter_margin_char;
		cfg_margin_line = 0;
		multiline = false;
		break;
	case SAVE_TEXT_MSG:
		cfg_sel = conf_gui_save_msg_font_select;
		cfg_size = conf_gui_save_msg_font_size;
		cfg_r = conf_gui_save_msg_font_r;
		cfg_g = conf_gui_save_msg_font_g;
		cfg_b = conf_gui_save_msg_font_b;
		cfg_o_w = conf_gui_save_msg_font_outline_width;
		cfg_o_r = conf_gui_save_msg_font_outline_r;
		cfg_o_g = conf_gui_save_msg_font_outline_g;
		cfg_o_b = conf_gui_save_msg_font_outline_b;
		cfg_ruby = conf_gui_save_msg_font_ruby;
		cfg_tategaki = conf_gui_save_msg_font_tategaki;
		cfg_margin_char = conf_gui_save_msg_margin_char;
		cfg_margin_line = conf_gui_save_msg_margin_line;
		multiline = conf_gui_save_msg_multiline;
		break;
	default:
		cfg_sel = conf_gui_save_msg_font_select;
		cfg_size = conf_gui_save_msg_font_size;
		cfg_r = conf_gui_save_msg_font_r;
		cfg_g = conf_gui_save_msg_font_g;
		cfg_b = conf_gui_save_msg_font_b;
		cfg_o_w = conf_gui_save_msg_font_outline_width;
		cfg_o_r = conf_gui_save_msg_font_outline_r;
		cfg_o_g = conf_gui_save_msg_font_outline_g;
		cfg_o_b = conf_gui_save_msg_font_outline_b;
		cfg_ruby = conf_gui_save_msg_font_ruby;
		cfg_tategaki = conf_gui_save_msg_font_tategaki;
		cfg_margin_line = conf_gui_save_msg_margin_line;
		cfg_margin_char = conf_gui_save_msg_margin_char;
		multiline = conf_gui_save_msg_multiline;
		break;
	}

	/* Determine the color. */
	color = s3_make_pixel(0xff,
			      (uint32_t)cfg_r,
			      (uint32_t)cfg_g,
			      (uint32_t)cfg_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)cfg_o_r,
				      (uint32_t)cfg_o_g,
				      (uint32_t)cfg_o_b);

	/* Draw the text. */
	context = s3_create_drawmsg(
		target,
		text,
		cfg_sel,
		cfg_size,
		cfg_size,
		cfg_ruby,
		cfg_o_w,
		x,
		y,
		b->width,
		b->height,
		x,			/* left_margin */
		b->margin_right,	/* right_margin */
		0,			/* top_margin */
		b->margin_bottom,	/* bottom_margin */
		multiline ? cfg_margin_line : 0,
		cfg_margin_char,
		color,
		outline_color,
		0,		/* bg_color */
		false,		/* fill_bg */
		false,		/* is_dimming */
		!multiline,	/* ignore_linefeed */
		true,		/* ignore_font */
		true,		/* ignore_outline */
		true,		/* ignore_color */
		true,		/* ignore_size */
		true,		/* ignore_position */
		false,		/* ignore_ruby */
		true,		/* ignore_wait */
		NULL,		/* inline_wait_hook */
		cfg_tategaki);
	if (context == NULL)
		return 0;
	total_chars = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, total_chars);
	width = s3_get_string_width(context->font, context->font_size, text);
	s3_destroy_drawmsg(context);
	
	return width;
}

/* Perform a save operation. */
static void
process_save(
	int button_index)
{
	int data_index;

	/* Convert the button index to a save data index. */
	data_index = save_page * save_slots + button[button_index].index;

	/* Play the SE. */
	play_sys_se(button[button_index].clickse);

	/* If "@gui save.txt", advance to the next command. */
	if (!is_sys_gui && !did_save)
		did_save = true;

	/* Make a thumnail. */
	s3_draw_stage_to_thumb();

	/* Execute a save. */
	s3_execute_save_local(data_index);

	/* Save the last saved page. */
	conf_gui_save_last_page = save_page;

	/*
	 * To prevent the save button's SE from being drowned out by the mouse
	 * moving and changing the pointed item when clicking the overwrite
	 * dialog button immediately after saving.
	 */
	is_saved_in_this_frame = true;

	/* Update the button image. */
	draw_save_button(button_index);
}

/* Perform a load operation. */
static void
process_load(
	int button_index)
{
	int data_index;

	/* Convert a button index to a save data index. */
	data_index = save_page * save_slots + button[button_index].index;

	/* If there is no save data. */
	if (s3_get_save_timestamp(data_index) == 0)
		return;

	/* Execute a load. */
	if (!s3_execute_load_local(data_index))
		is_load_failed = true;

	/* Save the last loaded page. */
	conf_gui_save_last_page = save_page;

	did_load = true;
	if (s3_is_in_command_repetition())
		s3_stop_command_repetition();
	s3_stop_gui();
}

/* Render a save/load button. */
static void
process_button_render_save(
	int index)
{
	struct gui_button *b;
	int save_index;

	b = &button[index];

	if (index != pointed_index) {
		if (b->rt.img_canvas_idle != NULL)
			render_image_helper(b->rt.img_canvas_idle, b->bid);
	} else {
		if (b->rt.is_pressed) {
			if (b->rt.img_canvas_press != NULL)
				render_image_helper(b->rt.img_canvas_press, b->bid);
			else if (b->rt.img_canvas_hover != NULL)
				render_image_helper(b->rt.img_canvas_hover, b->bid);
			else if (b->rt.img_canvas_idle != NULL)
				render_image_helper(b->rt.img_canvas_idle, b->bid);
		} else {
			if (b->rt.img_canvas_hover != NULL)
				render_image_helper(b->rt.img_canvas_hover, b->bid);
			else if (b->rt.img_canvas_idle != NULL)
				render_image_helper(b->rt.img_canvas_idle, b->bid);
		}
	}

	/* Render the NEW image. */
	save_index = save_page * save_slots + b->index;
	if (b->rt.is_new_enabled &&
	    save_index == s3_get_latest_save_index()) {
		struct s3_image *img = s3i_get_savenew_image();
		if (img != NULL)
			render_image_helper(img, b->bid);
	}
}

static void
process_auto_mode(void)
{
	if (s3_is_skip_mode()) {
		s3_stop_skip_mode();
		s3_show_skipmode_banner(false);
	}

	s3_start_auto_mode();
	s3_show_automode_banner(true);
}

static void
process_skip_mode(void)
{
	if (s3_is_auto_mode()) {
		s3_stop_auto_mode();
		s3_show_automode_banner(false);
	}

	s3_start_skip_mode();
	s3_show_skipmode_banner(true);
}

/*
 * History Buttons
 */

/* Initialize TYPE_HISTORY buttons. */
static bool
init_history_buttons(void)
{
	int i, history_count;

	/* Get the number of history items. */
	history_count = s3_get_history_count();
	if (history_count == 0)
		return true;

	/* Calculate the top of the history. */
	if (history_count <= history_slots) {
		history_top = history_count - 1;
		transient_history_slider = 0;
	} else {
		history_top = history_slots - 1;
		transient_history_slider = 1.0f;
	}

	/* Create images for each button. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_HISTORY)
			continue;
		if (button[i].rt.img_idle != NULL) {
			button[i].rt.img_canvas_idle = s3_create_image(
				s3_get_image_width(button[i].rt.img_idle),
				s3_get_image_height(button[i].rt.img_idle));
			if (button[i].rt.img_canvas_idle == NULL)
				return false;
		}
		if (button[i].rt.img_hover != NULL) {
			button[i].rt.img_canvas_hover = s3_create_image(
				s3_get_image_width(button[i].rt.img_hover),
				s3_get_image_height(button[i].rt.img_hover));
			if (button[i].rt.img_canvas_hover == NULL)
				return false;
		}
		if (button[i].rt.img_press != NULL) {
			button[i].rt.img_canvas_press = s3_create_image(
				s3_get_image_width(button[i].rt.img_press),
				s3_get_image_height(button[i].rt.img_press));
			if (button[i].rt.img_canvas_press == NULL)
				return false;
		}
	}

	return true;
}

/* Render a history button. */
static void
process_button_render_history(
	int index)
{
	struct gui_button *b;

	b = &button[index];

	/* If there are no history items. (For when history is empty or few). */
	if (b->rt.img_canvas_idle == NULL)
		return;

	if (!b->rt.is_disabled && index == pointed_index) {
		if (b->rt.is_pressed) {
			if (b->rt.img_canvas_press != NULL)
				render_image_helper(b->rt.img_canvas_press, b->bid);
			else if (b->rt.img_canvas_hover != NULL)
				render_image_helper(b->rt.img_canvas_hover, b->bid);
			else if (b->rt.img_canvas_idle != NULL)
				render_image_helper(b->rt.img_canvas_idle, b->bid);
		} else {
			if (b->rt.img_canvas_hover != NULL)
				render_image_helper(b->rt.img_canvas_hover, b->bid);
			else if (b->rt.img_canvas_idle != NULL)
				render_image_helper(b->rt.img_canvas_idle, b->bid);
		}
	} else {
		if (b->rt.img_canvas_idle != NULL)
			render_image_helper(b->rt.img_canvas_idle, b->bid);
	}
}

/* Render all history slots. */
static void
draw_history_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_HISTORY)
			continue;
		if (button[i].rt.img_canvas_idle != NULL) {
			draw_history_button(button[i].rt.img_canvas_idle,
					    button[i].rt.img_idle,
					    i);
		}
		if (button[i].rt.img_canvas_hover != NULL) {
			draw_history_button(button[i].rt.img_canvas_hover,
					    button[i].rt.img_hover,
					    i);
		}
		if (button[i].rt.img_canvas_press != NULL) {
			draw_history_button(button[i].rt.img_canvas_press,
					    button[i].rt.img_hover,
					    i);
		}
	}
}

/* Render a history slot. */
static void
draw_history_button(
	struct s3_image *target,
	struct s3_image *source,
	int index)
{
	struct gui_button *b;
	int history_count;

	b = &button[index];

#if defined(PF_TARGET_WASM)
	/* To fill the sound buffer in case font rendering takes time */
	void fill_sound_buffer(void);
	fill_sound_buffer();
#endif

	/* Calculate the history offset */
	history_count = s3_get_history_count();
	if (history_count < history_slots)
		b->rt.history_offset = history_count - b->index - 1;
	else
		b->rt.history_offset = history_top - b->index;
	if (b->rt.history_offset < 0)
		b->rt.history_offset = -1;

	/* Determine the active state of the slot */
	if (b->rt.history_offset >= 0 &&
	    b->rt.history_offset < s3_get_history_count()) {
		if (s3_get_history_voice(b->rt.history_offset) != NULL)
			b->rt.is_disabled = false;
		else if (conf_tts_enable)
			b->rt.is_disabled = false;
		else
			b->rt.is_disabled = true;
	} else {
		b->rt.is_disabled = true;
	}

	/* Clear the image. */
	s3_draw_image(target,
		      0,
		      0,
		      source,
		      0,
		      0,
		      s3_get_image_width(source),
		      s3_get_image_height(source),
		      255,
		      S3_BLEND_COPY);

	/* Draw the message. */
	if (b->rt.history_offset != -1) {
		const char *name = s3_get_history_name(b->rt.history_offset);
		const char *text = s3_get_history_text(b->rt.history_offset);
		if (name != NULL && !conf_game_novel)
			draw_history_name_item(target, index, name);
		if (text != NULL)
			draw_history_text_item(target, index, text, name != NULL);
	}

	s3_notify_image_update(target);
}

/* Draw the history text. */
static void
draw_history_name_item(
	struct s3_image *target,
	int index,
	const char *name)
{
	struct s3_drawmsg *context;
	struct gui_button *b;
	s3_pixel_t color, outline_color;
	int total_chars;
	int pen_x, pen_y;

	b = &button[index];

	/* Calculate the pen position. */
	if (!conf_gui_history_name_font_tategaki) {
		pen_x = b->name_x;
		pen_y = b->name_y;
	} else {
		pen_x = b->name_x - conf_gui_history_name_font_size;
		pen_y = b->name_y;
	}

	/* Determine the color */
	color = s3_make_pixel(0xff,
			      (uint32_t)conf_gui_history_name_font_r,
			      (uint32_t)conf_gui_history_name_font_g,
			      (uint32_t)conf_gui_history_name_font_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)conf_gui_history_name_font_outline_r,
				      (uint32_t)conf_gui_history_name_font_outline_g,
				      (uint32_t)conf_gui_history_name_font_outline_b);

	/* Render. */
	context = s3_create_drawmsg(
		target,
		name,
		conf_gui_history_name_font_select,
		conf_gui_history_name_font_size,
		conf_gui_history_name_font_size,
		conf_gui_history_name_font_ruby,
		conf_gui_history_name_font_outline_width,
		pen_x,
		pen_y,
		b->width,	/* area_width */
		b->height,	/* area_height */
		0,			/* left_margin */
		b->margin_right,	/* right_margin */
		0,			/* top_margin */
		b->margin_bottom,	/* bottom_margin */
		0,			/* line margin */
		conf_gui_history_name_margin_char,
		color,
		outline_color,
		0,		/* bg_color */
		false,		/* fill_bg */
		false,		/* is_dimming */
		false,		/* ignore_linefeed */
		false,		/* ignore_font */
		false,		/* ignore_outline */
		false,		/* ignore_color */
		true,		/* ignore_size */
		true,		/* ignore_position */
		false,		/* ignore_ruby */
		true,		/* ignore_wait */
		NULL,		/* inline_wait_hook */
		conf_gui_history_name_font_tategaki);
	if (context == NULL)
		return;
	total_chars = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, total_chars);
	s3_destroy_drawmsg(context);
}

/* Draw the history text. */
static void
draw_history_text_item(
	struct s3_image *target,
	int index,
	const char *text,
	bool has_name)
{
	struct s3_drawmsg *context;
	struct gui_button *b;
	s3_pixel_t color, outline_color;
	int total_chars;
	int pen_x, pen_y;
	int margin_left, margin_top;

	b = &button[index];

	/* Calculate the pen position. */
	if (!conf_gui_history_name_font_tategaki) {
		if (has_name) {
			pen_x = b->text_x;
			pen_y = b->text_y;
		} else {
			pen_x = b->msg_x;
			pen_y = b->msg_y;
		}
		margin_left = pen_x;
		margin_top = 0;
	} else {
		if (has_name) {
			pen_x = b->text_x - conf_gui_history_text_font_size;
			pen_y = b->text_y;
		} else {
			pen_x = b->msg_x - conf_gui_history_text_font_size;
			pen_y = b->msg_y;
		}
		margin_top = pen_y;
		margin_left = 0;
	}

	/* Determine the color */
	color = s3_make_pixel(0xff,
			      (uint32_t)conf_gui_history_text_font_r,
			      (uint32_t)conf_gui_history_text_font_g,
			      (uint32_t)conf_gui_history_text_font_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)conf_gui_history_text_font_outline_r,
				      (uint32_t)conf_gui_history_text_font_outline_g,
				      (uint32_t)conf_gui_history_text_font_outline_b);

	/* Render. */
	context = s3_create_drawmsg(
		target,
		text,
		conf_gui_history_text_font_select,
		conf_gui_history_text_font_size,
		conf_gui_history_text_font_size,
		conf_gui_history_text_font_ruby,
		conf_gui_history_text_font_outline_width,
		pen_x,
		pen_y,
		b->width,	/* area_width */
		b->height,	/* area_height */
		margin_left,		/* left_margin */
		b->margin_right,	/* right_margin */
		margin_top,		/* top_margin */
		b->margin_bottom,	/* bottom_margin */
		conf_gui_history_text_margin_line,
		conf_gui_history_text_margin_char,
		color,
		outline_color,
		0,		/* bg_color */
		false,		/* fill_bg */
		false,		/* is_dimming */
		false,		/* ignore_linefeed */
		false,		/* ignore_font */
		false,		/* ignore_outline */
		false,		/* ignore_color */
		true,		/* ignore_size */
		true,		/* ignore_position */
		false,		/* ignore_ruby */
		true,		/* ignore_wait */
		NULL,		/* inline_wait_hook */
		conf_gui_history_text_font_tategaki);
	if (context == NULL)
		return;
	total_chars = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, total_chars);
	s3_destroy_drawmsg(context);
}

/* Process scroll up. */
static void
process_history_scroll_up(void)
{
	int history_count, old_history_top;
	float pos;

	old_history_top = history_top;

	/* Calculate the top of the history. */
	history_count = s3_get_history_count();
	history_top++;
	if (history_top >= history_count)
		history_top = history_count - 1;

	/* Calculate the scroll position. */
	pos = 1.0f - (float)(history_top - (history_slots - 1)) / (float)((history_count - 1) - (history_slots - 1));
	if (pos < 0)
		pos = 0;
	if (pos > 1.0f)
		pos = 1.0f;

	/* Adjust the slider position. */
	transient_history_slider = pos;

	/* If a redraw is necessary. */
	if (history_top != old_history_top)
		need_update_history_buttons = true;
}

/* Process scroll down. */
static void process_history_scroll_down(void)
{
	int history_count, old_history_top;
	float pos;

	old_history_top = history_top;

	/* Calculate the top of the history. */
	history_count = s3_get_history_count();
	history_top--;
	if (history_top < 0)
		history_top = 0;

	/* Calculate the scroll position. */
	pos = 1.0f - (float)(history_top - (history_slots - 1)) / (float)((history_count - 1) - (history_slots - 1));
	if (pos < 0)
		pos = 0;
	if (pos > 1.0f)
		pos = 1.0f;

	/* Adjust the slider position. */
	transient_history_slider = adjust_history_slider_value(pos);

	/* If a redraw is necessary. */
	if (history_top != old_history_top)
		need_update_history_buttons = true;
}

/* Process scroll at a specific position on the history bar. */
static void process_history_scroll_at(float pos, bool force)
{
	float new_pos;

	/* Adjust the slider position. */
	new_pos = adjust_history_slider_value(pos);
	if (force || new_pos != transient_history_slider) {
		/* Update. */
		transient_history_slider = new_pos;

		/* Redraw. */
		need_update_history_buttons = true;
	}
}

/* Adjust the history scroll bar position. */
static float
adjust_history_slider_value(
	float pos)
{
	int history_count;

	/* If the number of history items is less than the number of slots. */
	history_count = s3_get_history_count();
	if (history_count <= history_slots) {
		pos = 0.0f;
	} else {
		/* Calculate the scroll position. */
		if (history_count <= history_slots)
			history_top = history_count - 1;
		else
			history_top = (int)(((float)((history_count - 1) - (history_slots - 1)) * (1.0f - pos)) + (float)(history_slots - 1) + 0.5f);

		/* Fit the scroll position. */
		pos = 1.0f - (float)(history_top - (history_slots - 1)) / (float)((history_count - 1) - (history_slots - 1));
		if (pos < 0)
			pos = 0;
		if (pos > 1.0f)
			pos = 1.0f;
	}

	return pos;
}

/* Play the voice from the history. */
static void process_history_voice(int button_index)
{
	const char *voice;

	if (button[button_index].rt.history_offset == -1)
		return;
	if (s3_get_history_count() == 0)
		return ;

	/* Play the voice. */
	voice = s3_get_history_voice(button[button_index].rt.history_offset);
	if (voice != NULL)
		play_se(voice);
	else if (conf_tts_enable)
		speak(s3_get_history_text(button[button_index].rt.history_offset));
}

/*
 * Text Preview Buttons
 */

/* Initialize buttons of type TYPE_PREVIEW */
static bool init_preview_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_PREVIEW)
			continue;
		if (button[i].msg == NULL) {
			button[i].msg = strdup("");
			if (button[i].msg == NULL) {
				s3_log_out_of_memory();
				return false;
			}
		}
		if (button[i].width <= 0)
			button[i].width = 1;
		if (button[i].height <= 0)
			button[i].height = 1;

		button[i].rt.img_canvas_idle = s3_create_image(button[i].width, button[i].height);
		if (button[i].rt.img_canvas_idle == NULL)
			return false;
	}

	reset_preview_buttons();

	return true;
}

/* Reset text previews. */
static void reset_preview_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_PREVIEW)
			continue;

		assert(button[i].msg != NULL);

		reset_preview_button(i);
	}
}

/* Reset text preview. */
static void reset_preview_button(int index)
{
	struct gui_button *b;
	s3_pixel_t color, outline_color;
	int pen_x, pen_y;

	assert(button[index].type == TYPE_PREVIEW);

	b = &button[index];

	/* Clear the image. */
	s3_draw_image(b->rt.img_canvas_idle,
		      0,
		      0,
		      b->rt.img_idle,
		      0,
		      0,
		      s3_get_image_width(b->rt.img_idle),
		      s3_get_image_height(b->rt.img_idle),
		      255,
		      S3_BLEND_COPY);
	s3_notify_image_update(b->rt.img_canvas_idle);

	color = s3_make_pixel(0xff,
			      (uint32_t)conf_msgbox_font_r,
			      (uint32_t)conf_msgbox_font_g,
			      (uint32_t)conf_msgbox_font_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)conf_msgbox_font_outline_r,
				      (uint32_t)conf_msgbox_font_outline_g,
				      (uint32_t)conf_msgbox_font_outline_b);

	/* Calculate the pen position. */
	if (!conf_gui_preview_font_tategaki) {
		pen_x = b->margin_left;
		pen_y = b->margin_top;
	} else {
		pen_x = b->width - b->margin_right - conf_msgbox_font_size;
		pen_y = b->margin_top;
	}

	if (b->rt.msg_context != NULL) {
		s3_destroy_drawmsg(b->rt.msg_context);
		b->rt.msg_context = NULL;
	}

	b->rt.msg_context = s3_create_drawmsg(
		b->rt.img_canvas_idle,
		get_localized_preview_message(index),
		conf_msgbox_font_select,
		conf_msgbox_font_size,
		conf_msgbox_font_size,
		conf_msgbox_font_ruby,
		conf_msgbox_font_outline_width,
		pen_x,
		pen_y,
		b->width,
		b->height,
		b->margin_left,
		b->margin_right,
		b->margin_top,
		b->margin_bottom,
		conf_msgbox_margin_line,
		conf_msgbox_margin_char,
		color,
		outline_color,
		0,		/* bg_color */
		false,		/* fill_bg */
		false,		/* is_dimming */
		false,		/* ignore_linefeed */
		false,		/* ignore_font */
		false,		/* ignore_outline */
		false,		/* ignore_color */
		false,		/* ignore_size */
		false,		/* ignore_position */
		false,		/* ignore_ruby */
		true,		/* ignore_wait */
		NULL,		/* inline_wait_hook */
		conf_gui_preview_font_tategaki);

	b->rt.is_waiting = false;
	b->rt.total_chars = s3_count_drawmsg_chars(b->rt.msg_context, NULL);
	b->rt.drawn_chars = 0;
	s3_reset_lap_timer(&b->rt.sw);
}

/* Get a preview text by the current language. */
static const char *
get_localized_preview_message(
	int index)
{
	const char *lang;

	lang = s3_get_system_language();

	if (button[index].msg_en != NULL && strncmp(lang, "en", 2) == 0)
		return button[index].msg_en;

	if (button[index].msg_fr != NULL && strncmp(lang, "fr", 2) == 0)
		return button[index].msg_fr;

	if (button[index].msg_es != NULL && strncmp(lang, "es", 2) == 0)
		return button[index].msg_es;

	if (button[index].msg_de != NULL && strncmp(lang, "de", 2) == 0)
		return button[index].msg_de;

	if (button[index].msg_it != NULL && strcmp(lang, "it") == 0)
		return button[index].msg_it;

	if (button[index].msg_ru != NULL && strcmp(lang, "ru") == 0)
		return button[index].msg_ru;

	if (button[index].msg_el != NULL && strcmp(lang, "el") == 0)
		return button[index].msg_el;

	if (button[index].msg_zh_cn != NULL && strcmp(lang, "zh-cn") == 0)
		return button[index].msg_zh_cn;

	if (button[index].msg_zh_tw != NULL && strcmp(lang, "zh-tw") == 0)
		return button[index].msg_zh_tw;

	if (button[index].msg_ja != NULL && strcmp(lang, "ja") == 0)
		return button[index].msg_ja;

	if (button[index].msg != NULL)
		return button[index].msg;

	/* Not specified. */
	return "";
}

/* Draw (update) text previews. */
static void draw_preview_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_PREVIEW)
			continue;

		assert(button[i].msg != NULL);

		draw_preview_button(i);
	}
}

/* Draw (update) text preview buttons. */
static void draw_preview_button(int index)
{
	struct gui_button *b;
	uint64_t lap;

	b = &button[index];

	/* If the message is in progress. */
	if (!b->rt.is_waiting) {
		/* Draw the image in main memory. */
		draw_preview_message(index);

		/* If all characters have been drawn. */
		if (b->rt.drawn_chars == b->rt.total_chars) {
			/* Initialize the stopwatch. */
			s3_reset_lap_timer(&b->rt.sw);

			/* Enter the wait time for auto mode. */
			b->rt.is_waiting = true;
		}
	} else {
		/* If in auto mode wait time. */
		lap = s3_get_lap_timer_millisec(&b->rt.sw);
		if (lap >= (uint64_t)get_wait_time(index)) {
			/* End the wait. */
			reset_preview_button(index);
		}
	}
}

/* Draw the message. */
static void draw_preview_message(int index)
{
	struct gui_button *b;
	int char_count;
	int ret_chars;

	b = &button[index];

	/* Get the number of characters to draw this frame. */
	char_count = get_frame_chars(index);
	if (char_count == 0)
		return;

	/* Draw the message. */
	ret_chars = s3_draw_message(b->rt.msg_context, char_count);
	s3_notify_image_update(b->rt.img_canvas_idle);

	/* Add the number of characters drawn. */
	b->rt.drawn_chars += ret_chars;
}

/* Get the number of characters to draw this frame. */
static int get_frame_chars(int index)
{
	float lap;
	int char_count;
	float MESSAGE_SPEED = 80.0f;

	/* If the text speed is maximum, draw all characters without waiting. */
	if (s3_get_text_speed() == 1.0f) {
		return button[index].rt.total_chars -
			button[index].rt.drawn_chars;
	}

	/* Get the elapsed time. */
	lap = (float)s3_get_lap_timer_millisec(&button[index].rt.sw) / 1000.0f;

	/* Get the number of characters to draw this frame. */
	char_count = (int)ceil(MESSAGE_SPEED * (s3_get_text_speed() + 0.1) * lap) - button[index].rt.drawn_chars;
	if (char_count > button[index].rt.total_chars - button[index].rt.drawn_chars)
		char_count = button[index].rt.total_chars - button[index].rt.drawn_chars;

	return char_count;
}

/* Calculate the wait time for auto mode. */
static int get_wait_time(int index)
{
	const float AUTO_MODE_TEXT_WAIT_SCALE = 0.15f;

	return (int)((float)button[index].rt.total_chars * AUTO_MODE_TEXT_WAIT_SCALE * s3_get_auto_speed() * 1000.0f);
}

/*
 * Variable Buttons
 */

/* Initialize buttons of type TYPE_VARIABLE. */
static bool
init_var_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_VAR)
			continue;
		if (button[i].width <= 0)
			button[i].width = 1;
		if (button[i].height <= 0)
			button[i].height = 1;

		button[i].rt.img_canvas_idle = s3_create_image(button[i].width, button[i].height);
		if (button[i].rt.img_canvas_idle == NULL)
			return false;

		s3_draw_image(button[i].rt.img_canvas_idle,
			      0,
			      0,
			      button[i].rt.img_idle,
			      0,
			      0,
			      s3_get_image_width(button[i].rt.img_idle),
			      s3_get_image_height(button[i].rt.img_idle),
			      255,
			      S3_BLEND_COPY);
		s3_notify_image_update(button[i].rt.img_canvas_idle);
	}

	return true;
}

/* Draw all buttons of type TYPE_VAR. */
static void
draw_var_buttons(void)
{
	int i;

	for (i = 0; i < S3_BUTTON_LAYERS; i++)
		if (button[i].type == TYPE_VAR)
			draw_var_button(i);
}

/* Draw buttons of type TYPE_VAR. */
static void
draw_var_button(int index)
{
	s3_draw_image(button[index].rt.img_canvas_idle,
		      0,
		      0,
		      button[index].rt.img_idle,
		      0,
		      0,
		      s3_get_image_width(button[index].rt.img_idle),
		      s3_get_image_height(button[index].rt.img_idle),
		      255,
		      S3_BLEND_COPY);
	draw_var_value(index);
}

/* Draw variable value. */
static void
draw_var_value(
	     int index)
{
	struct s3_drawmsg *context;
	struct gui_button *b;
	const char *name;
	s3_pixel_t color, outline_color;
	int char_count;

	b = &button[index];

	/* Get colors. */
	color = s3_make_pixel(0xff,
			      (uint32_t)conf_msgbox_font_r,
			      (uint32_t)conf_msgbox_font_g,
			      (uint32_t)conf_msgbox_font_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)conf_msgbox_font_outline_r,
				      (uint32_t)conf_msgbox_font_outline_g,
				      (uint32_t)conf_msgbox_font_outline_b);

	/* Get the string to draw. */
	name = s3_get_variable_string(b->var);
	assert(name != NULL);

	/* Draw the variable value. */
	context = s3_create_drawmsg(
		b->rt.img_canvas_idle,
		name,
		conf_msgbox_font_select,
		conf_msgbox_font_size,
		conf_msgbox_font_size,
		conf_msgbox_font_ruby,
		conf_msgbox_font_outline_width,
		0,		/* pen_x */
		0,		/* pen_y */
		b->width,
		b->height,
		b->margin_left,		/* left_margin */
		b->margin_right,	/* right_margin */
		b->margin_top,		/* top_margin */
		b->margin_bottom,	/* bottom_margin */
		0,		/* line_margin */
		conf_msgbox_margin_char,
		color,
		outline_color,
		0,		/* bg_color */
		false,		/* fill_bg */
		false,		/* is_dimming */
		false,		/* ignore_linefeed */
		false,		/* ignore_font */
		false,		/* ignore_outline */
		false,		/* ignore_color */
		false,		/* ignore_size */
		false,		/* ignore_position */
		false,		/* ignore_ruby */
		true,		/* ignore_wait */
		NULL,		/* inline_wait_hook */
		false);		/* use_tategaki */
	if (context == NULL)
		return;
	char_count = s3_count_drawmsg_chars(context, NULL);
	s3_draw_message(context, char_count);
	s3_destroy_drawmsg(context);
}

/* Process the press of a name string edit button. */
static void process_char(int index)
{
	char buf[1204];
	struct gui_button *b;
	const char *orig;

	b = &button[index];
	if (b->msg == NULL)
		return;

	/* Clear button. */
	if (strcmp(b->msg, "[clear]") == 0) {
		s3_set_variable_string(b->var, "");
		return;
	}

	/* Backspace button. */
	if (strcmp(b->msg, "[backspace]") == 0) {
		truncate_variable(b->var);
		return;
	}

	/* OK button. */
	if (strcmp(b->msg, "[ok]") == 0) {
		/*  If the name variable is empty, it cannot be decided. */
		if (strcmp(s3_get_variable_string(b->var), "") != 0)
			result_index = index;
		return;
	}

	/* Get the current value of the variable. */
	orig = s3_get_variable_string(b->var);

	/* If the maximum number of characters has already been reached. */
	if (s3_count_utf8_chars(orig) >= b->max)
		return;

	/* Create a string to append to the end. */
	snprintf(buf, sizeof(buf), "%s%s", orig, b->msg);

	/* Update the value of the name variable. */
	s3_set_variable_string(b->var, buf);
}

static void
truncate_variable(
	const char *var)
{
	int count;
	uint32_t wc;
	const char *val;
	const char *s;
	size_t copy_len, last_len;
	char buf[1024];

	val = s3_get_variable_string(var);

	count = 0;
	last_len = 0;
	s = val;
	while (*s != '\0') {
		int len = s3_utf8_to_utf32(s, &wc);
		if (len < 0)
			return;
		count++;
		last_len = (size_t)len;
		s += len;
	}

	if (count == 0)
		return;

	copy_len = strlen(val) - last_len;
	if (copy_len >= sizeof(buf) - 1)
		return;

	memcpy(buf, val, copy_len);
	buf[copy_len] = '\0';

	s3_set_variable_string(var, buf);
}

static void
process_language(
	int index)
{
	struct gui_button *b;

	b = &button[index];
	if (b->lang == NULL)
		return;

	/* Set the locale. */
	s3_set_config("game.locale", b->lang);
}

/* Render a button image. */
static void
render_image_helper(
	struct s3_image *img,
	int bid)
{
	int x, y, cx, cy, blend, alpha, layer;
	float sx, sy, rot;

	assert(bid >= 1 && bid <= S3_BUTTON_LAYERS);
	if (bid < 1 || bid > S3_BUTTON_LAYERS)
		return;

	/* Get the layer index. (ID 1 = S3_LAYER_GUI_BTN1) */
	layer = S3_LAYER_GUI_BTN1 + bid - 1;

	/* Get the layer parameter. (They are updated by the anime system.) */
	x = s3_get_layer_x(layer);
	y = s3_get_layer_y(layer);
	sx = s3_get_layer_scale_x(layer);
	sy = s3_get_layer_scale_y(layer);
	cx = s3_get_layer_center_x(layer);
	cy = s3_get_layer_center_y(layer);
	rot = s3_get_layer_rotate(layer);
	blend = s3_get_layer_blend(layer);
	alpha = (int)(
		  ((float)s3_get_layer_alpha(layer) / 255.0f) *
		  ((float)cur_alpha / 255.0f) *
		  255.0f
		);

	/* If 3D. */
	if (rot != 0.0f ||
	    sx != 1.0f ||
	    sy != 1.0f) {
		float x1 = 0;
		float y1 = 0;
		float x2 = (float)img->width - 1.0f;
		float y2 = 0;
		float x3 = 0;
		float y3 = (float)img->height - 1.0f;;
		float x4 = (float)img->width - 1.0f;
		float y4 = (float)img->height - 1.0f;
		float center_x = (float)cx;
		float center_y = (float)cy;
		float rad = (float)rot;

		/* 1. Shift for the centering. */
		x1 -= center_x;
		y1 -= center_y;
		x2 -= center_x;
		y2 -= center_y;
		x3 -= center_x;
		y3 -= center_y;
		x4 -= center_x;
		y4 -= center_y;

		/* 2. Scale. */
		x1 *= sx;
		y1 *= sy;
		x2 *= sx;
		y2 *= sy;
		x3 *= sx;
		y3 *= sy;
		x4 *= sx;
		y4 *= sy;

		/* 3. Rotate. */
		if (rad != 0) {
			float tmp_x, tmp_y;

			tmp_x = x1;
			tmp_y = y1;
			x1 = tmp_x * cosf(rad) - tmp_y * sinf(rad);
			y1 = tmp_x * sinf(rad) + tmp_y * cosf(rad);

			tmp_x = x2;
			tmp_y = y2;
			x2 = tmp_x * cosf(rad) - tmp_y * sinf(rad);
			y2 = tmp_x * sinf(rad) + tmp_y * cosf(rad);

			tmp_x = x3;
			tmp_y = y3;
			x3 = tmp_x * cosf(rad) - tmp_y * sinf(rad);
			y3 = tmp_x * sinf(rad) + tmp_y * cosf(rad);

			tmp_x = x4;
			tmp_y = y4;
			x4 = tmp_x * cosf(rad) - tmp_y * sinf(rad);
			y4 = tmp_x * sinf(rad) + tmp_y * cosf(rad);
		}

		/* 4. Shift again for the centering. */
		x1 += (float)cx;
		y1 += (float)cy;
		x2 += (float)cx;
		y2 += (float)cy;
		x3 += (float)cx;
		y3 += (float)cy;
		x4 += (float)cx;
		y4 += (float)cy;

		/* 5. Shift for the layer position. */
		x1 += (float)x;
		y1 += (float)y;
		x2 += (float)x;
		y2 += (float)y;
		x3 += (float)x;
		y3 += (float)y;
		x4 += (float)x;
		y4 += (float)y;

		/* Render. */
		s3_render_image_3d(x1,
				   y1,
				   x2,
				   y2,
				   x3,
				   y3,
				   x4,
				   y4,
				   img,
				   0,
				   0,
				   img->width,
				   img->height,
				   alpha,
				   blend);
		return;
	}

	/* Otherwise 2D. */
	s3_render_image(x,
			y,
			(int)((float)img->width * sx),
			(int)((float)img->height * sy),
			img,
			0,
			0,
			img->width,
			img->height,
			alpha,
			blend);
}

/*
 * Sound Effect Playback and Text-to-Speech
 */

/* Play a sound effect. */
static void
play_se(const char *file)
{
	if (file == NULL || strcmp(file, "") == 0)
		return;

	s3_set_mixer_input_file(S3_TRACK_SYS, file, false);
}

/* Play a system sound effect. */
static void
play_sys_se(const char *file)
{
	if (file == NULL || strcmp(file, "") == 0)
		return;

	s3_set_mixer_input_file(S3_TRACK_SYS, file, false);
}

/* Speak the alternative text of a button. */
static void
speak(const char *text)
{
	if (text != NULL)
		s3_speak_text(text);
}


/*
 * GUI File Loading
 */

/* Load a GUI file. */
static bool
load_gui_file(const char *file)
{
	enum {
		ST_SCOPE,
		ST_OPEN,
		ST_KEY,
		ST_COLON,
		ST_VALUE,
		ST_VALUE_DQ,
		ST_SEMICOLON,
		ST_ERROR
	};

	char word[256], key[256];
	char *buf;
	size_t fsize, pos;
	int st, len, line, btn;
	char c;
	bool is_global, is_comment;

	assert(file != NULL);

	memset(word, 0, sizeof(word));
	memset(key, 0, sizeof(key));

	/* Get the file content. */
	if (!s3_read_file_content(file, &buf, &fsize))
		return false;

	/* Convert comments to spaces. */
	is_comment = false;
	for (pos = 0; pos < fsize; pos++) {
		if (!is_comment) {
			if (buf[pos] == '#') {
				buf[pos] = ' ';
				is_comment = true;
			}
		} else {
			if (buf[pos] == '\n')
				is_comment = false;
			else
				buf[pos] = ' ';
		}
	}

	/* Parse the file. */
	st = ST_SCOPE;
	line = 0;
	len = 0;
	btn = -1;
	pos = 0;
	is_global = false;
	while (pos < fsize) {
		/* Read one character. */
		c = buf[pos++];

		/* Interpret according to the state. */
		switch (st) {
		case ST_SCOPE:
			if (len == 0) {
				if (c == ' ' || c == '\t' || c == '\r' ||
				    c == '\n') {
					st = ST_SCOPE;
					break;
				}
				if (c == ':' || c == '{' || c == '}') {
					s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
					st = ST_ERROR;
					break;
				}
			}
			if (c == '}' || c == ':') {
				s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
			    c == '{') {
				assert(len > 0);
				word[len] = '\0';
				if (strcmp(word, "global") == 0) {
					is_global = true;
				} else {
					is_global = false;
					if (!add_button(++btn, file)) {
						st = ST_ERROR;
						break;
					}
				}
				if (c == '{')
					st = ST_KEY;
				else
					st = ST_OPEN;
				len = 0;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file \"%s\" line %d."), file, line + 1);
				st = ST_ERROR;
				break;
			}
			word[len++] = c;
			st = ST_SCOPE;
			break;
		case ST_OPEN:
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				st = ST_OPEN;
				break;
			}
			if (c == '{') {
				st = ST_KEY;
				len = 0;
				break;
			}
			s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
			st = ST_ERROR;
			break;
		case ST_KEY:
			if (len == 0) {
				if (c == ' ' || c == '\t' || c == '\r' ||
				    c == '\n') {
					st = ST_KEY;
					break;
				}
				if (c == ':') {
					s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
					st = ST_ERROR;
					break;
				}
				if (c == '}') {
					st = ST_SCOPE;
					break;
				}
			}
			if (c == '{' || c == '}') {
				s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				word[len] = '\0';
				strcpy(key, word);
				st = ST_COLON;
				len = 0;
				break;
			}
			if (c == ':') {
				word[len] = '\0';
				strcpy(key, word);
				st = ST_VALUE;
				len = 0;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file \"%s\" line %d."), file, line + 1);
				st = ST_ERROR;
				break;
			}
			word[len++] = c;
			st = ST_KEY;
			break;
		case ST_COLON:
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				st = ST_COLON;
				break;
			}
			if (c == ':') {
				st = ST_VALUE;
				len = 0;
				break;
			}
			s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
			st = ST_ERROR;
			break;
		case ST_VALUE:
			if (len == 0) {
				if (c == ' ' || c == '\t' || c == '\r' ||
				    c == '\n') {
					st = ST_VALUE;
					break;
				}
				if (c == '\"') {
					st = ST_VALUE_DQ;
					break;
				}
			}
			if (c == ':' || c == '{') {
				s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
			    c == ';' || c == '}') {
				word[len] = '\0';
				if (is_global) {
					if (!set_global_key_value(key, word, file, line + 1)) {
						st = ST_ERROR;
						break;
					}
				} else {
					if (!set_button_key_value(btn, key, word, file, line + 1)) {
						st = ST_ERROR;
						break;
					}
				}
				if (c == ';')
					st = ST_KEY;
				else if (c == '}')
					st = ST_SCOPE;
				else
					st = ST_SEMICOLON;
				len = 0;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file \"%s\" line %d."), file, line + 1);
				st = ST_ERROR;
				break;
			}
			word[len++] = c;
			st = ST_VALUE;
			break;
		case ST_VALUE_DQ:
			if (c == '\"') {
				word[len] = '\0';
				if (is_global) {
					if (!set_global_key_value(key, word, file, line + 1)) {
						st = ST_ERROR;
						break;
					}
				} else {
					if (!set_button_key_value(btn, key, word, file, line + 1)) {
						st = ST_ERROR;
						break;
					}
				}
				st = ST_SEMICOLON;
				len = 0;
				break;
			}
			if (c == '\r' || c == '\n') {
				s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file \"%s\" line %d."), file, line + 1);
				st = ST_ERROR;
				break;
			}
			word[len++] = c;
			st = ST_VALUE_DQ;
			break;
		case ST_SEMICOLON:
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				st = ST_SEMICOLON;
				break;
			}
			if (c == ';') {
				st = ST_KEY;
				len = 0;
				break;
			}
			if (c == '}') {
				st = ST_SCOPE;
				len = 0;
				break;
			}
			s3_log_error(S3_TR("Invalid character \"%c\" while parsing anime file \"%s\" line %d."), c, file, line + 1);
			st = ST_ERROR;
			break;
		}

		/* In case of error. */
		if (st == ST_ERROR)
			break;

		/* FIXME: Handle line breaks with only '\r'. */
		if (c == '\n')
			line++;
	}

	/* In case of error. */
	if (st == ST_ERROR) {
		s3_log_error(S3_TR("Error loading GUI file %s."), file);
	} else if (st != ST_SCOPE || len > 0) {
		s3_log_error(S3_TR("Invalid EOF on GUI file %s."), file);
	}

	/* Release the buffer. */
	free(buf);

	return st != ST_ERROR;
}

/* Set global key and value. */
static bool
set_global_key_value(
	const char *key,
	const char *val,
	const char *file,
	int line)
{
	if (strcmp(key, "saveslots") == 0) {
		save_slots = atoi(val);
		return true;
	}

	if (strcmp(key, "historyslots") == 0) {
		history_slots = atoi(val);
		return true;
	}

	if (strcmp(key, "cancelse") == 0) {
		cancel_se = strdup(val);
		if (cancel_se == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	if (strcmp(key, "fadein") == 0) {
		fade_in_time = (float)atof(val);
		return true;
	}

	if (strcmp(key, "fadeout") == 0) {
		fade_out_time = (float)atof(val);
		return true;
	}

	if (strcmp(key, "timed") == 0) {
		bomb_time = (float)atof(val);
		s3_reset_lap_timer(&bomb_sw);
		return true;
	}

	if (strcmp(key, "clickcancel") == 0) {
		is_click_cancel = strcmp(val, "yes") == 0 ? true : false;
		return true;
	}

	if (strcmp(key, "escapecancel") == 0) {
		is_escape_cancel = strcmp(val, "yes") == 0 ? true : false;
		return true;
	}

	if (strcmp(key, "alt") == 0) {
		if (conf_tts_enable)
			speak(val);
		return true;
	}

	s3_log_error(S3_TR("Unknown global key \"%s\" found in GUI file \"%s\" line %d."), key, file, line);
	return false;
}

/* Add a button. */
static bool
add_button(
	int index,
	const char *file)
{
	/* If the number of buttons exceeds the limit. */
	if (index >= S3_BUTTON_LAYERS) {
		s3_log_error(S3_TR("Too many GUI buttons in GUI file \"%s\"."), file);
		return false;
	}

	button[index].is_initialized = true;

	/* Get an unused layer. */
	button[index].bid = get_unused_bid();
	if (button[index].bid == -1) {
		s3_log_error(S3_TR("No available button-id for a GUI button in GUI file \"%s\"."), file);
		return false;
	}

	/* There is no specific processing for adding a button, just checking the count. */
	return true;
}

static int
get_unused_bid(void)
{
	int i;

	for (i = 1; i < S3_BUTTON_LAYERS; i++)
		if (is_bid_unused(i))
			return i;

	return -1;
}

static int
is_bid_unused(int bid)
{
	int i;

	for (i = 1; i < S3_BUTTON_LAYERS; i++)
		if (button[i].bid == bid)
			return false;

	return true;
}

/* Set the key of a button. */
static bool
set_button_key_value(
	const int index,
	const char *key,
	const char *val,
	const char *file,
	int line)
{
	struct gui_button *b;

	assert(index >= 0 && index < S3_BUTTON_LAYERS);

	b = &button[index];

	/*
	 * Process keys other than type.
	 */

	/* id */
	if (strcmp("id", key) == 0) {
		int bid = atoi(val);
		if (bid < 1)
			bid = 1;
		if (bid > S3_BUTTON_LAYERS)
			bid = S3_BUTTON_LAYERS;
		if (bid != b->bid) {
			if (is_bid_unused(bid)) {
				b->bid = bid;
			} else {
				s3_log_warn(S3_TR("Specified ID is already used in GUI file \"%s\" line %d."), file, line);
			}
		}
		b->type = TYPE_NOACTION;
		return true;
	}

	/* type */
	if (strcmp("type", key) == 0) {
		b->type = get_type_for_name(val, file, line);
		if (b->type == TYPE_INVALID)
			return false;
		return true;
	}

	/* x */
	if (strcmp("x", key) == 0) {
		b->x = atoi(val);
		return true;
	}

	/* y */
	if (strcmp("y", key) == 0) {
		b->y = atoi(val);
		return true;
	}

	/* width */
	if (strcmp("width", key) == 0) {
		b->width = atoi(val);
		if (b->width <= 0)
			b->width = 1;
		return true;
	}

	/* height */
	if (strcmp("height", key) == 0) {
		b->height = atoi(val);
		if (b->height <= 0)
			b->height = 1;
		return true;
	}

	/* image-idle */
	if (strcmp("image-idle", key) == 0) {
		if (b->rt.img_idle != NULL)
			s3_destroy_image(b->rt.img_idle);
		b->rt.img_idle = s3_create_image_from_file(val);
		if (b->rt.img_idle == NULL)
			return false;
		if (b->width == 0 || b->height == 0) {
			b->width = s3_get_image_width(b->rt.img_idle);
			b->height = s3_get_image_height(b->rt.img_idle);
		}
		return true;
	}

	/* image-hover */
	if (strcmp("image-hover", key) == 0) {
		if (b->rt.img_hover != NULL)
			s3_destroy_image(b->rt.img_hover);
		b->rt.img_hover = s3_create_image_from_file(val);
		if (b->rt.img_hover == NULL)
			return false;
		if (b->width == 0 || b->height == 0) {
			b->width = s3_get_image_width(b->rt.img_hover);
			b->height = s3_get_image_height(b->rt.img_hover);
		}
		return true;
	}

	/* image-press */
	if (strcmp("image-press", key) == 0) {
		if (b->rt.img_press != NULL)
			s3_destroy_image(b->rt.img_press);
		b->rt.img_press = s3_create_image_from_file(val);
		if (b->rt.img_press == NULL)
			return false;
		if (b->width == 0 || b->height == 0) {
			b->width = s3_get_image_width(b->rt.img_press);
			b->height = s3_get_image_height(b->rt.img_press);
		}
		return true;
	}

	/* image-disable */
	if (strcmp("image-active", key) == 0) {
		if (b->rt.img_active != NULL)
			s3_destroy_image(b->rt.img_active);
		b->rt.img_active = s3_create_image_from_file(val);
		if (b->rt.img_active == NULL)
			return false;
		if (b->width == 0 || b->height == 0) {
			b->width = s3_get_image_width(b->rt.img_active);
			b->height = s3_get_image_height(b->rt.img_active);
		}
		return true;
	}

	/* image-disable */
	if (strcmp("image-disable", key) == 0 ||
	    strcmp("image-knob", key) == 0) {
		if (b->rt.img_disable != NULL)
			s3_destroy_image(b->rt.img_disable);
		b->rt.img_disable = s3_create_image_from_file(val);
		if (b->rt.img_disable == NULL)
			return false;
		if (b->width == 0 || b->height == 0) {
			b->width = s3_get_image_width(b->rt.img_disable);
			b->height = s3_get_image_height(b->rt.img_disable);
		}
		return true;
	}

	/* label */
	if (strcmp("label", key) == 0) {
		b->label = strdup(val);
		if (b->label == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* file */
	if (strcmp("file", key) == 0) {
		b->file = strdup(val);
		if (b->file == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* alt */
	if (strcmp("alt", key) == 0) {
		b->alt = strdup(val);
		if (b->alt == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* order */
	if (strcmp("order", key) == 0) {
		b->order = atoi(val);
		return true;
	}

	/* var */
	if (strcmp("var", key) == 0) {
		b->var = strdup(val);
		if (b->var == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* index */
	if (strcmp("index", key) == 0) {
		b->index = atoi(val);
		if (b->type == TYPE_CHARACTERVOL) {
			if (b->index < 0)
				b->index = 0;
			if (b->index >= S3_CH_VOL_SLOTS)
				b->index = S3_CH_VOL_SLOTS - 1;
		}
		if (b->type == TYPE_HISTORY) {
			if (b->index >= history_slots)
				s3_log_warn("history slot index is out-of-range.");
		}
		if (b->type == TYPE_SAVE || b->type == TYPE_LOAD) {
			if (b->index >= save_slots + 1)
				s3_log_warn("save slot index is out-of-range.");
		}
		return true;
	}

	/* msg */
	if (strcmp("msg", key) == 0) {
		b->msg = strdup(val);
		if (b->msg == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-en */
	if (strcmp("msg-en", key) == 0) {
		b->msg_en = strdup(val);
		if (b->msg_en == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-fr */
	if (strcmp("msg-fr", key) == 0) {
		b->msg_fr = strdup(val);
		if (b->msg_fr == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-es */
	if (strcmp("msg-es", key) == 0) {
		b->msg_es = strdup(val);
		if (b->msg_es == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-de */
	if (strcmp("msg-de", key) == 0) {
		b->msg_de = strdup(val);
		if (b->msg_de == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-it */
	if (strcmp("msg-it", key) == 0) {
		b->msg_it = strdup(val);
		if (b->msg_it == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-el */
	if (strcmp("msg-el", key) == 0) {
		b->msg_el = strdup(val);
		if (b->msg_el == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-ru */
	if (strcmp("msg-ru", key) == 0) {
		b->msg_ru = strdup(val);
		if (b->msg_ru == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-zh-cn */
	if (strcmp("msg-zh-cn", key) == 0) {
		b->msg_zh_cn = strdup(val);
		if (b->msg_zh_cn == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-zh-tw */
	if (strcmp("msg-zh-tw", key) == 0) {
		b->msg_zh_tw = strdup(val);
		if (b->msg_zh_tw == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* msg-ja */
	if (strcmp("msg-ja", key) == 0) {
		b->msg_ja = strdup(val);
		if (b->msg_ja == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* clickse */
	if (strcmp("clickse", key) == 0) {
		b->clickse = strdup(val);
		if (b->clickse == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* pointse */
	if (strcmp("pointse", key) == 0) {
		b->pointse = strdup(val);
		if (b->pointse == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* variable */
	if (strcmp("variable", key) == 0) {
		b->var = strdup(val);
		if (b->var == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* max */
	if (strcmp("max", key) == 0) {
		b->max = abs(atoi(val));
		return true;
	}

	/* index-x */
	if (strcmp("index-x", key) == 0) {
		b->index_x = atoi(val);
		return true;
	}

	/* index-y */
	if (strcmp("index-y", key) == 0) {
		b->index_y = atoi(val);
		return true;
	}

	/* date-x */
	if (strcmp("date-x", key) == 0) {
		b->date_x = atoi(val);
		return true;
	}

	/* date-y */
	if (strcmp("date-y", key) == 0) {
		b->date_y = atoi(val);
		return true;
	}

	/* thumb-x */
	if (strcmp("thumb-x", key) == 0) {
		b->thumb_x = atoi(val);
		return true;
	}

	/* thumb-y */
	if (strcmp("thumb-y", key) == 0) {
		b->thumb_y = atoi(val);
		return true;
	}

	/* thumb-width */
	if (strcmp("thumb-width", key) == 0) {
		b->thumb_width = atoi(val);
		return true;
	}

	/* thumb-height */
	if (strcmp("thumb-height", key) == 0) {
		b->thumb_height = atoi(val);
		return true;
	}

	/* chapter-x */
	if (strcmp("chapter-x", key) == 0) {
		b->chapter_x = atoi(val);
		return true;
	}

	/* chapter-y */
	if (strcmp("chapter-y", key) == 0) {
		b->chapter_y = atoi(val);
		return true;
	}

	/* msg-x */
	if (strcmp("msg-x", key) == 0) {
		b->msg_x = atoi(val);
		return true;
	}

	/* msg-y */
	if (strcmp("msg-y", key) == 0) {
		b->msg_y = atoi(val);
		return true;
	}

	/* new-x */
	if (strcmp("new-x", key) == 0) {
		b->new_x = atoi(val);
		b->rt.is_new_enabled = true;
		return true;
	}

	/* new-y */
	if (strcmp("new-y", key) == 0) {
		b->new_y = atoi(val);
		b->rt.is_new_enabled = true;
		return true;
	}

	/* name-x */
	if (strcmp("name-x", key) == 0) {
		b->name_x = atoi(val);
		return true;
	}

	/* name-y */
	if (strcmp("name-y", key) == 0) {
		b->name_y = atoi(val);
		return true;
	}

	/* text-x */
	if (strcmp("text-x", key) == 0) {
		b->text_x = atoi(val);
		return true;
	}

	/* text-y */
	if (strcmp("text-y", key) == 0) {
		b->text_y = atoi(val);
		return true;
	}

	/* margin-left */
	if (strcmp("margin-left", key) == 0) {
		b->margin_left = atoi(val);
		return true;
	}

	/* margin-top */
	if (strcmp("margin-top", key) == 0) {
		b->margin_top = atoi(val);
		return true;
	}

	/* margin-right */
	if (strcmp("margin-right", key) == 0) {
		b->margin_right = atoi(val);
		return true;
	}

	/* margin-bottom */
	if (strcmp("margin-bottom", key) == 0) {
		b->margin_bottom = atoi(val);
		return true;
	}

	/* anime-idle */
	if (strcmp("anime-idle", key) == 0) {
		b->anime_idle = strdup(val);
		if (b->anime_idle == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* anime-hover */
	if (strcmp("anime-hover", key) == 0) {
		b->anime_hover = strdup(val);
		if (b->anime_hover == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* anime-press */
	if (strcmp("anime-press", key) == 0) {
		b->anime_press = strdup(val);
		if (b->anime_press == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* anime-active */
	if (strcmp("anime-active", key) == 0) {
		b->anime_active = strdup(val);
		if (b->anime_active == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	/* anime-idle */
	if (strcmp("lang", key) == 0) {
		b->lang = strdup(val);
		if (b->lang == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		return true;
	}

	s3_log_error(S3_TR("Unknown button property \"%s\" found in GUI file \"%s\" line %d."), key, file, line);
	return false;
}

/* Get the type index by type name. */
static int
get_type_for_name(
	const char *name,
	const char *file,
	int line)
{
	struct {
		const char *name;
		int value;
	} type_array[] = {
		{"noaction",			TYPE_NOACTION},
		{"goto",			TYPE_LABEL},
		{"mastervol",			TYPE_MASTERVOL},
		{"bgmvol",			TYPE_BGMVOL},
		{"voicevol",			TYPE_VOICEVOL},
		{"sevol",			TYPE_SEVOL},
		{"charactervol",		TYPE_CHARACTERVOL},
		{"textspeed",			TYPE_TEXTSPEED},
		{"autospeed",			TYPE_AUTOSPEED},
		{"preview",			TYPE_PREVIEW},
		{"fullscreen",			TYPE_FULLSCREEN},
		{"window",			TYPE_WINDOW},
		{"gui",				TYPE_GUI},
		{"savepage",			TYPE_SAVEPAGE},
		{"save",			TYPE_SAVE},
		{"load",			TYPE_LOAD},
		{"history",			TYPE_HISTORY},
		{"auto",			TYPE_AUTO},
		{"skip",			TYPE_SKIP},
		{"historyscroll",		TYPE_HISTORYSCROLL},
		{"historyscroll-horizontal",	TYPE_HISTORYSCROLL_HORIZONTAL},
		{"default",			TYPE_DEFAULT},
		{"title",			TYPE_TITLE},
		{"cancel",			TYPE_CANCEL},
		{"var",				TYPE_VAR},
		{"char",			TYPE_CHAR},
		{"language",			TYPE_LANGUAGE},
	};
	size_t i;

	/* Search for the type name. */
	for (i = 0; i < sizeof(type_array) / sizeof(type_array[0]); i++)
		if (strcmp(name, type_array[i].name) == 0)
			return type_array[i].value;

	/* If not found. */
	s3_log_error(S3_TR("Unknown button type name \"%s\" in GUI file \"%s\" line %d."), name, file, line);
	return TYPE_INVALID;
}

static void
set_gui_call_arg(
	int bid)
{
	char layer[32];
	int i;

	snprintf(layer, sizeof(layer), "gui%d", bid);

	s3_set_call_argument(0, layer);

	for (i = 1; i < S3_CALL_ARGS; i++)
		s3_set_call_argument(i, NULL);
}

static void
stop_button_anime(
	int index)
{
	int i, layer;

	/* Stop existing animations. */
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (button[index].rt.used_layers[i]) {
			s3_clear_layer_anime_sequence(i);
			button[index].rt.used_layers[i] = false;
		}
	}

	/* Reset layer parameters. */
	if (button[index].bid != -1) {
		layer = S3_LAYER_GUI_BTN1 + button[index].bid - 1;
		s3_set_layer_position(layer, button[index].x, button[index].y);
		s3_set_layer_scale(layer, 1.0f, 1.0f);
		s3_set_layer_center(layer, 0, 0);
		s3_set_layer_rotate(layer, 0);
	}
}

static bool
start_button_anime(
	int index,
	const char *anime_file)
{
	int layer;

	if (anime_file == NULL)
		return true;
	if (button[index].bid == -1)
		return true;

	/* Reset the position. */
	layer = S3_LAYER_GUI_BTN1 + button[index].bid - 1;
	s3_set_layer_position(layer, button[index].x, button[index].y);
	s3_set_layer_scale(layer, 1.0f, 1.0f);
	s3_set_layer_center(layer, 0, 0);
	s3_set_layer_rotate(layer, 0);

	/* Set the call argument for the anime. ($0 = "gui%d") */
	set_gui_call_arg(button[index].bid);

	/* Load an anime. */
	if (!s3_load_anime_from_file(anime_file, NULL, button[index].rt.used_layers))
		return false;

	return true;
}

static void
change_save_page(
	int index)
{
	int old_save_page, i;

	old_save_page = save_page;
	save_page = index;

	/* Guard. */
	if (!is_gui_running)
		return;
	if (old_save_page == save_page)
		return;

	/* Restart animations. */
	for (i = 0; i < S3_BUTTON_LAYERS; i++) {
		if (button[i].type != TYPE_SAVEPAGE)
			continue;
		if (button[i].bid == -1)
			continue;

		/* Active --> Inactive */
		if (button[i].index == old_save_page) {
			stop_button_anime(i);
			start_button_anime(i, button[i].anime_idle);
			button[i].rt.is_disabled = true;
		}

		/* Inactive --> Active */
		if (button[i].index == save_page) {
			stop_button_anime(i);
			start_button_anime(i, button[i].anime_active);
			button[i].rt.is_disabled = false;
		}
	}
}

static bool
is_current_language(
	const char *lang)
{
	if (strcmp(lang, s3_get_system_language()) == 0)
		return true;

	return false;
}
