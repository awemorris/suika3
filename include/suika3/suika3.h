/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Public C Interface
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

#ifndef SUIKA3_SUIKA3_H
#define SUIKA3_SUIKA3_H

#include "playfield/c89compat.h"

/*
 * Constants
 */

/*
 * Basic Paths
 */

/*
 * Name of the first tag file.
 */
#define S3_PATH_START_TAG	"start.novel"

/*
 * Name of the config file.
 */
#define S3_PATH_CONFIG		"config.ini"

/*
 * Name of the system menu GUI file.
 */
#define S3_PATH_SYSMENU_GUI	"system/sysmenu/sysmenu.gui"

/*
 * Name of the save GUI file.
 */
#define S3_PATH_SAVE_GUI	"system/save/save.gui"

/*
 * Name of the load GUI file.
 */
#define S3_PATH_LOAD_GUI	"system/load/load.gui"

/*
 * Name of the history GUI file.
 */
#define S3_PATH_HISTORY_GUI	"system/history/history.gui"

/*
 * Name of the config GUI file.
 */
#define S3_PATH_CONFIG_GUI	"system/config/config.gui"


/*
 * Constants for Stage Subsystem (stage.c)
 */

/*
 * Stage layers.
 */
#define S3_LAYER_BG		(0)	/* Background */
#define S3_LAYER_BG_FO		(1)	/*   Fade-Out */
#define S3_LAYER_BG2		(2)	/* Background2 */
#define S3_LAYER_EFB1		(3)	/* Back effect 1 */
#define S3_LAYER_EFB2		(4)	/* Back effect 2 */
#define S3_LAYER_EFB3		(5)	/* Back effect 3 */
#define S3_LAYER_EFB4		(6)	/* Back effect 4 */
#define S3_LAYER_CHB		(7)	/* Character "back" */
#define S3_LAYER_CHB_EYE	(8)	/*   Eyes */
#define S3_LAYER_CHB_LIP	(9)	/*   Lips */
#define S3_LAYER_CHB_FO		(10)	/*   Fade-Out */
#define S3_LAYER_CHL		(11)	/* Character "left" */
#define S3_LAYER_CHL_EYE	(12)	/*   Eyes */
#define S3_LAYER_CHL_LIP	(13)	/*   Lips */
#define S3_LAYER_CHL_FO		(14)	/*   Fade-Out */
#define S3_LAYER_CHLC		(15)	/* Character "left-center" */
#define S3_LAYER_CHLC_EYE	(16)	/*   Eyes */
#define S3_LAYER_CHLC_LIP	(17)	/*   Lips */
#define S3_LAYER_CHLC_FO	(18)	/*   Fade-Out */
#define S3_LAYER_CHR		(19)	/* Character "right" */
#define S3_LAYER_CHR_EYE	(20)	/*   Eyes */
#define S3_LAYER_CHR_LIP	(21)	/*   Lips */
#define S3_LAYER_CHR_FO		(22)	/*   Fade-Out */
#define S3_LAYER_CHRC		(23)	/* Character "right-center" */
#define S3_LAYER_CHRC_EYE	(24)	/*   Eyes */
#define S3_LAYER_CHRC_LIP	(25)	/*   Lips */
#define S3_LAYER_CHRC_FO	(26)	/*   Fade-Out */
#define S3_LAYER_CHC		(27)	/* Character "right-center" */
#define S3_LAYER_CHC_EYE	(28)	/*   Eyes */
#define S3_LAYER_CHC_LIP	(29)	/*   Lips */
#define S3_LAYER_CHC_FO		(30)	/*   Fade-Out */
#define S3_LAYER_EFF1		(31)	/* Front effect 1 */
#define S3_LAYER_EFF2		(32)	/* Front effect 2 */
#define S3_LAYER_EFF3		(33)	/* Front effect 3 */
#define S3_LAYER_EFF4		(34)	/* Front effect 4 */
#define S3_LAYER_MSGBOX		(35)	/* Message box (text layer) */
#define S3_LAYER_NAMEBOX	(36)	/* Name box (text layer) */
#define S3_LAYER_CHOOSE1_IDLE	(37)	/* Choose box 1 (idle, text layer) */
#define S3_LAYER_CHOOSE1_HOVER	(38)	/* Choose box 1 (hover, text layer) */
#define S3_LAYER_CHOOSE2_IDLE	(39)	/* Choose box 2 (idle, text layer) */
#define S3_LAYER_CHOOSE2_HOVER	(40)	/* Choose box 2 (hover, text layer) */
#define S3_LAYER_CHOOSE3_IDLE	(41)	/* Choose box 3 (idle, text layer) */
#define S3_LAYER_CHOOSE3_HOVER	(42)	/* Choose box 3 (hover, text layer) */
#define S3_LAYER_CHOOSE4_IDLE	(43)	/* Choose box 4 (idle, text layer) */
#define S3_LAYER_CHOOSE4_HOVER	(44)	/* Choose box 4 (hover, text layer) */
#define S3_LAYER_CHOOSE5_IDLE	(45)	/* Choose box 5 (idle, text layer) */
#define S3_LAYER_CHOOSE5_HOVER	(46)	/* Choose box 5 (hover, text layer) */
#define S3_LAYER_CHOOSE6_IDLE	(47)	/* Choose box 6 (idle, text layer) */
#define S3_LAYER_CHOOSE6_HOVER	(48)	/* Choose box 6 (hover, text layer) */
#define S3_LAYER_CHOOSE7_IDLE	(49)	/* Choose box 7 (idle, text layer) */
#define S3_LAYER_CHOOSE7_HOVER	(50)	/* Choose box 7 (hover, text layer) */
#define S3_LAYER_CHOOSE8_IDLE	(51)	/* Choose box 8 (idle, text layer) */
#define S3_LAYER_CHOOSE8_HOVER	(52)	/* Choose box 8 (hover, text layer) */
#define S3_LAYER_CHF		(53)	/* Character "face" */
#define S3_LAYER_CHF_EYE	(54)	/*   Eyes */
#define S3_LAYER_CHF_LIP	(55)	/*   Lips */
#define S3_LAYER_CHF_FO		(56)	/*   Fade-Out */
#define S3_LAYER_CLICK		(57)	/* Click animation */
#define S3_LAYER_AUTO		(58)	/* Auto mode banner */
#define S3_LAYER_SKIP		(59)	/* Skip mode banner */
#define S3_LAYER_TEXT1		(60)	/* Text layer 1 */
#define S3_LAYER_TEXT2		(61)	/* Text layer 2 */
#define S3_LAYER_TEXT3		(62)	/* Text layer 3 */
#define S3_LAYER_TEXT4		(63)	/* Text layer 4 */
#define S3_LAYER_TEXT5		(64)	/* Text layer 5 */
#define S3_LAYER_TEXT6		(65)	/* Text layer 6 */
#define S3_LAYER_TEXT7		(66)	/* Text layer 7 */
#define S3_LAYER_TEXT8		(67)	/* Text layer 8 */
#define S3_LAYER_GUI_BTN1	(68)	/* GUI button 1 */
#define S3_LAYER_GUI_BTN2	(69)	/* GUI button 2 */
#define S3_LAYER_GUI_BTN3	(70)	/* GUI button 3 */
#define S3_LAYER_GUI_BTN4	(71)	/* GUI button 4 */
#define S3_LAYER_GUI_BTN5	(72)	/* GUI button 5 */
#define S3_LAYER_GUI_BTN6	(73)	/* GUI button 6 */
#define S3_LAYER_GUI_BTN7	(74)	/* GUI button 7 */
#define S3_LAYER_GUI_BTN8	(75)	/* GUI button 8 */
#define S3_LAYER_GUI_BTN9	(76)	/* GUI button 9 */
#define S3_LAYER_GUI_BTN10	(77)	/* GUI button 10 */
#define S3_LAYER_GUI_BTN11	(78)	/* GUI button 11 */
#define S3_LAYER_GUI_BTN12	(79)	/* GUI button 12 */
#define S3_LAYER_GUI_BTN13	(80)	/* GUI button 13 */
#define S3_LAYER_GUI_BTN14	(81)	/* GUI button 14 */
#define S3_LAYER_GUI_BTN15	(82)	/* GUI button 15 */
#define S3_LAYER_GUI_BTN16	(83)	/* GUI button 16 */
#define S3_LAYER_GUI_BTN17	(84)	/* GUI button 17 */
#define S3_LAYER_GUI_BTN18	(85)	/* GUI button 18 */
#define S3_LAYER_GUI_BTN19	(86)	/* GUI button 19 */
#define S3_LAYER_GUI_BTN20	(87)	/* GUI button 20 */
#define S3_LAYER_GUI_BTN21	(88)	/* GUI button 21 */
#define S3_LAYER_GUI_BTN22	(89)	/* GUI button 22 */
#define S3_LAYER_GUI_BTN23	(90)	/* GUI button 23 */
#define S3_LAYER_GUI_BTN24	(91)	/* GUI button 24 */
#define S3_LAYER_GUI_BTN25	(92)	/* GUI button 25 */
#define S3_LAYER_GUI_BTN26	(93)	/* GUI button 26 */
#define S3_LAYER_GUI_BTN27	(94)	/* GUI button 27 */
#define S3_LAYER_GUI_BTN28	(95)	/* GUI button 28 */
#define S3_LAYER_GUI_BTN29	(96)	/* GUI button 29 */
#define S3_LAYER_GUI_BTN30	(97)	/* GUI button 30 */
#define S3_LAYER_GUI_BTN31	(98)	/* GUI button 31 */
#define S3_LAYER_GUI_BTN32	(99)	/* GUI button 32 */
#define S3_STAGE_LAYERS		(100)	/* Total layers */

