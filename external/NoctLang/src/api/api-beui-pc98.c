/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * NEC PC-9800 BeUI implementation.
 */

#include <noct/noct.h>
#include "jisx0208.h"

#include <conio.h>
#include <i86.h>

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define GDC_PLANE_BYTES		(640U * 400U / 8U)
#define GDC_WIDTH		640U
#define GDC_HEIGHT		400U
#define GDC_STRIDE		(GDC_WIDTH / 8U)

#define CIRRUS_WIDTH		640U
#define CIRRUS_HEIGHT		480U
#define CIRRUS_STRIDE_8		CIRRUS_WIDTH
#define CIRRUS_STRIDE_24	(CIRRUS_WIDTH * 3U)
#define CIRRUS_VISIBLE_BYTES	(CIRRUS_STRIDE_24 * CIRRUS_HEIGHT)
#define CIRRUS_SLEEP		0x0ca3U
#define CIRRUS_IO		0x0ca0U
#define CIRRUS_CRTC		0x0da4U
#define CIRRUS_CRTC_MONO	0x0ba4U
#define CIRRUS_STATUS		0x0daaU
#define CIRRUS_PHYS_APERTURE	0xf0000000UL	/* The Core-Graph aperture the Cirrus backend expects at board level. */

#define WAB_INDEX		0x0faaU
#define WAB_DATA		0x0fabU
#define WAB_REG_ID		0x00U
#define WAB_REG_WINDOW		0x01U
#define WAB_REG_LINEAR		0x02U
#define WAB_REG_RELAY		0x03U
#define WAB_RELAY_SETUP		0x01U
#define WAB_RELAY_WAB		0x03U

#define PC98_WAIT		0x005fU
#define PC98_GDC_MODE		0x0068U
#define PC98_VRAM_SWITCH	0x006aU

#define IMAGE_SOURCE_MAX	(2U * 1024U * 1024U)
#define IMAGE_PIXELS_MAX	(2U * 1024U * 1024U)

/* i8253 system timer. */
#define PIT_COUNTER1		0x73U
#define PIT_CONTROL		0x77U
#define PIT_PROGRAM_CH1		0x74U	/* Channel 1, low/high byte access, mode 2, binary. */
#define PIT_LATCH_CH1		0x40U
#define PIT_LATCH_CH0		0x00U
#define PIT_COUNTER0		0x71U

#define TICKS_PER_5MS_2457600HZ	12288U
#define TICKS_PER_5MS_1996800HZ	9984U

/* BIOS work area. */
#define BIOS_SYSTEM_CLOCK_FLAG	0x501U
#define BIOS_KEY_STATE_TABLE	0x52aU


enum image_format {
	IMAGE_INDEX8 = 1,
	IMAGE_RGB24 = 2
};

enum pointer_button {
	BUTTON_LEFT = 1U << 0,
	BUTTON_RIGHT = 1U << 1,
	BUTTON_MIDDLE = 1U << 2
};

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
	KEY_SHIFT = 0x170	/* State-only: modifiers never appear in buffered key streams. */
};

enum display_kind {
	DISPLAY_NONE,
	DISPLAY_CIRRUS,
	DISPLAY_GDC
};

struct rect {
	unsigned x;
	unsigned y;
	unsigned width;
	unsigned height;
};

struct display_info {
	unsigned preferred_bits_per_pixel;
	unsigned width;
	unsigned height;
	unsigned bits_per_pixel;
	unsigned stride;
};

struct image {
	enum image_format format;
	unsigned width;
	unsigned height;
	size_t stride;
	const uint8_t *pixels;
	uint32_t palette[256];
	unsigned palette_size;
};

struct gdc_state {
	volatile uint8_t *planes[4];
};

struct glyph_cache_entry {
	uint16_t jis;
	uint8_t valid;
	uint8_t font[32];
};

struct glyph_state {
	volatile uint8_t *cg_window;
	struct glyph_cache_entry cache[64];
	unsigned cache_next;
};

struct cirrus_state {
	volatile uint8_t *framebuffer;
	uint8_t saved_sleep;
	uint8_t saved_window;
	uint8_t saved_linear;
	uint8_t saved_relay;
	uint8_t bits_per_pixel;
	uint8_t active;
};

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

struct image_entry {
	struct image_entry *next;
	int handle;
	struct image image;
	uint8_t pixels[1];
};

struct beui_state {
	enum display_kind display_kind;
	struct display_info display;
	int display_open;
	struct image_entry *images;
	int next_image_handle;
};

struct int_constant {
	const char *name;
	int value;
};

struct ffi_item {
	const char *global_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};

struct clock_state {
	int initialized;
	int use_channel0;
	uint16_t reload;
	uint16_t last_count;
	uint64_t ticks;
	uint32_t ticks_per_5ms;
};

static struct beui_state state;
static struct gdc_state gdc;
static struct glyph_state glyph;
static struct cirrus_state cirrus;
static struct clock_state clock_state;

static const struct int_constant beui_keys[] = {
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
	{"Shift", KEY_SHIFT},
};

/* Bit values returned by BeUI.getPointerButtons. */
static const struct int_constant beui_buttons[] = {
	{"Left", BUTTON_LEFT},
	{"Right", BUTTON_RIGHT},
	{"Middle", BUTTON_MIDDLE},
};

/* Bitmap decoding. */
static uint16_t read_u16(const uint8_t *bytes);
static uint32_t read_u32(const uint8_t *bytes);
static int32_t read_s32(const uint8_t *bytes);
static int add_overflows(size_t left, size_t right);
static int multiply_overflows(size_t left, size_t right);
static int parse_layout(const void *data, size_t size, struct bmp_layout *layout);
static int bmp_measure(const void *data, size_t size, enum image_format *format, unsigned *width, unsigned *height, size_t *pixel_bytes);
static int bmp_decode(const void *data, size_t size, void *pixel_storage, size_t pixel_capacity, struct image *image);

/* BeUI lifecycle and drawing. */
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

/* Display selection. */
static void backend_initialize(volatile uint8_t *cirrus_framebuffer);
static int display_enter(struct display_info *info);
static void display_leave(void);
static int display_fill(const struct rect *rect, uint32_t color);
static int display_line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int display_pattern_fill(const struct rect *rect, uint32_t color, uint64_t pattern);
static int display_draw_image(unsigned x, unsigned y, const struct image *image);
static int display_draw_image_pattern(unsigned x, unsigned y, const struct image *image, uint64_t pattern);

/* PC-98 glyph rendering. */
static void wait_vsync(void);
static uint16_t unicode_to_jis(uint32_t codepoint);
static unsigned glyph_width(uint16_t jis);
static int read_font(struct glyph_state *glyph, uint16_t jis, uint8_t font[32]);
static int glyph_measure(uint32_t codepoint, unsigned *width, unsigned *height);
static uint64_t font_pattern(const uint8_t font[32], unsigned bytes_per_row, unsigned byte_index, unsigned first_row);
static int glyph_draw(unsigned x, unsigned y, uint32_t codepoint, uint32_t foreground, uint32_t background);

