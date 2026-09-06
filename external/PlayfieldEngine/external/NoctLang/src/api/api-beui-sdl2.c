/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The SDL2 BeUI backend for desktop hosts.
 */

#include <noct/noct.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_SOURCE_MAX	(2U * 1024U * 1024U)
#define IMAGE_PIXELS_MAX	(2U * 1024U * 1024U)

#define DISPLAY_WIDTH		640U
#define DISPLAY_HEIGHT		400U

enum image_format {
	IMAGE_INDEX8 = 1,
	IMAGE_RGB24 = 2
};

enum pointer_button {
	BUTTON_LEFT = 1U << 0,
	BUTTON_RIGHT = 1U << 1,
	BUTTON_MIDDLE = 1U << 2
};

/*
 * Keeps the language-visible Key.* values identical on every platform.
 */
enum key_code {
	KEY_ESCAPE = 0x1b,
	KEY_BACKSPACE = 0x08,
	KEY_TAB = 0x09,
	KEY_ENTER = 0x0d,
	KEY_PAGE_UP = 0x136,
	KEY_PAGE_DOWN = 0x137,
	KEY_INSERT = 0x138,
	KEY_DELETE = 0x139,
	KEY_UP = 0x13a,
	KEY_LEFT = 0x13b,
	KEY_RIGHT = 0x13c,
	KEY_DOWN = 0x13d,
	KEY_HOME = 0x13e,
	KEY_END = 0x13f,
	KEY_F1 = 0x162,
	KEY_F2 = 0x163,
	KEY_F3 = 0x164,
	KEY_F4 = 0x165,
	KEY_F5 = 0x166,
	KEY_F6 = 0x167,
	KEY_F7 = 0x168,
	KEY_F8 = 0x169,
	KEY_F9 = 0x16a,
	KEY_F10 = 0x16b,
	KEY_SHIFT = 0x170
};

struct rect {
	unsigned x;
	unsigned y;
	unsigned width;
	unsigned height;
};

struct display_info {
	/* Input hint to enter(); zero means the backend's default depth. */
	unsigned preferred_bits_per_pixel;
	unsigned width;
	unsigned height;
	unsigned bits_per_pixel;
	unsigned stride;
};

/*
 * Images use a target-independent representation.  Indexed images always
 * contain one palette index per pixel; RGB24 pixels are tightly packed in
 * R, G, B order.  Palette entries and solid colors use 0x00RRGGBB.
 */
struct image {
	enum image_format format;
	unsigned width;
	unsigned height;
	size_t stride;
	const uint8_t *pixels;
	uint32_t palette[256];
	unsigned palette_size;
};

/*
 * Pointer positions are absolute display coordinates.  Targets whose
 * hardware reports motion deltas (the PC-98 bus mouse) integrate and
 * clamp inside their backend, so scripts see one coordinate space on
 * every host.
 */
struct pointer_event {
	unsigned x;
	unsigned y;
	unsigned buttons;
};

/*
 * A decoded image lives until the script destroys it or the VM shuts
 * down.  Pixels are appended to the entry so one allocation covers both.
 */
struct image_entry {
	struct image_entry *next;
	int handle;
	struct image image;
	uint8_t pixels[1];
};

struct state {
	struct display_info display;
	int display_open;
	int pointer_open;
	int close_requested;
	unsigned pointer_x;
	unsigned pointer_y;
	unsigned pointer_buttons;
	struct image_entry *images;
	int next_image_handle;
};

/* Describes the validated layout of one supported BMP image. */
struct bmp_layout {
	const uint8_t *bytes;
	size_t size;
	size_t data_offset;
	size_t source_stride;
	size_t output_stride;
	size_t output_size;
	size_t palette_offset;
	unsigned palette_size;
	unsigned width;
	unsigned height;
	unsigned bits_per_pixel;
	int top_down;
	enum image_format format;
};

struct ffi_item {
	const char *global_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};

struct int_constant {
	const char *name;
	int value;
};

struct sdl2_context {
	SDL_Window *window;
	SDL_Surface *framebuffer;
	int video_initialized;
	int timer_initialized;
	int alive;
};

static struct state state;

/* Key names exposed through the Key dictionary. */
static const struct int_constant key_constants[] = {
	{"Escape", KEY_ESCAPE},
	{"Tab", KEY_TAB},
	{"Enter", KEY_ENTER},
	{"Backspace", KEY_BACKSPACE},
	{"Delete", KEY_DELETE},
	{"Insert", KEY_INSERT},
	{"Up", KEY_UP},
	{"Down", KEY_DOWN},
	{"Left", KEY_LEFT},
	{"Right", KEY_RIGHT},
	{"Home", KEY_HOME},
	{"End", KEY_END},
	{"PageUp", KEY_PAGE_UP},
	{"PageDown", KEY_PAGE_DOWN},
	{"F1", KEY_F1}, {"F2", KEY_F2},
	{"F3", KEY_F3}, {"F4", KEY_F4},
	{"F5", KEY_F5}, {"F6", KEY_F6},
	{"F7", KEY_F7}, {"F8", KEY_F8},
	{"F9", KEY_F9}, {"F10", KEY_F10},
	{"Space", ' '},
	{"Shift", KEY_SHIFT}
};

/* Bit values returned by BeUI.getPointerButtons. */
static const struct int_constant button_constants[] = {
	{"Left", BUTTON_LEFT},
	{"Right", BUTTON_RIGHT},
	{"Middle", BUTTON_MIDDLE}
};

static struct sdl2_context sdl2_context;

