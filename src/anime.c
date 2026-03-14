/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Animation Subsystem
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
#include "anime.h"
#include "image.h"
#include "conf.h"

#include <playfield/playfield.h>

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define INVALID_ACCEL_TYPE	(0)

/*
 * Maximum number of the anime sequences per a layer.
 */
#define SEQUENCE_COUNT		(1024)

/*
 * Anime sequence.
 */
struct sequence {
	int layer;
	float start_time;
	float end_time;
	float from_x;
	float from_y;
	float from_a;
	float from_scale_x;
	float from_scale_y;
	float to_x;
	float to_y;
	float to_a;
	float to_scale_x;
	float to_scale_y;
	float center_x;
	float center_y;
	float from_rotate;
	float to_rotate;
	int frame;	/* for eye/lip */
	int blend;
	int accel;
	bool loop;
	bool hide_special;
	bool show_special;
};
static struct sequence sequence[S3_STAGE_LAYERS][SEQUENCE_COUNT];

/*
 * Context of a layer anime.
 */
struct layer_context {
	/* props */
	int seq_count;
	bool is_running;
	bool is_finished;
	uint64_t loop_len;
	uint64_t loop_ofs;

	/* prop (deprecated) */
	char *file;

	/* runtime */
	uint64_t sw;
	float cur_lap;
};
static struct layer_context context[S3_STAGE_LAYERS];

/*
 * Mapping from a layer name to a layer index.
 */
struct layer_name_map {
	const char *name;
	int index;
};
static struct layer_name_map layer_name_map[] = {
	{"bg",			S3_LAYER_BG},
	{"bg_fo",		S3_LAYER_BG_FO},
	{"bg2",			S3_LAYER_BG2},
	{"efb1",		S3_LAYER_EFB1},
	{"efb2",		S3_LAYER_EFB2},
	{"efb3",		S3_LAYER_EFB3},
	{"efb4",		S3_LAYER_EFB4},
	{"chb",			S3_LAYER_CHB},
	{"chb-eye",		S3_LAYER_CHB_EYE},
	{"chb-lip",		S3_LAYER_CHB_LIP},
	{"chb-fo",		S3_LAYER_CHB_FO},
	{"chl",			S3_LAYER_CHL},
	{"chl-eye",		S3_LAYER_CHL_EYE},
	{"chl-lip",		S3_LAYER_CHL_LIP},
	{"chl-fo",		S3_LAYER_CHL_FO},
	{"chlc",		S3_LAYER_CHLC},
	{"chlc-eye",		S3_LAYER_CHLC_EYE},
	{"chlc-lip",		S3_LAYER_CHLC_LIP},
	{"chlc-fo",		S3_LAYER_CHLC_FO},
	{"chr",			S3_LAYER_CHR},
	{"chr-eye",		S3_LAYER_CHR_EYE},
	{"chr-lip",		S3_LAYER_CHR_LIP},
	{"chr-fo",		S3_LAYER_CHR_FO},
	{"chrc",		S3_LAYER_CHRC},
	{"chrc-eye",		S3_LAYER_CHRC_EYE},
	{"chrc-lip",		S3_LAYER_CHRC_LIP},
	{"chrc-fo",		S3_LAYER_CHRC_FO},
	{"chc",			S3_LAYER_CHC},
	{"chc-eye",		S3_LAYER_CHC_EYE},
	{"chc-lip",		S3_LAYER_CHC_LIP},
	{"chc-fo",		S3_LAYER_CHC_FO},
	{"eff1",		S3_LAYER_EFF1},
	{"eff2",		S3_LAYER_EFF2},
	{"eff3",		S3_LAYER_EFF3},
	{"eff4",		S3_LAYER_EFF4},
	{"msgbox",		S3_LAYER_MSGBOX},
	{"namebox"	,	S3_LAYER_NAMEBOX},
	{"choose1-idle",	S3_LAYER_CHOOSE1_IDLE},
	{"choose1-hover",	S3_LAYER_CHOOSE1_HOVER},
	{"choose2-idle",	S3_LAYER_CHOOSE2_IDLE},
	{"choose2-hover",	S3_LAYER_CHOOSE2_HOVER},
	{"choose3-idle",	S3_LAYER_CHOOSE3_IDLE},
	{"choose3-hover",	S3_LAYER_CHOOSE3_HOVER},
	{"choose4-idle",	S3_LAYER_CHOOSE4_IDLE},
	{"choose4-hover",	S3_LAYER_CHOOSE4_HOVER},
	{"choose5-idle",	S3_LAYER_CHOOSE5_IDLE},
	{"choose5-hover",	S3_LAYER_CHOOSE5_HOVER},
	{"choose6-idle",	S3_LAYER_CHOOSE6_IDLE},
	{"choose6-hover",	S3_LAYER_CHOOSE6_HOVER},
	{"choose7-idle",	S3_LAYER_CHOOSE7_IDLE},
	{"choose7-hover",	S3_LAYER_CHOOSE7_HOVER},
	{"choose8-idle",	S3_LAYER_CHOOSE8_IDLE},
	{"choose8-hover",	S3_LAYER_CHOOSE8_HOVER},
	{"face",		S3_LAYER_CHF},
	{"face-eye",		S3_LAYER_CHF_EYE},
	{"face-lip",		S3_LAYER_CHF_LIP},
	{"face-fo",		S3_LAYER_CHF_FO},
	{"click",		S3_LAYER_CLICK},
	{"auto",		S3_LAYER_AUTO},
	{"skip",		S3_LAYER_SKIP},
	{"text1",		S3_LAYER_TEXT1},
	{"text2",		S3_LAYER_TEXT2},
	{"text3",		S3_LAYER_TEXT3},
	{"text4",		S3_LAYER_TEXT4},
	{"text5",		S3_LAYER_TEXT5},
	{"text6",		S3_LAYER_TEXT6},
	{"text7",		S3_LAYER_TEXT7},
	{"text8",		S3_LAYER_TEXT8},
	{"gui1",		S3_LAYER_GUI_BTN1},
	{"gui2",		S3_LAYER_GUI_BTN2},
	{"gui3",		S3_LAYER_GUI_BTN3},
	{"gui4",		S3_LAYER_GUI_BTN4},
	{"gui5",		S3_LAYER_GUI_BTN5},
	{"gui6",		S3_LAYER_GUI_BTN6},
	{"gui7",		S3_LAYER_GUI_BTN7},
	{"gui8",		S3_LAYER_GUI_BTN8},
	{"gui9",		S3_LAYER_GUI_BTN9},
	{"gui10",		S3_LAYER_GUI_BTN10},
	{"gui11",		S3_LAYER_GUI_BTN11},
	{"gui12",		S3_LAYER_GUI_BTN12},
	{"gui13",		S3_LAYER_GUI_BTN13},
	{"gui14",		S3_LAYER_GUI_BTN14},
	{"gui15",		S3_LAYER_GUI_BTN15},
	{"gui16",		S3_LAYER_GUI_BTN16},
	{"gui17",		S3_LAYER_GUI_BTN17},
	{"gui18",		S3_LAYER_GUI_BTN18},
	{"gui19",		S3_LAYER_GUI_BTN19},
	{"gui20",		S3_LAYER_GUI_BTN20},
	{"gui21",		S3_LAYER_GUI_BTN21},
	{"gui22",		S3_LAYER_GUI_BTN22},
	{"gui23",		S3_LAYER_GUI_BTN23},
	{"gui24",		S3_LAYER_GUI_BTN24},
	{"gui25",		S3_LAYER_GUI_BTN25},
	{"gui26",		S3_LAYER_GUI_BTN26},
	{"gui27",		S3_LAYER_GUI_BTN27},
	{"gui28",		S3_LAYER_GUI_BTN28},
	{"gui29",		S3_LAYER_GUI_BTN29},
	{"gui30",		S3_LAYER_GUI_BTN30},
	{"gui31",		S3_LAYER_GUI_BTN31},
	{"gui32",		S3_LAYER_GUI_BTN31},
};