/* PC-98 GDC display. */
static int gdc_command(uint8_t command);
static void gdc_clear_planes(struct gdc_state *gdc);
static int gdc_clear_graphics(struct gdc_state *gdc);
static uint8_t rgb_to_gdc(uint32_t color);
static void gdc_write_pixel(struct gdc_state *gdc, unsigned x, unsigned y, uint8_t color);
static int gdc_pattern_bit(uint64_t pattern, unsigned x, unsigned y);
static int gdc_enter(struct gdc_state *gdc, struct display_info *info);
static void gdc_leave(struct gdc_state *gdc);
static int gdc_fill(struct gdc_state *gdc, const struct rect *rect, uint32_t color);
static int gdc_line(struct gdc_state *gdc, unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int gdc_pattern_fill(struct gdc_state *gdc, const struct rect *rect, uint32_t color, uint64_t pattern);
static int gdc_draw_image_common(struct gdc_state *gdc, unsigned destination_x, unsigned destination_y, const struct image *image, uint64_t pattern);
static int gdc_draw_image(struct gdc_state *gdc, unsigned destination_x, unsigned destination_y, const struct image *image);
static int gdc_draw_image_pattern(struct gdc_state *gdc, unsigned destination_x, unsigned destination_y, const struct image *image, uint64_t pattern);

/* PC-98 Core-Graph/Cirrus display. */
static uint8_t port_in8(uint16_t port);
static void port_out8(uint16_t port, uint8_t value);
static void wab_write(uint8_t index, uint8_t value);
static uint8_t wab_read(uint8_t index);
static void seq_write(uint8_t index, uint8_t value);
static uint8_t seq_read(uint8_t index);
static void gfx_write(uint8_t index, uint8_t value);
static void crtc_write(uint8_t index, uint8_t value);
static uint8_t crtc_read(uint8_t index);
static void hidden_dac_write(uint8_t value);
static void load_rgb332_palette(void);
static int coregraph_id_present(void);
static void coregraph_gate_enter(void);
static void coregraph_gate_leave(void);
static void coregraph_mode_640x480(unsigned bits_per_pixel);
static uint8_t rgb332(uint32_t color);
static int cirrus_pattern_bit(uint64_t pattern, unsigned x, unsigned y);
static void cirrus_write_pixel(struct cirrus_state *cirrus, unsigned x, unsigned y, uint32_t color);
static int cirrus_enter(struct cirrus_state *cirrus, struct display_info *info);
static void cirrus_leave(struct cirrus_state *cirrus);
static int cirrus_fill(struct cirrus_state *cirrus, const struct rect *rect, uint32_t color);
static int cirrus_line(struct cirrus_state *cirrus, unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int cirrus_pattern_fill(struct cirrus_state *cirrus, const struct rect *rect, uint32_t color, uint64_t pattern);
static int cirrus_draw_image_common(struct cirrus_state *cirrus, unsigned destination_x, unsigned destination_y, const struct image *image, uint64_t pattern);
static int cirrus_draw_image(struct cirrus_state *cirrus, unsigned x, unsigned y, const struct image *image);
static int cirrus_draw_image_pattern(struct cirrus_state *cirrus, unsigned x, unsigned y, const struct image *image, uint64_t pattern);

/* Native BeUI functions. */
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

/* BeUI registration. */
static bool register_int_dictionary(NoctEnv *env, const char *name, const struct int_constant *entries, size_t count);
static bool register_beui_api(NoctEnv *env);

/* PC-98 platform services. */
static uint8_t read_low_byte(uint32_t address);
static void display_reset(void);
static void display_stop(void);
static uint16_t latch_counter(uint8_t latch_command, uint16_t data_port);
static int counter_is_running(uint8_t latch_command, uint16_t data_port);
static void clock_start(void);
static uint64_t clock_milliseconds(void);
static int key_to_scan(int key);
static int input_is_key_down(int key);
static void input_drain(void);
static volatile uint8_t *map_cirrus_aperture(void);

/*
 * Registers the BeUI API.
 */

NOCT_DLL
bool
noct_register_api_beui(
	NoctEnv *env)
{
	bool call_result;
	volatile uint8_t *cirrus_framebuffer;

	/* Resets the process-wide BeUI state. */
	beui_cleanup();

	/* Maps the optional Cirrus linear framebuffer. */
	cirrus_framebuffer = map_cirrus_aperture();

	/* Initializes both PC-98 display backends. */
	backend_initialize(cirrus_framebuffer);

	/* Registers the native BeUI functions and constants. */
	call_result = register_beui_api(env);

	/* Reports the noct_register_api_beui result. */
	return call_result;
}

/* Implements read_u16(). */
static uint16_t
read_u16(
	const uint8_t *bytes)
{
	/* Reports the read_u16 result. */
	return (uint16_t)(bytes[0] | (uint16_t)bytes[1] << 8);
}

/* Implements read_u32(). */
static uint32_t
read_u32(
	const uint8_t *bytes)
{
	/* Reports the read_u32 result. */
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
	       (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* Implements read_s32(). */
static int32_t
read_s32(
	const uint8_t *bytes)
{
	int32_t call_result;

	/* Reports the read_s32 result. */
	call_result = (int32_t)read_u32(bytes);

	/* Reports the read_s32 result. */
	return call_result;
}

/* Implements add_overflows(). */
static int
add_overflows(
	size_t left,
	size_t right)
{
	/* Reports the add_overflows result. */
	return left > SIZE_MAX - right;
}

/* Implements multiply_overflows(). */
static int
multiply_overflows(
	size_t left,
	size_t right)
{
	/* Treats multiplication by zero as representable. */
	if (left == 0)
		return 0;

	/* Reports whether the nonzero multiplication overflows. */
	return right > SIZE_MAX / left;
}

/* Implements parse_layout(). */
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
	int32_t signed_width;
	int32_t signed_height;
	size_t row_bits;
	size_t palette_end;
	size_t source_bytes;
	unsigned bytes_per_pixel;

	/* Views the bitmap source as addressable bytes. */
	bytes = data;

	/* Rejects incomplete bitmap headers before reading their fields. */
	if (bytes == NULL ||
	    layout == NULL ||
	    size < 54U ||
	    bytes[0] != 'B' ||
	    bytes[1] != 'M')
		return 0;

	/* Reads the fields needed to bound the DIB header. */
	dib_size = read_u32(bytes + 14);
	data_offset = read_u32(bytes + 10);

	/* Rejects unsupported DIB headers before computing their end. */
	if (dib_size < 40U)
		return 0;

	/* Rejects a DIB header whose end cannot be represented. */
	if (add_overflows(14U, dib_size))
		return 0;

	/* Rejects bitmap offsets outside the source buffer. */
	if (14U + dib_size > size || data_offset > size)
		return 0;

	/* Reads the signed bitmap geometry. */
	signed_width = read_s32(bytes + 18);
	signed_height = read_s32(bytes + 22);

	/* Rejects dimensions that cannot describe a supported bitmap. */
	if (signed_width <= 0 ||
	    signed_height == 0 ||
	    signed_height == INT32_MIN)
		return 0;

	/* Requires the single plane used by Windows bitmap files. */
	if (read_u16(bytes + 26) != 1U)
		return 0;

	/* Requires uncompressed source pixels. */
	if (read_u32(bytes + 30) != 0U)
		return 0;

	/* Initializes the target-independent bitmap layout. */
	memset(layout, 0, sizeof(*layout));
	layout->bytes = bytes;
	layout->size = size;
	layout->data_offset = data_offset;
	layout->width = (unsigned)signed_width;
	layout->height = signed_height < 0 ? (unsigned)-signed_height :
		(unsigned)signed_height;
	layout->top_down = signed_height < 0;
	layout->bits_per_pixel = read_u16(bytes + 28);

	/* Selects the matching parse_layout operation. */
	switch (layout->bits_per_pixel) {
	case 1:
	case 4:
	case 8:
		layout->format = IMAGE_INDEX8;
		bytes_per_pixel = 1;
		colors_used = read_u32(bytes + 46);
		layout->palette_size = colors_used != 0 ? colors_used :
			1U << layout->bits_per_pixel;

		/* Handles the next parse_layout decision. */
		if (layout->palette_size == 0 || layout->palette_size > 256U)
			return 0;
		layout->palette_offset = 14U + dib_size;

		/* Rejects a palette byte count that cannot be represented. */
		if (multiply_overflows(layout->palette_size, 4U))
			return 0;

		/* Rejects a palette end that cannot be represented. */
		if (add_overflows(
			layout->palette_offset,
			(size_t)layout->palette_size * 4U))
			return 0;

		/* Computes the validated palette end. */
		palette_end = layout->palette_offset +
			(size_t)layout->palette_size * 4U;

		/* Handles the next parse_layout decision. */
		if (palette_end > data_offset || palette_end > size)
			return 0;
		break;
	case 24:
		layout->format = IMAGE_RGB24;
		bytes_per_pixel = 3;
		break;
	default:
		/* Reports the parse_layout result. */
		return 0;
	}

	/* Handles the next parse_layout decision. */
	if (multiply_overflows(layout->width, layout->bits_per_pixel))
		return 0;
	row_bits = (size_t)layout->width * layout->bits_per_pixel;

	/* Handles the next parse_layout decision. */
	if (add_overflows(row_bits, 31U))
		return 0;
	layout->source_stride = ((row_bits + 31U) / 32U) * 4U;

	/* Handles the next parse_layout decision. */
	if (multiply_overflows(layout->width, bytes_per_pixel))
		return 0;
	layout->output_stride = (size_t)layout->width * bytes_per_pixel;

	/* Rejects a source image byte count that cannot be represented. */
	if (multiply_overflows(layout->source_stride, layout->height))
		return 0;

	/* Rejects an output image byte count that cannot be represented. */
	if (multiply_overflows(layout->output_stride, layout->height))
		return 0;

	/* Computes the validated source and output byte counts. */
	source_bytes = layout->source_stride * layout->height;
	layout->output_size = layout->output_stride * layout->height;

	/* Rejects a source pixel end that cannot be represented. */
	if (add_overflows(data_offset, source_bytes))
		return 0;

	/* Requires every source pixel row to fit in the input. */
	if (data_offset + source_bytes > size)
		return 0;

	/* Reports the parse_layout result. */
	return 1;
}

/* Measures a supported bitmap image. */
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

	/* Requires every bitmap measurement destination. */
	if (format == NULL ||
	    width == NULL ||
	    height == NULL ||
	    pixel_bytes == NULL)
		return 0;

	/* Parses the source bitmap layout. */
	if (!parse_layout(data, size, &layout))
		return 0;

	/* Publishes the measured bitmap layout. */
	*format = layout.format;
	*width = layout.width;
	*height = layout.height;
	*pixel_bytes = layout.output_size;

	/* Reports the bmp_measure result. */
	return 1;
}

/* Decodes a supported bitmap image. */
static int
bmp_decode(
	const void *data,
	size_t size,
	void *pixel_storage,
	size_t pixel_capacity,
	struct image *image)
{
	struct bmp_layout layout;
	const uint8_t *entry;
	const uint8_t *source;
	uint8_t *output;
	uint8_t *destination;
	unsigned source_y;
	unsigned x;
	unsigned y;

	/* Views the caller-provided storage as decoded pixels. */
	output = pixel_storage;

	/* Requires the bitmap output destinations. */
	if (output == NULL || image == NULL)
		return 0;

	/* Parses the source bitmap layout. */
	if (!parse_layout(data, size, &layout))
		return 0;

	/* Requires enough storage for the decoded pixels. */
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

	/* Processes each bmp_decode item. */
	for (y = 0; y < layout.palette_size; y++) {
		entry = layout.bytes + layout.palette_offset + (size_t)y * 4U;
		image->palette[y] = (uint32_t)entry[2] << 16 |
			(uint32_t)entry[1] << 8 | entry[0];
	}

	/* Processes each bmp_decode item. */
	for (y = 0; y < layout.height; y++) {
		source_y = layout.top_down ? y : layout.height - 1U - y;
		source = layout.bytes + layout.data_offset +
			(size_t)source_y * layout.source_stride;
		destination = output + (size_t)y * layout.output_stride;

		/* Handles the next bmp_decode decision. */
		if (layout.bits_per_pixel == 1U) {
			/* Processes each bmp_decode item. */
			for (x = 0; x < layout.width; x++) {
				destination[x] = (uint8_t)(
					(source[x >> 3] >> (7U - (x & 7U))) & 1U);
			}
		} else if (layout.bits_per_pixel == 4U) {
			/* Processes each bmp_decode item. */
			for (x = 0; x < layout.width; x++) {
				destination[x] = (uint8_t)(
					(source[x >> 1] >> ((x & 1U) ? 0U : 4U)) &
					0x0fU);
			}
		} else if (layout.bits_per_pixel == 8U) {
			memcpy(destination, source, layout.width);
		} else {
			/* Processes each bmp_decode item. */
			for (x = 0; x < layout.width; x++) {
				destination[(size_t)x * 3U] = source[(size_t)x * 3U + 2U];
				destination[(size_t)x * 3U + 1U] = source[(size_t)x * 3U + 1U];
				destination[(size_t)x * 3U + 2U] = source[(size_t)x * 3U];
			}
		}
	}

	/* Reports the bmp_decode result. */
	return 1;
}

/* Initializes the default PC-98 display mode. */
static int
beui_init(
	void)
{
	int call_result;

	/* Initializes the display without requesting a color depth. */
	call_result = beui_init_with_hint(0);

	/* Reports whether the display was initialized. */
	return call_result;
}

/* Initializes a PC-98 display with a preferred color depth. */
static int
beui_init_with_hint(
	unsigned preferred_bits_per_pixel)
{
	/* Treats an already open display as initialized. */
	if (state.display_open)
		return 1;

	/* Prepares the requested display description. */
	memset(&state.display, 0, sizeof(state.display));
	state.display.preferred_bits_per_pixel = preferred_bits_per_pixel;

	/* Selects and enters the first usable PC-98 display. */
	if (!display_enter(&state.display))
		return 0;

	/* Publishes the open display state. */
	state.display_open = 1;

	/* Discards type-ahead from the caller's previous screen. */
	beui_drain_input();

	/* Rejects an unusable display mode. */
	if (state.display.width == 0 || state.display.height == 0) {
		beui_close();

		/* Reports the unusable mode. */
		return 0;
	}

	/* Reports a usable display mode. */
	return 1;
}

/* Closes the active PC-98 display. */
static void
beui_close(
	void)
{
	/* Keys held during a session must not leak to the caller. */
	beui_drain_input();

	/* Leaves an open display through its selected implementation. */
	if (state.display_open)
		display_leave();

	/* Clears the published display description. */
	state.display_open = 0;
	memset(&state.display, 0, sizeof(state.display));
}

/* Releases every process-wide BeUI resource. */
static void
beui_cleanup(
	void)
{
	struct image_entry *entry;
	struct image_entry *next;

	/* Closes the display before releasing stored images. */
	entry = state.images;
	beui_close();

	/* Releases every stored image. */
	while (entry != NULL) {
		next = entry->next;
		free(entry);
		entry = next;
	}

	/* Resets the complete BeUI state. */
	memset(&state, 0, sizeof(state));
}

/* Tests whether a PC-98 display is open. */
static int
beui_is_open(
	void)
{
	/* Reports the current lifecycle state. */
	return state.display_open;
}

/* Copies the active PC-98 display description. */
static int
beui_get_display_info(
	struct display_info *info)
{
	/* Requires an open display and a result destination. */
	if (!state.display_open || info == NULL)
		return 0;

	/* Copies the active display description. */
	*info = state.display;

	/* Reports a copied display description. */
	return 1;
}

/* Fills a validated display rectangle. */
static int
beui_fill(
	const struct rect *rect,
	uint32_t color)
{
	int call_result;

	/* Requires a visible rectangle on the active display. */
	if (!state.display_open ||
	    rect == NULL ||
	    rect->width == 0 ||
	    rect->height == 0 ||
	    rect->x >= state.display.width ||
	    rect->y >= state.display.height ||
	    rect->width > state.display.width - rect->x ||
	    rect->height > state.display.height - rect->y)
		return 0;

	/* Fills through the selected PC-98 display. */
	call_result = display_fill(rect, color);

	/* Reports whether the rectangle was filled. */
	return call_result;
}

/* Draws a validated display line. */
static int
beui_line(
	unsigned x0,
	unsigned y0,
	unsigned x1,
	unsigned y1,
	uint32_t color)
{
	int call_result;

	/* Requires visible endpoints supported by the active display. */
	if (!state.display_open ||
	    x0 >= state.display.width ||
	    x1 >= state.display.width ||
	    y0 >= state.display.height ||
	    y1 >= state.display.height)
		return 0;

	/* Draws through the selected PC-98 display. */
	call_result = display_line(x0, y0, x1, y1, color);

	/* Reports whether the line was drawn. */
	return call_result;
}

/* Fills a validated display rectangle through a pattern. */
static int
beui_pattern_fill(
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	int call_result;

	/* Requires a visible rectangle supported by patterned fills. */
	if (!state.display_open ||
	    rect == NULL ||
	    rect->width == 0 ||
	    rect->height == 0 ||
	    rect->x >= state.display.width ||
	    rect->y >= state.display.height ||
	    rect->width > state.display.width - rect->x ||
	    rect->height > state.display.height - rect->y)
		return 0;

	/* Fills through the selected PC-98 display. */
	call_result = display_pattern_fill(rect, color, pattern);

	/* Reports whether the patterned rectangle was filled. */
	return call_result;
}

/* Implements image_valid(). */
static int
image_valid(
	const struct image *image)
{
	/* Rejects incomplete or unsupported image descriptions. */
	if (image == NULL ||
	    image->pixels == NULL ||
	    image->width == 0 ||
	    image->height == 0 ||
	    (image->format != IMAGE_INDEX8 &&
	     image->format != IMAGE_RGB24) ||
	    (image->format == IMAGE_INDEX8 &&
	     (image->palette_size == 0 || image->palette_size > 256)))
		return 0;

	/* Handles the next image_valid decision. */
	if (image->format == IMAGE_RGB24)
		return image->stride / 3U >= image->width;

	/* Reports the image_valid result. */
	return (image->stride >= image->width);
}

/* Draws a validated image. */
static int
beui_draw_image(
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int call_result;

	/* Requires an active display before validating the image. */
	if (!state.display_open)
		return 0;

	/* Validates the source image. */
	if (!image_valid(image))
		return 0;

	/* Requires the complete image to fit the display. */
	if (x >= state.display.width ||
	    y >= state.display.height ||
	    image->width > state.display.width - x ||
	    image->height > state.display.height - y)
		return 0;

	/* Draws through the selected PC-98 display. */
	call_result = display_draw_image(
		x,
		y,
		image);

	/* Reports whether the image was drawn. */
	return call_result;
}

/* Draws a validated source-image region. */
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
	int call_result;
	struct image region;
	size_t pixel_size;
	size_t offset;

	/* Validates the source image before reading its geometry. */
	if (!image_valid(image))
		return 0;

	/* Requires the complete source region to fit the image. */
	if (width == 0 ||
	    height == 0 ||
	    source_x >= image->width ||
	    source_y >= image->height ||
	    width > image->width - source_x ||
	    height > image->height - source_y ||
	    source_y > (size_t)-1 / image->stride)
		return 0;

	pixel_size = image->format == IMAGE_RGB24 ? 3U : 1U;
	offset = (size_t)source_y * image->stride;

	/* Handles the next beui_draw_image_region decision. */
	if (source_x > ((size_t)-1 - offset) / pixel_size)
		return 0;

	offset += (size_t)source_x * pixel_size;
	region = *image;
	region.width = width;
	region.height = height;
	region.pixels += offset;

	/* Draws the selected region. */
	call_result = beui_draw_image(destination_x, destination_y, &region);

	/* Reports whether the region was drawn. */
	return call_result;
}

/* Draws a validated image through a pattern. */
static int
beui_draw_image_pattern(
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int call_result;

	/* Requires an active display before validating the image. */
	if (!state.display_open)
		return 0;

	/* Validates the patterned source image. */
	if (!image_valid(image))
		return 0;

	/* Requires the complete image to fit the display. */
	if (x >= state.display.width ||
	    y >= state.display.height ||
	    image->width > state.display.width - x ||
	    image->height > state.display.height - y)
		return 0;

	/* Draws through the selected PC-98 display. */
	call_result = display_draw_image_pattern(
		x,
		y,
		image,
		pattern);

	/* Reports whether the patterned image was drawn. */
	return call_result;
}

/* Implements decode_utf8(). */
static uint32_t
decode_utf8(
	const char **cursor)
{
	const unsigned char *text;
	uint32_t codepoint;
	unsigned length;
	unsigned index;

	/* Reads the next UTF-8 sequence from the string cursor. */
	text = (const unsigned char *)*cursor;

	/* Handles the next decode_utf8 decision. */
	if (text[0] < 0x80U) {
		(*cursor)++;

		/* Reports the decode_utf8 result. */
		return text[0];
	}

	/* Handles the next decode_utf8 decision. */
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

		/* Reports the decode_utf8 result. */
		return 0xfffdU;
	}

	/* Processes each decode_utf8 item. */
	for (index = 1; index < length; index++) {
		/* Handles the next decode_utf8 decision. */
		if ((text[index] & 0xc0U) != 0x80U) {
			(*cursor)++;

			/* Reports the decode_utf8 result. */
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

		/* Reports the decode_utf8 result. */
		return 0xfffdU;
	}
	*cursor += length;

	/* Reports the decode_utf8 result. */
	return codepoint;
}

/* Measures a UTF-8 text block in PC-98 glyph cells. */
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

	cursor = text;
	line_width = 0;
	maximum_width = 0;
	total_height = 16;

	/* Requires text, result destinations, and an active display. */
	if (!state.display_open ||
	    text == NULL ||
	    width == NULL ||
	    height == NULL)
		return 0;

	/* Continues beui_measure_text processing while work remains. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Handles the next beui_measure_text decision. */
		if (codepoint == '\r')
			continue;

		/* Handles the next beui_measure_text decision. */
		if (codepoint == '\n') {
			/* Handles the next beui_measure_text decision. */
			if (line_width > maximum_width)
				maximum_width = line_width;
			line_width = 0;

			/* Handles the next beui_measure_text decision. */
			if (total_height > (unsigned)-1 - 16U)
				return 0;
			total_height += 16U;
			continue;
		}

		/* Measures the next printable glyph. */
		if (!glyph_measure(codepoint, &glyph_width,&glyph_height))
			return 0;

		/* Rejects unsupported height or an overflowing line width. */
		if (glyph_height > 16U ||
		    line_width > (unsigned)-1 - glyph_width)
			return 0;
		line_width += glyph_width;
	}

	/* Handles the next beui_measure_text decision. */
	if (line_width > maximum_width)
		maximum_width = line_width;
	*width = maximum_width;
	*height = total_height;

	/* Reports a measured text block. */
	return 1;
}

