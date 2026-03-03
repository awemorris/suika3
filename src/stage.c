/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Stage Subsystem
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
#include "conf.h"
#include "stage.h"
#include "image.h"
#include "text.h"
#include "anime.h"
#include "gui.h"

#include <playfield/playfield.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/*
 * False Assertions
 */
#define BAD_POSITION		(0)
#define INVALID_FADE_METHOD	(0)

/*
 * Stage Operation Mode
 */

static int stage_mode;

enum stage_mode {
	STAGE_MODE_IDLE,
	STAGE_MODE_FADE,
};

/*
 * Stage Images
 */

/* Layer images. */
static struct s3_image *layer_image[S3_STAGE_LAYERS];

/* Image for the message box background. */
static struct s3_image *msgbox_image;

/* Image for the name box background. */
static struct s3_image *namebox_image;

/* Images for the click animation. */
static struct s3_image *click_image[S3_CLICK_FRAMES];

/* Image for the choose box (idle). */
static struct s3_image *choose_idle_image[S3_CHOOSEBOX_COUNT];

/* Image for the choose box (hover). */
static struct s3_image *choose_hover_image[S3_CHOOSEBOX_COUNT];

/* Image for the system button (idle). */
static struct s3_image *sysbtn_idle_image;

/* Image for the system button (hover). */
static struct s3_image *sysbtn_hover_image;

/* Image for the save slot "NEW". */
static struct s3_image *savenew_image;

/* Image for the save data thumbnail. */
static struct s3_image *thumb_image;

/* Rule image. */
static struct s3_image *fade_rule_img;

/* Images for Kira Kira Effect. */
static struct s3_image *kirakira_image[S3_KIRAKIRA_FRAMES];

/*
 * Layer Visibility
 */

/* Whether to show the message box. */
static bool is_msgbox_visible;

/* Whether to show the name box. */
static bool is_namebox_visible;

/* Whether to show the choose box. */
static bool is_choosebox_idle_visible[S3_CHOOSEBOX_COUNT];
static bool is_choosebox_hover_visible[S3_CHOOSEBOX_COUNT];

/* Whether to show the click animation. */
static bool is_click_visible;

/* Whether to show the auto mode bannder. */
static bool is_auto_visible;

/* Whether to show the skip mode banner. */
static bool is_skip_visible;


/*
 * Layer Properties
 */

/* X positions for the layers. */
static int layer_x[S3_STAGE_LAYERS];

/* Y positions for the layers. */
static int layer_y[S3_STAGE_LAYERS];

/* Alpha values for the layers. */
static int layer_alpha[S3_STAGE_LAYERS];

/* Blending modes for the layers. */
static int layer_blend[S3_STAGE_LAYERS];

/* X scales for the layers. */
static float layer_scale_x[S3_STAGE_LAYERS];

/* Y scales for the layers. */
static float layer_scale_y[S3_STAGE_LAYERS];

/* X centers for the layers. */
static int layer_center_x[S3_STAGE_LAYERS];

/* Y centers for the layers. */
static int layer_center_y[S3_STAGE_LAYERS];

/* Rotations for the layers in radian. */
static float layer_rotate[S3_STAGE_LAYERS];

/* Dimming state for the layers. */
static float layer_dim[S3_STAGE_LAYERS];

/* File names for the layers. */
static char *layer_file_name[S3_STAGE_LAYERS];

/* Frame index for the eye and lips layers. */
static int layer_frame[S3_STAGE_LAYERS];

/* Is layer fading? */
static int layer_fading[S3_STAGE_LAYERS];

/*
 * Text Layers
 */

/* Strings for the text layers.  */
static char *layer_text[S3_STAGE_LAYERS];

/*
 * Non-Speaker Dimming
 */

/* Character name indices. */
static int ch_name_mapping[S3_CH_BASIC_LAYERS] = {-1, -1, -1, -1, -1, -1};

/* Speaker character name index. */
static int ch_talking = -1;

/*
 * Fading Mode
 */

/* Fading method. */
static int fade_method;

/* Fading progress. */
static float fade_progress;

/* Offsets for the shake command. */
static int shake_offset_x;
static int shake_offset_y;

/*
 * Kira Kira Effect
 */

/* Position to show Kira Kira Effect. */
static int kirakira_x;
static int kirakira_y;

/* Start time of the last Kira Kira Effect. */
static uint64_t sw_kirakira;

/*
 * Forward Declarations
 */
static bool setup_kirakira(void);
static bool setup_savenew(void);
static bool setup_thumb(void);
static void restore_text_layers(void);
static bool create_fade_layer_images(void);
static void destroy_layer(int layer);
static void render_layer(int layer);

/*
 * Initialization
 */

/*
 * Initialize the stage subsystem.
 */
bool
s3i_init_stage(void)
{
	int i;

	/* Cleanup for when our DLL is reused. */
	s3i_cleanup_stage();

	/* Load the stage images. */
	if (!s3_reload_stage_images())
		return false;

	/* Setup the image for the save data thumbnail. */
	if (!setup_thumb())
		return false;

	/* Set the initial values. */
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		layer_scale_x[i] = 1.0f;
		layer_scale_y[i] = 1.0f;
		layer_center_x[i] = 0;
		layer_center_y[i] = 0;
		layer_rotate[i] = 0.0f;
		layer_alpha[i] = 255;
	}

	return true;
}

/*
 * Reload the stage images by the config.
 */
bool
s3_reload_stage_images(void)
{
	/* Setup the name box. */
	if (!s3i_setup_namebox())
		return false;

	/* Setup the message box. */
	if (!s3i_setup_msgbox())
		return false;

	/* Setup the click animation. */
	if (!s3i_setup_click())
		return false;

	/* Setup the choose boxes. */
	if (!s3i_setup_choose(false, -1))
		return false;

	/* Setup the system button. */
	if (!s3i_setup_sysbtn())
		return false;

	/* Setup the auto and skip modes banners. */
	if (!s3i_setup_banners())
		return false;

	/* Setup Kira Kira Effect. */
	if (!setup_kirakira())
		return false;

	/* Setup the "NEW" image for the save lots. */
	if (!setup_savenew())
		return false;

	/* Restore the text layers. */
	restore_text_layers();

	return true;
}

/*
 * Reload the stage positions by the config.
 */
void
s3_reload_stage_positions(void)
{
	layer_x[S3_LAYER_MSGBOX] = conf_msgbox_x;
	layer_y[S3_LAYER_MSGBOX] = conf_msgbox_y;

	layer_x[S3_LAYER_NAMEBOX] = conf_namebox_x;
	layer_y[S3_LAYER_NAMEBOX] = conf_namebox_y;

	layer_x[S3_LAYER_CLICK] = conf_click_x;
	layer_y[S3_LAYER_CLICK] = conf_click_y;

	layer_x[S3_LAYER_AUTO] = conf_automode_x;
	layer_y[S3_LAYER_AUTO] = conf_automode_y;

	layer_x[S3_LAYER_SKIP] = conf_skipmode_x;
	layer_y[S3_LAYER_SKIP] = conf_skipmode_y;
}

/*
 * Setup the name box.
 */
bool
s3i_setup_namebox(void)
{
	is_namebox_visible = false;

	/* Destroy when reinitialized. */
	if (namebox_image != NULL) {
		s3_destroy_image(namebox_image);
		namebox_image = NULL;
	}
	if (layer_image[S3_LAYER_NAMEBOX] != NULL) {
		s3_destroy_image(layer_image[S3_LAYER_NAMEBOX]);
		layer_image[S3_LAYER_NAMEBOX] = NULL;
	}

	/* Load the name box image. */
	namebox_image = s3_create_image_from_file(conf_namebox_image);
	if (namebox_image == NULL)
		return false;

	/* Create a name box layer image. */
	layer_image[S3_LAYER_NAMEBOX] = s3_create_image(namebox_image->width, namebox_image->height);
	if (layer_image[S3_LAYER_NAMEBOX] == NULL)
		return false;

	/* Layout the name box layer. */
	layer_x[S3_LAYER_NAMEBOX] = conf_namebox_x;
	layer_y[S3_LAYER_NAMEBOX] = conf_namebox_y;

	/* Transfer the name box image to the name box layer. */
	s3_fill_namebox();

	return true;
}

/*
 * Setup the message box.
 */