static int beui_init(void);
static int beui_init_with_hint(unsigned preferred_bits_per_pixel);
static void beui_close(void);
static void beui_cleanup(void);
static int beui_is_open(void);
static int beui_get_display_info(struct display_info *info);
static int beui_fill(const struct rect *rect, uint32_t color);
static int beui_line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int beui_pattern_fill(const struct rect *rect, uint32_t color, uint64_t pattern);
static int image_valid(const struct image *image);
static int beui_draw_image(unsigned x, unsigned y, const struct image *image);
static int beui_draw_image_region(const struct image *image, unsigned source_x, unsigned source_y, unsigned width, unsigned height, unsigned destination_x, unsigned destination_y);
static int beui_draw_image_pattern(unsigned x, unsigned y, const struct image *image, uint64_t pattern);
static uint32_t decode_utf8(const char **cursor);
static int beui_measure_text(const char *text, unsigned *width, unsigned *height);
static int beui_draw_text(const char *text, unsigned x, unsigned y, uint32_t foreground, uint32_t background);
static int beui_poll(void);
static int beui_get_pointer(unsigned *x, unsigned *y, unsigned *buttons);
static int beui_flush(void);
static int beui_get_milliseconds(uint64_t *milliseconds);
static int beui_sleep(unsigned milliseconds);
static int beui_is_key_down(int key);
static void beui_drain_input(void);
static int image_load_bmp(const void *data, size_t size);
static const struct image *image_get(int handle);
static int image_destroy(int handle);
static uint16_t read_u16(const uint8_t *bytes);
static uint32_t read_u32(const uint8_t *bytes);
static int32_t read_s32(const uint8_t *bytes);
static int add_overflows(size_t left, size_t right);
static int multiply_overflows(size_t left, size_t right);
static int parse_layout(const void *data, size_t size, struct bmp_layout *layout);
static int bmp_measure(const void *data, size_t size, enum image_format *format, unsigned *width, unsigned *height, size_t *pixel_bytes);
static int bmp_decode(const void *data, size_t size, void *pixel_storage, size_t pixel_capacity, struct image *image);
static bool return_int(NoctEnv *env, int value);
static bool get_int_arg(NoctEnv *env, uint32_t index, int *result);
static bool cfunc_BeUI_init(NoctEnv *env);
static bool cfunc_BeUI_initWithHint(NoctEnv *env);
static bool cfunc_BeUI_close(NoctEnv *env);
static bool cfunc_BeUI_isOpen(NoctEnv *env);
static bool cfunc_BeUI_getWidth(NoctEnv *env);
static bool cfunc_BeUI_getHeight(NoctEnv *env);
static bool cfunc_BeUI_poll(NoctEnv *env);
static bool cfunc_BeUI_flush(NoctEnv *env);
static bool cfunc_BeUI_fill(NoctEnv *env);
static bool cfunc_BeUI_line(NoctEnv *env);
static bool cfunc_BeUI_patternFill(NoctEnv *env);
static bool measure_text_arg(NoctEnv *env, const char *api, unsigned *width, unsigned *height);
static bool cfunc_BeUI_textWidth(NoctEnv *env);
static bool cfunc_BeUI_textHeight(NoctEnv *env);
static bool cfunc_BeUI_drawText(NoctEnv *env);
static bool cfunc_BeUI_getMilliseconds(NoctEnv *env);
static bool cfunc_BeUI_sleep(NoctEnv *env);
static bool cfunc_BeUI_isKeyDown(NoctEnv *env);
static bool pointer_field(NoctEnv *env, const char *api, unsigned *x, unsigned *y, unsigned *buttons);
static bool cfunc_BeUI_getPointerX(NoctEnv *env);
static bool cfunc_BeUI_getPointerY(NoctEnv *env);
static bool cfunc_BeUI_getPointerButtons(NoctEnv *env);
static bool cfunc_BeUI_loadImage(NoctEnv *env);
static bool cfunc_BeUI_getImageWidth(NoctEnv *env);
static bool cfunc_BeUI_getImageHeight(NoctEnv *env);
static bool cfunc_BeUI_drawImage(NoctEnv *env);
static bool cfunc_BeUI_drawImageRegion(NoctEnv *env);
static bool cfunc_BeUI_drawImagePattern(NoctEnv *env);
static bool cfunc_BeUI_destroyImage(NoctEnv *env);
static bool register_int_dictionary(NoctEnv *env, const char *name, const struct int_constant *entries, size_t count);
static bool register_beui_api(NoctEnv *env);
static uint32_t sdl2_color(uint32_t color);
static int sdl2_pattern_bit(uint64_t pattern, unsigned x, unsigned y);
static void sdl2_put_pixel(struct sdl2_context *context, unsigned x, unsigned y, uint32_t color);
static int sdl2_lock_framebuffer(struct sdl2_context *context);
static void sdl2_unlock_framebuffer(struct sdl2_context *context);
static int sdl2_enter(struct sdl2_context *context, struct display_info *info);
static void sdl2_leave(struct sdl2_context *context);
static int sdl2_poll_events(struct sdl2_context *context);
static int sdl2_fill(struct sdl2_context *context, const struct rect *rect, uint32_t color);
static int sdl2_line(struct sdl2_context *context, unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int sdl2_pattern_fill(struct sdl2_context *context, const struct rect *rect, uint32_t color, uint64_t pattern);
static uint32_t sdl2_image_pixel(const struct image *image, unsigned x, unsigned y);
static int sdl2_draw_image_common(struct sdl2_context *context, unsigned x, unsigned y, const struct image *image, uint64_t pattern, int patterned);
static int sdl2_draw_image(struct sdl2_context *context, unsigned x, unsigned y, const struct image *image);
static int sdl2_draw_image_pattern(struct sdl2_context *context, unsigned x, unsigned y, const struct image *image, uint64_t pattern);
static int sdl2_flush(struct sdl2_context *context, const struct rect *rectangles, size_t rectangle_count);
static void sdl2_ascii_glyph(uint32_t codepoint, uint8_t rows[7]);
static int sdl2_glyph_measure(uint32_t codepoint, unsigned *width, unsigned *height);
static int sdl2_glyph_draw(struct sdl2_context *context, unsigned x, unsigned y, uint32_t codepoint, uint32_t foreground, uint32_t background);
static int sdl2_pointer_start(struct sdl2_context *context, const struct display_info *display);
static int sdl2_pointer_poll(struct sdl2_context *context, struct pointer_event *event);
static uint64_t sdl2_milliseconds(struct sdl2_context *context);
static SDL_Scancode sdl2_key_scancode(int key);
static int sdl2_is_key_down(struct sdl2_context *context, int key);
static void sdl2_drain_input(struct sdl2_context *context);

/*
 * Registers the SDL2 BeUI API.
 */
NOCT_DLL
bool
noct_register_api_beui(
	NoctEnv *env)
{
	bool registered;

	/* Releases resources and handles owned by a previous registration. */
	beui_cleanup();

	/* Registers the SDL2 implementation and language dictionaries. */
	registered = register_beui_api(env);

	/* Reports whether the SDL2 BeUI API was registered. */
	return registered;
}

/* Opens BeUI with SDL2's default pixel depth. */
static int
beui_init(
	void)
{
	int initialized;

	/* Opens SDL2 without a preferred pixel-depth hint. */
	initialized = beui_init_with_hint(0);

	/* Reports whether SDL2 opened. */
	return initialized;
}

/* Opens SDL2 with an optional preferred pixel depth. */
static int
beui_init_with_hint(
	unsigned preferred_bits_per_pixel)
{
	int entered;
	int pointer_started;

	/* Preserves an already open display session. */
	if (state.display_open)
		return 1;

	/* Initializes the display request and preferred pixel depth. */
	memset(&state.display, 0, sizeof(state.display));
	state.display.preferred_bits_per_pixel = preferred_bits_per_pixel;

	/* Enters the SDL2 display mode. */
	entered = sdl2_enter(&sdl2_context, &state.display);
	if (!entered)
		return 0;

	/* Publishes the initial display and input state. */
	state.display_open = 1;
	state.close_requested = 0;
	state.pointer_buttons = 0;

	/* Discards input typed before the graphics session began. */
	beui_drain_input();

	/* Closes SDL2 when it returned unusable display dimensions. */
	if (state.display.width == 0 || state.display.height == 0) {
		beui_close();
		return 0;
	}

	/* Centers the initial pointer state within the new display. */
	state.pointer_x = state.display.width / 2U;
	state.pointer_y = state.display.height / 2U;

	/* Centers the SDL2 pointer after the display is valid. */
	pointer_started = sdl2_pointer_start(&sdl2_context, &state.display);
	if (!pointer_started) {
		beui_close();
		return 0;
	}
	state.pointer_open = 1;

	/* Reports a successfully opened BeUI session. */
	return 1;
}

/* Closes the active SDL2 display and pointer. */
static void
beui_close(
	void)
{
	/* Discards keys held or buffered during the graphics session. */
	beui_drain_input();

	/* Releases the logical pointer before leaving the display. */
	state.pointer_open = 0;

	/* Leaves the active SDL2 display. */
	if (state.display_open)
		sdl2_leave(&sdl2_context);

	state.display_open = 0;

	/* Clears display geometry that is no longer valid. */
	memset(&state.display, 0, sizeof(state.display));
}

/* Releases all BeUI services and registered images. */
static void
beui_cleanup(
	void)
{
	struct image_entry *entry;
	struct image_entry *next;

	/* Captures the registry before SDL2 resources are released. */
	entry = state.images;

	/* Releases active SDL2 resources before image storage. */
	beui_close();

	/* Releases every decoded image in registry order. */
	while (entry != NULL) {
		next = entry->next;
		free(entry);
		entry = next;
	}

	/* Clears the complete BeUI state. */
	memset(&state, 0, sizeof(state));
}

/* Reports whether the BeUI display is open. */
static int
beui_is_open(
	void)
{
	/* Returns the current display ownership state. */
	return state.display_open;
}

/* Copies the active BeUI display geometry. */
static int
beui_get_display_info(
	struct display_info *info)
{
	/* Requires an active display and a result destination. */
	if (!state.display_open || info == NULL)
		return 0;

	/* Copies the complete display description. */
	*info = state.display;

	/* Reports that the display description was copied. */
	return 1;
}

/* Fills one validated BeUI display rectangle. */
static int
beui_fill(
	const struct rect *rect,
	uint32_t color)
{
	int filled;

	/* Requires a visible rectangle on the active display. */
	if (!state.display_open ||
	    rect == NULL ||
	    rect->width == 0 ||
	    rect->height == 0 ||
	    rect->x >= state.display.width ||
	    rect->y >= state.display.height ||
	    rect->width > state.display.width - rect->x ||
	    rect->height > state.display.height - rect->y) {
		return 0;
	}

	/* Draws the validated fill into the SDL2 framebuffer. */
	filled = sdl2_fill(&sdl2_context, rect, color);

	/* Returns the SDL2 fill status. */
	return filled;
}

/* Draws one validated BeUI display line. */
static int
beui_line(
	unsigned x0,
	unsigned y0,
	unsigned x1,
	unsigned y1,
	uint32_t color)
{
	int drawn;

	/* Requires visible endpoints on the active display. */
	if (!state.display_open ||
	    x0 >= state.display.width ||
	    x1 >= state.display.width ||
	    y0 >= state.display.height ||
	    y1 >= state.display.height) {
		return 0;
	}

	/* Draws the validated line into the SDL2 framebuffer. */
	drawn = sdl2_line(&sdl2_context, x0, y0, x1, y1, color);

	/* Returns the SDL2 line status. */
	return drawn;
}

/* Fills one validated BeUI rectangle through an 8-by-8 pattern. */
static int
beui_pattern_fill(
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	int filled;

	/* Requires a visible rectangle on the active display. */
	if (!state.display_open ||
	    rect == NULL ||
	    rect->width == 0 ||
	    rect->height == 0 ||
	    rect->x >= state.display.width ||
	    rect->y >= state.display.height ||
	    rect->width > state.display.width - rect->x ||
	    rect->height > state.display.height - rect->y) {
		return 0;
	}

	/* Draws the validated pattern into the SDL2 framebuffer. */
	filled = sdl2_pattern_fill(&sdl2_context, rect, color, pattern);

	/* Returns the SDL2 patterned-fill status. */
	return filled;
}

/* Validates one target-independent BeUI image description. */
static int
image_valid(
	const struct image *image)
{
	int valid;

	/* Rejects a missing image before dereferencing its fields. */
	if (image == NULL)
		return 0;

	/* Rejects incomplete geometry and unsupported image formats. */
	if (image->pixels == NULL ||
	    image->width == 0 ||
	    image->height == 0 ||
	    (image->format != IMAGE_INDEX8 &&
	     image->format != IMAGE_RGB24)) {
		return 0;
	}

	/* Rejects an indexed image without a valid palette. */
	if (image->format == IMAGE_INDEX8 &&
	    (image->palette_size == 0 || image->palette_size > 256)) {
		return 0;
	}

	/* Validates the stride for the selected image representation. */
	if (image->format == IMAGE_RGB24)
		valid = image->stride / 3U >= image->width;
	else
		valid = image->stride >= image->width;

	/* Returns the normalized stride-validation result. */
	return valid;
}

/* Draws one complete validated BeUI image. */
static int
beui_draw_image(
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int drawn;
	int valid;

	/* Requires an active display before validating the image. */
	if (!state.display_open)
		return 0;

	/* Validates the source image before reading its geometry. */
	valid = image_valid(image);
	if (!valid)
		return 0;

	/* Requires the complete image to fit the SDL2 display. */
	if (x >= state.display.width ||
	    y >= state.display.height ||
	    image->width > state.display.width - x ||
	    image->height > state.display.height - y) {
		return 0;
	}

	/* Draws the validated image into the SDL2 framebuffer. */
	drawn = sdl2_draw_image(&sdl2_context, x, y, image);

	/* Returns the SDL2 image status. */
	return drawn;
}

/* Draws one validated region of a BeUI image. */
static int
beui_draw_image_region(
	const struct image *image,
	unsigned source_x,
	unsigned source_y,
	unsigned width,
	unsigned height,
	unsigned destination_x,
	unsigned destination_y)
{
	struct image region;
	size_t pixel_size;
	size_t offset;
	int drawn;
	int valid;

	/* Validates the source image before reading its geometry. */
	valid = image_valid(image);
	if (!valid)
		return 0;

	/* Requires the complete requested region to fit the source image. */
	if (width == 0 ||
	    height == 0 ||
	    source_x >= image->width ||
	    source_y >= image->height ||
	    width > image->width - source_x ||
	    height > image->height - source_y ||
	    source_y > (size_t)-1 / image->stride) {
		return 0;
	}

	/* Computes the source row offset without overflowing size_t. */
	pixel_size = image->format == IMAGE_RGB24 ? 3U : 1U;
	offset = (size_t)source_y * image->stride;

	/* Rejects an overflowing horizontal pixel offset. */
	if (source_x > ((size_t)-1 - offset) / pixel_size)
		return 0;

	/* Builds a borrowed image view over the requested source region. */
	offset += (size_t)source_x * pixel_size;
	region = *image;
	region.width = width;
	region.height = height;
	region.pixels += offset;

	/* Draws the borrowed image view at the requested destination. */
	drawn = beui_draw_image(destination_x, destination_y, &region);

	/* Returns the delegated image status. */
	return drawn;
}

/* Draws one complete BeUI image through an 8-by-8 pattern. */
static int
beui_draw_image_pattern(
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int drawn;
	int valid;

	/* Requires an active display before validating the image. */
	if (!state.display_open)
		return 0;

	/* Validates the patterned source image. */
	valid = image_valid(image);
	if (!valid)
		return 0;

	/* Requires the complete image to fit the SDL2 display. */
	if (x >= state.display.width ||
	    y >= state.display.height ||
	    image->width > state.display.width - x ||
	    image->height > state.display.height - y) {
		return 0;
	}

	/* Draws the patterned image into the SDL2 framebuffer. */
	drawn = sdl2_draw_image_pattern(&sdl2_context, x, y, image, pattern);

	/* Returns the SDL2 patterned-image status. */
	return drawn;
}

/* Decodes one UTF-8 codepoint and advances its byte cursor. */
static uint32_t
decode_utf8(
	const char **cursor)
{
	const unsigned char *text;
	uint32_t codepoint;
	unsigned length;
	unsigned index;

	/* Reads the current UTF-8 sequence without advancing it yet. */
	text = (const unsigned char *)*cursor;

	/* Returns one ASCII byte directly. */
	if (text[0] < 0x80U) {
		(*cursor)++;
		return text[0];
	}

	/* Decodes the lead byte or replaces an invalid lead byte. */
	if ((text[0] & 0xe0U) == 0xc0U) {
		codepoint = text[0] & 0x1fU;
		length = 2;
	} else if ((text[0] & 0xf0U) == 0xe0U) {
		codepoint = text[0] & 0x0fU;
		length = 3;
	} else if ((text[0] & 0xf8U) == 0xf0U) {
		codepoint = text[0] & 0x07U;
		length = 4;
	} else {
		(*cursor)++;
		return 0xfffdU;
	}

	/* Accumulates every required continuation byte. */
	for (index = 1; index < length; index++) {
		/* Replaces a sequence at its first invalid continuation byte. */
		if ((text[index] & 0xc0U) != 0x80U) {
			(*cursor)++;
			return 0xfffdU;
		}

		codepoint = (codepoint << 6) | (text[index] & 0x3fU);
	}

	/* Replaces overlong, out-of-range, and surrogate encodings. */
	if ((length == 2 && codepoint < 0x80U) ||
	    (length == 3 && codepoint < 0x800U) ||
	    (length == 4 && codepoint < 0x10000U) ||
	    codepoint > 0x10ffffU ||
	    (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
		(*cursor)++;
		return 0xfffdU;
	}

	/* Advances over the complete valid UTF-8 sequence. */
	*cursor += length;

	/* Returns the decoded Unicode codepoint. */
	return codepoint;
}

/* Measures one UTF-8 text block with the SDL2 glyph implementation. */
static int
beui_measure_text(
	const char *text,
	unsigned *width,
	unsigned *height)
{
	const char *cursor;
	uint32_t codepoint;
	unsigned glyph_width;
	unsigned glyph_height;
	unsigned line_width;
	unsigned maximum_width;
	unsigned total_height;
	int measured;

	/* Initializes the text traversal and default single-line height. */
	cursor = text;
	line_width = 0;
	maximum_width = 0;
	total_height = 16;

	/* Requires text, result destinations, and an active display. */
	if (!state.display_open ||
	    text == NULL ||
	    width == NULL ||
	    height == NULL) {
		return 0;
	}

	/* Measures every decoded character in source order. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Ignores carriage returns in the platform-neutral layout. */
		if (codepoint == '\r')
			continue;

		/* Completes one line and starts the next at a newline. */
		if (codepoint == '\n') {
			/* Retains the widest completed line. */
			if (line_width > maximum_width)
				maximum_width = line_width;
			line_width = 0;

			/* Rejects an overflowing text-block height. */
			if (total_height > (unsigned)-1 - 16U)
				return 0;

			total_height += 16U;
			continue;
		}

		/* Measures the next printable glyph. */
		measured = sdl2_glyph_measure(
			codepoint,
			&glyph_width,
			&glyph_height);
		if (!measured)
			return 0;

		/* Rejects an unsupported height or overflowing line width. */
		if (glyph_height > 16U ||
		    line_width > (unsigned)-1 - glyph_width) {
			return 0;
		}

		line_width += glyph_width;
	}

	/* Includes the final line in the maximum measured width. */
	if (line_width > maximum_width)
		maximum_width = line_width;

	/* Publishes the completed text-block dimensions. */
	*width = maximum_width;
	*height = total_height;

	/* Reports a successful text measurement. */
	return 1;
}

/* Draws one measured UTF-8 text block. */
static int
beui_draw_text(
	const char *text,
	unsigned x,
	unsigned y,
	uint32_t foreground,
	uint32_t background)
{
	const char *cursor;
	uint32_t codepoint;
	unsigned origin_x;
	unsigned glyph_width;
	unsigned glyph_height;
	unsigned width;
	unsigned height;
	int measured;
	int drawn;

	/* Initializes the traversal and line origin. */
	cursor = text;
	origin_x = x;

	/* Measures the complete text before validating its destination. */
	measured = beui_measure_text(text, &width, &height);
	if (!measured)
		return 0;

	/* Requires the text block to fit the active display. */
	if (x > state.display.width ||
	    y > state.display.height ||
	    width > state.display.width - x ||
	    height > state.display.height - y) {
		return 0;
	}

	/* Draws every decoded character in source order. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Ignores carriage returns in the platform-neutral layout. */
		if (codepoint == '\r')
			continue;

		/* Advances to the next fixed-height text line. */
		if (codepoint == '\n') {
			x = origin_x;
			y += 16U;
			continue;
		}

		/* Measures the next printable glyph before drawing it. */
		measured = sdl2_glyph_measure(codepoint, &glyph_width, &glyph_height);
		if (!measured)
			return 0;

		/* Draws the measured glyph at the current pen position. */
		drawn = sdl2_glyph_draw(&sdl2_context, x, y, codepoint, foreground, background);
		if (!drawn)
			return 0;

		x += glyph_width;
	}

	/* Reports a fully drawn text block. */
	return 1;
}