/* Draws a validated UTF-8 text block. */
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

	cursor = text;
	origin_x = x;

	/* Measures the text before validating its destination. */
	if (!beui_measure_text(text, &width, &height))
		return 0;

	/* Requires the complete text block to fit the display. */
	if (x > state.display.width ||
	    y > state.display.height ||
	    width > state.display.width - x ||
	    height > state.display.height - y)
		return 0;

	/* Continues beui_draw_text processing while work remains. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Handles the next beui_draw_text decision. */
		if (codepoint == '\r')
			continue;

		/* Handles the next beui_draw_text decision. */
		if (codepoint == '\n') {
			x = origin_x;
			y += 16U;
			continue;
		}

		/* Measures the next printable glyph. */
		if (!glyph_measure(codepoint, &glyph_width, &glyph_height))
			return 0;

		/* Draws the measured glyph. */
		if (!glyph_draw(x, y, codepoint, foreground, background))
			return 0;

		x += glyph_width;
	}

	/* Reports a drawn text block. */
	return 1;
}

/* Services PC-98 input while the display is open. */
static int
beui_poll(
	void)
{
	/* Requires an open PC-98 display. */
	if (!state.display_open)
		return 0;

	/* Discards buffered DOS keyboard input. */
	beui_drain_input();

	/* Keeps the machine-owned display alive. */
	return 1;
}

/* Reports that PC-98 pointer input is unavailable. */
static int
beui_get_pointer(
	unsigned *x,
	unsigned *y,
	unsigned *buttons)
{
	UNUSED_PARAMETER(x);
	UNUSED_PARAMETER(y);
	UNUSED_PARAMETER(buttons);

	/* Reports the unsupported pointer service. */
	return 0;
}

/* Completes direct-VRAM drawing. */
static int
beui_flush(
	void)
{
	/* Requires an open display. */
	if (!state.display_open)
		return 0;

	/* Reports completed direct-VRAM drawing. */
	return 1;
}

/* Reads the PC-98 millisecond clock. */
static int
beui_get_milliseconds(
	uint64_t *milliseconds)
{
	/* Requires a clock destination. */
	if (milliseconds == NULL)
		return 0;

	/* Reads the direct PC-98 clock. */
	*milliseconds = clock_milliseconds();

	/* Reports a clock sample. */
	return 1;
}

/* Busy-waits for a bounded millisecond interval. */
static int
beui_sleep(
	unsigned milliseconds)
{
	uint64_t start;
	uint64_t now;

	/* Samples the beginning of the interval. */
	if (!beui_get_milliseconds(&start))
		return 0;

	/* Keeps keyboard input drained while the interval elapses. */
	do {
		beui_drain_input();

		/* Samples the elapsed interval. */
		if (!beui_get_milliseconds(&now))
			return 0;
	} while (now - start < milliseconds);

	/* Reports a completed wait. */
	return 1;
}

/* Reads one PC-98 key state. */
static int
beui_is_key_down(
	int key)
{
	int call_result;

	/* Reads the direct BIOS key-state table. */
	call_result = input_is_key_down(key);

	/* Reports the normalized key state. */
	return call_result;
}

/* Discards buffered PC-98 keyboard input. */
static void
beui_drain_input(
	void)
{
	/* Drains the direct DOS keyboard buffer. */
	input_drain();
}

/* Loads one bitmap into the image registry. */
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

	/* Requires a bounded source bitmap. */
	if (data == NULL ||
	    size == 0 ||
	    size > IMAGE_SOURCE_MAX)
		return 0;

	/* Measures the source bitmap. */
	if (!bmp_measure(data, size, &format, &width, &height, &pixel_size))
		return 0;

	/* Requires a bounded decoded pixel buffer. */
	if (pixel_size == 0 || pixel_size > IMAGE_PIXELS_MAX)
		return 0;

	/* Handles the next image_load_bmp decision. */
	entry = malloc(offsetof(struct image_entry, pixels) + pixel_size);
	if (entry == NULL)
		return 0;

	/* Handles the next image_load_bmp decision. */
	if (!bmp_decode(data, size, entry->pixels, pixel_size, &entry->image)) {
		free(entry);

		/* Reports the image_load_bmp result. */
		return 0;
	}

	/* Handles the next image_load_bmp decision. */
	if (state.next_image_handle <= 0)
		state.next_image_handle = 1;
	entry->handle = state.next_image_handle++;
	entry->next = state.images;
	state.images = entry;

	/* Reports the image_load_bmp result. */
	return entry->handle;
}

/* Resolves one image handle. */
static const struct image *
image_get(
	int handle)
{
	struct image_entry *entry;

	/* Handles the next image_get decision. */
	if (handle <= 0)
		return NULL;

	/* Finds the image with the requested handle. */
	for (entry = state.images; entry != NULL; entry = entry->next) {
		/* Handles the next image_get decision. */
		if (entry->handle == handle)
			return &entry->image;
	}

	/* Reports the image_get result. */
	return NULL;
}

/* Destroys one registered image. */
static int
image_destroy(
	int handle)
{
	struct image_entry **link;
	struct image_entry *entry;

	/* Handles the next image_destroy decision. */
	if (handle <= 0)
		return 0;

	/* Processes each image_destroy item. */
	for (link = &state.images; *link != NULL; link = &(*link)->next) {
		entry = *link;

		/* Handles the next image_destroy decision. */
		if (entry->handle != handle)
			continue;
		*link = entry->next;
		free(entry);

		/* Reports the image_destroy result. */
		return 1;
	}

	/* Reports the image_destroy result. */
	return 0;
}

/* Waits for the next PC-98 vertical synchronization interval. */
static void
wait_vsync(
	void)
{
	/* Waits for the current retrace to finish. */
	while (port_in8(0x60) & 0x20U)
		;

	/* Waits for the next retrace to begin. */
	while (!(port_in8(0x60) & 0x20U))
		;
}

/* Converts one Unicode code point to a PC-98 JIS code. */
static uint16_t
unicode_to_jis(
	uint32_t codepoint)
{
	size_t index;
	uint16_t value;
	unsigned row;
	unsigned cell;

	/* Handles the next unicode_to_jis decision. */
	if (codepoint > 0xffffU)
		return 0;
	value = (uint16_t)codepoint;

	/* Handles the next unicode_to_jis decision. */
	if (value < 0x80U)
		return (uint16_t)(0x2000U | value);

	/* Handles the next unicode_to_jis decision. */
	if (value >= 0xff61U && value <= 0xff9fU)
		return (uint16_t)(0x20a1U + (value - 0xff61U));

	/* Processes each unicode_to_jis item. */
	for (index = 0; index < 7896U; index++) {
		/* Handles the next unicode_to_jis decision. */
		if (noct_jisx0208_to_ucs[index] == value) {
			row = (unsigned)(index / 94U) + 0x21U;
			cell = (unsigned)(index % 94U) + 0x21U;

			/* Reports the unicode_to_jis result. */
			return (uint16_t)((row << 8) | cell);
		}
	}

	/* Reports the unicode_to_jis result. */
	return 0;
}

/* Implements glyph_width(). */
static unsigned
glyph_width(
	uint16_t jis)
{
	/* Reports the glyph_width result. */
	return (jis >> 8) == 0x20U ? 8U : 16U;
}

/* Reads one glyph from the PC-98 character generator. */
static int
read_font(
	struct glyph_state *glyph,
	uint16_t jis,
	uint8_t font[32])
{
	uint8_t row;
	uint8_t cell;
	int special;
	unsigned index;

	/* Decodes the JIS row and cell classification. */
	row = (uint8_t)(jis >> 8);
	cell = (uint8_t)jis;
	special = (row >= 0x29U &&
		   row <= 0x2fU) ||
		  (row >= 0x76U &&
		   row <= 0x7fU);

	/* Requires the character-generator window. */
	if (glyph->cg_window == NULL)
		return 0;

	/* Handles the next read_font decision. */
	if (!special) {
		/* Processes each read_font item. */
		for (index = 0; index < 64U; index++) {
			/* Handles the next read_font decision. */
			if (glyph->cache[index].valid &&
			    glyph->cache[index].jis == jis) {
				memcpy(font, glyph->cache[index].font, 32);

				/* Reports the read_font result. */
				return 1;
			}
		}
	}
	memset(font, 0, 32);
	wait_vsync();
	port_out8(0x68, 0x0b);

	/* Handles the next read_font decision. */
	if (row == 0x20U) {
		port_out8(0xa1, 0x00);
		port_out8(0xa3, cell);
		port_out8(0xa5, 0x00);

		/* Processes each read_font item. */
		for (index = 0; index < 16U; index++)
			font[index] = glyph->cg_window[index * 2U + 1U];
	} else if (!special) {
		port_out8(0xa1, cell);
		port_out8(0xa3, (uint8_t)(row - 0x20U));
		port_out8(0xa5, 0x00);

		/* Processes each read_font item. */
		for (index = 0; index < 32U; index++)
			font[index] = glyph->cg_window[index];
	} else {
		port_out8(0xa1, cell);
		port_out8(0xa3, (uint8_t)(row - 0x20U));
		port_out8(0xa5, 0x20);

		/* Processes each read_font item. */
		for (index = 0; index < 16U; index++) {
			font[index * 2U] =
				glyph->cg_window[index * 2U + 1U];
		}
		port_out8(0xa5, 0x00);

		/* Processes each read_font item. */
		for (index = 0; index < 16U; index++) {
			font[index * 2U + 1U] =
				glyph->cg_window[index * 2U + 1U];
		}
	}

	port_out8(0x68, 0x0a);

	/* Handles the next read_font decision. */
	if (!special) {
		index = glyph->cache_next++ % 64U;
		glyph->cache[index].jis = jis;
		glyph->cache[index].valid = 1;
		memcpy(glyph->cache[index].font, font, 32);
	}

	/* Reports the read_font result. */
	return 1;
}

/* Implements glyph_measure(). */
static int
glyph_measure(
	uint32_t codepoint,
	unsigned *width,
	unsigned *height)
{
	uint16_t jis;

	/* Handles the next glyph_measure decision. */
	if (width == NULL || height == NULL)
		return 0;
	jis = unicode_to_jis(codepoint);

	/* Handles the next glyph_measure decision. */
	if (jis == 0)
		jis = unicode_to_jis('?');
	*width = glyph_width(jis);
	*height = 16;

	/* Reports the glyph_measure result. */
	return 1;
}

/* Implements font_pattern(). */
static uint64_t
font_pattern(
	const uint8_t font[32],
	unsigned bytes_per_row,
	unsigned byte_index,
	unsigned first_row)
{
	uint64_t pattern;
	unsigned row;

	/* Starts with an empty eight-row drawing pattern. */
	pattern = 0;

	/* Processes each font_pattern item. */
	for (row = 0; row < 8U; row++) {
		pattern |= (uint64_t)font[(first_row + row) * bytes_per_row +
			byte_index] << (row * 8U);
	}

	/* Reports the font_pattern result. */
	return pattern;
}

/* Draws one PC-98 glyph. */
static int
glyph_draw(
	unsigned x,
	unsigned y,
	uint32_t codepoint,
	uint32_t foreground,
	uint32_t background)
{
	struct rect rectangle;
	uint8_t font[32];
	uint16_t jis;
	uint64_t pattern;
	unsigned width;
	unsigned column;
	unsigned band;

	/* Resolves the requested glyph. */
	jis = unicode_to_jis(codepoint);
	if (jis == 0)
		jis = unicode_to_jis('?');

	width = glyph_width(jis);

	/* Reads the selected PC-98 glyph bitmap. */
	if (!read_font(&glyph, jis, font))
		return 0;

	rectangle.x = x;
	rectangle.y = y;
	rectangle.width = width;
	rectangle.height = 16;

	/* Fills the complete glyph background. */
	if (!display_fill(&rectangle, background))
		return 0;

	/* Processes each glyph_draw item. */
	for (band = 0; band < 2U; band++) {
		/* Processes each glyph_draw item. */
		for (column = 0; column < width / 8U; column++) {
			/* Extracts the next eight-row glyph tile. */
			rectangle.x = x + column * 8U;
			rectangle.y = y + band * 8U;
			rectangle.width = 8;
			rectangle.height = 8;
			pattern = font_pattern(
				font,
				width / 8U,
				column,
				band * 8U);

			/* Draws the extracted glyph tile. */
			if (!display_pattern_fill(
				&rectangle,
				foreground,
				pattern))
				return 0;
		}
	}

	/* Reports the glyph_draw result. */
	return 1;
}

/* Implements gdc_command(). */
static int
gdc_command(
	uint8_t command)
{
	unsigned timeout;

	/* Processes each gdc_command item. */
	for (timeout = 100000U; timeout != 0; timeout--) {
		/* Handles the next gdc_command decision. */
		if (!(port_in8(0x60) & 0x02U))
			break;
	}

	/* Handles the next gdc_command decision. */
	if (timeout == 0)
		return 0;

	port_out8(0x62, command);

	/* Reports the gdc_command result. */
	return 1;
}

/* Clears every planar GDC framebuffer. */
static void
gdc_clear_planes(
	struct gdc_state *gdc)
{
	unsigned plane;
	unsigned offset;

	/* Processes each clear_planes item. */
	for (plane = 0; plane < 4; plane++) {
		/* Processes each clear_planes item. */
		for (offset = 0; offset < GDC_PLANE_BYTES; offset++)
			gdc->planes[plane][offset] = 0;
	}
}

/* Restores and clears the planar GDC graphics apertures. */
static int
gdc_clear_graphics(
	struct gdc_state *gdc)
{
	/* Requires every planar framebuffer. */
	if (gdc == NULL ||
	    gdc->planes[0] == NULL ||
	    gdc->planes[1] == NULL ||
	    gdc->planes[2] == NULL ||
	    gdc->planes[3] == NULL)
		return 0;

	/*
	 * Match the real-mode loader's transition sequence.  In
	 * particular, disable GRCG/EGC interception before touching
	 * all four planar VRAM apertures; firmware and Cirrus may
	 * leave those controls non-default.
	 */
	port_out8(0x7cU, 0x00U);
	port_out8(0x5fU, 0x00U);
	port_out8(0x6aU, 0x07U);
	port_out8(0x6aU, 0x20U);
	port_out8(0x6aU, 0x04U);
	port_out8(0x6aU, 0x06U);
	port_out8(0x6aU, 0x01U);
	port_out8(0x5fU, 0x01U);
	gdc_clear_planes(gdc);