/*
 * Registered anime file.
 */
static char *reg_anime_name[S3_REG_ANIME_COUNT];
static char *reg_anime_file[S3_REG_ANIME_COUNT];
static bool reg_anime_layers[S3_REG_ANIME_COUNT][S3_STAGE_LAYERS];

/*
 * Information during anime file loading.
 */
static int cur_seq_layer;	/* Current sequece's layer. */
static uint64_t cur_sw;		/* Current lap time origin. */
static bool *used_layer_tbl;	/* Cuurent detected layer in use. */

/*
 * Forward declaration.
 */
static bool start_sequence(const char *name);
static bool on_key_value(const char *file, int line, const char *key, const char *val);
static int layer_name_to_index(const char *name);
static float calc_pos_x_from(int anime_layer, int index, const char *value);
static float calc_pos_x_to(int anime_layer, int index, const char *value);
static float calc_pos_y_from(int anime_layer, int index, const char *value);
static float calc_pos_y_to(int anime_layer, int index, const char *value);
static bool load_anime_file(const char *file);
static bool update_layer_params(int layer);
static void synthesis_eye_anime(int chpos);

/*
 * Initialize the anime subsystem.
 */
bool
s3i_init_anime(void)
{
	/* Cleanup when DLL is reused. */
	s3i_cleanup_anime();

	return true;
}

/*
 * Cleanup the anime subsystem.
 */
void
s3i_cleanup_anime(void)
{
	int i, j;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		memset(sequence, 0, sizeof(sequence));
		for (j = 0 ; j < SEQUENCE_COUNT; j++) {
			sequence[i][j].from_scale_x = 1.0f;
			sequence[i][j].from_scale_y = 1.0f;
			sequence[i][j].to_scale_x = 1.0f;
			sequence[i][j].to_scale_y = 1.0f;
		}
	}
	memset(context, 0, sizeof(context));

	for (i = 0; i < S3_REG_ANIME_COUNT; i++) {
		if (reg_anime_name[i] != NULL) {
			free(reg_anime_name[i]);
			reg_anime_name[i] = NULL;
		}
		if (reg_anime_file[i] != NULL) {
			free(reg_anime_file[i]);
			reg_anime_file[i] = NULL;
		}
	}
}

/*
 * Load an anime file and start it.
 */
bool
s3_load_anime_from_file(
	const char *fname,
	const char *reg_name,
	bool *used_layer)
{
	bool used_tbl[S3_STAGE_LAYERS];
	int reg_index;

	/* Allocate a reg_index. */
	reg_index = -1;
	if (reg_name != NULL) {
		for (reg_index = 0; reg_index < S3_REG_ANIME_COUNT; reg_index++) {
			if (reg_anime_name[reg_index] == NULL)
				break;
		}
		if (reg_index == S3_REG_ANIME_COUNT) {
			s3_log_tag_error(S3_TR("Too many registered animes."));
			return false;
		}
	}

	/* Load the anime file. */
	cur_seq_layer = -1;
	s3_reset_lap_timer(&cur_sw);
	memset(used_tbl, 0, sizeof(bool) * S3_STAGE_LAYERS);
	used_layer_tbl = used_tbl;
	if (!load_anime_file(fname))
		return false;

	/* Register an anime file for looping. */
	if (reg_index != -1) {
		reg_anime_name[reg_index] = strdup(reg_name);
		if (reg_anime_name[reg_index] == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		reg_anime_file[reg_index] = strdup(fname);
		if (reg_anime_file[reg_index] == NULL) {
			s3_log_out_of_memory();
			return false;
		}
		memcpy(reg_anime_layers[reg_index], used_layer_tbl, sizeof(used_tbl));
	}

	if (used_layer != NULL)
		memcpy(used_layer, used_tbl, sizeof(used_tbl));

	return true;
}

/*
 * Clear the layer anime sequences.
 */
void
s3_clear_layer_anime_sequence(
	int layer)
{
	int i;

	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	if (context[layer].seq_count > 0) {
		struct sequence *s;
		s = &sequence[layer][context[layer].seq_count - 1];
		s3_set_layer_position(layer, (int)s->to_x, (int)s->to_y);
		s3_set_layer_scale(layer, s->to_scale_x, s->to_scale_y);
		s3_set_layer_alpha(layer, (int)s->to_a);
		s3_set_layer_center(layer, (int)s->center_x, (int)s->center_y);
		s3_set_layer_rotate(layer, s->to_rotate * (3.14159265f / 180.0f));
		s3_set_layer_blend(layer, s->blend);
	}

	context[layer].seq_count = 0;
	context[layer].is_running = false;
	context[layer].is_finished = false;
	context[layer].loop_len = 0;
	context[layer].loop_ofs = 0;
	if (context[layer].file != NULL) {
		free(context[layer].file);
		context[layer].file = NULL;
	}

	for (i = 0 ; i < SEQUENCE_COUNT; i++) {
		memset(&sequence[layer][i], 0, sizeof(struct sequence));
		sequence[layer][i].from_scale_x = 1.0f;
		sequence[layer][i].from_scale_y = 1.0f;
		sequence[layer][i].to_scale_x = 1.0f;
		sequence[layer][i].to_scale_y = 1.0f;
	}
}

/*
 * Clear all anime sequences.
 */
void
s3_clear_all_anime_sequence(void)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++)
		s3_clear_layer_anime_sequence(i);
}

