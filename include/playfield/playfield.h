/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Copyright (c) 2025, 2026, Awe Morris. All rights reserved.
 */

/*
 * Playfield Engine C API
 */

#ifndef PLAYFIELD_PLAYFIELD_H
#define PLAYFIELD_PLAYFIELD_H

#include <playfield/c89compat.h>
#include <stratohal/platform.h>
#include <noct/noct.h>

/*
 * Input States
 */

/*
 * Mouse Position
 */
extern int pf_mouse_pos_x;
extern int pf_mouse_pos_y;

/*
 * Mouse Button
 */
extern bool pf_is_mouse_left_pressed;
extern bool pf_is_mouse_right_pressed;
extern bool pf_is_mouse_left_clicked;
extern bool pf_is_mouse_right_clicked;
extern bool pf_is_mouse_dragging;
extern bool pf_is_touch_canceled;
extern bool pf_is_swiped;

/*
 * Gamepad
 */
extern bool pf_is_gamepad_left_pressed;
extern bool pf_is_gamepad_right_pressed;
extern bool pf_is_gamepad_up_pressed;
extern bool pf_is_gamepad_down_pressed;
extern bool pf_is_gamepad_a_pressed;
extern bool pf_is_gamepad_b_pressed;
extern bool pf_is_gamepad_x_pressed;
extern bool pf_is_gamepad_y_pressed;
extern bool pf_is_gamepad_l_pressed;
extern bool pf_is_gamepad_r_pressed;
extern int pf_gamepad_analog_x1;
extern int pf_gamepad_analog_y1;
extern int pf_gamepad_analog_x2;
extern int pf_gamepad_analog_y2;
extern int pf_gamepad_analog_l;
extern int pf_gamepad_analog_r;

/*
 * Key
 */
extern bool pf_is_escape_key_pressed;
extern bool pf_is_return_key_pressed;
extern bool pf_is_space_key_pressed;
extern bool pf_is_tab_key_pressed;
extern bool pf_is_backspace_key_pressed;
extern bool pf_is_delete_key_pressed;
extern bool pf_is_home_key_pressed;
extern bool pf_is_end_key_pressed;
extern bool pf_is_pageup_key_pressed;
extern bool pf_is_pagedown_key_pressed;
extern bool pf_is_shift_key_pressed;
extern bool pf_is_control_key_pressed;
extern bool pf_is_alt_key_pressed;
extern bool pf_is_left_key_pressed;
extern bool pf_is_right_key_pressed;
extern bool pf_is_up_key_pressed;
extern bool pf_is_down_key_pressed;
extern bool pf_is_a_key_pressed;
extern bool pf_is_b_key_pressed;
extern bool pf_is_c_key_pressed;
extern bool pf_is_d_key_pressed;
extern bool pf_is_e_key_pressed;
extern bool pf_is_f_key_pressed;
extern bool pf_is_g_key_pressed;
extern bool pf_is_h_key_pressed;
extern bool pf_is_i_key_pressed;
extern bool pf_is_j_key_pressed;
extern bool pf_is_k_key_pressed;
extern bool pf_is_l_key_pressed;
extern bool pf_is_m_key_pressed;
extern bool pf_is_n_key_pressed;
extern bool pf_is_o_key_pressed;
extern bool pf_is_p_key_pressed;
extern bool pf_is_q_key_pressed;
extern bool pf_is_r_key_pressed;
extern bool pf_is_s_key_pressed;
extern bool pf_is_t_key_pressed;
extern bool pf_is_u_key_pressed;
extern bool pf_is_v_key_pressed;
extern bool pf_is_w_key_pressed;
extern bool pf_is_x_key_pressed;
extern bool pf_is_y_key_pressed;
extern bool pf_is_z_key_pressed;
extern bool pf_is_1_key_pressed;
extern bool pf_is_2_key_pressed;
extern bool pf_is_3_key_pressed;
extern bool pf_is_4_key_pressed;
extern bool pf_is_5_key_pressed;
extern bool pf_is_6_key_pressed;
extern bool pf_is_7_key_pressed;
extern bool pf_is_8_key_pressed;
extern bool pf_is_9_key_pressed;
extern bool pf_is_0_key_pressed;
extern bool pf_is_f1_key_pressed;
extern bool pf_is_f2_key_pressed;
extern bool pf_is_f3_key_pressed;
extern bool pf_is_f4_key_pressed;
extern bool pf_is_f5_key_pressed;
extern bool pf_is_f6_key_pressed;
extern bool pf_is_f7_key_pressed;
extern bool pf_is_f8_key_pressed;
extern bool pf_is_f9_key_pressed;
extern bool pf_is_f10_key_pressed;
extern bool pf_is_f11_key_pressed;
extern bool pf_is_f12_key_pressed;

/*
 * Texture
 */

/*
 * Pixel
 */
typedef uint32_t pf_pixel_t;

