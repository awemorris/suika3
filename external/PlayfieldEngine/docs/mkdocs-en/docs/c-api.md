Playfield C API
===============

## Project Skeleton

Here is the Ray Script skeleton for native code integration.

```
// Called when booted.
func setup() {
    // You have to return the window configuration.
    return {
        width:      1280,
        height:     720,
        title:      "My First Game",
        fullscreen: false
    };
}

// "pf_init_hook()" in called between "setup()" and "start()".

// Called when the game starts.
func start() {
}

// Called every frame before rendering.
func update() {
    // You can call a native code function.
    MyEngine.foo();
}

// Called every frame to render graphics.
func render() {
}
```

Here is the native code skeleton.

```
#include <playfield/playfield.h>

static bool foo(void *p);

bool pf_init_hook(int width, int height)
{
    // Register your API function to be called in Ray.
    if (!pf_install_package_api("MyEngine", "foo", foo))
        return false;

    return true;
}

static bool foo(void *p)
{
    char *message;
    pf_get_call_arg_string(
        "message",  /* Argument name. */
        true,       /* Omissible.     */
        NULL);      /* Default value. */

    pf_log_info("MyEngine.foo(): %s", message);

    free(message);
    return true;
}
```

---

## Input States

### Mouse Position

```
extern int pf_mouse_pos_x;
extern int pf_mouse_pos_y;
```

### Mouse Button

```
extern bool pf_is_mouse_left_pressed;
extern bool pf_is_mouse_right_pressed;
extern bool pf_is_mouse_left_clicked;
extern bool pf_is_mouse_right_clicked;
extern bool pf_is_mouse_dragging;
extern bool pf_is_touch_canceled;
extern bool pf_is_swiped;
```

### Gamepad

```
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
```

### Key

```
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
```

---

## Textures and Offscreen Drawing

A texture manages image pixel data. Pixels data are automatically
uploaded to GPU when required. You have to call
`pf_notify_texture_update()` after pixel modification.

A texture is identified by an integer.

A pixel value is a packed RGBA8888, and has a type:
```
typedef uint32_t pf_pixel_t;
```

Actually, `pf_pixel_t` is equivalent to `hal_pixel_t` in `StratoHAL`.
To manipulate pixel values, please include `stratohal/platform.h` and
use `hal_make_pixel()`.


### `pf_create_color_texture()`

Creates a texture that is uniformlly filled by a color.

```
bool
pf_create_color_texture(
        int width,
        int height,
        int r,
        int g,
        int b,
        int a,
        int *ret);
```

### `pf_load_texture()`

Loads a texture from a file.

Supported formats are:
- PNG
- JPEG
- WebP

```
bool
pf_load_texture(
        const char *fname,
        int *ret,
        int *width,
        int *height);
```

### `pf_destroy_texture()`

Destroys a texture.

```
void
pf_destroy_texture(
        int tex_id);
```

### `pf_notify_texture_update()`

Notifies a texture update.
This tells the library that a GPU upload is required.

```
bool
pf_notify_texture_update(
        int tex_id);
```

### `pf_draw_texture()`

Draws a texture image on a texture image. (copy)

```
void
pf_draw_texture(
        int dst_tex_id,
        int dst_left,
        int dst_top,
        int src_tex_id,
        int src_left,
        int src_top,
        int src_width,
        int src_height,
        int alpha,
        int blend);
```

Blending types:
| Type           | Description                                   |
|----------------|-----------------------------------------------|
| PF_BLEND_COPY  | Copy RGBA components.                         |
| PF_BLEND_ALPHA | Blend. DstA = 255.                            |
| PF_BLEND_ADD         | Add.                                          |
| PF_BLEND_SUB         | Subtract.                                     |
| PF_BLEND_DIM         | 50% dim blend.                                |
| PF_BLEND_GLYPH | Blend. DstA = SrcA * a. Use single component. |
| PF_BLEND_EMOJI | Blend. DstA = SrcA * a. Use RGBA.             |

### `pf_draw_texture_3d()`

Draws a texture image on a texture image. (3D)