/*
 * Begin describing an anime for a layer.
 */
bool
s3_new_anime_sequence(
	int layer)
{
	struct sequence *s;

	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	if (context[layer].seq_count >= SEQUENCE_COUNT)
		return false;

	cur_seq_layer = layer;

	s = &sequence[layer][context[layer].seq_count];
	memset(s, 0, sizeof(struct sequence));
	s->from_scale_x = 1.0f;
	s->from_scale_y = 1.0f;
	s->to_scale_x = 1.0f;
	s->to_scale_y = 1.0f;

	context[layer].seq_count++;
	context[layer].is_running = false;
	context[layer].is_finished = false;
	context[layer].loop_ofs = 0;
	context[layer].loop_len = 0;

	return true;
}

/*
 * Add a float property into an anime sequence.
 */
bool
s3_add_anime_sequence_property_f(
	const char *key,
	float val)
{
	char s[128];

	snprintf(s, sizeof(s), "%f", val);

	if (!on_key_value("", 0, key, s))
		return false;

	return true;
}

/*
 * Add an integer property into an anime sequence.
 */
bool
s3_add_anime_sequence_property_i(
	const char *key,
	int val)
{
	char s[128];

	snprintf(s, sizeof(s), "%d", val);

	if (!on_key_value("", 0, key, s))
		return false;

	return true;
}

/*
 * Start the anime for a layer.
 */
bool
s3_start_layer_anime(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	if (context[layer].seq_count == 0)
		return true;

	s3_reset_lap_timer(&context[layer].sw);
	context[layer].is_running = true;
	context[layer].is_finished = false;
	context[layer].cur_lap = 0;
	context[layer].loop_ofs = 0;
	context[layer].loop_len =
		sequence[layer][context[layer].seq_count - 1].loop ?
		(uint64_t)(sequence[layer][context[layer].seq_count - 1].end_time * 1000.0f) :
		0;

	return true;
}

/*
 * Check if there are running animes.
 */
bool
s3_is_anime_running(void)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (context[i].is_running)
			return true;
	}
	return false;
}

/*
 * Check if there are running animes. (with mask)
 */
bool
s3_is_anime_running_with_layer_mask(
	bool *used_layers)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (!used_layers[i])
			continue;
		if (context[i].is_running)
			return true;
	}
	return false;
}

/*
 * Check if the layer anime is finished.
 */
bool
s3_is_anime_finished_for_layer(
	int layer)
{
	return context[layer].is_finished;
}

/*
 * Update the anime frame.
 */
void
s3_update_anime_frame(void)
{
	int i, last_seq;
	uint64_t lap;

	/* Update the laptime, and detect the anime retirements. */
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (!context[i].is_running)
			continue;
		if (context[i].is_finished)
			continue;

		lap = s3_get_lap_timer_millisec(&context[i].sw);
		if (context[i].loop_len > 0 &&
		    lap > context[i].loop_ofs + context[i].loop_len) {
			lap -= context[i].loop_ofs;
			lap %= context[i].loop_len;
			lap += context[i].loop_ofs;
		}
		context[i].cur_lap = (float)lap / 1000.0f;

		last_seq = context[i].seq_count - 1;
		assert(last_seq >= 0);

		if (context[i].loop_len == 0 &&
		    context[i].cur_lap >= sequence[i][last_seq].end_time) {
			context[i].is_running = false;
			context[i].is_finished = true;
		}
	}

	/* For each layer. */
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		/* Update layer params. */
		update_layer_params(i);
	}
}

/*
 * Update layer params.
 */