	/* Reports the gdc_clear_graphics result. */
	return 1;
}

/* Implements rgb_to_gdc(). */
static uint8_t
rgb_to_gdc(
	uint32_t color)
{
	unsigned red;
	unsigned green;
	unsigned blue;
	unsigned luminance;

	/* Separates the color components and computes weighted luminance. */
	red = (color >> 16) & 0xffU;
	green = (color >> 8) & 0xffU;
	blue = color & 0xffU;
	luminance = (red + (green << 1) + blue) >> 2;

	/*
	 * Keep the existing integer-only RGBI conversion model, but use a
	 * half-range threshold for each component and a green-weighted
	 * luminance for the intensity plane.  The B/R/G bit order is the
	 * native PC-98 GDC plane order.
	 */
	return (uint8_t)((blue > 127U ? 1U : 0U) |
			 (red > 127U ? 2U : 0U) |
			 (green > 127U ? 4U : 0U) |
			 (luminance > 127U ? 8U : 0U));
}

/* Implements gdc_write_pixel(). */
static void
gdc_write_pixel(
	struct gdc_state *gdc,
	unsigned x,
	unsigned y,
	uint8_t color)
{
	unsigned offset;
	uint8_t mask;
	uint8_t old;
	unsigned plane;

	/* Locates the destination bit in every graphics plane. */
	offset = y * GDC_STRIDE + (x >> 3);
	mask = (uint8_t)(0x80U >> (x & 7U));

	/* Processes each gdc_write_pixel item. */
	for (plane = 0; plane < 4; plane++) {
		old = gdc->planes[plane][offset];
		gdc->planes[plane][offset] = (uint8_t)(
			(old & (uint8_t)~mask) |
			(((color >> plane) & 1U) ? mask : 0U));
	}
}

/* Implements gdc_pattern_bit(). */
static int
gdc_pattern_bit(
	uint64_t pattern,
	unsigned x,
	unsigned y)
{
	uint8_t row;

	/* Selects the requested row of the eight-row pattern. */
	row = (uint8_t)(pattern >> ((y & 7U) * 8U));

	/* Reports the gdc_pattern_bit result. */
	return (row & (uint8_t)(0x80U >> (x & 7U))) != 0;
}

/* Implements gdc_enter(). */
static int
gdc_enter(
	struct gdc_state *gdc,
	struct display_info *info)
{
	/* Requires the complete GDC backend and display result. */
	if (gdc == NULL ||
	    info == NULL ||
	    gdc->planes[0] == NULL ||
	    gdc->planes[1] == NULL ||
	    gdc->planes[2] == NULL ||
	    gdc->planes[3] == NULL)
		return 0;

	/* Clear every graphics plane before starting the slave GDC.  Otherwise
	 * firmware VRAM is briefly visible between GDC_START and this clear. */
	if (!gdc_clear_graphics(gdc))
		return 0;

	/* Starts the cleared slave GDC display. */
	display_reset();

	/* Hide text only after the clean graphics display is running. */
	if (!gdc_command(0x0c)) {
		(void)gdc_command(0x0d);

		/* Reports the gdc_enter result. */
		return 0;
	}
	info->width = GDC_WIDTH;
	info->height = GDC_HEIGHT;
	info->bits_per_pixel = 4;
	info->stride = GDC_STRIDE;

	/* Reports the gdc_enter result. */
	return 1;
}

/* Implements gdc_leave(). */
static void
gdc_leave(
	struct gdc_state *gdc)
{
	/* Restores text output before clearing the graphics planes. */
	display_stop();
	gdc_clear_planes(gdc);
	(void)gdc_command(0x0d);
}

/* Implements gdc_fill(). */
static int
gdc_fill(
	struct gdc_state *gdc,
	const struct rect *rect,
	uint32_t color)
{
	uint8_t gdc_color;
	unsigned first_byte;
	unsigned last_pixel;
	unsigned last_byte;
	unsigned y;
	unsigned byte;
	unsigned plane;
	unsigned offset;
	uint8_t mask;
	uint8_t old;

	/* Computes the planar color and affected byte range. */
	gdc_color = rgb_to_gdc(color);
	first_byte = rect->x >> 3;
	last_pixel = rect->x + rect->width - 1U;
	last_byte = last_pixel >> 3;

	/* Processes each gdc_fill item. */
	for (y = rect->y; y < rect->y + rect->height; y++) {
		/* Processes each gdc_fill item. */
		for (byte = first_byte; byte <= last_byte; byte++) {
			mask = 0xffU;
			offset = y * GDC_STRIDE + byte;

			/* Handles the next gdc_fill decision. */
			if (byte == first_byte)
				mask &= (uint8_t)(0xffU >> (rect->x & 7U));

			/* Handles the next gdc_fill decision. */
			if (byte == last_byte)
				mask &= (uint8_t)(0xffU << (7U - (last_pixel & 7U)));

			/* Processes each gdc_fill item. */
			for (plane = 0; plane < 4; plane++) {
				old = gdc->planes[plane][offset];
				gdc->planes[plane][offset] =
					(uint8_t)((old & (uint8_t)~mask) |
					(((gdc_color >> plane) & 1U) ? mask : 0));
			}
		}
	}

	/* Reports the gdc_fill result. */
	return 1;
}

/* Implements gdc_line(). */
static int
gdc_line(
	struct gdc_state *gdc,
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
	int step_x;
	int delta_y;
	int step_y;
	int error;
	int twice_error;
	uint8_t gdc_color;

	/* Initializes Bresenham's line state and planar color. */
	x = (int)x0;
	y = (int)y0;
	target_x = (int)x1;
	target_y = (int)y1;
	delta_x = target_x >= x ? target_x - x : x - target_x;
	step_x = x < target_x ? 1 : -1;
	delta_y = target_y >= y ? y - target_y : target_y - y;
	step_y = y < target_y ? 1 : -1;
	error = delta_x + delta_y;
	gdc_color = rgb_to_gdc(color);

	/* Processes each gdc_line item. */
	for (;;) {
		gdc_write_pixel(gdc, (unsigned)x, (unsigned)y, gdc_color);

		/* Handles the next gdc_line decision. */
		if (x == target_x && y == target_y)
			break;
		twice_error = error * 2;

		/* Handles the next gdc_line decision. */
		if (twice_error >= delta_y) {
			error += delta_y;
			x += step_x;
		}

		/* Handles the next gdc_line decision. */
		if (twice_error <= delta_x) {
			error += delta_x;
			y += step_y;
		}
	}

	/* Reports the gdc_line result. */
	return 1;
}

/* Implements gdc_pattern_fill(). */
static int
gdc_pattern_fill(
	struct gdc_state *gdc,
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	uint8_t gdc_color;
	unsigned x;
	unsigned y;

	/* Converts the fill color once for every patterned pixel. */
	gdc_color = rgb_to_gdc(color);

	/* Processes each gdc_pattern_fill item. */
	for (y = rect->y; y < rect->y + rect->height; y++) {
		/* Processes each gdc_pattern_fill item. */
		for (x = rect->x; x < rect->x + rect->width; x++) {
			/* Handles the next gdc_pattern_fill decision. */
			if (gdc_pattern_bit(pattern, x - rect->x, y - rect->y))
				gdc_write_pixel(gdc, x, y, gdc_color);
		}
	}

	/* Reports the gdc_pattern_fill result. */
	return 1;
}

/* Implements gdc_draw_image_common(). */
static int
gdc_draw_image_common(
	struct gdc_state *gdc,
	unsigned destination_x,
	unsigned destination_y,
	const struct image *image,
	uint64_t pattern)
{
	const uint8_t *row;
	const uint8_t *pixel;
	uint32_t rgb;
	uint8_t gdc_color;
	unsigned index;
	unsigned x;
	unsigned y;

	/* Processes each gdc_draw_image_common item. */
	for (y = 0; y < image->height; y++) {
		row = image->pixels + (size_t)y * image->stride;

		/* Processes each gdc_draw_image_common item. */
		for (x = 0; x < image->width; x++) {
			/* Handles the next gdc_draw_image_common decision. */
			if (!gdc_pattern_bit(pattern, x, y))
				continue;

			/* Handles the next gdc_draw_image_common decision. */
			if (image->format == IMAGE_INDEX8) {
				index = row[x];
				rgb = index < image->palette_size ?
					image->palette[index] : 0;
			} else {
				pixel = row + (size_t)x * 3U;
				rgb = ((uint32_t)pixel[0] << 16) |
				      ((uint32_t)pixel[1] << 8) | pixel[2];
			}
			/* Converts and writes the selected image pixel. */
			gdc_color = rgb_to_gdc(rgb);
			gdc_write_pixel(
				gdc,
				destination_x + x,
				destination_y + y,
				gdc_color);
		}
	}

	/* Reports the gdc_draw_image_common result. */
	return 1;
}

/* Implements gdc_draw_image(). */
static int
gdc_draw_image(
	struct gdc_state *gdc,
	unsigned destination_x,
	unsigned destination_y,
	const struct image *image)
{
	int call_result;

	/* Reports the gdc_draw_image result. */
	call_result = gdc_draw_image_common(
		gdc,
		destination_x,
		destination_y,
		image,
		UINT64_MAX);

	/* Reports the gdc_draw_image result. */
	return call_result;
}

/* Implements gdc_draw_image_pattern(). */
static int
gdc_draw_image_pattern(
	struct gdc_state *gdc,
	unsigned destination_x,
	unsigned destination_y,
	const struct image *image,
	uint64_t pattern)
{
	int call_result;

	/* Reports the gdc_draw_image_pattern result. */
	call_result = gdc_draw_image_common(
		gdc,
		destination_x,
		destination_y,
		image,
		pattern);

	/* Reports the gdc_draw_image_pattern result. */
	return call_result;
}

/* Implements wab_write(). */
static void
wab_write(
	uint8_t index,
	uint8_t value)
{
	port_out8(WAB_INDEX, index);
	port_out8(WAB_DATA, value);
}

/* Implements wab_read(). */
static uint8_t
wab_read(
	uint8_t index)
{
	uint8_t call_result;

	port_out8(WAB_INDEX, index);

	/* Reports the wab_read result. */
	call_result = port_in8(WAB_DATA);

	/* Reports the wab_read result. */
	return call_result;
}

/* Implements seq_write(). */
static void
seq_write(
	uint8_t index,
	uint8_t value)
{
	port_out8(CIRRUS_IO + 4U, index);
	port_out8(CIRRUS_IO + 5U, value);
}

/* Implements seq_read(). */
static uint8_t
seq_read(
	uint8_t index)
{
	uint8_t call_result;

	port_out8(CIRRUS_IO + 4U, index);

	/* Reports the seq_read result. */
	call_result = port_in8(CIRRUS_IO + 5U);

	/* Reports the seq_read result. */
	return call_result;
}

/* Implements gfx_write(). */
static void
gfx_write(
	uint8_t index,
	uint8_t value)
{
	port_out8(CIRRUS_IO + 0x0eU, index);
	port_out8(CIRRUS_IO + 0x0fU, value);
}

/* Implements crtc_write(). */
static void
crtc_write(
	uint8_t index,
	uint8_t value)
{
	uint16_t port;

	/* Selects the active CRTC register pair. */
	port = (port_in8(CIRRUS_IO + 0x0cU) & 1U) ?
		CIRRUS_CRTC : CIRRUS_CRTC_MONO;
	port_out8(port, index);
	port_out8(port + 1U, value);
}

/* Implements crtc_read(). */
static uint8_t
crtc_read(
	uint8_t index)
{
	uint8_t call_result;
	uint16_t port;

	/* Selects the active CRTC register pair. */
	port = (port_in8(CIRRUS_IO + 0x0cU) & 1U) ?
		CIRRUS_CRTC : CIRRUS_CRTC_MONO;
	port_out8(port, index);

	/* Reports the crtc_read result. */
	call_result = port_in8(port + 1U);

	/* Reports the crtc_read result. */
	return call_result;
}

/* Implements hidden_dac_write(). */
static void
hidden_dac_write(
	uint8_t value)
{
	unsigned i;

	(void)port_in8(CIRRUS_IO + 8U);

	/* Processes each hidden_dac_write item. */
	for (i = 0; i < 4; i++)
		(void)port_in8(CIRRUS_IO + 6U);

	port_out8(CIRRUS_IO + 6U, value);
}

/* Implements load_rgb332_palette(). */
static void
load_rgb332_palette(
	void)
{
	unsigned i;
	unsigned red;
	unsigned green;
	unsigned blue;

	port_out8(CIRRUS_IO + 6U, 0xffU);
	port_out8(CIRRUS_IO + 8U, 0);

	/* Processes each load_rgb332_palette item. */
	for (i = 0; i < 256; i++) {
		red = (i >> 5) & 7U;
		green = (i >> 2) & 7U;
		blue = i & 3U;
		port_out8(CIRRUS_IO + 9U, (uint8_t)(red * 63U / 7U));
		port_out8(CIRRUS_IO + 9U, (uint8_t)(green * 63U / 7U));
		port_out8(CIRRUS_IO + 9U, (uint8_t)(blue * 63U / 3U));
	}
}

/* Implements coregraph_id_present(). */
static int
coregraph_id_present(
	void)
{
	uint8_t id;

	/* Reads the Core-Graph board identifier. */
	id = wab_read(WAB_REG_ID);

	/* Rejects identifiers below the Core-Graph range. */
	if (id < 0x58U)
		return 0;

	/* Reports whether the identifier is inside the Core-Graph range. */
	return id <= 0x5dU;
}

/* Implements coregraph_gate_enter(). */
static void
coregraph_gate_enter(
	void)
{
	port_out8(PC98_GDC_MODE, 0x0eU);
	port_out8(PC98_VRAM_SWITCH, 0x07U);
	port_out8(PC98_VRAM_SWITCH, 0x8fU);
	port_out8(PC98_VRAM_SWITCH, 0x06U);

	wab_write(WAB_REG_RELAY, WAB_RELAY_WAB);

	port_out8(PC98_WAIT, 0);
	port_out8(PC98_WAIT, 0);

	port_out8(CIRRUS_SLEEP, 0x01U);
}

/* Implements coregraph_gate_leave(). */
static void
coregraph_gate_leave(
	void)
{
	unsigned i;

	port_out8(CIRRUS_SLEEP, 0);
	wab_write(WAB_REG_RELAY, 0);
	port_out8(PC98_WAIT, 0);
	port_out8(PC98_VRAM_SWITCH, 0x07U);
	port_out8(PC98_VRAM_SWITCH, 0x8eU);
	port_out8(PC98_VRAM_SWITCH, 0x06U);

	/* Processes each coregraph_gate_leave item. */
	for (i = 0; i < 200000U; i++)
		port_out8(PC98_WAIT, 0);

	port_out8(PC98_GDC_MODE, 0x0fU);
}