```
void
pf_draw_texture_3d(
        int dst_tex_id,
        float x1,
        float y1,
        float x2,
        float y2,
        float x3,
        float y3,
        float x4,
        float y4,
        int src_tex_id,
        int src_left,
        int src_top,
        int src_width,
        int src_height,
        int alpha,
        int blend);
```

Blending types:
| Type           | Description                                   |
|----------------|-----------------------------------------------|
| PF_BLEND_COPY  | Copy RGBA components.                         |
| PF_BLEND_ALPHA | Blend. DstA = 255.                            |
| PF_BLEND_ADD         | Add.                                          |
| PF_BLEND_SUB         | Subtract.                                     |
| PF_BLEND_DIM         | 50% dim blend.                                |
| PF_BLEND_GLYPH | Blend. DstA = SrcA * a. Use single component. |
| PF_BLEND_EMOJI | Blend. DstA = SrcA * a. Use RGBA.             |

### `pf_fill_texture_rect()`

Fills a rectangle on a texture image uniformly by a color.

```
void
pf_fill_texture_rect(
        int tex_id,
        int left,
        int top,
        int width,
        int height,
        pf_pixel_t color);
```

### `pf_get_texture_pixels()`

Gets a raw pixel pointer.

The size of the pixel data is `width * height`, and rows are aligned
to 4-bytes.

```
pf_pixel_t *
pf_get_texture_pixels(
        int tex_id);
```

---

## On-Screen Rendering

### `pf_render_texture()`

Renders a texture. (alpha blending)

```
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
        int alpha,
        int blend);
```

Blending types:
| Type           | Description                                   |
|----------------|-----------------------------------------------|
| PF_BLEND_COPY  | Copy RGBA components.                         |
| PF_BLEND_ALPHA | Blend. DstA = 255.                            |
| PF_BLEND_ADD         | Add.                                          |
| PF_BLEND_SUB         | Subtract.                                     |
| PF_BLEND_DIM         | 50% dim blend.                                |

### `pf_render_texture_rule()`

Renders textures for 1-bit universal transition.

```
void
pf_render_texture_rule(
        int tex1_id,
        int tex2_id,
        int alpha);
```

### `pf_render_texture_melt()`

Renders textures for 8-bit universal transition.

```
void
pf_render_texture_melt(
        int tex1_id,
        int tex2_id,
        int alpha);
```

### `pf_render_texture_cross()`

Renders textures for cross fading.

```
void
pf_render_texture_cross(
        int tex1_id,
        int tex2_id,
        int src1_left,
        int src1_top,
        int src2_left,
        int src2_top,
        int alpha);
```

### `pf_render_texture_3d()`

Renders a texture. (3D, alpha blending)

```
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
        int alpha,
        int blend);
```

Blending types:
| Type           | Description                                   |
|----------------|-----------------------------------------------|
| PF_BLEND_COPY  | Copy RGBA components.                         |
| PF_BLEND_ALPHA | Blend. DstA = 255.                            |
| PF_BLEND_ADD         | Add.                                          |
| PF_BLEND_SUB         | Subtract.                                     |
| PF_BLEND_DIM         | 50% dim blend.                                |

### `pf_render_texture_3d_cross()`

Renders textures for cross fading.

```
void
pf_render_texture_3d_cross(
        int tex1_id,
        int tex2_id,
        float src1_x1,
        float src1_y1,
        float src1_x2,
        float src1_y2,
        float src1_x3,
        float src1_y3,
        float src1_x4,
        float src1_y4,
        float src2_x1,
        float src2_y1,
        float src2_x2,
        float src2_y2,
        float src2_x3,
        float src2_y3,
        float src2_x4,
        float src2_y4,
        int alpha);
```

### `pf_draw()`

Renders a texture.

```
void
pf_draw(
        int tex_id,
        int x,
        int y);
```

---

## Font

Fonts are loaded to 4 font slots, and you can generate textures of
glyphs.

### `pf_load_font()`

Loads a font file. The allowed format is TrueType and OpenType fonts.

```
bool
pf_load_font(
        int slot,
        const char *file);
```

### `pf_create_text_texture()`

Creates a text texture.

