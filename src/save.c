/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Save Subsystem
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
#include "save.h"
#include "conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#define FREE(x) \
	do { \
		if (x != NULL) { \
			free(x); \
			x = NULL; \
		} \
	} while (0)

#define STRDUP(x, y) \
	do { \
		x = strdup(y); \
		if (x == NULL) { \
			s3_log_out_of_memory(); \
			return false;\
		} \
	} while (0)

/* Save data compatibility version. */
#define SAVE_VER	(0x00000001)

/* False assertion */
#define CONFIG_TYPE_ERROR	(0)

/* Terminator of the config. */
#define END_OF_CONFIG		"eoc"

/* Flag to show if right after load. */
static bool is_right_after_load;

/*
 * Save data that are loaded onto the memory.
 */

/* Timestamp of the save data. (0 for no data) */
static uint64_t save_time[S3_ALL_SAVE_SLOTS];

/* Index of the last saved. */
static int latest_index;

/* Chapter title of the save data. */
static char *save_title[S3_ALL_SAVE_SLOTS];

/* Last message of the save data. */
static char *save_message[S3_ALL_SAVE_SLOTS];

/* Thumbnail of the save data. */
static struct s3_image *save_thumb[S3_ALL_SAVE_SLOTS];

/*
 * Temporary Buffers
 */

/* Buffer for the buffered stream. */
static unsigned char *stream_buf;
static size_t stream_buf_alloc_size;
static size_t stream_buf_pos;
static char sbuf[1024];

/*
 * Forward declaration.
 */
static bool load_basic_save_info_all(void);
static bool load_basic_save_info(int index);
static bool copy_thumb(int index);
static bool open_write_stream(void);
static bool write_u32(uint32_t val);
static bool write_u64(uint64_t val);
static bool write_f32(float val);
static bool write_string(const char *val);
static bool write_data(const void *data, size_t size);
static bool resize_buffer(size_t inc_size);
static bool close_write_stream(const char *file, ...);
static bool open_read_stream(const char *file, ...);
static bool read_u32(uint32_t *ret);
static bool read_u64(uint64_t *ret);
static bool read_f32(float *ret);
static bool read_string(char *val, size_t size);
static bool read_data(void *data, size_t size);
static bool read_skip(size_t size);
static bool close_read_stream(void);


/*
 * Initialization
 */

/*
 * Initialize the save subsystem.
 */
bool
s3i_init_save(void)
{
	int i;

	/* Clear the save slots. */
	for (i = 0; i < S3_ALL_SAVE_SLOTS; i++) {
		save_time[i] = 0;
		FREE(save_title[i]);
		FREE(save_message[i]);
		if (save_thumb[i] != NULL) {
			s3_destroy_image(save_thumb[i]);
			save_thumb[i] = NULL;
		}
	}

	/* Load the basic data from the local save files. */
	load_basic_save_info_all();

	/* Load the global save file. */
	s3_execute_load_global();

	return true;
}

/*
 * Cleanup the save subsystem.
 */
void
s3i_cleanup_save(void)
{
	int i;

	for (i = 0; i < S3_ALL_SAVE_SLOTS; i++) {
		FREE(save_title[i]);
		FREE(save_message[i]);
		if (save_thumb[i] != NULL) {
			s3_destroy_image(save_thumb[i]);
			save_thumb[i] = NULL;
		}
	}

	s3_execute_save_global();
}

/*
 * Execute a global save.
 */