/* Implements coregraph_mode_640x480(). */
static void
coregraph_mode_640x480(
	unsigned bits_per_pixel)
{
	static const uint8_t seq_index[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x07, 0x08,
		0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x16, 0x18,
		0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	static const uint8_t seq_value[] = {
		0x01, 0x01, 0x0f, 0x00, 0x0e, 0x11, 0x00,
		0x66, 0x48, 0x56, 0x60, 0x30, 0x58, 0x40,
		0x3b, 0x23, 0x3d, 0x3b, 0x20
	};
	static const uint8_t crtc[0x1c] = {
		0x5f,0x4f,0x50,0x84,0x54,0x80,0x0b,0x3e,
		0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
		0xe5,0x87,0xdf,0x50,0x00,0xe7,0x04,0xe3,
		0xff,0x00,0x90,0x22
	};
	static const uint8_t graphics[9] = {
		0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff
	};
	static const uint8_t attribute[21] = {
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
		0x41,0x00,0x0f,0x00,0x00
	};

	uint8_t value;
	uint8_t register_value;
	unsigned i;

	gfx_write(0x33U, 0);
	gfx_write(0x31U, 0x04U);
	gfx_write(0x31U, 0);

	seq_write(0x06U, 0x12U);
	seq_write(0x12U, 0);

	/* Processes each coregraph_mode_640x480 item. */
	for (i = 0; i < sizeof(seq_index); i++) {
		value = seq_value[i];

		/* Handles the next coregraph_mode_640x480 decision. */
		if (seq_index[i] == 0x07U && bits_per_pixel == 24U)
			value = 0x15U;
		seq_write(seq_index[i], value);
	}

	/* Updates the sequencer extension register. */
	register_value = seq_read(0x0fU);
	register_value = (uint8_t)((register_value & 0xdfU) | 0x20U);
	seq_write(0x0fU, register_value);
	port_out8(CIRRUS_IO + 2U, 0xe3U);
	gfx_write(0x06U, 0x05U);
	seq_write(0x00U, 0x03U);
	crtc_write(0x11U, 0x20U);

	/* Processes each coregraph_mode_640x480 item. */
	for (i = 0; i < sizeof(crtc); i++) {
		value = crtc[i];

		/* Handles the next coregraph_mode_640x480 decision. */
		if (i == 0x13U && bits_per_pixel == 24U)
			value = 0xf0U;
		crtc_write((uint8_t)i, value);
	}

	/* Processes each coregraph_mode_640x480 item. */
	for (i = 0; i < sizeof(graphics); i++)
		gfx_write((uint8_t)i, graphics[i]);

	(void)port_in8(CIRRUS_STATUS);

	/* Processes each coregraph_mode_640x480 item. */
	for (i = 0; i < sizeof(attribute); i++) {
		port_out8(CIRRUS_IO, (uint8_t)i);
		port_out8(CIRRUS_IO, attribute[i]);
	}

	(void)port_in8(CIRRUS_STATUS);

	port_out8(CIRRUS_IO, 0x20U);

	hidden_dac_write(bits_per_pixel == 24U ? 0xc5U : 0x20U);

	port_out8(CIRRUS_IO + 6U, 0xffU);

	gfx_write(0x09U, 0);
	gfx_write(0x0aU, 0);
	gfx_write(0x0bU, 0x21U);

	/* Enables the linear framebuffer sequencer gates. */
	register_value = seq_read(0x17U);
	register_value = (uint8_t)(register_value | 0x44U);
	seq_write(0x17U, register_value);
	register_value = seq_read(0x18U);
	register_value = (uint8_t)(register_value & 0xbfU);
	seq_write(0x18U, register_value);
	gfx_write(0x31U, 0x04U);
	gfx_write(0x31U, 0);

	/* Handles the next coregraph_mode_640x480 decision. */
	if (bits_per_pixel == 8U)
		load_rgb332_palette();

	seq_write(0x01U, 0x21U);
}

/* Implements rgb332(). */
static uint8_t
rgb332(
	uint32_t color)
{
	/* Reports the rgb332 result. */
	return (uint8_t)(((color >> 16) & 0xe0U) |
			 ((color >> 11) & 0x1cU) | ((color >> 6) & 0x03U));
}

/* Implements cirrus_pattern_bit(). */
static int
cirrus_pattern_bit(
	uint64_t pattern,
	unsigned x,
	unsigned y)
{
	uint8_t row;

	/* Selects the requested row of the eight-row pattern. */
	row = (uint8_t)(pattern >> ((y & 7U) * 8U));

	/* Reports the cirrus_pattern_bit result. */
	return (row & (uint8_t)(0x80U >> (x & 7U))) != 0;
}

/* Writes one pixel to the active Cirrus framebuffer. */
static void
cirrus_write_pixel(
	struct cirrus_state *cirrus,
	unsigned x,
	unsigned y,
	uint32_t color)
{
	volatile uint8_t *pixel;

	/* Writes one palette index in the eight-bit mode. */
	if (cirrus->bits_per_pixel == 8U) {
		cirrus->framebuffer[y * CIRRUS_STRIDE_8 + x] =
			rgb332(color);

		/* Finishes write_pixel processing. */
		return;
	}

	pixel = cirrus->framebuffer + y * CIRRUS_STRIDE_24 + x * 3U;
	pixel[0] = (uint8_t)color;
	pixel[1] = (uint8_t)(color >> 8);
	pixel[2] = (uint8_t)(color >> 16);
}

/* Implements cirrus_enter(). */
static int
cirrus_enter(
	struct cirrus_state *cirrus,
	struct display_info *info)
{
	uint8_t relay_setup;
	uint8_t chip;
	unsigned bits_per_pixel;
	unsigned stride;
	unsigned visible_bytes;
	unsigned i;

	/* Requires the complete Cirrus backend and display result. */
	if (cirrus == NULL ||
	    info == NULL ||
	    cirrus->framebuffer == NULL)
		return 0;

	/* Detects the Core-Graph board before touching its registers. */
	if (!coregraph_id_present())
		return 0;

	bits_per_pixel = info->preferred_bits_per_pixel == 24U ? 24U : 8U;
	stride = bits_per_pixel == 24U ? CIRRUS_STRIDE_24 : CIRRUS_STRIDE_8;
	visible_bytes = stride * CIRRUS_HEIGHT;

	cirrus->saved_sleep = port_in8(CIRRUS_SLEEP);
	cirrus->saved_window = wab_read(WAB_REG_WINDOW);
	cirrus->saved_linear = wab_read(WAB_REG_LINEAR);
	cirrus->saved_relay = wab_read(WAB_REG_RELAY);

	port_out8(CIRRUS_SLEEP, (uint8_t)(cirrus->saved_sleep | 1U));

	relay_setup = (uint8_t)((cirrus->saved_relay & (uint8_t)~2U) |
				WAB_RELAY_SETUP);

	wab_write(WAB_REG_RELAY, relay_setup);

	/* The motherboard ID is only a hint; validate the temporarily woken VGA. */
	seq_write(0x06U, 0x12U);
	chip = crtc_read(0x27U);

	/* Handles the next cirrus_enter decision. */
	if (chip == 0 || chip == 0xffU) {
		wab_write(WAB_REG_LINEAR, cirrus->saved_linear);
		wab_write(WAB_REG_WINDOW, cirrus->saved_window);
		wab_write(WAB_REG_RELAY, cirrus->saved_relay);
		port_out8(CIRRUS_SLEEP, cirrus->saved_sleep);

		/* Reports the cirrus_enter result. */
		return 0;
	}

	wab_write(WAB_REG_LINEAR, 0xf0U);

	/* Handles the next cirrus_enter decision. */
	if (wab_read(WAB_REG_LINEAR) != 0xf0U) {
		wab_write(WAB_REG_LINEAR, cirrus->saved_linear);
		wab_write(WAB_REG_WINDOW, cirrus->saved_window);
		wab_write(WAB_REG_RELAY, cirrus->saved_relay);
		port_out8(CIRRUS_SLEEP, cirrus->saved_sleep);

		/* Reports the cirrus_enter result. */
		return 0;
	}

	coregraph_mode_640x480(bits_per_pixel);

	/*
	 * Keep the motherboard GDC on the monitor while Cirrus is
	 * configured and its visible framebuffer is erased.
	 * WAB_REG_RELAY bit 1 in coregraph_gate_enter() is the actual
	 * GDC-to-Cirrus scanout switch.
	 */
	for (i = 0; i < visible_bytes; i++)
		cirrus->framebuffer[i] = 0;

	coregraph_gate_enter();

	seq_write(0x01U, 0x01U);

	cirrus->bits_per_pixel = (uint8_t)bits_per_pixel;
	cirrus->active = 1;

	info->width = CIRRUS_WIDTH;
	info->height = CIRRUS_HEIGHT;
	info->bits_per_pixel = bits_per_pixel;
	info->stride = stride;

	/* Reports the cirrus_enter result. */
	return 1;
}

/* Implements cirrus_leave(). */
static void
cirrus_leave(
	struct cirrus_state *cirrus)
{
	/* Handles the next cirrus_leave decision. */
	if (cirrus == NULL || !cirrus->active)
		return;

	seq_write(0x01U, 0x21U);

	coregraph_gate_leave();

	wab_write(WAB_REG_LINEAR, cirrus->saved_linear);
	wab_write(WAB_REG_WINDOW, cirrus->saved_window);

	/*
	 * coregraph_gate_leave() deliberately selects the motherboard
	 * GDC and puts Cirrus to sleep.  Restoring the saved
	 * relay/sleep registers here would immediately select and
	 * wake Cirrus again, leaving later text-VRAM output
	 * invisible.  Saved values are only for the failed-enter
	 * rollback.
	 */
	cirrus->active = 0;
	cirrus->bits_per_pixel = 0;
}

/* Implements cirrus_fill(). */
static int
cirrus_fill(
	struct cirrus_state *cirrus,
	const struct rect *rect,
	uint32_t color)
{
	volatile uint8_t *row;
	uint8_t pixel;
	unsigned x;
	unsigned y;

	/* Handles the next cirrus_fill decision. */
	if (cirrus->bits_per_pixel == 8U) {
		pixel = rgb332(color);

		/* Processes each cirrus_fill item. */
		for (y = rect->y; y < rect->y + rect->height; y++) {
			row = cirrus->framebuffer +
				y * CIRRUS_STRIDE_8 + rect->x;

			/* Processes each cirrus_fill item. */
			for (x = 0; x < rect->width; x++)
				row[x] = pixel;
		}

		/* Reports the cirrus_fill result. */
		return 1;
	}

	/* Processes each cirrus_fill item. */
	for (y = rect->y; y < rect->y + rect->height; y++) {
		row = cirrus->framebuffer +
			y * CIRRUS_STRIDE_24 + rect->x * 3U;

		/* Processes each cirrus_fill item. */
		for (x = 0; x < rect->width; x++) {
			row[x * 3U] = (uint8_t)color;
			row[x * 3U + 1U] = (uint8_t)(color >> 8);
			row[x * 3U + 2U] = (uint8_t)(color >> 16);
		}
	}

	/* Reports the cirrus_fill result. */
	return 1;
}

/* Implements cirrus_line(). */
static int
cirrus_line(
	struct cirrus_state *cirrus,
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
	int step_x;
	int delta_y;
	int step_y;
	int error;
	int twice_error;

	/* Initializes Bresenham's line state. */
	x = (int)x0;
	y = (int)y0;
	target_x = (int)x1;
	target_y = (int)y1;
	delta_x = target_x >= x ? target_x - x : x - target_x;
	step_x = x < target_x ? 1 : -1;
	delta_y = target_y >= y ? y - target_y : target_y - y;
	step_y = y < target_y ? 1 : -1;
	error = delta_x + delta_y;

	/* Processes each cirrus_line item. */
	for (;;) {
		cirrus_write_pixel(cirrus, (unsigned)x, (unsigned)y, color);

		/* Handles the next cirrus_line decision. */
		if (x == target_x && y == target_y)
			break;
		twice_error = error * 2;

		/* Handles the next cirrus_line decision. */
		if (twice_error >= delta_y) {
			error += delta_y;
			x += step_x;
		}

		/* Handles the next cirrus_line decision. */
		if (twice_error <= delta_x) {
			error += delta_x;
			y += step_y;
		}
	}

	/* Reports the cirrus_line result. */
	return 1;
}

/* Implements cirrus_pattern_fill(). */
static int
cirrus_pattern_fill(
	struct cirrus_state *cirrus,
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	unsigned x;
	unsigned y;

	/* Processes each cirrus_pattern_fill item. */
	for (y = 0; y < rect->height; y++) {
		/* Processes each cirrus_pattern_fill item. */
		for (x = 0; x < rect->width; x++) {
			/* Handles the next cirrus_pattern_fill decision. */
			if (cirrus_pattern_bit(pattern, x, y)) {
				cirrus_write_pixel(
					cirrus,
					rect->x + x,
					rect->y + y,
					color);
			}
		}
	}

	/* Reports the cirrus_pattern_fill result. */
	return 1;
}

/* Implements cirrus_draw_image_common(). */
static int
cirrus_draw_image_common(
	struct cirrus_state *cirrus,
	unsigned destination_x,
	unsigned destination_y,
	const struct image *image,
	uint64_t pattern)
{
	const uint8_t *row;
	const uint8_t *source;
	uint32_t rgb;
	unsigned index;
	unsigned x;
	unsigned y;

	/* Processes each cirrus_draw_image_common item. */
	for (y = 0; y < image->height; y++) {
		row = image->pixels + (size_t)y * image->stride;

		/* Processes each cirrus_draw_image_common item. */
		for (x = 0; x < image->width; x++) {
			/* Handles the next cirrus_draw_image_common decision. */
			if (!cirrus_pattern_bit(pattern, x, y))
				continue;

			/* Handles the next cirrus_draw_image_common decision. */
			if (image->format == IMAGE_INDEX8) {
				index = row[x];
				rgb = index < image->palette_size ?
					image->palette[index] : 0;
			} else {
				source = row + (size_t)x * 3U;
				rgb = ((uint32_t)source[0] << 16) |
				      ((uint32_t)source[1] << 8) | source[2];
			}
			cirrus_write_pixel(
				cirrus,
				destination_x + x,
				destination_y + y,
				rgb);
		}
	}

	/* Reports the cirrus_draw_image_common result. */
	return 1;
}

/* Implements cirrus_draw_image(). */
static int
cirrus_draw_image(
	struct cirrus_state *cirrus,
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int call_result;

	/* Reports the cirrus_draw_image result. */
	call_result = cirrus_draw_image_common(cirrus, x, y, image, UINT64_MAX);

	/* Reports the cirrus_draw_image result. */
	return call_result;
}

/* Implements cirrus_draw_image_pattern(). */
static int
cirrus_draw_image_pattern(
	struct cirrus_state *cirrus,
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int call_result;

	/* Reports the cirrus_draw_image_pattern result. */
	call_result = cirrus_draw_image_common(cirrus, x, y, image, pattern);

	/* Reports the cirrus_draw_image_pattern result. */
	return call_result;
}

/* Initializes every direct PC-98 display backend. */
static void
backend_initialize(
	volatile uint8_t *cirrus_framebuffer)
{
	/* Initializes the planar GDC apertures. */
	memset(&gdc, 0, sizeof(gdc));
	gdc.planes[0] = (volatile uint8_t *)0x000a8000U;
	gdc.planes[1] = (volatile uint8_t *)0x000b0000U;
	gdc.planes[2] = (volatile uint8_t *)0x000b8000U;
	gdc.planes[3] = (volatile uint8_t *)0x000e0000U;

	/* Initializes the PC-98 character-generator cache. */
	memset(&glyph, 0, sizeof(glyph));
	glyph.cg_window = (volatile uint8_t *)0x000a4000U;

	/* Initializes the optional Cirrus framebuffer. */
	memset(&cirrus, 0, sizeof(cirrus));
	cirrus.framebuffer = cirrus_framebuffer;
}

/* Selects and enters the first usable PC-98 display. */
static int
display_enter(
	struct display_info *info)
{
	/* Starts without an active display. */
	state.display_kind = DISPLAY_NONE;

	/* Tries the accelerated Cirrus display first. */
	if (cirrus_enter(&cirrus, info)) {
		state.display_kind = DISPLAY_CIRRUS;

		/* Reports the selected Cirrus display. */
		return 1;
	}

	/* Falls back to the planar GDC display. */
	if (gdc_enter(&gdc, info)) {
		state.display_kind = DISPLAY_GDC;

		/* Reports the selected GDC display. */
		return 1;
	}

	/* Reports that no display could be entered. */
	return 0;
}

/* Leaves the selected PC-98 display. */
static void
display_leave(
	void)
{
	/* Leaves the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS)
		cirrus_leave(&cirrus);
	else if (state.display_kind == DISPLAY_GDC)
		gdc_leave(&gdc);

	/* Clears the display selection. */
	state.display_kind = DISPLAY_NONE;
}

/* Fills through the selected PC-98 display. */
static int
display_fill(
	const struct rect *rect,
	uint32_t color)
{
	int call_result;

	/* Fills through the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS)
		call_result = cirrus_fill(&cirrus, rect, color);
	else if (state.display_kind == DISPLAY_GDC)
		call_result = gdc_fill(&gdc, rect, color);
	else
		return 0;

	/* Reports whether the rectangle was filled. */
	return call_result;
}

/* Draws a line through the selected PC-98 display. */
static int
display_line(
	unsigned x0,
	unsigned y0,
	unsigned x1,
	unsigned y1,
	uint32_t color)
{
	int call_result;

	/* Draws through the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS)
		call_result = cirrus_line(&cirrus, x0, y0, x1, y1, color);
	else if (state.display_kind == DISPLAY_GDC)
		call_result = gdc_line(&gdc, x0, y0, x1, y1, color);
	else
		return 0;

	/* Reports whether the line was drawn. */
	return call_result;
}

/* Fills a pattern through the selected PC-98 display. */
static int
display_pattern_fill(
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	int call_result;

	/* Fills through the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS)
		call_result = cirrus_pattern_fill(&cirrus, rect, color, pattern);
	else if (state.display_kind == DISPLAY_GDC)
		call_result = gdc_pattern_fill(&gdc, rect, color, pattern);
	else
		return 0;

	/* Reports whether the pattern was filled. */
	return call_result;
}

/* Draws an image through the selected PC-98 display. */
static int
display_draw_image(
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int call_result;

	/* Draws through the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS)
		call_result = cirrus_draw_image(&cirrus, x, y, image);
	else if (state.display_kind == DISPLAY_GDC)
		call_result = gdc_draw_image(&gdc, x, y, image);
	else
		return 0;

	/* Reports whether the image was drawn. */
	return call_result;
}

/* Draws a patterned image through the selected PC-98 display. */
static int
display_draw_image_pattern(
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int call_result;

	/* Draws through the active concrete display. */
	if (state.display_kind == DISPLAY_CIRRUS) {
		call_result = cirrus_draw_image_pattern(
			&cirrus,
			x,
			y,
			image,
			pattern);
	} else if (state.display_kind == DISPLAY_GDC) {
		call_result = gdc_draw_image_pattern(
			&gdc,
			x,
			y,
			image,
			pattern);
	} else {
		return 0;
	}

	/* Reports whether the patterned image was drawn. */
	return call_result;
}

/* Implements return_int(). */
static bool
return_int(
	NoctEnv *env,
	int value)
{
	NoctValue result;
	bool ok;

	memset(&result, 0, sizeof(result));

	/* Handles the next return_int decision. */
	if (!noct_pin_local(env, 1, &result))
		return false;
	ok = noct_set_return_make_int(env, &result, value);
	(void)noct_unpin_local(env, 1, &result);

	/* Reports the return_int result. */
	return ok;
}

/* Implements get_int_arg(). */
static bool
get_int_arg(
	NoctEnv *env,
	uint32_t index,
	int *result)
{
	NoctValue value;
	int64_t long_value;
	int int_value;
	bool ok;

	memset(&value, 0, sizeof(value));

	/* Handles the next get_int_arg decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;
	ok = noct_get_arg_check_long(env, index, &value, &long_value);

	/* Handles the next get_int_arg decision. */
	if (!ok) {
		ok = noct_get_arg_check_int(env, index, &value, &int_value);

		/* Handles the next get_int_arg decision. */
		if (ok)
			long_value = int_value;
	}

	/* Handles the next get_int_arg decision. */
	if (ok)
		*result = (int)long_value;
	(void)noct_unpin_local(env, 1, &value);

	/* Reports the get_int_arg result. */
	return ok;
}

/* Implements cfunc_BeUI_init(). */
static bool
cfunc_BeUI_init(
	NoctEnv *env)
{
	bool call_result;
	int api_result;

	/* Initializes BeUI through the bound backend. */
	api_result = beui_init() ? 1 : 0;

	/* Publishes the initialization result. */
	call_result = return_int(env, api_result);

	/* Reports the cfunc_BeUI_init result. */
	return call_result;
}

/* Implements cfunc_BeUI_initWithHint(). */
static bool
cfunc_BeUI_initWithHint(
	NoctEnv *env)
{
	bool call_result;
	int bits_per_pixel;
	int api_result;

	/* Reads the requested display depth. */
	if (!get_int_arg(env, 0, &bits_per_pixel)) {
		noct_error(
			env,
			N_TR("BeUI.initWithHint expects 8 or 24 bits per pixel."));

		/* Reports an invalid display depth argument. */
		return false;
	}

	/* Accepts only display depths supported by the public contract. */
	if (bits_per_pixel != 8 && bits_per_pixel != 24) {
		noct_error(
			env,
			N_TR("BeUI.initWithHint expects 8 or 24 bits per pixel."));

		/* Reports the cfunc_BeUI_initWithHint result. */
		return false;
	}

	/* Initializes BeUI with the validated display depth. */
	api_result = beui_init_with_hint((unsigned)bits_per_pixel) ? 1 : 0;

	/* Publishes the initialization result. */
	call_result = return_int(env, api_result);

	/* Reports the cfunc_BeUI_initWithHint result. */
	return call_result;
}

/* Implements cfunc_BeUI_close(). */
static bool
cfunc_BeUI_close(
	NoctEnv *env)
{
	bool call_result;

	beui_close();

	/* Reports the cfunc_BeUI_close result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_close result. */
	return call_result;
}

/* Implements cfunc_BeUI_isOpen(). */
static bool
cfunc_BeUI_isOpen(
	NoctEnv *env)
{
	bool call_result;
	int api_result;

	/* Reads the current BeUI lifecycle state. */
	api_result = beui_is_open() ? 1 : 0;

	/* Publishes the lifecycle state. */
	call_result = return_int(env, api_result);

	/* Reports the cfunc_BeUI_isOpen result. */
	return call_result;
}

/* Implements cfunc_BeUI_getWidth(). */
static bool
cfunc_BeUI_getWidth(
	NoctEnv *env)
{
	bool call_result;
	struct display_info info;

	/* Handles the next cfunc_BeUI_getWidth decision. */
	if (!beui_get_display_info(&info)) {
		noct_error(env, N_TR("BeUI is not open."));

		/* Reports the cfunc_BeUI_getWidth result. */
		return false;
	}

	/* Reports the cfunc_BeUI_getWidth result. */
	call_result = return_int(env, (int)info.width);

	/* Reports the cfunc_BeUI_getWidth result. */
	return call_result;
}

/* Implements cfunc_BeUI_getHeight(). */
static bool
cfunc_BeUI_getHeight(
	NoctEnv *env)
{
	bool call_result;
	struct display_info info;

	/* Handles the next cfunc_BeUI_getHeight decision. */
	if (!beui_get_display_info(&info)) {
		noct_error(env, N_TR("BeUI is not open."));

		/* Reports the cfunc_BeUI_getHeight result. */
		return false;
	}

	/* Reports the cfunc_BeUI_getHeight result. */
	call_result = return_int(env, (int)info.height);

	/* Reports the cfunc_BeUI_getHeight result. */
	return call_result;
}

/*
 * Returns 1 while the display is alive and 0 once it has closed, so the
 * canonical loop is "while (BeUI.poll()) { ... }".  Targets that own the
 * whole machine never return 0.
 */
static bool
cfunc_BeUI_poll(
	NoctEnv *env)
{
	bool call_result;
	int api_result;

	/* Services the active BeUI backends. */
	api_result = beui_poll() ? 1 : 0;

	/* Publishes the poll result. */
	call_result = return_int(env, api_result);

	/* Reports the cfunc_BeUI_poll result. */
	return call_result;
}

/* Implements cfunc_BeUI_flush(). */
static bool
cfunc_BeUI_flush(
	NoctEnv *env)
{
	bool call_result;

	/* Handles the next cfunc_BeUI_flush decision. */
	if (!beui_flush()) {
		noct_error(env, N_TR("BeUI.flush failed."));

		/* Reports the cfunc_BeUI_flush result. */
		return false;
	}

	/* Reports the cfunc_BeUI_flush result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_flush result. */
	return call_result;
}

/* Implements cfunc_BeUI_fill(). */
static bool
cfunc_BeUI_fill(
	NoctEnv *env)
{
	bool call_result;
	struct rect rectangle;
	int x, y, width, height, color;

	/* Reads the fill origin. */
	if (!get_int_arg(env, 0, &x)) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports an invalid fill argument. */
		return false;
	}

	/* Reads the vertical fill origin. */
	if (!get_int_arg(env, 1, &y)) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports an invalid fill argument. */
		return false;
	}

	/* Reads the fill extent. */
	if (!get_int_arg(env, 2, &width)) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports an invalid fill argument. */
		return false;
	}

	/* Reads the fill height. */
	if (!get_int_arg(env, 3, &height)) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports an invalid fill argument. */
		return false;
	}

	/* Reads the fill color. */
	if (!get_int_arg(env, 4, &color)) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports an invalid fill argument. */
		return false;
	}

	/* Validates the complete fill request. */
	if (x < 0 ||
	    y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(env, N_TR("BeUI.fill received an invalid argument."));

		/* Reports the cfunc_BeUI_fill result. */
		return false;
	}
	rectangle.x = (unsigned)x;
	rectangle.y = (unsigned)y;
	rectangle.width = (unsigned)width;
	rectangle.height = (unsigned)height;

	/* Handles the next cfunc_BeUI_fill decision. */
	if (!beui_fill(&rectangle, (uint32_t)color)) {
		noct_error(env, N_TR("BeUI.fill failed."));

		/* Reports the cfunc_BeUI_fill result. */
		return false;
	}

	/* Reports the cfunc_BeUI_fill result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_fill result. */
	return call_result;
}