/*
 * Number of the text layers.
 */
#define S3_TEXT_LAYERS		(8)

/*
 * Number of the effect layers.
 */
#define S3_EFFECT_LAYERS	(4)

/*
 * Number of the GUI button layers.
 */
#define S3_BUTTON_LAYERS	(32)

/*
 * Maximum number of the click animation frames.
 */
#define S3_CLICK_FRAMES		(16)

/*
 * Number of the choose box.
 */
#define S3_CHOOSEBOX_COUNT	(8)

/*
 * Character positions.
 */
#define S3_CH_BACK		(0)
#define S3_CH_LEFT		(1)
#define S3_CH_LEFT_CENTER	(2)
#define S3_CH_RIGHT		(3)
#define S3_CH_RIGHT_CENTER	(4)
#define S3_CH_CENTER		(5)
#define S3_CH_FACE		(6)
#define S3_CH_BASIC_LAYERS	(6)	/* Excluding S3_CH_FACE */
#define S3_CH_ALL_LAYERS	(7)	/* Including S3_CH_FACE */

/*
 * Fading methods.
 */
#define S3_FADE_INVALID		(0)
#define S3_FADE_NORMAL		(1)
#define S3_FADE_RULE		(2)
#define S3_FADE_MELT		(3)

/*
 * Blending mode.
 */
#define S3_BLEND_ALPHA		(0)
#define S3_BLEND_ADD		(1)
#define S3_BLEND_SUB		(2)

/*
 * Fading descriptor index.
 */
#define S3_FADE_DESC_BG		(0)
#define S3_FADE_DESC_CHB	(1)
#define S3_FADE_DESC_CHL	(2)
#define S3_FADE_DESC_CHLC	(3)
#define S3_FADE_DESC_CHR	(4)
#define S3_FADE_DESC_CHRC	(5)
#define S3_FADE_DESC_CHC	(6)
#define S3_FADE_DESC_CHF	(7)
#define S3_FADE_DESC_COUNT	(8)

/*
 * Kira Kira Effect frames.
 */
#define S3_KIRAKIRA_FRAMES	(16)


/*
 * Constants for Mixer Subsystem (mixer.c)
 */

/*
 * Mixer tracks.
 */
#define S3_MIXER_TRACKS		(4)
#define S3_TRACK_BGM		(0)
#define S3_TRACK_VOICE		(1)
#define S3_TRACK_SE		(2)
#define S3_TRACK_SYS		(3)

/*
 * Per-Character volume slots.
 */
#define S3_CH_VOL_SLOTS		(33)
#define S3_CH_VOL_SLOT_DEFAULT	(0)

/*
 * Character map count.
 */
#define S3_CHARACTER_MAP_COUNT	(32)


/*
 * Constants for Anime Subsystem (anime.c)
 */

/*
 * Maximum amount of registered anime files.
 */
#define S3_REG_ANIME_COUNT	(16)

/*
 * Acceleration types.
 */
