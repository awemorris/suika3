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

#ifndef SUIKA3_CONF_H
#define SUIKA3_CONF_H

#include <suika3/suika3.h>

/*
 * Game Settings
 */

/* Game title */
extern char *conf_game_title;
extern char *conf_game_title_en;
extern char *conf_game_title_fr;
extern char *conf_game_title_es;
extern char *conf_game_title_de;
extern char *conf_game_title_it;
extern char *conf_game_title_ru;
extern char *conf_game_title_el;
extern char *conf_game_title_zh_cn;
extern char *conf_game_title_zh_tw;
extern char *conf_game_title_ja;

/* Novel mode */
extern bool conf_game_novel;

/* Locale */
extern char *conf_game_locale;

/*
 * Font File Settings
 */

/* TTF file name */
extern char *conf_font_ttf[4];

/*
 * Message Box
 */

/* Image file */
extern char *conf_msgbox_image;

/* Animation */
extern char *conf_msgbox_anime_hide;
extern char *conf_msgbox_anime_show;

/* Position */
extern int conf_msgbox_x;
extern int conf_msgbox_y;

/* Margins */
extern int conf_msgbox_margin_left;
extern int conf_msgbox_margin_top;
extern int conf_msgbox_margin_right;
extern int conf_msgbox_margin_bottom;
extern int conf_msgbox_margin_line;
extern int conf_msgbox_margin_char;

/* Font */
extern int conf_msgbox_font_select;
extern int conf_msgbox_font_size;
extern int conf_msgbox_font_r;
extern int conf_msgbox_font_g;
extern int conf_msgbox_font_b;
extern int conf_msgbox_font_outline_width;
extern int conf_msgbox_font_outline_r;
extern int conf_msgbox_font_outline_g;
extern int conf_msgbox_font_outline_b;
extern int conf_msgbox_font_ruby;
extern bool conf_msgbox_font_tategaki;

/* Fill */
extern bool conf_msgbox_fill_enable;
extern int conf_msgbox_fill_r;
extern int conf_msgbox_fill_g;
extern int conf_msgbox_fill_b;

/* Dimming */
extern bool conf_msgbox_dim_enable;
extern int conf_msgbox_dim_r;
extern int conf_msgbox_dim_g;
extern int conf_msgbox_dim_b;
extern int conf_msgbox_dim_outline_width;
extern int conf_msgbox_dim_outline_r;
extern int conf_msgbox_dim_outline_g;
extern int conf_msgbox_dim_outline_b;

/* Seen coloring */
extern bool conf_msgbox_seen_enable;
extern int conf_msgbox_seen_r;
extern int conf_msgbox_seen_g;
extern int conf_msgbox_seen_b;
extern int conf_msgbox_seen_outline_width;
extern int conf_msgbox_seen_outline_r;
extern int conf_msgbox_seen_outline_g;
extern int conf_msgbox_seen_outline_b;

/* Misc. */
extern bool conf_msgbox_skip_unseen;

/*
 * Name Box Settings
 */

/* Enable. */
extern bool conf_namebox_enable;

/* Image file. */
extern char *conf_namebox_image;

/* Anime. */
extern char *conf_namebox_anime_hide;
extern char *conf_namebox_anime_show;

/* Position. */
extern int conf_namebox_x;
extern int conf_namebox_y;

/* Margins. */
extern int conf_namebox_margin_top;
extern int conf_namebox_margin_left;
extern bool conf_namebox_centering;

/* Font */
extern int conf_namebox_font_select;
extern int conf_namebox_font_size;
extern int conf_namebox_font_r;
extern int conf_namebox_font_g;
extern int conf_namebox_font_b;
extern int conf_namebox_font_outline_width;
extern int conf_namebox_font_outline_r;
extern int conf_namebox_font_outline_g;
extern int conf_namebox_font_outline_b;
extern int conf_namebox_font_ruby;
extern bool conf_namebox_font_tategaki;

/*
 * Click Animation
 */

/* Position. */
extern int conf_click_x;
extern int conf_click_y;

/* Interval. */
extern float conf_click_interval;

/* Image files. */
extern char *conf_click_image[S3_CLICK_FRAMES];

/* Misc. */
extern bool conf_click_move;

/*
 * Choose Box Settings
 */

/* Font */
extern int conf_choose_font_select;
extern int conf_choose_font_size;
extern int conf_choose_font_idle_r;
extern int conf_choose_font_idle_g;
extern int conf_choose_font_idle_b;
extern int conf_choose_font_idle_outline_width;
extern int conf_choose_font_idle_outline_r;
extern int conf_choose_font_idle_outline_g;
extern int conf_choose_font_idle_outline_b;
extern int conf_choose_font_hover_r;
extern int conf_choose_font_hover_g;
extern int conf_choose_font_hover_b;
extern int conf_choose_font_hover_outline_width;
extern int conf_choose_font_hover_outline_r;
extern int conf_choose_font_hover_outline_g;
extern int conf_choose_font_hover_outline_b;
extern int conf_choose_font_ruby;
extern bool conf_choose_font_tategaki;