bool
s3i_setup_msgbox(void)
{
	is_msgbox_visible = false;

	/* Destroy when reinitialized. */
	if (msgbox_image != NULL) {
		s3_destroy_image(msgbox_image);
		msgbox_image = NULL;
	}
	if (layer_image[S3_LAYER_MSGBOX] != NULL) {
		s3_destroy_image(layer_image[S3_LAYER_MSGBOX]);
		layer_image[S3_LAYER_MSGBOX] = NULL;
	}

	/* Load the message box image. */
	msgbox_image = s3_create_image_from_file(conf_msgbox_image);
	if (msgbox_image == NULL)
		return false;

	/* Create a message box layer image. */
	layer_image[S3_LAYER_MSGBOX] = s3_create_image(msgbox_image->width, msgbox_image->height);
	if (layer_image[S3_LAYER_MSGBOX] == NULL)
		return false;

	/* Layout the message box layer. */
	layer_x[S3_LAYER_MSGBOX] = conf_msgbox_x;
	layer_y[S3_LAYER_MSGBOX] = conf_msgbox_y;

	/* Transfer the message box image to the message box layer. */
	s3_fill_msgbox();

	return true;
}

/*
 * Setup the click animation.
 */
bool
s3i_setup_click(void)
{
	int i;

	is_click_visible = false;

	/* Destroy when reinitialized. */
	for (i = 0; i < S3_CLICK_FRAMES; i++) {
		if (click_image[i] != NULL) {
			s3_destroy_image(click_image[i]);
			click_image[i] = NULL;
		}
	}

	/* Load the click animation images. */
	for (i = 0; i < conf_click_frames; i++) {
		if (conf_click_image[i] != NULL) {
			/* Load the image if the file name is specified. */
			click_image[i] = s3_create_image_from_file(conf_click_image[i]);
			if (click_image[i] == NULL)
				return false;
		} else {
			/* Otherwise, create a transparent iamge. */
			click_image[i] = s3_create_image(1, 1);
			if (click_image[i] == NULL)
				return false;
		}
	}

	/* Layout the click animation layer. */
	layer_x[S3_LAYER_CLICK] = conf_click_x;
	layer_y[S3_LAYER_CLICK] = conf_click_y;

	/* For now, let the layer point to NULL. */
	layer_image[S3_LAYER_CLICK] = NULL;

	return true;
}

/*
 * Setup the choose boxes.
 */
bool
s3i_setup_choose(bool is_hover, int index)
{
	int i;

	/* Destroy when reinitialized. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i ++) {
		if (index == -1 || i == index) {
			if (choose_idle_image[i] != NULL) {
				s3_destroy_image(choose_idle_image[i]);
				choose_idle_image[i] = NULL;
			}
			if (choose_hover_image[i] != NULL) {
				s3_destroy_image(choose_hover_image[i]);
				choose_hover_image[i] = NULL;
			}
		}

		destroy_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i));
		destroy_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1);
	}

	/* Load the images. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i ++) {
		if (index != -1 && i != index)
			continue;

		assert(conf_choose_idle[i] != NULL);
		assert(conf_choose_hover[i] != NULL);

		/* Load the idle image. */
		if (index == -1 || !is_hover) {
			choose_idle_image[i] = s3_create_image_from_file(conf_choose_idle[i]);
			if (choose_idle_image[i] == NULL)
				return false;
		}

		/* Load the hover image. */
		if (index == -1 || is_hover) {
			choose_hover_image[i] = s3_create_image_from_file(conf_choose_idle[i]);
			if (choose_hover_image[i] == NULL)
				return false;
		}
	}

	/* Create the layer images. */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		layer_image[S3_LAYER_CHOOSE1_IDLE + (2 * i)] = s3_create_image(choose_idle_image[i]->width, choose_idle_image[i]->height);
		if (layer_image[S3_LAYER_CHOOSE1_IDLE + (2 * i)] == NULL)
			return false;

		layer_image[S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1] = s3_create_image(choose_hover_image[i]->width, choose_hover_image[i]->height);
		if (layer_image[S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1] == NULL)
			return false;
	}

	return true;
}

/*
 * Setup the system button.
 */
bool
s3i_setup_sysbtn(void)
{
	/* Destroy when reinitialized. */
	if (sysbtn_idle_image != NULL) {
		s3_destroy_image(sysbtn_idle_image);
		sysbtn_idle_image = NULL;
	}
	if (sysbtn_hover_image != NULL) {
		s3_destroy_image(sysbtn_hover_image);
		sysbtn_hover_image = NULL;
	}

	/* Load the sysbtn idle image. */
	sysbtn_idle_image = s3_create_image_from_file(conf_sysbtn_idle);
	if (sysbtn_idle_image == NULL)
		return false;

	/* Load the sysbtn hover image. */
	sysbtn_hover_image = s3_create_image_from_file(conf_sysbtn_hover);
	if (sysbtn_hover_image == NULL)
		return false;

	return true;
}

/*
 * Setup the banners.
 */
bool
s3i_setup_banners(void)
{
	is_auto_visible = false;
	is_skip_visible = false;

	/* Destroy when reinitialized. */
	if (layer_image[S3_LAYER_AUTO] != NULL) {
		s3_destroy_image(layer_image[S3_LAYER_AUTO]);
		layer_image[S3_LAYER_AUTO] = NULL;
	}
	if (layer_image[S3_LAYER_SKIP] != NULL) {
		s3_destroy_image(layer_image[S3_LAYER_SKIP]);
		layer_image[S3_LAYER_SKIP] = NULL;
	}

	/* Load the auto mode banner image. */
	layer_image[S3_LAYER_AUTO] = s3_create_image_from_file(conf_automode_image);
	if (layer_image[S3_LAYER_AUTO] == NULL)
		return false;

	layer_x[S3_LAYER_AUTO] = conf_automode_x;
	layer_y[S3_LAYER_AUTO] = conf_automode_y;

	/* Load the skip mode banner image. */
	layer_image[S3_LAYER_SKIP] = s3_create_image_from_file(conf_skipmode_image);
	if (layer_image[S3_LAYER_SKIP] == NULL)
		return false;

	layer_x[S3_LAYER_SKIP] = conf_skipmode_x;
	layer_y[S3_LAYER_SKIP] = conf_skipmode_y;

	return true;
}

/* Setup Kira Kira Effect images. */
static bool
setup_kirakira(void)
{
	int i;

	/* Destroy when reinitialized. */
	for (i = 0; i < S3_KIRAKIRA_FRAMES; i++) {
		if (kirakira_image[i] != NULL) {
			s3_destroy_image(kirakira_image[i]);
			kirakira_image[i] = NULL;
		}
	}

	/* Load the images. */
	for (i = 0; i < S3_KIRAKIRA_FRAMES; i++) {
		if (conf_kirakira_image[i] == NULL)
			continue;
		kirakira_image[i] = s3_create_image_from_file(conf_kirakira_image[i]);
		if (kirakira_image[i] == NULL)
			return false;
	}

	return true;
}

/* Setup the save data thumbnail. */
static bool
setup_thumb(void)
{
	/* Destroy when reinitialized. */
	if (thumb_image != NULL) {
		s3_destroy_image(thumb_image);
		thumb_image = NULL;
	}

	/* Adjust the config values. */
	if (conf_save_thumb_width <= 0)
		conf_save_thumb_width = 1;
	if (conf_save_thumb_height <= 0)
		conf_save_thumb_height = 1;

	/* Create an image. */
	thumb_image = s3_create_image(conf_save_thumb_width,
				      conf_save_thumb_height);
	if (thumb_image == NULL)
		return false;

	return true;
}

/* Setup the save slot "NEW" image. */
static bool
setup_savenew(void)
{
	/* Destroy when reinitialized. */
	if (savenew_image != NULL) {
		s3_destroy_image(savenew_image);
		savenew_image = NULL;
	}

	/* If there is no config. */
	if (conf_save_new_image == NULL)
		return true;

	/* Load the image. */
	savenew_image = s3_create_image_from_file(conf_save_new_image);
	if (savenew_image == NULL)
		return false;

	return true;
}

/* Restore the text layer characters. */
static void
restore_text_layers(void)
{
	struct s3_draw_msg_context context;
	s3_pixel_t color, outline_color;
	int i, total_chars;

	/* Load the default colors. */
	color = s3_make_pixel(0xff,
			      conf_msgbox_font_r,
			      conf_msgbox_font_g,
			      conf_msgbox_font_b);
	outline_color = s3_make_pixel(0xff,
				      conf_msgbox_font_outline_r,
				      conf_msgbox_font_outline_g,
				      conf_msgbox_font_outline_b);

	for (i = S3_LAYER_TEXT1; i <= S3_LAYER_TEXT8; i++) {
		if (layer_text[i] == NULL)
			continue;

		/* Draw. */
		s3_construct_draw_msg_context(
			&context,
			layer_image[i],
			layer_text[i],
			conf_msgbox_font_select,
			conf_msgbox_font_size,
			conf_msgbox_font_size,
			conf_msgbox_font_ruby,
			conf_msgbox_font_outline_width,
			0,	/* pen_x */
			0,	/* pen_y */
			layer_image[i]->width,
			layer_image[i]->height,
			0,	/* left_margin */
			0,	/* right_margin */
			0,	/* top_margin */
			0,	/* bottom_margin */
			0,	/* line_margin */
			conf_msgbox_margin_char,
			color,
			outline_color,
			0,	/* bg_color */
			false,	/* fill_bg */
			false,	/* is_dimming */
			false,	/* ignore_linefeed */
			false,	/* ignore_font */
			false,	/* ignore_outline */
			false,	/* ignore_color */
			false,	/* ignore_size */
			false,	/* ignore_position */
			false,	/* ignore_ruby */
			true,	/* ignore_wait */
			NULL,	/* inline_wait_hook */
			false);	/* use_tategaki */
		total_chars = s3_count_chars_common(&context, NULL);
		s3_draw_msg_common(&context, total_chars);
	}
}