static bool update_layer_params(int layer)
{
	struct sequence *s;
	float progress;
	int i, x, y, alpha;
	float scale_x, scale_y, rotate;

	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	/* If no sequence. */
	if (context[layer].seq_count == 0)
		return true;

	/* If already finished. */
	if (context[layer].is_finished &&
	    context[layer].seq_count > 0) {
		s = &sequence[layer][context[layer].seq_count - 1];
		x = (int)s->to_x;
		y = (int)s->to_y;
		alpha = (int)s->to_a;
		rotate = s->to_rotate;
		s3_set_layer_position(layer, (int)s->to_x, (int)s->to_y);
		s3_set_layer_scale(layer, s->to_scale_x, s->to_scale_y);
		s3_set_layer_alpha(layer, alpha);
		s3_set_layer_center(layer, (int)s->center_x, (int)s->center_y);
		s3_set_layer_rotate(layer, rotate * (3.14159265f / 180.0f));
		s3_set_layer_blend(layer, s->blend);
		if (layer == S3_LAYER_MSGBOX && s->hide_special)
			s3_show_msgbox(false);
		if (layer == S3_LAYER_MSGBOX && s->show_special)
			s3_show_msgbox(true);
		if (layer == S3_LAYER_NAMEBOX && s->hide_special)
			s3_show_namebox(false);
		if (layer == S3_LAYER_NAMEBOX && s->show_special)
			s3_show_namebox(true);
		return true;
	}

	/* Interpolate. */
	for (i = 0; i < context[layer].seq_count; i++) {
		s = &sequence[layer][i];
		if (context[layer].cur_lap < s->start_time)
			continue;
		if (i != context[layer].seq_count - 1 &&
		    context[layer].cur_lap > s->end_time)
			continue;

		/* Calculate the progress. */
		progress = (context[layer].cur_lap - s->start_time) /
			(s->end_time - s->start_time);
		if (progress > 1.0f)
			progress = 1.0f;

		/* Caclulate the acceleration. */
		switch (s->accel) {
		case S3_ANIME_ACCEL_UNIFORM:
			break;
		case S3_ANIME_ACCEL_ACCEL:
			progress = progress * progress;
			break;
		case S3_ANIME_ACCEL_DEACCEL:
			progress = sqrtf(progress);
			break;
		case S3_ANIME_ACCEL_SMOOTHSTEP:
			//progress = pow(progress, 2.2f);
			break;
		case S3_ANIME_ACCEL_INVSMOOTHSTEP:
			//progress = 1.0f - pow(progress, 2.2f);
			break;
		default:
			assert(INVALID_ACCEL_TYPE);
			break;
		}

		/* Calculate the params. */
		x = (int)(s->from_x + (s->to_x - s->from_x) * progress);
		y = (int)(s->from_y + (s->to_y - s->from_y) * progress);
		scale_x = s->from_scale_x + (s->to_scale_x - s->from_scale_x) * progress;
		scale_y = s->from_scale_y + (s->to_scale_y - s->from_scale_y) * progress;
		rotate = s->from_rotate + (s->to_rotate - s->from_rotate) * progress;
		if (s->accel == S3_ANIME_ACCEL_SMOOTHSTEP) {
			alpha = (int)(s->from_a + (s->to_a - s->from_a) * sinf((float)M_PI / 2.1f * progress));
			if (alpha > 255)
				alpha = 255;
		} else if (s->accel == S3_ANIME_ACCEL_INVSMOOTHSTEP) {
			alpha = (int)(s->from_a + (s->to_a - s->from_a) * cosf((float)M_PI / 2.1f * progress));
			if (alpha > 255)
				alpha = 255;
		} else {
			alpha = (int)(s->from_a + (s->to_a - s->from_a) * progress);
		}
		s3_set_layer_position(layer, x, y);
		s3_set_layer_center(layer, (int)s->center_x, (int)s->center_y);
		s3_set_layer_alpha(layer, alpha);
		s3_set_layer_scale(layer, scale_x, scale_y);
		s3_set_layer_rotate(layer, rotate * (3.14159265f / 180.0f));
		s3_set_layer_blend(layer, s->blend);
		s3_set_layer_frame(layer, s->frame);
		if (layer == S3_LAYER_MSGBOX && s->hide_special)
			s3_show_msgbox(false);
		if (layer == S3_LAYER_MSGBOX && s->show_special)
			s3_show_msgbox(true);
		if (layer == S3_LAYER_NAMEBOX && s->hide_special)
			s3_show_namebox(false);
		if (layer == S3_LAYER_NAMEBOX && s->show_special)
			s3_show_namebox(true);
		break;
	}

	return true;
}

/*
 * Anime File Loading
 */

/* Called when a sequence is opened. */
static bool
start_sequence(
	const char *name)
{
	UNUSED_PARAMETER(name);
	cur_seq_layer = -1;
	return true;
}

