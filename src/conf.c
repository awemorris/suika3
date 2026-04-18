/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Config Subsystem
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
#include "mixer.h"
#include "stage.h"
#include "save.h"

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

/* Animation */
char *conf_msgbox_anime_hide;
char *conf_msgbox_anime_show;

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
bool conf_msgbox_skip_unseen;

/*
 * Name Box Settings
 */

/* Enable. */
bool conf_namebox_enable;

/* Image file. */
char *conf_namebox_image;

/* Anime. */
char *conf_namebox_anime_hide;
char *conf_namebox_anime_show;

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
char *conf_choose_idle_anime[S3_CHOOSEBOX_COUNT];
char *conf_choose_hover_anime[S3_CHOOSEBOX_COUNT];

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

/* Anime */
char *conf_sysbtn_anime_out;
char *conf_sysbtn_anime_fadein;
char *conf_sysbtn_anime_appear;
char *conf_sysbtn_anime_hover;
char *conf_sysbtn_anime_fadeout;

/* Position */
int conf_sysbtn_x;
int conf_sysbtn_y;
int conf_sysbtn_width;
int conf_sysbtn_height;

/* SE */
char *conf_sysbtn_enter_se;
char *conf_sysbtn_leave_se;
char *conf_sysbtn_click_se;

/*
 * Auto Mode Settings
 */

/* File name */
char *conf_automode_image;

/* Animation */
char *conf_automode_anime_hide;
char *conf_automode_anime_show;

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

/* Animation */
char *conf_skipmode_anime_hide;
char *conf_skipmode_anime_show;

/* Position */
int conf_skipmode_x;
int conf_skipmode_y;

/* SE */
char *conf_skipmode_enter_se;
char *conf_skipmode_leave_se;

/*
 * GUI Settings
 */

/* Save index font */
int conf_gui_save_index_font_select;
int conf_gui_save_index_font_size;
int conf_gui_save_index_font_r;
int conf_gui_save_index_font_g;
int conf_gui_save_index_font_b;
int conf_gui_save_index_font_outline_width;
int conf_gui_save_index_font_outline_r;
int conf_gui_save_index_font_outline_g;
int conf_gui_save_index_font_outline_b;
int conf_gui_save_index_font_ruby;
bool conf_gui_save_index_font_tategaki;
int conf_gui_save_index_margin_char;

/* Save date font */
int conf_gui_save_date_font_select;
int conf_gui_save_date_font_size;
int conf_gui_save_date_font_r;
int conf_gui_save_date_font_g;
int conf_gui_save_date_font_b;
int conf_gui_save_date_font_outline_width;
int conf_gui_save_date_font_outline_r;
int conf_gui_save_date_font_outline_g;
int conf_gui_save_date_font_outline_b;
int conf_gui_save_date_font_ruby;
bool conf_gui_save_date_font_tategaki;
int conf_gui_save_date_margin_char;

/* Save chapter font */
int conf_gui_save_chapter_font_select;
int conf_gui_save_chapter_font_size;
int conf_gui_save_chapter_font_r;
int conf_gui_save_chapter_font_g;
int conf_gui_save_chapter_font_b;
int conf_gui_save_chapter_font_outline_width;
int conf_gui_save_chapter_font_outline_r;
int conf_gui_save_chapter_font_outline_g;
int conf_gui_save_chapter_font_outline_b;
int conf_gui_save_chapter_font_ruby;
bool conf_gui_save_chapter_font_tategaki;
int conf_gui_save_chapter_margin_char;

/* Save message font */
int conf_gui_save_msg_font_select;
int conf_gui_save_msg_font_size;
int conf_gui_save_msg_font_r;
int conf_gui_save_msg_font_g;
int conf_gui_save_msg_font_b;
int conf_gui_save_msg_font_outline_width;
int conf_gui_save_msg_font_outline_r;
int conf_gui_save_msg_font_outline_g;
int conf_gui_save_msg_font_outline_b;
int conf_gui_save_msg_font_ruby;
bool conf_gui_save_msg_font_tategaki;
int conf_gui_save_msg_margin_line;
int conf_gui_save_msg_margin_char;
bool conf_gui_save_msg_multiline;

/* History Name Font */
int conf_gui_history_name_font_select;
int conf_gui_history_name_font_size;
int conf_gui_history_name_font_r;
int conf_gui_history_name_font_g;
int conf_gui_history_name_font_b;
int conf_gui_history_name_font_outline_width;
int conf_gui_history_name_font_outline_r;
int conf_gui_history_name_font_outline_g;
int conf_gui_history_name_font_outline_b;
int conf_gui_history_name_font_ruby;
bool conf_gui_history_name_font_tategaki;
int conf_gui_history_name_margin_line;
int conf_gui_history_name_margin_char;

/* History Text Font */
int conf_gui_history_text_font_select;
int conf_gui_history_text_font_size;
int conf_gui_history_text_font_r;
int conf_gui_history_text_font_g;
int conf_gui_history_text_font_b;
int conf_gui_history_text_font_outline_width;
int conf_gui_history_text_font_outline_r;
int conf_gui_history_text_font_outline_g;
int conf_gui_history_text_font_outline_b;
int conf_gui_history_text_font_ruby;
bool conf_gui_history_text_font_tategaki;
int conf_gui_history_text_margin_line;
int conf_gui_history_text_margin_char;

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