/*
 * Cleanup the stage subsystem.
 */
void
s3i_cleanup_stage(void)
{
	int i;

	stage_mode = STAGE_MODE_IDLE;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (i == S3_LAYER_CLICK)
			layer_image[i] = NULL;
		else
			destroy_layer(i);
	}
	for (i = 0; i < S3_CLICK_FRAMES; i++) {
		if (click_image[i] != NULL) {
			s3_destroy_image(click_image[i]);
			click_image[i] = NULL;
		}
	}
	if (msgbox_image != NULL) {
		s3_destroy_image(msgbox_image);
		msgbox_image = NULL;
	}
	if (namebox_image != NULL) {
		s3_destroy_image(namebox_image);
		namebox_image = NULL;
	}
	for (i = 0; i < 8; i++) {
		if (choose_idle_image[i] != NULL) {
			s3_destroy_image(choose_idle_image[i]);
			choose_idle_image[i] = NULL;
		}
		if (choose_hover_image[i] != NULL) {
			s3_destroy_image(choose_hover_image[i]);
			choose_hover_image[i] = NULL;
		}
	}
	if (sysbtn_idle_image != NULL) {
		s3_destroy_image(sysbtn_idle_image);
		sysbtn_idle_image = NULL;
	}
	if (sysbtn_hover_image != NULL) {
		s3_destroy_image(sysbtn_hover_image);
		sysbtn_hover_image = NULL;
	}
	if (thumb_image != NULL) {
		s3_destroy_image(thumb_image);
		thumb_image = NULL;
	}
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (layer_file_name[i] != NULL) {
			free(layer_file_name[i]);
			layer_file_name[i] = NULL;
		}
	}
	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (layer_file_name[i] != NULL) {
			free(layer_file_name[i]);
			layer_file_name[i] = NULL;
		}
	}
	if (fade_rule_img != NULL) {
		s3_destroy_image(fade_rule_img);
		fade_rule_img = NULL;
	}
}

/*
 * Destroy the layer image.
 */
static void
destroy_layer(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	if (layer_image[layer] != NULL) {
		s3_destroy_image(layer_image[layer]);
		layer_image[layer] = NULL;
	}

	layer_x[layer] = 0;
	layer_y[layer] = 0;
	layer_alpha[layer] = 255;
	layer_scale_x[layer] = 1.0f;
	layer_scale_y[layer] = 1.0f;
	layer_center_x[layer] = 0;
	layer_center_y[layer] = 0;
	layer_rotate[layer] = 0.0f;

	if (layer != S3_LAYER_CLICK &&
	    layer != S3_LAYER_MSGBOX &&
	    layer != S3_LAYER_NAMEBOX &&
	    layer != S3_LAYER_AUTO &&
	    layer != S3_LAYER_SKIP)
		s3_set_layer_file_name(layer, NULL);
}

/*
 * Basic Functionality
 */

/*
 * Get the layer x position.
 */
int
s3_get_layer_x(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_x[layer];
}

/*
 * Get the layer y position.
 */
int
s3_get_layer_y(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_y[layer];
}

/*
 * Set the layer position.
 */
void
s3_set_layer_position(
	int layer,
	int x,
	int y)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	layer_x[layer] = x;
	layer_y[layer] = y;

	switch (layer) {
	case S3_LAYER_CLICK: return;
	case S3_LAYER_AUTO: return;
	case S3_LAYER_SKIP: return;
	default: break;
	}
}

/*
 * Get the layer X scale.
 */
float
s3_get_layer_scale_x(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_scale_x[layer];
}

/*
 * Get the layer X scale.
 */
float
s3_get_layer_scale_y(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_scale_y[layer];
}

/*
 * Sets the layer scale.
 */
void
s3_set_layer_scale(
	int layer,
	float scale_x,
	float scale_y)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	if (scale_x == 0)
		s3_log_info("warning: scale_x = 0");
	if (scale_y == 0)
		s3_log_info("warning: scale_y = 0");

	layer_scale_x[layer] = scale_x;
	layer_scale_y[layer] = scale_y;
}

/*
 * Get the layer center X position.
 */
int
s3_get_layer_center_x(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_center_x[layer];
}

/*
 * Get the layer center Y position.
 */
int
s3_get_layer_center_y(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_center_y[layer];
}

/*
 * Set the layer center position.
 */
void
s3_set_layer_center(
	int layer,
	int center_x,
	int center_y)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	layer_center_x[layer] = center_x;
	layer_center_y[layer] = center_y;
}

/*
 * Get the layer rotate
 */
float
s3_get_layer_rotate(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_rotate[layer];
}

/*
 * Set the layer center position.
 */
void
s3_set_layer_rotate(
	int layer,
	float rot)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	layer_rotate[layer] = rot;
}

/*
 * Get the layer dim state.
 */
bool
s3_get_layer_dim(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	return layer_dim[layer];
}

/*
 * Set the layer dim state.
 */
void
s3_set_layer_dim(
	int layer,
	bool dim)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	layer_dim[layer] = dim;
}


/*
 * Get the layer image width.
 */
int
s3_get_layer_width(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	assert(layer_image[layer] != NULL);
	return layer_image[layer]->width;
}

/*
 * Get the layer image height.
 */
int
s3_get_layer_height(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	assert(layer_image[layer] != NULL);
	return layer_image[layer]->height;
}

/*
 * Get the layer alpha.
 */
int
s3_get_layer_alpha(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_alpha[layer];
}

/*
 * Set the layer alpha.
 */
void
s3_set_layer_alpha(
	int layer,
	int alpha)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	layer_alpha[layer] = alpha;
}

/*
 * Set the layer belnd mode.
 */
void
s3_set_layer_blend(
	int layer,
	int blend)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	layer_blend[layer] = blend;
}

/*
 * Get the layer file name.
 */
const char *
s3_get_layer_file_name(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_file_name[layer];
}

/*
 * Set the layer file name.
 */
bool
s3_set_layer_file_name(
	int layer,
	const char *file_name)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	assert(layer != S3_LAYER_CLICK);
	assert(layer != S3_LAYER_MSGBOX);
	assert(layer != S3_LAYER_NAMEBOX);
	assert(layer != S3_LAYER_AUTO);
	assert(layer != S3_LAYER_SKIP);

	if (layer_file_name[layer] != NULL) {
		free(layer_file_name[layer]);
		layer_file_name[layer] = NULL;
	}
	if (file_name != NULL) {
		layer_file_name[layer] = strdup(file_name);
		if (layer_file_name[layer] == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	}
	return true;
}

/*
 * Get the layer image.
 */
struct s3_image *
s3_get_layer_image(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_image[layer];
}

/*
 * Set the layer image.
 */
void
s3_set_layer_image(
	int layer,
	struct s3_image *img)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	assert(layer != S3_LAYER_CLICK);
	assert(layer != S3_LAYER_MSGBOX);
	assert(layer != S3_LAYER_NAMEBOX);
	assert(layer != S3_LAYER_AUTO);
	assert(layer != S3_LAYER_SKIP);

	destroy_layer(layer);

	layer_image[layer] = img;
}

/*
 * Set the layer frame for eye blinking and lip synchronizing.
 */
void
s3_set_layer_frame(
	int layer,
	int frame)
{
	layer_frame[layer] = frame;
}

/*
 * Get the text layer string.
 */
const char *
s3_get_layer_text(
	int layer)
{
	assert(layer >= S3_LAYER_TEXT1);
	assert(layer <= S3_LAYER_TEXT8);

	return layer_text[layer];
}

/*
 * Set the text layer string.
 */
bool
s3_set_layer_text(
	int layer,
	const char *msg)
{
	assert(layer >= S3_LAYER_TEXT1);
	assert(layer <= S3_LAYER_TEXT8);

	if (layer_text[layer] != NULL) {
		free(layer_text[layer]);
		layer_text[layer] = NULL;
	}

	if (msg != NULL && strcmp(msg, "") != 0) {
		layer_text[layer] = strdup(msg);
		if (layer_text[layer] == NULL) {
			s3_log_out_of_memory();
			return false;
		}
	}

	return true;
}

