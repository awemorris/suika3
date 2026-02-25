/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Config Subsystem
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
#include "mixer.h"
#include "stage.h"

#include <playfield/playfield.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>	/* NAN */
#include <assert.h>

/* False assertion */
#define CONFIG_KEY_NOT_FOUND	(0)
#define INVALID_CONFIG_TYPE	(0)

/*
 * Game Settings
 */

/* Game title */
char *conf_game_title_en;
char *conf_game_title_ja;

/* Novel mode */
bool conf_game_novel;

/* Locale */
char *conf_game_locale;

/*
 * Font File Settings
 */

/* TTF file name */
char *conf_font_ttf[4];

/*
 * Message Box
 */

/* Image file */
char *conf_msgbox_image;

/* Position */
int conf_msgbox_x;
int conf_msgbox_y;

/* Margins */
int conf_msgbox_margin_left;
int conf_msgbox_margin_top;
int conf_msgbox_margin_right;
int conf_msgbox_margin_bottom;
int conf_msgbox_margin_line;
int conf_msgbox_margin_char;

/* Font */
int conf_msgbox_font_select;
int conf_msgbox_font_size;
int conf_msgbox_font_r;
int conf_msgbox_font_g;
int conf_msgbox_font_b;
int conf_msgbox_font_outline_width;
int conf_msgbox_font_outline_r;
int conf_msgbox_font_outline_g;
int conf_msgbox_font_outline_b;
int conf_msgbox_font_ruby;
bool conf_msgbox_font_tategaki;

/* Fill */
bool conf_msgbox_fill_enable;
int conf_msgbox_fill_r;
int conf_msgbox_fill_g;
int conf_msgbox_fill_b;

/* Dimming */
bool conf_msgbox_dim_enable;
int conf_msgbox_dim_r;
int conf_msgbox_dim_g;
int conf_msgbox_dim_b;
int conf_msgbox_dim_outline_width;
int conf_msgbox_dim_outline_r;
int conf_msgbox_dim_outline_g;
int conf_msgbox_dim_outline_b;

/* Seen coloring */
bool conf_msgbox_seen_enable;
int conf_msgbox_seen_r;
int conf_msgbox_seen_g;
int conf_msgbox_seen_b;
int conf_msgbox_seen_outline_width;
int conf_msgbox_seen_outline_r;
int conf_msgbox_seen_outline_g;
int conf_msgbox_seen_outline_b;

/* Misc. */
bool conf_msgbox_show_on_ch;
bool conf_msgbox_show_on_bg;
bool conf_msgbox_show_on_choose;
bool conf_msgbox_skip_unseen;

/*
 * Name Box Settings
 */

/* Enable. */
bool conf_namebox_enable;

/* Image file. */
char *conf_namebox_image;

/* Position. */
int conf_namebox_x;
int conf_namebox_y;

/* Margins. */
int conf_namebox_margin_top;
int conf_namebox_margin_left;
bool conf_namebox_centering;

/* Font */
int conf_namebox_font_select;
int conf_namebox_font_size;
int conf_namebox_font_r;
int conf_namebox_font_g;
int conf_namebox_font_b;
int conf_namebox_font_outline_width;
int conf_namebox_font_outline_r;
int conf_namebox_font_outline_g;
int conf_namebox_font_outline_b;
int conf_namebox_font_ruby;
bool conf_namebox_font_tategaki;

/*
 * Click Animation
 */

/* Position. */
int conf_click_x;
int conf_click_y;

/* Interval. */
float conf_click_interval;

/* Image files. */
char *conf_click_image[16];

/* Misc. */
bool conf_click_move;

/*
 * Choose Box Settings
 */

/* Font */
int conf_choose_font_select;
int conf_choose_font_size;
int conf_choose_font_idle_r;
int conf_choose_font_idle_g;
int conf_choose_font_idle_b;
int conf_choose_font_idle_outline_width;
int conf_choose_font_idle_outline_r;
int conf_choose_font_idle_outline_g;
int conf_choose_font_idle_outline_b;
int conf_choose_font_hover_r;
int conf_choose_font_hover_g;
int conf_choose_font_hover_b;
int conf_choose_font_hover_outline_width;
int conf_choose_font_hover_outline_r;
int conf_choose_font_hover_outline_g;
int conf_choose_font_hover_outline_b;
int conf_choose_font_ruby;
bool conf_choose_font_tategaki;

/* SE */
char *conf_choose_change_se;
char *conf_choose_click_se;

/* File names */
char *conf_choose_idle[S3_CHOOSEBOX_COUNT];
char *conf_choose_hover[S3_CHOOSEBOX_COUNT];

/* Positions */
int conf_choose_x[S3_CHOOSEBOX_COUNT];
int conf_choose_y[S3_CHOOSEBOX_COUNT];

/* Margin */
int conf_choose_margin_top[S3_CHOOSEBOX_COUNT];

/* Anime */
char *conf_choose_focus_anime[S3_CHOOSEBOX_COUNT];
char *conf_choose_unfocus_anime[S3_CHOOSEBOX_COUNT];

/*
 * Save Data Settings
 */

/* Thumbnail size */
int conf_save_thumb_width;
int conf_save_thumb_height;

/* NEW image file name */
char *conf_save_new_image;

/* 
 * System Button Settings
 */

/* Enable */
bool conf_sysbtn_enable;

/* File names */
char *conf_sysbtn_idle;
char *conf_sysbtn_hover;

/* Position */
int conf_sysbtn_x;
int conf_sysbtn_y;

/* SE */
char *conf_sysbtn_enter_se;
char *conf_sysbtn_leave_se;
char *conf_sysbtn_click_se;

/* Misc. */
bool conf_sysbtn_show_on_transition;