```
bool
pf_create_text_texture(
        int slot,
        const char *text,
        int size,
        pf_pixel_t color,
        int *tex_id,
        int *width,
        int *height);
```

### `pf_create_text_texture_outline()`

Creates a text texture with outlining.

```
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
```

---

## Sound

There are 6 sound output streams.

### `pf_play_sound()`

Plays a sound on a stream.

```
bool
pf_play_sound(
        int stream,
        const char *file,
        bool is_loop);
```

### `pf_stop_sound()`

Stops a sound on a stream.

```
bool
pf_stop_sound(
        int stream);
```

### `pf_set_sound_volume()`

Sets a sound volume on a stream.

```
bool
pf_set_sound_volume(
        int stream,
        float val);
```

### `pf_is_sound_finished()`

Checks if a sound stream is completed.

```
bool
pf_is_sound_finished(
        int stream);
```

---

## Video Playback

### `pf_play_video()`

Plays a video.

```
bool
pf_play_video(
        const char *file,
        bool is_skippable);
```

### `pf_stop_video()`

Stops the video.

```
void
pf_stop_video(void);
```

### `pf_is_video_playing()`

Checks if a video is playing back.

```
bool
pf_is_video_playing(void);
```

---

## Lap Timer

### `pf_reset_lap_timer()`

Resets a lap timer and initializes it with a current time.

```
void
pf_reset_lap_timer(
        uint64_t *origin);
```

### `pf_get_lap_timer_millisec()`

Gets a lap time in milliseconds.

```
uint64_t
pf_get_lap_timer_millisec(
        uint64_t *origin);
```

---

## Save Data

In Playfield Engine, save data are realized as a Key-Value Store.
This is because some platforms don't allow a raw file write access,
and they may provide Key-Value Stores instead.

### `pf_write_save_data()`

Writes save data.

```
bool
pf_write_save_data(
        const char *key,
        const void *data,
        size_t size);
```

### `pf_read_save_data()`

Reads save data.

```
bool
pf_read_save_data(
        const char *key,
        void *data,
        size_t size,
        size_t *ret);
```

### `pf_check_save_data()`

Checks whether save data exist or not.

```
bool
pf_check_save_data(
        const char *key);
```

### `pf_get_save_data_size()`

Gets the size of save data.

```
bool
pf_get_save_data_size(
        const char *key,
        size_t *ret);
```

---

## Raw Asset Read

### `pf_check_file_exists()`

Checks if a file exists.

```
bool
pf_check_file_exists(
        const char *fname);

### `pf_read_file_content()`

Reads a file.

```
bool
pf_read_file_content(
        const char *fname,
        char **buf,
        size_t *len);
```

---

## VM

### `pf_get_vm_env()`

Gets the VM environment.

```
void *
pf_get_vm_env(void);
```

### `pf_call_vm_function()`

Calls a VM function.

```
bool
pf_call_vm_function(
        const char *func_name);
```

### `pf_set_vm_int()`

Sets an integer under `Engine` dictionary.

```
bool
pf_set_vm_int(
        const char *prop_name,
        int val);
```

### `pf_get_vm_int()`

Gets an integer under `Engine` dictionary.

```
bool
pf_get_vm_int(
        const char *prop_name,
        int *val);
```

---

## Scripting Interface

### `pf_install_api()`

Installs an API function in the global name space.

```
bool
pf_install_api(
        const char *name,
        bool (*func)(void *));
```

### `pf_install_package_api()`

Installs an API function in a package name space.
(e.g., "Engine.*")

```
bool
pf_install_package_api(
        const char *package,
        const char *name,
        bool (*func)(void *));
```

### `pf_get_call_arg_int()`

Gets a function call parameter as an integer.

```
bool
pf_get_call_arg_int(
        const char *name,
        int *val,
        bool omissible,
        int def_val);
```

### `pf_get_call_arg_float()`

Gets a function call parameter as a float.

```
bool
pf_get_call_arg_float(
        const char *name,
        float *val,
        bool omissible,
        float def_val);