/* Services SDL2 and reports whether the display remains alive. */
static int
beui_poll(
	void)
{
	struct pointer_event event;
	int updated;

	/* Reports a closed or previously failed display immediately. */
	if (!state.display_open || state.close_requested)
		return 0;

	/* Services the SDL2 display event source. */
	updated = sdl2_poll_events(&sdl2_context);
	if (updated != 1) {
		/* Makes a closed window sticky for every later poll. */
		state.close_requested = 1;
		return 0;
	}

	/* Discards platform type-ahead after servicing display events. */
	beui_drain_input();

	/* Captures the current absolute pointer state. */
	if (state.pointer_open) {
		memset(&event, 0, sizeof(event));
		updated = sdl2_pointer_poll(&sdl2_context, &event);

		/* Makes an SDL2 pointer failure close the BeUI session. */
		if (updated < 0) {
			state.close_requested = 1;
			return 0;
		}

		/* Clamps a reported pointer state to the active display. */
		if (updated > 0) {
			/* Clamps the horizontal coordinate to the display. */
			if (event.x < state.display.width)
				state.pointer_x = event.x;
			else
				state.pointer_x = state.display.width - 1U;

			/* Clamps the vertical coordinate to the display. */
			if (event.y < state.display.height)
				state.pointer_y = event.y;
			else
				state.pointer_y = state.display.height - 1U;

			state.pointer_buttons = event.buttons;
		}
	}

	/* Reports a live display after SDL2 services succeeded. */
	return 1;
}

/* Copies the last known absolute pointer state. */
static int
beui_get_pointer(
	unsigned *x,
	unsigned *y,
	unsigned *buttons)
{
	/* Requires an active display and pointer session. */
	if (!state.display_open || !state.pointer_open)
		return 0;

	/* Copies each requested pointer field independently. */
	if (x != NULL)
		*x = state.pointer_x;

	/* Copies the vertical coordinate when requested. */
	if (y != NULL)
		*y = state.pointer_y;

	/* Copies the button state when requested. */
	if (buttons != NULL)
		*buttons = state.pointer_buttons;

	/* Reports that every requested pointer field was copied. */
	return 1;
}

/* Flushes pending drawing to the SDL2 window. */
static int
beui_flush(
	void)
{
	int flushed;

	/* Rejects flushing without an active display. */
	if (!state.display_open)
		return 0;

	/* Flushes the complete SDL2 surface. */
	flushed = sdl2_flush(&sdl2_context, NULL, 0);

	/* Returns the SDL2 flush status. */
	return flushed;
}

/* Reads the current SDL2 clock. */
static int
beui_get_milliseconds(
	uint64_t *milliseconds)
{
	uint64_t current;

	/* Requires a clock destination. */
	if (milliseconds == NULL) {
		return 0;
	}

	/* Reads and publishes the current SDL2 time. */
	current = sdl2_milliseconds(&sdl2_context);
	*milliseconds = current;

	/* Reports an available SDL2 clock. */
	return 1;
}

/* Busy-waits while servicing SDL2. */
static int
beui_sleep(
	unsigned milliseconds)
{
	uint64_t start;
	uint64_t now;
	int available;
	int alive;

	/* Captures the starting SDL2 time. */
	available = beui_get_milliseconds(&start);
	if (!available)
		return 0;

	/* Services SDL2 until the requested clock interval expires. */
	do {
		beui_drain_input();

		/* Stops early when an open display closes during the wait. */
		if (state.display_open) {
			alive = beui_poll();
			if (!alive)
				break;
		}

		/* Reads the time for the next interval decision. */
		available = beui_get_milliseconds(&now);
		if (!available)
			return 0;
	} while (now - start < milliseconds);

	/* Reports a completed or display-interrupted wait. */
	return 1;
}

/* Reads one real-time SDL2 key state. */
static int
beui_is_key_down(
	int key)
{
	int down;

	/* Reads the requested key state from SDL2. */
	down = sdl2_is_key_down(&sdl2_context, key);

	/* Returns the SDL2 key-state convention unchanged. */
	return down;
}

/* Drains pending SDL2 input events. */
static void
beui_drain_input(
	void)
{
	/* Drains events through the SDL2 input implementation. */
	sdl2_drain_input(&sdl2_context);
}

/* Decodes and registers one BMP image. */
static int
image_load_bmp(
	const void *data,
	size_t size)
{
	struct image_entry *entry;
	enum image_format format;
	unsigned width;
	unsigned height;
	size_t pixel_size;
	size_t allocation_size;
	int measured;
	int decoded;

	/* Rejects an absent, empty, or oversized encoded image. */
	if (data == NULL ||
	    size == 0 ||
	    size > IMAGE_SOURCE_MAX) {
		return 0;
	}

	/* Measures the BMP before allocating its decoded storage. */
	measured = bmp_measure(
		data,
		size,
		&format,
		&width,
		&height,
		&pixel_size);
	if (!measured)
		return 0;

	/* Rejects an empty or oversized decoded image. */
	if (pixel_size == 0 || pixel_size > IMAGE_PIXELS_MAX)
		return 0;

	/* Allocates one registry entry with trailing pixel storage. */
	allocation_size = offsetof(struct image_entry, pixels) + pixel_size;
	entry = malloc(allocation_size);
	if (entry == NULL)
		return 0;

	/* Decodes the BMP directly into the owned trailing storage. */
	decoded = bmp_decode(data, size, entry->pixels, pixel_size, &entry->image);
	if (!decoded) {
		free(entry);
		return 0;
	}

	/* Repairs a nonpositive stored handle counter. */
	if (state.next_image_handle <= 0)
		state.next_image_handle = 1;

	/* Prepends the decoded image to the registry. */
	entry->handle = state.next_image_handle++;
	entry->next = state.images;
	state.images = entry;

	/* Returns the new positive image handle. */
	return entry->handle;
}

/* Resolves one positive image handle. */
static const struct image *
image_get(
	int handle)
{
	struct image_entry *entry;

	/* Rejects values outside the positive handle namespace. */
	if (handle <= 0)
		return NULL;

	/* Searches the image registry from newest to oldest. */
	for (entry = state.images;
	     entry != NULL;
	     entry = entry->next) {
		/* Returns the image owned by a matching registry entry. */
		if (entry->handle == handle)
			return &entry->image;
	}

	/* Reports an unknown image handle. */
	return NULL;
}

/* Destroys one registered image handle. */
static int
image_destroy(
	int handle)
{
	struct image_entry **link;
	struct image_entry *entry;

	/* Rejects values outside the positive handle namespace. */
	if (handle <= 0)
		return 0;

	/* Searches the registry link that owns the requested entry. */
	for (link = &state.images;
	     *link != NULL;
	     link = &(*link)->next) {
		entry = *link;

		/* Continues past a different image handle. */
		if (entry->handle != handle)
			continue;

		/* Unlinks and releases the matching image entry. */
		*link = entry->next;
		free(entry);

		/* Reports that the image was destroyed. */
		return 1;
	}

	/* Reports an unknown image handle. */
	return 0;
}

/* Reads one little-endian unsigned 16-bit integer. */
static uint16_t
read_u16(
	const uint8_t *bytes)
{
	uint16_t value;

	/* Combines the two source bytes in little-endian order. */
	value = (uint16_t)(bytes[0] | (uint16_t)bytes[1] << 8);

	/* Returns the decoded integer. */
	return value;
}

/* Reads one little-endian unsigned 32-bit integer. */
static uint32_t
read_u32(
	const uint8_t *bytes)
{
	uint32_t value;

	/* Combines the four source bytes in little-endian order. */
	value = (uint32_t)bytes[0] |
		(uint32_t)bytes[1] << 8 |
		(uint32_t)bytes[2] << 16 |
		(uint32_t)bytes[3] << 24;

	/* Returns the decoded integer. */
	return value;
}

/* Reads one little-endian signed 32-bit integer. */
static int32_t
read_s32(
	const uint8_t *bytes)
{
	uint32_t unsigned_value;
	int32_t signed_value;

	/* Reads the encoded bit pattern before converting its signedness. */
	unsigned_value = read_u32(bytes);
	signed_value = (int32_t)unsigned_value;

	/* Returns the decoded signed integer. */
	return signed_value;
}

/* Tests whether one size_t addition would overflow. */
static int
add_overflows(
	size_t left,
	size_t right)
{
	int overflows;

	/* Compares the left operand with the remaining size_t range. */
	overflows = left > SIZE_MAX - right;

	/* Returns the normalized overflow result. */
	return overflows;
}

/* Tests whether one size_t multiplication would overflow. */
static int
multiply_overflows(
	size_t left,
	size_t right)
{
	int overflows;

	/* Avoids division by zero while checking the remaining size_t range. */
	overflows = left != 0 && right > SIZE_MAX / left;

	/* Returns the normalized overflow result. */
	return overflows;
}

/* Parses and validates one supported BMP layout. */
static int
parse_layout(
	const void *data,
	size_t size,
	struct bmp_layout *layout)
{
	const uint8_t *bytes;
	uint32_t dib_size;
	uint32_t data_offset;
	uint32_t colors_used;
	uint32_t compression;
	uint16_t planes;
	int32_t signed_width;
	int32_t signed_height;
	size_t row_bits;
	size_t palette_bytes;
	size_t palette_end;
	size_t source_bytes;
	unsigned bytes_per_pixel;
	int overflows;

	/* Views the encoded image as unsigned bytes. */
	bytes = data;

	/* Requires a destination and a complete Windows BMP file header. */
	if (bytes == NULL ||
	    layout == NULL ||
	    size < 54U ||
	    bytes[0] != 'B' ||
	    bytes[1] != 'M') {
		return 0;
	}

	/* Reads the DIB and pixel-data offsets in the original order. */
	dib_size = read_u32(bytes + 14);
	data_offset = read_u32(bytes + 10);

	/* Rejects an unsupported or overflowing DIB header size. */
	if (dib_size < 40U)
		return 0;

	overflows = add_overflows(14U, dib_size);
	if (overflows)
		return 0;

	/* Requires the complete DIB header and an in-range pixel offset. */
	if (14U + dib_size > size || data_offset > size)
		return 0;

	/* Reads and validates the signed BMP dimensions. */
	signed_width = read_s32(bytes + 18);
	signed_height = read_s32(bytes + 22);
	if (signed_width <= 0 ||
	    signed_height == 0 ||
	    signed_height == INT32_MIN) {
		return 0;
	}

	/* Requires one uncompressed image plane. */
	planes = read_u16(bytes + 26);
	if (planes != 1U)
		return 0;

	/* Requires uncompressed pixel storage. */
	compression = read_u32(bytes + 30);
	if (compression != 0U)
		return 0;

	/* Initializes the validated common BMP layout. */
	memset(layout, 0, sizeof(*layout));
	layout->bytes = bytes;
	layout->size = size;
	layout->data_offset = data_offset;
	layout->width = (unsigned)signed_width;

	/* Converts the signed BMP height to logical row order. */
	if (signed_height < 0)
		layout->height = (unsigned)-signed_height;
	else
		layout->height = (unsigned)signed_height;

	/* Publishes the row order and encoded pixel depth. */
	layout->top_down = signed_height < 0;
	layout->bits_per_pixel = read_u16(bytes + 28);