/*
 * Auto Mode Settings
 */

/* File name */
char *conf_automode_image;

/* Position */
int conf_automode_x;
int conf_automode_y;

/* SE */
char *conf_automode_enter_se;
char *conf_automode_leave_se;

/*
 * Skip Mode Settings
 */

/* File name */
char *conf_skipmode_image;

/* Position */
int conf_skipmode_x;
int conf_skipmode_y;

/* SE */
char *conf_skipmode_enter_se;
char *conf_skipmode_leave_se;

/*
 * GUI Settings
 */

/* Save font */
int conf_gui_save_font_select;
int conf_gui_save_font_size;
int conf_gui_save_font_r;
int conf_gui_save_font_g;
int conf_gui_save_font_b;
int conf_gui_save_font_outline_width;
int conf_gui_save_font_outline_r;
int conf_gui_save_font_outline_g;
int conf_gui_save_font_outline_b;
int conf_gui_save_font_ruby;
bool conf_gui_save_font_tategaki;

/* History margin */
int conf_gui_history_margin_line;

/* History Font */
int conf_gui_history_font_select;
int conf_gui_history_font_size;
int conf_gui_history_font_r;
int conf_gui_history_font_g;
int conf_gui_history_font_b;
int conf_gui_history_font_outline_width;
int conf_gui_history_font_outline_r;
int conf_gui_history_font_outline_g;
int conf_gui_history_font_outline_b;
int conf_gui_history_font_ruby;
bool conf_gui_history_font_tategaki;

/* Misc. */
char *conf_gui_history_quote_name_separator;
char *conf_gui_history_quote_start;
char *conf_gui_history_quote_end;
bool conf_gui_history_hide_last;

/* Preview */
int conf_gui_preview_font_select;
int conf_gui_preview_font_size;
int conf_gui_preview_font_r;
int conf_gui_preview_font_g;
int conf_gui_preview_font_b;
int conf_gui_preview_font_outline_width;
int conf_gui_preview_font_outline_r;
int conf_gui_preview_font_outline_g;
int conf_gui_preview_font_outline_b;
int conf_gui_preview_font_ruby;
bool conf_gui_preview_font_tategaki;

/*
 * Sound Settings
 */

/* Default */
float conf_sound_vol_bgm;
float conf_sound_vol_voice;
float conf_sound_vol_se;
float conf_sound_vol_per_character;

/*
 * Character Settings
 */

/* Auto Focus */
bool conf_character_auto_focus;

/* Character name and folder mapping */
char *conf_character_name[S3_CHARACTER_MAP_COUNT];
char *conf_character_folder[S3_CHARACTER_MAP_COUNT];

/* Eye-blink */
float conf_character_eyeblink_interval;
float conf_character_eyeblink_frame;

/* Lip-sync */
float conf_character_lipsync_frame;
int conf_character_lipsync_chars;

/*
 * Stage Settings
 */

/* Margins */
int conf_stage_ch_margin_bottom;
int conf_stage_ch_margin_left;
int conf_stage_ch_margin_right;

/*
 * Kirakira Effect
 */

bool conf_kirakira_enable;
bool conf_kirakira_add_blend;
float conf_kirakira_frame;
char *conf_kirakira_image[S3_KIRAKIRA_FRAMES];

/*
 * Emoji
 */

char *conf_emoji_name[S3_EMOJI_COUNT];
char *conf_emoji_image[S3_EMOJI_COUNT];

/*
 * Text-to-speech
 */

bool conf_tts_enable;

/*
 * Misc.
 */

/* Is release app? */
bool conf_release_mode_enable;

/*
 * Private Usage
 */

/* Screen width */
int conf_game_width;

/* Screen height */
int conf_game_height;

/* Click animation frames */
int conf_click_frames;

/* Last saved/loaded page */
int conf_gui_save_last_page;

/*
 * Line Size
 */
#define BUF_SIZE	(1024)

/*
 * Config Table
 */

#define MUST		true
#define OPTIONAL	false
#define SAVE		true
#define NOSAVE		false
#define GLOBAL		true
#define LOCAL		false

static struct rule {
	/* Type: 's' for string, 'b' for boolean, 'i' for integer, 'f' for float */
	char type;

	/* Key */
	const char *key;

	/* Value */
	void *var_ptr;

	/* Is always-required? */
	bool must;

	/* Is stored to save file? */
	bool save;