bool
s3_execute_save_global(void)
{
	int i;
	int count;
	bool success;

	success = false;
	do {
		/* Open the stream. */
		if (!open_write_stream())
			break;

		/* Write the save data versoin. */
		if (!write_u32(SAVE_VER))
			break;
		
		/* Write the global variables. */
		count = s3_get_variable_count();
		if (!write_u32((uint32_t)count))
			break;
		for (i = 0; i < count; i++) {
			const char *name = s3_get_variable_name(i);
			if (s3_is_global_variable(name)) {
				if (!write_string(name))
					break;
				if (!write_string(s3_get_variable_string(name)))
					break;
			} else {
				if (!write_string("#l"))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* Write the master volume. */
		if (!write_f32(s3_get_master_volume()))
		    break;

		/* Write the global volumes. */
		for (i = 0; i < S3_MIXER_TRACKS; i++) {
			if (!write_f32(s3_get_mixer_global_volume(i)))
				break;
		}
		if (i == 0)
			break;	/* Error. */

		/* Write the character volumes. */
		for (i = 0; i < S3_CH_VOL_SLOTS; i++) {
			if (!write_f32(s3_get_character_volume(i)))
				break;
		}
		if (i == 0)
			break;	/* Error. */

		/* Write the text speed. */
		if (!write_f32(s3_get_text_speed()))
			break;

		/* Write the auto speed. */
		if (!write_f32(s3_get_auto_speed()))
			break;

		/* Write the global config. */
		count = s3_get_config_count();
		if (!write_u32((uint32_t)count))
			break;
		for (i = 0; i < count; i++) {
			const char *key = s3_get_config_key(i);
			if (s3_is_global_save_config(key)) {
				const char *val = s3_get_config_as_string(key);
				if (!write_string(val))
					break;
			}
		}

		/* Close the stream. */
		if (!close_write_stream(GLOBAL_SAVE_FILE))
			break;

		success = true;
	} while (0);

	if (!success) {
		close_write_stream(GLOBAL_SAVE_FILE);
		return false;
	}

	return true;
}

/* Execute a global load. */
bool
s3_execute_load_global(void)
{
	uint32_t ver;
	float f;
	int i;
	int count;
	bool success;
	char key[128];

	success = false;
	do {
		/* Open the stream. */
		if (!open_read_stream(GLOBAL_SAVE_FILE))
			break;

		/* Read the save format version. */
		if (!read_u32(&ver))
			break;
		if (ver != SAVE_VER) {
			s3_log_error(S3_TR("Save data version mismatched."));
			break;
		}
		
		/* Read the global variables. */
		if (!read_u32((uint32_t *)&count))
			break;
		for (i = 0; i < count; i++) {
			if (!read_string((char *)&key[0], sizeof(key)))
				break;
			if (strcmp(key, "#l") != 0) {
				if (!read_string(sbuf, sizeof(sbuf)))
					break;
				if (!s3_set_variable_string(key, sbuf))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* Read the master volume. */
		if (!read_f32(&f))
		    break;
		s3_set_master_volume(f);

		/* Write the global volumes. */
		for (i = 0; i < S3_MIXER_TRACKS; i++) {
			if (!read_f32(&f))
				break;
			s3_set_mixer_global_volume(i, f);
		}
		if (i == 0)
			break;	/* Error. */

		/* Read the character volumes. */
		for (i = 0; i < S3_CH_VOL_SLOTS; i++) {
			if (!read_f32(&f))
				break;
			s3_set_character_volume(i, f);
		}
		if (i == 0)
			break;	/* Error. */

		/* Read the text speed. */
		if (!read_f32(&f))
			break;
		s3_set_text_speed(f);

		/* Read the auto speed. */
		if (!read_f32(&f))
			break;
		s3_set_auto_speed(f);

		/* Read the global config. */
		if (!read_u32((uint32_t *)&count))
			break;
		for (i = 0; i < count; i++) {
			if (!read_string(key, sizeof(key)))
				break;
			if (strcmp(key, "#l") != 0) {
				if (!read_string(sbuf, sizeof(sbuf)))
					break;
				if (!s3_set_config(key, sbuf))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* Close the stream. */
		if (!close_read_stream())
			break;

		success = true;
	} while (0);

	if (!success)
		return false;

	return true;
}

/*
 * Execute a local save.
 */
bool
s3_execute_save_local(
	int index)
{
	uint64_t timestamp;
	uint32_t ver;
	int i;
	int count;
	bool success;

	success = false;
	do {
		/* Save the seen flags for the current tag file. */
		s3_save_seen();

		/* Save the global save data. */
		s3_execute_save_global();

		/* Open a buffer for streaming. */
		if (!open_write_stream())
			return false;

		/* Write the save format version. */
		ver = (uint32_t)SAVE_VER;
		if (!write_u32(ver))
			break;

		/* Write the timestamp. */
		timestamp = (uint64_t)time(NULL);
		if (!write_u64(timestamp))
			break;

		/* Write the chapter name. */
		if (!write_string(s3_get_chapter_name()))
			break;

		/* Write the last message. */
		if (!write_string(s3_get_last_message()))
			break;

		/* Write the previous last message. */
		if (!write_string(s3_get_prev_last_message()))
			break;

		/* Write the thumbnail. */
		if (!copy_thumb(index))
			break;
		if (!write_data(s3_get_image_pixels(save_thumb[index]),
				(size_t)(conf_save_thumb_width * conf_save_thumb_height * 4)))
			break;

		/* Write the tag file name. */
		if (!write_string(s3_get_tag_file()))
			break;

		/* Write the tag index. */
		if (!write_u32((uint32_t)s3_get_tag_index()))
			break;
		if (!write_u32((uint32_t)s3_get_last_english_tag_index()))
			break;

		/* Write the gosub return index. */
		for (i = 0; i < S3_CALL_STACK_MAX; i++) {
			char *file;
			int index;
			s3_read_call_stack(i, &file, &index);
			if (!write_string(file))
				break;
			if (!write_u32((uint32_t)index))
				break;
			free(file);
		}

		/* Write the stage. */
		for (i = 0; i < S3_STAGE_LAYERS; i++) {
			if (i == S3_LAYER_BG    || i == S3_LAYER_BG2 ||
			    i == S3_LAYER_EFB1  || i == S3_LAYER_EFB2 ||
			    i == S3_LAYER_EFB3  || i == S3_LAYER_EFB4 ||
			    i == S3_LAYER_CHB   || i == S3_LAYER_CHL ||
			    i == S3_LAYER_CHLC  || i == S3_LAYER_CHR ||
			    i == S3_LAYER_CHRC  ||i == S3_LAYER_CHC ||
			    i == S3_LAYER_EFF1  ||i == S3_LAYER_EFF2 ||
			    i == S3_LAYER_EFF3  ||i == S3_LAYER_EFF4 ||
			    i == S3_LAYER_CHF   ||i == S3_LAYER_TEXT1 ||
			    i == S3_LAYER_TEXT2 ||i == S3_LAYER_TEXT3 ||
			    i == S3_LAYER_TEXT4) {
				if (!write_string(s3_get_layer_file_name(i)))
					break;
				if (!write_u32((uint32_t)s3_get_layer_x(i)))
					break;
				if (!write_u32((uint32_t)s3_get_layer_y(i)))
					break;
				if (!write_u32((uint32_t)s3_get_layer_alpha(i)))
					break;
				if (!write_f32(s3_get_layer_scale_x(i)))
					break;
				if (!write_f32(s3_get_layer_scale_y(i)))
					break;
				if (!write_u32((uint32_t)s3_get_layer_center_x(i)))
					break;
				if (!write_u32((uint32_t)s3_get_layer_center_y(i)))
					break;
				if (!write_f32((uint32_t)s3_get_layer_rotate(i)))
					break;
				if (i >= S3_LAYER_TEXT1 && i <= S3_LAYER_TEXT8) {
					if (!write_string(s3_get_layer_text(i)))
						break;
				}
			}
		}
		if (i != S3_STAGE_LAYERS)
			break; /* Error. */

		/* Serialize the anime. */
		for (i = 0; i < S3_REG_ANIME_COUNT; i++) {
			if (!write_string(s3_get_reg_anime_name(i)))
				break;
			if (!write_string(s3_get_reg_anime_file_name(i)))
				break;
		}
		if (i != S3_REG_ANIME_COUNT)
			break;	/* Error. */

		/* Write the sound tracks. */
		for (i = 0; i < S3_MIXER_TRACKS; i++) {
			if (!write_f32(s3_get_mixer_volume(i)))
				break;
			if (!write_string(s3_get_track_file_name(i)))
				break;
		}

		/* Write the variables. */
		count = s3_get_variable_count();
		if (!write_u32((uint32_t)count))
			break;
		for (i = 0; i < count; i++) {
			const char *name = s3_get_variable_name(i);
			if (!s3_is_global_variable(name)) {
				if (!write_string(name))
					break;
				if (!write_string(s3_get_variable_string(name)))
					break;
			} else {
				if (!write_string("#g"))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* TODO: Serialize the temporary stage of Ciel. */

		/* Write the non-global config. */
		count = s3_get_config_count();
		if (!write_u32((uint32_t)count))
			break;
		for (i = 0; i < count; i++) {
			const char *key = s3_get_config_key(i);
			if (s3_is_local_save_config(key)) {
				if (!write_string(s3_get_config_as_string(key)))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* Close the stream. */
		if (!close_write_stream("%03d.sav", index)) {
			s3_log_error(S3_TR("Failed to write save data."));
			return false;
		}

		/* Store the timestamp. */
		save_time[index] = timestamp;

		/* Update the latest index. */
		latest_index = index;

		/* Update the basic information. */
		load_basic_save_info(index);

		/* Succeeded. */
		success = true;
	} while (0);

	if (!success) {
		s3_log_error(S3_TR("Failed to write save data."));
		close_write_stream(NULL);
		return false;
	}

	return true;
}

/*
 * Execute a local load.
 */
bool
s3_execute_load_local(
	int index)
{
	char key[128];
	uint64_t timestamp;
	uint32_t ver, u;
	uint32_t i;
	uint32_t x, y, count;
	float f, sx, sy, rot;
	struct s3_image *img;
	bool success;

	success = false;
	do {
		/* Save the seen flags for the current tag file. */
		s3_save_seen();

		/* Save the global save data. */
		s3_execute_save_global();

		/* Stop the anime. */
		s3_clear_all_anime_sequence();

		/* Clear the stage. */
		s3_clear_stage();

		/* Clear the history. */
		s3_clear_history();

		/* Clear the message status. */
		if (s3_is_message_active())
			s3_clear_message_active();

		/* Open a buffer for streaming. */
		if (!open_read_stream("%03d.sav", index))
			break;

		/* Read the save format version. */
		if (!read_u32(&ver))
			break;
		if (ver != SAVE_VER) {
			s3_log_error(S3_TR("Save data version mismatched."));
			break;
		}

		/* Skip the timestamp. */
		if (!read_u64(&timestamp))
			break;

		/* Read the chapter title. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		if (!s3_set_chapter_name(sbuf))
			break;

		/* Read the last message. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		if (!s3_set_last_message(sbuf, false))
			break;

		/* Read the previous last message. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		if (!s3_set_last_message(sbuf, false))
			break;

		/* Skip the thumbnail. */
		if (!read_skip((size_t)(conf_save_thumb_width * conf_save_thumb_height * 4)))
			break;

		/* Read the tag file name. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		if (!s3_move_to_tag_file(sbuf))
			break;

		/* Read the tag index. */
		if (!read_u32(&u))
			break;
		if (!s3_move_to_tag_index((int)u))
			break;

		/* Read the gosub return index. */
		for (i = 0; i < S3_CALL_STACK_MAX; i++) {
			if (!read_string(sbuf, sizeof(sbuf)))
				break;
			if (!read_u32(&u))
				break;
			if (!s3_write_call_stack((int)i, sbuf, (int)u))
				break;
		}

		/* Read the stage. */
		for (i = 0; i < S3_STAGE_LAYERS; i++) {
			if (i == S3_LAYER_BG    || i == S3_LAYER_BG2 ||
			    i == S3_LAYER_EFB1  || i == S3_LAYER_EFB2 ||
			    i == S3_LAYER_EFB3  || i == S3_LAYER_EFB4 ||
			    i == S3_LAYER_CHB   || i == S3_LAYER_CHL ||
			    i == S3_LAYER_CHLC  || i == S3_LAYER_CHR ||
			    i == S3_LAYER_CHRC  ||i == S3_LAYER_CHC ||
			    i == S3_LAYER_EFF1  ||i == S3_LAYER_EFF2 ||
			    i == S3_LAYER_EFF3  ||i == S3_LAYER_EFF4 ||
			    i == S3_LAYER_CHF   ||i == S3_LAYER_TEXT1 ||
			    i == S3_LAYER_TEXT2 ||i == S3_LAYER_TEXT3 ||
			    i == S3_LAYER_TEXT4) {
				/* Read the file name. */
				if (!read_string(sbuf, sizeof(sbuf)))
					break;
				if (strcmp(sbuf, "") != 0) {
					img = s3_create_image_from_file(sbuf);
					if (img == NULL)
						break;
					if (!s3_set_layer_file_name((int)i, sbuf))
						break;
					s3_set_layer_image((int)i, img);
				}

				/* Read the position. */
				if (!read_u32(&x))
					break;
				if (!read_u32(&y))
					break;
				s3_set_layer_position((int)i, (int)x, (int)y);

				/* Read the alpha. */
				if (!read_u32(&u))
					break;
				s3_set_layer_alpha((int)i, (int)u);

				/* Read the scale. */
				if (!read_f32(&sx))
					break;
				if (!read_f32(&sy))
					break;
				s3_set_layer_scale((int)i, sx, sy);

				/* Read the center. */
				if (!read_u32(&x))
					break;
				if (!read_u32(&y))
					break;
				s3_set_layer_center((int)i, (int)x, (int)y);

				/* Read the center. */
				if (!read_f32(&rot))
					break;
				s3_set_layer_rotate((int)i, rot);

				/* Read the text. */
				if (i >= S3_LAYER_TEXT1 && i <= S3_LAYER_TEXT8) {
					if (!read_string(sbuf, sizeof(sbuf)))
						break;
					if (!s3_set_layer_text((int)i, sbuf))
						break;
				}
			}

		}
		if (i != S3_STAGE_LAYERS)
			break; /* Error. */

		/* Start the eyes and lips animes. */
		for (i = 0; i < S3_CH_ALL_LAYERS; i++) {
			int layer = s3_chpos_to_layer((int)i);
			const char *fname = s3_get_layer_file_name(layer);
			if (!s3_load_eye_image_if_exists((int)i, fname))
				break;
			if (!s3_load_lip_image_if_exists((int)i, fname))
				break;
		}
		if (i != S3_CH_ALL_LAYERS)
			break; /* Error. */

		/* Read the anime. */
		for (i = 0; i < S3_REG_ANIME_COUNT; i++) {
			if (!read_string(key, sizeof(key)))
				break;
			if (!read_string(sbuf, sizeof(sbuf)))
				break;
			if (strcmp(key, "") != 0) {
				if (!s3_load_anime_from_file(sbuf, key, NULL))
					break;
			}
		}
		if (i != S3_REG_ANIME_COUNT)
			break;

		/* Read the sound tracks. */
		for (i = 0; i < S3_MIXER_TRACKS; i++) {
			if (!read_f32(&f))
				break;
			s3_set_mixer_volume((int)i, f, 0);

			if (!read_string(sbuf, sizeof(sbuf)))
				break;
			s3_set_mixer_input_file((int)i, sbuf, true);
		}
		if (i != S3_MIXER_TRACKS)
			break;	/* Error. */

		/* Read the variables. */
		if (!read_u32(&count))
			break;
		for (i = 0; i < count; i++) {
			if (!read_string(key, sizeof(key)))
				break;
			if (strcmp(key, "#g") != 0) {
				if (!read_string(sbuf, sizeof(sbuf)))
					break;
				if (!s3_set_variable_string(key, sbuf))
					break;
			}
		}
		if (i != count)
			break;	/* Error. */

		/* TODO: Ciel temporary stage. */

		/* Read the config. */
		if (!read_u32(&count))
			break;
		for (i = 0; i < count; i++) {
			if (!read_string(key, sizeof(key)))
				break;
			if (!read_string(sbuf, sizeof(sbuf)))
				break;
			if (!s3_set_config(key, sbuf))
				break;
		}
		if (i != count)
			break;	/* Error. */

		/* Close the read stream. */
		close_read_stream();

		/* Reload the stage. */
		if (!s3_reload_stage_images())
			return false;

		/* Hide the name box, message box, and choose boxes. */
		s3_show_namebox(false);
		s3_show_msgbox(false);
		s3_show_choosebox(-1, false, false);

		/* Set the flag. */
		is_right_after_load = true;

		/* Succeeded. */
		success = true;
	} while (0);

	if (!success) {
		close_read_stream();
		return false;
	}

	return true;
}

/*
 * Check if the save data exist.
 */
bool
s3_check_save_exists(
	int index)
{
	assert(index >= 0);
	if (index >= S3_ALL_SAVE_SLOTS)
		return false;

	if (save_time[index] == 0)
		return false;

	return true;
}

/*
 * Delete a save data.
 */
void
s3_delete_local_save(
	int index)
{
	UNUSED_PARAMETER(index);

	/* TODO */
}

/*
 * Delete the global save data.
 */
void
s3_delete_global_save(void)
{
	/* TODO */
}

/*
 * Check if right after load.
 */
bool
s3_check_right_after_load(void)
{
	if (is_right_after_load) {
		is_right_after_load = false;
		return true;
	}
	return false;
}

/*
 * Get the timestamp of a save data.
 */
uint64_t
s3_get_save_timestamp(
	int index)
{
	assert(index >= 0);
	if (index >= S3_ALL_SAVE_SLOTS)
		return 0;

	return save_time[index];
}

/*
 * Get the latest save index.
 */
int
s3_get_latest_save_index(void)
{
	return latest_index;
}

/*
 * Get the chapter title of a save data.
 */
const char *
s3_get_save_chapter_name(
	int index)
{
	assert(index >= 0);
	if (index >= S3_ALL_SAVE_SLOTS)
		return 0;

	return save_title[index];
}

/*
 * Get the last message of a save data.
 */
const char *
s3_get_save_last_message(
	int index)
{
	assert(index >= 0);
	if (index >= S3_ALL_SAVE_SLOTS)
		return 0;

	return save_message[index];
}

/*
 * Get the thumbnail of a save data.
 */
struct s3_image *
s3_get_save_thumbnail(
	int index)
{
	assert(index >= 0);
	if (index >= S3_ALL_SAVE_SLOTS)
		return NULL;

	return save_thumb[index];
}

/* Load the basic information from all local save files. */
static bool
load_basic_save_info_all(void)
{
	int i;

	latest_index = 0;

	/* For each save slot. */
	for (i = 0; i < S3_ALL_SAVE_SLOTS; i++) {
		if (!load_basic_save_info(i))
			return false;
	}

	return true;
}

/* Load the basic information from all local save files. */
static bool
load_basic_save_info(
	int index)
{
	uint32_t ver;
	bool success;

	success = false;
	do {
		/* Open a buffer for streaming. */
		if (!open_read_stream("%03d.sav", index))
			continue;

		/* Read the save format version. */
		if (!read_u32(&ver))
			continue;
		if (ver != SAVE_VER)
			continue;

		/* Skip the timestamp. */
		if (!read_u64(&save_time[index]))
			break;
		if (save_time[index] > save_time[latest_index])
			latest_index = index;

		/* Read the chapter title. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		save_title[index] = strdup(sbuf);
		if (save_title[index] == NULL) {
			s3_log_out_of_memory();
			break;
		}

		/* Skip the last message. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;
		save_message[index] = strdup(sbuf);
		if (save_message[index] == NULL) {
			s3_log_out_of_memory();
			break;
		}

		/* Skip the previous last message. */
		if (!read_string(sbuf, sizeof(sbuf)))
			break;

		/* Read the thumbnail. */
		if (!read_data(s3_get_image_pixels(save_thumb[index]),
			       (size_t)(conf_save_thumb_width * conf_save_thumb_height * 4)))
			break;
		s3_notify_image_update(save_thumb[index]);

		/* Close the stream. */
		close_read_stream();

		success = true;
	} while (0);

	if (!success) {
		close_read_stream();
		return false;
	}

	return true;
}

static bool
copy_thumb(
	int index)
{
	struct s3_image *src, *dst;
	int w, h;

	src = s3_get_thumb_image();
	if (src == NULL)
		return false;

	w = s3_get_image_width(src);
	h = s3_get_image_height(src);

	dst = s3_create_image(w, h);
	if (dst == NULL)
		return false;

	memcpy(s3_get_image_pixels(dst),
	       s3_get_image_pixels(src),
	       (size_t)(w * 4 * h));

	if (save_thumb[index] != NULL) {
		s3_destroy_image(save_thumb[index]);
		save_thumb[index] = NULL;
	}
	save_thumb[index] = dst;
	return true;
}


/*
 * Helpers
 */

/* Open the write stream. */
static bool
open_write_stream(void)
{
	const uint32_t START_SIZE = 1 * 1024 * 1024; /* 1 MB */

	assert(stream_buf == NULL);
	if (stream_buf != NULL)
		return false;

	stream_buf = malloc(START_SIZE);
	if (stream_buf == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	stream_buf_alloc_size = START_SIZE;
	stream_buf_pos = 0;

	return true;
}

/* Write u32 to the stream. */
static bool
write_u32(
	uint32_t val)
{
	if (!resize_buffer(4))
		return false;

	stream_buf[stream_buf_pos + 0] = (uint8_t)(val & 0xff);
	stream_buf[stream_buf_pos + 1] = (uint8_t)((val >> 8) & 0xff);
	stream_buf[stream_buf_pos + 2] = (uint8_t)((val >> 16) & 0xff);
	stream_buf[stream_buf_pos + 3] = (uint8_t)((val >> 24) & 0xff);

	stream_buf_pos += 4;

	return true;
}

/* Write u64 to the stream. */
static bool
write_u64(
	  uint64_t val)
{
	if (!resize_buffer(8))
		return false;

	stream_buf[stream_buf_pos + 0] = (uint8_t)(val & 0xff);
	stream_buf[stream_buf_pos + 1] = (uint8_t)((val >> 8) & 0xff);
	stream_buf[stream_buf_pos + 2] = (uint8_t)((val >> 16) & 0xff);
	stream_buf[stream_buf_pos + 3] = (uint8_t)((val >> 24) & 0xff);
	stream_buf[stream_buf_pos + 4] = (uint8_t)((val >> 32) & 0xff);
	stream_buf[stream_buf_pos + 5] = (uint8_t)((val >> 40) & 0xff);
	stream_buf[stream_buf_pos + 6] = (uint8_t)((val >> 48) & 0xff);
	stream_buf[stream_buf_pos + 7] = (uint8_t)((val >> 56) & 0xff);

	stream_buf_pos += 8;

	return true;
}

/* Write foat to the stream. */
static bool
write_f32(
	  float val)
{
	uint32_t v;

	if (!resize_buffer(4))
		return false;

	v = *(uint32_t *)&val;
	stream_buf[stream_buf_pos + 0] = (uint8_t)(v & 0xff);
	stream_buf[stream_buf_pos + 1] = (uint8_t)((v >> 8) & 0xff);
	stream_buf[stream_buf_pos + 2] = (uint8_t)((v >> 16) & 0xff);
	stream_buf[stream_buf_pos + 3] = (uint8_t)((v >> 24) & 0xff);

	stream_buf_pos += 4;

	return true;
}

/* Write foat to the stream. */
static bool
write_string(
	const char *val)
{
	size_t len, i;

	if (val == NULL)
		val = "";

	len = strlen(val) + 1;
	if (!resize_buffer(len))
		return false;

	for (i = 0; i < len; i++)
		stream_buf[stream_buf_pos + i] = (uint8_t)val[i];

	stream_buf_pos += len;

	return true;
}

/* Write foat to the stream. */
static bool
write_data(
	const void *data,
	size_t size)
{
	if (!resize_buffer(size))
		return false;

	memcpy(stream_buf + stream_buf_pos, data, size);

	stream_buf_pos += size;

	return true;
}

/* Resize the write stream buffer. */
static bool
resize_buffer(
	size_t inc_size)
{
	if (stream_buf_pos + inc_size < stream_buf_alloc_size)
		return true;

	stream_buf_alloc_size *= 2;
	stream_buf = realloc(stream_buf, stream_buf_alloc_size);
	if (stream_buf == NULL)
		return false;

	return true;
}

/* Close the write stream */
static bool
close_write_stream(
	const char *file,
	...)
{
	char fname[128];
	va_list ap;

	if (file != NULL) {
		va_start(ap, file);
		vsnprintf(fname, sizeof(fname), file, ap);
		va_end(ap);

		if (!s3_write_save_data(fname, stream_buf, stream_buf_pos))
			return false;
	}

	if (stream_buf != NULL) {
		free(stream_buf);
		stream_buf = NULL;
		stream_buf_alloc_size = 0;
		stream_buf_pos = 0;
	}

	return true;
}

/* Open the read stream. */
static bool
open_read_stream(
	const char *file,
	...)
{
	char fname[128];
	size_t size, ret_size;
	va_list ap;

	assert(stream_buf == NULL);
	if (stream_buf != NULL)
		return false;

	va_start(ap, file);
	vsnprintf(fname, sizeof(fname), file, ap);
	va_end(ap);
	
	size = s3_get_save_data_size(fname);

	stream_buf = malloc(size);
	if (stream_buf == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	if (!s3_read_save_data(fname, stream_buf, size, &ret_size))
		return false;
	if (ret_size != size)
		return false;

	stream_buf_alloc_size = size;
	stream_buf_pos = 0;

	return true;
}

/* Read u32 from the stream. */
static bool
read_u32(
	uint32_t *ret)
{
	if (stream_buf_pos + 4 > stream_buf_alloc_size)
		return false;

	*ret = (stream_buf[stream_buf_pos + 0] |
		((uint32_t)stream_buf[stream_buf_pos + 1] << 8) |
		((uint32_t)stream_buf[stream_buf_pos + 2] << 16) |
		((uint32_t)stream_buf[stream_buf_pos + 3] << 24));

	stream_buf_pos += 4;

	return true;
}

/* Read u64 from the stream. */
static bool
read_u64(
	uint64_t *ret)
{
	if (stream_buf_pos + 8 > stream_buf_alloc_size)
		return false;

	*ret = ((uint64_t)stream_buf[stream_buf_pos + 0] |
		((uint64_t)stream_buf[stream_buf_pos + 1] << 8) |
		((uint64_t)stream_buf[stream_buf_pos + 2] << 16) |
		((uint64_t)stream_buf[stream_buf_pos + 3] << 24) |
		((uint64_t)stream_buf[stream_buf_pos + 4] << 32) |
		((uint64_t)stream_buf[stream_buf_pos + 5] << 40) |
		((uint64_t)stream_buf[stream_buf_pos + 6] << 48) |
		((uint64_t)stream_buf[stream_buf_pos + 7] << 56));

	stream_buf_pos += 8;

	return true;
}

/* Read float from the stream. */
static bool
read_f32(
	float *ret)
{
	if (stream_buf_pos + 4 > stream_buf_alloc_size)
		return false;

	*(uint32_t *)ret = (stream_buf[stream_buf_pos + 0] |
			    ((uint32_t)stream_buf[stream_buf_pos + 1] << 8) |
			    ((uint32_t)stream_buf[stream_buf_pos + 2] << 16) |
			    ((uint32_t)stream_buf[stream_buf_pos + 3] << 24));

	stream_buf_pos += 4;

	return true;
}

/* Read a string to the stream. */
static bool
read_string(
	char *val,
	size_t size)
{
	size_t i;

	i = 0;
	while (1) {
		if (i >= size)
			return false;
		if (stream_buf_pos + 1 > stream_buf_alloc_size)
			return false;

		val[i++] = (char)stream_buf[stream_buf_pos];

		if (stream_buf[stream_buf_pos] == '\0') {
			stream_buf_pos++;
			break;
		}

		stream_buf_pos++;
	}

	return true;
}

/* Read foat to the stream. */
static bool
read_data(
	void *data,
	size_t size)
{
	if (stream_buf_pos + size > stream_buf_alloc_size)
		return false;

	memcpy(data, stream_buf + stream_buf_pos, size);

	stream_buf_pos += size;

	return true;
}

/* Skip reading from the stream. */
static bool
read_skip(
	size_t size)
{
	if (stream_buf_pos + size > stream_buf_alloc_size)
		return false;

	stream_buf_pos += size;

	return true;
}

/* Close the read stream */
static bool
close_read_stream(void)
{
	if (stream_buf != NULL) {
		free(stream_buf);
		stream_buf = NULL;
		stream_buf_alloc_size = 0;
		stream_buf_pos = 0;
	}

	return true;
}