	/* Selects and validates the supported BMP pixel representation. */
	switch (layout->bits_per_pixel) {
	case 1:
		/* Shares the indexed-image setup with wider indices. */
	case 4:
		/* Shares the indexed-image setup with byte-sized indices. */
	case 8:
		/* Configures an indexed image and reads its palette size. */
		layout->format = IMAGE_INDEX8;
		bytes_per_pixel = 1;
		colors_used = read_u32(bytes + 46);

		/* Selects an explicit or depth-derived palette size. */
		if (colors_used != 0)
			layout->palette_size = colors_used;
		else
			layout->palette_size = 1U << layout->bits_per_pixel;

		/* Requires a nonempty palette that fits the image contract. */
		if (layout->palette_size == 0 ||
		    layout->palette_size > 256U) {
			return 0;
		}

		/* Computes the palette range without overflowing size_t. */
		layout->palette_offset = 14U + dib_size;
		overflows = multiply_overflows(layout->palette_size, 4U);
		if (overflows)
			return 0;

		palette_bytes = (size_t)layout->palette_size * 4U;
		overflows = add_overflows(
			layout->palette_offset,
			palette_bytes);
		if (overflows)
			return 0;

		/* Requires the complete palette before pixel data and file end. */
		palette_end = layout->palette_offset + palette_bytes;
		if (palette_end > data_offset || palette_end > size)
			return 0;

		/* Completes indexed-image layout selection. */
		break;
	case 24:
		/* Configures a tightly represented RGB image. */
		layout->format = IMAGE_RGB24;
		bytes_per_pixel = 3;

		/* Completes RGB-image layout selection. */
		break;
	default:
		/* Rejects every unsupported BMP pixel depth. */
		return 0;
	}

	/* Computes the padded source-row stride without overflow. */
	overflows = multiply_overflows(layout->width, layout->bits_per_pixel);
	if (overflows)
		return 0;

	row_bits = (size_t)layout->width * layout->bits_per_pixel;
	overflows = add_overflows(row_bits, 31U);
	if (overflows)
		return 0;

	layout->source_stride = ((row_bits + 31U) / 32U) * 4U;

	/* Computes the target-independent output-row stride. */
	overflows = multiply_overflows(layout->width, bytes_per_pixel);
	if (overflows)
		return 0;

	layout->output_stride = (size_t)layout->width * bytes_per_pixel;

	/* Validates the complete source pixel-array size. */
	overflows = multiply_overflows(layout->source_stride, layout->height);
	if (overflows)
		return 0;

	/* Validates the complete decoded pixel-array size. */
	overflows = multiply_overflows(layout->output_stride, layout->height);
	if (overflows)
		return 0;

	/* Publishes the validated source and output byte counts. */
	source_bytes = layout->source_stride * layout->height;
	layout->output_size = layout->output_stride * layout->height;

	/* Requires the complete source pixel array within the encoded file. */
	overflows = add_overflows(data_offset, source_bytes);
	if (overflows)
		return 0;

	/* Requires the complete source pixel data within the file. */
	if (data_offset + source_bytes > size)
		return 0;

	/* Reports a complete supported BMP layout. */
	return 1;
}

/* Measures the decoded representation of one supported BMP image. */
static int
bmp_measure(
	const void *data,
	size_t size,
	enum image_format *format,
	unsigned *width,
	unsigned *height,
	size_t *pixel_bytes)
{
	struct bmp_layout layout;
	int parsed;

	/* Requires every measurement result destination. */
	if (format == NULL ||
	    width == NULL ||
	    height == NULL ||
	    pixel_bytes == NULL) {
		return 0;
	}

	/* Parses and validates the complete encoded BMP layout. */
	parsed = parse_layout(data, size, &layout);
	if (!parsed)
		return 0;

	/* Publishes the decoded image representation and dimensions. */
	*format = layout.format;
	*width = layout.width;
	*height = layout.height;
	*pixel_bytes = layout.output_size;

	/* Reports a successfully measured BMP. */
	return 1;
}

/* Decodes one supported BMP into caller-owned pixel storage. */
static int
bmp_decode(
	const void *data,
	size_t size,
	void *pixel_storage,
	size_t pixel_capacity,
	struct image *image)
{
	struct bmp_layout layout;
	uint8_t *output;
	const uint8_t *entry;
	const uint8_t *source;
	uint8_t *destination;
	unsigned source_y;
	unsigned x;
	unsigned y;
	size_t pixel_offset;
	int parsed;

	/* Views the caller-owned decoded pixel storage. */
	output = pixel_storage;

	/* Requires output storage and an image description destination. */
	if (output == NULL || image == NULL)
		return 0;

	/* Parses the encoded BMP before consulting its measured capacity. */
	parsed = parse_layout(data, size, &layout);
	if (!parsed)
		return 0;

	/* Requires enough caller-owned storage for every decoded pixel. */
	if (pixel_capacity < layout.output_size)
		return 0;

	/* Initializes the decoded image description. */
	memset(image, 0, sizeof(*image));
	image->format = layout.format;
	image->width = layout.width;
	image->height = layout.height;
	image->stride = layout.output_stride;
	image->pixels = output;
	image->palette_size = layout.palette_size;

	/* Converts every BGRA palette entry to 0x00RRGGBB. */
	for (y = 0; y < layout.palette_size; y++) {
		entry = layout.bytes + layout.palette_offset + (size_t)y * 4U;
		image->palette[y] = (uint32_t)entry[2] << 16 |
			(uint32_t)entry[1] << 8 |
			entry[0];
	}

	/* Decodes each logical output row in top-to-bottom order. */
	for (y = 0; y < layout.height; y++) {
		/* Maps the logical row to the BMP storage direction. */
		if (layout.top_down)
			source_y = y;
		else
			source_y = layout.height - 1U - y;

		source = layout.bytes + layout.data_offset +
			(size_t)source_y * layout.source_stride;
		destination = output + (size_t)y * layout.output_stride;

		/* Expands the source row according to its encoded pixel depth. */
		if (layout.bits_per_pixel == 1U) {
			/* Expands every most-significant-bit-first palette index. */
			for (x = 0; x < layout.width; x++) {
				destination[x] = (uint8_t)(
					(source[x >> 3] >> (7U - (x & 7U))) & 1U);
			}
		} else if (layout.bits_per_pixel == 4U) {
			/* Expands every high-then-low palette-index nibble. */
			for (x = 0; x < layout.width; x++) {
				destination[x] = (uint8_t)(
					(source[x >> 1] >>
					 ((x & 1U) ? 0U : 4U)) &
					0x0fU);
			}
		} else if (layout.bits_per_pixel == 8U) {
			/* Copies the already byte-sized palette indices. */
			memcpy(destination, source, layout.width);
		} else {
			/* Converts each BGR source pixel to packed RGB order. */
			for (x = 0; x < layout.width; x++) {
				pixel_offset = (size_t)x * 3U;
				destination[pixel_offset] =
					source[pixel_offset + 2U];
				destination[pixel_offset + 1U] =
					source[pixel_offset + 1U];
				destination[pixel_offset + 2U] =
					source[pixel_offset];
			}
		}
	}

	/* Reports a complete decoded image. */
	return 1;
}

/* Returns one integer through a temporary GC root. */
static bool
return_int(
	NoctEnv *env,
	int value)
{
	NoctValue result;
	bool pinned;
	bool returned;

	/* Initializes the result before it becomes a GC root. */
	memset(&result, 0, sizeof(result));

	/* Pins the return value during integer construction. */
	pinned = noct_pin_local(env, 1, &result);
	if (!pinned)
		return false;

	/* Constructs and publishes the requested integer. */
	returned = noct_set_return_make_int(env, &result, value);

	/* Releases the temporary return root. */
	(void)noct_unpin_local(env, 1, &result);

	/* Reports whether the integer was returned. */
	return returned;
}

/* Reads one integer argument while accepting int and long values. */
static bool
get_int_arg(
	NoctEnv *env,
	uint32_t index,
	int *result)
{
	NoctValue value;
	int64_t long_value;
	int int_value;
	bool converted;
	bool pinned;

	/* Initializes the argument before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the argument during numeric conversion. */
	pinned = noct_pin_local(env, 1, &value);
	if (!pinned)
		return false;

	/* Tries the long representation first. */
	converted = noct_get_arg_check_long(env, index, &value, &long_value);

	/* Falls back to the common integer representation. */
	if (!converted) {
		converted = noct_get_arg_check_int(env, index, &value, &int_value);

		/* Promotes a converted integer to the shared representation. */
		if (converted)
			long_value = int_value;
	}

	/* Publishes a successfully converted argument. */
	if (converted)
		*result = (int)long_value;

	/* Releases the converted argument root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports whether the argument was converted. */
	return converted;
}