/* Character name and folder mapping */
char *conf_character_name[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_en[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_fr[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_es[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_de[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_it[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_ru[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_el[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_zh_cn[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_zh_tw[S3_CHARACTER_MAP_COUNT];
char *conf_character_name_ja[S3_CHARACTER_MAP_COUNT];
char *conf_character_folder[S3_CHARACTER_MAP_COUNT];

/* Eye-blink */
float conf_character_eyeblink_interval;
float conf_character_eyeblink_frame;

/* Lip-sync */
float conf_character_lipsync_frame;
int conf_character_lipsync_chars;

/*
 * Auto Focus
 */

bool conf_autofocus_on_text_name;
bool conf_autofocus_on_text_no_name;
bool conf_autofocus_on_ch;
bool conf_autofocus_on_choose;

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
	{'s',	"game.title.en",		&conf_game_title_en,			MUST,		NOSAVE,	GLOBAL},
	{'s',	"game.title.ja",		&conf_game_title_ja,			MUST,		NOSAVE,	GLOBAL},
	{'b',	"game.novel",			&conf_game_novel,			MUST,		SAVE,	LOCAL},
	{'s',	"game.locale",			&conf_game_locale,			OPTIONAL,	SAVE,	GLOBAL},

	/* Font */
	{'s',	"font.ttf1",			&conf_font_ttf[0],			MUST,		SAVE,	GLOBAL},
	{'s',	"font.ttf2",			&conf_font_ttf[1],			OPTIONAL,	SAVE,	GLOBAL},
	{'s',	"font.ttf3",			&conf_font_ttf[2],			OPTIONAL,	SAVE,	GLOBAL},
	{'s',	"font.ttf4",			&conf_font_ttf[3],			OPTIONAL,	SAVE,	GLOBAL},

	/* Message Box */
	{'s',	"msgbox.image",			&conf_msgbox_image,			MUST,	SAVE,	LOCAL},
	{'s',	"msgbox.anime.hide",		&conf_msgbox_anime_hide,		MUST,	SAVE,	LOCAL},
	{'s',	"msgbox.anime.show",		&conf_msgbox_anime_show,		MUST,	SAVE,	LOCAL},
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
	{'b',	"msgbox.skip_unseen",		&conf_msgbox_skip_unseen,		MUST,	SAVE,	LOCAL},

	/* Name Box */
	{'b',	"namebox.enable",		&conf_namebox_enable,			MUST,	SAVE,	LOCAL},
	{'s',	"namebox.image",		&conf_namebox_image,			MUST,	SAVE,	LOCAL},
	{'s',	"namebox.anime.hide",		&conf_namebox_anime_hide,		MUST,	SAVE,	LOCAL},
	{'s',	"namebox.anime.show",		&conf_namebox_anime_show,		MUST,	SAVE,	LOCAL},
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
	{'s',	"choose.box1.idle_anime",	&conf_choose_idle_anime[0],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box1.hover_anime",	&conf_choose_hover_anime[0],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box2.idle",		&conf_choose_idle[1],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box2.hover",		&conf_choose_hover[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.x",		&conf_choose_x[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.y",		&conf_choose_y[1],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box2.margin.top",	&conf_choose_margin_top[1],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box2.idle_anime",	&conf_choose_idle_anime[1],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box2.hover_anime",	&conf_choose_hover_anime[1],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box3.idle",		&conf_choose_idle[2],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box3.hover",		&conf_choose_hover[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.x",		&conf_choose_x[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.y",		&conf_choose_y[2],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box3.margin.top",	&conf_choose_margin_top[2],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box3.idle_anime",	&conf_choose_idle_anime[2],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box3.hover_anime",	&conf_choose_hover_anime[2],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box4.idle",		&conf_choose_idle[3],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box4.hover",		&conf_choose_hover[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.x",		&conf_choose_x[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.y",		&conf_choose_y[3],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box4.margin.top",	&conf_choose_margin_top[3],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box4.idle_anime",	&conf_choose_idle_anime[3],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box4.hover_anime",	&conf_choose_hover_anime[3],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box5.idle",		&conf_choose_idle[4],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box5.hover",		&conf_choose_hover[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.x",		&conf_choose_x[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.y",		&conf_choose_y[4],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box5.margin.top",	&conf_choose_margin_top[4],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box5.idle_anime",	&conf_choose_idle_anime[4],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box5.hover_anime",	&conf_choose_hover_anime[4],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box6.idle",		&conf_choose_idle[5],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box6.hover",		&conf_choose_hover[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.x",		&conf_choose_x[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.y",		&conf_choose_y[5],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box6.margin.top",	&conf_choose_margin_top[5],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box6.idle_anime",	&conf_choose_idle_anime[5],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box6.hover_anime",	&conf_choose_hover_anime[5],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box7.idle",		&conf_choose_idle[6],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box7.hover",		&conf_choose_hover[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.x",		&conf_choose_x[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.y",		&conf_choose_y[6],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box7.margin.top",	&conf_choose_margin_top[6],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box7.idle_anime",	&conf_choose_idle_anime[6],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box7.hover_anime",	&conf_choose_hover_anime[6],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box8.idle",		&conf_choose_idle[7],			MUST,	SAVE,	LOCAL},
	{'s',	"choose.box8.hover",		&conf_choose_hover[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.x",		&conf_choose_x[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.y",		&conf_choose_y[7],			MUST,	SAVE,	LOCAL},
	{'i',	"choose.box8.margin.top",	&conf_choose_margin_top[7],		MUST,	SAVE,	LOCAL},
	{'s',	"choose.box8.idle_anime",	&conf_choose_idle_anime[7],		OPTIONAL, SAVE,	LOCAL},
	{'s',	"choose.box8.hover_anime",	&conf_choose_hover_anime[7],		OPTIONAL, SAVE,	LOCAL},

	/* Save Data */
	{'i',	"save.thumb.width",		&conf_save_thumb_width,			MUST,	NOSAVE,	GLOBAL},
	{'i',	"save.thumb.height",		&conf_save_thumb_height,		MUST,	NOSAVE,	GLOBAL},
	{'s',	"save.new_image",		&conf_save_new_image,			OPTIONAL, NOSAVE,	GLOBAL},

	/* System Button */
	{'b',	"sysbtn.enable",		&conf_sysbtn_enable,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.idle",			&conf_sysbtn_idle,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.hover",			&conf_sysbtn_hover,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.anime.out",		&conf_sysbtn_anime_out,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.anime.fadein",		&conf_sysbtn_anime_fadein,		MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.anime.appear",		&conf_sysbtn_anime_appear,		MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.anime.hover",		&conf_sysbtn_anime_hover,		MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.anime.fadeout",		&conf_sysbtn_anime_fadeout,		MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.x",			&conf_sysbtn_x,				MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.y",			&conf_sysbtn_y,				MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.width",			&conf_sysbtn_width,			MUST,	SAVE,	LOCAL},
	{'i',	"sysbtn.height",		&conf_sysbtn_height,			MUST,	SAVE,	LOCAL},
	{'s',	"sysbtn.enter_se",		&conf_sysbtn_enter_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"sysbtn.leave_se",		&conf_sysbtn_leave_se,			OPTIONAL, SAVE,	LOCAL},
	{'s',	"sysbtn.click_se",		&conf_sysbtn_click_se,			OPTIONAL, SAVE,	LOCAL},

	/* Auto Mode */
	{'s',	"automode.image",		&conf_automode_image,			MUST,	SAVE,	LOCAL},
	{'s',	"automode.anime.hide",		&conf_automode_anime_hide,		MUST,	SAVE,	LOCAL},
	{'s',	"automode.anime.show",		&conf_automode_anime_show,		MUST,	SAVE,	LOCAL},
	{'i',	"automode.x",			&conf_automode_x,			MUST,	SAVE,	LOCAL},
	{'i',	"automode.y",			&conf_automode_y,			MUST,	SAVE,	LOCAL},
	{'s',	"automode.enter_se",		&conf_automode_enter_se,		OPTIONAL, SAVE,	LOCAL},
	{'s',	"automode.leave_se",		&conf_automode_leave_se,		OPTIONAL, SAVE,	LOCAL},

	/* Skip Mode */
	{'s',	"skipmode.image",		&conf_skipmode_image,			MUST,	SAVE,	LOCAL},
	{'s',	"skipmode.anime.hide",		&conf_skipmode_anime_hide,		MUST,	SAVE,	LOCAL},
	{'s',	"skipmode.anime.show",		&conf_skipmode_anime_show,		MUST,	SAVE,	LOCAL},
	{'i',	"skipmode.x",			&conf_skipmode_x,			MUST,	SAVE,	LOCAL},
	{'i',	"skipmode.y",			&conf_skipmode_y,			MUST,	SAVE,	LOCAL},
	{'s',	"skipmode.enter_se",		&conf_skipmode_enter_se,		OPTIONAL, SAVE,	LOCAL},
	{'s',	"skipmode.leave_se",		&conf_skipmode_leave_se,		OPTIONAL, SAVE,	LOCAL},

	/* GUI */
	{'i',	"gui.save.index.font.select",		&conf_gui_save_index_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.size",		&conf_gui_save_index_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.r",		&conf_gui_save_index_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.g",		&conf_gui_save_index_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.b",		&conf_gui_save_index_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.outline.width",	&conf_gui_save_index_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.outline.r",	&conf_gui_save_index_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.outline.g",	&conf_gui_save_index_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.outline.b",	&conf_gui_save_index_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.font.ruby",		&conf_gui_save_index_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.index.font.tategaki",		&conf_gui_save_index_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.index.margin.char",		&conf_gui_save_index_margin_char,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.select",		&conf_gui_save_date_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.size",		&conf_gui_save_date_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.r",			&conf_gui_save_date_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.g",			&conf_gui_save_date_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.b",			&conf_gui_save_date_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.outline.width",	&conf_gui_save_date_font_outline_width,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.outline.r",		&conf_gui_save_date_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.outline.g",		&conf_gui_save_date_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.outline.b",		&conf_gui_save_date_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.font.ruby",		&conf_gui_save_date_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.date.font.tategaki",		&conf_gui_save_date_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.date.margin.char",		&conf_gui_save_date_margin_char,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.select",		&conf_gui_save_chapter_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.size",		&conf_gui_save_chapter_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.r",		&conf_gui_save_chapter_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.g",		&conf_gui_save_chapter_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.b",		&conf_gui_save_chapter_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.outline.width",	&conf_gui_save_chapter_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.outline.r",	&conf_gui_save_chapter_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.outline.g",	&conf_gui_save_chapter_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.outline.b",	&conf_gui_save_chapter_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.font.ruby",		&conf_gui_save_chapter_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.chapter.font.tategaki",	&conf_gui_save_chapter_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.chapter.margin.char",		&conf_gui_save_chapter_margin_char,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.select",		&conf_gui_save_msg_font_select,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.size",		&conf_gui_save_msg_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.r",			&conf_gui_save_msg_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.g",			&conf_gui_save_msg_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.b",			&conf_gui_save_msg_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.outline.width",	&conf_gui_save_msg_font_outline_width,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.outline.r",		&conf_gui_save_msg_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.outline.g",		&conf_gui_save_msg_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.outline.b",		&conf_gui_save_msg_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.font.ruby",		&conf_gui_save_msg_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.msg.font.tategaki",		&conf_gui_save_msg_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.margin.line",		&conf_gui_save_msg_margin_line,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.save.msg.margin.char",		&conf_gui_save_msg_margin_char,			MUST,	SAVE,	LOCAL},
	{'b',	"gui.save.msg.multiline",		&conf_gui_save_msg_multiline,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.select",		&conf_gui_history_name_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.size",		&conf_gui_history_name_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.r",		&conf_gui_history_name_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.g",		&conf_gui_history_name_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.b",		&conf_gui_history_name_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.outline.width",	&conf_gui_history_name_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.outline.r",	&conf_gui_history_name_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.outline.g",	&conf_gui_history_name_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.outline.b",	&conf_gui_history_name_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.font.ruby",		&conf_gui_history_name_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.history.name.font.tategaki",	&conf_gui_history_name_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.margin.line",		&conf_gui_history_text_margin_line,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.name.margin.char",		&conf_gui_history_text_margin_char,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.select",		&conf_gui_history_text_font_select,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.size",		&conf_gui_history_text_font_size,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.r",		&conf_gui_history_text_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.g",		&conf_gui_history_text_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.b",		&conf_gui_history_text_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.outline.width",	&conf_gui_history_text_font_outline_width,	MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.outline.r",	&conf_gui_history_text_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.outline.g",	&conf_gui_history_text_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.outline.b",	&conf_gui_history_text_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.font.ruby",		&conf_gui_history_text_font_ruby,		MUST,	SAVE,	LOCAL},
	{'b',	"gui.history.text.font.tategaki",	&conf_gui_history_text_font_tategaki,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.margin.line",		&conf_gui_history_text_margin_line,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.history.text.margin.char",		&conf_gui_history_text_margin_char,		MUST,	SAVE,	LOCAL},
	{'s',	"gui.history.quote.name_separator",	&conf_gui_history_quote_name_separator,		MUST,	SAVE,	LOCAL},
	{'s',	"gui.history.quote.start",		&conf_gui_history_quote_start, 			MUST,	SAVE,	LOCAL},
	{'s',	"gui.history.quote.end",		&conf_gui_history_quote_end, 			MUST,	SAVE,	LOCAL},
	{'b',	"gui.history.hide_last",		&conf_gui_history_hide_last,	 		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.select",		&conf_gui_preview_font_select,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.size",		&conf_gui_preview_font_size,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.r",			&conf_gui_preview_font_r,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.g",			&conf_gui_preview_font_g,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.b",			&conf_gui_preview_font_b,			MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.width",	&conf_gui_preview_font_outline_width,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.r",		&conf_gui_preview_font_outline_r,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.g",		&conf_gui_preview_font_outline_g,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.outline.b",		&conf_gui_preview_font_outline_b,		MUST,	SAVE,	LOCAL},
	{'i',	"gui.preview.font.ruby",		&conf_gui_preview_font_ruby,			MUST,	SAVE,	LOCAL},
	{'b',	"gui.preview.font.tategaki",		&conf_gui_preview_font_tategaki,		MUST,	SAVE,	LOCAL},

	/* Initial Volumes (no need to save) */
	{'f',	"sound.vol.bgm",		&conf_sound_vol_bgm,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.voice",		&conf_sound_vol_voice,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.se",			&conf_sound_vol_se,			MUST,	NOSAVE,	GLOBAL},
	{'f',	"sound.vol.per_character",	&conf_sound_vol_per_character,		MUST,	NOSAVE,	GLOBAL},

	/* Character */
	{'f',	"character.eyeblink.interval",	&conf_character_eyeblink_interval,	MUST,	NOSAVE,	GLOBAL},
	{'f',	"character.eyeblink.frame",	&conf_character_eyeblink_frame,		MUST,	NOSAVE, GLOBAL},
	{'f',	"character.lipsync.frame",	&conf_character_lipsync_frame,		MUST,	NOSAVE, GLOBAL},
	{'i',	"character.lipsync.chars",	&conf_character_lipsync_chars,		MUST,	NOSAVE, GLOBAL},

	/* Character Name to File Mapping */
	{'s',	"character.name1",		&conf_character_name[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.en",		&conf_character_name_en[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.fr",		&conf_character_name_fr[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.es",		&conf_character_name_es[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.de",		&conf_character_name_de[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.it",		&conf_character_name_it[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.ru",		&conf_character_name_ru[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.el",		&conf_character_name_el[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.zh-cn",	&conf_character_name_zh_cn[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.zh-tw",	&conf_character_name_zh_tw[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name1.ja",		&conf_character_name_ja[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder1",		&conf_character_folder[0],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2",		&conf_character_name[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.en",		&conf_character_name_en[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.fr",		&conf_character_name_fr[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.es",		&conf_character_name_es[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.de",		&conf_character_name_de[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.it",		&conf_character_name_it[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.ru",		&conf_character_name_ru[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.el",		&conf_character_name_el[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.zh-cn",	&conf_character_name_zh_cn[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.zh-tw",	&conf_character_name_zh_tw[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name2.ja",		&conf_character_name_ja[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder2",		&conf_character_folder[1],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3",		&conf_character_name[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.en",		&conf_character_name_en[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.fr",		&conf_character_name_fr[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.es",		&conf_character_name_es[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.de",		&conf_character_name_de[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.it",		&conf_character_name_it[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.ru",		&conf_character_name_ru[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.el",		&conf_character_name_el[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.zh-cn",	&conf_character_name_zh_cn[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.zh-tw",	&conf_character_name_zh_tw[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name3.ja",		&conf_character_name_ja[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder3",		&conf_character_folder[2],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4",		&conf_character_name[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.en",		&conf_character_name_en[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.fr",		&conf_character_name_fr[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.es",		&conf_character_name_es[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.de",		&conf_character_name_de[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.it",		&conf_character_name_it[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.ru",		&conf_character_name_ru[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.el",		&conf_character_name_el[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.zh-cn",	&conf_character_name_zh_cn[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.zh-tw",	&conf_character_name_zh_tw[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name4.ja",		&conf_character_name_ja[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder4",		&conf_character_folder[3],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5",		&conf_character_name[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.en",		&conf_character_name_en[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.fr",		&conf_character_name_fr[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.es",		&conf_character_name_es[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.de",		&conf_character_name_de[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.it",		&conf_character_name_it[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.ru",		&conf_character_name_ru[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.el",		&conf_character_name_el[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.zh-cn",	&conf_character_name_zh_cn[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.zh-tw",	&conf_character_name_zh_tw[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name5.ja",		&conf_character_name_ja[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder5",		&conf_character_folder[4],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6",		&conf_character_name[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.en",		&conf_character_name_en[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.fr",		&conf_character_name_fr[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.es",		&conf_character_name_es[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.de",		&conf_character_name_de[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.it",		&conf_character_name_it[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.ru",		&conf_character_name_ru[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.el",		&conf_character_name_el[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.zh-cn",	&conf_character_name_zh_cn[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.zh-tw",	&conf_character_name_zh_tw[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name6.ja",		&conf_character_name_ja[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder6",		&conf_character_folder[5],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7",		&conf_character_name[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.en",		&conf_character_name_en[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.fr",		&conf_character_name_fr[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.es",		&conf_character_name_es[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.de",		&conf_character_name_de[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.it",		&conf_character_name_it[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.ru",		&conf_character_name_ru[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.el",		&conf_character_name_el[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.zh-cn",	&conf_character_name_zh_cn[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.zh-tw",	&conf_character_name_zh_tw[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name7.ja",		&conf_character_name_ja[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder7",		&conf_character_folder[6],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8",		&conf_character_name[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.en",		&conf_character_name_en[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.fr",		&conf_character_name_fr[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.es",		&conf_character_name_es[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.de",		&conf_character_name_de[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.it",		&conf_character_name_it[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.ru",		&conf_character_name_ru[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.el",		&conf_character_name_el[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.zh-cn",	&conf_character_name_zh_cn[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.zh-tw",	&conf_character_name_zh_tw[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name8.ja",		&conf_character_name_ja[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder8",		&conf_character_folder[7],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9",		&conf_character_name[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.en",		&conf_character_name_en[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.fr",		&conf_character_name_fr[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.es",		&conf_character_name_es[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.de",		&conf_character_name_de[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.it",		&conf_character_name_it[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.ru",		&conf_character_name_ru[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.el",		&conf_character_name_el[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.zh-cn",	&conf_character_name_zh_cn[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.zh-tw",	&conf_character_name_zh_tw[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name9.ja",		&conf_character_name_ja[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder9",		&conf_character_folder[8],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10",		&conf_character_name[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.en",		&conf_character_name_en[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.fr",		&conf_character_name_fr[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.es",		&conf_character_name_es[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.de",		&conf_character_name_de[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.it",		&conf_character_name_it[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.ru",		&conf_character_name_ru[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.el",		&conf_character_name_el[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.zh-cn",	&conf_character_name_zh_cn[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.zh-tw",	&conf_character_name_zh_tw[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name10.ja",		&conf_character_name_ja[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder10",		&conf_character_folder[9],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11",		&conf_character_name[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.en",		&conf_character_name_en[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.fr",		&conf_character_name_fr[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.es",		&conf_character_name_es[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.de",		&conf_character_name_de[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.it",		&conf_character_name_it[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.ru",		&conf_character_name_ru[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.el",		&conf_character_name_el[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.zh-cn",	&conf_character_name_zh_cn[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.zh-tw",	&conf_character_name_zh_tw[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name11.ja",		&conf_character_name_ja[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder11",		&conf_character_folder[10],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12",		&conf_character_name[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.en",		&conf_character_name_en[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.fr",		&conf_character_name_fr[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.es",		&conf_character_name_es[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.de",		&conf_character_name_de[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.it",		&conf_character_name_it[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.ru",		&conf_character_name_ru[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.el",		&conf_character_name_el[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.zh-cn",	&conf_character_name_zh_cn[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.zh-tw",	&conf_character_name_zh_tw[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name12.ja",		&conf_character_name_ja[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder12",		&conf_character_folder[11],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13",		&conf_character_name[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.en",		&conf_character_name_en[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.fr",		&conf_character_name_fr[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.es",		&conf_character_name_es[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.de",		&conf_character_name_de[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.it",		&conf_character_name_it[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.ru",		&conf_character_name_ru[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.el",		&conf_character_name_el[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.zh-cn",	&conf_character_name_zh_cn[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.zh-tw",	&conf_character_name_zh_tw[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name13.ja",		&conf_character_name_ja[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder13",		&conf_character_folder[12],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14",		&conf_character_name[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.en",		&conf_character_name_en[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.fr",		&conf_character_name_fr[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.es",		&conf_character_name_es[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.de",		&conf_character_name_de[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.it",		&conf_character_name_it[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.ru",		&conf_character_name_ru[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.el",		&conf_character_name_el[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.zh-cn",	&conf_character_name_zh_cn[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.zh-tw",	&conf_character_name_zh_tw[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name14.ja",		&conf_character_name_ja[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder14",		&conf_character_folder[13],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15",		&conf_character_name[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.en",		&conf_character_name_en[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.fr",		&conf_character_name_fr[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.es",		&conf_character_name_es[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.de",		&conf_character_name_de[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.it",		&conf_character_name_it[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.ru",		&conf_character_name_ru[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.el",		&conf_character_name_el[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.zh-cn",	&conf_character_name_zh_cn[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.zh-tw",	&conf_character_name_zh_tw[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name15.ja",		&conf_character_name_ja[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder15",		&conf_character_folder[14],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16",		&conf_character_name[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.en",		&conf_character_name_en[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.fr",		&conf_character_name_fr[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.es",		&conf_character_name_es[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.de",		&conf_character_name_de[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.it",		&conf_character_name_it[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.ru",		&conf_character_name_ru[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.el",		&conf_character_name_el[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.zh-cn",	&conf_character_name_zh_cn[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.zh-tw",	&conf_character_name_zh_tw[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name16.ja",		&conf_character_name_ja[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder16",		&conf_character_folder[15],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17",		&conf_character_name[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.en",		&conf_character_name_en[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.fr",		&conf_character_name_fr[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.es",		&conf_character_name_es[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.de",		&conf_character_name_de[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.it",		&conf_character_name_it[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.ru",		&conf_character_name_ru[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.el",		&conf_character_name_el[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.zh-cn",	&conf_character_name_zh_cn[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.zh-tw",	&conf_character_name_zh_tw[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name17.ja",		&conf_character_name_ja[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder17",		&conf_character_folder[16],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18",		&conf_character_name[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.en",		&conf_character_name_en[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.fr",		&conf_character_name_fr[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.es",		&conf_character_name_es[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.de",		&conf_character_name_de[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.it",		&conf_character_name_it[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.ru",		&conf_character_name_ru[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.el",		&conf_character_name_el[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.zh-cn",	&conf_character_name_zh_cn[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.zh-tw",	&conf_character_name_zh_tw[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name18.ja",		&conf_character_name_ja[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder18",		&conf_character_folder[17],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19",		&conf_character_name[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.en",		&conf_character_name_en[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.fr",		&conf_character_name_fr[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.es",		&conf_character_name_es[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.de",		&conf_character_name_de[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.it",		&conf_character_name_it[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.ru",		&conf_character_name_ru[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.el",		&conf_character_name_el[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.zh-cn",	&conf_character_name_zh_cn[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.zh-tw",	&conf_character_name_zh_tw[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name19.ja",		&conf_character_name_ja[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder19",		&conf_character_folder[18],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20",		&conf_character_name[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.en",		&conf_character_name_en[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.fr",		&conf_character_name_fr[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.es",		&conf_character_name_es[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.de",		&conf_character_name_de[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.it",		&conf_character_name_it[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.ru",		&conf_character_name_ru[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.el",		&conf_character_name_el[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.zh-cn",	&conf_character_name_zh_cn[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.zh-tw",	&conf_character_name_zh_tw[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name20.ja",		&conf_character_name_ja[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder20",		&conf_character_folder[19],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21",		&conf_character_name[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.en",		&conf_character_name_en[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.fr",		&conf_character_name_fr[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.es",		&conf_character_name_es[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.de",		&conf_character_name_de[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.it",		&conf_character_name_it[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.ru",		&conf_character_name_ru[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.el",		&conf_character_name_el[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.zh-cn",	&conf_character_name_zh_cn[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.zh-tw",	&conf_character_name_zh_tw[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name21.ja",		&conf_character_name_ja[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder21",		&conf_character_folder[20],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22",		&conf_character_name[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.en",		&conf_character_name_en[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.fr",		&conf_character_name_fr[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.es",		&conf_character_name_es[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.de",		&conf_character_name_de[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.it",		&conf_character_name_it[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.ru",		&conf_character_name_ru[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.el",		&conf_character_name_el[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.zh-cn",	&conf_character_name_zh_cn[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.zh-tw",	&conf_character_name_zh_tw[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name22.ja",		&conf_character_name_ja[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder22",		&conf_character_folder[21],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23",		&conf_character_name[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.en",		&conf_character_name_en[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.fr",		&conf_character_name_fr[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.es",		&conf_character_name_es[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.de",		&conf_character_name_de[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.it",		&conf_character_name_it[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.ru",		&conf_character_name_ru[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.el",		&conf_character_name_el[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.zh-cn",	&conf_character_name_zh_cn[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.zh-tw",	&conf_character_name_zh_tw[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name23.ja",		&conf_character_name_ja[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder23",		&conf_character_folder[22],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24",		&conf_character_name[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.en",		&conf_character_name_en[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.fr",		&conf_character_name_fr[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.es",		&conf_character_name_es[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.de",		&conf_character_name_de[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.it",		&conf_character_name_it[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.ru",		&conf_character_name_ru[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.el",		&conf_character_name_el[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.zh-cn",	&conf_character_name_zh_cn[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.zh-tw",	&conf_character_name_zh_tw[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name24.ja",		&conf_character_name_ja[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder24",		&conf_character_folder[23],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25",		&conf_character_name[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.en",		&conf_character_name_en[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.fr",		&conf_character_name_fr[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.es",		&conf_character_name_es[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.de",		&conf_character_name_de[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.it",		&conf_character_name_it[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.ru",		&conf_character_name_ru[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.el",		&conf_character_name_el[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.zh-cn",	&conf_character_name_zh_cn[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.zh-tw",	&conf_character_name_zh_tw[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name25.ja",		&conf_character_name_ja[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder25",		&conf_character_folder[24],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26",		&conf_character_name[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.en",		&conf_character_name_en[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.fr",		&conf_character_name_fr[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.es",		&conf_character_name_es[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.de",		&conf_character_name_de[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.it",		&conf_character_name_it[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.ru",		&conf_character_name_ru[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.el",		&conf_character_name_el[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.zh-cn",	&conf_character_name_zh_cn[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.zh-tw",	&conf_character_name_zh_tw[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name26.ja",		&conf_character_name_ja[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder26",		&conf_character_folder[25],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27",		&conf_character_name[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.en",		&conf_character_name_en[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.fr",		&conf_character_name_fr[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.es",		&conf_character_name_es[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.de",		&conf_character_name_de[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.it",		&conf_character_name_it[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.ru",		&conf_character_name_ru[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.el",		&conf_character_name_el[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.zh-cn",	&conf_character_name_zh_cn[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.zh-tw",	&conf_character_name_zh_tw[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name27.ja",		&conf_character_name_ja[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder27",		&conf_character_folder[26],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28",		&conf_character_name[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.en",		&conf_character_name_en[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.fr",		&conf_character_name_fr[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.es",		&conf_character_name_es[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.de",		&conf_character_name_de[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.it",		&conf_character_name_it[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.ru",		&conf_character_name_ru[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.el",		&conf_character_name_el[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.zh-cn",	&conf_character_name_zh_cn[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.zh-tw",	&conf_character_name_zh_tw[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name28.ja",		&conf_character_name_ja[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder28",		&conf_character_folder[27],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29",		&conf_character_name[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.en",		&conf_character_name_en[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.fr",		&conf_character_name_fr[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.es",		&conf_character_name_es[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.de",		&conf_character_name_de[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.it",		&conf_character_name_it[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.ru",		&conf_character_name_ru[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.el",		&conf_character_name_el[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.zh-cn",	&conf_character_name_zh_cn[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.zh-tw",	&conf_character_name_zh_tw[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name29.ja",		&conf_character_name_ja[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder29",		&conf_character_folder[28],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30",		&conf_character_name[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.en",		&conf_character_name_en[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.fr",		&conf_character_name_fr[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.es",		&conf_character_name_es[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.de",		&conf_character_name_de[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.it",		&conf_character_name_it[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.ru",		&conf_character_name_ru[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.el",		&conf_character_name_el[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.zh-cn",	&conf_character_name_zh_cn[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.zh-tw",	&conf_character_name_zh_tw[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name30.ja",		&conf_character_name_ja[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder30",		&conf_character_folder[29],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31",		&conf_character_name[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.en",		&conf_character_name_en[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.fr",		&conf_character_name_fr[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.es",		&conf_character_name_es[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.de",		&conf_character_name_de[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.it",		&conf_character_name_it[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.ru",		&conf_character_name_ru[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.el",		&conf_character_name_el[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.zh-cn",	&conf_character_name_zh_cn[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.zh-tw",	&conf_character_name_zh_tw[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name31.ja",		&conf_character_name_ja[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder31",		&conf_character_folder[30],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32",		&conf_character_name[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.en",		&conf_character_name_en[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.fr",		&conf_character_name_fr[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.es",		&conf_character_name_es[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.de",		&conf_character_name_de[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.it",		&conf_character_name_it[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.ru",		&conf_character_name_ru[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.el",		&conf_character_name_el[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.zh-cn",	&conf_character_name_zh_cn[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.zh-tw",	&conf_character_name_zh_tw[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.name32.ja",		&conf_character_name_ja[31],		OPTIONAL, NOSAVE,	GLOBAL},
	{'s',	"character.folder32",		&conf_character_folder[31],		OPTIONAL, NOSAVE,	GLOBAL},

	/* Auto Focus */
	{'b',	"autofocus.on_text_name",	&conf_autofocus_on_text_name,		OPTIONAL,	SAVE,	LOCAL},
	{'b',	"autofocus.on_text_no_name",	&conf_autofocus_on_text_no_name,	OPTIONAL,	SAVE,	LOCAL},
	{'b',	"autofocus.on_ch",		&conf_autofocus_on_ch,			OPTIONAL,	SAVE,	LOCAL},
	{'b',	"autofocus.on_choose",		&conf_autofocus_on_choose,		OPTIONAL,	SAVE,	LOCAL},

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

	/* Hook for ADV/NVL switching. */
	if (!s3i_is_load_in_progress() &&
	    strcmp(key, "game.novel") == 0 &&
	    !conf_game_novel &&
	    (strcmp(val, "true") == 0 ||
	     strcmp(val, "yes") == 0 ||
	     strcmp(val, "1") == 0)) {
		/* Flush the prev_last_message. */
		s3_set_last_message("");
		s3_set_last_message("");
	}

	/* Assign by the type. */
	switch (rule_tbl[i].type) {
	case 'b':
		/* var_ptr is a pointer to a bool var. */
		*(bool *)rule_tbl[i].var_ptr = 
			(strcmp(val, "yes") == 0 ||
			 strcmp(val, "true") == 0 ||
			 strcmp(val, "1") == 0) ?
			true : false;
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

	/* Postprocess for image load. */
	if (strcmp(key, "msgbox.image") == 0)
		s3i_setup_msgbox();
	else if (strcmp(key, "namebox.image") == 0)
		s3i_setup_namebox();
	else if (strcmp(key, "choose.box1.idle") == 0)
		s3i_setup_choose(false, 0);
	else if (strcmp(key, "choose.box1.hover") == 0)
		s3i_setup_choose(true, 0);
	else if (strcmp(key, "choose.box2.idle") == 0)
		s3i_setup_choose(false, 1);
	else if (strcmp(key, "choose.box2.hover") == 0)
		s3i_setup_choose(true, 1);
	else if (strcmp(key, "choose.box3.idle") == 0)
		s3i_setup_choose(false, 2);
	else if (strcmp(key, "choose.box3.hover") == 0)
		s3i_setup_choose(true, 2);
	else if (strcmp(key, "choose.box4.idl") == 0)
		s3i_setup_choose(false, 3);
	else if (strcmp(key, "choose.box4.hover") == 0)
		s3i_setup_choose(true, 3);
	else if (strcmp(key, "choose.box5.idle") == 0)
		s3i_setup_choose(false, 4);
	else if (strcmp(key, "choose.box5.hover") == 0)
		s3i_setup_choose(true, 4);
	else if (strcmp(key, "choose.box6.idle") == 0)
		s3i_setup_choose(false, 5);
	else if (strcmp(key, "choose.box6.hover") == 0)
		s3i_setup_choose(true, 5);
	else if (strcmp(key, "choose.box7.idle") == 0)
		s3i_setup_choose(false, 6);
	else if (strcmp(key, "choose.box7.hover") == 0)
		s3i_setup_choose(true, 6);
	else if (strcmp(key, "choose.box8.idle") == 0)
		s3i_setup_choose(false, 7);
	else if (strcmp(key, "choose.box8.hover") == 0)
		s3i_setup_choose(true, 7);

	/* Update the layer (x, y) positions by config keys. */
	s3_reload_stage_positions();

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
	return rule_tbl[index].key;
}

/*
 * Check if a config key is stored to global save data.
 */
bool
s3_is_global_save_config(
	const char *key)
{
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			/* Found the key. */
			if (rule_tbl[i].save && rule_tbl[i].global) {
				/* Save && Global. */
				return true;
			}
			return false;
		}
	}

	/* Not found. */
	return false;
}

/*
 * Check if a config is stored to local save data.
 */
int
s3_is_local_save_config(
	const char *key)
{
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (strcmp(rule_tbl[i].key, key) == 0) {
			/* Found the key. */
			if (rule_tbl[i].save && !rule_tbl[i].global) {
				/* Save && Global. */
				return true;
			}
			return false;
		}
	}

	/* Not found. */
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
s3_get_string_config(
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
s3_get_bool_config(
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
s3_get_int_config(
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
s3_get_float_config(
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
 * Get the locale.
 */
const char *
s3_get_locale(void)
{
	const char *loc;

	if (conf_game_locale != NULL)
		loc = conf_game_locale;
	else
		loc = pf_get_system_language();

	return loc;
}

/*
 * Get the major locale. (e.g. returns "en" for "en-us")
 */
const char *
s3i_get_major_locale(void)
{
	const char *locale;

	locale = s3_get_locale();
	assert(locale != NULL);
	
	if (strncmp(locale, "en-", 3) == 0)
		return "en";
	if (strncmp(locale, "fr-", 3) == 0)
		return "fr";
	if (strncmp(locale, "es-", 3) == 0)
		return "es";

	return NULL;
}

/* Read the config file. */
static bool read_conf(void)
{
	char *file_buf;
	char *line_ptr;
	char *end_line_ptr;
	int line;
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
			/* eof = false; */
		} else {
			/* eof = true; */
		}

		/* Get a key name. */
		k = strtok(line_ptr, "=");
		if (k == NULL || k[0] == '\0')
			continue;

		/* Get a value string. */
		v = strtok(NULL, "=");
		if (v == NULL || v[0] == '\0') {
			s3_log_error(
				S3_TR("Invalid config line at file \"%s\" line %d."),
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

	s3_log_error(S3_TR("Unknown config key \"%s\"."), k);

	return false;
}

/* Check for non-existent config keys that has a MUST flag. */
static bool check_conf(void)
{
	int i;

	for (i = 0; i < RULE_TBL_SIZE; i++) {
		if (rule_tbl[i].must && !loaded_tbl[i]) {
			s3_log_error(S3_TR("Undefined config key \"%s\"."), rule_tbl[i].key);
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