```

### `pf_get_call_arg_string()`

Gets a function call parameter as a string.

```
bool
pf_get_call_arg_string(
        const char *name,
        char **val,
        bool omissible,
        const char *def_val);
```

### `pf_get_call_arg_array_length()`

Gets the length of an array of a function call parameter.

```
bool
pf_get_call_arg_array_length(
        const char *name,
        int *val);
```

### `pf_get_call_arg_array_int()`

Gets an integer element of an array of a function call parameter.

```
bool
pf_get_call_arg_array_int(
        const char *name,
        int index,
        int *val);
```

### `pf_get_call_arg_array_float()`

Gets a float element of an array of a function call parameter.

```
bool
pf_get_call_arg_array_float(
        const char *name,
        int index,
        float *val);
```

### `pf_get_call_arg_array_string()`

Gets a string element of an array of a function call parameter.

```
bool
pf_get_call_arg_array_string(
        const char *name,
        int index,
        char **val);
```

### `pf_get_call_arg_dict_int()`

Gets an integer element of a dictionary of a function call parameter.

```
bool
pf_get_call_arg_dict_int(
        const char *name,
        const char *key,
        int *val,
        bool omissible,
        int def_val);
```

### `pf_get_call_arg_dict_float()`

Gets a float element of a dictionary of a function call parameter.

```
bool
pf_get_call_arg_dict_float(
        const char *name,
        const char *key,
        float *val,
        bool omissible,
        float def_val);
```

### `pf_get_call_arg_dict_string()`

Gets a string element of a dictionary of a function call parameter.

```
bool
pf_get_call_arg_dict_string(
        const char *name,
        const char *key,
        char **val,
        bool omissible,
        const char *def_val);
```

### `pf_set_return_int()`

Sets a return value of an integer.

```
bool
pf_set_return_int(
        int val);
```

### `pf_set_return_float()`

Sets a return value of a float.

```
bool
pf_set_return_float(
        float val);
```

### `pf_set_return_string()`

Sets a return value of a string.

```
bool
pf_set_return_string(
        const char *val);
```

### `pf_set_return_int_array()`

Sets a return value of an integer array.

```
bool
pf_set_return_int_array(
        int *val,
        int len);
```

### `pf_set_return_float_array()`

Sets a return value of a float array.

```
bool
pf_set_return_float_array(
        float *val,
        int len);
```

### `pf_set_return_string_array()`

Sets a return value of a string array.

```
bool
pf_set_return_string_array(
        const char **val,
        int len);
```

### `pf_set_return_dictionary()`

Sets a dictionary return value.

`type_desc` is a string that describes data types.
- "III" means 3 integers.
- "IIS" means 2 integers and 1 string.
- "IFS" means 1 integers, 1 float, and 1 string.

```
bool
pf_set_return_dictionary(
        const char *type_desc,
        const char **keys,
        void **data);
```

---

## Misc

### `pf_is_full_screen_supported()`

Returns whether we support the full screen mode.

```
bool
pf_is_full_screen_supported(void);
```

### `pf_is_full_screen_mode()`

Returns whether the we are in the full screen mode.

```
bool
pf_is_full_screen_mode(void);
```

### `pf_enter_full_screen_mode()`

Enters the full screen mode.

```
void
pf_enter_full_screen_mode(void);
```

### `pf_leave_full_screen_mode()`

Leaves the full screen mode.

```
void
pf_leave_full_screen_mode(void);
```

### `pf_set_continuous_swipe_enabled()`

Enables or disables continuous moves of swipe gestures.

```
void
pf_set_continuous_swipe_enabled(
        bool is_enabled);
```

### `pf_get_system_language()`

Gets the system language.

```
const char *
pf_get_system_language(void);
```

### `pf_log_info()`

Prints a debug message.

```
void
pf_log_info(
        const char *msg,
        ...);
```

### `pf_log_warn()`

Prints a warning message.

```
void
pf_log_warn(
        const char *msg,
        ...);
```

### `pf_log_error()`

Prints an error message.

```
void
pf_log_error(
        const char *msg,
        ...);
```

### `pf_log_out_of_memory()`

Prints an out-of-memory error message.

```
void
pf_log_out_of_memory(void);
```