/* Implements BeUI.init(). */
static bool
cfunc_BeUI_init(
	NoctEnv *env)
{
	int initialized;
	bool returned;

	/* Opens BeUI and normalizes its status. */
	initialized = beui_init() ? 1 : 0;

	/* Returns the normalized open status. */
	returned = return_int(env, initialized);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.initWithHint(). */
static bool
cfunc_BeUI_initWithHint(
	NoctEnv *env)
{
	int bits_per_pixel;
	int initialized;
	bool converted;
	bool returned;

	/* Reads the requested pixel-depth hint. */
	converted = get_int_arg(env, 0, &bits_per_pixel);
	if (!converted ||
	    (bits_per_pixel != 8 && bits_per_pixel != 24)) {
		noct_error(env, N_TR("BeUI.initWithHint expects 8 or 24 bits per pixel."));
		return false;
	}

	/* Opens BeUI with the validated pixel-depth hint. */
	initialized = beui_init_with_hint((unsigned)bits_per_pixel) ? 1 : 0;

	/* Returns the normalized open status. */
	returned = return_int(env, initialized);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.close(). */
static bool
cfunc_BeUI_close(
	NoctEnv *env)
{
	bool returned;

	/* Releases the active SDL2 BeUI services. */
	beui_close();

	/* Returns a successful close status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.isOpen(). */
static bool
cfunc_BeUI_isOpen(
	NoctEnv *env)
{
	int open;
	bool returned;

	/* Reads and normalizes the display ownership state. */
	open = beui_is_open() ? 1 : 0;

	/* Returns the normalized display state. */
	returned = return_int(env, open);

	/* Reports whether the state was returned. */
	return returned;
}

/* Implements BeUI.getWidth(). */
static bool
cfunc_BeUI_getWidth(
	NoctEnv *env)
{
	struct display_info info;
	int available;
	bool returned;

	/* Reads the active display description. */
	available = beui_get_display_info(&info);
	if (!available) {
		noct_error(env, N_TR("BeUI is not open."));
		return false;
	}

	/* Returns the active display width. */
	returned = return_int(env, (int)info.width);

	/* Reports whether the width was returned. */
	return returned;
}

/* Implements BeUI.getHeight(). */
static bool
cfunc_BeUI_getHeight(
	NoctEnv *env)
{
	struct display_info info;
	int available;
	bool returned;

	/* Reads the active display description. */
	available = beui_get_display_info(&info);
	if (!available) {
		noct_error(env, N_TR("BeUI is not open."));
		return false;
	}

	/* Returns the active display height. */
	returned = return_int(env, (int)info.height);

	/* Reports whether the height was returned. */
	return returned;
}

/* Implements BeUI.poll(). */
static bool
cfunc_BeUI_poll(
	NoctEnv *env)
{
	int alive;
	bool returned;

	/* Services SDL2 and normalizes the display state. */
	alive = beui_poll() ? 1 : 0;

	/* Returns the normalized display state. */
	returned = return_int(env, alive);

	/* Reports whether the state was returned. */
	return returned;
}

/* Implements BeUI.flush(). */
static bool
cfunc_BeUI_flush(
	NoctEnv *env)
{
	int flushed;
	bool returned;

	/* Flushes pending display output. */
	flushed = beui_flush();
	if (!flushed) {
		noct_error(env, N_TR("BeUI.flush failed."));
		return false;
	}

	/* Returns a successful flush status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.fill(). */
static bool
cfunc_BeUI_fill(
	NoctEnv *env)
{
	struct rect rectangle;
	int x;
	int y;
	int width;
	int height;
	int color;
	int filled;
	bool valid;
	bool returned;

	/* Reads the horizontal rectangle coordinate. */
	valid = get_int_arg(env, 0, &x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Reads the vertical rectangle coordinate. */
	valid = get_int_arg(env, 1, &y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Reads the rectangle width. */
	valid = get_int_arg(env, 2, &width);
	if (!valid) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Reads the rectangle height. */
	valid = get_int_arg(env, 3, &height);
	if (!valid) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Reads the solid fill color. */
	valid = get_int_arg(env, 4, &color);
	if (!valid) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Validates the complete rectangle and 24-bit solid color. */
	if (x < 0 ||
	    y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));
		return false;
	}

	/* Constructs the validated unsigned drawing rectangle. */
	rectangle.x = (unsigned)x;
	rectangle.y = (unsigned)y;
	rectangle.width = (unsigned)width;
	rectangle.height = (unsigned)height;

	/* Fills the requested display rectangle. */
	filled = beui_fill(&rectangle, (uint32_t)color);
	if (!filled) {
		noct_error(env, N_TR("BeUI.fill failed."));
		return false;
	}

	/* Returns a successful fill status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.line(). */
static bool
cfunc_BeUI_line(
	NoctEnv *env)
{
	int x0;
	int y0;
	int x1;
	int y1;
	int color;
	int drawn;
	bool valid;
	bool returned;

	/* Reads the first horizontal endpoint. */
	valid = get_int_arg(env, 0, &x0);
	if (!valid) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Reads the first vertical endpoint. */
	valid = get_int_arg(env, 1, &y0);
	if (!valid) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Reads the second horizontal endpoint. */
	valid = get_int_arg(env, 2, &x1);
	if (!valid) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Reads the second vertical endpoint. */
	valid = get_int_arg(env, 3, &y1);
	if (!valid) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Reads the solid line color. */
	valid = get_int_arg(env, 4, &color);
	if (!valid) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Validates the complete line and 24-bit solid color. */
	if (x0 < 0 ||
	    y0 < 0 ||
	    x1 < 0 ||
	    y1 < 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));
		return false;
	}

	/* Draws the validated line. */
	drawn = beui_line(
		(unsigned)x0,
		(unsigned)y0,
		(unsigned)x1,
		(unsigned)y1,
		(uint32_t)color);
	if (!drawn) {
		noct_error(env, N_TR("BeUI.line failed."));
		return false;
	}

	/* Returns a successful line status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.patternFill(). */
static bool
cfunc_BeUI_patternFill(
	NoctEnv *env)
{
	struct rect rectangle;
	NoctValue value;
	int x;
	int y;
	int width;
	int height;
	int color;
	int int_pattern;
	int filled;
	int64_t pattern;
	bool valid;
	bool converted;
	bool pinned;
	bool returned;

	/* Reads the horizontal rectangle coordinate. */
	valid = get_int_arg(env, 0, &x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Reads the vertical rectangle coordinate. */
	valid = get_int_arg(env, 1, &y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Reads the rectangle width. */
	valid = get_int_arg(env, 2, &width);
	if (!valid) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Reads the rectangle height. */
	valid = get_int_arg(env, 3, &height);
	if (!valid) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Reads the patterned-fill color. */
	valid = get_int_arg(env, 4, &color);
	if (!valid) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Validates the complete rectangle and 24-bit solid color. */
	if (x < 0 ||
	    y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Initializes the pattern argument before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the pattern during numeric conversion. */
	pinned = noct_pin_local(env, 1, &value);
	if (!pinned)
		return false;

	/* Reads the pattern as long and then as int. */
	converted = noct_get_arg_check_long(env, 5, &value, &pattern);
	if (!converted) {
		converted = noct_get_arg_check_int(env, 5, &value, &int_pattern);

		/* Promotes a converted integer pattern without sign extension. */
		if (converted)
			pattern = (int64_t)(uint32_t)int_pattern;
	}

	/* Releases the converted pattern root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports an invalid pattern through the existing API error. */
	if (!converted) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));
		return false;
	}

	/* Constructs the validated unsigned drawing rectangle. */
	rectangle.x = (unsigned)x;
	rectangle.y = (unsigned)y;
	rectangle.width = (unsigned)width;
	rectangle.height = (unsigned)height;

	/* Fills the requested rectangle through the supplied pattern. */
	filled = beui_pattern_fill(&rectangle, (uint32_t)color, (uint64_t)pattern);
	if (!filled) {
		noct_error(env, N_TR("BeUI.patternFill failed."));
		return false;
	}

	/* Returns a successful patterned-fill status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Measures the first string argument while preserving its GC root. */
static bool
measure_text_arg(
	NoctEnv *env,
	const char *api,
	unsigned *width,
	unsigned *height)
{
	NoctValue value;
	const char *text;
	bool read;
	bool measured;
	bool pinned;

	/* Initializes the string before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the string while its borrowed bytes are measured. */
	pinned = noct_pin_local(env, 1, &value);
	if (!pinned)
		return false;

	/* Reads the borrowed UTF-8 string. */
	read = noct_get_arg_check_string(env, 0, &value, &text);
	if (!read) {
		noct_error(env, N_TR("%s failed."), api);
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Measures the string while its owner remains pinned. */
	measured = beui_measure_text(text, width, height);
	if (!measured) {
		noct_error(env, N_TR("%s failed."), api);
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Releases the measured string root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports a successful text measurement. */
	return true;
}

/* Implements BeUI.textWidth(). */
static bool
cfunc_BeUI_textWidth(
	NoctEnv *env)
{
	unsigned width;
	unsigned height;
	bool measured;
	bool returned;

	/* Measures the first string argument. */
	measured = measure_text_arg(
		env,
		"BeUI.textWidth",
		&width,
		&height);
	if (!measured)
		return false;

	/* Returns the measured text width. */
	returned = return_int(env, (int)width);

	/* Reports whether the width was returned. */
	return returned;
}

/* Implements BeUI.textHeight(). */
static bool
cfunc_BeUI_textHeight(
	NoctEnv *env)
{
	unsigned width;
	unsigned height;
	bool measured;
	bool returned;

	/* Measures the first string argument. */
	measured = measure_text_arg(env, "BeUI.textHeight", &width, &height);
	if (!measured)
		return false;

	/* Returns the measured text height. */
	returned = return_int(env, (int)height);

	/* Reports whether the height was returned. */
	return returned;
}

/* Implements BeUI.drawText(). */
static bool
cfunc_BeUI_drawText(
	NoctEnv *env)
{
	NoctValue value;
	const char *text;
	int x;
	int y;
	int foreground;
	int background;
	int drawn;
	bool valid;
	bool read;
	bool pinned;
	bool returned;

	/* Reads the horizontal text coordinate. */
	valid = get_int_arg(env, 1, &x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));
		return false;
	}

	/* Reads the vertical text coordinate. */
	valid = get_int_arg(env, 2, &y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));
		return false;
	}

	/* Reads the foreground color. */
	valid = get_int_arg(env, 3, &foreground);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));
		return false;
	}

	/* Reads the background color. */
	valid = get_int_arg(env, 4, &background);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));
		return false;
	}

	/* Validates both coordinates and 24-bit colors. */
	if (x < 0 ||
	    y < 0 ||
	    foreground < 0 ||
	    foreground > 0xffffff ||
	    background < 0 ||
	    background > 0xffffff) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));
		return false;
	}

	/* Initializes the text before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the string while the drawing code consumes its borrowed bytes. */
	pinned = noct_pin_local(env, 1, &value);
	if (!pinned)
		return false;

	/* Reads the borrowed UTF-8 string. */
	read = noct_get_arg_check_string(env, 0, &value, &text);
	if (!read) {
		noct_error(env, N_TR("BeUI.drawText failed."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Draws the text while its owner remains pinned. */
	drawn = beui_draw_text(text, (unsigned)x, (unsigned)y, (uint32_t)foreground, (uint32_t)background);
	if (!drawn) {
		noct_error(env, N_TR("BeUI.drawText failed."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Returns success while the borrowed string remains rooted. */
	returned = return_int(env, 1);

	/* Releases the consumed string root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.getMilliseconds(). */
static bool
cfunc_BeUI_getMilliseconds(
	NoctEnv *env)
{
	uint64_t milliseconds;
	int available;
	bool returned;

	/* Reads the SDL2 clock. */
	available = beui_get_milliseconds(&milliseconds);
	if (!available) {
		noct_error(env, N_TR("BeUI.getMilliseconds is unavailable."));
		return false;
	}

	/* Returns the historical positive 31-bit clock value. */
	returned = return_int(env, (int)(milliseconds & 0x7fffffffu));

	/* Reports whether the clock value was returned. */
	return returned;
}

/* Implements BeUI.sleep(). */
static bool
cfunc_BeUI_sleep(
	NoctEnv *env)
{
	int milliseconds;
	int slept;
	bool converted;
	bool returned;

	/* Reads and validates the bounded sleep interval. */
	converted = get_int_arg(env, 0, &milliseconds);
	if (!converted || milliseconds < 0 || milliseconds > 3600000) {
		noct_error(env, N_TR("BeUI.sleep received an invalid argument."));
		return false;
	}

	/* Sleeps through the SDL2 clock. */
	slept = beui_sleep((unsigned)milliseconds);
	if (!slept) {
		noct_error(env, N_TR("BeUI.sleep is unavailable."));
		return false;
	}

	/* Returns a successful sleep status. */
	returned = return_int(env, 1);

	/* Reports whether the status was returned. */
	return returned;
}

/* Implements BeUI.isKeyDown(). */
static bool
cfunc_BeUI_isKeyDown(
	NoctEnv *env)
{
	int key;
	int down;
	bool converted;
	bool returned;

	/* Reads and validates the requested key code. */
	converted = get_int_arg(env, 0, &key);
	if (!converted || key < 0) {
		noct_error(env, N_TR("BeUI.isKeyDown received an invalid argument."));
		return false;
	}

	/* Treats keys that the target cannot sense as released. */
	down = beui_is_key_down(key) == 1;

	/* Returns the normalized real-time key state. */
	returned = return_int(env, down);

	/* Reports whether the key state was returned. */
	return returned;
}

/* Reads one selected field from the active pointer state. */
static bool
pointer_field(
	NoctEnv *env,
	const char *api,
	unsigned *x,
	unsigned *y,
	unsigned *buttons)
{
	int available;

	/* Copies the requested pointer state field. */
	available = beui_get_pointer(x, y, buttons);
	if (!available) {
		noct_error(env, N_TR("%s is unavailable."), api);
		return false;
	}

	/* Reports an available pointer state. */
	return true;
}

/* Implements BeUI.getPointerX(). */
static bool
cfunc_BeUI_getPointerX(
	NoctEnv *env)
{
	unsigned x;
	bool available;
	bool returned;

	/* Reads the horizontal pointer coordinate. */
	available = pointer_field(env,
				  "BeUI.getPointerX",
				  &x,
				  NULL,
				  NULL);
	if (!available)
		return false;

	/* Returns the horizontal pointer coordinate. */
	returned = return_int(env, (int)x);

	/* Reports whether the coordinate was returned. */
	return returned;
}

/* Implements BeUI.getPointerY(). */
static bool
cfunc_BeUI_getPointerY(
	NoctEnv *env)
{
	unsigned y;
	bool available;
	bool returned;

	/* Reads the vertical pointer coordinate. */
	available = pointer_field(env,
				  "BeUI.getPointerY",
				  NULL,
				  &y,
				  NULL);
	if (!available)
		return false;

	/* Returns the vertical pointer coordinate. */
	returned = return_int(env, (int)y);

	/* Reports whether the coordinate was returned. */
	return returned;
}

/* Implements BeUI.getPointerButtons(). */
static bool
cfunc_BeUI_getPointerButtons(
	NoctEnv *env)
{
	unsigned buttons;
	bool available;
	bool returned;

	/* Reads the pointer button bitmask. */
	available = pointer_field(env,
				  "BeUI.getPointerButtons",
				  NULL,
				  NULL,
				  &buttons);
	if (!available)
		return false;

	/* Returns the pointer button bitmask. */
	returned = return_int(env, (int)buttons);

	/* Reports whether the button state was returned. */
	return returned;
}

/*
 * BeUI.loadImage takes the file contents rather than a path: BeUI draws
 * and the File API reads, so the graphical layer needs no filesystem of
 * its own and behaves identically on every host.
 */
static bool
cfunc_BeUI_loadImage(
	NoctEnv *env)
{
	NoctValue value;
	void *data;
	size_t size;
	int handle;
	bool valid;
	bool returned;

	/* Roots the packed argument while its storage is inspected. */
	memset(&value, 0, sizeof(value));
	valid = noct_pin_local(env, 1, &value);
	if (!valid)
		return false;

	/* Reads a packed byte-array argument. */
	valid = noct_get_arg_check_packed(env,
					  0,
					  &value,
					  NOCT_PACKED_UINT8);
	if (!valid) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Reads the byte-array length. */
	valid = noct_get_packed_size(env, &value, &size);
	if (!valid) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Reads the byte-array storage. */
	valid = noct_get_packed_pointer(env, &value, &data);
	if (!valid) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Decodes and retains the BMP image. */
	handle = image_load_bmp(data, size);
	if (handle == 0) {
		noct_error(env, N_TR("BeUI.loadImage received an unsupported image."));
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Returns the image handle while its source remains rooted. */
	returned = return_int(env, handle);

	/* Releases the packed argument root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports whether the handle was returned. */
	return returned;
}

/* Implements BeUI.getImageWidth(). */
static bool
cfunc_BeUI_getImageWidth(
	NoctEnv *env)
{
	const struct image *image;
	int handle;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(
			env,
			N_TR("BeUI.getImageWidth received an invalid handle."));
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env,
			   N_TR("BeUI.getImageWidth received an invalid handle."));
		return false;
	}

	/* Returns the image width. */
	returned = return_int(env, (int)image->width);

	/* Reports whether the width was returned. */
	return returned;
}

/* Implements BeUI.getImageHeight(). */
static bool
cfunc_BeUI_getImageHeight(
	NoctEnv *env)
{
	const struct image *image;
	int handle;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(
			env,
			N_TR("BeUI.getImageHeight received an invalid handle."));
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env,
			   N_TR("BeUI.getImageHeight received an invalid handle."));
		return false;
	}

	/* Returns the image height. */
	returned = return_int(env, (int)image->height);

	/* Reports whether the height was returned. */
	return returned;
}

/* Implements BeUI.drawImage(). */
static bool
cfunc_BeUI_drawImage(
	NoctEnv *env)
{
	const struct image *image;
	int handle;
	int x;
	int y;
	int drawn;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Reads the horizontal destination coordinate. */
	valid = get_int_arg(env, 1, &x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Reads the vertical destination coordinate. */
	valid = get_int_arg(env, 2, &y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Rejects destinations outside the unsigned coordinate space. */
	if (x < 0 || y < 0) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Draws the image at the requested destination. */
	drawn = beui_draw_image((unsigned)x, (unsigned)y, image);
	if (!drawn) {
		noct_error(env, N_TR("BeUI.drawImage failed."));
		return false;
	}

	/* Returns a successful drawing result. */
	returned = return_int(env, 1);

	/* Reports whether the result was returned. */
	return returned;
}

/* Implements BeUI.drawImageRegion(). */
static bool
cfunc_BeUI_drawImageRegion(
	NoctEnv *env)
{
	const struct image *image;
	int handle;
	int source_x;
	int source_y;
	int width;
	int height;
	int x;
	int y;
	int drawn;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the horizontal source coordinate. */
	valid = get_int_arg(env, 1, &source_x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the vertical source coordinate. */
	valid = get_int_arg(env, 2, &source_y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the source width. */
	valid = get_int_arg(env, 3, &width);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the source height. */
	valid = get_int_arg(env, 4, &height);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the horizontal destination coordinate. */
	valid = get_int_arg(env, 5, &x);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Reads the vertical destination coordinate. */
	valid = get_int_arg(env, 6, &y);
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Validates the source and destination geometry. */
	if (source_x < 0 ||
	    source_y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    x < 0 ||
	    y < 0) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Draws the selected source region. */
	drawn = beui_draw_image_region(image,
				       (unsigned)source_x,
				       (unsigned)source_y,
				       (unsigned)width,
				       (unsigned)height,
				       (unsigned)x,
				       (unsigned)y);
	if (!drawn) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));
		return false;
	}

	/* Returns a successful drawing result. */
	returned = return_int(env, 1);

	/* Reports whether the result was returned. */
	return returned;
}

/* Implements BeUI.drawImagePattern(). */
static bool
cfunc_BeUI_drawImagePattern(
	NoctEnv *env)
{
	const struct image *image;
	NoctValue value;
	int handle;
	int x;
	int y;
	int int_pattern;
	int drawn;
	int64_t pattern;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(env,
			   N_TR("BeUI.drawImagePattern received an invalid argument."));
		return false;
	}

	/* Reads the horizontal destination coordinate. */
	valid = get_int_arg(env, 1, &x);
	if (!valid) {
		noct_error(env,
			   N_TR("BeUI.drawImagePattern received an invalid argument."));
		return false;
	}

	/* Reads the vertical destination coordinate. */
	valid = get_int_arg(env, 2, &y);
	if (!valid) {
		noct_error(env,
			   N_TR("BeUI.drawImagePattern received an invalid argument."));
		return false;
	}

	/* Rejects destinations outside the unsigned coordinate space. */
	if (x < 0 || y < 0) {
		noct_error(env,
			   N_TR("BeUI.drawImagePattern received an invalid argument."));
		return false;
	}

	/* Roots the numeric pattern argument during conversion. */
	memset(&value, 0, sizeof(value));
	valid = noct_pin_local(env, 1, &value);
	if (!valid)
		return false;

	/* Reads a long pattern, with an integer compatibility fallback. */
	valid = noct_get_arg_check_long(env, 3, &value, &pattern);
	if (!valid) {
		valid = noct_get_arg_check_int(env, 3, &value, &int_pattern);

		/* Promotes a converted integer pattern without sign extension. */
		if (valid)
			pattern = (int64_t)(uint32_t)int_pattern;
	}

	/* Releases the numeric argument root after conversion. */
	(void)noct_unpin_local(env, 1, &value);

	/* Rejects a pattern that could not be converted. */
	if (!valid) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));
		return false;
	}

	/* Draws the image through the requested pattern. */
	drawn = beui_draw_image_pattern((unsigned)x,
					(unsigned)y,
					image,
					(uint64_t)pattern);
	if (!drawn) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));
		return false;
	}

	/* Returns a successful drawing result. */
	returned = return_int(env, 1);

	/* Reports whether the result was returned. */
	return returned;
}