#define S3_ANIME_ACCEL_UNIFORM		(0)
#define S3_ANIME_ACCEL_ACCEL		(1)
#define S3_ANIME_ACCEL_DEACCEL		(2)
#define S3_ANIME_ACCEL_SMOOTHSTEP	(3)
#define S3_ANIME_ACCEL_INVSMOOTHSTEP	(4)


/*
 * Constants for Text Subsystem (text.c)
 */

/*
 * Font selection.
 */
#define S3_FONT_SELECT1		(0)
#define S3_FONT_SELECT2		(1)
#define S3_FONT_SELECT3		(2)
#define S3_FONT_SELECT4		(3)
#define S3_FONT_COUNT		(4)

/*
 * Emoji count.
 */
#define S3_EMOJI_COUNT		(32)


/*
 * Constants for Game Subsystem (game.c)
 */

/*
 * Call Arguments (For GUI and anime)
 */
#define S3_CALL_ARGS		(32)


/*
 * Constants for Save Subsystem (save.c)
 */

/*
 * Numbers of the save slots, and the special save index.
 */
#define S3_ALL_SAVE_SLOTS	(101)
#define S3_NORMAL_SAVE_SLOTS	(100)
#define S3_QUICK_SAVE_INDEX	(100)


/*
 * Message Translation
 */
#if !defined(S3_TR)
#if defined(USE_TRANSLATION)
#define S3_TR(s)	s3_gettext(s)
const char *s3_gettext(const char *msg);
#else
#define S3_TR(s)	(s)
#endif
#endif


/*
 * Types
 */

/*
 * Types for Image Subsystem (image.c)
 */

/*
 * Pixel.
 */
typedef uint32_t s3_pixel_t;

/*
 * Image.
 */
struct s3_image;


/*
 * Types for Stage Subsystem (stage.c)
 */

/*
 * Fade descriptor.
 */
struct s3_fade_desc {
	bool stay;
	const char *fname;
	struct s3_image *image;
	int x;
	int y;
	int alpha;
	float scale_x;
	float scale_y;
	int center_x;
	int center_y;
	float rotate;
	bool dim;
};


/*
 * Types for Text Subsystem (text.c)
 */

/*
 * Context for message drawing.
 */
struct s3_draw_msg_context;


/*
 * Functions
 */

/*
 * Functions for Config Subsystem (conf.c)
 */

/*
 * Set a config.
 */
bool
s3_set_config(
	const char *key,
	const char *val);

/*
 * Get the number of the config keys.
 */
int
s3_get_config_count(void);

/*
 * Get a config key for index.
 */
const char *
s3_get_config_key(
	int index);

/*
 * Check if config key is stored to global save data.
 */
bool
s3_is_config_global(
	const char *key);

/*
 * Get a config value type. ('s', 'b', 'i', 'f')
 */
char
s3_get_config_type(
	const char *key);

/*
 * Get a string config value.
 */
const char *
s3_get_config_string(
	const char *key);

/*
 * Get a boolean config value.
 */
bool
s3_get_config_bool(
	const char *key);

/*
 * Get an integer config value.
 */
int
s3_get_int_config_value(
	const char *key);

/*
 * Get a float config value.
 */
float
s3_get_float_config_value(
	const char *key);

/*
 * Get a config value as a string.
 */
const char *
s3_get_config_as_string(
	const char *key);

/*
 * Check if the specified locale is same as the current locale.
 */
bool
s3_compare_locale(
	const char *s);


/*
 * Functions for Input State (game.c)
 */

/*
 * Get the mouse X position.
 */
int
s3_get_mouse_pos_x(void);

/*
 * Get the mouse Y position.
 */
int
s3_get_mouse_pos_y(void);

/*
 * Check if mouse left button is pressed.
 */
bool
s3_is_mouse_left_pressed(void);

/*
 * Check if mouse right button is pressed.
 */
bool
s3_is_mouse_right_pressed(void);

/*
 * Check if mouse left button is pressed then released.
 */
bool
s3_is_mouse_left_clicked(void);

/*
 * Check if mouse right button is pressed then released.
 */
bool
s3_is_mouse_right_clicked(void);

/*
 * Check if mouse is dragging.
 */
bool
s3_is_mouse_dragging(void);

/*
 * Check if return key is pressed.
 */
bool
s3_is_return_key_pressed(void);

/* Check if space key is pressed. */
bool
s3_is_space_key_pressed(void);

/*
 * Check if escape key is pressed.
 */
bool
s3_is_escape_key_pressed(void);

/*
 * Check if up key is pressed.
 */
bool
s3_is_up_key_pressed(void);

/*
 * Check if down key is pressed.
 */
bool
s3_is_down_key_pressed(void);

/*
 * Check if left key is pressed.
 */
bool
s3_is_left_key_pressed(void);

/*
 * Check if right key is pressed.
 */
bool
s3_is_right_key_pressed(void);

/*
 * Check if pageup key is pressed.
 */
bool
s3_is_pageup_key_pressed(void);

/*
 * Check if pagedown key is pressed.
 */
bool
s3_is_pagedown_key_pressed(void);

/*
 * Check if control key is pressed.
 */
bool
s3_is_control_key_pressed(void);

/*
 * Check if S key is pressed.
 */
bool
s3_is_s_key_pressed(void);

/*
 * Check if L key is pressed.
 */
bool
s3_is_l_key_pressed(void);

/*
 * Check if H key is pressed.
 */
bool
s3_is_h_key_pressed(void);

/*
 * Check if touch is canceled.
 */
bool
s3_is_touch_canceled(void);

/*
 * Check if swiped.
 */
bool
s3_is_swiped(void);

/*
 * Clear input states to avoid further input processing in the current frame.
 */
void
s3_clear_input_state(void);


/*
 * Functions for Game Subsystem. (game.c)
 */

/*
 * Start a multiple-frame command execution.
 */
void
s3_start_command_repetition(void);

/*
 * Stop a multiple-frame command execution.
 */
void
s3_stop_command_repetition(void);

/*
 * Check whether we are in a multiple-frame command execution or not.
 */
bool
s3_is_in_command_repetition(void);

/*
 * Set the message showing state.
 *  - Set active when a message shown.
 *  - Keep active when move to a GUI.
 *  - Cleared when move to a next command or loaded a save file.
 */
void
s3_set_message_active(void);

/*
 * Reset the message showing state.
 */
void
s3_clear_message_active(void);

/*
 * Check whether the message showing state is set or not.
 */
bool
s3_is_message_active(void);

