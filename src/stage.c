/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Stage Subsystem
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
#include "stage.h"
#include "image.h"
#include "text.h"
#include "anime.h"
#include "gui.h"
#include "sysbtn.h"

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
static bool layer_dim[S3_STAGE_LAYERS];

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
static void destroy_layer(int layer);
static void render_layer(int layer);
static void render_layers_cross(int fi_layer, int fo_layer);

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

	layer_alpha[S3_LAYER_MSGBOX] = 0;
	layer_alpha[S3_LAYER_NAMEBOX] = 0;
	layer_alpha[S3_LAYER_AUTO] = 0;
	layer_alpha[S3_LAYER_SKIP] = 0;

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
	layer_alpha[S3_LAYER_MSGBOX] = 0;

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
			if (choose_idle_image[i] != NULL &&
			    (!is_hover || index == -1)) {
				s3_destroy_image(choose_idle_image[i]);
				choose_idle_image[i] = NULL;
			}
			if (choose_hover_image[i] != NULL &&
			    (is_hover || index == -1)) {
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
	struct s3_drawmsg *context;
	s3_pixel_t color, outline_color;
	int i, total_chars;

	/* Load the default colors. */
	color = s3_make_pixel(0xff,
			      (uint32_t)conf_msgbox_font_r,
			      (uint32_t)conf_msgbox_font_g,
			      (uint32_t)conf_msgbox_font_b);
	outline_color = s3_make_pixel(0xff,
				      (uint32_t)conf_msgbox_font_outline_r,
				      (uint32_t)conf_msgbox_font_outline_g,
				      (uint32_t)conf_msgbox_font_outline_b);

	for (i = S3_LAYER_TEXT1; i <= S3_LAYER_TEXT8; i++) {
		if (layer_text[i] == NULL)
			continue;

		/* Draw. */
		context = s3_create_drawmsg(
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
		if (context == NULL)
			continue;
		total_chars = s3_count_drawmsg_chars(context, NULL);
		s3_draw_message(context, total_chars);
		s3_destroy_drawmsg(context);
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
 * Get the layer belnd mode.
 */
int
s3_get_layer_blend(
	int layer)
{
	assert(layer >= 0 && layer < S3_STAGE_LAYERS);
	return layer_blend[layer];
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

	if (layer_image[layer] != NULL) {
		s3_destroy_image(layer_image[layer]);
		layer_image[layer] = NULL;
	}

	layer_image[layer] = img;
}

/*
 * Get the layer frame for eye blinking and lip synchronizing.
 */
int
s3_get_layer_frame(
	int layer)
{
	return layer_frame[layer];
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
		case S3_LAYER_BG:	/* fall-thru */
		case S3_LAYER_BG_FO:	/* fall-thru */
		case S3_LAYER_BG2:	/* fall-thru */
		case S3_LAYER_EFB1:	/* fall-thru */
		case S3_LAYER_EFB2:	/* fall-thru */
		case S3_LAYER_EFB3:	/* fall-thru */
		case S3_LAYER_EFB4:	/* fall-thru */
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
		case S3_LAYER_EFF1:	/* fall-thru */
		case S3_LAYER_EFF2:	/* fall-thru */
		case S3_LAYER_EFF3:	/* fall-thru */
		case S3_LAYER_EFF4:	/* fall-thru */
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
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			break;
		case S3_LAYER_MSGBOX:
			s3_set_layer_position(i, conf_msgbox_x, conf_msgbox_y);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_msgbox(false);
			break;
		case S3_LAYER_NAMEBOX:
			s3_set_layer_position(i, conf_namebox_x, conf_namebox_y);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_namebox(false);
			break;
		case S3_LAYER_CLICK:
			s3_set_layer_position(i, conf_click_x, conf_click_y);
			s3_set_layer_alpha(i, 255);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_click(false);
			break;
		case S3_LAYER_AUTO:
			s3_set_layer_position(i, conf_automode_x, conf_automode_y);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_automode_banner(false);
			break;
		case S3_LAYER_SKIP:
			s3_set_layer_position(i, conf_skipmode_x, conf_skipmode_y);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_skipmode_banner(false);
			break;
		case S3_LAYER_CHOOSE1_IDLE:
		case S3_LAYER_CHOOSE2_IDLE:
		case S3_LAYER_CHOOSE3_IDLE:
		case S3_LAYER_CHOOSE4_IDLE:
		case S3_LAYER_CHOOSE5_IDLE:
		case S3_LAYER_CHOOSE6_IDLE:
		case S3_LAYER_CHOOSE7_IDLE:
		case S3_LAYER_CHOOSE8_IDLE:
			s3_set_layer_position(i,
					      conf_choose_x[(i - S3_LAYER_CHOOSE1_IDLE) / 2],
					      conf_choose_y[(i - S3_LAYER_CHOOSE1_IDLE) / 2]);
			s3_set_layer_alpha(i, 255);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_msgbox(false);
			break;
		case S3_LAYER_CHOOSE1_HOVER:
		case S3_LAYER_CHOOSE2_HOVER:
		case S3_LAYER_CHOOSE3_HOVER:
		case S3_LAYER_CHOOSE4_HOVER:
		case S3_LAYER_CHOOSE5_HOVER:
		case S3_LAYER_CHOOSE6_HOVER:
		case S3_LAYER_CHOOSE7_HOVER:
		case S3_LAYER_CHOOSE8_HOVER:
			s3_set_layer_position(i,
					      conf_choose_x[(i - S3_LAYER_CHOOSE1_HOVER) / 2],
					      conf_choose_y[(i - S3_LAYER_CHOOSE1_HOVER) / 2]);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			s3_show_msgbox(false);
			break;
		case S3_LAYER_GUI_BTN1:
		case S3_LAYER_GUI_BTN2:
		case S3_LAYER_GUI_BTN3:
		case S3_LAYER_GUI_BTN4:
		case S3_LAYER_GUI_BTN5:
		case S3_LAYER_GUI_BTN6:
		case S3_LAYER_GUI_BTN7:
		case S3_LAYER_GUI_BTN8:
		case S3_LAYER_GUI_BTN9:
		case S3_LAYER_GUI_BTN10:
		case S3_LAYER_GUI_BTN11:
		case S3_LAYER_GUI_BTN12:
		case S3_LAYER_GUI_BTN13:
		case S3_LAYER_GUI_BTN14:
		case S3_LAYER_GUI_BTN15:
		case S3_LAYER_GUI_BTN16:
		case S3_LAYER_GUI_BTN17:
		case S3_LAYER_GUI_BTN18:
		case S3_LAYER_GUI_BTN19:
		case S3_LAYER_GUI_BTN20:
		case S3_LAYER_GUI_BTN21:
		case S3_LAYER_GUI_BTN22:
		case S3_LAYER_GUI_BTN23:
		case S3_LAYER_GUI_BTN24:
		case S3_LAYER_GUI_BTN25:
		case S3_LAYER_GUI_BTN26:
		case S3_LAYER_GUI_BTN27:
		case S3_LAYER_GUI_BTN28:
		case S3_LAYER_GUI_BTN29:
		case S3_LAYER_GUI_BTN30:
		case S3_LAYER_GUI_BTN31:
		case S3_LAYER_GUI_BTN32:
			s3_set_layer_position(i, 0, 0);
			s3_set_layer_alpha(i, 255);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
			break;
		case S3_LAYER_SYSBTN_IDLE:
		case S3_LAYER_SYSBTN_HOVER:
			s3_set_layer_position(i, conf_sysbtn_x, conf_sysbtn_y);
			s3_set_layer_alpha(i, 0);
			s3_set_layer_scale(i, 1.0f, 1.0f);
			s3_set_layer_center(i, 0, 0);
			s3_set_layer_rotate(i, 0.0f);
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

	/* Update the sysbtn state. */
	s3_update_sysbtn_state();

	/*
	 * Render stage layers.
	 */

	/* BG - Background */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_BG] != NULL &&
	    layer_image[S3_LAYER_BG_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_BG, S3_LAYER_BG_FO);
	} else {
		render_layer(S3_LAYER_BG_FO);
		render_layer(S3_LAYER_BG);
	}

	/* BG2 - Background 2 (for seemless scrolling) */
	render_layer(S3_LAYER_BG2);

	/* EFB - Effect Back */
	for (i = S3_LAYER_EFB1; i <= S3_LAYER_EFB4; i++)
		render_layer(i);

	/* CHB - Character Center */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHB] != NULL &&
	    layer_image[S3_LAYER_CHB_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHB, S3_LAYER_CHB_FO);
	} else {
		render_layer(S3_LAYER_CHB_FO);
		render_layer(S3_LAYER_CHB);
		render_layer(S3_LAYER_CHB_EYE);
		render_layer(S3_LAYER_CHB_LIP);
	}

	/* CHL - Character Left */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHL] != NULL &&
	    layer_image[S3_LAYER_CHL_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHL, S3_LAYER_CHL_FO);
	} else {
		render_layer(S3_LAYER_CHL_FO);
		render_layer(S3_LAYER_CHL);
		render_layer(S3_LAYER_CHL_EYE);
		render_layer(S3_LAYER_CHL_LIP);
	}

	/* CHLC - Character Left Center */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHLC] != NULL &&
	    layer_image[S3_LAYER_CHLC_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHLC, S3_LAYER_CHLC_FO);
	} else {
		render_layer(S3_LAYER_CHLC_FO);
		render_layer(S3_LAYER_CHLC);
		render_layer(S3_LAYER_CHLC_EYE);
		render_layer(S3_LAYER_CHLC_LIP);
	}

	/* CHR - Character Right */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHR] != NULL &&
	    layer_image[S3_LAYER_CHR_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHR, S3_LAYER_CHR_FO);
	} else {
		render_layer(S3_LAYER_CHR_FO);
		render_layer(S3_LAYER_CHR);
		render_layer(S3_LAYER_CHR_EYE);
		render_layer(S3_LAYER_CHR_LIP);
	}

	/* CHRC - Character Right Center */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHRC] != NULL &&
	    layer_image[S3_LAYER_CHRC_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHRC, S3_LAYER_CHRC_FO);
	} else {
		render_layer(S3_LAYER_CHRC_FO);
		render_layer(S3_LAYER_CHRC);
		render_layer(S3_LAYER_CHRC_EYE);
		render_layer(S3_LAYER_CHRC_LIP);
	}

	/* CHC - Character Center */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHC] != NULL &&
	    layer_image[S3_LAYER_CHC_FO] != NULL &&
	    fade_rule_img == NULL) {
		render_layers_cross(S3_LAYER_CHC, S3_LAYER_CHC_FO);
	} else {
		render_layer(S3_LAYER_CHC_FO);
		render_layer(S3_LAYER_CHC);
		render_layer(S3_LAYER_CHC_EYE);
		render_layer(S3_LAYER_CHC_LIP);
	}

	/* EFF - Effect Front */
	for (i = S3_LAYER_EFF1; i <= S3_LAYER_EFF4; i++)
		render_layer(i);

	/* MSGBOX - Message Box */
	render_layer(S3_LAYER_MSGBOX);

	/* NAMEBOX - Name Box */
	if (conf_namebox_enable)
		render_layer(S3_LAYER_NAMEBOX);

	/* CHF - Character Face (for message box face) */
	if (stage_mode == STAGE_MODE_FADE &&
	    layer_image[S3_LAYER_CHF] != NULL &&
	    layer_image[S3_LAYER_CHF_FO] != NULL) {
		render_layers_cross(S3_LAYER_CHF, S3_LAYER_CHF_FO);
	} else {
		render_layer(S3_LAYER_CHF_FO);
		render_layer(S3_LAYER_CHF);
		render_layer(S3_LAYER_CHF_EYE);
		render_layer(S3_LAYER_CHF_LIP);
	}

	/* CLICK - Click Prompt Animation */
	if (is_click_visible)
		render_layer(S3_LAYER_CLICK);

	/* AUTO - Auto Mode Banner */
	render_layer(S3_LAYER_AUTO);

	/* SKIP - Skip Mode Banner */
	render_layer(S3_LAYER_SKIP);

	/* CHOOSEBOX - Choose Option Box */
	for (i = 0; i < S3_CHOOSEBOX_COUNT; i++) {
		/* TODO: Leave it alpha, not a flag. */
		if (is_choosebox_idle_visible[i])
			render_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i));
		if (is_choosebox_hover_visible[i])
			render_layer(S3_LAYER_CHOOSE1_IDLE + (2 * i) + 1);
	}

	/* TEXT - Text Drawing Layers */
	for (i = S3_LAYER_TEXT1; i <= S3_LAYER_TEXT8; i++)
		render_layer(i);

	/* GUI_BUTTON - GUI buttons (offroad to gui.c) */
	if (s3_is_gui_running())
		s3i_run_gui_render();

	/* SYSBTN - System Button */
	render_layer(S3_LAYER_SYSBTN_IDLE);
	render_layer(S3_LAYER_SYSBTN_HOVER);
}