/* SE */
extern char *conf_choose_change_se;
extern char *conf_choose_click_se;

/* File names */
extern char *conf_choose_idle[S3_CHOOSEBOX_COUNT];
extern char *conf_choose_hover[S3_CHOOSEBOX_COUNT];

/* Positions */
extern int conf_choose_x[S3_CHOOSEBOX_COUNT];
extern int conf_choose_y[S3_CHOOSEBOX_COUNT];

/* Margin */
extern int conf_choose_margin_top[S3_CHOOSEBOX_COUNT];

/* Anime */
extern char *conf_choose_idle_anime[S3_CHOOSEBOX_COUNT];
extern char *conf_choose_hover_anime[S3_CHOOSEBOX_COUNT];

/*
 * Save Data Settings
 */

/* Thumbnail size */
extern int conf_save_thumb_width;
extern int conf_save_thumb_height;

/* NEW image file name */
extern char *conf_save_new_image;

/* 
 * System Button Settings
 */

/* Enable */
extern bool conf_sysbtn_enable;

/* File names */
extern char *conf_sysbtn_idle;
extern char *conf_sysbtn_hover;

/* Anime */
extern char *conf_sysbtn_anime_out;
extern char *conf_sysbtn_anime_fadein;
extern char *conf_sysbtn_anime_appear;
extern char *conf_sysbtn_anime_hover;
extern char *conf_sysbtn_anime_fadeout;

/* Position */
extern int conf_sysbtn_x;
extern int conf_sysbtn_y;
extern int conf_sysbtn_width;
extern int conf_sysbtn_height;

/* SE */
extern char *conf_sysbtn_enter_se;
extern char *conf_sysbtn_leave_se;
extern char *conf_sysbtn_click_se;

/*
 * Auto Mode Settings
 */

/* File name */
extern char *conf_automode_image;

/* Animation */
extern char *conf_automode_anime_hide;
extern char *conf_automode_anime_show;

/* Position */
extern int conf_automode_x;
extern int conf_automode_y;

/* SE */
extern char *conf_automode_enter_se;
extern char *conf_automode_leave_se;

/*
 * Skip Mode Settings
 */

/* File name */
extern char *conf_skipmode_image;

/* Animation */
extern char *conf_skipmode_anime_hide;
extern char *conf_skipmode_anime_show;

/* Position */
extern int conf_skipmode_x;
extern int conf_skipmode_y;

/* SE */
extern char *conf_skipmode_enter_se;
extern char *conf_skipmode_leave_se;

/*
 * GUI Settings
 */

/* Save index font */
extern int conf_gui_save_index_font_select;
extern int conf_gui_save_index_font_size;
extern int conf_gui_save_index_font_r;
extern int conf_gui_save_index_font_g;
extern int conf_gui_save_index_font_b;
extern int conf_gui_save_index_font_outline_width;
extern int conf_gui_save_index_font_outline_r;
extern int conf_gui_save_index_font_outline_g;
extern int conf_gui_save_index_font_outline_b;
extern int conf_gui_save_index_font_ruby;
extern bool conf_gui_save_index_font_tategaki;
extern int conf_gui_save_index_margin_char;

/* Save date font */
extern int conf_gui_save_date_font_select;
extern int conf_gui_save_date_font_size;
extern int conf_gui_save_date_font_r;
extern int conf_gui_save_date_font_g;
extern int conf_gui_save_date_font_b;
extern int conf_gui_save_date_font_outline_width;
extern int conf_gui_save_date_font_outline_r;
extern int conf_gui_save_date_font_outline_g;
extern int conf_gui_save_date_font_outline_b;
extern int conf_gui_save_date_font_ruby;
extern bool conf_gui_save_date_font_tategaki;
extern int conf_gui_save_date_margin_char;

/* Save chapter font */
extern int conf_gui_save_chapter_font_select;
extern int conf_gui_save_chapter_font_size;
extern int conf_gui_save_chapter_font_r;
extern int conf_gui_save_chapter_font_g;
extern int conf_gui_save_chapter_font_b;
extern int conf_gui_save_chapter_font_outline_width;
extern int conf_gui_save_chapter_font_outline_r;
extern int conf_gui_save_chapter_font_outline_g;
extern int conf_gui_save_chapter_font_outline_b;
extern int conf_gui_save_chapter_font_ruby;
extern bool conf_gui_save_chapter_font_tategaki;
extern int conf_gui_save_chapter_margin_char;

/* Save message font */
extern int conf_gui_save_msg_font_select;
extern int conf_gui_save_msg_font_size;
extern int conf_gui_save_msg_font_r;
extern int conf_gui_save_msg_font_g;
extern int conf_gui_save_msg_font_b;
extern int conf_gui_save_msg_font_outline_width;
extern int conf_gui_save_msg_font_outline_r;
extern int conf_gui_save_msg_font_outline_g;
extern int conf_gui_save_msg_font_outline_b;
extern int conf_gui_save_msg_font_ruby;
extern bool conf_gui_save_msg_font_tategaki;
extern int conf_gui_save_msg_margin_line;
extern int conf_gui_save_msg_margin_char;
extern bool conf_gui_save_msg_multiline;