/*
 * Get the sysbtn idle image.
 */
struct s3_image *
s3_get_sysbtn_idle_image(void)
{
	return sysbtn_idle_image;
}

/*
 * Get the sysbtn hover image.
 */
struct s3_image *
s3_get_sysbtn_hover_image(void)
{
	return sysbtn_hover_image;
}

/*
 * Clear basic layers.
 */
void
s3_clear_stage_basic(void)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (i == S3_LAYER_MSGBOX ||
		    i == S3_LAYER_NAMEBOX ||
		    i == S3_LAYER_CLICK ||
		    i == S3_LAYER_AUTO ||
		    i == S3_LAYER_SKIP)
			continue;

		destroy_layer(i);
	}

	for (i = S3_LAYER_TEXT1; i <= S3_LAYER_TEXT8; i++) {
		if (layer_text[i] != NULL) {
			free(layer_text[i]);
			layer_text[i] = NULL;
		}
	}
}

/*
 * Clear the stage and make it initial state.
 */
void
s3_clear_stage(void)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		switch (i) {
		case S3_LAYER_BG:
			destroy_layer(i);
			s3_set_layer_position(i, 0, 0);
			s3_set_layer_alpha(i, 255);
			break;
		case S3_LAYER_MSGBOX:
			s3_set_layer_position(i, conf_msgbox_x, conf_msgbox_y);
			s3_set_layer_alpha(i, 255);
			s3_show_msgbox(false);
			break;
		case S3_LAYER_NAMEBOX:
			s3_set_layer_position(i, conf_namebox_x, conf_namebox_y);
			s3_set_layer_alpha(i, 255);
			s3_show_namebox(false);
			break;
		case S3_LAYER_BG2:	/* fall-thru */
		case S3_LAYER_CHB:	/* fall-thru */
		case S3_LAYER_CHB_EYE:	/* fall-thru */
		case S3_LAYER_CHB_LIP:	/* fall-thru */
		case S3_LAYER_CHB_FO:	/* fall-thru */
		case S3_LAYER_CHL:	/* fall-thru */
		case S3_LAYER_CHL_EYE:	/* fall-thru */
		case S3_LAYER_CHL_LIP:	/* fall-thru */
		case S3_LAYER_CHL_FO:	/* fall-thru */
		case S3_LAYER_CHLC:	/* fall-thru */
		case S3_LAYER_CHLC_EYE:	/* fall-thru */
		case S3_LAYER_CHLC_LIP:	/* fall-thru */
		case S3_LAYER_CHLC_FO:	/* fall-thru */
		case S3_LAYER_CHR:	/* fall-thru */
		case S3_LAYER_CHR_EYE:	/* fall-thru */
		case S3_LAYER_CHR_LIP:	/* fall-thru */
		case S3_LAYER_CHR_FO:	/* fall-thru */
		case S3_LAYER_CHRC:	/* fall-thru */
		case S3_LAYER_CHRC_EYE:	/* fall-thru */
		case S3_LAYER_CHRC_LIP:	/* fall-thru */
		case S3_LAYER_CHRC_FO:	/* fall-thru */
		case S3_LAYER_CHC:	/* fall-thru */
		case S3_LAYER_CHC_EYE:	/* fall-thru */
		case S3_LAYER_CHC_LIP:	/* fall-thru */
		case S3_LAYER_CHC_FO:	/* fall-thru */
		case S3_LAYER_CHF:	/* fall-thru */
		case S3_LAYER_CHF_EYE:	/* fall-thru */
		case S3_LAYER_CHF_LIP:	/* fall-thru */
		case S3_LAYER_CHF_FO:	/* fall-thru */
		case S3_LAYER_EFB1:	/* fall-thru */
		case S3_LAYER_EFB2:	/* fall-thru */
		case S3_LAYER_EFB3:	/* fall-thru */
		case S3_LAYER_EFB4:	/* fall-thru */
		case S3_LAYER_EFF1:	/* fall-thru */
		case S3_LAYER_EFF2:	/* fall-thru */
		case S3_LAYER_EFF3:	/* fall-thru */
		case S3_LAYER_EFF4:	/* fall-thru */
			destroy_layer(i);
			s3_set_layer_position(i, 0, 0);
			s3_set_layer_alpha(i, 255);
			break;
		case S3_LAYER_CLICK:
			s3_set_layer_position(i, conf_click_x, conf_click_y);
			s3_set_layer_alpha(i, 255);
			s3_show_click(false);
			break;
		case S3_LAYER_AUTO:
			s3_set_layer_position(i, conf_automode_x, conf_automode_y);
			s3_set_layer_alpha(i, 255);
			s3_show_automode_banner(false);
			break;
		case S3_LAYER_SKIP:
			s3_set_layer_position(i, conf_skipmode_x, conf_skipmode_y);
			s3_set_layer_alpha(i, 255);
			s3_show_skipmode_banner(false);
			break;
		case S3_LAYER_TEXT1:	/* fall-thru */
		case S3_LAYER_TEXT2:	/* fall-thru */
		case S3_LAYER_TEXT3:	/* fall-thru */
		case S3_LAYER_TEXT4:	/* fall-thru */
		case S3_LAYER_TEXT5:	/* fall-thru */
		case S3_LAYER_TEXT6:	/* fall-thru */
		case S3_LAYER_TEXT7:	/* fall-thru */
		case S3_LAYER_TEXT8:
			destroy_layer(i);
			s3_set_layer_position(i, 0, 0);
			s3_set_layer_alpha(i, 255);
			break;
		default:
			assert(0);
			break;
		}
	}
}

/*
 * Conversion of Layer Index and Character Position
 */

/*
 * Convert a character position to a stage layer index.
 */
int
s3_chpos_to_layer(
	int chpos)
{
	switch (chpos) {
	case S3_CH_BACK:
		return S3_LAYER_CHB;
	case S3_CH_LEFT:
		return S3_LAYER_CHL;
	case S3_CH_LEFT_CENTER:
		return S3_LAYER_CHLC;
	case S3_CH_RIGHT:
		return S3_LAYER_CHR;
	case S3_CH_RIGHT_CENTER:
		return S3_LAYER_CHRC;
	case S3_CH_CENTER:
		return S3_LAYER_CHC;
	case S3_CH_FACE:
		return S3_LAYER_CHF;
	default:
		assert(0);
		break;
	}
	return -1;
}

/*
 * Convert a character position to a stage layer index (character eye).
 */
int
s3_chpos_to_eye_layer(
	int chpos)
{
	switch (chpos) {
	case S3_CH_BACK:
		return S3_LAYER_CHB_EYE;
	case S3_CH_LEFT:
		return S3_LAYER_CHL_EYE;
	case S3_CH_LEFT_CENTER:
		return S3_LAYER_CHLC_EYE;
	case S3_CH_RIGHT:
		return S3_LAYER_CHR_EYE;
	case S3_CH_RIGHT_CENTER:
		return S3_LAYER_CHRC_EYE;
	case S3_CH_CENTER:
		return S3_LAYER_CHC_EYE;
	case S3_CH_FACE:
		return S3_LAYER_CHF_EYE;
	default:
		assert(0);
		break;
	}
	return -1;
}

/*
 * Convert a character position to a stage layer index (character lip).
 */
int
s3_chpos_to_lip_layer(
	int chpos)
{
	switch (chpos) {
	case S3_CH_BACK:
		return S3_LAYER_CHB_LIP;
	case S3_CH_LEFT:
		return S3_LAYER_CHL_LIP;
	case S3_CH_LEFT_CENTER:
		return S3_LAYER_CHLC_LIP;
	case S3_CH_RIGHT:
		return S3_LAYER_CHR_LIP;
	case S3_CH_RIGHT_CENTER:
		return S3_LAYER_CHRC_LIP;
	case S3_CH_CENTER:
		return S3_LAYER_CHC_LIP;
	case S3_CH_FACE:
		return S3_LAYER_CHF_LIP;
	default:
		assert(0);
		break;
	}
	return -1;
}

/*
 * Convert a stage layer index to a character position.
 */