/* Implements BeUI.destroyImage(). */
static bool
cfunc_BeUI_destroyImage(
	NoctEnv *env)
{
	int handle;
	int destroyed;
	bool valid;
	bool returned;

	/* Reads the image handle. */
	valid = get_int_arg(env, 0, &handle);
	if (!valid) {
		noct_error(env, N_TR("BeUI.destroyImage received an invalid handle."));
		return false;
	}

	/* Destroys the selected image. */
	destroyed = image_destroy(handle);
	if (!destroyed) {
		noct_error(env, N_TR("BeUI.destroyImage received an invalid handle."));
		return false;
	}

	/* Returns a successful destruction result. */
	returned = return_int(env, 1);

	/* Reports whether the result was returned. */
	return returned;
}

/* Publishes one dictionary of integer constants. */
static bool
register_int_dictionary(
	NoctEnv *env,
	const char *name,
	const struct int_constant *entries,
	size_t count)
{
	NoctValue dictionary;
	NoctValue scratch;
	size_t index;
	bool registered;

	/* Roots the dictionary and temporary value during publication. */
	memset(&dictionary, 0, sizeof(dictionary));
	memset(&scratch, 0, sizeof(scratch));
	registered = noct_pin_local(env, 2, &dictionary, &scratch);
	if (!registered)
		return false;

	/* Creates the constant dictionary. */
	registered = noct_make_empty_dict(env, &dictionary);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &dictionary, &scratch);
		return false;
	}

	/* Adds every integer constant in declaration order. */
	for (index = 0; index < count; index++) {
		registered = noct_set_dict_elem_make_int(env,
							 &dictionary,
							 entries[index].name,
							 &scratch,
							 entries[index].value);
		if (!registered) {
			(void)noct_unpin_local(
				env,
				2,
				&dictionary,
				&scratch);
			return false;
		}
	}

	/* Publishes the completed constant dictionary. */
	registered = noct_set_global(env, name, &dictionary);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &dictionary, &scratch);
		return false;
	}

	/* Releases the temporary GC roots. */
	(void)noct_unpin_local(env, 2, &dictionary, &scratch);

	/* Reports a published dictionary. */
	return true;
}

/* Registers the BeUI functions and public constants. */
static bool
register_beui_api(
	NoctEnv *env)
{
	static const struct ffi_item ffi_items[] = {
		{"BeUI.init", "init", 0, {NULL}, cfunc_BeUI_init},
		{"BeUI.initWithHint", "initWithHint", 1, {"bitsPerPixel"}, cfunc_BeUI_initWithHint},
		{"BeUI.close", "close", 0, {NULL}, cfunc_BeUI_close},
		{"BeUI.isOpen", "isOpen", 0, {NULL}, cfunc_BeUI_isOpen},
		{"BeUI.getWidth", "getWidth", 0, {NULL}, cfunc_BeUI_getWidth},
		{"BeUI.getHeight", "getHeight", 0, {NULL}, cfunc_BeUI_getHeight},
		{"BeUI.poll", "poll", 0, {NULL}, cfunc_BeUI_poll},
		{"BeUI.flush", "flush", 0, {NULL}, cfunc_BeUI_flush},
		{"BeUI.fill", "fill", 5, {"x", "y", "width", "height", "color"}, cfunc_BeUI_fill},
		{"BeUI.line", "line", 5, {"x0", "y0", "x1", "y1", "color"}, cfunc_BeUI_line},
		{"BeUI.patternFill", "patternFill", 6, {"x", "y", "width", "height", "color", "pattern"}, cfunc_BeUI_patternFill},
		{"BeUI.textWidth", "textWidth", 1, {"text"}, cfunc_BeUI_textWidth},
		{"BeUI.textHeight", "textHeight", 1, {"text"}, cfunc_BeUI_textHeight},
		{"BeUI.drawText", "drawText", 5, {"text", "x", "y", "foreground", "background"}, cfunc_BeUI_drawText},
		{"BeUI.getMilliseconds", "getMilliseconds", 0, {NULL}, cfunc_BeUI_getMilliseconds},
		{"BeUI.sleep", "sleep", 1, {"milliseconds"}, cfunc_BeUI_sleep},
		{"BeUI.isKeyDown", "isKeyDown", 1, {"key"}, cfunc_BeUI_isKeyDown},
		{"BeUI.getPointerX", "getPointerX", 0, {NULL}, cfunc_BeUI_getPointerX},
		{"BeUI.getPointerY", "getPointerY", 0, {NULL}, cfunc_BeUI_getPointerY},
		{"BeUI.getPointerButtons", "getPointerButtons", 0, {NULL}, cfunc_BeUI_getPointerButtons},
		{"BeUI.loadImage", "loadImage", 1, {"bytes"}, cfunc_BeUI_loadImage},
		{"BeUI.getImageWidth", "getImageWidth", 1, {"image"}, cfunc_BeUI_getImageWidth},
		{"BeUI.getImageHeight", "getImageHeight", 1, {"image"}, cfunc_BeUI_getImageHeight},
		{"BeUI.drawImage", "drawImage", 3, {"image", "x", "y"}, cfunc_BeUI_drawImage},
		{"BeUI.drawImageRegion", "drawImageRegion", 7, {"image", "sourceX", "sourceY", "width", "height", "x", "y"}, cfunc_BeUI_drawImageRegion},
		{"BeUI.drawImagePattern", "drawImagePattern", 4, {"image", "x", "y", "pattern"}, cfunc_BeUI_drawImagePattern},
		{"BeUI.destroyImage", "destroyImage", 1, {"image"}, cfunc_BeUI_destroyImage},
	};

	NoctValue beui_dict;
	NoctValue function;
	const struct ffi_item *item;
	size_t index;
	size_t item_count;
	size_t key_count;
	size_t button_count;
	bool registered;

	/* Roots the BeUI dictionary and one function during publication. */
	memset(&beui_dict, 0, sizeof(beui_dict));
	memset(&function, 0, sizeof(function));
	registered = noct_pin_local(env, 2, &beui_dict, &function);
	if (!registered)
		return false;

	/* Creates the public BeUI dictionary. */
	registered = noct_make_empty_dict(env, &beui_dict);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);
		return false;
	}

	/* Publishes the empty dictionary before filling its fields. */
	registered = noct_set_global(env, "BeUI", &beui_dict);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);
		return false;
	}

	/* Registers and publishes every BeUI function in API order. */
	item_count = sizeof(ffi_items) / sizeof(ffi_items[0]);
	for (index = 0; index < item_count; index++) {
		item = &ffi_items[index];
		registered = noct_register_cfunc(env,
						 item->global_name,
						 item->param_count,
						 (const char **)item->param,
						 item->cfunc,
						 NULL);
		if (!registered) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);
			return false;
		}

		/* Reads the registered function value. */
		registered = noct_get_global(env,
					     item->global_name,
					     &function);
		if (!registered) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);
			return false;
		}

		/* Adds the registered function to the public dictionary. */
		registered = noct_set_dict_elem_cstr(env,
						     &beui_dict,
						     item->field_name,
						     &function);
		if (!registered) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);
			return false;
		}
	}

	/* Publishes the key-code dictionary. */
	key_count = sizeof(key_constants) / sizeof(key_constants[0]);
	registered = register_int_dictionary(env,
					     "Key",
					     key_constants,
					     key_count);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);
		return false;
	}

	/* Publishes the pointer-button dictionary. */
	button_count = sizeof(button_constants) / sizeof(button_constants[0]);
	registered = register_int_dictionary(env,
					     "Button",
					     button_constants,
					     button_count);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);
		return false;
	}

	/* Releases the temporary GC roots. */
	(void)noct_unpin_local(env, 2, &beui_dict, &function);

	/* Reports a complete BeUI API registration. */
	return true;
}

/* Converts a BeUI color to an opaque SDL framebuffer pixel. */
static uint32_t
sdl2_color(
	uint32_t color)
{
	uint32_t pixel;

	/* Adds the opaque alpha channel to the BeUI RGB value. */
	pixel = 0xff000000U | (color & 0x00ffffffU);

	/* Returns the SDL framebuffer pixel. */
	return pixel;
}

/* Tests one destination pixel against an eight-row drawing pattern. */
static int
sdl2_pattern_bit(
	uint64_t pattern,
	unsigned x,
	unsigned y)
{
	uint8_t row;
	int selected;

	/* Selects the pattern row and destination column. */
	row = (uint8_t)(pattern >> ((y & 7U) * 8U));
	selected = (row & (uint8_t)(0x80U >> (x & 7U))) != 0;

	/* Reports whether the pattern selects the pixel. */
	return selected;
}

/* Writes one converted color to the software framebuffer. */
static void
sdl2_put_pixel(
	struct sdl2_context *context,
	unsigned x,
	unsigned y,
	uint32_t color)
{
	uint32_t *row;
	uint32_t pixel;

	/* Locates the destination framebuffer row. */
	row = (uint32_t *)((uint8_t *)context->framebuffer->pixels +
			   y * (unsigned)context->framebuffer->pitch);

	/* Converts and stores the requested color. */
	pixel = sdl2_color(color);
	row[x] = pixel;
}