/* History Name Font */
extern int conf_gui_history_name_font_select;
extern int conf_gui_history_name_font_size;
extern int conf_gui_history_name_font_r;
extern int conf_gui_history_name_font_g;
extern int conf_gui_history_name_font_b;
extern int conf_gui_history_name_font_outline_width;
extern int conf_gui_history_name_font_outline_r;
extern int conf_gui_history_name_font_outline_g;
extern int conf_gui_history_name_font_outline_b;
extern int conf_gui_history_name_font_ruby;
extern bool conf_gui_history_name_font_tategaki;
extern int conf_gui_history_name_margin_line;
extern int conf_gui_history_name_margin_char;

/* History Text Font */
extern int conf_gui_history_text_font_select;
extern int conf_gui_history_text_font_size;
extern int conf_gui_history_text_font_r;
extern int conf_gui_history_text_font_g;
extern int conf_gui_history_text_font_b;
extern int conf_gui_history_text_font_outline_width;
extern int conf_gui_history_text_font_outline_r;
extern int conf_gui_history_text_font_outline_g;
extern int conf_gui_history_text_font_outline_b;
extern int conf_gui_history_text_font_ruby;
extern bool conf_gui_history_text_font_tategaki;
extern int conf_gui_history_text_margin_line;
extern int conf_gui_history_text_margin_char;

/* Misc. */
extern char *conf_gui_history_quote_name_separator;
extern char *conf_gui_history_quote_start;
extern char *conf_gui_history_quote_end;
extern bool conf_gui_history_hide_last;

/* Preview */
extern int conf_gui_preview_font_select;
extern int conf_gui_preview_font_size;
extern int conf_gui_preview_font_r;
extern int conf_gui_preview_font_g;
extern int conf_gui_preview_font_b;
extern int conf_gui_preview_font_outline_width;
extern int conf_gui_preview_font_outline_r;
extern int conf_gui_preview_font_outline_g;
extern int conf_gui_preview_font_outline_b;
extern int conf_gui_preview_font_ruby;
extern bool conf_gui_preview_font_tategaki;

/*
 * Sound Settings
 */

/* Default */
extern float conf_sound_vol_bgm;
extern float conf_sound_vol_voice;
extern float conf_sound_vol_se;
extern float conf_sound_vol_per_character;

/*
 * Character Settings
 */

/* Character name and folder mapping */
extern char *conf_character_name[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_en[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_fr[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_es[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_de[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_it[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_ru[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_el[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_zh_cn[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_zh_tw[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_name_ja[S3_CHARACTER_MAP_COUNT];
extern char *conf_character_folder[S3_CHARACTER_MAP_COUNT];

/* Eye-blink */
extern float conf_character_eyeblink_interval;
extern float conf_character_eyeblink_frame;

/* Lip-sync */
extern float conf_character_lipsync_frame;
extern int conf_character_lipsync_chars;

/*
 * Auto Focus
 */

extern bool conf_autofocus_on_text_name;
extern bool conf_autofocus_on_text_no_name;
extern bool conf_autofocus_on_ch;
extern bool conf_autofocus_on_choose;

/*
 * Stage Settings
 */

/* Margins */
extern int conf_stage_ch_margin_bottom;
extern int conf_stage_ch_margin_left;
extern int conf_stage_ch_margin_right;

/*
 * Kirakira Effect
 */

extern bool conf_kirakira_enable;
extern bool conf_kirakira_add_blend;
extern float conf_kirakira_frame;
extern char *conf_kirakira_image[S3_KIRAKIRA_FRAMES];

/*
 * Emoji
 */

extern char *conf_emoji_name[S3_EMOJI_COUNT];
extern char *conf_emoji_image[S3_EMOJI_COUNT];

/*
 * Text-to-speech
 */

extern bool conf_tts_enable;

/*
 * Misc.
 */

/* Is release app? */
extern bool conf_release_mode_enable;

/*
 * Private Usage
 */

/* Screen width */
extern int conf_game_width;

/* Screen height */
extern int conf_game_height;

/* Click animation frames */
extern int conf_click_frames;

/* Last saved/loaded page */
extern int conf_gui_save_last_page;

/*
 * Functions
 */

/*
 * Initialize the config subsystem.
 */
bool
s3i_init_conf(void);

/*
 * Cleanup the config subsystem.
 */
void
s3i_cleanup_conf(void);

/*
 * Initialize various settings by config values.
 */
bool
s3i_apply_initial_values(void);

/*
 * Get the major locale. (e.g. returns "en" for "en-us")
 */
const char *
s3i_get_major_locale(void);

#endif