/*
 * Create a color texture.
 */
bool
pf_create_color_texture(
	int width,
	int height,
	int r,
	int g,
	int b,
	int a,
	int *ret);

/*
 * Load a texture.
 */
bool
pf_load_texture(
	const char *fname,
	int *ret,
	int *width,
	int *height);

/*
 * Destroy a texture.
 */
void
pf_destroy_texture(
	int tex_id);

/*
 * Notify a texture update.
 */
bool
pf_notify_texture_update(
	int tex_id);

/*
 * Draw a texture image on a texture image. (copy)
 */
void
pf_draw_texture_copy(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top);

/*
 * Draw a texture image on a texture image. (alpha-blending, dst_alpha=255)
 */
void
pf_draw_texture_alpha(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a texture image on a texture image. (add-blending)
 */
void
pf_draw_texture_add(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a texture image on a texture image. (add-blending)
 */
void
pf_draw_texture_sub(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a glyph texture image on a texture image.
 * (alphablending, special alpha value)
 */
void
pf_draw_texture_glyph(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw an emoji texture image on a texture image.
 * (alphablending, special alpha value)
 */
void
pf_draw_texture_emoji(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a texture image on a texture image. (50% dimming)
 */
void
pf_draw_texture_dim(
	int dst_tex_id,
	int dst_left,
	int dst_top,
	int src_tex_id,
	int width,
	int height,
	int src_left,
	int src_top,
	int alpha);

/*
 * Draw a texture image  on a texture image with scaling.
 */
void
pf_draw_texture_scale(
	int dst_tex_id,
	int virtual_dst_width,
	int virtual_dst_height,
	int virtual_dst_left,
	int virtual_dst_top,
	int src_tex_id);

/*
 * Fill a rectangle on a texture image.
 */
void
pf_fill_texture_rect(
	int tex_id,
	int left,
	int top,
	int width,
	int height,
	pf_pixel_t color);

/*
 * Get a raw pixel pointer.
 */
pf_pixel_t *
pf_get_texture_pixels(
	int tex_id);

/*
 * Rendering
 */

/*
 * Render a texture. (alpha blending)
 */
void
pf_render_texture(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	int tex_id,
	int src_x,
	int src_y,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (add blending)
 */
void
pf_render_texture_add(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	int tex_id,
	int src_x,
	int src_y,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (sub blending)
 */
void
pf_render_texture_sub(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	int tex_id,
	int src_x,
	int src_y,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (dim blending)
 */
void
pf_render_texture_dim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	int tex_id,
	int src_x,
	int src_y,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (3D, alpha blending)
 */
void
pf_render_texture_3d(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	int tex_id,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (3D, add blending)
 */
void
pf_render_texture_3d_add(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	int tex_id,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (3D, sub blending)
 */
void
pf_render_texture_3d_sub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	int tex_id,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture. (3D, dim blending)
 */
void
pf_render_texture_3d_dim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	int tex_id,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha);

/*
 * Render a texture.
 */
void
pf_draw(
	int tex_id,
	int x,
	int y);

/*
 * Font
 */

/*
 * Load a font.
 */
bool
pf_load_font(
	int slot,
	const char *file);

/*
 * Create a text texture.
 */
bool
pf_create_text_texture(
	int slot,
	const char *text,
	int size,
	pf_pixel_t color,
	int *tex_id,
	int *width,
	int *height);

/*
 * Create a text texture with outlining.
 */
bool
pf_create_text_texture_outline(
	int slot,
	const char *text,
	int size,
	pf_pixel_t color,
	int outline_width,
	pf_pixel_t outline_color,
	int *tex_id,
	int *width,
	int *height);

/*
 * Sound
 */

/*
 * Play a sound on a stream.
 */
bool
pf_play_sound(
	int stream,
	const char *file);

/*
 * Stop a sound on a stream.
 */
bool
pf_stop_sound(
	int stream);

/*
 * Set a sound volume on a stream.
 */
bool
pf_set_sound_volume(
	int stream,
	float val);

/*
 * Check if a sound stream is completed.
 */
bool
pf_is_sound_finished(
	int stream);

/*
 * Video Playback
 */

/*
 * Play a video.
 */
bool
pf_play_video(
	const char *file,
	bool is_skippable);

/*
 * Stop the video.
 */
void
pf_stop_video(void);

/*
 * Check if a video is playing back.
 */
bool
pf_is_video_playing(void);

/*
 * Lap Timer
 */

/*
 * Reset a lap timer and initializes it with a current time.
 */
void pf_reset_lap_timer(uint64_t *origin);

/*
 * Get a lap time in milliseconds.
 */
uint64_t pf_get_lap_timer_millisec(uint64_t *origin);

/*
 * Save Data
 */

/*
 * Write save data.
 */
bool
pf_write_save_data(
	const char *key,
	const void *data,
	size_t size);

/*
 * Read save data.
 */
bool
pf_read_save_data(
	const char *key,
	void *data,
	size_t size,
	size_t *ret);

/*
 * Check whether save data exist or not.
 */
bool
pf_check_save_data(
	const char *key);

/*
 * Get the size of save data.
 */
bool
pf_get_save_data_size(
	const char *key,
	size_t *ret);

/*
 * Raw File I/O
 */

/*
 * Check if a file exists.
 */
bool
pf_check_file_exists(
	const char *fname);

/*
 * Read a file.
 */
bool
pf_read_file_content(
	const char *fname,
	char **buf,
	size_t *len);

/*
 * VM
 */

/*
 * Get the VM environment.
 */
void *
pf_get_vm_env(void);

/*
 * Call a VM function.
 */
bool
pf_call_vm_function(
	const char *func_name);

/*
 * Set a VM integer.
 */
bool
pf_set_vm_int(
	const char *prop_name,
	int val);

/*
 * Get a VM integer.
 */
bool
pf_get_vm_int(
	const char *prop_name,
	int *val);

/*
 * Scripting Interface
 */

/*
 * Install an API function.
 */
bool
pf_install_api(
	const char *name,
	bool (*func)(void *));

/*
 * Get a function call parameter as an integer.
 */
bool
pf_get_call_arg_int(
	const char *name,
	int *val);

/*
 * Get a function call parameter as a float.
 */
bool
pf_get_call_arg_float(
	const char *name,
	float *val);

/*
 * Get a function call parameter as a string.
 */
bool
pf_get_call_arg_string(
	const char *name,
	char **val);

/*
 * Get the length of an array of a function call parameter.
 */
bool
pf_get_call_arg_array_length(
	const char *name,
	int *val);

/*
 * Get an integer element of an array of a function call parameter.
 */
bool
pf_get_call_arg_array_int(
	const char *name,
	int index,
	int *val);

/*
 * Get a float element of an array of a function call parameter.
 */
bool
pf_get_call_arg_array_float(
	const char *name,
	int index,
	float *val);

/*
 * Get a string element of an array of a function call parameter.
 */
bool
pf_get_call_arg_array_string(
	const char *name,
	int index,
	char **val);

/*
 * Get an integer element of a dictionary of a function call parameter.
 */
bool
pf_get_call_arg_dict_int(
	const char *name,
	const char *key,
	int *val);

/*
 * Get a float element of a dictionary of a function call parameter.
 */
bool
pf_get_call_arg_dict_float(
	const char *name,
	const char *key,
	float *val);

/*
 * Get a string element of an array of a function call parameter.
 */
bool
pf_get_call_arg_dict_string(
	const char *name,
	const char *key,
	char **val);

/*
 * Set an integer return value.
 */
bool
pf_set_return_int(
	int val);

/*
 * Set a float return value.
 */
bool
pf_set_return_float(
	float val);

/*
 * Set a string return value.
 */
bool
pf_set_return_string(
	const char *val);

/*
 * Set an integer array return value.
 */
bool
pf_set_return_int_array(
	int *val,
	int len);

/*
 * Set a float array return value.
 */
bool
pf_set_return_float_array(
	float *val,
	int len);

/*
 * Set a string array return value.
 */
bool
pf_set_return_string_array(
	const char **val,
	int len);

/*
 * Set a dictionary return value.
 *  - type_desc ... A string that describes data types.
 *    "III" means 3 integers.
 *    "IIS" means 2 integers and 1 string.
 *    "IFS" means 1 integers, 1 float, and 1 string.
 */
bool
pf_set_return_dictionary(
	const char *type_desc,
	const char **keys,
	void **data);


/*
 * Misc
 */

/*
 * Return whether we support the full screen mode.
 */
bool
pf_is_full_screen_supported(void);

/*
 * Return whether the we are in the full screen mode.
 */
bool
pf_is_full_screen_mode(void);

/*
 * Enter the full screen mode.
 */
void
pf_enter_full_screen_mode(void);

/*
 * Leave the full screen mode.
 */
void
pf_leave_full_screen_mode(void);

/*
 * Enable/disable message skip by touch move.
 */
void
pf_set_continuous_swipe_enabled(
	bool is_enabled);

/*
 * Get the system language.
 */
const char *
pf_get_system_language(void);

/*
 * Print a debug message.
 */
void
pf_log_info(
	const char *msg,
	...);

/*
 * Print a warning message.
 */
void
pf_log_warn(
	const char *msg,
	...);

/*
 * Print an error message.
 */
void
pf_log_error(
	const char *msg,
	...);

/*
 * Print an out-of-memory error message.
 */
void
pf_log_out_of_memory(void);

/*
 * Special initialization function for derived engines.
 */
#ifdef PF_USE_INITHOOK
bool
pf_init_hook(
	int width,
	int height);
#endif

#endif