/* Locks the software framebuffer when SDL requires locking. */
static int
sdl2_lock_framebuffer(
	struct sdl2_context *context)
{
	int status;

	/* Rejects a missing framebuffer. */
	if (context == NULL || context->framebuffer == NULL)
		return 0;

	/* Accepts a framebuffer that SDL exposes without locking. */
	if (!SDL_MUSTLOCK(context->framebuffer))
		return 1;

	/* Locks a framebuffer that SDL protects. */
	status = SDL_LockSurface(context->framebuffer);
	if (status != 0)
		return 0;

	/* Reports an accessible framebuffer. */
	return 1;
}

/* Unlocks the software framebuffer when SDL requires locking. */
static void
sdl2_unlock_framebuffer(
	struct sdl2_context *context)
{
	/* Releases an SDL-managed framebuffer lock. */
	if (SDL_MUSTLOCK(context->framebuffer))
		SDL_UnlockSurface(context->framebuffer);
}

/* Opens the SDL2 display and its software framebuffer. */
static int
sdl2_enter(
	struct sdl2_context *context,
	struct display_info *info)
{
	int status;
	uint32_t black;

	/* Validates the SDL2 display state. */
	if (context == NULL || info == NULL)
		return 0;

	/* Initializes the SDL video and event subsystems once. */
	if (!context->video_initialized) {
		SDL_SetMainReady();

		/* Starts both SDL subsystems required by the display. */
		status = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		if (status != 0)
			return 0;
		context->video_initialized = 1;
	}

	/* Creates the host window. */
	context->window = SDL_CreateWindow("Noct BeUI",
					   SDL_WINDOWPOS_CENTERED,
					   SDL_WINDOWPOS_CENTERED,
					   (int)DISPLAY_WIDTH,
					   (int)DISPLAY_HEIGHT,
					   SDL_WINDOW_SHOWN);
	if (context->window == NULL)
		return 0;

	/* Creates the fixed-format software framebuffer. */
	context->framebuffer = SDL_CreateRGBSurfaceWithFormat(0,
							      (int)DISPLAY_WIDTH,
							      (int)DISPLAY_HEIGHT,
							      32,
							      SDL_PIXELFORMAT_ARGB8888);
	if (context->framebuffer == NULL) {
		SDL_DestroyWindow(context->window);
		context->window = NULL;
		return 0;
	}

	/* Clears the new framebuffer to opaque black. */
	black = sdl2_color(0);
	(void)SDL_FillRect(context->framebuffer, NULL, black);

	/* Publishes the active fixed display geometry. */
	context->alive = 1;
	info->width = DISPLAY_WIDTH;
	info->height = DISPLAY_HEIGHT;
	info->bits_per_pixel = 32;
	info->stride = (unsigned)context->framebuffer->pitch;

	/* Reports an open display. */
	return 1;
}

/* Closes every SDL2 display resource. */
static void
sdl2_leave(
	struct sdl2_context *context)
{
	/* Ignores a missing display context. */
	if (context == NULL)
		return;

	/* Releases the software framebuffer. */
	if (context->framebuffer != NULL) {
		SDL_FreeSurface(context->framebuffer);
		context->framebuffer = NULL;
	}

	/* Releases the host window. */
	if (context->window != NULL) {
		SDL_DestroyWindow(context->window);
		context->window = NULL;
	}

	/* Marks the display as closed. */
	context->alive = 0;
}

/* Services pending SDL2 window events. */
static int
sdl2_poll_events(
	struct sdl2_context *context)
{
	SDL_Event event;
	uint32_t window_id;
	int available;

	/* Validates the active window. */
	if (context == NULL || context->window == NULL)
		return 0;

	/* Processes every pending SDL event. */
	available = SDL_PollEvent(&event);
	while (available) {
		/* Recognizes application and current-window close requests. */
		if (event.type == SDL_QUIT) {
			context->alive = 0;
		} else if (event.type == SDL_WINDOWEVENT) {
			/* Recognizes a close request for the current window. */
			window_id = SDL_GetWindowID(context->window);
			if (event.window.windowID == window_id &&
			    event.window.event == SDL_WINDOWEVENT_CLOSE)
				context->alive = 0;
		}
		available = SDL_PollEvent(&event);
	}

	/* Reports whether the host window remains alive. */
	return context->alive;
}

/* Fills one framebuffer rectangle with a solid color. */
static int
sdl2_fill(
	struct sdl2_context *context,
	const struct rect *rect,
	uint32_t color)
{
	SDL_Rect destination;
	uint32_t pixel;
	int status;

	/* Validates the fill destination. */
	if (context == NULL || context->framebuffer == NULL || rect == NULL)
		return 0;

	/* Converts the BeUI rectangle to SDL coordinates. */
	destination.x = (int)rect->x;
	destination.y = (int)rect->y;
	destination.w = (int)rect->width;
	destination.h = (int)rect->height;

	/* Fills the destination rectangle. */
	pixel = sdl2_color(color);
	status = SDL_FillRect(context->framebuffer, &destination, pixel);
	if (status != 0)
		return 0;

	/* Reports a completed fill. */
	return 1;
}

/* Draws one Bresenham line into the software framebuffer. */
static int
sdl2_line(
	struct sdl2_context *context,
	unsigned x0,
	unsigned y0,
	unsigned x1,
	unsigned y1,
	uint32_t color)
{
	int x;
	int y;
	int target_x;
	int target_y;
	int delta_x;
	int delta_y;
	int step_x;
	int step_y;
	int error;
	int twice_error;
	int locked;

	/* Locks the destination framebuffer. */
	locked = sdl2_lock_framebuffer(context);
	if (!locked)
		return 0;

	/* Initializes the integer Bresenham traversal. */
	x = (int)x0;
	y = (int)y0;
	target_x = (int)x1;
	target_y = (int)y1;
	delta_x = target_x >= x ? target_x - x : x - target_x;
	step_x = x < target_x ? 1 : -1;
	delta_y = target_y >= y ? y - target_y : target_y - y;
	step_y = y < target_y ? 1 : -1;
	error = delta_x + delta_y;

	/* Draws every pixel through the requested endpoint. */
	for (;;) {
		sdl2_put_pixel(context, (unsigned)x, (unsigned)y, color);

		/* Stops after drawing the requested endpoint. */
		if (x == target_x && y == target_y)
			break;

		/* Advances horizontally when the error crosses the edge. */
		twice_error = error * 2;
		if (twice_error >= delta_y) {
			error += delta_y;
			x += step_x;
		}

		/* Advances vertically when the error crosses the edge. */
		if (twice_error <= delta_x) {
			error += delta_x;
			y += step_y;
		}
	}

	/* Releases the destination framebuffer. */
	sdl2_unlock_framebuffer(context);

	/* Reports a completed line. */
	return 1;
}

/* Fills one framebuffer rectangle through an eight-row pattern. */
static int
sdl2_pattern_fill(
	struct sdl2_context *context,
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	unsigned x;
	unsigned y;
	int selected;
	int locked;

	/* Rejects a missing destination rectangle. */
	if (rect == NULL)
		return 0;

	/* Locks the destination framebuffer. */
	locked = sdl2_lock_framebuffer(context);
	if (!locked)
		return 0;

	/* Draws every pattern-selected pixel in the rectangle. */
	for (y = rect->y; y < rect->y + rect->height; y++) {
		/* Draws every selected pixel in this pattern row. */
		for (x = rect->x; x < rect->x + rect->width; x++) {
			/* Tests the current destination against the pattern. */
			selected = sdl2_pattern_bit(pattern, x, y);
			if (selected)
				sdl2_put_pixel(context, x, y, color);
		}
	}

	/* Releases the destination framebuffer. */
	sdl2_unlock_framebuffer(context);

	/* Reports a completed patterned fill. */
	return 1;
}

/* Reads one target-independent image pixel. */
static uint32_t
sdl2_image_pixel(
	const struct image *image,
	unsigned x,
	unsigned y)
{
	const uint8_t *pixel;
	uint32_t color;

	/* Locates the source image row. */
	pixel = image->pixels + y * image->stride;

	/* Resolves an indexed source pixel through its palette. */
	if (image->format == IMAGE_INDEX8) {
		color = image->palette[pixel[x]];

		/* Returns the palette-resolved BeUI color. */
		return color;
	}

	/* Decodes one tightly packed RGB source pixel. */
	pixel += x * 3U;
	color = ((uint32_t)pixel[0] << 16) |
		((uint32_t)pixel[1] << 8) |
		pixel[2];

	/* Returns the decoded BeUI color. */
	return color;
}

/* Draws an image with an optional destination-space pattern. */
static int
sdl2_draw_image_common(
	struct sdl2_context *context,
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern,
	int patterned)
{
	unsigned source_x;
	unsigned source_y;
	uint32_t color;
	int selected;
	int locked;

	/* Rejects a missing source image. */
	if (image == NULL || image->pixels == NULL)
		return 0;

	/* Locks the destination framebuffer. */
	locked = sdl2_lock_framebuffer(context);
	if (!locked)
		return 0;

	/* Copies every source row to the destination. */
	for (source_y = 0; source_y < image->height; source_y++) {
		/* Copies every selected source pixel in this row. */
		for (source_x = 0; source_x < image->width; source_x++) {
			/* Tests the optional pattern at the destination pixel. */
			selected = !patterned;
			if (!selected) {
				selected = sdl2_pattern_bit(
					pattern,
					x + source_x,
					y + source_y);
			}

			/* Copies a pixel selected by the optional pattern. */
			if (selected) {
				color = sdl2_image_pixel(image, source_x, source_y);
				sdl2_put_pixel(context,
					       x + source_x,
					       y + source_y,
					       color);
			}
		}
	}

	/* Releases the destination framebuffer. */
	sdl2_unlock_framebuffer(context);

	/* Reports a completed image draw. */
	return 1;
}

/* Draws an unpatterned image. */
static int
sdl2_draw_image(
	struct sdl2_context *context,
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int drawn;

	/* Draws the complete source image. */
	drawn = sdl2_draw_image_common(context, x, y, image, 0, 0);

	/* Reports whether the image was drawn. */
	return drawn;
}

/* Draws an image through a destination-space pattern. */
static int
sdl2_draw_image_pattern(
	struct sdl2_context *context,
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int drawn;

	/* Draws the source image through the requested pattern. */
	drawn = sdl2_draw_image_common(context, x, y, image, pattern, 1);

	/* Reports whether the image was drawn. */
	return drawn;
}

/* Copies the software framebuffer to the host window. */
static int
sdl2_flush(
	struct sdl2_context *context,
	const struct rect *rectangles,
	size_t rectangle_count)
{
	SDL_Surface *window_surface;
	int status;

	UNUSED_PARAMETER(rectangles);
	UNUSED_PARAMETER(rectangle_count);

	/* Validates the active display surfaces. */
	if (context == NULL || context->window == NULL ||
	    context->framebuffer == NULL)
		return 0;

	/* Resolves the window-owned presentation surface. */
	window_surface = SDL_GetWindowSurface(context->window);
	if (window_surface == NULL)
		return 0;

	/* Copies the software framebuffer into the presentation surface. */
	status = SDL_BlitSurface(context->framebuffer,
				 NULL,
				 window_surface,
				 NULL);
	if (status != 0)
		return 0;

	/* Presents the updated window surface. */
	status = SDL_UpdateWindowSurface(context->window);
	if (status != 0)
		return 0;

	/* Reports a completed presentation. */
	return 1;
}