int
s3_layer_to_chpos(
	int layer)
{
	assert(layer == S3_LAYER_CHB  || layer == S3_LAYER_CHB_EYE  || layer == S3_LAYER_CHB_LIP ||
	       layer == S3_LAYER_CHL  || layer == S3_LAYER_CHL_EYE  || layer == S3_LAYER_CHL_LIP ||
	       layer == S3_LAYER_CHR  || layer == S3_LAYER_CHR_EYE  || layer == S3_LAYER_CHR_LIP ||
	       layer == S3_LAYER_CHC  || layer == S3_LAYER_CHC_EYE  || layer == S3_LAYER_CHC_LIP ||
	       layer == S3_LAYER_CHRC || layer == S3_LAYER_CHRC_EYE || layer == S3_LAYER_CHRC_LIP ||
	       layer == S3_LAYER_CHLC || layer == S3_LAYER_CHLC_EYE || layer == S3_LAYER_CHLC_LIP ||
	       layer == S3_LAYER_CHF  || layer == S3_LAYER_CHF_EYE  || layer == S3_LAYER_CHF_LIP);

	switch (layer) {
	case S3_LAYER_CHB:
	case S3_LAYER_CHB_EYE:
	case S3_LAYER_CHB_LIP:
		return S3_CH_BACK;
	case S3_LAYER_CHL:
	case S3_LAYER_CHL_EYE:
	case S3_LAYER_CHL_LIP:
		return S3_CH_LEFT;
	case S3_LAYER_CHLC:
	case S3_LAYER_CHLC_EYE:
	case S3_LAYER_CHLC_LIP:
		return S3_CH_LEFT_CENTER;
	case S3_LAYER_CHR:
	case S3_LAYER_CHR_EYE:
	case S3_LAYER_CHR_LIP:
		return S3_CH_RIGHT;
	case S3_LAYER_CHRC:
	case S3_LAYER_CHRC_EYE:
	case S3_LAYER_CHRC_LIP:
		return S3_CH_RIGHT_CENTER;
	case S3_LAYER_CHC:
	case S3_LAYER_CHC_EYE:
	case S3_LAYER_CHC_LIP:
		return S3_CH_CENTER;
	case S3_LAYER_CHF:
	case S3_LAYER_CHF_EYE:
	case S3_LAYER_CHF_LIP:
		return S3_CH_FACE;
	default:
		assert(0);
		break;
	}
	return -1;
}

/*
 * Stage Rendering
 */

/*
 * Render the stage with all stage layers.
 */
void
s3_render_stage(void)
{
	int i;

	/* Update an anime frame. */
	s3_update_anime_frame();

	/* Render stage layers. */
	render_layer(S3_LAYER_BG);
	render_layer(S3_LAYER_BG_FO);
	render_layer(S3_LAYER_BG2);
	for (i = S3_LAYER_EFB1; i <= S3_LAYER_EFB4; i++)
		render_layer(i);
	render_layer(S3_LAYER_CHB);
	render_layer(S3_LAYER_CHB_EYE);
	render_layer(S3_LAYER_CHB_LIP);
	render_layer(S3_LAYER_CHB_FO);
	render_layer(S3_LAYER_CHL);
	render_layer(S3_LAYER_CHL_EYE);
	render_layer(S3_LAYER_CHL_LIP);
	render_layer(S3_LAYER_CHL_FO);
	render_layer(S3_LAYER_CHLC);
	render_layer(S3_LAYER_CHLC_EYE);
	render_layer(S3_LAYER_CHLC_LIP);
	render_layer(S3_LAYER_CHLC_FO);
	render_layer(S3_LAYER_CHR);
	render_layer(S3_LAYER_CHR_EYE);
	render_layer(S3_LAYER_CHR_LIP);
	render_layer(S3_LAYER_CHR_FO);
	render_layer(S3_LAYER_CHRC);
	render_layer(S3_LAYER_CHRC_EYE);
	render_layer(S3_LAYER_CHRC_LIP);
	render_layer(S3_LAYER_CHRC_FO);
	render_layer(S3_LAYER_CHC);
	render_layer(S3_LAYER_CHC_EYE);
	render_layer(S3_LAYER_CHC_LIP);
	render_layer(S3_LAYER_CHC_FO);
	for (i = S3_LAYER_EFF1; i <= S3_LAYER_EFF4; i++)
		render_layer(i);
	if (is_msgbox_visible)
		render_layer(S3_LAYER_MSGBOX);
	if (is_namebox_visible && conf_namebox_enable)
		render_layer(S3_LAYER_NAMEBOX);
	if (is_msgbox_visible) {
		render_layer(S3_LAYER_CHF);
		render_layer(S3_LAYER_CHF_EYE);
		render_layer(S3_LAYER_CHF_LIP);
		render_layer(S3_LAYER_CHF_FO);
	}
	if (is_click_visible)
		render_layer(S3_LAYER_CLICK);
	if (is_auto_visible)
		render_layer(S3_LAYER_AUTO);
	if (is_skip_visible)
		render_layer(S3_LAYER_SKIP);
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		if (is_choosebox_idle_visible[i])
			render_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i));
		if (is_choosebox_hover_visible[i])
			render_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1);
	}
	for (i = S3_LAYER_TEXT1; i <= S3_LAYER_TEXT8; i++)
		render_layer(i);

	/* Render the GUI buttons. */
	if (s3_is_gui_running())
		s3i_run_gui_render();

	/* Render the sysbtn. */
	if (s3_is_sysbtn_visible()) {
		if (!s3_is_sysbtn_pointed()) {
			pf_render_texture(conf_sysbtn_x,
					  conf_sysbtn_y,
					  sysbtn_idle_image->width,
					  sysbtn_idle_image->height,
					  sysbtn_idle_image->tex_id,
					  0,
					  0,
					  sysbtn_idle_image->width,
					  sysbtn_idle_image->height,
					  255);
		} else {
			pf_render_texture(conf_sysbtn_x,
					  conf_sysbtn_y,
					  sysbtn_hover_image->width,
					  sysbtn_hover_image->height,
					  sysbtn_hover_image->tex_id,
					  0,
					  0,
					  sysbtn_hover_image->width,
					  sysbtn_hover_image->height,
					  255);
		}
	}
}