/* Implements cfunc_BeUI_line(). */
static bool
cfunc_BeUI_line(
	NoctEnv *env)
{
	bool call_result;
	int x0, y0, x1, y1, color;

	/* Reads the first line endpoint. */
	if (!get_int_arg(env, 0, &x0)) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports an invalid line argument. */
		return false;
	}

	/* Reads the first endpoint's vertical coordinate. */
	if (!get_int_arg(env, 1, &y0)) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports an invalid line argument. */
		return false;
	}

	/* Reads the second line endpoint. */
	if (!get_int_arg(env, 2, &x1)) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports an invalid line argument. */
		return false;
	}

	/* Reads the second endpoint's vertical coordinate. */
	if (!get_int_arg(env, 3, &y1)) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports an invalid line argument. */
		return false;
	}

	/* Reads the line color. */
	if (!get_int_arg(env, 4, &color)) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports an invalid line argument. */
		return false;
	}

	/* Validates the complete line request. */
	if (x0 < 0 ||
	    y0 < 0 ||
	    x1 < 0 ||
	    y1 < 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(env, N_TR("BeUI.line received an invalid argument."));

		/* Reports the cfunc_BeUI_line result. */
		return false;
	}

	/* Handles the next cfunc_BeUI_line decision. */
	if (!beui_line(
		(unsigned)x0,
		(unsigned)y0,
		(unsigned)x1,
		(unsigned)y1,
		(uint32_t)color)) {
		noct_error(env, N_TR("BeUI.line failed."));

		/* Reports the cfunc_BeUI_line result. */
		return false;
	}

	/* Reports the cfunc_BeUI_line result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_line result. */
	return call_result;
}