/* Called when a key-value pair is established. */
static bool
on_key_value(
	const char *file,
	int line,
	const char *key,
	const char *val)
{
	struct sequence *s;
	int top;

	/* Layer must be specified first. */
	if (strcmp(key, "layer") == 0) {
		/* Get the layer. */
		cur_seq_layer = layer_name_to_index(val);
		if (cur_seq_layer == -1) {
			s3_log_error(S3_TR("Invalid layer name %s while parsing anime file %s:%d"), val, file, line + 1);
			return false;
		}

		/* Clear sequences if this is the first sequence of the layer. */
		if (used_layer_tbl != NULL) {
			if (!used_layer_tbl[cur_seq_layer])
				s3_clear_layer_anime_sequence(cur_seq_layer);
			used_layer_tbl[cur_seq_layer] = true;
		}

		/* Get the top sequence, and set scales to 1.0f. */
		s = &sequence[cur_seq_layer][context[cur_seq_layer].seq_count];
		s->from_scale_x = 1.0f;
		s->from_scale_y = 1.0f;
		s->to_scale_x = 1.0f;
		s->to_scale_y = 1.0f;

		/* Set the layer context. */
		context[cur_seq_layer].seq_count++;
		context[cur_seq_layer].sw = cur_sw;
		context[cur_seq_layer].is_running = true;
		context[cur_seq_layer].is_finished = false;
		context[cur_seq_layer].loop_len = 0;
		context[cur_seq_layer].loop_ofs = 0;

		return true;
	}
	if (cur_seq_layer == -1) {
		s3_log_error(S3_TR("Layer not specifed while parsing anime file %s:%d"), file, line + 1);
		return false;
	}

	/* Check the sequence length of the layer. */
	top = context[cur_seq_layer].seq_count - 1;
	if (top == SEQUENCE_COUNT) {
		s3_log_error(S3_TR("Sequecen too long while parsing anime file %s:%d"), file, line + 1);
		return false;
	}
	assert(top >= 0);
	assert(top < SEQUENCE_COUNT);

	/* Other keys. */
	s = &sequence[cur_seq_layer][top];
	if (strcmp(key, "start") == 0) {
		s->start_time = (float)atof(val);
	} else if (strcmp(key, "end") == 0) {
		s->end_time = (float)atof(val);
	} else if (strcmp(key, "from-x") == 0) {
		s->from_x = calc_pos_x_from(cur_seq_layer, top, val);
	} else if (strcmp(key, "from-y") == 0) {
		s->from_y = calc_pos_y_from(cur_seq_layer, top, val);
	} else if (strcmp(key, "from-a") == 0) {
		s->from_a = (float)atoi(val);
	} else if (strcmp(key, "from-scale-x") == 0) {
		s->from_scale_x = (float)atof(val);
	} else if (strcmp(key, "from-scale-y") == 0) {
		s->from_scale_y = (float)atof(val);
	} else if (strcmp(key, "to-x") == 0) {
		s->to_x = calc_pos_x_to(cur_seq_layer, top, val);
	} else if (strcmp(key, "to-y") == 0) {
		s->to_y = calc_pos_y_to(cur_seq_layer, top, val);
	} else if (strcmp(key, "to-a") == 0) {
		s->to_a = (float)atoi(val);
	} else if (strcmp(key, "to-scale-x") == 0) {
		s->to_scale_x = (float)atof(val);
	} else if (strcmp(key, "to-scale-y") == 0) {
		s->to_scale_y = (float)atof(val);
	} else if (strcmp(key, "from-rotate") == 0) {
		s->from_rotate = (float)atof(val);
	} else if (strcmp(key, "to-rotate") == 0) {
		s->to_rotate = (float)atof(val);
	} else if (strcmp(key, "center-x") == 0) {
		s->center_x = (float)atof(val);
	} else if (strcmp(key, "center-y") == 0) {
		s->center_y = (float)atof(val);
	} else if (strcmp(key, "blend") == 0) {
		s->blend = atoi(val);
	} else if (strcmp(key, "accel") == 0) {
		if (strcmp(val, "normal") == 0)
			s->accel = S3_ANIME_ACCEL_UNIFORM;
		else if (strcmp(val, "accel") == 0)
			s->accel = S3_ANIME_ACCEL_ACCEL;
		else if (strcmp(val, "brake") == 0)
			s->accel = S3_ANIME_ACCEL_DEACCEL;
		else if (strcmp(val, "smoothstep") == 0)
			s->accel = S3_ANIME_ACCEL_SMOOTHSTEP;
		else if (strcmp(val, "invsmoothstep") == 0)
			s->accel = S3_ANIME_ACCEL_INVSMOOTHSTEP;
		else
			s->accel = atoi(val);
	} else if (strcmp(key, "loop") == 0) {
		s->loop = true;
		context[cur_seq_layer].loop_ofs = (uint64_t)(atof(val) * 1000.0f);
		context[cur_seq_layer].loop_len = (uint64_t)(s->end_time * 1000.0f) - context[cur_seq_layer].loop_ofs;
	} else if (strcmp(key, "frame") == 0) {
		s->frame = atoi(val);
	} else if (strcmp(key, "hide-special") == 0) {
		s->hide_special = true;
	} else if (strcmp(key, "show-special") == 0) {
		s->show_special = true;
	} else {
		s3_log_error(S3_TR("Unkwon key %s while parsing anime file %s:%d"), key, file, line + 1);
		return false;
	}

	return true;
}

/* Convert a layer name to a layer index. */
static int
layer_name_to_index(
	const char *name)
{
	int i;

	for (i = 0;
	     i < (int)(sizeof(layer_name_map) / sizeof(struct layer_name_map));
	     i++) {
		if (strcmp(layer_name_map[i].name, name) == 0)
			return layer_name_map[i].index;
	}
	return -1;
}

/* Calculate a position. (x from) */
static float
calc_pos_x_from(
	int layer,
	int index,
	const char *value)
{
	float ret;

	assert(value != NULL);

	if (value[0] == '+') {
		if (index == 0)
			ret = (float)s3_get_layer_x(layer);
		else
			ret = sequence[layer][index - 1].to_x;
		ret += (float)atoi(value + 1);
	} else {
		ret = (float)atoi(value);
	}

	return ret;
}

/* Caculate a position. (x to) */
static float
calc_pos_x_to(
	int layer,
	int index,
	const char *value)
{
	float ret;

	assert(value != NULL);

	if (value[0] == '+')
		ret = sequence[layer][index].from_x + (float)atoi(value + 1);
	else
		ret = (float)atoi(value);

	return ret;
}

/* Calculate a positoin. (y from) */
static float
calc_pos_y_from(
	int anime_layer,
	int index,
	const char *value)
{
	float ret;

	assert(value != NULL);

	if (value[0] == '+') {
		if (index == 0)
			ret = (float)s3_get_layer_y(anime_layer);
		else
			ret = sequence[anime_layer][index - 1].to_y;
		ret += (float)atoi(value + 1);
	} else {
		ret = (float)atoi(value);
	}

	return ret;
}

/* Calculate a positoin. (y to) */
static float
calc_pos_y_to(
	int anime_layer,
	int index,
	const char *value)
{
	float ret;

	assert(value != NULL);

	if (value[0] == '+')
		ret = sequence[anime_layer][index].from_y + (float)atoi(value + 1);
	else
		ret = (float)atoi(value);

	return ret;
}

/*
 * Unregister a looped anime.
 */
bool
s3_unregister_anime(
	const char *reg_name)
{
	int reg_index, i;

	for (reg_index = 0; reg_index < S3_REG_ANIME_COUNT; i++) {
		if (reg_anime_name[reg_index] == NULL)
			continue;
		if (strcmp(reg_anime_name[reg_index], reg_name) == 0)
			break;
	}
	if (reg_index == S3_REG_ANIME_COUNT) {
		s3_log_tag_error(S3_TR("Invalid registered anime \"%s\"."), reg_name);
		return false;
	}

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (reg_anime_layers[reg_index][i])
			s3_clear_layer_anime_sequence(i);
	}

	free(reg_anime_name[reg_index]);
	free(reg_anime_file[reg_index]);
	reg_anime_name[reg_index] = NULL;
	reg_anime_file[reg_index] = NULL;
	memset(reg_anime_file, 0, sizeof(reg_anime_file));

	return true;
}