/* Builds one compact 5x7 desktop glyph. */
static void
sdl2_ascii_glyph(
	uint32_t codepoint,
	uint8_t rows[7])
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static const uint8_t glyphs[][7] = {
		{14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
		{14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
		{31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
		{14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
		{14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
		{17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
		{17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
		{14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
		{14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
		{15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
		{17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
		{17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
		{17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
		{14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
		{14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
		{2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
		{14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
		{14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
	};
	const char *found;

	/* Starts with a blank glyph. */
	memset(rows, 0, 7);

	/* Shares uppercase shapes with lowercase letters. */
	if (codepoint >= 'a' && codepoint <= 'z')
		codepoint -= 'a' - 'A';

	/* Copies a table-driven alphanumeric glyph. */
	found = strchr(alphabet, (int)codepoint);
	if (found != NULL) {
		memcpy(rows, glyphs[found - alphabet], 7);

		/* Leaves the completed table glyph in the caller's rows. */
		return;
	}

	/* Builds punctuation and the fallback glyph explicitly. */
	switch (codepoint) {
	case '.':
		/* Draws a period. */
		rows[6] = 4;
		break;
	case ',':
		/* Draws a comma. */
		rows[5] = 4;
		rows[6] = 8;
		break;
	case ':':
		/* Draws a colon. */
		rows[2] = 4;
		rows[5] = 4;
		break;
	case ';':
		/* Draws a semicolon. */
		rows[2] = 4;
		rows[5] = 4;
		rows[6] = 8;
		break;
	case '-':
		/* Draws a hyphen. */
		rows[3] = 31;
		break;
	case '_':
		/* Draws an underscore. */
		rows[6] = 31;
		break;
	case '+':
		/* Draws a plus sign. */
		rows[2] = 4;
		rows[3] = 31;
		rows[4] = 4;
		break;
	case '/':
		/* Draws a forward slash. */
		rows[0] = 1;
		rows[1] = 2;
		rows[2] = 2;
		rows[3] = 4;
		rows[4] = 8;
		rows[5] = 8;
		rows[6] = 16;
		break;
	case '\\':
		/* Draws a backslash. */
		rows[0] = 16;
		rows[1] = 8;
		rows[2] = 8;
		rows[3] = 4;
		rows[4] = 2;
		rows[5] = 2;
		rows[6] = 1;
		break;
	case '!':
		/* Draws an exclamation mark. */
		rows[0] = 4;
		rows[1] = 4;
		rows[2] = 4;
		rows[3] = 4;
		rows[5] = 4;
		break;
	case '?':
		/* Draws a question mark. */
		rows[0] = 14;
		rows[1] = 17;
		rows[2] = 1;
		rows[3] = 2;
		rows[4] = 4;
		rows[6] = 4;
		break;
	case '(':
		/* Draws a left parenthesis. */
		rows[0] = 2;
		rows[1] = 4;
		rows[2] = 8;
		rows[3] = 8;
		rows[4] = 8;
		rows[5] = 4;
		rows[6] = 2;
		break;
	case ')':
		/* Draws a right parenthesis. */
		rows[0] = 8;
		rows[1] = 4;
		rows[2] = 2;
		rows[3] = 2;
		rows[4] = 2;
		rows[5] = 4;
		rows[6] = 8;
		break;
	case '[':
		/* Draws a left bracket. */
		rows[0] = 14;
		rows[1] = 8;
		rows[2] = 8;
		rows[3] = 8;
		rows[4] = 8;
		rows[5] = 8;
		rows[6] = 14;
		break;
	case ']':
		/* Draws a right bracket. */
		rows[0] = 14;
		rows[1] = 2;
		rows[2] = 2;
		rows[3] = 2;
		rows[4] = 2;
		rows[5] = 2;
		rows[6] = 14;
		break;
	case '=':
		/* Draws an equals sign. */
		rows[2] = 31;
		rows[4] = 31;
		break;
	case '*':
		/* Draws an asterisk. */
		rows[1] = 21;
		rows[2] = 14;
		rows[3] = 31;
		rows[4] = 14;
		rows[5] = 21;
		break;
	case ' ':
		/* Leaves a space blank. */
		break;
	default:
		/* Draws a visible question-mark fallback. */
		rows[0] = 14;
		rows[1] = 17;
		rows[2] = 1;
		rows[3] = 2;
		rows[4] = 4;
		rows[6] = 4;
		break;
	}
}

/* Measures one built-in glyph cell. */
static int
sdl2_glyph_measure(
	uint32_t codepoint,
	unsigned *width,
	unsigned *height)
{
	/* Rejects missing dimension outputs. */
	if (width == NULL || height == NULL)
		return 0;

	/* Selects an ASCII or fallback glyph cell. */
	*width = codepoint < 0x80U ? 8U : 16U;
	*height = 16U;

	/* Reports valid glyph dimensions. */
	return 1;
}

/* Draws one built-in glyph into the software framebuffer. */
static int
sdl2_glyph_draw(
	struct sdl2_context *context,
	unsigned x,
	unsigned y,
	uint32_t codepoint,
	uint32_t foreground,
	uint32_t background)
{
	struct rect rect;
	uint8_t rows[7];
	unsigned width;
	unsigned row;
	unsigned column;
	int filled;
	int locked;

	/* Validates the destination framebuffer. */
	if (context == NULL || context->framebuffer == NULL)
		return 0;

	/* Initializes the glyph-cell background rectangle. */
	width = codepoint < 0x80U ? 8U : 16U;
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = 16;

	/* Clears the complete glyph cell. */
	filled = sdl2_fill(context, &rect, background);
	if (!filled)
		return 0;

	/* Locks the destination framebuffer. */
	locked = sdl2_lock_framebuffer(context);
	if (!locked)
		return 0;

	/* Draws a visible fallback box for non-ASCII codepoints. */
	if (codepoint >= 0x80U) {
		/* Draws the horizontal fallback-box edges. */
		for (column = 1; column + 1 < width; column++) {
			sdl2_put_pixel(context, x + column, y + 1, foreground);
			sdl2_put_pixel(context, x + column, y + 14, foreground);
		}

		/* Draws the vertical fallback-box edges. */
		for (row = 1; row < 15; row++) {
			sdl2_put_pixel(context, x + 1, y + row, foreground);
			sdl2_put_pixel(context, x + width - 2, y + row, foreground);
		}

		/* Releases the completed fallback glyph. */
		sdl2_unlock_framebuffer(context);

		/* Reports a completed fallback glyph. */
		return 1;
	}

	/* Builds the compact ASCII bitmap. */
	sdl2_ascii_glyph(codepoint, rows);

	/* Expands every compact bitmap row to the glyph cell. */
	for (row = 0; row < 7; row++) {
		/* Doubles every selected bitmap pixel vertically. */
		for (column = 0; column < 5; column++) {
			/* Draws one selected compact-font pixel. */
			if ((rows[row] & (uint8_t)(16U >> column)) != 0) {
				sdl2_put_pixel(
					context,
					x + column + 1,
					y + row * 2U + 1U,
					foreground);
				sdl2_put_pixel(
					context,
					x + column + 1,
					y + row * 2U + 2U,
					foreground);
			}
		}
	}

	/* Releases the completed ASCII glyph. */
	sdl2_unlock_framebuffer(context);

	/* Reports a completed ASCII glyph. */
	return 1;
}

/* Starts pointer input and centers the host cursor. */
static int
sdl2_pointer_start(
	struct sdl2_context *context,
	const struct display_info *display)
{
	int x;
	int y;

	/* Validates the active display. */
	if (context == NULL || context->window == NULL || display == NULL)
		return 0;

	/* Centers the pointer in the active host window. */
	x = (int)(display->width / 2U);
	y = (int)(display->height / 2U);
	SDL_WarpMouseInWindow(context->window, x, y);

	/* Reports an active pointer. */
	return 1;
}

/* Reads the current absolute host pointer state. */
static int
sdl2_pointer_poll(
	struct sdl2_context *context,
	struct pointer_event *event)
{
	int x;
	int y;
	uint32_t buttons;

	/* Validates the pointer destination. */
	if (context == NULL || context->window == NULL || event == NULL)
		return -1;

	/* Reads and normalizes the current SDL pointer state. */
	buttons = SDL_GetMouseState(&x, &y);
	event->x = x < 0 ? 0U : (unsigned)x;
	event->y = y < 0 ? 0U : (unsigned)y;
	event->buttons = 0;

	/* Publishes the primary-button state. */
	if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0)
		event->buttons |= BUTTON_LEFT;

	/* Publishes the secondary-button state. */
	if ((buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0)
		event->buttons |= BUTTON_RIGHT;

	/* Publishes the middle-button state. */
	if ((buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0)
		event->buttons |= BUTTON_MIDDLE;

	/* Reports an available pointer state. */
	return 1;
}

/* Reads the SDL2 monotonic clock in milliseconds. */
static uint64_t
sdl2_milliseconds(
	struct sdl2_context *context)
{
	uint64_t counter;
	uint64_t frequency;
	uint64_t milliseconds;
	int status;

	/* Initializes the SDL timer subsystem once when context is available. */
	if (context != NULL && !context->timer_initialized) {
		SDL_SetMainReady();

		/* Starts the SDL timer subsystem. */
		status = SDL_InitSubSystem(SDL_INIT_TIMER);
		if (status == 0)
			context->timer_initialized = 1;
	}

	/* Samples the SDL performance counter and its frequency. */
	counter = SDL_GetPerformanceCounter();
	frequency = SDL_GetPerformanceFrequency();

	/* Rejects an unavailable performance frequency. */
	if (frequency == 0)
		return 0;

	/* Converts the whole and fractional seconds to milliseconds. */
	milliseconds = counter / frequency * 1000U +
		(counter % frequency) * 1000U / frequency;

	/* Returns the current monotonic time. */
	return milliseconds;
}

/* Maps one BeUI key code to an SDL scancode. */
static SDL_Scancode
sdl2_key_scancode(
	int key)
{
	SDL_Scancode scancode;

	/* Maps every named BeUI key to its SDL counterpart. */
	switch (key) {
	case KEY_ESCAPE:
		/* Maps Escape. */
		return SDL_SCANCODE_ESCAPE;
	case KEY_BACKSPACE:
		/* Maps Backspace. */
		return SDL_SCANCODE_BACKSPACE;
	case KEY_TAB:
		/* Maps Tab. */
		return SDL_SCANCODE_TAB;
	case KEY_ENTER:
		/* Maps Enter. */
		return SDL_SCANCODE_RETURN;
	case KEY_PAGE_UP:
		/* Maps Page Up. */
		return SDL_SCANCODE_PAGEUP;
	case KEY_PAGE_DOWN:
		/* Maps Page Down. */
		return SDL_SCANCODE_PAGEDOWN;
	case KEY_INSERT:
		/* Maps Insert. */
		return SDL_SCANCODE_INSERT;
	case KEY_DELETE:
		/* Maps Delete. */
		return SDL_SCANCODE_DELETE;
	case KEY_UP:
		/* Maps Up. */
		return SDL_SCANCODE_UP;
	case KEY_LEFT:
		/* Maps Left. */
		return SDL_SCANCODE_LEFT;
	case KEY_RIGHT:
		/* Maps Right. */
		return SDL_SCANCODE_RIGHT;
	case KEY_DOWN:
		/* Maps Down. */
		return SDL_SCANCODE_DOWN;
	case KEY_HOME:
		/* Maps Home. */
		return SDL_SCANCODE_HOME;
	case KEY_END:
		/* Maps End. */
		return SDL_SCANCODE_END;
	case KEY_F1:
		/* Maps F1. */
		return SDL_SCANCODE_F1;
	case KEY_F2:
		/* Maps F2. */
		return SDL_SCANCODE_F2;
	case KEY_F3:
		/* Maps F3. */
		return SDL_SCANCODE_F3;
	case KEY_F4:
		/* Maps F4. */
		return SDL_SCANCODE_F4;
	case KEY_F5:
		/* Maps F5. */
		return SDL_SCANCODE_F5;
	case KEY_F6:
		/* Maps F6. */
		return SDL_SCANCODE_F6;
	case KEY_F7:
		/* Maps F7. */
		return SDL_SCANCODE_F7;
	case KEY_F8:
		/* Maps F8. */
		return SDL_SCANCODE_F8;
	case KEY_F9:
		/* Maps F9. */
		return SDL_SCANCODE_F9;
	case KEY_F10:
		/* Maps F10. */
		return SDL_SCANCODE_F10;
	default:
		/* Maps a printable ASCII key through SDL. */
		if (key >= 0x20 && key < 0x7f) {
			scancode = SDL_GetScancodeFromKey((SDL_Keycode)key);

			/* Returns the SDL mapping for the printable key. */
			return scancode;
		}

		/* Reports an unsupported BeUI key. */
		return SDL_SCANCODE_UNKNOWN;
	}
}

/* Reports whether one BeUI key is currently held. */
static int
sdl2_is_key_down(
	struct sdl2_context *context,
	int key)
{
	const uint8_t *keyboard;
	SDL_Scancode scancode;
	int down;

	/* Validates the active host window. */
	if (context == NULL || context->window == NULL)
		return -1;

	/* Refreshes and reads the current SDL keyboard state. */
	SDL_PumpEvents();
	keyboard = SDL_GetKeyboardState(NULL);

	/* Combines the two host Shift keys into one BeUI modifier. */
	if (key == KEY_SHIFT) {
		down = keyboard[SDL_SCANCODE_LSHIFT] ||
			keyboard[SDL_SCANCODE_RSHIFT];

		/* Reports whether either host Shift key is held. */
		return down;
	}

	/* Maps the requested BeUI key to SDL. */
	scancode = sdl2_key_scancode(key);

	/* Rejects a key that this host cannot sense. */
	if (scancode == SDL_SCANCODE_UNKNOWN)
		return -1;

	/* Reads the mapped host key state. */
	down = keyboard[scancode] != 0;

	/* Reports whether the key is held. */
	return down;
}

/* Drains queued SDL2 keyboard events. */
static void
sdl2_drain_input(
	struct sdl2_context *context)
{
	/* Ignores input before the SDL video subsystem starts. */
	if (context == NULL || !context->video_initialized)
		return;

	/* Refreshes and discards buffered key transitions. */
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
}