/* Implements cfunc_BeUI_patternFill(). */
static bool
cfunc_BeUI_patternFill(
	NoctEnv *env)
{
	bool call_result;
	struct rect rectangle;
	NoctValue value;
	int x, y, width, height, color;
	int int_pattern;
	int64_t pattern;
	bool ok;

	/* Reads the patterned fill origin. */
	if (!get_int_arg(env, 0, &x)) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the vertical patterned-fill origin. */
	if (!get_int_arg(env, 1, &y)) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned fill extent. */
	if (!get_int_arg(env, 2, &width)) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned-fill height. */
	if (!get_int_arg(env, 3, &height)) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned fill color. */
	if (!get_int_arg(env, 4, &color)) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Validates the complete patterned fill request. */
	if (x < 0 ||
	    y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    color < 0 ||
	    color > 0xffffff) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports the cfunc_BeUI_patternFill result. */
		return false;
	}
	memset(&value, 0, sizeof(value));

	/* Handles the next cfunc_BeUI_patternFill decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;
	ok = noct_get_arg_check_long(env, 5, &value, &pattern);

	/* Handles the next cfunc_BeUI_patternFill decision. */
	if (!ok) {
		ok = noct_get_arg_check_int(env, 5, &value, &int_pattern);

		/* Handles the next cfunc_BeUI_patternFill decision. */
		if (ok)
			pattern = (int64_t)(uint32_t)int_pattern;
	}
	(void)noct_unpin_local(env, 1, &value);

	/* Handles the next cfunc_BeUI_patternFill decision. */
	if (!ok) {
		noct_error(
			env,
			N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports the cfunc_BeUI_patternFill result. */
		return false;
	}
	rectangle.x = (unsigned)x;
	rectangle.y = (unsigned)y;
	rectangle.width = (unsigned)width;
	rectangle.height = (unsigned)height;

	/* Handles the next cfunc_BeUI_patternFill decision. */
	if (!beui_pattern_fill(
		&rectangle,
		(uint32_t)color,
		(uint64_t)pattern)) {
		noct_error(env, N_TR("BeUI.patternFill failed."));

		/* Reports the cfunc_BeUI_patternFill result. */
		return false;
	}

	/* Reports the cfunc_BeUI_patternFill result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_patternFill result. */
	return call_result;
}

/* Implements measure_text_arg(). */
static bool
measure_text_arg(
	NoctEnv *env,
	const char *api,
	unsigned *width,
	unsigned *height)
{
	NoctValue value;
	const char *text;

	memset(&value, 0, sizeof(value));

	/* Handles the next measure_text_arg decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Handles the next measure_text_arg decision. */
	if (!noct_get_arg_check_string(env, 0, &value, &text)) {
		noct_error(env, N_TR("%s failed."), api);
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the measure_text_arg result. */
		return false;
	}

	/* Handles the next measure_text_arg decision. */
	if (!beui_measure_text(text, width, height)) {
		noct_error(env, N_TR("%s failed."), api);
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the measure_text_arg result. */
		return false;
	}
	(void)noct_unpin_local(env, 1, &value);

	/* Reports the measure_text_arg result. */
	return true;
}

/* Implements cfunc_BeUI_textWidth(). */
static bool
cfunc_BeUI_textWidth(
	NoctEnv *env)
{
	bool call_result;
	unsigned width, height;

	/* Handles the next cfunc_BeUI_textWidth decision. */
	if (!measure_text_arg(env, "BeUI.textWidth", &width, &height))
		return false;

	/* Reports the cfunc_BeUI_textWidth result. */
	call_result = return_int(env, (int)width);

	/* Reports the cfunc_BeUI_textWidth result. */
	return call_result;
}

/* Implements cfunc_BeUI_textHeight(). */
static bool
cfunc_BeUI_textHeight(
	NoctEnv *env)
{
	bool call_result;
	unsigned width, height;

	/* Handles the next cfunc_BeUI_textHeight decision. */
	if (!measure_text_arg(env, "BeUI.textHeight", &width, &height))
		return false;

	/* Reports the cfunc_BeUI_textHeight result. */
	call_result = return_int(env, (int)height);

	/* Reports the cfunc_BeUI_textHeight result. */
	return call_result;
}

/* Implements cfunc_BeUI_drawText(). */
static bool
cfunc_BeUI_drawText(
	NoctEnv *env)
{
	NoctValue value;
	const char *text;
	int x, y, foreground, background;
	bool ok;

	/* Reads the text origin. */
	if (!get_int_arg(env, 1, &x)) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));

		/* Reports an invalid text argument. */
		return false;
	}

	/* Reads the vertical text origin. */
	if (!get_int_arg(env, 2, &y)) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));

		/* Reports an invalid text argument. */
		return false;
	}

	/* Reads the text colors. */
	if (!get_int_arg(env, 3, &foreground)) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));

		/* Reports an invalid text argument. */
		return false;
	}

	/* Reads the text background color. */
	if (!get_int_arg(env, 4, &background)) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));

		/* Reports an invalid text argument. */
		return false;
	}

	/* Validates the text position and colors. */
	if (x < 0 ||
	    y < 0 ||
	    foreground < 0 ||
	    foreground > 0xffffff ||
	    background < 0 ||
	    background > 0xffffff) {
		noct_error(env, N_TR("BeUI.drawText received an invalid argument."));

		/* Reports the cfunc_BeUI_drawText result. */
		return false;
	}
	memset(&value, 0, sizeof(value));

	/* Handles the next cfunc_BeUI_drawText decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Handles the next cfunc_BeUI_drawText decision. */
	if (!noct_get_arg_check_string(env, 0, &value, &text)) {
		noct_error(env, N_TR("BeUI.drawText failed."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_drawText result. */
		return false;
	}

	/* Handles the next cfunc_BeUI_drawText decision. */
	if (!beui_draw_text(
		text,
		(unsigned)x,
		(unsigned)y,
		(uint32_t)foreground,
		(uint32_t)background)) {
		noct_error(env, N_TR("BeUI.drawText failed."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_drawText result. */
		return false;
	}
	ok = return_int(env, 1);
	(void)noct_unpin_local(env, 1, &value);

	/* Reports the cfunc_BeUI_drawText result. */
	return ok;
}

/* Implements cfunc_BeUI_getMilliseconds(). */
static bool
cfunc_BeUI_getMilliseconds(
	NoctEnv *env)
{
	bool call_result;
	uint64_t milliseconds;

	/* Handles the next cfunc_BeUI_getMilliseconds decision. */
	if (!beui_get_milliseconds(&milliseconds)) {
		noct_error(env, N_TR("BeUI.getMilliseconds is unavailable."));

		/* Reports the cfunc_BeUI_getMilliseconds result. */
		return false;
	}

	/* Reports the cfunc_BeUI_getMilliseconds result. */
	call_result = return_int(env, (int)(milliseconds & 0x7fffffffu));

	/* Reports the cfunc_BeUI_getMilliseconds result. */
	return call_result;
}

/* Implements cfunc_BeUI_sleep(). */
static bool
cfunc_BeUI_sleep(
	NoctEnv *env)
{
	bool call_result;
	int milliseconds;

	/* Reads the requested sleep duration. */
	if (!get_int_arg(env, 0, &milliseconds)) {
		noct_error(env, N_TR("BeUI.sleep received an invalid argument."));

		/* Reports an invalid sleep argument. */
		return false;
	}

	/* Bounds the sleep duration accepted by the API. */
	if (milliseconds < 0 || milliseconds > 3600000) {
		noct_error(env, N_TR("BeUI.sleep received an invalid argument."));

		/* Reports the cfunc_BeUI_sleep result. */
		return false;
	}

	/* Handles the next cfunc_BeUI_sleep decision. */
	if (!beui_sleep((unsigned)milliseconds)) {
		noct_error(env, N_TR("BeUI.sleep is unavailable."));

		/* Reports the cfunc_BeUI_sleep result. */
		return false;
	}

	/* Reports the cfunc_BeUI_sleep result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_sleep result. */
	return call_result;
}

/* Implements cfunc_BeUI_isKeyDown(). */
static bool
cfunc_BeUI_isKeyDown(
	NoctEnv *env)
{
	bool call_result;
	int key;
	int key_down;

	/* Reads the requested key code. */
	if (!get_int_arg(env, 0, &key)) {
		noct_error(env, N_TR("BeUI.isKeyDown received an invalid argument."));

		/* Reports an invalid key argument. */
		return false;
	}

	/* Rejects key codes outside the public namespace. */
	if (key < 0) {
		noct_error(env, N_TR("BeUI.isKeyDown received an invalid argument."));

		/* Reports the cfunc_BeUI_isKeyDown result. */
		return false;
	}
	/* Keys the target cannot sense read as released. */
	key_down = beui_is_key_down(key) == 1;

	/* Publishes the normalized key state. */
	call_result = return_int(env, key_down);

	/* Reports the cfunc_BeUI_isKeyDown result. */
	return call_result;
}

/* Implements pointer_field(). */
static bool
pointer_field(
	NoctEnv *env,
	const char *api,
	unsigned *x,
	unsigned *y,
	unsigned *buttons)
{
	/* Handles the next pointer_field decision. */
	if (!beui_get_pointer(x, y, buttons)) {
		noct_error(env, N_TR("%s is unavailable."), api);

		/* Reports the pointer_field result. */
		return false;
	}

	/* Reports the pointer_field result. */
	return true;
}

/* Implements cfunc_BeUI_getPointerX(). */
static bool
cfunc_BeUI_getPointerX(
	NoctEnv *env)
{
	bool call_result;
	unsigned x;

	/* Handles the next cfunc_BeUI_getPointerX decision. */
	if (!pointer_field(env, "BeUI.getPointerX", &x, NULL, NULL))
		return false;

	/* Reports the cfunc_BeUI_getPointerX result. */
	call_result = return_int(env, (int)x);

	/* Reports the cfunc_BeUI_getPointerX result. */
	return call_result;
}

/* Implements cfunc_BeUI_getPointerY(). */
static bool
cfunc_BeUI_getPointerY(
	NoctEnv *env)
{
	bool call_result;
	unsigned y;

	/* Handles the next cfunc_BeUI_getPointerY decision. */
	if (!pointer_field(env, "BeUI.getPointerY", NULL, &y, NULL))
		return false;

	/* Reports the cfunc_BeUI_getPointerY result. */
	call_result = return_int(env, (int)y);

	/* Reports the cfunc_BeUI_getPointerY result. */
	return call_result;
}

/* Implements cfunc_BeUI_getPointerButtons(). */
static bool
cfunc_BeUI_getPointerButtons(
	NoctEnv *env)
{
	bool call_result;
	unsigned buttons;

	/* Handles the next cfunc_BeUI_getPointerButtons decision. */
	if (!pointer_field(env, "BeUI.getPointerButtons", NULL, NULL, &buttons))
		return false;

	/* Reports the cfunc_BeUI_getPointerButtons result. */
	call_result = return_int(env, (int)buttons);

	/* Reports the cfunc_BeUI_getPointerButtons result. */
	return call_result;
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
	bool ok;

	memset(&value, 0, sizeof(value));

	/* Handles the next cfunc_BeUI_loadImage decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Handles the next cfunc_BeUI_loadImage decision. */
	if (!noct_get_arg_check_packed(env, 0, &value, NOCT_PACKED_UINT8)) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_loadImage result. */
		return false;
	}

	/* Handles the next cfunc_BeUI_loadImage decision. */
	if (!noct_get_packed_size(env, &value, &size)) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_loadImage result. */
		return false;
	}

	/* Handles the next cfunc_BeUI_loadImage decision. */
	if (!noct_get_packed_pointer(env, &value, &data)) {
		noct_error(env, N_TR("BeUI.loadImage expects a byte array."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_loadImage result. */
		return false;
	}
	handle = image_load_bmp(data, size);

	/* Handles the next cfunc_BeUI_loadImage decision. */
	if (handle == 0) {
		noct_error(env, N_TR("BeUI.loadImage received an unsupported image."));
		(void)noct_unpin_local(env, 1, &value);

		/* Reports the cfunc_BeUI_loadImage result. */
		return false;
	}
	ok = return_int(env, handle);
	(void)noct_unpin_local(env, 1, &value);

	/* Reports the cfunc_BeUI_loadImage result. */
	return ok;
}

/* Implements cfunc_BeUI_getImageWidth(). */
static bool
cfunc_BeUI_getImageWidth(
	NoctEnv *env)
{
	bool call_result;
	const struct image *image;
	int handle;

	/* Reads the image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(
			env,
			N_TR("BeUI.getImageWidth received an invalid handle."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(
			env,
			N_TR("BeUI.getImageWidth received an invalid handle."));

		/* Reports the cfunc_BeUI_getImageWidth result. */
		return false;
	}

	/* Reports the cfunc_BeUI_getImageWidth result. */
	call_result = return_int(env, (int)image->width);

	/* Reports the cfunc_BeUI_getImageWidth result. */
	return call_result;
}

/* Implements cfunc_BeUI_getImageHeight(). */
static bool
cfunc_BeUI_getImageHeight(
	NoctEnv *env)
{
	bool call_result;
	const struct image *image;
	int handle;

	/* Reads the image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(
			env,
			N_TR("BeUI.getImageHeight received an invalid handle."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(
			env,
			N_TR("BeUI.getImageHeight received an invalid handle."));

		/* Reports the cfunc_BeUI_getImageHeight result. */
		return false;
	}

	/* Reports the cfunc_BeUI_getImageHeight result. */
	call_result = return_int(env, (int)image->height);

	/* Reports the cfunc_BeUI_getImageHeight result. */
	return call_result;
}

/* Implements cfunc_BeUI_drawImage(). */
static bool
cfunc_BeUI_drawImage(
	NoctEnv *env)
{
	bool call_result;
	const struct image *image;
	int handle, x, y;

	/* Reads the image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports an invalid image argument. */
		return false;
	}

	/* Reads the image destination. */
	if (!get_int_arg(env, 1, &x)) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports an invalid image argument. */
		return false;
	}

	/* Reads the vertical image destination. */
	if (!get_int_arg(env, 2, &y)) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports an invalid image argument. */
		return false;
	}

	/* Validates the image destination. */
	if (x < 0 || y < 0) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports an invalid image destination. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Draws the resolved image. */
	if (!beui_draw_image((unsigned)x, (unsigned)y, image)) {
		noct_error(env, N_TR("BeUI.drawImage failed."));

		/* Reports the cfunc_BeUI_drawImage result. */
		return false;
	}

	/* Reports the cfunc_BeUI_drawImage result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_drawImage result. */
	return call_result;
}