/*
 * Get a looped anime name.
 */
const char *
s3_get_reg_anime_name(
	int reg_index)
{
	return reg_anime_name[reg_index];
}

/*
 * Get a looped anime file name.
 */
const char *
s3_get_reg_anime_file_name(
	int reg_index)
{
	return reg_anime_file[reg_index];
}

/*
 * Eye Blinking
 */

/*
 * Load an eye blinking image.
 */
bool
s3_load_eye_image_if_exists(
	int chpos,
	const char *fname)
{
	char eye_fname[1024], *slash, *dot;
	struct s3_image *eye_img;
	int eye_layer;

	/* Firstly, disable the eye layer. */
	eye_layer = s3_chpos_to_eye_layer(chpos);
	s3_set_layer_file_name(eye_layer, NULL);
	s3_set_layer_image(eye_layer, NULL);

	/* In case that we erase the character. */
	if (fname == NULL || strcmp(fname, "none") == 0 || strcmp(fname, U8("消去")) == 0) {
		/* Finish the existing eye anime. */
		s3_clear_layer_anime_sequence(eye_layer);
		return true;
	}

	/* Make an eye file name. (base + "/eye/" + file) */
	slash = strrchr(fname, '/');
	if (slash != NULL) {
		char base[1024], file[1024];

		strcpy(base, fname);
		slash = strrchr(base, '/');
		*slash = '\0';
		strcpy(file, slash + 1);

		strcpy(eye_fname, base);
		strcat(eye_fname, "/eye/");
		strcat(eye_fname, file);
	} else {
		strcpy(eye_fname, "eye/");
		strcat(eye_fname, fname);
	}

	/* Check if the file name includes extension. */
	dot = strstr(eye_fname, ".");
	if (dot != NULL) {
		/* There is an extension. */
		if (!pf_check_file_exists(eye_fname)) {
			/* No such file. */
			return true;
		}
	} else {
		/* There is no extension. */
		do {
			char tmp[1024];

			strcpy(tmp, eye_fname);
			strcat(tmp, ".png");
			if (pf_check_file_exists(tmp)) {
				strcpy(eye_fname, tmp);
				break;
			}
			strcpy(tmp, eye_fname);
			strcat(tmp, ".webp");
			if (pf_check_file_exists(tmp)) {
				strcpy(eye_fname, tmp);
				break;
			}

			/* No such file. */
			return true;
		} while (0);
	}

	/* Load an image. */
	eye_img = s3_create_image_from_file(eye_fname);
	if (eye_img == NULL)
		return false;

	/* Initialize the layer. */
	s3_set_layer_file_name(eye_layer, eye_fname);
	s3_set_layer_image(eye_layer, eye_img);
	s3_set_layer_alpha(eye_layer, 0);
	s3_set_layer_scale(eye_layer, 1.0f, 1.0f);

	/* Make an eye anime. */
	synthesis_eye_anime(chpos);

	return true;
}

/*
 * Reload an eye blinking image.
 */
bool
s3_reload_eye_anime(
	int chpos)
{
	int eye_layer;

	/* If no character. */
	eye_layer = s3_chpos_to_eye_layer(chpos);
	if (s3_get_layer_image(eye_layer) == NULL)
		return true;

	/* Synthesis the eye anime. */
	synthesis_eye_anime(chpos);

	return true;
}

/* Synthesis the eye anime. */
static void
synthesis_eye_anime(
	int chpos)
{
	float ofs_time, base_time;
	int x, y, i, frame_count, repeat_count;
	int base_layer, eye_layer;

	base_layer = s3_chpos_to_layer(chpos);
	eye_layer = s3_chpos_to_eye_layer(chpos);

	if (s3_get_layer_image(base_layer) == NULL)
		return;
	if (s3_get_layer_image(eye_layer) == NULL)
		return;

	/* Get the character position. */
	x = s3_get_layer_x(base_layer);
	y = s3_get_layer_y(base_layer);

	/* Set the eye layer position. */
	s3_set_layer_position(eye_layer, x, y);

	/* Describe the eye anime. */
	frame_count = s3_get_layer_image(eye_layer)->width / s3_get_layer_image(base_layer)->width;
	base_time = conf_character_eyeblink_interval + conf_character_eyeblink_interval * 0.3f * (2.0f * (float)rand() / (float)RAND_MAX - 1.0f);
	ofs_time = base_time;
	s3_clear_layer_anime_sequence(eye_layer);
	s3_new_anime_sequence(eye_layer);
	s3_add_anime_sequence_property_f("start",	0);
	s3_add_anime_sequence_property_f("end",	ofs_time);
	s3_add_anime_sequence_property_i("from-x",	x);
	s3_add_anime_sequence_property_i("from-y",	y);
	s3_add_anime_sequence_property_i("to-x",	x);
	s3_add_anime_sequence_property_i("to-y",	y);
	for (repeat_count = 10; repeat_count > 0; repeat_count--) {
		int blink_times;

		/* Dual blink for p=2/10, single blink for p=8/10. */
		for (blink_times = rand() % 10 >= 2 ? 1 : 2; blink_times > 0; blink_times--) {
			for (i = 0; i < frame_count; i++) {
				s3_new_anime_sequence(eye_layer);
				s3_add_anime_sequence_property_f("start",	ofs_time);
				ofs_time += conf_character_eyeblink_frame * (float)(i + 1);
				s3_add_anime_sequence_property_f("end",	ofs_time);
				s3_add_anime_sequence_property_i("from-x",	x);
				s3_add_anime_sequence_property_i("from-y",	y);
				s3_add_anime_sequence_property_i("from-a",	255);
				s3_add_anime_sequence_property_i("to-x",	x);
				s3_add_anime_sequence_property_i("to-y",	y);
				s3_add_anime_sequence_property_i("to-a",	255);
				s3_add_anime_sequence_property_i("frame",	i);
			}
		}

		/* Open eyes. */
		if (repeat_count != 1) {
			s3_new_anime_sequence(eye_layer);
			s3_add_anime_sequence_property_f("start",	ofs_time);
			ofs_time += base_time;
			s3_add_anime_sequence_property_f("end",	ofs_time);
			s3_add_anime_sequence_property_i("from-x",	x);
			s3_add_anime_sequence_property_i("from-y",	y);
			s3_add_anime_sequence_property_i("to-x",	x);
			s3_add_anime_sequence_property_i("to-y",	y);
			s3_add_anime_sequence_property_i("frame",	0);
		}
	}
	s3_add_anime_sequence_property_i("loop", 0);

	/* Start the eye anime. */
	s3_start_layer_anime(eye_layer);
}