/* Render a layer. */
static void
render_layer(
	int layer)
{
	struct s3_image *layer_img, *base_img;
	int src_x, src_width, src_height;
	int base_layer;

	assert(layer >= 0 && layer < S3_STAGE_LAYERS);

	/* Get the layer image. */
	switch (layer) {
	case S3_LAYER_SYSBTN_IDLE:
		layer_img = sysbtn_idle_image;
		break;
	case S3_LAYER_SYSBTN_HOVER:
		layer_img = sysbtn_hover_image;
		break;
	default:
		layer_img = layer_image[layer];
		break;
	}
	if (layer_img == NULL) {
		/* Don't render if no image. */
		return;
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
		base_layer = s3_chpos_to_layer(layer_chpos);
		base_img = layer_image[base_layer];
		if (base_img == NULL)
			return;
		src_width = base_img->width;
		src_height = layer_img->height;
		src_x = src_width * layer_frame[layer];
	} else {
		src_width = layer_img->width;
		src_height = layer_img->height;
		src_x = 0;
		base_layer = layer;
	}

	/* If 3D. */
	if (layer_rotate[base_layer] != 0 ||
	    layer_scale_x[base_layer] != 1.0f ||
	    layer_scale_y[base_layer] != 1.0f) {
		float x1 = 0;
		float y1 = 0;
		float x2 = (float)src_width;
		float y2 = 0;
		float x3 = 0;
		float y3 = (float)src_height;
		float x4 = (float)src_width;
		float y4 = (float)src_height;
		float center_x = (float)layer_center_x[base_layer] + (float)src_x;
		float center_y = (float)layer_center_y[base_layer];
		float rad = (float)layer_rotate[base_layer];

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
		x1 *= layer_scale_x[base_layer];
		y1 *= layer_scale_y[base_layer];
		x2 *= layer_scale_x[base_layer];
		y2 *= layer_scale_y[base_layer];
		x3 *= layer_scale_x[base_layer];
		y3 *= layer_scale_y[base_layer];
		x4 *= layer_scale_x[base_layer];
		y4 *= layer_scale_y[base_layer];

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
		x1 += (float)layer_x[base_layer];
		y1 += (float)layer_y[base_layer];
		x2 += (float)layer_x[base_layer];
		y2 += (float)layer_y[base_layer];
		x3 += (float)layer_x[base_layer];
		y3 += (float)layer_y[base_layer];
		x4 += (float)layer_x[base_layer];
		y4 += (float)layer_y[base_layer];

		/* Render. */
		if (layer >= S3_LAYER_CHB && layer <= S3_LAYER_CHC && layer_dim[base_layer]) {
			pf_render_texture_3d_dim(x1,
						 y1,
						 x2,
						 y2,
						 x3,
						 y3,
						 x4,
						 y4,
						 layer_img->tex_id,
						 src_x,
						 0,
						 src_width,
						 src_height,
						 layer_alpha[layer]);
		} else if (layer_blend[layer] == S3_BLEND_RULE && fade_rule_img != NULL) {
			/* TODO: 3D transform. */
			pf_render_texture_rule(layer_img->tex_id,
					       fade_rule_img->tex_id,
					       layer_alpha[layer]);
		} else if (layer_blend[layer] == S3_BLEND_MELT && fade_rule_img != NULL) {
			/* TODO: 3D transform. */
			pf_render_texture_rule(layer_img->tex_id,
					       fade_rule_img->tex_id,
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
						 layer_img->tex_id,
						 src_x,
						 0,
						 src_width,
						 src_height,
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
						 layer_img->tex_id,
						 src_x,
						 0,
						 src_width,
						 src_height,
						 layer_alpha[layer]);
		} else {
			pf_render_texture_3d(x1,
					     y1,
					     x2,
					     y2,
					     x3,
					     y3,
					     x4,
					     y4,
					     layer_img->tex_id,
					     src_x,
					     0,
					     src_width,
					     src_height,
					     layer_alpha[layer]);
		}
		return;
	}

	/* Otherwise 2D. */
	if (layer >= S3_LAYER_CHB && layer <= S3_LAYER_CHC && layer_dim[layer]) {
		/* Dim blending. */
		pf_render_texture_dim(layer_x[layer],
				      layer_y[layer],
				      (int)((float)src_width * layer_scale_x[layer]),
				      (int)((float)layer_img->height * layer_scale_y[layer]),
				      layer_img->tex_id,
				      src_x,
				      0,
				      src_width,
				      src_height,
				      layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_RULE && fade_rule_img != NULL) {
		/* Rule transition. */
		pf_render_texture_rule(layer_img->tex_id,
				       fade_rule_img->tex_id,
				       layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_MELT && fade_rule_img != NULL) {
		/* Melt transition. */
		pf_render_texture_melt(layer_img->tex_id,
				       fade_rule_img->tex_id,
				       layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_ADD) {
		/* Add blending. */
		pf_render_texture_add(layer_x[layer],
				      layer_y[layer],
				      (int)((float)src_width * layer_scale_x[layer]),
				      (int)((float)layer_img->height * layer_scale_y[layer]),
				      layer_img->tex_id,
				      src_x,
				      0,
				      src_width,
				      src_height,
				      layer_alpha[layer]);
	} else if (layer_blend[layer] == S3_BLEND_SUB) {
		/* Sub blending. */
		pf_render_texture_sub(layer_x[layer],
				      layer_y[layer],
				      (int)((float)layer_img->width * layer_scale_x[layer]),
				      (int)((float)layer_img->height * layer_scale_y[layer]),
				      layer_img->tex_id,
				      src_x,
				      0,
				      src_width,
				      src_height,
				      layer_alpha[layer]);
	} else {
		/* Normal alpha blending. */
		pf_render_texture(layer_x[layer],
				  layer_y[layer],
				  (int)((float)src_width * layer_scale_x[layer]),
				  (int)((float)layer_img->height * layer_scale_y[layer]),
				  layer_img->tex_id,
				  src_x,
				  0,
				  src_width,
				  src_height,
				  layer_alpha[layer]);
	}
}

/* Render a fade-in layer and a fade-out layer using the cross-fading shader. */
static void
render_layers_cross(
	int fi_layer,
	int fo_layer)
{
	struct s3_image *fi_img;
	struct s3_image *fo_img;
	int alpha;

	assert(fi_layer >= 0 && fi_layer < S3_STAGE_LAYERS);
	assert(fo_layer >= 0 && fo_layer < S3_STAGE_LAYERS);

	fi_img = layer_image[fi_layer];
	if (fi_img == NULL)
		return;

	fo_img = layer_image[fo_layer];
	if (fo_img == NULL)
		return;

	alpha = layer_alpha[fi_layer];

	/*
	 * No 3D transformation needed.
	 * (For speedup on software rendering)
	 */
	if (layer_rotate[fi_layer] == 0.0f &&
	    layer_scale_x[fi_layer] == 1.0f &&
	    layer_scale_y[fi_layer] == 1.0f &&
	    layer_rotate[fo_layer] == 0.0f &&
	    layer_scale_x[fo_layer] == 1.0f &&
	    layer_scale_y[fo_layer] == 1.0f) {
		/* Use the positions. */
		int src1_left = layer_x[fi_layer];
		int src1_top  = layer_y[fi_layer];
		int src2_left = layer_x[fo_layer];
		int src2_top  = layer_y[fo_layer];

		/* Render a composited polygon. */
		pf_render_texture_cross(fi_img->tex_id,
					fo_img->tex_id,
					src1_left,
					src1_top,
					src2_left,
					src2_top,
					alpha);
	} else {
		float src1_x1  = 0.0f;
		float src1_y1  = 0.0f;
		float src1_x2  = (float)fi_img->width;
		float src1_y2  = 0.0f;
		float src1_x3  = 0.0f;
		float src1_y3  = (float)fi_img->height;
		float src1_x4  = (float)fi_img->width;
		float src1_y4  = (float)fi_img->height;
		float src1_cx  = (float)layer_center_x[fi_layer];
		float src1_cy  = (float)layer_center_y[fi_layer];
		float src1_rad = (float)layer_rotate[fi_layer];

		float src2_x1  = 0.0f;
		float src2_y1  = 0.0f;
		float src2_x2  = (float)fo_img->width;
		float src2_y2  = 0.0f;
		float src2_x3  = 0.0f;
		float src2_y3  = (float)fo_img->height;
		float src2_x4  = (float)fo_img->width;
		float src2_y4  = (float)fo_img->height;
		float src2_cx  = (float)layer_center_x[fo_layer];
		float src2_cy  = (float)layer_center_y[fo_layer];
		float src2_rad = (float)layer_rotate[fo_layer];

		/* 1. Shift for the centering. */
		src1_x1 -= src1_cx;
		src1_y1 -= src1_cy;
		src1_x2 -= src1_cx;
		src1_y2 -= src1_cy;
		src1_x3 -= src1_cx;
		src1_y3 -= src1_cy;
		src1_x4 -= src1_cx;
		src1_y4 -= src1_cy;

		src2_x1 -= src2_cx;
		src2_y1 -= src2_cy;
		src2_x2 -= src2_cx;
		src2_y2 -= src2_cy;
		src2_x3 -= src2_cx;
		src2_y3 -= src2_cy;
		src2_x4 -= src2_cx;
		src2_y4 -= src2_cy;

		/* 2. Scale. */
		src1_x1 *= layer_scale_x[fi_layer];
		src1_y1 *= layer_scale_y[fi_layer];
		src1_x2 *= layer_scale_x[fi_layer];
		src1_y2 *= layer_scale_y[fi_layer];
		src1_x3 *= layer_scale_x[fi_layer];
		src1_y3 *= layer_scale_y[fi_layer];
		src1_x4 *= layer_scale_x[fi_layer];
		src1_y4 *= layer_scale_y[fi_layer];

		src2_x1 *= layer_scale_x[fo_layer];
		src2_y1 *= layer_scale_y[fo_layer];
		src2_x2 *= layer_scale_x[fo_layer];
		src2_y2 *= layer_scale_y[fo_layer];
		src2_x3 *= layer_scale_x[fo_layer];
		src2_y3 *= layer_scale_y[fo_layer];
		src2_x4 *= layer_scale_x[fo_layer];
		src2_y4 *= layer_scale_y[fo_layer];

		/* 3. Rotate. */
		if (src1_rad != 0) {
			float tmp_x, tmp_y;

			tmp_x = src1_x1; tmp_y = src1_y1;
			src1_x1 = tmp_x * cosf(src1_rad) - tmp_y * sinf(src1_rad);
			src1_y1 = tmp_x * sinf(src1_rad) + tmp_y * cosf(src1_rad);

			tmp_x = src1_x2; tmp_y = src1_y2;
			src1_x2 = tmp_x * cosf(src1_rad) - tmp_y * sinf(src1_rad);
			src1_y2 = tmp_x * sinf(src1_rad) + tmp_y * cosf(src1_rad);

			tmp_x = src1_x3; tmp_y = src1_y3;
			src1_x3 = tmp_x * cosf(src1_rad) - tmp_y * sinf(src1_rad);
			src1_y3 = tmp_x * sinf(src1_rad) + tmp_y * cosf(src1_rad);

			tmp_x = src1_x4; tmp_y = src1_y4;
			src1_x4 = tmp_x * cosf(src1_rad) - tmp_y * sinf(src1_rad);
			src1_y4 = tmp_x * sinf(src1_rad) + tmp_y * cosf(src1_rad);
		}
		if (src2_rad != 0) {
			float tmp_x, tmp_y;

			tmp_x = src2_x1; tmp_y = src2_y1;
			src2_x1 = tmp_x * cosf(src2_rad) - tmp_y * sinf(src2_rad);
			src2_y1 = tmp_x * sinf(src2_rad) + tmp_y * cosf(src2_rad);

			tmp_x = src2_x2; tmp_y = src2_y2;
			src2_x2 = tmp_x * cosf(src2_rad) - tmp_y * sinf(src2_rad);
			src2_y2 = tmp_x * sinf(src2_rad) + tmp_y * cosf(src2_rad);

			tmp_x = src2_x3; tmp_y = src2_y3;
			src2_x3 = tmp_x * cosf(src2_rad) - tmp_y * sinf(src2_rad);
			src2_y3 = tmp_x * sinf(src2_rad) + tmp_y * cosf(src2_rad);

			tmp_x = src2_x4; tmp_y = src2_y4;
			src2_x4 = tmp_x * cosf(src2_rad) - tmp_y * sinf(src2_rad);
			src2_y4 = tmp_x * sinf(src2_rad) + tmp_y * cosf(src2_rad);
		}

		/* 4. Shift again for the centering. */
		src1_x1 += src1_cx;
		src1_y1 += src1_cy;
		src1_x2 += src1_cx;
		src1_y2 += src1_cy;
		src1_x3 += src1_cx;
		src1_y3 += src1_cy;
		src1_x4 += src1_cx;
		src1_y4 += src1_cy;

		src2_x1 += src2_cx;
		src2_y1 += src2_cy;
		src2_x2 += src2_cx;
		src2_y2 += src2_cy;
		src2_x3 += src2_cx;
		src2_y3 += src2_cy;
		src2_x4 += src2_cx;
		src2_y4 += src2_cy;

		/* 5. Shift for the layer position. */
		src1_x1 += (float)layer_x[fi_layer];
		src1_y1 += (float)layer_y[fi_layer];
		src1_x2 += (float)layer_x[fi_layer];
		src1_y2 += (float)layer_y[fi_layer];
		src1_x3 += (float)layer_x[fi_layer];
		src1_y3 += (float)layer_y[fi_layer];
		src1_x4 += (float)layer_x[fi_layer];
		src1_y4 += (float)layer_y[fi_layer];

		src2_x1 += (float)layer_x[fo_layer];
		src2_y1 += (float)layer_y[fo_layer];
		src2_x2 += (float)layer_x[fo_layer];
		src2_y2 += (float)layer_y[fo_layer];
		src2_x3 += (float)layer_x[fo_layer];
		src2_y3 += (float)layer_y[fo_layer];
		src2_x4 += (float)layer_x[fo_layer];
		src2_y4 += (float)layer_y[fo_layer];

		/* Render. */
		pf_render_texture_3d_cross(fi_img->tex_id,
					   fo_img->tex_id,
					   src1_x1,
					   src1_y1,
					   src1_x2,
					   src1_y2,
					   src1_x3,
					   src1_y3,
					   src1_x4,
					   src1_y4,
					   src2_x1,
					   src2_y1,
					   src2_x2,
					   src2_y2,
					   src2_x3,
					   src2_y3,
					   src2_x4,
					   src2_y4,
					   alpha);
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
	float root_scale_x, root_scale_y;
	float x1, y1, x2, y2, x3, y3, x4, y4, cx, cy, rad;
	int i;

	root_scale_x = (float)conf_save_thumb_width / (float)conf_game_width;
	root_scale_y = (float)conf_save_thumb_height / (float)conf_game_height;

	for (i = 0; i < S3_STAGE_LAYERS; i++) {
		/* Cannot draw if no image.*/
		if (layer_image[i] == NULL)
			continue;

		/* Skip drawing if alpha = 0. */
		if (layer_alpha[i] == 0)
			continue;

		/* Exclude special layers. */
		if (i == S3_LAYER_BG_FO ||
		    i == S3_LAYER_CHB_EYE ||
		    i == S3_LAYER_CHB_LIP ||
		    i == S3_LAYER_CHB_FO ||
		    i == S3_LAYER_CHL_EYE ||
		    i == S3_LAYER_CHL_LIP ||
		    i == S3_LAYER_CHL_FO ||
		    i == S3_LAYER_CHLC_EYE ||
		    i == S3_LAYER_CHLC_LIP ||
		    i == S3_LAYER_CHLC_FO ||
		    i == S3_LAYER_CHC_EYE ||
		    i == S3_LAYER_CHC_LIP ||
		    i == S3_LAYER_CHC_FO ||
		    i == S3_LAYER_CHRC_EYE ||
		    i == S3_LAYER_CHRC_LIP ||
		    i == S3_LAYER_CHRC_FO ||
		    i == S3_LAYER_CHR_EYE ||
		    i == S3_LAYER_CHR_LIP ||
		    i == S3_LAYER_CHR_FO ||
		    i == S3_LAYER_CHF_EYE ||
		    i == S3_LAYER_CHF_LIP ||
		    i == S3_LAYER_CHF_FO ||
		    i == S3_LAYER_CHOOSE1_HOVER ||
		    i == S3_LAYER_CHOOSE2_HOVER ||
		    i == S3_LAYER_CHOOSE3_HOVER ||
		    i == S3_LAYER_CHOOSE4_HOVER ||
		    i == S3_LAYER_CHOOSE5_HOVER ||
		    i == S3_LAYER_CHOOSE6_HOVER ||
		    i == S3_LAYER_CHOOSE7_HOVER ||
		    i == S3_LAYER_CHOOSE8_HOVER ||
		    i == S3_LAYER_CLICK ||
		    i == S3_LAYER_AUTO ||
		    i == S3_LAYER_SKIP ||
		    (i >= S3_LAYER_GUI_BTN1 && i <= S3_LAYER_GUI_BTN32))
			continue;

		/* Do not draw the message box if not visible. */
		if (i == S3_LAYER_MSGBOX && !is_msgbox_visible)
			continue;

		/* Do not draw the name box if not visible. */
		if (i== S3_LAYER_NAMEBOX && (!is_namebox_visible || !conf_namebox_enable))
			continue;

		/* Do not draw the choose boxes if not visible. */
		if (i == S3_LAYER_CHOOSE1_IDLE && !is_choosebox_idle_visible[0])
			continue;
		if (i == S3_LAYER_CHOOSE1_HOVER && !is_choosebox_hover_visible[0])
			continue;
		if (i == S3_LAYER_CHOOSE2_IDLE && !is_choosebox_idle_visible[1])
			continue;
		if (i == S3_LAYER_CHOOSE2_HOVER && !is_choosebox_hover_visible[1])
			continue;
		if (i == S3_LAYER_CHOOSE3_IDLE && !is_choosebox_idle_visible[2])
			continue;
		if (i == S3_LAYER_CHOOSE3_HOVER && !is_choosebox_hover_visible[2])
			continue;
		if (i == S3_LAYER_CHOOSE4_IDLE && !is_choosebox_idle_visible[3])
			continue;
		if (i == S3_LAYER_CHOOSE4_HOVER && !is_choosebox_hover_visible[3])
			continue;
		if (i == S3_LAYER_CHOOSE5_IDLE && !is_choosebox_idle_visible[4])
			continue;
		if (i == S3_LAYER_CHOOSE5_HOVER && !is_choosebox_hover_visible[4])
			continue;
		if (i == S3_LAYER_CHOOSE6_IDLE && !is_choosebox_idle_visible[5])
			continue;
		if (i == S3_LAYER_CHOOSE6_HOVER && !is_choosebox_hover_visible[5])
			continue;
		if (i == S3_LAYER_CHOOSE7_IDLE && !is_choosebox_idle_visible[6])
			continue;
		if (i == S3_LAYER_CHOOSE7_HOVER && !is_choosebox_hover_visible[6])
			continue;
		if (i == S3_LAYER_CHOOSE8_IDLE && !is_choosebox_idle_visible[7])
			continue;
		if (i == S3_LAYER_CHOOSE8_HOVER && !is_choosebox_hover_visible[7])
			continue;

		x1 = 0;
		y1 = 0;
		x2 = (float)layer_image[i]->width * root_scale_x;
		y2 = 0;
		x3 = 0;
		y3 = (float)layer_image[i]->height * root_scale_y;
		x4 = (float)layer_image[i]->width * root_scale_x;
		y4 = (float)layer_image[i]->height * root_scale_y;
		cx = (float)layer_center_x[i] * root_scale_x;
		cy = (float)layer_center_y[i] * root_scale_y;
		rad = (float)layer_rotate[i];

		/* 1. Shift for the centering. */
		x1 -= cx;
		y1 -= cy;
		x2 -= cx;
		y2 -= cy;
		x3 -= cx;
		y3 -= cy;
		x4 -= cx;
		y4 -= cy;

		/* 2. Scale. */
		x1 *= layer_scale_x[i];
		y1 *= layer_scale_y[i];
		x2 *= layer_scale_x[i];
		y2 *= layer_scale_y[i];
		x3 *= layer_scale_x[i];
		y3 *= layer_scale_y[i];
		x4 *= layer_scale_x[i];
		y4 *= layer_scale_y[i];

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
		x1 += cx;
		y1 += cy;
		x2 += cx;
		y2 += cy;
		x3 += cx;
		y3 += cy;
		x4 += cx;
		y4 += cy;

		/* 5. Shift for the layer position. */
		x1 += (float)layer_x[i] * root_scale_x;
		y1 += (float)layer_y[i] * root_scale_y;
		x2 += (float)layer_x[i] * root_scale_x;
		y2 += (float)layer_y[i] * root_scale_y;
		x3 += (float)layer_x[i] * root_scale_x;
		y3 += (float)layer_y[i] * root_scale_y;
		x4 += (float)layer_x[i] * root_scale_x;
		y4 += (float)layer_y[i] * root_scale_y;

		if (layer_dim[i]) {
			pf_draw_texture_3d(thumb_image->tex_id,
					   x1, y1, x2, y2, x3, y3, x4, y4,
					   layer_image[i]->tex_id,
					   0,
					   0,
					   layer_image[i]->width,
					   layer_image[i]->height,
					   layer_alpha[i],
					   S3_BLEND_DIM);
		} else {
			int blend;

			switch (layer_blend[i]) {
			case S3_BLEND_COPY:
			case S3_BLEND_ALPHA:
			case S3_BLEND_GLYPH:
			case S3_BLEND_EMOJI:
				blend = PF_BLEND_ALPHA;
				break;
			case S3_BLEND_ADD:
				blend = PF_BLEND_ADD;
				break;
			case S3_BLEND_SUB:
				blend = PF_BLEND_SUB;
				break;
			case S3_BLEND_DIM:
				blend = PF_BLEND_DIM;
				break;
			default:
				blend = PF_BLEND_ALPHA;
				break;
			}

			pf_draw_texture_3d(thumb_image->tex_id,
					   x1, y1, x2, y2, x3, y3, x4, y4,
					   layer_image[i]->tex_id,
					   0,
					   0,
					   layer_image[i]->width,
					   layer_image[i]->height,
					   layer_alpha[i],
					   blend);
		}
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
	int i, blend;

	assert(stage_mode == STAGE_MODE_IDLE);

	/* Enable the fading mode. */
	stage_mode = STAGE_MODE_FADE;
	fade_method = method;
	fade_rule_img = rule_img;

	/* Get the blend. */
	switch (method) {
	case S3_FADE_RULE:
		blend = S3_BLEND_RULE;
		break;
	case S3_FADE_MELT:
		blend = S3_BLEND_MELT;
		break;
	default:
		blend = S3_BLEND_ALPHA;
		break;
	}

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
			s3_add_anime_sequence_property_f("to-rotate", desc[i].rotate);
			s3_add_anime_sequence_property_i("blend", blend);

			s3_new_anime_sequence(fo_layer);
			s3_add_anime_sequence_property_f("start", 0);
			s3_add_anime_sequence_property_f("end", t);
			s3_add_anime_sequence_property_i("from-x", layer_x[layer]);
			s3_add_anime_sequence_property_i("to-x", layer_x[layer]);
			s3_add_anime_sequence_property_i("from-y", layer_y[layer]);
			s3_add_anime_sequence_property_i("to-y", layer_y[layer]);
			if (blend == S3_BLEND_RULE || blend == S3_BLEND_MELT) {
				s3_add_anime_sequence_property_i("from-a", 255);
				s3_add_anime_sequence_property_i("to-a", 255);
			} else {
				s3_add_anime_sequence_property_i("from-a", layer_alpha[layer]);
				s3_add_anime_sequence_property_i("to-a", 0);
			}
			s3_add_anime_sequence_property_f("from-scale-x", layer_scale_x[layer]);
			s3_add_anime_sequence_property_f("to-scale-x", layer_scale_x[layer]);
			s3_add_anime_sequence_property_f("from-scale-y", layer_scale_y[layer]);
			s3_add_anime_sequence_property_f("to-scale-y", layer_scale_y[layer]);
			s3_add_anime_sequence_property_i("center-x", layer_center_x[layer]);
			s3_add_anime_sequence_property_i("center-y", layer_center_y[layer]);
			s3_add_anime_sequence_property_f("from-rotate", layer_rotate[layer]);
			s3_add_anime_sequence_property_f("to-rotate", layer_rotate[layer]);
			if (blend == S3_BLEND_RULE || blend == S3_BLEND_MELT)
				s3_add_anime_sequence_property_i("blend", S3_BLEND_ALPHA);
			else
				s3_add_anime_sequence_property_i("blend", blend);

			layer_fading[layer] = true;
			layer_fading[fo_layer] = true;

			/* Transfer the existing layer to the fade-out layer. */
			layer_image[fo_layer] = layer_image[layer];
			layer_image[layer] = desc[i].image;

			/* Blend. */
			layer_blend[layer] = method;
			layer_blend[fo_layer] = method;

			/* Dimming. */
			layer_dim[layer] = desc[i].dim;

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
 * Get the offset for the shake command.
 */
void
s3_get_shake_offset(
	int *x,
	int *y)
{
	assert(stage_mode == STAGE_MODE_FADE);

	*x = shake_offset_x;
	*y = shake_offset_y;
}

/*
 * Set the offset for the shake command.
 */
void
s3_set_shake_offset(
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

	uint32_t i;

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
			layer_blend[l] = S3_BLEND_ALPHA;
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
 * Get the talker character name index. (-1 for no speaker)
 */
int
s3_get_ch_talking(void)
{
	return ch_talking;
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
			/* No one is taling. */
			layer_dim[layer] = false;
		} else if (ch_name_mapping[i] == -1) {
			/* This character is unknown. */
			layer_dim[layer] = true;
		} else if (ch_name_mapping[i] == ch_talking) {
			/* This character is known and talking. */
			layer_dim[layer] = false;
		} else if (strncmp(conf_character_folder[ch_name_mapping[i]],
				   conf_character_folder[ch_talking],
				   strlen(conf_character_folder[ch_talking])) == 0) {
			/* This is the same character as talking one. */
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

	s3_draw_image(layer_image[S3_LAYER_NAMEBOX],
		      0,
		      0,
		      namebox_image,
		      0,
		      0,
		      layer_image[S3_LAYER_NAMEBOX]->width,
		      layer_image[S3_LAYER_NAMEBOX]->height,
		      255,
		      S3_BLEND_COPY);
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
	const char *file;
	int i;

	/* Decide an anime to run. */
	file = NULL;
	if (!is_namebox_visible && show)
		file = conf_namebox_anime_show;
	else if (is_namebox_visible && !show)
		file = conf_namebox_anime_hide;

	/* Set the flag. */
	is_namebox_visible = show;

	/* Run an anime. */
	if (file != NULL) {
		/* Stop the anime. */
		s3_clear_layer_anime_sequence(S3_LAYER_NAMEBOX);

		/* Reset the position. */
		s3_set_layer_position(S3_LAYER_NAMEBOX, conf_namebox_x, conf_namebox_y);
		s3_set_layer_scale(S3_LAYER_NAMEBOX, 1.0f, 1.0f);
		s3_set_layer_center(S3_LAYER_NAMEBOX, 0, 0);
		s3_set_layer_rotate(S3_LAYER_NAMEBOX, 0);

		/* Set the argument. ($0 = "namebox") */
		s3_set_call_argument(0, "namebox");
		for (i = 1; i < S3_CALL_ARGS; i++)
			s3_set_call_argument(i, NULL);

		/* Start the anime. */
		s3_load_anime_from_file(file, NULL, NULL);
	}
}

/*
 * Chech if the name box is visible.
 */
bool
s3_is_namebox_visible(void)
{
	return is_msgbox_visible;
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

	s3_draw_image(layer_image[S3_LAYER_MSGBOX],
		      0,
		      0,
		      msgbox_image,
		      0,
		      0,
		      layer_image[S3_LAYER_MSGBOX]->width,
		      layer_image[S3_LAYER_MSGBOX]->height,
		      255,
		      S3_BLEND_COPY);
}

/*
 * Show or hides the message box.
 */
void
s3_show_msgbox(
	bool show)
{
	const char *file;
	int i;

	/* Decide an anime to run. */
	file = NULL;
	if (!is_msgbox_visible && show)
		file = conf_msgbox_anime_show;
	else if (is_msgbox_visible && !show)
		file = conf_msgbox_anime_hide;

	/* Set the flag. */
	is_msgbox_visible = show;

	/* Run an anime. */
	if (file != NULL) {
		/* Stop the anime. */
		s3_clear_layer_anime_sequence(S3_LAYER_MSGBOX);

		/* Reset the position. */
		s3_set_layer_position(S3_LAYER_MSGBOX, conf_msgbox_x, conf_msgbox_y);
		s3_set_layer_scale(S3_LAYER_MSGBOX, 1.0f, 1.0f);
		s3_set_layer_center(S3_LAYER_MSGBOX, 0, 0);
		s3_set_layer_rotate(S3_LAYER_MSGBOX, 0);

		/* Set the argument. ($0 = "msgbox") */
		s3_set_call_argument(0, "msgbox");
		for (i = 1; i < S3_CALL_ARGS; i++)
			s3_set_call_argument(i, NULL);

		/* Start the anime. */
		s3_load_anime_from_file(file, NULL, NULL);
	}
}

/*
 * Chech if the message box is visible.
 */
bool
s3_is_msgbox_visible(void)
{
	return is_msgbox_visible;
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

	s3_draw_image(layer_image[S3_LAYER_CHOOSE1_IDLE + index * 2],
		      0,
		      0,
		      choose_idle_image[index],
		      0,
		      0,
		      choose_idle_image[index]->width,
		      choose_idle_image[index]->height,
		      255,
		      S3_BLEND_COPY);
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

	s3_draw_image(layer_image[S3_LAYER_CHOOSE1_HOVER + index * 2],
		      0,
		      0,
		      choose_hover_image[index],
		      0,
		      0,
		      choose_hover_image[index]->width,
		      choose_hover_image[index]->height,
		      255,
		      S3_BLEND_COPY);
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
	const char *file;
	int i;

	/* Decide an anime to run. */
	file = NULL;
	if (!is_auto_visible && show)
		file = conf_automode_anime_show;
	else if (is_auto_visible && !show)
		file = conf_automode_anime_hide;

	/* Set the flag. */
	is_auto_visible = show;

	/* Run an anime. */
	if (file != NULL) {
		/* Stop the anime. */
		s3_clear_layer_anime_sequence(S3_LAYER_AUTO);

		/* Reset the position. */
		s3_set_layer_position(S3_LAYER_AUTO, conf_automode_x, conf_automode_y);
		s3_set_layer_scale(S3_LAYER_AUTO, 1.0f, 1.0f);
		s3_set_layer_center(S3_LAYER_AUTO, 0, 0);
		s3_set_layer_rotate(S3_LAYER_AUTO, 0);

		/* Set the argument. ($0 = "auto") */
		s3_set_call_argument(0, "auto");
		for (i = 1; i < S3_CALL_ARGS; i++)
			s3_set_call_argument(i, NULL);

		/* Start the anime. */
		s3_load_anime_from_file(file, NULL, NULL);
	}
}

/*
 * Show or hides the skip mode banner.
 */
void
s3_show_skipmode_banner(
	bool show)
{
	const char *file;
	int i;

	/* Decide an anime to run. */
	file = NULL;
	if (!is_skip_visible && show)
		file = conf_skipmode_anime_show;
	else if (is_skip_visible && !show)
		file = conf_skipmode_anime_hide;

	/* Set the flag. */
	is_skip_visible = show;

	/* Run an anime. */
	if (file != NULL) {
		/* Stop the anime. */
		s3_clear_layer_anime_sequence(S3_LAYER_SKIP);

		/* Reset the position. */
		s3_set_layer_position(S3_LAYER_SKIP, conf_skipmode_x, conf_skipmode_y);
		s3_set_layer_scale(S3_LAYER_SKIP, 1.0f, 1.0f);
		s3_set_layer_center(S3_LAYER_SKIP, 0, 0);
		s3_set_layer_rotate(S3_LAYER_SKIP, 0);

		/* Set the argument. ($0 = "skip") */
		s3_set_call_argument(0, "skip");
		for (i = 1; i < S3_CALL_ARGS; i++)
			s3_set_call_argument(i, NULL);

		/* Start the anime. */
		s3_load_anime_from_file(file, NULL, NULL);
	}
}

/*
 * GUI Rendering
 */

/*
 * Render an image.
 */
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
	int alpha,
	int blend)
{
	switch (blend) {
	case S3_BLEND_ALPHA:
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
		break;
	case S3_BLEND_ADD:
		pf_render_texture_add(
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
		break;
	case S3_BLEND_SUB:
		pf_render_texture_sub(
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
		break;
	}		
}

/*
 * Render a image with free transform.
 */
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
	int alpha,
	int blend)
{
	switch (blend) {
	case S3_BLEND_ALPHA:
		pf_render_texture_3d(
			x1,
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
		break;
	case S3_BLEND_ADD:
		pf_render_texture_3d_add(
			x1,
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
		break;
	case S3_BLEND_SUB:
		pf_render_texture_3d_sub(
			x1,
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
		break;
	}
}

/*
 * Get the save-new image
 */
struct s3_image *
s3i_get_savenew_image(void)
{
	return savenew_image;
}

/*
 * Kirakira Effect
 */

/*
 * Starts the Kirakira effect.
 */
void
s3i_start_kirakira(
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
s3i_render_kirakira(void)
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