/* Render a layer. */
static void
render_layer(
	int layer)
{
	struct s3_image *base_img;
	int src_x, src_width;
	int alpha;

	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	/* Don't render if no image. */
	if (layer_image[layer] == NULL)
		return;

	/* Calculate the alpha value. */
	alpha = layer_alpha[layer];
	if (stage_mode == STAGE_MODE_FADE) {
		if (layer == S3_LAYER_BG_FO ||
		    layer == S3_LAYER_CHL_FO ||
		    layer == S3_LAYER_CHLC_FO ||
		    layer == S3_LAYER_CHR_FO ||
		    layer == S3_LAYER_CHRC_FO ||
		    layer == S3_LAYER_CHC_FO ||
		    layer == S3_LAYER_CHF_FO)
			alpha = (int)((1.0f - fade_progress) * (float)alpha);
		else
			alpha = (int)(fade_progress * (float)alpha);
	}

	/* Calculate the eye/lip frame index. */
	if (layer == S3_LAYER_CHB_EYE ||
	    layer == S3_LAYER_CHL_EYE ||
	    layer == S3_LAYER_CHLC_EYE ||
	    layer == S3_LAYER_CHC_EYE ||
	    layer == S3_LAYER_CHRC_EYE ||
	    layer == S3_LAYER_CHR_EYE ||
	    layer == S3_LAYER_CHF_EYE ||
	    layer == S3_LAYER_CHB_LIP ||
	    layer == S3_LAYER_CHL_LIP ||
	    layer == S3_LAYER_CHLC_LIP ||
	    layer == S3_LAYER_CHC_LIP ||
	    layer == S3_LAYER_CHRC_LIP ||
	    layer == S3_LAYER_CHR_LIP ||
	    layer == S3_LAYER_CHF_LIP) {
		int layer_chpos = s3_layer_to_chpos(layer);
		int ch_layer = s3_chpos_to_layer(layer_chpos);
		base_img = layer_image[ch_layer];
		if (base_img == NULL)
			return;
		src_width = base_img->width;
		src_x = src_width * layer_frame[layer];
	} else {
		src_width = layer_image[layer]->width;
		src_x = 0;
	}

	/* If 3D. */
	if (layer_rotate[layer] != 0 ||
	    layer_scale_x[layer] != 1.0f ||
	    layer_scale_y[layer] != 1.0f) {
		float x1 = 0;
		float y1 = 0;
		float x2 = (float)layer_image[layer]->width - 1.0f;
		float y2 = 0;
		float x3 = 0;
		float y3 = (float)layer_image[layer]->height - 1.0f;;
		float x4 = (float)layer_image[layer]->width - 1.0f;
		float y4 = (float)layer_image[layer]->height - 1.0f;
		float center_x = (float)layer_center_x[layer];
		float center_y = (float)layer_center_y[layer];
		float rad = (float)layer_rotate[layer];

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
		x1 *= layer_scale_x[layer];
		y1 *= layer_scale_y[layer];
		x2 *= layer_scale_x[layer];
		y2 *= layer_scale_y[layer];
		x3 *= layer_scale_x[layer];
		y3 *= layer_scale_y[layer];
		x4 *= layer_scale_x[layer];
		y4 *= layer_scale_y[layer];

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
		x1 += center_x;
		y1 += center_y;
		x2 += center_x;
		y2 += center_y;
		x3 += center_x;
		y3 += center_y;
		x4 += center_x;
		y4 += center_y;

		/* 5. Shift for the layer position. */
		x1 += (float)layer_x[layer];
		y1 += (float)layer_y[layer];
		x2 += (float)layer_x[layer];
		y2 += (float)layer_y[layer];
		x3 += (float)layer_x[layer];
		y3 += (float)layer_y[layer];
		x4 += (float)layer_x[layer];
		y4 += (float)layer_y[layer];

		/* Render. */
		if (layer >= S3_LAYER_CHB && layer <= S3_LAYER_CHC && layer_dim[layer]) {
			pf_render_texture_3d_dim(x1,
						 y1,
						 x2,
						 y2,
						 x3,
						 y3,
						 x4,
						 y4,
						 layer_image[layer]->tex_id,
						 0,
						 0,
						 layer_image[layer]->width,
						 layer_image[layer]->height,
						 layer_alpha[layer]);
		} else if (layer_blend[layer] == S3_BLEND_ALPHA) {
			pf_render_texture_3d(x1,
					     y1,
					     x2,
					     y2,
					     x3,
					     y3,
					     x4,
					     y4,
					     layer_image[layer]->tex_id,
					     0,
					     0,
					     layer_image[layer]->width,
					     layer_image[layer]->height,
					     layer_alpha[layer]);
		} else if (layer_blend[layer] == S3_BLEND_ADD) {
			pf_render_texture_3d_add(x1,
						 y1,
						 x2,
						 y2,
						 x3,
						 y3,
						 x4,
						 y4,
						 layer_image[layer]->tex_id,
						 0,
						 0,
						 layer_image[layer]->width,
						 layer_image[layer]->height,
						 layer_alpha[layer]);
		} else if (layer_blend[layer] == S3_BLEND_SUB) {
			pf_render_texture_3d_sub(x1,
						 y1,
						 x2,
						 y2,
						 x3,
						 y3,
						 x4,
						 y4,
						 layer_image[layer]->tex_id,
						 0,
						 0,
						 layer_image[layer]->width,
						 layer_image[layer]->height,
						 layer_alpha[layer]);
		}
		return;
	}

	/* If dim. */
	if (layer >= S3_LAYER_CHB && layer <= S3_LAYER_CHC && layer_dim[layer]) {
		pf_render_texture_dim(layer_x[layer],
				      layer_y[layer],
				      (int)((float)src_width * layer_scale_x[layer]),
				      (int)((float)layer_image[layer]->height * layer_scale_y[layer]),
				      layer_image[layer]->tex_id,
				      src_x,
				      0,
				      src_width,
				      layer_image[layer]->height,
				      layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_ALPHA) {
		pf_render_texture(layer_x[layer],
				  layer_y[layer],
				  (int)((float)src_width * layer_scale_x[layer]),
				  (int)((float)layer_image[layer]->height * layer_scale_y[layer]),
				  layer_image[layer]->tex_id,
				  src_x,
				  0,
				  src_width,
				  layer_image[layer]->height,
				  layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_ADD) {
		pf_render_texture_add(layer_x[layer],
				      layer_y[layer],
				      (int)((float)src_width * layer_scale_x[layer]),
				      (int)((float)layer_image[layer]->height * layer_scale_y[layer]),
				      layer_image[layer]->tex_id,
				      src_x,
				      0,
				      src_width,
				      layer_image[layer]->height,
				      layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_SUB) {
		pf_render_texture_sub(layer_x[layer],
				      layer_y[layer],
				      (int)((float)src_width * layer_scale_x[layer]),
				      (int)((float)layer_image[layer]->height * layer_scale_y[layer]),
				      layer_image[layer]->tex_id,
				      src_x,
				      0,
				      src_width,
				      layer_image[layer]->height,
				      layer_alpha[layer]);
	}
}

/*
 * Save Data Thumbnail Drawing
 */

/*
 * Draws entire stage into the save data thumbnail.
 */
void
s3_draw_stage_to_thumb(void)
{
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (i == S3_LAYER_MSGBOX)
			if (!is_msgbox_visible)
				continue;
		if (i== S3_LAYER_NAMEBOX)
			if (!is_namebox_visible || !conf_namebox_enable)
				continue;
		if (i == S3_LAYER_AUTO)
			continue;
		if (i == S3_LAYER_SKIP)
			continue;
		if (layer_image[i] == NULL)
			continue;
		if (layer_alpha[i] == 0)
			continue;
		s3_draw_image_scale(thumb_image,
				    conf_game_width,
				    conf_game_height,
				    layer_x[i],
				    layer_y[i],
				    layer_image[i]);
	}
}

/*
 * Get the image of the save data thumbnail.
 */
struct s3_image *
s3_get_thumb_image(void)
{
	return thumb_image;
}

/*
 * Fading Control
 */

/*
 * Get a fading method from a string.
 */
int
s3_get_fade_method(
	const char *method)
{
	if (method == NULL)
		return S3_FADE_NORMAL;
	if (strcmp(method, "") == 0)
		return S3_FADE_NORMAL;

	/*
	 * Normal fading.
	 */

	if (strcmp(method, "normal") == 0 ||
	    strcmp(method, "n") == 0 ||
	    strcmp(method, "") == 0)
		return S3_FADE_NORMAL;

	/*
	 * Rule fading. (1-bit universal transition)
	 */

	if (strcmp(method, "rule") == 0)
		return S3_FADE_RULE;

	/*
	 * Melt fading. (1-bit universal transition)
	 */

	if (strcmp(method, "melt") == 0)
		return S3_FADE_MELT;

	/* Invalid. */
	s3_log_tag_error(S3_TR("Invalid fade type \"%s\"."), method);
	return S3_FADE_INVALID;
}

/*
 * Get a acceleration method from a string.
 */
int
s3_get_accel_method(
	const char *method)
{
	if (method == NULL ||
	    strcmp(method, "") == 0 ||
	    strcmp(method, "uniform") == 0)
		return S3_ANIME_ACCEL_UNIFORM;
	if (strcmp(method, "accel") == 0)
		return S3_ANIME_ACCEL_ACCEL;
	if (strcmp(method, "deaccel") == 0)
		return S3_ANIME_ACCEL_DEACCEL;
	if (strcmp(method, "smoothstep") == 0)
		return S3_ANIME_ACCEL_SMOOTHSTEP;
	if (strcmp(method, "invsmoothstep") == 0)
		return S3_ANIME_ACCEL_INVSMOOTHSTEP;

	/* Invalid. */
	s3_log_tag_error(S3_TR("Invalid accel type \"%s\"."), method);
	return S3_ANIME_ACCEL_INVALID;
}

/*
 * Start a fading.
 */
bool
s3_start_fade(
	struct s3_fade_desc *desc,
	int method,
	float t,
	struct s3_image *rule_img)
{
	struct info {
		int layer;
		int fo_layer;
		int eye_layer;
		int lip_layer;
	} info[S3_FADE_DESC_COUNT] = {
		/* base layer,  fade-out layer,   eye layer,         lip layer */
		{S3_LAYER_BG,   S3_LAYER_BG_FO,   -1,                -1},
		{S3_LAYER_CHB,  S3_LAYER_CHB_FO,  S3_LAYER_CHB_EYE,  S3_LAYER_CHB_LIP},
		{S3_LAYER_CHL,  S3_LAYER_CHL_FO,  S3_LAYER_CHL_EYE,  S3_LAYER_CHL_LIP},
		{S3_LAYER_CHLC, S3_LAYER_CHLC_FO, S3_LAYER_CHLC_EYE, S3_LAYER_CHLC_LIP},
		{S3_LAYER_CHR,  S3_LAYER_CHR_FO,  S3_LAYER_CHR_EYE,  S3_LAYER_CHR_LIP},
		{S3_LAYER_CHRC, S3_LAYER_CHRC_FO, S3_LAYER_CHRC_EYE, S3_LAYER_CHRC_LIP},
		{S3_LAYER_CHC,  S3_LAYER_CHC_FO,  S3_LAYER_CHC_EYE,  S3_LAYER_CHC_LIP},
		{S3_LAYER_CHF,  S3_LAYER_CHF_FO,  S3_LAYER_CHF_EYE,  S3_LAYER_CHF_LIP},
	};
	int i;

	assert(stage_mode == STAGE_MODE_IDLE);

	/* Enable the fading mode. */
	stage_mode = STAGE_MODE_FADE;
	fade_method = method;
	fade_rule_img = rule_img;

	/* Setup the layers. */
	for (i = 0; i < S3_FADE_DESC_COUNT; i++) {
		int layer = info[i].layer;
		int fo_layer= info[i].fo_layer;

		/* Swap the image if the stay flag is not set. */
		if (!desc[i].stay) {
			s3_new_anime_sequence(layer);
			s3_add_anime_sequence_property_f("start", 0);
			s3_add_anime_sequence_property_f("end", t);
			s3_add_anime_sequence_property_i("from-x", desc[i].x);
			s3_add_anime_sequence_property_i("to-x", desc[i].x);
			s3_add_anime_sequence_property_i("from-y", desc[i].y);
			s3_add_anime_sequence_property_i("to-y", desc[i].y);
			s3_add_anime_sequence_property_i("from-a", 0);
			s3_add_anime_sequence_property_i("to-a", desc[i].alpha);
			s3_add_anime_sequence_property_f("from-scale-x", desc[i].scale_x);
			s3_add_anime_sequence_property_f("to-scale-x", desc[i].scale_x);
			s3_add_anime_sequence_property_f("from-scale-y", desc[i].scale_y);
			s3_add_anime_sequence_property_f("to-scale-y", desc[i].scale_y);
			s3_add_anime_sequence_property_i("center-x", desc[i].center_x);
			s3_add_anime_sequence_property_i("center-y", desc[i].center_y);
			s3_add_anime_sequence_property_f("from-rotate", desc[i].rotate);
			s3_add_anime_sequence_property_f("to-rotate", desc[i].rotate);
			s3_add_anime_sequence_property_i("accel", S3_ANIME_ACCEL_SMOOTHSTEP);

			s3_new_anime_sequence(fo_layer);
			s3_add_anime_sequence_property_f("start", 0);
			s3_add_anime_sequence_property_f("end", t);
			s3_add_anime_sequence_property_i("from-x", layer_x[layer]);
			s3_add_anime_sequence_property_i("to-x", layer_x[layer]);
			s3_add_anime_sequence_property_i("from-y", layer_y[layer]);
			s3_add_anime_sequence_property_i("to-y", layer_y[layer]);
			s3_add_anime_sequence_property_i("from-a", 0);
			s3_add_anime_sequence_property_i("to-a", layer_alpha[layer]);
			s3_add_anime_sequence_property_f("from-scale-x", layer_scale_x[layer]);
			s3_add_anime_sequence_property_f("to-scale-x", layer_scale_x[layer]);
			s3_add_anime_sequence_property_f("from-scale-y", layer_scale_y[layer]);
			s3_add_anime_sequence_property_f("to-scale-y", layer_scale_y[layer]);
			s3_add_anime_sequence_property_i("center-x", layer_center_x[layer]);
			s3_add_anime_sequence_property_i("center-y", layer_center_y[layer]);
			s3_add_anime_sequence_property_f("from-rotate", layer_rotate[layer]);
			s3_add_anime_sequence_property_f("to-rotate", layer_rotate[layer]);
			s3_add_anime_sequence_property_i("accel", S3_ANIME_ACCEL_INVSMOOTHSTEP);

			layer_fading[layer] = true;
			layer_fading[fo_layer] = true;

			/* Transfer the existing layer to the fade-out layer. */
			layer_image[fo_layer] = layer_image[layer];
			layer_image[layer] = desc[i].image;

			/* Disable the eyes/lips layers. */
			if (info[i].eye_layer != -1)
				destroy_layer(info[i].eye_layer);
			if (info[i].lip_layer != -1)
				destroy_layer(info[i].lip_layer);

			/* Set the new image file name. */
			if (!s3_set_layer_file_name(layer, desc[i].fname))
				return false;

			/* Start the animes. */
			s3_start_layer_anime(layer);
			s3_start_layer_anime(fo_layer);
		}
	}

	return true;
}

/*
 * Set the offset for the shake command.
 */
void
s3set_shake_offset(
	int x,
	int y)
{
	assert(stage_mode == STAGE_MODE_FADE);

	shake_offset_x = x;
	shake_offset_y = y;
}

/*
 * Check if the fading is running.
 */
bool
s3_is_fade_running(void)
{
	bool ret;
	int i;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		if (layer_fading[i]) {
			if (s3_is_anime_finished_for_layer(i))
				ret = true;
		}
	}

	return ret;
}

/*
 * End the fading.
 */
void
s3_finish_fade(void)
{
	int layer[] = {
		S3_LAYER_BG,
		S3_LAYER_BG_FO,
		S3_LAYER_CHB,
		S3_LAYER_CHB_FO,
		S3_LAYER_CHB_EYE,
		S3_LAYER_CHB_LIP,
		S3_LAYER_CHL,
		S3_LAYER_CHL_FO,
		S3_LAYER_CHL_EYE,
		S3_LAYER_CHL_LIP,
		S3_LAYER_CHLC,
		S3_LAYER_CHLC_FO,
		S3_LAYER_CHLC_EYE,
		S3_LAYER_CHLC_LIP,
		S3_LAYER_CHR,
		S3_LAYER_CHR_FO,
		S3_LAYER_CHR_EYE,
		S3_LAYER_CHR_LIP,
		S3_LAYER_CHRC,
		S3_LAYER_CHRC_FO,
		S3_LAYER_CHRC_EYE,
		S3_LAYER_CHRC_LIP,
		S3_LAYER_CHC,
		S3_LAYER_CHC_FO,
		S3_LAYER_CHC_EYE,
		S3_LAYER_CHC_LIP,
		S3_LAYER_CHF,
		S3_LAYER_CHF_FO,
		S3_LAYER_CHF_EYE,
		S3_LAYER_CHF_LIP
	};

	int i;

	assert(stage_mode == STAGE_MODE_FADE);

	/* Destroy the rule image. */
	if (fade_rule_img != NULL) {
		s3_destroy_image(fade_rule_img);
		fade_rule_img = NULL;
	}

	for (i = 0; i < sizeof(layer) / sizeof(int); i++) {
		int l = layer[i];
		if (layer_fading[l]) {
			s3_clear_layer_anime_sequence(l);
			layer_fading[l] = false;
		}
	}

	/* Destroy the fade-in layer images. */
	if (layer_image[S3_LAYER_BG_FO] != layer_image[S3_LAYER_BG])
		destroy_layer(S3_LAYER_BG_FO);
	if (layer_image[S3_LAYER_CHB_FO] != layer_image[S3_LAYER_CHB])
		destroy_layer(S3_LAYER_CHB_FO);
	if (layer_image[S3_LAYER_CHL_FO] != layer_image[S3_LAYER_CHL])
		destroy_layer(S3_LAYER_CHL_FO);
	if (layer_image[S3_LAYER_CHLC_FO] != layer_image[S3_LAYER_CHLC])
		destroy_layer(S3_LAYER_CHLC_FO);
	if (layer_image[S3_LAYER_CHR_FO] != layer_image[S3_LAYER_CHR])
		destroy_layer(S3_LAYER_CHR_FO);
	if (layer_image[S3_LAYER_CHRC_FO] != layer_image[S3_LAYER_CHRC])
		destroy_layer(S3_LAYER_CHRC_FO);
	if (layer_image[S3_LAYER_CHC_FO] != layer_image[S3_LAYER_CHC])
		destroy_layer(S3_LAYER_CHC_FO);
	if (layer_image[S3_LAYER_CHF_FO] != layer_image[S3_LAYER_CHF])
		destroy_layer(S3_LAYER_CHF_FO);

	/* Reset the stage mode. */
	stage_mode = STAGE_MODE_IDLE;
}

/*
 * Character Control
 */

/*
 * Specify a character index for a character position.
 */
void
s3_set_ch_name_mapping(
	int pos,
	int ch_name_index)
{
	assert(pos >= 0 && pos < S3_CH_ALL_LAYERS);

	ch_name_mapping[pos] = ch_name_index;
}

/*
 * Set the talker character name index. (-1 for no speaker)
 */
void
s3_set_ch_talking(
	int ch_name_index)
{
	ch_talking = ch_name_index;
}

/*
 * Get the talker character position.
 */
int
s3_get_talking_chpos(void)
{
	int i;

	if (ch_talking == -1)
		return -1;

	for (i = 0; i < S3_CH_BASIC_LAYERS; i++) {
		if (ch_name_mapping[i] == ch_talking)
			return i;
	}
	return -1;
}

/*
 * Update the character dimming automatically.
 */
void
s3_update_ch_dim_by_talking_ch(void)
{
	int i;

	for (i = 0; i < S3_CH_BASIC_LAYERS; i++) {
		int layer;

		layer = s3_chpos_to_layer(i);

		if (ch_talking == -1) {
			/* No character is talking. */
			if (!conf_character_auto_focus) {
				/* All characters are light if auto focus is not enabled. */
				layer_dim[layer] = true;
			} else {
				/* All characters are dark if auto focus is enabled. */
				layer_dim[layer] = false;
			}
		} else if (ch_name_mapping[i] == -1) {
			/* This character is unknown. */
			if (!conf_character_auto_focus) {
				/* Unknown character is light if auto focus is not enabled. */
				layer_dim[layer] = false;
			} else {
				/* Unknown character is dark if auto focus if enabled. */
				layer_dim[layer] = true;
			}

		} else if (ch_name_mapping[i] == ch_talking) {
			/* This character is talking. */
			layer_dim[layer] = false;
		} else if (strncmp(conf_character_folder[ch_name_mapping[i]],
				   conf_character_folder[ch_talking],
				   strlen(conf_character_folder[ch_talking])) == 0) {
			/* XXX: This is the same character as talking one. */
			layer_dim[i] = false;
		} else {
			/* Not a talking character. */
			layer_dim[i] = true;
		}
	}
}

/*
 * Update the character dimming manually.
 */
void
s3_force_ch_dim(
	int chpos,
	bool is_dim)
{
	int layer;

	assert(chpos >= 0 && chpos < S3_CH_BASIC_LAYERS);

	layer = s3_chpos_to_layer(chpos);

	layer_dim[layer] = is_dim;
}

/*
 * Get the dimming state.
 */
bool
s3_get_ch_dim(
	int chpos)
{
	int layer;

	layer = s3_chpos_to_layer(chpos);

	return layer_dim[layer];
}

/*
 * Name Box Control
 */

/*
 * Fill the name box by the name box image.
 */
void
s3_fill_namebox(void)
{
	if (namebox_image == NULL)
		return;

	s3_draw_image_copy(layer_image[S3_LAYER_NAMEBOX],
			   0,
			   0,
			   namebox_image,
			   layer_image[S3_LAYER_NAMEBOX]->width,
			   layer_image[S3_LAYER_NAMEBOX]->height,
			   0,
			   0);
}

/*
 * Get the name box position and size.
 */
void
s3_get_namebox_rect(
	int *namebox_x,
	int *namebox_y,
	int *namebox_w,
	int *namebox_h)
{
	*namebox_x = conf_namebox_x;
	*namebox_y = conf_namebox_x;
	*namebox_w = s3_get_image_width(namebox_image);
	*namebox_h = s3_get_image_height(namebox_image);
}

/*
 * Show or hides the name box.
 */
void
s3_show_namebox(
	bool show)
{
	is_namebox_visible = show;
}

/*
 * Message Box Control
 */

/*
 * Fill the message box by the message box image.
 */
void
s3_fill_msgbox(void)
{
	if (msgbox_image == NULL)
		return;

	s3_draw_image_copy(layer_image[S3_LAYER_MSGBOX],
			   0,
			   0,
			   msgbox_image,
			   layer_image[S3_LAYER_MSGBOX]->width,
			   layer_image[S3_LAYER_MSGBOX]->height,
			   0,
			   0);
}

/*
 * Show or hides the message box.
 */
void
s3_show_msgbox(
	bool show)
{
	is_msgbox_visible = show;
}

/*
 * Get the message box position and size.
 */
void
s3_get_msgbox_rect(
	int *msgbox_x,
	int *msgbox_y,
	int *msgbox_w,
	int *msgbox_h)
{
	*msgbox_x = conf_msgbox_x;
	*msgbox_y = conf_msgbox_y;
	*msgbox_w = s3_get_image_width(msgbox_image);
	*msgbox_h = s3_get_image_height(msgbox_image);
}

/*
 * Click Animation Control
 */

/*
 * Set the position of the click animation.
 */
void
s3_set_click_position(
	int x,
	int y)
{
	layer_x[S3_LAYER_CLICK] = x;
	layer_y[S3_LAYER_CLICK] = y;
}

/*
 * Show or hides the click animation.
 */
void
s3_show_click(
	bool show)
{
	is_click_visible = show;
}

/*
 * Set the click animation frame index.
 */
void
s3_set_click_index(
	int index)
{
	assert(index >= 0 && index < S3_CLICK_FRAMES);
	assert(index < conf_click_frames);

	layer_image[S3_LAYER_CLICK] = click_image[index];
}

/*
 * Get the click animation frame position and size.
 */
void
s3_get_click_rect(
	int *click_x,
	int *click_y,
	int *click_w,
	int *click_h)
{
	if (layer_image[S3_LAYER_CLICK] == NULL) {
		*click_x = conf_click_x;
		*click_y = conf_click_y;
		*click_w = 0;
		*click_h = 0;
		return;
	}

	*click_x = conf_click_x;
	*click_y = conf_click_y;
	*click_w = s3_get_image_width(layer_image[S3_LAYER_CLICK]);
	*click_h = s3_get_image_height(layer_image[S3_LAYER_CLICK]);
}

/*
 * Choose Box Control
 */

/*
 * Fill the choose box by the choose box bg image.
 */
void
s3_fill_choosebox_idle_image(
	int index)
{
	if (choose_idle_image[index] == NULL)
		return;

	s3_draw_image_copy(layer_image[S3_LAYER_CHOOSE1_IDLE + index * 2],
			   0,
			   0,
			   choose_idle_image[index],
			   choose_idle_image[index]->width,
			   choose_idle_image[index]->height,
			   0,
			   0);
}

/*
 * Fill the choose box by the choose box fg image.
 */
void
s3_fill_choosebox_hover_image(
	int index)
{
	if (choose_hover_image[index] == NULL)
		return;

	s3_draw_image_copy(layer_image[S3_LAYER_CHOOSE1_HOVER + index * 2],
			   0,
			   0,
			   choose_hover_image[index],
			   choose_hover_image[index]->width,
			   choose_hover_image[index]->height,
			   0,
			   0);
}

/*
 * Show or hide the choose box.
 */
void
s3_show_choosebox(
	int index,
	bool show_idle,
	bool show_hover)
{
	int i;

	if (index == -1) {
		for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
			is_choosebox_idle_visible[i] = show_idle;
			is_choosebox_hover_visible[i] = show_hover;
		}
	} else {
		is_choosebox_idle_visible[index] = show_idle;
		is_choosebox_hover_visible[index] = show_hover;
	}
}