/*
 * Lip Synchronization
 */

/*
 * Load a lip synchronization image.
 */
bool
s3_load_lip_image_if_exists(
	int chpos,
	const char *fname)
{
	char lip_fname[1024], *slash, *dot;
	struct s3_image *lip_img;
	int lip_layer;

	/* Firstly, disable the lip layer. */
	lip_layer = s3_chpos_to_lip_layer(chpos);
	s3_set_layer_file_name(lip_layer, NULL);
	s3_set_layer_image(lip_layer, NULL);

	/* In case that we erase the character. */
	if (fname == NULL || strcmp(fname, "none") == 0 || strcmp(fname, U8("消去")) == 0) {
		/* Finish the existing lip anime. */
		s3_clear_layer_anime_sequence(lip_layer);
		return true;
	}

	/* Make a lip file name. (base + "/lip/" + file) */
	slash = strrchr(fname, '/');
	if (slash != NULL) {
		char base[1024], file[1024];

		strcpy(base, fname);
		slash = strrchr(base, '/');
		*slash = '\0';
		strcpy(file, slash + 1);

		strcpy(lip_fname, base);
		strcat(lip_fname, "/lip/");
		strcat(lip_fname, file);
	} else {
		strcpy(lip_fname, "lip/");
		strcat(lip_fname, fname);
	}

	/* Check if the file name includes extension. */
	dot = strstr(lip_fname, ".");
	if (dot != NULL) {
		/* There is an extension. */
		if (!pf_check_file_exists(lip_fname)) {
			/* No such file. */
			return true;
		}
	} else {
		/* There is no extension. */
		do {
			char tmp[1024];

			strcpy(tmp, lip_fname);
			strcat(tmp, ".png");
			if (pf_check_file_exists(tmp)) {
				strcpy(lip_fname, tmp);
				break;
			}
			strcpy(tmp, lip_fname);
			strcat(tmp, ".webp");
			if (pf_check_file_exists(tmp)) {
				strcpy(lip_fname, tmp);
				break;
			}

			/* No such file. */
			return true;
		} while (0);
	}

	/* Load an image. */
	lip_img = s3_create_image_from_file(lip_fname);
	if (lip_img == NULL)
		return false;

	/* Set the layer. */
	s3_set_layer_file_name(lip_layer, lip_fname);
	s3_set_layer_image(lip_layer, lip_img);
	s3_set_layer_alpha(lip_layer, 0);
	s3_set_layer_scale(lip_layer, 1.0f, 1.0f);

	return true;
}

/*
 * Run a lip synchronization anime.
 */
void
s3_run_lip_anime(
	int chpos,
	const char *msg)
{
	float ofs_time, base_time;
	int i, x, y, base_layer, lip_layer, frame_count, word_count;
	int WORD_COUNT;

	base_layer = s3_chpos_to_layer(chpos);
	lip_layer = s3_chpos_to_lip_layer(chpos);

	if (s3_get_layer_image(base_layer) == NULL)
		return;
	if (s3_get_layer_image(lip_layer) == NULL)
		return;

	/* Get the character position. */
	x = s3_get_layer_x(base_layer);
	y = s3_get_layer_y(base_layer);

	/* Set the lip layer position. */
	s3_set_layer_position(lip_layer, x, y);

	/* Determine how many characters to lip-sync per characters. */
	WORD_COUNT = conf_character_lipsync_chars == 0 ? 3 : conf_character_lipsync_chars;

	/* Describe the lip anime. */
	frame_count = s3_get_layer_image(lip_layer)->width / s3_get_layer_image(base_layer)->width;
	base_time = conf_character_lipsync_frame == 0 ? 0.02f : conf_character_lipsync_frame;
	ofs_time = 0;
	word_count = WORD_COUNT;
	s3_clear_layer_anime_sequence(lip_layer);
	while (msg) {
		uint32_t wc;
		int n;

		n = s3_utf8_to_utf32(msg, &wc);
		if (n <= 0)
			break;
		msg += n;

		if (wc == U32C('、', 0x3001) || wc == U32C('。', 0x3002)) {
			ofs_time += base_time * 10;
			word_count = WORD_COUNT;
			continue;
		}

		if (wc == U32C('、', 0x3001) || wc == U32C('。', 0x3002) ||
		    wc == U32C('！', 0xff01) || wc == U32C('？', 0xff1f) ||
		    wc == U32C('・', 0x30fb) || wc == U32C('…', 0x2026) ||
		    wc == U32C('―', 0x2015)) {
			ofs_time += base_time * 20;
			word_count = WORD_COUNT;
			continue;
		}

		/* Do lip-sync per (WORD_COUNT) characters. */
		if (word_count == 0)
			word_count = WORD_COUNT;
		if (word_count-- != WORD_COUNT)
			continue;

		/* Display lip sync in the right order. */
		for (i = 0; i < frame_count; i++) {
			s3_new_anime_sequence(lip_layer);
			s3_add_anime_sequence_property_f("start",	ofs_time);
			ofs_time += base_time;
			s3_add_anime_sequence_property_f("end",	ofs_time);
			s3_add_anime_sequence_property_i("from-x",	x);
			s3_add_anime_sequence_property_i("from-y",	y);
			s3_add_anime_sequence_property_i("from-a",	255);
			s3_add_anime_sequence_property_i("to-x",	x);
			s3_add_anime_sequence_property_i("to-y",	y);
			s3_add_anime_sequence_property_i("to-a",	255);
			s3_add_anime_sequence_property_i("frame",	i);
			ofs_time += base_time;
		}

		/* Display lip sync in the reversed order if there are 3+ frames. */
		if (frame_count > 2) {
			for (i = frame_count - 1; i >= 0; i--) {
				s3_new_anime_sequence(lip_layer);
				s3_add_anime_sequence_property_f("start",	ofs_time);
				ofs_time += base_time;
				s3_add_anime_sequence_property_f("end",	ofs_time);
				s3_add_anime_sequence_property_i("from-x",	x);
				s3_add_anime_sequence_property_i("from-y",	y);
				s3_add_anime_sequence_property_i("from-a",	255);
				s3_add_anime_sequence_property_i("to-x",	x);
				s3_add_anime_sequence_property_i("to-y",	y);
				s3_add_anime_sequence_property_i("to-a",	255);
				s3_add_anime_sequence_property_i("frame",	i);
				ofs_time += base_time;
			}
		}
	}

	/* Close lips. */
	s3_new_anime_sequence(lip_layer);
	s3_add_anime_sequence_property_f("start",	ofs_time);
	ofs_time += base_time;
	s3_add_anime_sequence_property_f("end",	ofs_time);
	s3_add_anime_sequence_property_i("from-x",	x);
	s3_add_anime_sequence_property_i("from-y",	y);
	s3_add_anime_sequence_property_i("from-a",	0);
	s3_add_anime_sequence_property_i("to-x",	x);
	s3_add_anime_sequence_property_i("to-y",	y);
	s3_add_anime_sequence_property_i("to-a",	0);
	s3_add_anime_sequence_property_i("frame",	0);

	/* Start the lip anime. */
	s3_start_layer_anime(lip_layer);
}