/*
 * Start the auto-mode.
 */
void
s3_start_auto_mode(void);

/*
 * Stop the auto-mode.
 */
void
s3_stop_auto_mode(void);

/*
 * Check whether we are in the auto-mode or not.
 */
bool
s3_is_auto_mode(void);

/*
 * Start the skip-mode.
 */
void
s3_start_skip_mode(void);

/*
 * Stop the skip-mode.
 */
void
s3_stop_skip_mode(void);

/*
 * Check whether we are in the skip-mode or not.
 */
bool
s3_is_skip_mode(void);

/*
 * Set the save/load enable setting.
 */
void
s3_set_save_load(bool enable);

/*
 * Get the save/load enable setting.
 */
bool
s3_is_save_load_enabled(void);

/*
 * Set the non-interruptible mode setting.
 */
void
s3_set_non_interruptible(
	bool mode);

/*
 * Get the non-interruptible mode setting.
 */
bool
s3_is_non_interruptible(void);

/*
 * Set the pen position.
 */
void
s3_set_pen_position(
	int x,
	int y);

/*
 * Get the pen X position.
 */
int
s3_get_pen_position_x(void);

/*
 * Get the pen Y position.
 */
int
s3_get_pen_position_y(void);

/*
 * Set the tag index for return destination.
 */
void
s3_set_return_index(
	int index);

/*
 * Get the tag index for return destination.
 */
int
s3_get_return_index(void);

/*
 * Set a calling argument.
 */
bool
s3_set_call_argument(
	int index,
	const char *val);

/*
 * Get a calling argument.
 */
const char *
s3_get_call_argument(
	int index);

/*
 * Set the script page mode.
 */
bool
s3_is_page_mode(void);

/*
 * Append a string to the page mode buffer string.
 */
bool
s3_append_buffered_message(
	const char *msg);

/*
 * Get the page mode buffer string.
 */
const char *
s3_get_buffered_message(void);

/*
 * Clear the page mode buffer string.
 */
void
s3_clear_buffered_message(void);

/*
 * Reset the message line count in a page.
 */
void
s3_reset_page_line(void);

/*
 * Increment the line count in a page.
 */
void
s3_inc_page_line(void);

/*
 * Check we are at the first line in a page.
 */
bool
s3_is_page_top(void);

/*
 * Register a BGVoice.
 */
bool
s3_register_bgvoice(
	const char *file);

/*
 * Get the BGVoice.
 */
const char *
s3_get_bgvoice(void);

/*
 * Set the BGVoice state playing.
 */
void
s3_set_bgvoice_playing(
	bool is_playing);

/*
 * Check whether the BGVoice is playing or not.
 */
bool
s3_is_bgvoice_playing(void);

/*
 * Set the chapter name.
 */
bool
s3_set_chapter_name(
	const char *name);

/*
 * Get the chapter name.
 */
const char *
s3_get_chapter_name(void);

/*
 * Set the last message.
 */
bool
s3_set_last_message(
	const char *msg,
	bool is_append);

/*
 * Set the previous last message.
 */
bool
s3_set_prev_last_message(
	const char *msg);

/*
 * Get the last message.
 */
const char *
s3_get_last_message(void);

/*
 * Get the previous last message.
 */
const char *
s3_get_prev_last_message(void);

/*
 * Set the text speed.
 */
void
s3_set_text_speed(
	float val);

/*
 * Get the text speed.
 */
float
s3_get_text_speed(void);

/*
 * Set the auto speed.
 */
void
s3_set_auto_speed(
	float val);

/*
 * Get the auto speed.
 */
float
s3_get_auto_speed(void);

/*
 * Mark the last English index.
 */
void
s3_mark_last_english_tag_index(void);

/*
 * Get the last English index.
 */
int
s3_get_last_english_tag_index(void);

/*
 * Clear the last English index.
 */
void
s3_clear_last_english_tag_index(void);

/*
 * Get the last tag name.
 */
const char *
s3_get_last_tag_name(void);


/*
 * Functions for Image Subsystem (image.c)
 */

/*
 * Load an image from a file.
 */
struct s3_image *
s3_create_image_from_file(
	const char *file);

/*
 * Create an image.
 */
struct s3_image *
s3_create_image(int width,
		int height);

/*
 * Get the width of an image.
 */
int
s3_get_image_width(
	struct s3_image *img);

/*
 * Get the height of an image.
 */
int
s3_get_image_height(
	struct s3_image *img);

/*
 * Load a glyph image.
 */
struct s3_image *
s3_load_glyph_image(
	int slot,
	int codepoint,
	int size,
	s3_pixel_t color,
	int outline_width,
	s3_pixel_t outline_color);

/*
 * Destroy an image.
 */
void s3_destroy_image(
	struct s3_image *image);

/*
 * Notify an update of an image.
 */
void s3_notify_image_update(
	struct s3_image *image);

/*
 * Copy an image to an image.
 */
void s3_draw_image_copy(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	struct s3_image *src,
	int dst_width,
	int dst_height,
	int src_left,
	int src_top);

/*
 * Draw an image to an image with alpha blending.
 */