/* Implements cfunc_BeUI_drawImageRegion(). */
static bool
cfunc_BeUI_drawImageRegion(
	NoctEnv *env)
{
	bool call_result;
	const struct image *image;
	int handle, source_x, source_y, width, height, x, y;

	/* Reads the image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the source origin. */
	if (!get_int_arg(env, 1, &source_x)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the vertical source origin. */
	if (!get_int_arg(env, 2, &source_y)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the source extent. */
	if (!get_int_arg(env, 3, &width)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the source height. */
	if (!get_int_arg(env, 4, &height)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the destination origin. */
	if (!get_int_arg(env, 5, &x)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Reads the vertical destination origin. */
	if (!get_int_arg(env, 6, &y)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image-region argument. */
		return false;
	}

	/* Validates the complete image-region geometry. */
	if (source_x < 0 ||
	    source_y < 0 ||
	    width <= 0 ||
	    height <= 0 ||
	    x < 0 ||
	    y < 0) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports invalid image-region geometry. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Draws the requested source region. */
	if (!beui_draw_image_region(
		image,
		(unsigned)source_x,
		(unsigned)source_y,
		(unsigned)width,
		(unsigned)height,
		(unsigned)x,
		(unsigned)y)) {
		noct_error(env, N_TR("BeUI.drawImageRegion failed."));

		/* Reports the cfunc_BeUI_drawImageRegion result. */
		return false;
	}

	/* Reports the cfunc_BeUI_drawImageRegion result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_drawImageRegion result. */
	return call_result;
}

/* Implements cfunc_BeUI_drawImagePattern(). */
static bool
cfunc_BeUI_drawImagePattern(
	NoctEnv *env)
{
	bool call_result;
	const struct image *image;
	NoctValue value;
	int handle, x, y;
	int int_pattern;
	int64_t pattern;
	bool ok;

	/* Reads the patterned image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(
			env,
			N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Reads the patterned image destination. */
	if (!get_int_arg(env, 1, &x)) {
		noct_error(
			env,
			N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Reads the vertical patterned-image destination. */
	if (!get_int_arg(env, 2, &y)) {
		noct_error(
			env,
			N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Validates the patterned image destination. */
	if (x < 0 || y < 0) {
		noct_error(
			env,
			N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports the cfunc_BeUI_drawImagePattern result. */
		return false;
	}
	memset(&value, 0, sizeof(value));

	/* Handles the next cfunc_BeUI_drawImagePattern decision. */
	if (!noct_pin_local(env, 1, &value))
		return false;
	ok = noct_get_arg_check_long(env, 3, &value, &pattern);

	/* Handles the next cfunc_BeUI_drawImagePattern decision. */
	if (!ok) {
		ok = noct_get_arg_check_int(env, 3, &value, &int_pattern);

		/* Handles the next cfunc_BeUI_drawImagePattern decision. */
		if (ok)
			pattern = (int64_t)(uint32_t)int_pattern;
	}
	(void)noct_unpin_local(env, 1, &value);

	/* Requires a supported pattern value. */
	if (!ok) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Draws the image through the patterned backend operation. */
	if (!beui_draw_image_pattern(
		(unsigned)x,
		(unsigned)y,
		image,
		(uint64_t)pattern)) {
		noct_error(env, N_TR("BeUI.drawImagePattern failed."));

		/* Reports the cfunc_BeUI_drawImagePattern result. */
		return false;
	}

	/* Reports the cfunc_BeUI_drawImagePattern result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_drawImagePattern result. */
	return call_result;
}

/* Implements cfunc_BeUI_destroyImage(). */
static bool
cfunc_BeUI_destroyImage(
	NoctEnv *env)
{
	bool call_result;
	int handle;

	/* Reads the image handle. */
	if (!get_int_arg(env, 0, &handle)) {
		noct_error(env, N_TR("BeUI.destroyImage received an invalid handle."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Destroys the resolved image handle. */
	if (!image_destroy(handle)) {
		noct_error(env, N_TR("BeUI.destroyImage received an invalid handle."));

		/* Reports the cfunc_BeUI_destroyImage result. */
		return false;
	}

	/* Reports the cfunc_BeUI_destroyImage result. */
	call_result = return_int(env, 1);

	/* Reports the cfunc_BeUI_destroyImage result. */
	return call_result;
}

/* Implements register_int_dictionary(). */
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

	memset(&dictionary, 0, sizeof(dictionary));
	memset(&scratch, 0, sizeof(scratch));

	/* Handles the next register_int_dictionary decision. */
	if (!noct_pin_local(env, 2, &dictionary, &scratch))
		return false;

	/* Handles the next register_int_dictionary decision. */
	if (!noct_make_empty_dict(env, &dictionary)) {
		(void)noct_unpin_local(env, 2, &dictionary, &scratch);

		/* Reports the register_int_dictionary result. */
		return false;
	}

	/* Processes each register_int_dictionary item. */
	for (index = 0; index < count; index++) {
		/* Handles the next register_int_dictionary decision. */
		if (!noct_set_dict_elem_make_int(
			env,
			&dictionary,
			entries[index].name,
			&scratch,
			entries[index].value)) {
			(void)noct_unpin_local(env, 2, &dictionary, &scratch);

			/* Reports the register_int_dictionary result. */
			return false;
		}
	}

	/* Handles the next register_int_dictionary decision. */
	if (!noct_set_global(env, name, &dictionary)) {
		(void)noct_unpin_local(env, 2, &dictionary, &scratch);

		/* Reports the register_int_dictionary result. */
		return false;
	}
	(void)noct_unpin_local(env, 2, &dictionary, &scratch);

	/* Reports the register_int_dictionary result. */
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
		registered = noct_register_cfunc(
			env,
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
		registered = noct_get_global(
			env,
			item->global_name,
			&function);
		if (!registered) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);
			return false;
		}

		/* Adds the registered function to the public dictionary. */
		registered = noct_set_dict_elem_cstr(
			env,
			&beui_dict,
			item->field_name,
			&function);
		if (!registered) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);
			return false;
		}
	}

	/* Publishes the key-code dictionary. */
	key_count = sizeof(beui_keys) / sizeof(beui_keys[0]);
	registered = register_int_dictionary(
		env,
		"Key",
		beui_keys,
		key_count);
	if (!registered) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);
		return false;
	}

	/* Publishes the pointer-button dictionary. */
	button_count = sizeof(beui_buttons) / sizeof(beui_buttons[0]);
	registered = register_int_dictionary(
		env,
		"Button",
		beui_buttons,
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

/* Implements port_in8(). */
static uint8_t
port_in8(
	uint16_t port)
{
	uint8_t call_result;

	/* Reads one byte from the requested I/O port. */
	call_result = (uint8_t)inp(port);

	/* Reports the port_in8 result. */
	return call_result;
}

/* Implements port_out8(). */
static void
port_out8(
	uint16_t port,
	uint8_t value)
{
	/* Writes one byte to the requested I/O port. */
	outp(port, value);
}

/* Implements read_low_byte(). */
static uint8_t
read_low_byte(
	uint32_t address)
{
	/* Reports the read_low_byte result. */
	return *(volatile uint8_t *)address;
}

/*
 * INT 18h AH=42h/CH=C0h selects the 640x400 display region. AH=40h
 * shows graphics.  AH=41h hides graphics again for the DOS prompt.
 */
static void
display_reset(
	void)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.h.ah = 0x42;
	regs.h.ch = 0xc0;
	int386(0x18, &regs, &regs);
	memset(&regs, 0, sizeof(regs));
	regs.h.ah = 0x40;
	int386(0x18, &regs, &regs);
}

/* Implements display_stop(). */
static void
display_stop(
	void)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.h.ah = 0x41;
	int386(0x18, &regs, &regs);
}

/* Millisecond clock. */
static uint16_t
latch_counter(
	uint8_t latch_command,
	uint16_t data_port)
{
	uint16_t value;

	_disable();
	outp(PIT_CONTROL, latch_command);
	value = (uint16_t)inp(data_port);
	value = (uint16_t)(value | ((uint16_t)inp(data_port) << 8));
	_enable();

	/* Reports the latch_counter result. */
	return value;
}

/* Implements counter_is_running(). */
static int
counter_is_running(
	uint8_t latch_command,
	uint16_t data_port)
{
	uint16_t first;
	unsigned spins;

	first = latch_counter(latch_command, data_port);

	/* Processes each counter_is_running item. */
	for (spins = 0; spins < 10000U; spins++) {
		/* Handles the next counter_is_running decision. */
		if (latch_counter(latch_command, data_port) != first)
			return 1;
	}

	/* Reports the counter_is_running result. */
	return 0;
}

/* Implements clock_start(). */
static void
clock_start(
	void)
{
	uint16_t maximum;
	uint16_t value;
	unsigned spins;

	maximum = 0;
	clock_state.ticks_per_5ms =
		(read_low_byte(BIOS_SYSTEM_CLOCK_FLAG) & 0x80U) != 0 ?
		TICKS_PER_5MS_1996800HZ : TICKS_PER_5MS_2457600HZ;
	clock_state.ticks = 0;
	/* Preferred source: channel 1 as a free-running rate generator. */
	outp(PIT_CONTROL, PIT_PROGRAM_CH1);
	outp(PIT_COUNTER1, 0x00);
	outp(PIT_COUNTER1, 0x00);

	/* Handles the next clock_start decision. */
	if (counter_is_running(PIT_LATCH_CH1, PIT_COUNTER1)) {
		clock_state.use_channel0 = 0;
		clock_state.reload = 0; /* 65536 */
		clock_state.last_count = 0;
	} else {
		/* Gate held off: fall back to the DOS system counter with a
		 * measured reload.  The maximum latched value across several
		 * periods approximates the reload. */
		for (spins = 0; spins < 200000U; spins++) {
			value = latch_counter(PIT_LATCH_CH0, PIT_COUNTER0);

			/* Handles the next clock_start decision. */
			if (value > maximum)
				maximum = value;
		}
		clock_state.use_channel0 = 1;
		clock_state.reload = (uint16_t)(maximum + 1U);
		clock_state.last_count = latch_counter(
			PIT_LATCH_CH0,
			PIT_COUNTER0);
	}
	clock_state.initialized = 1;
}

/* Implements clock_milliseconds(). */
static uint64_t
clock_milliseconds(
	void)
{
	uint32_t period;
	uint16_t count;
	uint16_t delta;

	/* Handles the next clock_milliseconds decision. */
	if (!clock_state.initialized) {
		clock_start();

		/* Reports the clock_milliseconds result. */
		return 0;
	}

	/* Handles the next clock_milliseconds decision. */
	if (clock_state.use_channel0) {
		period = clock_state.reload != 0 ? clock_state.reload : 65536U;
		count = latch_counter(PIT_LATCH_CH0, PIT_COUNTER0);

		/* Handles the next clock_milliseconds decision. */
		if (count <= clock_state.last_count) {
			delta = (uint16_t)(clock_state.last_count - count);
		} else {
			delta = (uint16_t)(clock_state.last_count +
					   (period - count));
		}
	} else {
		count = latch_counter(PIT_LATCH_CH1, PIT_COUNTER1);
		delta = (uint16_t)(clock_state.last_count - count);
	}
	clock_state.ticks += delta;
	clock_state.last_count = count;

	/* Reports the clock_milliseconds result. */
	return clock_state.ticks * 5U / clock_state.ticks_per_5ms;
}

/*
 * Key state and type-ahead drain.
 *
 * Normalized key code to PC-98 scan code (group * 8 + bit) for the BIOS
 * real-time key state table.  Letters use their lowercase codes.
 */
static int
key_to_scan(
	int key)
{
	static const uint8_t letters[26] = {
		0x1d, 0x2d, 0x2b, 0x1f, 0x12, 0x20, 0x21, 0x22, 0x17,
		0x23, 0x24, 0x25, 0x2f, 0x2e, 0x18, 0x19, 0x10, 0x13,
		0x1e, 0x14, 0x15, 0x2c, 0x11, 0x2a, 0x16, 0x29,
	};

	/* Handles the next key_to_scan decision. */
	if (key >= 'a' && key <= 'z')
		return letters[key - 'a'];

	/* Handles the next key_to_scan decision. */
	if (key >= '1' && key <= '9')
		return 0x01 + (key - '1');

	/* Selects the matching key_to_scan operation. */
	switch (key) {
	case '0':
		/* Reports the zero scan code. */
		return 0x0a;
	case ' ':
		/* Reports the space scan code. */
		return 0x34;
	case KEY_ESCAPE:
		/* Reports the Escape scan code. */
		return 0x00;
	case KEY_TAB:
		/* Reports the Tab scan code. */
		return 0x0f;
	case KEY_ENTER:
		/* Reports the Enter scan code. */
		return 0x1c;
	case KEY_BACKSPACE:
		/* Reports the Backspace scan code. */
		return 0x0e;
	case KEY_INSERT:
		/* Reports the Insert scan code. */
		return 0x38;
	case KEY_DELETE:
		/* Reports the Delete scan code. */
		return 0x39;
	case KEY_UP:
		/* Reports the up-arrow scan code. */
		return 0x3a;
	case KEY_LEFT:
		/* Reports the left-arrow scan code. */
		return 0x3b;
	case KEY_RIGHT:
		/* Reports the right-arrow scan code. */
		return 0x3c;
	case KEY_DOWN:
		/* Reports the down-arrow scan code. */
		return 0x3d;
	case KEY_HOME:
		/* Reports the Home scan code. */
		return 0x3e;
	case KEY_PAGE_UP:
		/* Reports the Page Up scan code. */
		return 0x36;
	case KEY_PAGE_DOWN:
		/* Reports the Page Down scan code. */
		return 0x37;
	case KEY_SHIFT:
		/* Reports the Shift scan code. */
		return 0x70;
	default:
		/* Reports an unsupported key. */
		return -1;
	}
}

/* Implements input_is_key_down(). */
static int
input_is_key_down(
	int key)
{
	int scan;
	uint8_t bits;

	scan = key_to_scan(key);

	/* Handles the next input_is_key_down decision. */
	if (scan < 0)
		return -1;
	/* The ROM keyboard handler keeps a 16-byte bitmap of pressed keys
	 * at 0000:052Ah; with interrupts enabled it is always current. */
	bits = read_low_byte(BIOS_KEY_STATE_TABLE + ((unsigned)scan >> 3));

	/* Reports the input_is_key_down result. */
	return (bits >> (scan & 7)) & 1;
}

/* Implements input_drain(). */
static void
input_drain(
	void)
{
	/* Continues input_drain processing while work remains. */
	while (kbhit())
		(void)getch();
}

/*
 * Core-Graph aperture.
 *
 * Protected mode under DOS/4GW does not map the board aperture, so
 * ask DPMI (INT 31h, AX=0800h "Physical Address Mapping") for a
 * linear view of it.  DOS/4GW runs a zero-based flat model, so the
 * linear address it returns is usable as a near pointer.  A machine
 * without the board, or a DPMI host that refuses the mapping, yields
 * NULL, and the display selector then falls back to the GDC.
 */
static volatile uint8_t *
map_cirrus_aperture(
	void)
{
	union REGS regs;
	unsigned long size;
	unsigned long linear;

	size = CIRRUS_VISIBLE_BYTES;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0800;
	regs.w.bx = (unsigned short)(CIRRUS_PHYS_APERTURE >> 16);
	regs.w.cx = (unsigned short)(CIRRUS_PHYS_APERTURE & 0xffff);
	regs.w.si = (unsigned short)(size >> 16);
	regs.w.di = (unsigned short)(size & 0xffff);
	int386(0x31, &regs, &regs);

	/* Handles the next map_cirrus_aperture decision. */
	if (regs.w.cflag != 0)
		return NULL;
	linear = ((unsigned long)regs.w.bx << 16) | regs.w.cx;

	/* Handles the next map_cirrus_aperture decision. */
	if (linear == 0)
		return NULL;

	/* Reports the map_cirrus_aperture result. */
	return (volatile uint8_t *)linear;
}