	/* Is a global config? */
	bool global;
} rule_tbl[] = {
	/* Game Info */
	{'s',	"game.title.en",		&conf_game_title_en,			MUST,	NOSAVE,	GLOBAL},
	{'s',	"game.title.ja",		&conf_game_title_ja,			MUST,	NOSAVE,	GLOBAL},
	{'b',	"game.novel",			&conf_game_novel,			MUST,	SAVE,	GLOBAL},
	{'s',	"game.locale",			&conf_game_locale,			MUST,	SAVE,	GLOBAL},

	/* Font */
	{'s',	"font.ttf1",			&conf_font_ttf[0],			MUST,	SAVE,	GLOBAL},
	{'s',	"font.ttf2",			&conf_font_ttf[1],			OPTIONAL, SAVE,	GLOBAL},
	{'s',	"font.ttf3",			&conf_font_ttf[2],			OPTIONAL, SAVE,	GLOBAL},
	{'s',	"font.ttf4",			&conf_font_ttf[3],			OPTIONAL, SAVE,	GLOBAL},

	/* Message Box */
	{'s',	"msgbox.image",			&conf_msgbox_image,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.x",			&conf_msgbox_x,				MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.y",			&conf_msgbox_y,				MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.left",		&conf_msgbox_margin_left,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.top",		&conf_msgbox_margin_top,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.right",		&conf_msgbox_margin_right,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.bottom",		&conf_msgbox_margin_bottom,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.line",		&conf_msgbox_margin_line,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.margin.char",		&conf_msgbox_margin_char,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.select",		&conf_msgbox_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.size",		&conf_msgbox_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.r",		&conf_msgbox_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.g",		&conf_msgbox_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.b",		&conf_msgbox_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.outline.width",	&conf_msgbox_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.outline.r",	&conf_msgbox_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.outline.g",	&conf_msgbox_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.outline.b",	&conf_msgbox_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.font.ruby",		&conf_msgbox_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.font.tategaki",		&conf_msgbox_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.fill.enable",		&conf_msgbox_fill_enable,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.fill.r",		&conf_msgbox_fill_r,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.fill.g",		&conf_msgbox_fill_g,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.fill.b",		&conf_msgbox_fill_b,			MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.dim.enable",		&conf_msgbox_dim_enable,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.r",			&conf_msgbox_dim_r,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.g",			&conf_msgbox_dim_g,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.b",			&conf_msgbox_dim_b,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.outline.width",	&conf_msgbox_dim_outline_width,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.outline.r",		&conf_msgbox_dim_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.outline.g",		&conf_msgbox_dim_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.dim.outline.b",		&conf_msgbox_dim_outline_b,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.seen.enable",		&conf_msgbox_seen_enable,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.r",		&conf_msgbox_seen_r,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.g",		&conf_msgbox_seen_g,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.b",		&conf_msgbox_seen_b,			MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.outline.width",	&conf_msgbox_seen_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.outline.r",	&conf_msgbox_seen_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.outline.g",	&conf_msgbox_seen_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"msgbox.seen.outline.b",	&conf_msgbox_seen_outline_b,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.show_on_ch",		&conf_msgbox_show_on_ch,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.show_on_bg",		&conf_msgbox_show_on_bg,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.show_on_choose",	&conf_msgbox_show_on_choose,		MUST,	SAVE,	LOCAL},
	{'b',	"msgbox.skip_unseen",		&conf_msgbox_skip_unseen,		MUST,	SAVE,	LOCAL},

	/* Name Box */
	{'b',	"namebox.enable",		&conf_namebox_enable,			MUST,	SAVE,	LOCAL},
	{'s',	"namebox.image",		&conf_namebox_image,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.x",			&conf_namebox_x,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.y", 			&conf_namebox_y,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.margin.top",		&conf_namebox_margin_top,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.margin.left",		&conf_namebox_margin_left,		MUST,	SAVE,	LOCAL},
	{'b',	"namebox.centering",		&conf_namebox_centering,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.select",		&conf_namebox_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.size",		&conf_namebox_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.r",		&conf_namebox_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.g",		&conf_namebox_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.b",		&conf_namebox_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.outline.width",	&conf_namebox_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.outline.r",	&conf_namebox_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.outline.g",	&conf_namebox_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.outline.b",	&conf_namebox_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"namebox.font.ruby",		&conf_namebox_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"namebox.font.tategaki",	&conf_namebox_font_tategaki,		MUST,	SAVE,	LOCAL},

	/* Click Animation */
	{'i',	"click.x",			&conf_click_x,				MUST,	SAVE,	LOCAL},
	{'i',	"click.y",			&conf_click_y,				MUST,	SAVE,	LOCAL},
	{'f',	"click.interval",		&conf_click_interval,			MUST,	SAVE,	LOCAL},
	{'s',	"click.image1",			&conf_click_image[0],			MUST,	SAVE,	LOCAL},
	{'s',	"click.image2",			&conf_click_image[1],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image3",			&conf_click_image[2],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image4",			&conf_click_image[3],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image5",			&conf_click_image[4],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image6",			&conf_click_image[5],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image7",			&conf_click_image[6],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image8",			&conf_click_image[7],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image9",			&conf_click_image[8],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image10",		&conf_click_image[9],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image11",		&conf_click_image[10],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image12",		&conf_click_image[11],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image13",		&conf_click_image[12],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image14",		&conf_click_image[13],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image15",		&conf_click_image[14],			OPTIONAL, SAVE,	LOCAL},
	{'s',	"click.image16",		&conf_click_image[15],			OPTIONAL, SAVE,	LOCAL},
	{'b',	"click.move",			&conf_click_move,			MUST,	SAVE,	LOCAL},

	/* Choose Box */
	{'i',	"choose.font.select",		&conf_choose_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.size",		&conf_choose_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.r",		&conf_choose_font_idle_r,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.g",		&conf_choose_font_idle_g,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.b",		&conf_choose_font_idle_b,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.outline.width", &conf_choose_font_idle_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.outline.r",	&conf_choose_font_idle_outline_r,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.outline.g",	&conf_choose_font_idle_outline_g,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.idle.outline.b",	&conf_choose_font_idle_outline_b,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.r",		&conf_choose_font_hover_r,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.g",		&conf_choose_font_hover_g,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.b",		&conf_choose_font_hover_b,		MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.outline.width", &conf_choose_font_hover_outline_width, MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.outline.r",	&conf_choose_font_hover_outline_r,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.outline.g",	&conf_choose_font_hover_outline_g,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.hover.outline.b",	&conf_choose_font_hover_outline_b,	MUST,	SAVE,	LOCAL},
	{'i',	"choose.font.ruby",		&conf_choose_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"choose.font.tategaki",		&conf_choose_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'s',	"choose.change_se",		&conf_choose_change_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.click_se",		&conf_choose_click_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box1.idle",		&conf_choose_idle[0],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box1.hover",		&conf_choose_hover[0],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box1.x",		&conf_choose_x[0],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box1.y",		&conf_choose_y[0],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box1.margin.top",	&conf_choose_margin_top[0],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box1.focus_anime",	&conf_choose_focus_anime[0],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box1.unfocus_anime",	&conf_choose_unfocus_anime[0],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box2.idle",		&conf_choose_idle[1],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box2.hover",		&conf_choose_hover[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.x",		&conf_choose_x[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.y",		&conf_choose_y[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.margin.top",	&conf_choose_margin_top[1],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box2.focus_anime",	&conf_choose_focus_anime[1],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box2.unfocus_anime",	&conf_choose_unfocus_anime[1],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box3.idle",		&conf_choose_idle[2],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box3.hover",		&conf_choose_hover[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.x",		&conf_choose_x[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.y",		&conf_choose_y[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.margin.top",	&conf_choose_margin_top[2],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box3.focus_anime",	&conf_choose_focus_anime[2],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box3.unfocus_anime",	&conf_choose_unfocus_anime[2],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box4.idle",		&conf_choose_idle[3],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box4.hover",		&conf_choose_hover[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.x",		&conf_choose_x[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.y",		&conf_choose_y[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.margin.top",	&conf_choose_margin_top[3],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box4.focus_anime",	&conf_choose_focus_anime[3],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box4.unfocus_anime",	&conf_choose_unfocus_anime[3],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box5.idle",		&conf_choose_idle[4],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box5.hover",		&conf_choose_hover[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.x",		&conf_choose_x[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.y",		&conf_choose_y[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.margin.top",	&conf_choose_margin_top[4],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box5.focus_anime",	&conf_choose_focus_anime[4],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box5.unfocus_anime",	&conf_choose_unfocus_anime[4],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box6.idle",		&conf_choose_idle[5],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box6.hover",		&conf_choose_hover[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.x",		&conf_choose_x[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.y",		&conf_choose_y[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.margin.top",	&conf_choose_margin_top[5],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box6.focus_anime",	&conf_choose_focus_anime[5],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box6.unfocus_anime",	&conf_choose_unfocus_anime[5],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box7.idle",		&conf_choose_idle[6],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box7.hover",		&conf_choose_hover[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.x",		&conf_choose_x[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.y",		&conf_choose_y[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.margin.top",	&conf_choose_margin_top[6],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box7.focus_anime",	&conf_choose_focus_anime[6],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box7.unfocus_anime",	&conf_choose_unfocus_anime[6],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box8.idle",		&conf_choose_idle[7],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box8.hover",		&conf_choose_hover[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.x",		&conf_choose_x[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.y",		&conf_choose_y[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.margin.top",	&conf_choose_margin_top[7],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box8.focus_anime",	&conf_choose_focus_anime[7],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box8.unfocus_anime",	&conf_choose_unfocus_anime[7],		OPTIONAL, SAVE,	LOCAL},

	/* Save Data */
	{'i',	"save.thumb.width",		&conf_save_thumb_width,			MUST,	NOSAVE,	GLOBAL},
	{'i',	"save.thumb.height",		&conf_save_thumb_height,		MUST,	NOSAVE,	GLOBAL},
	{'s',	"save.new_image",		&conf_save_new_image,			OPTIONAL, NOSAVE,	GLOBAL},

	/* System Button */
	{'b',	"sysbtn.enable",		&conf_sysbtn_enable,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.idle",			&conf_sysbtn_idle,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.hover",			&conf_sysbtn_hover,			MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.x",			&conf_sysbtn_x,				MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.y",			&conf_sysbtn_y,				MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.enter_se",		&conf_sysbtn_enter_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"sysbtn.leave_se",		&conf_sysbtn_leave_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"sysbtn.click_se",		&conf_sysbtn_click_se,			OPTIONAL, SAVE,	LOCAL},
	{'b',	"sysbtn.show_on_transition",	&conf_sysbtn_show_on_transition,	MUST,	SAVE,	LOCAL},

	/* Auto Mode */
	{'s',	"automode.image",		&conf_automode_image,			MUST,	SAVE,	LOCAL},
	{'i',	"automode.x",			&conf_automode_x,			MUST,	SAVE,	LOCAL},
	{'i',	"automode.y",			&conf_automode_y,			MUST,	SAVE,	LOCAL},
	{'s',	"automode.enter_se",		&conf_automode_enter_se,		OPTIONAL, SAVE,	LOCAL},
	{'s',	"automode.leave_se",		&conf_automode_leave_se,		OPTIONAL, SAVE,	LOCAL},

	/* Skip Mode */
	{'s',	"skipmode.image",		&conf_skipmode_image,			MUST,	SAVE,	LOCAL},
	{'i',	"skipmode.x",			&conf_skipmode_x,			MUST,	SAVE,	LOCAL},
	{'i',	"skipmode.y",			&conf_skipmode_y,			MUST,	SAVE,	LOCAL},
	{'s',	"skipmode.enter_se",		&conf_skipmode_enter_se,		OPTIONAL, SAVE,	LOCAL},
	{'s',	"skipmode.leave_se",		&conf_skipmode_leave_se,		OPTIONAL, SAVE,	LOCAL},

	/* GUI */
	{'i',	"gui.save.font.select",		&conf_gui_save_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.size",		&conf_gui_save_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.r",		&conf_gui_save_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.g",		&conf_gui_save_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.b",		&conf_gui_save_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.outline.width",	&conf_gui_save_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.outline.r",	&conf_gui_save_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.outline.g",	&conf_gui_save_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.outline.b",	&conf_gui_save_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.font.ruby",		&conf_gui_save_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.font.tategaki",	&conf_gui_save_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.margin.line",	&conf_gui_history_margin_line,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.select",	&conf_gui_history_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.size",	&conf_gui_history_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.r",		&conf_gui_history_font_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.g",		&conf_gui_history_font_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.b",		&conf_gui_history_font_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.outline.width", &conf_gui_history_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.outline.r",	&conf_gui_history_font_outline_r,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.outline.g",	&conf_gui_history_font_outline_g,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.outline.b",	&conf_gui_history_font_outline_b,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.font.ruby",	&conf_gui_history_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.history.font.tategaki",	&conf_gui_history_font_tategaki,	MUST,	SAVE,	LOCAL},
	{'s',	"gui.history.quote.name_separator", &conf_gui_history_quote_name_separator, MUST, SAVE,	LOCAL},
	{'s',	"gui.history.quote.start",	&conf_gui_history_quote_start, 		MUST,	SAVE,	LOCAL},
	{'s',	"gui.history.quote.end",	&conf_gui_history_quote_end, 		MUST,	SAVE,	LOCAL},
	{'b',	"gui.history.hide_last",	&conf_gui_history_hide_last,	 	MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.select",	&conf_gui_preview_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.size",	&conf_gui_preview_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.r",		&conf_gui_preview_font_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.g",		&conf_gui_preview_font_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.b",		&conf_gui_preview_font_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.width", &conf_gui_preview_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.r",	&conf_gui_preview_font_outline_r,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.g",	&conf_gui_preview_font_outline_g,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.b",	&conf_gui_preview_font_outline_b,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.ruby",	&conf_gui_preview_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.preview.font.tategaki",	&conf_gui_preview_font_tategaki,	MUST,	SAVE,	LOCAL},

	/* Initial Volumes (no need to save) */
	{'f',	"sound.vol.bgm",		&conf_sound_vol_bgm,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.voice",		&conf_sound_vol_voice,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.se",			&conf_sound_vol_se,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.per_character",	&conf_sound_vol_per_character,		MUST,	NOSAVE,	GLOBAL},

	/* Character */
	{'b',	"character.auto_focus",		&conf_character_auto_focus,		MUST,	SAVE,	LOCAL},
	{'f',	"character.eyeblink.interval",	&conf_character_eyeblink_interval,	MUST,	NOSAVE,	GLOBAL},
	{'f',	"character.eyeblink.frame",	&conf_character_eyeblink_frame,		MUST,	NOSAVE, GLOBAL},
	{'f',	"character.lipsync.frame",	&conf_character_lipsync_frame,		MUST,	NOSAVE, GLOBAL},
	{'i',	"character.lipsync.chars",	&conf_character_lipsync_chars,		MUST,	NOSAVE, GLOBAL},

	/* Character Name to File Mapping */
	{'s',	"character.name1",		&conf_character_name[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder1",		&conf_character_folder[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2",		&conf_character_name[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder2",		&conf_character_folder[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3",		&conf_character_name[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder3",		&conf_character_folder[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4",		&conf_character_name[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder4",		&conf_character_folder[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5",		&conf_character_name[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder5",		&conf_character_folder[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6",		&conf_character_name[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder6",		&conf_character_folder[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7",		&conf_character_name[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder7",		&conf_character_folder[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8",		&conf_character_name[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder8",		&conf_character_folder[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9",		&conf_character_name[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder9",		&conf_character_folder[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10",		&conf_character_name[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder10",		&conf_character_folder[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11",		&conf_character_name[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder11",		&conf_character_folder[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12",		&conf_character_name[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder12",		&conf_character_folder[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13",		&conf_character_name[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder13",		&conf_character_folder[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14",		&conf_character_name[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder14",		&conf_character_folder[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15",		&conf_character_name[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder15",		&conf_character_folder[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16",		&conf_character_name[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder16",		&conf_character_folder[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17",		&conf_character_name[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder17",		&conf_character_folder[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18",		&conf_character_name[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder18",		&conf_character_folder[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19",		&conf_character_name[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder19",		&conf_character_folder[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20",		&conf_character_name[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder20",		&conf_character_folder[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21",		&conf_character_name[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder21",		&conf_character_folder[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22",		&conf_character_name[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder22",		&conf_character_folder[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23",		&conf_character_name[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder23",		&conf_character_folder[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24",		&conf_character_name[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder24",		&conf_character_folder[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25",		&conf_character_name[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder25",		&conf_character_folder[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26",		&conf_character_name[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder26",		&conf_character_folder[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27",		&conf_character_name[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder27",		&conf_character_folder[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28",		&conf_character_name[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder28",		&conf_character_folder[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29",		&conf_character_name[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder29",		&conf_character_folder[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30",		&conf_character_name[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder30",		&conf_character_folder[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31",		&conf_character_name[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder31",		&conf_character_folder[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32",		&conf_character_name[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder32",		&conf_character_folder[31],		OPTIONAL, NOSAVE,	GLOBAL},

	/* Stage Margins */
	{'i',	"stage.ch_margin.bottom",	&conf_stage_ch_margin_bottom,		MUST,	SAVE,	LOCAL},
	{'i',	"stage.ch_margin.left",		&conf_stage_ch_margin_left,		MUST,	SAVE,	LOCAL},
	{'i',	"stage.ch_margin.right",	&conf_stage_ch_margin_right,		MUST,	SAVE,	LOCAL},

	/* Kira Kira Effect */
	{'b',	"kirakira.enable",		&conf_kirakira_enable,			OPTIONAL, NOSAVE,	GLOBAL},
	{'b',	"kirakira.add_blend",		&conf_kirakira_add_blend,		OPTIONAL, NOSAVE,	GLOBAL},
	{'f',	"kirakira.frame",		&conf_kirakira_frame,			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image1",		&conf_kirakira_image[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image2",		&conf_kirakira_image[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image3",		&conf_kirakira_image[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image4",		&conf_kirakira_image[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image5",		&conf_kirakira_image[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image6",		&conf_kirakira_image[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image7",		&conf_kirakira_image[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image8",		&conf_kirakira_image[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image9",		&conf_kirakira_image[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image10",		&conf_kirakira_image[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image11",		&conf_kirakira_image[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image12",		&conf_kirakira_image[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image13",		&conf_kirakira_image[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image14",		&conf_kirakira_image[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image15",		&conf_kirakira_image[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"kirakira.image16",		&conf_kirakira_image[15],		OPTIONAL, NOSAVE,	GLOBAL},

	/* Emoji */
	{'s',	"emoji.name1",		&conf_emoji_name[0],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name2",		&conf_emoji_name[1],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name3",		&conf_emoji_name[2],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name4",		&conf_emoji_name[3],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name5",		&conf_emoji_name[4],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name6",		&conf_emoji_name[5],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name7",		&conf_emoji_name[6],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name8",		&conf_emoji_name[7],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name9",		&conf_emoji_name[8],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name10",		&conf_emoji_name[9],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name11",		&conf_emoji_name[10],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name12",		&conf_emoji_name[11],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name13",		&conf_emoji_name[12],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name14",		&conf_emoji_name[13],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name15",		&conf_emoji_name[14],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name16",		&conf_emoji_name[15],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name17",		&conf_emoji_name[16],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name18",		&conf_emoji_name[17],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name19",		&conf_emoji_name[18],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name20",		&conf_emoji_name[19],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name21",		&conf_emoji_name[20],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name22",		&conf_emoji_name[21],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name23",		&conf_emoji_name[22],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name24",		&conf_emoji_name[23],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name25",		&conf_emoji_name[24],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name26",		&conf_emoji_name[25],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name27",		&conf_emoji_name[26],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name28",		&conf_emoji_name[27],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name29",		&conf_emoji_name[28],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name30",		&conf_emoji_name[29],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name31",		&conf_emoji_name[30],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.name32",		&conf_emoji_name[31],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image1",		&conf_emoji_image[0],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image2",		&conf_emoji_image[1],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image3",		&conf_emoji_image[2],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image4",		&conf_emoji_image[3],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image5",		&conf_emoji_image[4],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image6",		&conf_emoji_image[5],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image7",		&conf_emoji_image[6],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image8",		&conf_emoji_image[7],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image9",		&conf_emoji_image[8],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image10",	&conf_emoji_image[9],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image11",	&conf_emoji_image[10],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image12",	&conf_emoji_image[11],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image13",	&conf_emoji_image[12],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image14",	&conf_emoji_image[13],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image15",	&conf_emoji_image[14],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image16",	&conf_emoji_image[15],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image17",	&conf_emoji_image[16],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image18",	&conf_emoji_image[17],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image19",	&conf_emoji_image[18],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image20",	&conf_emoji_image[19],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image21",	&conf_emoji_image[20],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image22",	&conf_emoji_image[21],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image23",	&conf_emoji_image[22],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image24",	&conf_emoji_image[23],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image25",	&conf_emoji_image[24],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image26",	&conf_emoji_image[25],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image27",	&conf_emoji_image[26],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image28",	&conf_emoji_image[27],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image29",	&conf_emoji_image[28],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image30",	&conf_emoji_image[29],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image31",	&conf_emoji_image[30],			OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"emoji.image32",	&conf_emoji_image[31],			OPTIONAL, NOSAVE,	GLOBAL},

	/* Text-to-speech */
	{'b',	"tts.enable",		&conf_tts_enable,			MUST,	SAVE,	LOCAL},

	/* Release Mode */
	{'b',	"release_mode.enable",	&conf_release_mode_enable,		MUST,	NOSAVE,	GLOBAL},
};

#define RULE_TBL_SIZE	((int)(sizeof(rule_tbl) / sizeof(struct rule)))

/* Flags to indicate whether each config is loaded or not. */
static bool loaded_tbl[RULE_TBL_SIZE];

/*
 * Forward Declarations
 */
static bool read_conf(void);
static bool save_value(const char *k, const char *v);
static bool check_conf(void);
static bool apply_initial_values(void);
static bool overwrite_config_game_locale(const char *val);
static bool overwrite_config_font_ttf1(const char *val);

/*
 * Initialize the config subsystem.
 */
bool
s3i_init_conf(void)
{
	/* Cleanup for when DLL is recycled. */
	s3i_cleanup_conf();

	/* Read "config.ini" file. */
	if (!read_conf())
		return false;

	/* Check for non-existent config keys that has a MUST flag. */
	if (!check_conf())
		return false;

	/* Initialize various variables by config values. */
	if (!apply_initial_values())
		return false;

	return true;
}

/*
 * Cleanup the config subsystem.
 */
void
s3i_cleanup_conf(void)
{
	int i;

	/* Free string values. */
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		loaded_tbl[i] = false;
		if (rule_tbl[i].type == 's' && rule_tbl[i].var_ptr != NULL) {
			free(*(char **)rule_tbl[i].var_ptr);
			*(char **)rule_tbl[i].var_ptr = NULL;
		}
	}
}

/*
 * Set a config.
 */
bool
s3_set_config(
	const char *key,
	const char *val)
{
	int i;
	char *s;

	assert(key != NULL);
	assert(val != NULL);

	/* Search for a key index. */
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0)
			break;	/* Found. */
	}
	if (i == RULE_TBL_SIZE)
		return false;

	/* Ignore a NULL-valued key. */
	if (rule_tbl[i].var_ptr == NULL)
		return true;

	/* Assign by the type. */
	switch (rule_tbl[i].type) {
	case 'b':
		/* var_ptr is a pointer to a bool var. */
		*(bool *)rule_tbl[i].var_ptr = (strcmp(val, "yes") == 0 || strcmp(val, "1") == 0) ? true : false;
		break;
	case 'i':
		/* var_ptr is a pointer to an int var. */
		*(int *)rule_tbl[i].var_ptr = atoi(val);
		break;
	case 'f':
		/* var_ptr is a pointer to a float var. */
		*(float *)rule_tbl[i].var_ptr = (float)atof(val);
		break;
	case 's':
		/* Free the previous string. */
		if (*(char **)rule_tbl[i].var_ptr != NULL) {
			free(*(char **)rule_tbl[i].var_ptr);
			*(char **)rule_tbl[i].var_ptr = NULL;
		}

		/* Duplicate the new string. */
		if (strcmp(val, "") != 0) {
			s = strdup(val);
			if (s == NULL) {
				s3_log_out_of_memory();
				return false;
			}

			/* var_ptr is a pointer to char* var. */
			*(char **)rule_tbl[i].var_ptr = s;
		}
		break;
	default:
		assert(INVALID_CONFIG_TYPE);
		break;
	}

	/* Update the layer (x, y) positions by config keys. */
	s3_reload_stage_positions();

	/* Postprocess for image load. */
	if (strcmp(key, "msgbox.image") == 0)
		s3i_setup_msgbox();
	else if (strcmp(key, "namebox.image") == 0)
		s3i_setup_namebox();
	else if (strcmp(key, "choose.idle1") == 0)
		s3i_setup_choose(false, 0);
	else if (strcmp(key, "choose.hover1") == 0)
		s3i_setup_choose(false, 0);
	else if (strcmp(key, "choose.idle2") == 0)
		s3i_setup_choose(false, 1);
	else if (strcmp(key, "choose.hover2") == 0)
		s3i_setup_choose(true, 1);
	else if (strcmp(key, "choose.idle3") == 0)
		s3i_setup_choose(false, 2);
	else if (strcmp(key, "choose.hover3") == 0)
		s3i_setup_choose(true, 2);
	else if (strcmp(key, "choose.idl4") == 0)
		s3i_setup_choose(false, 3);
	else if (strcmp(key, "choose.hover4") == 0)
		s3i_setup_choose(true, 3);
	else if (strcmp(key, "choose.idle5") == 0)
		s3i_setup_choose(false, 4);
	else if (strcmp(key, "choose.hover5") == 0)
		s3i_setup_choose(true, 4);
	else if (strcmp(key, "choose.idle6") == 0)
		s3i_setup_choose(false, 5);
	else if (strcmp(key, "choose.hover6") == 0)
		s3i_setup_choose(true, 5);
	else if (strcmp(key, "choose.idle7") == 0)
		s3i_setup_choose(false, 6);
	else if (strcmp(key, "choose.hover7") == 0)
		s3i_setup_choose(true, 6);
	else if (strcmp(key, "choose.idle8") == 0)
		s3i_setup_choose(false, 7);
	else if (strcmp(key, "choose.hover8") == 0)
		s3i_setup_choose(true, 7);

	return true;
}

/*
 * Get the number of the config keys.
 */
int
s3_get_config_count(void)
{
	return RULE_TBL_SIZE;
}

/*
 * Get a config key for index.
 */
const char *
s3_get_config_key(
	int index)
{
	int i, save_key_count;

	save_key_count = 0;
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (!rule_tbl[i].save)
			continue;
		if (save_key_count == index)
			return rule_tbl[i].key;
		save_key_count++;
	}
	return NULL;
}

/*
 * Check if config key is stored to global save data.
 */
bool
s3_is_config_global(
	const char *key)
{
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0)
			return rule_tbl[i].global;
	}

	return false;
}

/*
 * Get a config value type. ('s', 'b', 'i', 'f')
 */
char
s3_get_config_type(
	const char *key)
{
	int i;

	assert(key != NULL);

	for (i = 0; i < RULE_TBL_SIZE; i++)
		if (strcmp(rule_tbl[i].key, key) == 0)
			return rule_tbl[i].type;

	assert(CONFIG_KEY_NOT_FOUND);
	return '?';
}

/*
 * Get a string config value.
 */
const char *
s3_get_config_string(
	const char *key)
{
	int i;

	assert(key != NULL);
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			assert(rule_tbl[i].type == 's');
			if (rule_tbl[i].var_ptr == NULL)
				return "";
			return *(char **)rule_tbl[i].var_ptr;
		}
	}
	assert(CONFIG_KEY_NOT_FOUND);
	return NULL;
}

/*
 * Get a boolean config value.
 */
bool
s3_get_config_bool(
	const char *key)
{
	int i;

	assert(key != NULL);
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			assert(rule_tbl[i].type == 'b');
			assert(rule_tbl[i].var_ptr != NULL);
			return *(bool *)rule_tbl[i].var_ptr;
		}
	}
	assert(CONFIG_KEY_NOT_FOUND);
	return -1;
}

/*
 * Get an integer config value.
 */
int
s3_get_config_int(
	const char *key)
{
	int i;

	assert(key != NULL);
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			assert(rule_tbl[i].type == 'i');
			assert(rule_tbl[i].var_ptr != NULL);
			return *(int *)rule_tbl[i].var_ptr;
		}
	}
	assert(CONFIG_KEY_NOT_FOUND);
	return -1;
}

/*
 * Get a float config value.
 */
float
s3_get_config_float(
	const char *key)
{
	int i;

	assert(key != NULL);
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			assert(rule_tbl[i].type == 'f');
			assert(rule_tbl[i].var_ptr != NULL);
			return *(float *)rule_tbl[i].var_ptr;
		}
	}
	assert(CONFIG_KEY_NOT_FOUND);
	return 0;
}

/*
 * Get a config value as a string.
 */
const char *
s3_get_config_as_string(
	const char *key)
{
	static char buf[128];
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			switch (rule_tbl[i].type) {
			case 'b':
				if (*(bool *)rule_tbl[i].var_ptr)
					snprintf(buf, sizeof(buf), "true");
				else
					snprintf(buf, sizeof(buf), "false");
				return buf;
			case 'i':
				snprintf(buf, sizeof(buf), "%d", *(int *)rule_tbl[i].var_ptr);
				return buf;
			case 'f':
				snprintf(buf, sizeof(buf), "%f", *(float *)rule_tbl[i].var_ptr);
				return buf;
			case 's':
				return *(char **)rule_tbl[i].var_ptr;
			default:
				assert(INVALID_CONFIG_TYPE);
				break;
			}
		}
	}

	return NULL;
}

/*
 * Check if the specified locale is same as the current locale.
 */
bool
s3_compare_locale(const char *s)
{
	const char *loc;

	assert(s != NULL);

	if (conf_game_locale != NULL)
		loc = conf_game_locale;
	else
		loc = pf_get_system_language();

	if (strcmp(loc, s) == 0)
		return true;

	return false;
}

/* Read the config file. */
static bool read_conf(void)
{
	char *file_buf;
	char *line_ptr;
	char *end_line_ptr;
	int line;
	bool eof;
	char *k, *v;
	size_t len;

	if (!pf_read_file_content(S3_PATH_CONFIG, &file_buf, &len))
		return false;

	line_ptr = file_buf;
	line = 0;
	while (*line_ptr) {
		line++;

		/* If an empty line. */
		if (*line_ptr == '\n') {
			line_ptr++;
			continue;
		}

		/* If a comment line. */
		if (*line_ptr == '#') {
			/* Skip the line. */
			while (*line_ptr != '\n' && *line_ptr != '\0')
				line_ptr++;
			if (*line_ptr == '\0')
				break; /* EOF */
			if (*line_ptr == '\n')
				line_ptr++;
			continue;
		}

		/* Terminate the string by NUL. */
		end_line_ptr = line_ptr;
		while (*end_line_ptr != '\n' && *end_line_ptr != '\0')
			end_line_ptr++;
		if (*end_line_ptr == '\n') {
			*end_line_ptr = '\0';
			eof = false;
		} else {
			eof = true;
		}

		/* Get a key name. */
		k = strtok(line_ptr, "=");
		if (k == NULL || k[0] == '\0')
			continue;

		/* Get a value string. */
		v = strtok(NULL, "=");
		if (v == NULL || v[0] == '\0') {
			s3_log_error(
				S3_TR("Invalid config line at file %s line %d."),
				S3_PATH_CONFIG,
				line);
			free(file_buf);
			return false;
		}

		/* Store the value. */
		if (!save_value(k, v)) {
			free(file_buf);
			return false;
		}

		line_ptr = end_line_ptr + 1;
	}

	free(file_buf);
	return true;
}

/* Store a key-value pair. */
static bool save_value(const char *k, const char *v)
{
	char *dup;
	int i;

	/* Search the rule table and store a value. */
	for (i = 0; i < RULE_TBL_SIZE; i++) {
		/* Go to next if the key doesn't match. */
		if (strcmp(rule_tbl[i].key, k) != 0)
			continue;

		/* If the key already has a value. */
		if (loaded_tbl[i]) {
			s3_log_error(S3_TR("Duplicated config key %s."), k);
			return false;
		}

		/* If an ignorable key. */
		if (rule_tbl[i].var_ptr == NULL)
			return true;

		/* Store by a type. */
		if (rule_tbl[i].type == 'b') {
			*(bool *)rule_tbl[i].var_ptr = (strcmp(v, "true") == 0 || strcmp(v, "yes") == 0) ? true : false;
		} else if (rule_tbl[i].type == 'i') {
			*(int *)rule_tbl[i].var_ptr = atoi(v);
		} else if (rule_tbl[i].type == 'f') {
			*(float *)rule_tbl[i].var_ptr = (float)atof(v);
		} else if (rule_tbl[i].type == 's') {
			/* Duplicate a string. */
			dup = strdup(v);
			if (dup == NULL) {
				s3_log_out_of_memory();
				return false;
			}
			*(char **)rule_tbl[i].var_ptr = dup;
		} else {
			assert(0);
		}

		loaded_tbl[i] = true;

		return true;
	}

	s3_log_error(S3_TR("Unknown config key %s"), k);

	return false;
}

/* Check for non-existent config keys that has a MUST flag. */
static bool check_conf(void)
{
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (rule_tbl[i].must && !loaded_tbl[i]) {
			s3_log_error(S3_TR("Undefined config key %s"), rule_tbl[i].key);
			return false;
		}
	}
	return true;
}

/* Initialize various variables by config values. */
static bool
apply_initial_values(void)
{
	int i;

	/*
	 * Set global volumes.
	 *  - These values will changed in load_global_data() if global save data exist.
	 */
	s3_set_mixer_global_volume(S3_TRACK_BGM, conf_sound_vol_bgm);
	s3_set_mixer_global_volume(S3_TRACK_VOICE, conf_sound_vol_voice);
	s3_set_mixer_global_volume(S3_TRACK_SE, conf_sound_vol_se);

	/* Set character volumes. */
	for (i = 0; i < S3_CH_VOL_SLOTS; i++)
		s3_set_character_volume(i, conf_sound_vol_per_character);

	/* Count click anime frames. */
	for (conf_click_frames = 16; conf_click_frames > 1; conf_click_frames--)
		if (conf_click_image[conf_click_frames - 1] != NULL)
			break;

	return true;
}