/*
 * Get a rect for a choose box.
 */
void
s3_get_choosebox_rect(
	int index,
	int *x,
	int *y,
	int *w,
	int *h)
{
	*x = conf_choose_x[index];
	*y = conf_choose_y[index];
	*w = s3_get_image_width(choose_idle_image[index]);
	*h = s3_get_image_height(choose_idle_image[index]);
}

/*
 * Banner Control
 */

/*
 * Show or hides the auto mode banner.
 */
void
s3_show_automode_banner(
	bool show)
{
	is_auto_visible = show;
}

/*
 * Show or hides the skip mode banner.
 */
void
s3_show_skipmode_banner(
	bool show)
{
	is_skip_visible = show;
}

/*
 * GUI Rendering
 */

/* Render an image. */
void
s3_render_image(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	pf_render_texture(
		dst_left,
		dst_top,
		dst_width,
		dst_height,
		image->tex_id,
		src_left,
		src_top,
		src_width,
		src_height,
		alpha);
}

/* Render a image with free transform. */
void
s3_render_image_3d(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct s3_image *image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	pf_render_texture_3d(x1,
			     y1,
			     x2,
			     y2,
			     x3,
			     y3,
			     x4,
			     y4,
			     image->tex_id,
			     src_left,
			     src_top,
			     src_width,
			     src_height,
			     alpha);
}