void s3_draw_image_alpha(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image to an image with add blending.
 */
void s3_draw_image_add(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image to an image with sub blending.
 */
void s3_draw_image_sub(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image to an image with sub blending.
 */
void s3_draw_image_dim(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a glyph image to an image.
 */
void
s3_draw_image_glyph(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an emoji image to an image.
 */
void
s3_draw_image_emoji(
	struct s3_image *dst,
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct s3_image *src,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an image with scaling.
 */
void
s3_draw_image_scale(
	struct s3_image *dst,
	int virtual_dst_width,
	int virtual_dst_height,
	int virtual_dst_left,
	int virtual_dst_top,
	struct s3_image *src);

/*
 * Make a pixel value.
 */
s3_pixel_t
s3_make_pixel(
	int r,
	int g,
	int b,
	int a);

/*
 * Fill a rectangle on an image.
 */
void
s3_fill_image_rect(
	struct s3_image *image,
	int left,
	int top,
	int width,
	int height,
	s3_pixel_t color);

/*
 * Get the raw pixel pointer of an image.
 */
s3_pixel_t *
s3_get_image_pixels(
	struct s3_image *image);

/*
 * Stage Subsystem (stage.c)
 */

/*
 * Reload the stage images by the config.
 */
bool
s3_reload_stage_images(void);

/*
 * Reload the stage positions by the config.
 */
void
s3_reload_stage_positions(void);

/*
 * Get the layer X position.
 */
int
s3_get_layer_x(
	int layer);

/*
 * Get the layer Y position.
 */
int
s3_get_layer_y(
	int layer);

/*
 * Set the layer position.
 */
void
s3_set_layer_position(
	int layer,
	int x,
	int y);

/*
 * Get the layer X scale.
 */
float
s3_get_layer_scale_x(
	int layer);

/*
 * Get the layer X scale.
 */
float
s3_get_layer_scale_y(
	int layer);

/*
 * Set the layer scale.
 */
void
s3_set_layer_scale(
	int layer,
	float scale_x,
	float scale_y);

/*
 * Get the layer center X position.
 */
int
s3_get_layer_center_x(
	int layer);

/*
 * Get the layer center Y position.
 */
int
s3_get_layer_center_y(
	int layer);

/*
 * Set the layer center position.
 */
void
s3_set_layer_center(
	int layer,
	int center_x,
	int center_y);

/*
 * Get the layer rotate
 */
float
s3_get_layer_rotate(
	int layer);

/*
 * Set the layer center position.
 */
void
s3_set_layer_rotate(
	int layer,
	float rot);

/*
 * Get the layer dim state.
 */
bool
s3_get_layer_dim(
	int layer);

/*
 * Set the layer dim state.
 */
void
s3_set_layer_dim(
	int layer,
	bool dim);

/*
 * Get the layer image width.
 */
int
s3_get_layer_width(
	int layer);

/*
 * Get the layer image height.
 */
int
s3_get_layer_height(
	int layer);

/*
 * Get the layer alpha.
 */
int
s3_get_layer_alpha(
	int layer);

/*
 * Set the layer alpha.
 */
void
s3_set_layer_alpha(
	int layer,
	int alpha);

/*
 * Set the layer belnd mode.
 */
void
s3_set_layer_blend(
	int layer,
	int blend);

/*
 * Set the layer center coordinate.
 */
void
s3_set_layer_center(
	int layer,
	int x,
	int y);

/*
 * Set the layer rotation.
 */
void
s3_set_layer_rotate(
	int layer,
	float rad);

/*
 * Get the layer file name.
 */
const char *
s3_get_layer_file_name(
	int layer);

/*
 * Set the layer file name.
 */
bool
s3_set_layer_file_name(
	int layer,
	const char *file_name);

/*
 * Get the layer image for a glyph drawing.
 */
struct s3_image *
s3_get_layer_image(
	int layer);

/*
 * Set the layer image for a load.
 */
void
s3_set_layer_image(
	int layer,
	struct s3_image *img);

/*
 * Set the layer frame for eye blinking and lip synchronization.
 */
void s3_set_layer_frame(int layer, int frame);

/*
 * Get the text layer string.
 */
const char *
s3_get_layer_text(
	int text_layer_index);

/*
 * Set the text layer string.
 */
bool
s3_set_layer_text(
	int textlayer_index,
	const char *msg);

/*
 * Get the sysbtn idle image.
 */
struct s3_image *
s3_get_sysbtn_idle_image(void);

/*
 * Get the sysbtn hover image.
 */
struct s3_image *
s3_get_sysbtn_hover_image(void);

/*
 * Clear the basic layers.
 */
void
s3_clear_stage_basic(void);

/*
 * Clear the stage and make it initial state.
 */
void
s3_clear_stage(void);

/*
 * Convert a character position to a stage layer index.
 */
int
s3_chpos_to_layer(
	int chpos);

/*
 * Convert a character position to a stage layer index (character eye).
 */
int
s3_chpos_to_eye_layer(
	int chpos);

/*
 * Convert a character position to a stage layer index (character lip).
 */
int
s3_chpos_to_lip_layer(
	int chpos);

/*
 * Convert a stage layer index to a character position.
 */
int
s3_layer_to_chpos(
	int chpos);

/*
 * Render the stage with all stage layers.
 */
void
s3_render_stage(void);

/*
 * Draw entire stage into the save data thumbnail.
 */
void
s3_draw_stage_to_thumb(void);

/*
 * Get the image of the save data thumbnail.
 */
struct s3_image *
s3_get_thumb_image(void);

/*
 * Get a fading method from a string.
 */
int
s3_get_fade_method(
	const char *method);

/*
 * Start a fading.
 */
bool
s3_start_fade(
	struct s3_fade_desc *desc,
	int method,
	float t,
	struct s3_image *rule_img);

/*
 * Set the offset for the shake command.
 */
void
s3_set_shake_offset(
	int x,
	int y);

/*
 * Check if the fading is running.
 */
bool
s3_is_fade_running(void);

/*
 * End the fading.
 */
void
s3_finish_fade(void);

/*
 * Specify a character index for a character position.
 */
void
s3_set_ch_name_mapping(
	int chpos,
	int ch_name_index);

/*
 * Get the talker character name index. (-1 for no speaker)
 */
void
s3_get_ch_talking(void);

/*
 * Set the talker character name index. (-1 for no speaker)
 */
void
s3_set_ch_talking(
	int ch_name_index);

/*
 * Get the talker character position.
 */
int
s3_get_talking_chpos(void);

/*
 * Update the character dimming automatically.
 */
void
s3_update_ch_dim_by_talking_ch(void);

/*
 * Update the character dimming manually.
 */
void
s3_force_ch_dim(
	int chpos,
	bool is_dim);

/*
 * Get the dimming state.
 */
bool
s3_get_ch_dim(
	int chpos);

/*
 * Fill the name box by the name box image.
 */
void
s3_fill_namebox(void);

/*
 * Get the name box position and size.
 */
void
s3_get_namebox_rect(
	int *namebox_x,
	int *namebox_y,
	int *namebox_w,
	int *namebox_h);

/*
 * Show or hides the name box.
 */
void
s3_show_namebox(
	bool show);

/*
 * Fill the message box by the message box image.
 */
void
s3_fill_msgbox(void);

/*
 * Show or hide the message box.
 */
void
s3_show_msgbox(
	bool show);

/*
 * Get the message box position and size.
 */
void
s3_get_msgbox_rect(
	int *msgbox_x,
	int *msgbox_y,
	int *msgbox_w,
	int *msgbox_h);

/*
 * Set the position of the click animation.
 */
void
s3_set_click_position(
	int x,
	int y);

/*
 * Show or hide the click animation.
 */
void
s3_show_click(
	bool show);

/*
 * Set the click animation frame index.
 */
void
s3_set_click_index(
	int index);

/*
 * Get the click animation frame position and size.
 */
void
s3_get_click_rect(
	int *click_x,
	int *click_y,
	int *click_w,
	int *click_h);

/*
 * Fill the choose box by the choose box bg image.
 */
void
s3_fill_choosebox_idle_image(
	int index);

/*
 * Fill the choose box by the choose box fg image.
 */
void
s3_fill_choosebox_hover_image(
	int index);

/*
 * Show or hide the choose box.
 */
void
s3_show_choosebox(
	int index,
	bool show_idle,
	bool show_hover);

/*
 * Get a rect for a choose box.
 */
void
s3_get_choosebox_rect(
	int index,
	int *x,
	int *y,
	int *w,
	int *h);

/*
 * Show or hides the auto mode banner.
 */
void
s3_show_automode_banner(
	bool show);

/*
 * Show or hides the skip mode banner.
 */
void
s3_show_skipmode_banner(
	bool show);

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
	int alpha);

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
	int alpha);


/*
 * Functions for System Button Subsystem (sysbtn.c)
 */

/*
 * Show or hide the system button.
 */
bool
s3_show_sysbtn(
	bool is_visible);

/*
 * Check if the system button is visible.
 */
bool
s3_is_sysbtn_visible(void);

/*
 * Update the system button's pointed state by the mouse position.
 */
void
s3_update_sysbtn_state(void);

/*
 * Check if the system bytton is pointed.
 */
bool
s3_is_sysbtn_pointed(void);

/*
 * Check if the system bytton is clicked.
 */
bool
s3_is_sysbtn_clicked(void);


/*
 * Functions for Text Subsystem (text.c)
 */

/*
 * Convert the top character of a utf-8 string to utf-32.
 */
int
s3_utf8_to_utf32(
	const char *mbs,
	uint32_t *wc);

/*
 * Count the characters of a utf-8 string.
 */
int
s3_count_utf8_chars(
	const char *mbs);

/*
 * Get the glyph width.
 */
int
s3_get_glyph_width(
	int font_type,
	int font_size,
	uint32_t codepoint);

/*
 * Get the glyph height.
 */
int
s3_get_glyph_height(
	int font_type,
	int font_size,
	uint32_t codepoint);

/*
 * Get the string width.
 */
int
s3_get_string_width(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Get the string height.
 */
int
s3_get_string_height(
	int font_type,
	int font_size,
	const char *mbs);

/*
 * Draw a glyph.
 */
bool
s3_draw_glyph(
	struct s3_image *img,
	int font_type,
	int font_size,
	int base_font_size,
	int outline_size,
	int x,
	int y,
	s3_pixel_t color,
	s3_pixel_t outline_color,
	uint32_t codepoint,
	int *ret_w,
	int *ret_h,
	bool is_dim);

/*
 * Initialize a message drawing context.
 */
void
s3_construct_draw_msg_context(
	struct s3_draw_msg_context *context,
	struct s3_image *dst,
	const char *msg,
	int font,
	int font_size,
	int base_font_size,
	int ruby_size,
	int outline_size,
	int pen_x,
	int pen_y,
	int area_width,
	int area_height,
	int left_margin,
	int right_margin,
	int top_margin,
	int bottom_margin,
	int line_margin,
	int char_margin,
	s3_pixel_t color,
	s3_pixel_t outline_color,
	s3_pixel_t bg_color,
	bool fill_bg,
	bool is_dimming,
	bool ignore_linefeed,
	bool ignore_font,
	bool ignore_outline,
	bool ignore_color,
	bool ignore_size,
	bool ignore_position,
	bool ignore_ruby,
	bool ignore_wait,
	void (*inline_wait_hook)(float),
	bool use_tategaki);

/*
 * Count the remaining characters excluding escape sequences.
 */
int
s3_count_chars_common(
	struct s3_draw_msg_context *context,
	int *width);

/*
 * Draw characters in a message up to (max_chars) characters.
 */
int
s3_draw_msg_common(
	struct s3_draw_msg_context *context,
	int max_chars);

/*
 * Get a pen position.
 */
void
s3_get_pen_position_common(
	struct s3_draw_msg_context *context,
	int *pen_x,
	int *pen_y);

/*
 * Ignore inline wait.
 */
void
s3_set_ignore_inline_wait(
	struct s3_draw_msg_context *context);

/*
 * Check if a message is a quoted serif. (TODO: delete)
 */
bool
s3_is_quoted_serif(
	const char *msg);


/*
 * Check if a character is escape sequence.
 */
bool
s3_is_escape_sequence_char(
	char c);


/*
 * Functions for Mixer Subsystem (mixer.c)
 */

/*
 * Play a sound file on a mixer track.
 */
bool
s3_set_mixer_input_file(
	int track,
	const char *file,
	bool is_looped);

/*
 * Set the volume for a mixer track.
 */
void
s3_set_mixer_volume(
	int track,
	float vol,
	float span);

/*
 * Get the volume for a mixer track.
 */
float
s3_get_mixer_volume(
	int track);

/*
 * Set the master volume.
 */
void
s3_set_master_volume(
	float vol);

/*
 * Get the master volume.
 */
float
s3_get_master_volume(void);

/*
 * Set the global volume for a track.
 */
void
s3_set_mixer_global_volume(
	int track,
	float vol);

/*
 * Get the global volume for a track.
 */
float
s3_get_mixer_global_volume(
	int track);

/*
 * Set the character volume.
 */
void
s3_set_character_volume(
	int ch_index,
	float vol);

/*
 * Get the character volume.
 */
float
s3_get_character_volume(
	int ch_index);

/*
 * Check if the track playback is finished.
 */
bool
s3_is_mixer_sound_finished(
	int track);

/*
 * Get the track file name.
 */
const char *
s3_get_track_file_name(
	int track);

/*
 * Apply a character volume to the VOICE track.
 */
void
s3_apply_character_volume(
	int ch);


/*
 * Functions for Tag Subsystem (tag.c)
 */

/*
 * Get the tag count.
 */
int
s3_get_tag_count(void);

/*
 * Load a tag file and move to it.
 */
bool
s3_move_to_tag_file(
	const char *file);

/*
 * Move to a tag by index.
 */
bool
s3_move_to_tag_index(int index);

/*
 * Move to the next tag.
 */
bool
s3_move_to_next_tag(void);

/*
 * Move to a label tag.
 */
bool
s3_move_to_label_tag(
	const char *label);

/*
 * Move to the else/elseif/endif tag.
 */
bool
s3_move_to_else_tag(void);

/*
 * Move to the endif tag.
 */
bool
s3_move_to_endif_tag(void);

/*
 * Get the current tag file name.
 */
const char *
s3_get_tag_file(void);

/*
 * Get the command index of the current tag.
 */
int
s3_get_tag_index(void);

/*
 * Get the line number of the current tag.
 */
int
s3_get_tag_line(void);

/*
 * Get the name of the current tag.
 */
const char*
s3_get_tag_name(void);

/*
 * Get the property count of the current tag.
 */
int
s3_get_tag_property_count(void);

/*
 * Get the property name of the current tag.
 */
const char *
s3_get_tag_property_name(
	int index);

/*
 * Get the property value of the current tag.
 */
const char *
s3_get_tag_property_value(
	int index);

/*
 * Check if a tag argument exists.
 */
bool
s3_check_tag_arg(
	const char *name);

/*
 * Get a boolean tag argument.
 */
bool
s3_get_tag_arg_bool(
	const char *name,
	bool omissible,
	bool def_val);

/*
 * Get an integer tag argument.
 */
int
s3_get_tag_arg_int(
	const char *name,
	bool omissible,
	int def_val);

/*
 * Get a float tag argument.
 */
float
s3_get_tag_arg_float(
	const char *name,
	bool omissible,
	float def_val);

/*
 * Get a string tag argument.
 */
const char *
s3_get_tag_arg_string(
	const char *name,
	bool omissible,
	const char *def_val);

/*
 * Evaluate property values of the current tag.
 */
bool
s3_evaluate_tag(void);

/*
 * Push an "if" to the tag stack.
 */
bool
s3_push_tag_stack_if(void);

/*
 * Pop an "if" from the tag stack.
 */
bool
s3_pop_tag_stack_if(void);

/*
 * Push a "while loop" to the tag stack.
 */
bool
s3_push_tag_stack_while(void);

/*
 * Pop a "while loop" from the tag stack.
 */
bool
s3_pop_tag_stack_while(void);

/*
 * Push a "for loop" to the tag stack.
 */
bool
s3_push_tag_stack_for(void);

/*
 * Pop a "for loop" from the tag stack.
 */
bool
s3_pop_tag_stack_for(void);


/*
 * Functions for Anime Subsystem (anime.c)
 */

/*
 * Load an anime file.
 */
bool
s3_load_anime_from_file(
	const char *fname,
	int reg_index,
	bool *used_layer);

/*
 * Clear the layer anime sequences.
 */
void
s3_clear_layer_anime_sequence(
	int layer);

/*
 * Clear all anime sequences.
 */
void
s3_clear_all_anime_sequence(void);

/*
 * Begin describing an anime for a layer.
 */
bool
s3_new_anime_sequence(
	int layer);

/*
 * Add a float property into an anime sequence.
 */
bool
s3_add_anime_sequence_property_f(
	const char *key,
	float val);

/*
 * Add an integer property into an anime sequence.
 */
bool
s3_add_anime_sequence_property_i(
	const char *key,
	int val);

/*
 * Start the anime for a layer.
 */
bool
s3_start_layer_anime(
	int layer);

/*
 * Check if there are running animes.
 */
bool
s3_is_anime_running(void);

/*
 * Check if there are running animes. (with mask)
 */
bool
s3_is_anime_running_with_layer_mask(
	bool *used_layers);

/*
 * Check if the layer anime is finished.
 */
bool
s3_is_anime_finished_for_layer(
	int layer);

/*
 * Update the anime frame.
 */
void
s3_update_anime_frame(void);

/*
 * Unregister a looped anime.
 */
void
s3_unregister_anime(
	int reg_index);

/*
 * Get a looped anime file name.
 */
const char *
s3_get_reg_anime_file_name(
	int reg_index);

/*
 * Load an eye blinking image.
 */
bool
s3_load_eye_image_if_exists(
	int chpos,
	const char *fname);

/*
 * Reload an eye blinking image.
 */
bool
s3_reload_eye_anime(
	int chpos);

/*
 * Load a lip synchronization image.
 */
bool
s3_load_lip_image_if_exists(
	int chpos,
	const char *fname);

/*
 * Run a lip synchronization anime.
 */
void
s3_run_lip_anime(
	int chpos,
	const char *msg);

/*
 * Stop a lip synchronization anime.
 */
void
s3_stop_lip_anime(
	int chpos);


/*
 * Functions for Variable Subsystem (vars.c)
 */

/*
 * Set an integer value to a variable.
 */
bool
s3_set_variable_int(
	const char *name,
	int val);

/*
 * Set a float value to a variable.
 */
bool
s3_set_variable_float(
	const char *name,
	float val);

/*
 * Set a string value to a variable.
 */
bool
s3_set_variable_string(
	const char *name,
	const char* val);

/*
 * Unset a variable.
 */
bool
s3_unset_variable(
	const char *name);

/*
 * Set a variable global.
 */
bool
s3_make_variable_global(
	const char *name,
	bool is_global);

/*
 * Get an integer value from a variable.
 */
int
s3_get_variable_int(
	const char *name);

/*
 * Get a float value from a variable.
 */
float
s3_get_variable_float(
	const char *name);

/*
 * Get a string value from a variable.
 */
const char *
s3_get_variable_string(
	const char *name);

/*
 * Get the numbers of the variables.
 */
int
s3_get_variable_count(void);

/*
 * Get the name of the variable at an index.
 */
const char *
s3_get_variable_name(
	int index);

/*
 * Check if a variable exists for a name.
 */
bool
s3_check_variable_exists(
	const char *name);

/*
 * Check if a variable is global.
 */
bool
s3_is_global_variable(
	const char *name);

/*
 * Expand a string that may contain variable references.
 */
char *
s3_expand_string_with_variable(
	const char *msg);

/*
 * Clear all local variables.
 */
void
s3_unset_local_variables(void);


/*
 * Functions for Save Subsystem (save.c)
 */

/*
 * Execute a global save.
 */
bool
s3_execute_save_global(void);

/*
 * Execute a global load.
 */
bool
s3_execute_load_global(void);

/*
 * Execute a local save.
 */
bool
s3_execute_save_local(
	int index);

/*
 * Execute a local load.
 */
bool
s3_execute_load_local(
	int index);

/*
 * Check if the quick save data exist.
 */
bool
s3_check_save_exists(
	int index);

/*
 * Delete a local save data.
 */
void
s3_delete_local_save(
	int index);

/*
 * Delete the global save data.
 */
void
s3_delete_global_save(void);

/*
 * Check if right after load.
 */
bool
s3_check_right_after_load(void);

/*
 * Get the timestamp of a save data.
 */
uint64_t
s3_get_save_timestamp(
	int index);

/*
 * Get the latest save index.
 */
int
s3_get_latest_save_index(void);

/*
 * Get the chapter title of a save data.
 */
const char *
s3_get_save_chapter_name(
	int index);

/*
 * Get the last message of a save data.
 */
const char *
s3_get_save_last_message(
	int index);

/*
 * Get the thumbnail of a save data.
 */
struct s3_image *
s3_get_save_thumbnail(
	int index);


/*
 * Functions for History Subsystem (history.c)
 */

/*
 * Clear the history.
 */
void
s3_clear_history(void);

/*
 * Add a history.
 */
bool
s3_add_history(
	const char *name,
	const char *msg,
	const char *voice,
	s3_pixel_t body_color,
	s3_pixel_t body_outline_color,
	s3_pixel_t name_color,
	s3_pixel_t name_outline_color);

/*
 * Get the number of the history.
 */
int
s3_get_history_count(void);

/*
 * Get the name at the history index.
 */
const char *
s3_get_history_name(int index);

/*
 * Get the message at the history index.
 */
const char *
s3_get_history_message(int index);

/*
 * Get the message at the history index.
 */
const char *
s3_get_history_voice(int index);


/*
 * Functions for Seen Subsystem (seen.c)
 */

/*
 * Load the seen file for the current tag file.
 */
bool
s3_load_seen(void);

/*
 * Save the seen file for the current tag file.
 */
bool
s3_save_seen(void);

/*
 * Get the seen flags for the current tag.
 */
int
s3_get_seen_flags(void);

/*
 * Set the seen flags for the current tag.
 */
void
s3_set_seen_flags(int flag);


/*
 * Functions for GUI Subsystem. (gui.c)
 */

/*
 * Check if right after returned from a system GUI.
 */
bool
s3_check_right_after_sys_gui(void);

/*
 * Load a GUI file and prepare for a start.
 */
bool
s3_load_gui_file(
	const char *file,
	bool sys);

/*
 * Start the loaded GUI.
 */
void
s3_start_gui(void);

/*
 * Stop the running GUI.
 */
void
s3_stop_gui(void);

/*
 * Check if a GUI is running.
 */
bool
s3_is_gui_running(void);

/*
 * Chech if a GUI is finished.
 */
bool
s3_is_gui_finished(void);

/*
 * Get the label of the selected button.
 */
const char *
s3_get_gui_result_label(void);

/*
 * Check if the selected GUI button is "back to title".
 */
bool
s3_is_gui_result_title(void);

/*
 * Check if any save is issued in the current GUI.
 */
bool
s3_check_if_saved_in_gui(void);

/*
 * Check if a load is issued in the current GUI.
 */
bool
s3_check_if_loaded_in_gui(void);


/*
 * Functions of Wrapper APIs (hal.c)
 */

/*
 * Reset a lap timer and initializes it with a current time.
 */
void
s3_reset_lap_timer(
	uint64_t *origin);

/*
 * Get a lap time in milliseconds.
 */
uint64_t
s3_get_lap_timer_millisec(
	uint64_t *origin);

/*
 * Check if a file exists.
 */
bool
s3_check_file_exists(
	const char *fname);

/*
 * Read a file.
 */
bool
s3_read_file_content(
	const char *fname,
	char **buf,
	size_t *len);

/*
 * Write save data.
 */
bool
s3_write_save_data(
	const char *key,
	const void *data,
	size_t size);

/*
 * Read save data.
 */
bool
s3_read_save_data(
	const char *key,
	void *data,
	size_t size,
	size_t *ret);

/*
 * Get the size of save data.
 */
size_t
s3_get_save_data_size(
	const char *key);

/*
 * Check whether save data exist or not.
 */
bool
s3_check_save_data(
	const char *key);

/*
 * Install an API function.
 */
bool
s3_install_api(
	const char *name,
	bool (*func)(void *),
	bool no_args);

/*
 * Install a tag function.
 */
bool
s3_install_tag(
	const char *name,
	bool (*func)(void *));

/*
 * Get a VM integer. (Suika.*)
 */
bool
s3_get_vm_int(
	const char *name,
	int *val);

/*
 * Set a VM integer. (Suika.*)
 */
bool
s3_set_vm_int(
	const char *name,
	int val);

/*
 * Set a VM float variable. (Suika.*)
 */
bool
s3_set_vm_float(
	const char *name,
	float val);

/*
 * Set a VM string variable. (Suika.*)
 */
bool
s3_set_vm_string(
	const char *name,
	const char *val);

/*
 * Call a VM function.
 */
bool
s3_call_vm_function(
	const char *name);

/*
 * Call a VM function that corresponds to the current tag.
 */
bool
s3_call_vm_tag_function(
	bool *tag_end);

/*
 * Enable/disable message skip by touch move.
 */
void
s3_set_continuous_swipe_enabled(
	bool is_enabled);

/*
 * Return whether we support the full screen mode.
 */
bool
s3_is_full_screen_supported(void);

/*
 * Return whether the we are in the full screen mode.
 */
bool
s3_is_full_screen_mode(void);

/*
 * Enter the full screen mode.
 */
void
s3_enter_full_screen_mode(void);

/*
 * Leave the full screen mode.
 */
void
s3_leave_full_screen_mode(void);

/*
 * Get the system language.
 */
const char *
s3_get_system_language(void);

/*
 * Print a debug message.
 */
void
s3_log_info(
	const char *msg,
	...);

/*
 * Print a warning message.
 */
void
s3_log_warn(
	const char *msg,
	...);

/*
 * Print an error message.
 */
void
s3_log_error(
	const char *msg,
	...);

/*
 * Print an out-of-memory error message.
 */
void
s3_log_out_of_memory(void);

/*
 * Print a tag execution error.
 */
void
s3_log_tag_error(
	const char *msg,
	...);

/*
 * Text-to-speech.
 */
void
s3_speak_text(
	const char *msg);

#endif