/*
 * Stop a lip synchronization anime.
 */
void
s3_stop_lip_anime(
	int chpos)
{
	int lip_layer;

	lip_layer = s3_chpos_to_lip_layer(chpos);

	s3_clear_layer_anime_sequence(lip_layer);
}

/*
 * Anime File Parse
 */

/* Load an anime file. */
static bool load_anime_file(const char *file)
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

	char *file_buf;
	size_t file_size;
	char word[256], key[256];
	size_t pos;
	int st, len, line;
	char c;
	bool is_comment;

	assert(file != NULL);

	/* Get the file content. */
	if (!pf_read_file_content(file, &file_buf, &file_size))
		return false;

	/* Fill comments by spaces. */
	is_comment = false;
	for (pos = 0; pos < file_size; pos++) {
		if (!is_comment) {
			if (file_buf[pos] == '#') {
				file_buf[pos] = ' ';
				is_comment = true;
			}
		} else {
			if (file_buf[pos] == '\n')
				is_comment = false;
			else
				file_buf[pos] = ' ';
		}
	}

	/* Parse the file. */
	st = ST_SCOPE;
	line = 0;
	len = 0;
	pos = 0;
	while (pos < file_size) {
		/* 1文字読み込む */
		c = file_buf[pos++];

		/* ステートに応じて解釈する */
		switch (st) {
		case ST_SCOPE:
			if (len == 0) {
				if (c == ' ' || c == '\t' || c == '\r' ||
				    c == '\n') {
					st = ST_SCOPE;
					break;
				}
				if (c == ':' || c == '{' || c == '}') {
					s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
					st = ST_ERROR;
					break;
				}
			}
			if (c == '}' || c == ':') {
				s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
			    c == '{') {
				assert(len > 0);
				word[len] = '\0';
				if (!start_sequence(word)) {
					st = ST_ERROR;
					break;
				}
				if (c == '{')
					st = ST_KEY;
				else
					st = ST_OPEN;
				len = 0;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file %s:%d"), file, line + 1);
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
			s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
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
					s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
					st = ST_ERROR;
					break;
				}
				if (c == '}') {
					st = ST_SCOPE;
					break;
				}
			}
			if (c == '{' || c == '}') {
				s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
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
				s3_log_error(S3_TR("Too long word while parsing anime file %s:%d"), file, line + 1);
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
			s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
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
				s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
			    c == ';' || c == '}') {
				word[len] = '\0';
				if (!on_key_value(file, line, key, word)) {
					st = ST_ERROR;
					break;
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
				s3_log_error(S3_TR("Too long word while parsing anime file %s:%d"), file, line + 1);
				st = ST_ERROR;
				break;
			}
			word[len++] = c;
			st = ST_VALUE;
			break;
		case ST_VALUE_DQ:
			if (c == '\"') {
				word[len] = '\0';
				if (!on_key_value(file, line, key, word)) {
					st = ST_ERROR;
					break;
				}
				st = ST_SEMICOLON;
				len = 0;
				break;
			}
			if (c == '\r' || c == '\n') {
				s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
				st = ST_ERROR;
				break;
			}
			if (len == sizeof(word) - 1) {
				s3_log_error(S3_TR("Too long word while parsing anime file %s:%d"), file, line + 1);
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
			s3_log_error(S3_TR("Invalid character %c while parsing anime file %s:%d"), c, file, line + 1);
			st = ST_ERROR;
			break;
		}

		/* On error. */
		if (st == ST_ERROR)
			break;

		/* FIXME: support '\r' only line feed. */
		if (c == '\n')
			line++;
	}

	/* On error. */
	if (st != ST_SCOPE || len > 0) {
		s3_log_error(S3_TR("Invalid EOF while parsing anime file %s"), file);
	}

	/* Free the buffer. */
	free(file_buf);

	return st != ST_ERROR;
}