/*
 * Kirakira Effect
 */

/*
 * Starts the Kirakira effect.
 */
void
start_kirakira(
	int x,
	int y)
{
	int w, h;

	kirakira_x = x;
	kirakira_y = y;

	if (kirakira_image[0] != NULL) {
		w = kirakira_image[0]->width;
		h = kirakira_image[0]->height;
		kirakira_x -= w / 2;
		kirakira_y -= h / 2;
	}

	s3_reset_lap_timer(&sw_kirakira);
}

/*
 * Renders a Kirakira effect frame.
 */
void
render_kirakira(void)
{
	float lap, frame_time;
	int index;

	frame_time = conf_kirakira_frame == 0 ? 0.333f : conf_kirakira_frame;

	lap = (float)s3_get_lap_timer_millisec(&sw_kirakira) / 1000.0f;
	index = (int)(lap / frame_time);
	if (index < 0 || index >= S3_KIRAKIRA_FRAMES)
		return;
	if (kirakira_image[index] == NULL)
		return;

	if (!conf_kirakira_add_blend) {
		pf_render_texture(kirakira_x,
				  kirakira_y,
				  -1,
				  -1,
				  kirakira_image[index]->tex_id,
				  0,
				  0,
				  -1,
				  -1,
				  255);
	} else {
		pf_render_texture_add(kirakira_x,
				      kirakira_y,
				      -1,
				      -1,
				      kirakira_image[index]->tex_id,
				      0,
				      0,
				      -1,
				      -1,
				      255);
	}
}
