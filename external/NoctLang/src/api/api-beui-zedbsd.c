/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The zedBSD BeUI backend and evdev input implementation.
 */

#include <noct/noct.h>

#include <zedbsd/graphics.h>
#include <zedbsd/input.h>

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define INPUT_MAX_SOURCES	16U
#define INPUT_BITS_PER_WORD	(sizeof(unsigned long) * 8U)

#define INPUT_WORDS(maximum)	(((maximum) + 1U + INPUT_BITS_PER_WORD - 1U) / INPUT_BITS_PER_WORD)

#define INPUT_EVENT_WORDS	INPUT_WORDS(EV_MAX)
#define INPUT_KEY_WORDS		INPUT_WORDS(KEY_MAX)
#define INPUT_REL_WORDS		INPUT_WORDS(REL_MAX)
#define INPUT_ABS_WORDS		INPUT_WORDS(ABS_MAX)

#define IMAGE_SOURCE_MAX	(2U * 1024U * 1024U)
#define IMAGE_PIXELS_MAX	(2U * 1024U * 1024U)

#define READ_EVENTS		16U
#define FLUSH_RECTS		32U
#define GLYPH_BYTES		256U
#define RESCAN_MS		1000U
#define BITS_PER_ULONG		((unsigned)(sizeof(unsigned long) * 8U))

#define BIT_WORDS(maximum)	(((unsigned)(maximum) + BITS_PER_ULONG) / BITS_PER_ULONG)

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
 * Key codes exposed through BeUI.Key are identical on every backend.
 */
enum key_code {
	BEUI_KEY_ESCAPE = 0x1b,
	BEUI_KEY_BACKSPACE = 0x08,
	BEUI_KEY_TAB = 0x09,
	BEUI_KEY_ENTER = 0x0d,
	BEUI_KEY_PAGE_UP = 0x136,
	BEUI_KEY_PAGE_DOWN = 0x137,
	BEUI_KEY_INSERT = 0x138,
	BEUI_KEY_DELETE = 0x139,
	BEUI_KEY_UP = 0x13a,
	BEUI_KEY_LEFT = 0x13b,
	BEUI_KEY_RIGHT = 0x13c,
	BEUI_KEY_DOWN = 0x13d,
	BEUI_KEY_HOME = 0x13e,
	BEUI_KEY_END = 0x13f,
	BEUI_KEY_F1 = 0x162,
	BEUI_KEY_F2 = 0x163,
	BEUI_KEY_F3 = 0x164,
	BEUI_KEY_F4 = 0x165,
	BEUI_KEY_F5 = 0x166,
	BEUI_KEY_F6 = 0x167,
	BEUI_KEY_F7 = 0x168,
	BEUI_KEY_F8 = 0x169,
	BEUI_KEY_F9 = 0x16a,
	BEUI_KEY_F10 = 0x16b,

	/* State-only: modifiers never appear in buffered key streams. */
	BEUI_KEY_SHIFT = 0x170
};

enum input_role {
	INPUT_ROLE_NONE = 0,
	INPUT_ROLE_KEYBOARD = 1U << 0,
	INPUT_ROLE_RELATIVE_POINTER = 1U << 1,
	INPUT_ROLE_ABSOLUTE_POINTER = 1U << 2
};

enum input_update {
	INPUT_UPDATE_NONE = 0,
	INPUT_UPDATE_KEY = 1U << 0,
	INPUT_UPDATE_POINTER = 1U << 1,

	/* Committed state was visibly cleared after SYN_DROPPED. */
	INPUT_UPDATE_RESET = 1U << 2,

	/* Query EVIOCGKEY/EVIOCGABS and call resync() for this source. */
	INPUT_UPDATE_RESYNC = 1U << 3
};

/*
 * Defines the values used by the BeUI implementation.
 */
struct rect {
	unsigned x;
	unsigned y;
	unsigned width;
	unsigned height;
};

struct display_info {
	/* Input hint to enter(), zero means the backend's default depth. */
	unsigned preferred_bits_per_pixel;
	unsigned width;
	unsigned height;
	unsigned bits_per_pixel;
	unsigned stride;
};

/*
 * Images use a target-independent representation.  Indexed images
 * always contain one palette index per pixel, RGB24 pixels are
 * tightly packed in R, G, B order.  Palette entries and solid colors
 * use 0x00RRGGBB.
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
 * Pointer positions are absolute display coordinates.  Relative
 * zedBSD input is integrated and clamped here so scripts see one
 * coordinate space for every input device.
 */
struct pointer_event {
	unsigned x;
	unsigned y;
	unsigned buttons;
};

struct input_capabilities {
	unsigned long event_bits[INPUT_EVENT_WORDS];
	unsigned long key_bits[INPUT_KEY_WORDS];
	unsigned long relative_bits[INPUT_REL_WORDS];
	unsigned long absolute_bits[INPUT_ABS_WORDS];
	struct input_absinfo absolute_x;
	struct input_absinfo absolute_y;
};

struct input_source {
	int active;
	unsigned roles;
	int synchronization_lost;
	struct input_capabilities capabilities;
	unsigned long keys[INPUT_KEY_WORDS];
	unsigned long staged_keys[INPUT_KEY_WORDS];
	int64_t relative_x;
	int64_t relative_y;
	int32_t absolute_x;
	int32_t absolute_y;
	int32_t staged_absolute_x;
	int32_t staged_absolute_y;
	int staged_absolute_x_valid;
	int staged_absolute_y_valid;
	unsigned char partial_event[sizeof(struct input_event)];
	size_t partial_event_size;
};

struct input_state {
	struct input_source
	    sources[INPUT_MAX_SOURCES];
	unsigned display_width;
	unsigned display_height;
	unsigned pointer_x;
	unsigned pointer_y;
	unsigned pointer_buttons;
	int pointer_changed;
};

/*
 * A decoded image lives until the script destroys it or the VM shuts
 * down.  Pixels are appended to the entry so one allocation covers
 * both.
 */
struct image_entry {
	struct image_entry *next;
	int handle;
	struct image image;
	uint8_t pixels[1];
};

struct ui_state {
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

/* BeUI bitmap decoder. */
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

struct input_device {
	int fd;
	char event_name[32];
	int engine_slot;
};

struct backend_context {
	int graphics_fd;
	uint32_t graphics_capabilities;
	struct display_info display;
	struct input_state input;
	struct input_device sources[INPUT_MAX_SOURCES];
	uint64_t next_rescan;
	int input_initialized;
};

static struct ui_state state;

/* Key names exposed through the BeUI.Key dictionary. */
static const struct int_constant keys[] = {
	{"Escape", BEUI_KEY_ESCAPE},
	{"Tab", BEUI_KEY_TAB},
	{"Enter", BEUI_KEY_ENTER},
	{"Backspace", BEUI_KEY_BACKSPACE},
	{"Delete", BEUI_KEY_DELETE},
	{"Insert", BEUI_KEY_INSERT},
	{"Up", BEUI_KEY_UP},
	{"Down", BEUI_KEY_DOWN},
	{"Left", BEUI_KEY_LEFT},
	{"Right", BEUI_KEY_RIGHT},
	{"Home", BEUI_KEY_HOME},
	{"End", BEUI_KEY_END},
	{"PageUp", BEUI_KEY_PAGE_UP},
	{"PageDown", BEUI_KEY_PAGE_DOWN},
	{"F1", BEUI_KEY_F1}, {"F2", BEUI_KEY_F2},
	{"F3", BEUI_KEY_F3}, {"F4", BEUI_KEY_F4},
	{"F5", BEUI_KEY_F5}, {"F6", BEUI_KEY_F6},
	{"F7", BEUI_KEY_F7}, {"F8", BEUI_KEY_F8},
	{"F9", BEUI_KEY_F9}, {"F10", BEUI_KEY_F10},
	{"Space", ' '},
	{"Shift", BEUI_KEY_SHIFT},
};

/* Bit values returned by BeUI.getPointerButtons. */
static const struct int_constant buttons[] = {
	{"Left", BUTTON_LEFT},
	{"Right", BUTTON_RIGHT},
	{"Middle", BUTTON_MIDDLE},
};

static struct backend_context backend;

static int bit_is_set(const unsigned long *bits, unsigned maximum, unsigned bit);
static void bit_set(unsigned long *bits, unsigned maximum, unsigned bit);
static void bit_clear(unsigned long *bits, unsigned maximum, unsigned bit);
static int is_keyboard_code(unsigned code);
static int bitmap_any(const unsigned long *bits, size_t word_count);
static int source_has_pointer(const struct input_source *source);
static int input_has_pointer(const struct input_state *input);
static unsigned aggregate_buttons(const struct input_state *input);
static unsigned refresh_buttons(struct input_state *input);
static unsigned clamp_coordinate(int64_t coordinate, unsigned extent);
static unsigned center_pointer(struct input_state *input);
static unsigned apply_relative(unsigned coordinate, int64_t delta, unsigned extent);
static unsigned scale_absolute(int32_t value, const struct input_absinfo *information, unsigned extent);
static int64_t saturating_add(int64_t left, int32_t right);
static void input_capabilities_clear(struct input_capabilities *capabilities);
static void input_capabilities_set_event(struct input_capabilities *capabilities, unsigned event_type);
static void input_capabilities_set_key(struct input_capabilities *capabilities, unsigned code);
static void input_capabilities_set_relative(struct input_capabilities *capabilities, unsigned axis);
static void input_capabilities_set_absolute(struct input_capabilities *capabilities, unsigned axis, const struct input_absinfo *information);
static unsigned input_classify(const struct input_capabilities *capabilities);
static void input_init(struct input_state *input, unsigned display_width, unsigned display_height);
static void input_set_display(struct input_state *input, unsigned display_width, unsigned display_height);
static void input_reset(struct input_state *input);
static int input_attach(struct input_state *input, const struct input_capabilities *capabilities);
static unsigned input_detach(struct input_state *input, unsigned source_index);
static unsigned reset_source_after_drop(struct input_state *input, struct input_source *source);
static unsigned commit_source(struct input_state *input, struct input_source *source);
static unsigned process_event(struct input_state *input, struct input_source *source, const struct input_event *event);
static unsigned input_feed(struct input_state *input, unsigned source_index, const void *bytes, size_t byte_count);
static unsigned input_resync(struct input_state *input, unsigned source_index, const void *key_bits, size_t key_byte_count, const struct input_absinfo *absolute_x, const struct input_absinfo *absolute_y);
static int key_to_evdev(int key);
static int source_key_state(const struct input_state *input, unsigned code, int alternate_code);
static int input_is_key_down(const struct input_state *input, int beui_key);
static int input_poll_pointer(struct input_state *input, struct pointer_event *event);
static int init_ui(void);
static int init_ui_with_hint(unsigned preferred_bits_per_pixel);
static void close_ui(void);
static void cleanup_ui(void);
static int is_ui_open(void);
static int get_display_info(struct display_info *info);
static int fill(const struct rect *rect, uint32_t color);
static int draw_line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int pattern_fill(const struct rect *rect, uint32_t color, uint64_t pattern);
static int image_valid(const struct image *image);
static int draw_image(unsigned x, unsigned y, const struct image *image);
static int draw_image_region(const struct image *image, unsigned source_x, unsigned source_y, unsigned width, unsigned height, unsigned destination_x, unsigned destination_y);
static int draw_image_pattern(unsigned x, unsigned y, const struct image *image, uint64_t pattern);
static uint32_t decode_utf8(const char **cursor);
static int measure_text(const char *text, unsigned *width, unsigned *height);
static int draw_text(const char *text, unsigned x, unsigned y, uint32_t foreground, uint32_t background);
static int poll_ui(void);
static int get_pointer(unsigned *x, unsigned *y, unsigned *buttons);
static int flush(void);
static int get_milliseconds(uint64_t *milliseconds);
static int sleep_milliseconds(unsigned milliseconds);
static int is_key_down(int key);
static void drain_input(void);
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
static bool register_api(NoctEnv *env);
static int uapi_bit_is_set(const unsigned long *bits, unsigned bit);
static int graphics_has(const struct backend_context *context, uint32_t capability);
static void copy_rect(struct graphics_rect *to, const struct rect *from);
static int zedbsd_display_enter(struct backend_context *context, struct display_info *info);
static void zedbsd_display_leave(struct backend_context *context);
static int zedbsd_display_poll(struct backend_context *context);
static int zedbsd_display_fill(struct backend_context *context, const struct rect *rect, uint32_t color);
static int zedbsd_display_line(struct backend_context *context, unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint32_t color);
static int zedbsd_display_pattern(struct backend_context *context, const struct rect *rect, uint32_t color, uint64_t pattern);
static int zedbsd_display_image_common(struct backend_context *context, unsigned x, unsigned y, const struct image *image, uint64_t pattern, int patterned);
static int zedbsd_display_image(struct backend_context *context, unsigned x, unsigned y, const struct image *image);
static int zedbsd_display_image_pattern(struct backend_context *context, unsigned x, unsigned y, const struct image *image, uint64_t pattern);
static int zedbsd_display_flush(struct backend_context *context, const struct rect *rectangles, size_t rectangle_count);
static int zedbsd_glyph_get(struct backend_context *context, uint32_t codepoint, struct graphics_glyph *glyph, uint8_t *bitmap, size_t capacity);
static int zedbsd_glyph_measure(struct backend_context *context, uint32_t codepoint, unsigned *width, unsigned *height);
static int zedbsd_glyph_draw(struct backend_context *context, unsigned x, unsigned y, uint32_t codepoint, uint32_t foreground, uint32_t background);
static uint64_t zedbsd_milliseconds(struct backend_context *context);
static int zedbsd_input_query_capabilities(int fd, struct input_capabilities *capabilities);
static unsigned zedbsd_input_resync_source(struct backend_context *context, unsigned source_index);
static int zedbsd_input_event_name(const char *name);
static int zedbsd_input_has_event_name(const struct backend_context *context, const char *event_name);
static void zedbsd_input_discover(struct backend_context *context, int force);
static void zedbsd_input_detach_source(struct backend_context *context, unsigned source_index);
static void zedbsd_input_service_source(struct backend_context *context, unsigned source_index);
static void zedbsd_input_service(struct backend_context *context);
static void zedbsd_input_close(struct backend_context *context);
static int zedbsd_pointer_start(struct backend_context *context, const struct display_info *display);
static int zedbsd_pointer_poll(struct backend_context *context, struct pointer_event *event);
static int zedbsd_key_state(struct backend_context *context, int key);
static void zedbsd_input_drain(struct backend_context *context);

/*
 * Registers the BeUI API.
 */
NOCT_DLL
bool
noct_register_api_beui(
	NoctEnv *env)
{
	bool call_result;
	unsigned index;

	/* Releases resources owned by a previous registration. */
	cleanup_ui();

	/* Initializes the zedBSD backend descriptor sentinels. */
	memset(&backend, 0, sizeof(backend));
	backend.graphics_fd = -1;
	for (index = 0; index < INPUT_MAX_SOURCES; index++) {
		backend.sources[index].fd = -1;
		backend.sources[index].engine_slot = -1;
	}

	/* Registers the BeUI functions and constants. */
	call_result = register_api(env);

	/* Reports whether the API was registered. */
	return call_result;
}

/* Implements bit_is_set(). */
static int
bit_is_set(
	const unsigned long *bits,
	unsigned maximum,
	unsigned bit)
{
	/* Handles the next bit_is_set decision. */
	if (bits == NULL || bit > maximum)
		return 0;

	/* Reports the bit_is_set result. */
	return (bits[bit / INPUT_BITS_PER_WORD] &
		(1UL << (bit % INPUT_BITS_PER_WORD))) != 0;
}

/* Implements bit_set(). */
static void
bit_set(
	unsigned long *bits,
	unsigned maximum,
	unsigned bit)
{
	/* Handles the next bit_set decision. */
	if (bits != NULL && bit <= maximum) {
		bits[bit / INPUT_BITS_PER_WORD] |=
		    1UL << (bit % INPUT_BITS_PER_WORD);
	}
}

/* Implements bit_clear(). */
static void
bit_clear(
	unsigned long *bits,
	unsigned maximum,
	unsigned bit)
{
	/* Handles the next bit_clear decision. */
	if (bits != NULL && bit <= maximum) {
		bits[bit / INPUT_BITS_PER_WORD] &=
		    ~(1UL << (bit % INPUT_BITS_PER_WORD));
	}
}

/* Implements is_keyboard_code(). */
static int
is_keyboard_code(
	unsigned code)
{
	/* Rejects the reserved key-code floor. */
	if (code <= KEY_RESERVED)
		return 0;

	/* Reports whether the code precedes the pointer-button range. */
	return code < BTN_MOUSE;
}

/* Implements bitmap_any(). */
static int
bitmap_any(
	const unsigned long *bits,
	size_t word_count)
{
	size_t i;

	/* Processes each bitmap_any item. */
	for (i = 0; i < word_count; i++) {
		/* Handles the next bitmap_any decision. */
		if (bits[i] != 0UL)
			return 1;
	}

	/* Reports the bitmap_any result. */
	return 0;
}

/* Implements source_has_pointer(). */
static int
source_has_pointer(
	const struct input_source *source)
{
	/* Requires an attached source before inspecting its roles. */
	if (!source->active)
		return 0;

	/* Reports whether the source exposes either pointer role. */
	return (source->roles &
		(INPUT_ROLE_RELATIVE_POINTER |
		 INPUT_ROLE_ABSOLUTE_POINTER)) != 0U;
}

/* Implements input_has_pointer(). */
static int
input_has_pointer(
	const struct input_state *input)
{
	unsigned i;

	/* Processes each input_has_pointer item. */
	for (i = 0; i < INPUT_MAX_SOURCES; i++) {
		/* Handles the next input_has_pointer decision. */
		if (source_has_pointer(&input->sources[i]))
			return 1;
	}

	/* Reports the input_has_pointer result. */
	return 0;
}

/* Implements aggregate_buttons(). */
static unsigned
aggregate_buttons(
	const struct input_state *input)
{
	const struct input_source *source;
	unsigned buttons;
	unsigned i;

	buttons = 0;

	/* Processes each aggregate_buttons item. */
	for (i = 0; i < INPUT_MAX_SOURCES; i++) {
		source = &input->sources[i];

		/* Handles the next aggregate_buttons decision. */
		if (!source_has_pointer(source))
			continue;

		/* Handles the next aggregate_buttons decision. */
		if (bit_is_set(source->keys, KEY_MAX, BTN_LEFT))
			buttons |= BUTTON_LEFT;

		/* Handles the next aggregate_buttons decision. */
		if (bit_is_set(source->keys, KEY_MAX, BTN_RIGHT))
			buttons |= BUTTON_RIGHT;

		/* Handles the next aggregate_buttons decision. */
		if (bit_is_set(source->keys, KEY_MAX, BTN_MIDDLE))
			buttons |= BUTTON_MIDDLE;
	}

	/* Reports the aggregate_buttons result. */
	return buttons;
}

/* Implements refresh_buttons(). */
static unsigned
refresh_buttons(
	struct input_state *input)
{
	unsigned buttons;

	buttons = aggregate_buttons(input);

	/* Handles the next refresh_buttons decision. */
	if (buttons == input->pointer_buttons)
		return INPUT_UPDATE_NONE;
	input->pointer_buttons = buttons;
	input->pointer_changed = 1;

	/* Reports the refresh_buttons result. */
	return INPUT_UPDATE_POINTER;
}

/* Implements clamp_coordinate(). */
static unsigned
clamp_coordinate(
	int64_t coordinate,
	unsigned extent)
{
	/* Handles the next clamp_coordinate decision. */
	if (extent == 0U || coordinate <= 0)
		return 0U;

	/* Handles the next clamp_coordinate decision. */
	if ((uint64_t)coordinate >= (uint64_t)extent)
		return extent - 1U;

	/* Reports the clamp_coordinate result. */
	return (unsigned)coordinate;
}

/* Implements center_pointer(). */
static unsigned
center_pointer(
	struct input_state *input)
{
	unsigned x;
	unsigned y;

	x = input->display_width == 0U ? 0U : input->display_width / 2U;
	y = input->display_height == 0U ? 0U : input->display_height / 2U;

	/* Handles the next center_pointer decision. */
	if (input->pointer_x == x && input->pointer_y == y)
		return INPUT_UPDATE_NONE;
	input->pointer_x = x;
	input->pointer_y = y;
	input->pointer_changed = 1;

	/* Reports the center_pointer result. */
	return INPUT_UPDATE_POINTER;
}

/* Implements apply_relative(). */
static unsigned
apply_relative(
	unsigned coordinate,
	int64_t delta,
	unsigned extent)
{
	uint64_t magnitude;

	/* Handles the next apply_relative decision. */
	if (extent == 0U)
		return 0U;
	coordinate = clamp_coordinate(coordinate, extent);

	/* Handles the next apply_relative decision. */
	if (delta >= 0) {
		/* Handles the next apply_relative decision. */
		if ((uint64_t)delta >= (uint64_t)(extent - 1U - coordinate))
			return extent - 1U;

		/* Reports the apply_relative result. */
		return coordinate + (unsigned)delta;
	}
	/* -(INT64_MIN) is not representable, so form the magnitude without
	 * negating that value directly. */
	magnitude = (uint64_t)(-(delta + 1)) + 1U;

	/* Handles the next apply_relative decision. */
	if (magnitude >= coordinate)
		return 0U;

	/* Reports the apply_relative result. */
	return coordinate - (unsigned)magnitude;
}

/* Implements scale_absolute(). */
static unsigned
scale_absolute(
	int32_t value,
	const struct input_absinfo *information,
	unsigned extent)
{
	int64_t offset;
	uint64_t span;
	uint64_t scaled;

	/* Rejects coordinates without a usable source or destination range. */
	if (extent <= 1U ||
	    information == NULL ||
	    information->maximum <= information->minimum)
		return 0U;

	/* Handles the next scale_absolute decision. */
	if (value <= information->minimum)
		return 0U;

	/* Handles the next scale_absolute decision. */
	if (value >= information->maximum)
		return extent - 1U;
	offset = (int64_t)value - information->minimum;
	span = (uint64_t)((int64_t)information->maximum - information->minimum);
	scaled = (uint64_t)offset * (uint64_t)(extent - 1U);

	/* Reports the scale_absolute result. */
	return (unsigned)(scaled / span);
}

/* Implements saturating_add(). */
static int64_t
saturating_add(
	int64_t left,
	int32_t right)
{
	/* Handles the next saturating_add decision. */
	if (right > 0 && left > INT64_MAX - right)
		return INT64_MAX;

	/* Handles the next saturating_add decision. */
	if (right < 0 && left < INT64_MIN - right)
		return INT64_MIN;

	/* Reports the saturating_add result. */
	return left + right;
}

/* Implements input_capabilities_clear(). */
static void
input_capabilities_clear(
	struct input_capabilities *capabilities)
{
	/* Handles the next input_capabilities_clear decision. */
	if (capabilities != NULL)
		memset(capabilities, 0, sizeof(*capabilities));
}

/* Implements input_capabilities_set_event(). */
static void
input_capabilities_set_event(
	struct input_capabilities *capabilities,
	unsigned event_type)
{
	/* Handles the next input_capabilities_set_event decision. */
	if (capabilities != NULL)
		bit_set(capabilities->event_bits, EV_MAX, event_type);
}

/* Implements input_capabilities_set_key(). */
static void
input_capabilities_set_key(
	struct input_capabilities *capabilities,
	unsigned code)
{
	/* Handles the next input_capabilities_set_key decision. */
	if (capabilities == NULL)
		return;
	bit_set(capabilities->event_bits, EV_MAX, EV_KEY);
	bit_set(capabilities->key_bits, KEY_MAX, code);
}

/* Implements input_capabilities_set_relative(). */
static void
input_capabilities_set_relative(
	struct input_capabilities *capabilities,
	unsigned axis)
{
	/* Handles the next input_capabilities_set_relative decision. */
	if (capabilities == NULL)
		return;
	bit_set(capabilities->event_bits, EV_MAX, EV_REL);
	bit_set(capabilities->relative_bits, REL_MAX, axis);
}

/* Implements input_capabilities_set_absolute(). */
static void
input_capabilities_set_absolute(
	struct input_capabilities *capabilities,
	unsigned axis,
	const struct input_absinfo *information)
{
	/* Handles the next input_capabilities_set_absolute decision. */
	if (capabilities == NULL)
		return;
	bit_set(capabilities->event_bits, EV_MAX, EV_ABS);
	bit_set(capabilities->absolute_bits, ABS_MAX, axis);

	/* Handles the next input_capabilities_set_absolute decision. */
	if (information == NULL)
		return;

	/* Handles the next input_capabilities_set_absolute decision. */
	if (axis == ABS_X)
		capabilities->absolute_x = *information;
	else if (axis == ABS_Y)
		capabilities->absolute_y = *information;
}

/* Implements input_classify(). */
static unsigned
input_classify(
	const struct input_capabilities *capabilities)
{
	unsigned code;
	unsigned roles;

	/* Requires a capability record before inspecting its event bits. */
	if (capabilities == NULL)
		return INPUT_ROLE_NONE;

	/*
	 * EVIOCGBIT(0) is the event-type bitmap.  Like Linux evdev it
	 * does not provide a distinct query for SYN codes; zedBSD
	 * registration already requires SYN_REPORT for every input
	 * device.
	 */
	if (!bit_is_set(capabilities->event_bits, EV_MAX, EV_SYN))
		return INPUT_ROLE_NONE;
	roles = INPUT_ROLE_NONE;

	/* Handles the next input_classify decision. */
	if (bit_is_set(capabilities->event_bits, EV_MAX, EV_KEY)) {
		/* Processes each input_classify item. */
		for (code = KEY_RESERVED + 1U; code < BTN_MOUSE; code++) {
			/* Selects only key codes exposed through the BeUI API. */
			if (is_keyboard_code(code)) {
				/* Detects support for the selected key code. */
				if (bit_is_set(
					capabilities->key_bits,
					KEY_MAX,
					code)) {
					roles |=
						INPUT_ROLE_KEYBOARD;
					break;
				}
			}
		}
	}

	/* Detects a relative pointer with both coordinate axes. */
	if (bit_is_set(capabilities->event_bits, EV_MAX, EV_REL)) {
		/* Requires the relative X axis. */
		if (bit_is_set(capabilities->relative_bits, REL_MAX, REL_X)) {
			/* Requires the relative Y axis. */
			if (bit_is_set(
				capabilities->relative_bits,
				REL_MAX,
				REL_Y)) {
				roles |=
					INPUT_ROLE_RELATIVE_POINTER;
			}
		}
	}

	/* Detects an absolute pointer with both coordinate axes. */
	if (bit_is_set(capabilities->event_bits, EV_MAX, EV_ABS)) {
		/* Requires the absolute X axis. */
		if (bit_is_set(capabilities->absolute_bits, ABS_MAX, ABS_X)) {
			/* Requires the absolute Y axis. */
			if (bit_is_set(
				capabilities->absolute_bits,
				ABS_MAX,
				ABS_Y)) {
				roles |=
					INPUT_ROLE_ABSOLUTE_POINTER;
			}
		}
	}

	/* Reports the input_classify result. */
	return roles;
}

/* Implements input_init(). */
static void
input_init(
	struct input_state *input,
	unsigned display_width,
	unsigned display_height)
{
	/* Handles the next input_init decision. */
	if (input == NULL)
		return;

	memset(input, 0, sizeof(*input));
	input->display_width = display_width;
	input->display_height = display_height;
	input->pointer_x = display_width == 0U ? 0U : display_width / 2U;
	input->pointer_y = display_height == 0U ? 0U : display_height / 2U;
}

/* Implements input_set_display(). */
static void
input_set_display(
	struct input_state *input,
	unsigned display_width,
	unsigned display_height)
{
	unsigned old_x;
	unsigned old_y;
	unsigned old_width;
	unsigned old_height;

	/* Handles the next input_set_display decision. */
	if (input == NULL)
		return;

	old_x = input->pointer_x;
	old_y = input->pointer_y;
	old_width = input->display_width;
	old_height = input->display_height;
	input->display_width = display_width;
	input->display_height = display_height;
	input->pointer_x = old_width == 0U && display_width != 0U ?
		display_width / 2U :
		clamp_coordinate(input->pointer_x, display_width);
	input->pointer_y = old_height == 0U && display_height != 0U ?
		display_height / 2U :
		clamp_coordinate(input->pointer_y, display_height);

	/* Handles the next input_set_display decision. */
	if (old_x != input->pointer_x || old_y != input->pointer_y)
		input->pointer_changed = 1;
}

/* Implements input_reset(). */
static void
input_reset(
	struct input_state *input)
{
	unsigned width;
	unsigned height;

	/* Handles the next input_reset decision. */
	if (input == NULL)
		return;

	width = input->display_width;
	height = input->display_height;

	input_init(input, width, height);
}

/* Implements input_attach(). */
static int
input_attach(
	struct input_state *input,
	const struct input_capabilities *capabilities)
{
	struct input_source *source;
	unsigned roles;
	unsigned i;

	/* Handles the next input_attach decision. */
	if (input == NULL || capabilities == NULL)
		return -1;
	roles = input_classify(capabilities);

	/* Handles the next input_attach decision. */
	if (roles == INPUT_ROLE_NONE)
		return -1;

	/* Processes each input_attach item. */
	for (i = 0; i < INPUT_MAX_SOURCES; i++) {
		/* Handles the next input_attach decision. */
		if (!input->sources[i].active)
			break;
	}

	/* Handles the next input_attach decision. */
	if (i == INPUT_MAX_SOURCES)
		return -1;

	source = &input->sources[i];
	memset(source, 0, sizeof(*source));
	source->active = 1;
	source->roles = roles;
	source->capabilities = *capabilities;

	/* Handles the next input_attach decision. */
	if ((roles & INPUT_ROLE_ABSOLUTE_POINTER) != 0U) {
		source->absolute_x = capabilities->absolute_x.value;
		source->absolute_y = capabilities->absolute_y.value;
		source->staged_absolute_x = source->absolute_x;
		source->staged_absolute_y = source->absolute_y;
		input->pointer_x = scale_absolute(
			source->absolute_x,
			&capabilities->absolute_x,
			input->display_width);
		input->pointer_y = scale_absolute(
			source->absolute_y,
			&capabilities->absolute_y,
			input->display_height);
	}

	/* Handles the next input_attach decision. */
	if (source_has_pointer(source))
		input->pointer_changed = 1;

	/* Reports the input_attach result. */
	return (int)i;
}

/* Implements input_detach(). */
static unsigned
input_detach(
	struct input_state *input,
	unsigned source_index)
{
	struct input_source *source;
	unsigned result;
	int had_keys;
	int had_pointer;

	/* Handles the next input_detach decision. */
	if (input == NULL || source_index >= INPUT_MAX_SOURCES)
		return INPUT_UPDATE_NONE;
	source = &input->sources[source_index];

	/* Handles the next input_detach decision. */
	if (!source->active)
		return INPUT_UPDATE_NONE;
	had_keys = bitmap_any(source->keys, INPUT_KEY_WORDS);
	had_pointer = source_has_pointer(source);
	memset(source, 0, sizeof(*source));
	result = had_keys ? INPUT_UPDATE_KEY
			  : INPUT_UPDATE_NONE;
	result |= refresh_buttons(input);

	/* Checks whether detaching a pointer removed the final pointer source. */
	if (had_pointer) {
		/* Detects removal of the final pointer source. */
		if (!input_has_pointer(input)) {
			input->pointer_buttons = 0U;
			result |= center_pointer(input);

			/*
			 * Publish one final all-up/centered state
			 * even though no source remains.  This
			 * prevents a button held at detach from
			 * becoming permanently stuck in the BeUI
			 * core.
			 */
			input->pointer_changed = 1;
			result |= INPUT_UPDATE_POINTER;
		}
	}

	/* Reports the input_detach result. */
	return result;
}

/* Implements reset_source_after_drop(). */
static unsigned
reset_source_after_drop(
	struct input_state *input,
	struct input_source *source)
{
	unsigned result;
	int had_keys;

	had_keys = bitmap_any(source->keys, INPUT_KEY_WORDS);
	memset(source->keys, 0, sizeof(source->keys));
	memset(source->staged_keys, 0, sizeof(source->staged_keys));
	source->relative_x = 0;
	source->relative_y = 0;
	source->staged_absolute_x_valid = 0;
	source->staged_absolute_y_valid = 0;
	source->synchronization_lost = 1;
	result = INPUT_UPDATE_RESET;

	/* Handles the next reset_source_after_drop decision. */
	if (had_keys)
		result |= INPUT_UPDATE_KEY;

	result |= refresh_buttons(input);

	/*
	 * Relative coordinates cannot be queried back after queue
	 * loss.  A centered state is the deterministic, visible reset
	 * from which later deltas continue.
	 */
	if ((source->roles & INPUT_ROLE_RELATIVE_POINTER) !=
	    0U)
		result |= center_pointer(input);

	/* Reports the reset_source_after_drop result. */
	return result;
}

/* Implements commit_source(). */
static unsigned
commit_source(
	struct input_state *input,
	struct input_source *source)
{
	unsigned old_x;
	unsigned old_y;
	unsigned result;
	int keys_changed;

	old_x = input->pointer_x;
	old_y = input->pointer_y;

	keys_changed = memcmp(source->keys, source->staged_keys, sizeof(source->keys)) != 0;

	memcpy(source->keys, source->staged_keys, sizeof(source->keys));

	/* Handles the next commit_source decision. */
	if ((source->roles & INPUT_ROLE_ABSOLUTE_POINTER) !=
	    0U) {
		/* Handles the next commit_source decision. */
		if (source->staged_absolute_x_valid)
			source->absolute_x = source->staged_absolute_x;

		/* Handles the next commit_source decision. */
		if (source->staged_absolute_y_valid)
			source->absolute_y = source->staged_absolute_y;

		/* Handles the next commit_source decision. */
		if (source->staged_absolute_x_valid ||
		    source->staged_absolute_y_valid) {
			input->pointer_x =
			    scale_absolute(
				source->absolute_x,
				&source->capabilities.absolute_x,
				input->display_width);
			input->pointer_y =
			    scale_absolute(
				source->absolute_y,
				&source->capabilities.absolute_y,
				input->display_height);
		}
	}

	/* Handles the next commit_source decision. */
	if ((source->roles & INPUT_ROLE_RELATIVE_POINTER) !=
	    0U) {
		input->pointer_x = apply_relative(
			input->pointer_x,
			source->relative_x,
			input->display_width);
		input->pointer_y =
		    apply_relative(
			input->pointer_y,
			source->relative_y,
			input->display_height);
	}
	source->relative_x = 0;
	source->relative_y = 0;
	source->staged_absolute_x_valid = 0;
	source->staged_absolute_y_valid = 0;
	result = keys_changed ? INPUT_UPDATE_KEY
			      : INPUT_UPDATE_NONE;
	result |= refresh_buttons(input);

	/* Handles the next commit_source decision. */
	if (old_x != input->pointer_x || old_y != input->pointer_y) {
		input->pointer_changed = 1;
		result |= INPUT_UPDATE_POINTER;
	}

	/* Reports the commit_source result. */
	return result;
}

/* Implements process_event(). */
static unsigned
process_event(
	struct input_state *input,
	struct input_source *source,
	const struct input_event *event)
{
	unsigned call_result;
	int matches_event;

	/* Handles the next process_event decision. */
	if (event->type == EV_SYN && event->code == SYN_DROPPED) {
		call_result = reset_source_after_drop(input, source);

		/* Reports the process_event result. */
		return call_result;
	}

	/* Handles the next process_event decision. */
	if (source->synchronization_lost) {
		/* Handles the next process_event decision. */
		if (event->type == EV_SYN && event->code == SYN_REPORT)
			return INPUT_UPDATE_RESYNC;

		/* Reports the process_event result. */
		return INPUT_UPDATE_NONE;
	}

	/* Handles the next process_event decision. */
	if (event->type == EV_SYN) {
		/* Handles the next process_event decision. */
		if (event->code == SYN_REPORT) {
			call_result = commit_source(input, source);

			/* Reports the process_event result. */
			return call_result;
		}

		/* Reports the process_event result. */
		return INPUT_UPDATE_NONE;
	}

	/* Matches a supported key event in short-circuit order. */
	matches_event = 0;
	if (event->type == EV_KEY && event->code <= KEY_MAX) {
		/* Requires key-event support from the source. */
		if (bit_is_set(
			source->capabilities.event_bits,
			EV_MAX,
			EV_KEY)) {
			/* Requires support for the specific key code. */
			if (bit_is_set(
				source->capabilities.key_bits,
				KEY_MAX,
				event->code))
				matches_event = 1;
		}
	}

	/* Processes a fully supported key event. */
	if (matches_event) {
		/* Handles the next process_event decision. */
		if (event->value == 0)
			bit_clear(source->staged_keys, KEY_MAX, event->code);
		else if (event->value == 1 || event->value == 2)
			bit_set(source->staged_keys, KEY_MAX, event->code);

		/* Reports the process_event result. */
		return INPUT_UPDATE_NONE;
	}

	/* Matches a supported relative-axis event in short-circuit order. */
	matches_event = 0;
	if (event->type == EV_REL && event->code <= REL_MAX) {
		/* Requires relative-event support from the source. */
		if (bit_is_set(
			source->capabilities.event_bits,
			EV_MAX,
			EV_REL)) {
			/* Requires support for the specific relative axis. */
			if (bit_is_set(
				source->capabilities.relative_bits,
				REL_MAX,
				event->code))
				matches_event = 1;
		}
	}

	/* Processes a fully supported relative-axis event. */
	if (matches_event) {
		/* Handles the next process_event decision. */
		if (event->code == REL_X) {
			source->relative_x =
			    saturating_add(source->relative_x, event->value);
		} else if (event->code == REL_Y) {
			source->relative_y =
			    saturating_add(source->relative_y, event->value);
		}

		/* Reports the process_event result. */
		return INPUT_UPDATE_NONE;
	}

	/* Matches a supported absolute-axis event in short-circuit order. */
	matches_event = 0;
	if (event->type == EV_ABS && event->code <= ABS_MAX) {
		/* Requires absolute-event support from the source. */
		if (bit_is_set(
			source->capabilities.event_bits,
			EV_MAX,
			EV_ABS)) {
			/* Requires support for the specific absolute axis. */
			if (bit_is_set(
				source->capabilities.absolute_bits,
				ABS_MAX,
				event->code))
				matches_event = 1;
		}
	}

	/* Processes a fully supported absolute-axis event. */
	if (matches_event) {
		/* Handles the next process_event decision. */
		if (event->code == ABS_X) {
			source->staged_absolute_x = event->value;
			source->staged_absolute_x_valid = 1;
		} else if (event->code == ABS_Y) {
			source->staged_absolute_y = event->value;
			source->staged_absolute_y_valid = 1;
		}
	}

	/* Reports the process_event result. */
	return INPUT_UPDATE_NONE;
}

/* Implements input_feed(). */
static unsigned
input_feed(
	struct input_state *input,
	unsigned source_index,
	const void *bytes,
	size_t byte_count)
{
	struct input_source *source;
	struct input_event event;
	const unsigned char *cursor;
	size_t copy_size;
	unsigned result;

	/* Handles the next input_feed decision. */
	if (input == NULL ||
	    source_index >= INPUT_MAX_SOURCES ||
	    (bytes == NULL && byte_count != 0U))
		return INPUT_UPDATE_NONE;
	source = &input->sources[source_index];

	/* Handles the next input_feed decision. */
	if (!source->active)
		return INPUT_UPDATE_NONE;
	cursor = bytes;
	result = INPUT_UPDATE_NONE;

	/* Continues input_feed processing while work remains. */
	while (byte_count != 0U) {
		copy_size =
		    sizeof(source->partial_event) - source->partial_event_size;

		/* Handles the next input_feed decision. */
		if (copy_size > byte_count)
			copy_size = byte_count;
		memcpy(
			source->partial_event + source->partial_event_size,
			cursor,
			copy_size);
		source->partial_event_size += copy_size;
		cursor += copy_size;
		byte_count -= copy_size;

		/* Handles the next input_feed decision. */
		if (source->partial_event_size == sizeof(struct input_event)) {
			memcpy(&event, source->partial_event, sizeof(event));
			source->partial_event_size = 0U;
			result |= process_event(input, source, &event);
		}
	}

	/* Reports the input_feed result. */
	return result;
}

/* Implements input_resync(). */
static unsigned
input_resync(
	struct input_state *input,
	unsigned source_index,
	const void *key_bits,
	size_t key_byte_count,
	const struct input_absinfo *absolute_x,
	const struct input_absinfo *absolute_y)
{
	struct input_source *source;
	unsigned long queried[INPUT_KEY_WORDS];
	unsigned old_x;
	unsigned old_y;
	unsigned result;
	unsigned code;
	int down;
	int keys_changed;

	/* Handles the next input_resync decision. */
	if (input == NULL || source_index >= INPUT_MAX_SOURCES)
		return INPUT_UPDATE_NONE;
	source = &input->sources[source_index];

	/* Handles the next input_resync decision. */
	if (!source->active)
		return INPUT_UPDATE_NONE;
	memset(queried, 0, sizeof(queried));

	/* Handles the next input_resync decision. */
	if (key_bits != NULL) {
		/* Handles the next input_resync decision. */
		if (key_byte_count > sizeof(queried))
			key_byte_count = sizeof(queried);
		memcpy(queried, key_bits, key_byte_count);
	}
	keys_changed = 0;

	/* Processes each input_resync item. */
	for (code = 0; code <= KEY_MAX; code++) {
		/* Intersects queried state with the source capabilities. */
		down = bit_is_set(queried, KEY_MAX, code);
		if (down) {
			down = bit_is_set(
				source->capabilities.key_bits,
				KEY_MAX,
				code);
		}

		/* Handles the next input_resync decision. */
		if (down != bit_is_set(source->keys, KEY_MAX, code))
			keys_changed = 1;

		/* Handles the next input_resync decision. */
		if (down)
			bit_set(source->keys, KEY_MAX, code);
		else
			bit_clear(source->keys, KEY_MAX, code);
	}
	memcpy(source->staged_keys, source->keys, sizeof(source->keys));
	old_x = input->pointer_x;
	old_y = input->pointer_y;

	/* Refreshes a supported absolute X axis. */
	if (absolute_x != NULL) {
		/* Requires the source to expose the absolute X axis. */
		if (bit_is_set(
			source->capabilities.absolute_bits,
			ABS_MAX,
			ABS_X)) {
			source->capabilities.absolute_x = *absolute_x;
			source->absolute_x = absolute_x->value;
		}
	}

	/* Refreshes a supported absolute Y axis. */
	if (absolute_y != NULL) {
		/* Requires the source to expose the absolute Y axis. */
		if (bit_is_set(
			source->capabilities.absolute_bits,
			ABS_MAX,
			ABS_Y)) {
			source->capabilities.absolute_y = *absolute_y;
			source->absolute_y = absolute_y->value;
		}
	}

	/* Handles the next input_resync decision. */
	if ((source->roles & INPUT_ROLE_ABSOLUTE_POINTER) !=
		0U &&
	    (absolute_x != NULL || absolute_y != NULL)) {
		input->pointer_x = scale_absolute(
			source->absolute_x,
			&source->capabilities.absolute_x,
			input->display_width);
		input->pointer_y = scale_absolute(
			source->absolute_y,
			&source->capabilities.absolute_y,
			input->display_height);
	}
	source->relative_x = 0;
	source->relative_y = 0;
	source->staged_absolute_x_valid = 0;
	source->staged_absolute_y_valid = 0;
	source->synchronization_lost = 0;
	result = keys_changed ? INPUT_UPDATE_KEY
			      : INPUT_UPDATE_NONE;
	result |= refresh_buttons(input);

	/* Handles the next input_resync decision. */
	if (old_x != input->pointer_x || old_y != input->pointer_y) {
		input->pointer_changed = 1;
		result |= INPUT_UPDATE_POINTER;
	}

	/* Reports the input_resync result. */
	return result;
}

/* Implements key_to_evdev(). */
static int
key_to_evdev(
	int key)
{
	static const uint16_t letters[26] = {
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
		KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
		KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z};
	static const uint16_t digits[10] = {
		KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
		KEY_5, KEY_6, KEY_7, KEY_8, KEY_9};

	/* Handles the next key_to_evdev decision. */
	if (key >= 'a' && key <= 'z')
		return letters[key - 'a'];

	/* Handles the next key_to_evdev decision. */
	if (key >= '0' && key <= '9')
		return digits[key - '0'];

	/* Selects the matching key_to_evdev operation. */
	switch (key) {
	case ' ':
		/* Reports the key_to_evdev result. */
		return KEY_SPACE;
	case '-':
	case '_':
		/* Reports the key_to_evdev result. */
		return KEY_MINUS;
	case '=':
	case '+':
		/* Reports the key_to_evdev result. */
		return KEY_EQUAL;
	case '[':
	case '{':
		/* Reports the key_to_evdev result. */
		return KEY_LEFTBRACE;
	case ']':
	case '}':
		/* Reports the key_to_evdev result. */
		return KEY_RIGHTBRACE;
	case ';':
	case ':':
		/* Reports the key_to_evdev result. */
		return KEY_SEMICOLON;
	case '\'':
	case '"':
		/* Reports the key_to_evdev result. */
		return KEY_APOSTROPHE;
	case '`':
	case '~':
		/* Reports the key_to_evdev result. */
		return KEY_GRAVE;
	case '\\':
	case '|':
		/* Reports the key_to_evdev result. */
		return KEY_BACKSLASH;
	case ',':
	case '<':
		/* Reports the key_to_evdev result. */
		return KEY_COMMA;
	case '.':
	case '>':
		/* Reports the key_to_evdev result. */
		return KEY_DOT;
	case '/':
	case '?':
		/* Reports the key_to_evdev result. */
		return KEY_SLASH;
	case '!':
		/* Reports the key_to_evdev result. */
		return KEY_1;
	case '@':
		/* Reports the key_to_evdev result. */
		return KEY_2;
	case '#':
		/* Reports the key_to_evdev result. */
		return KEY_3;
	case '$':
		/* Reports the key_to_evdev result. */
		return KEY_4;
	case '%':
		/* Reports the key_to_evdev result. */
		return KEY_5;
	case '^':
		/* Reports the key_to_evdev result. */
		return KEY_6;
	case '&':
		/* Reports the key_to_evdev result. */
		return KEY_7;
	case '*':
		/* Reports the key_to_evdev result. */
		return KEY_8;
	case '(':
		/* Reports the key_to_evdev result. */
		return KEY_9;
	case ')':
		/* Reports the key_to_evdev result. */
		return KEY_0;
	case BEUI_KEY_ESCAPE:
		/* Reports the key_to_evdev result. */
		return KEY_ESC;
	case BEUI_KEY_BACKSPACE:
		/* Reports the key_to_evdev result. */
		return KEY_BACKSPACE;
	case BEUI_KEY_TAB:
		/* Reports the key_to_evdev result. */
		return KEY_TAB;
	case BEUI_KEY_ENTER:
		/* Reports the key_to_evdev result. */
		return KEY_ENTER;
	case BEUI_KEY_PAGE_UP:
		/* Reports the key_to_evdev result. */
		return KEY_PAGEUP;
	case BEUI_KEY_PAGE_DOWN:
		/* Reports the key_to_evdev result. */
		return KEY_PAGEDOWN;
	case BEUI_KEY_INSERT:
		/* Reports the key_to_evdev result. */
		return KEY_INSERT;
	case BEUI_KEY_DELETE:
		/* Reports the key_to_evdev result. */
		return KEY_DELETE;
	case BEUI_KEY_UP:
		/* Reports the key_to_evdev result. */
		return KEY_UP;
	case BEUI_KEY_LEFT:
		/* Reports the key_to_evdev result. */
		return KEY_LEFT;
	case BEUI_KEY_RIGHT:
		/* Reports the key_to_evdev result. */
		return KEY_RIGHT;
	case BEUI_KEY_DOWN:
		/* Reports the key_to_evdev result. */
		return KEY_DOWN;
	case BEUI_KEY_HOME:
		/* Reports the key_to_evdev result. */
		return KEY_HOME;
	case BEUI_KEY_END:
		/* Reports the key_to_evdev result. */
		return KEY_END;
	case BEUI_KEY_F1:
		/* Reports the key_to_evdev result. */
		return KEY_F1;
	case BEUI_KEY_F2:
		/* Reports the key_to_evdev result. */
		return KEY_F2;
	case BEUI_KEY_F3:
		/* Reports the key_to_evdev result. */
		return KEY_F3;
	case BEUI_KEY_F4:
		/* Reports the key_to_evdev result. */
		return KEY_F4;
	case BEUI_KEY_F5:
		/* Reports the key_to_evdev result. */
		return KEY_F5;
	case BEUI_KEY_F6:
		/* Reports the key_to_evdev result. */
		return KEY_F6;
	case BEUI_KEY_F7:
		/* Reports the key_to_evdev result. */
		return KEY_F7;
	case BEUI_KEY_F8:
		/* Reports the key_to_evdev result. */
		return KEY_F8;
	case BEUI_KEY_F9:
		/* Reports the key_to_evdev result. */
		return KEY_F9;
	case BEUI_KEY_F10:
		/* Reports the key_to_evdev result. */
		return KEY_F10;
	default:
		/* Reports the key_to_evdev result. */
		return -1;
	}
}

/* Implements source_key_state(). */
static int
source_key_state(
	const struct input_state *input,
	unsigned code,
	int alternate_code)
{
	const struct input_source *source;
	unsigned i;
	int supported;

	supported = 0;

	/* Processes each source_key_state item. */
	for (i = 0; i < INPUT_MAX_SOURCES; i++) {
		source = &input->sources[i];

		/* Handles the next source_key_state decision. */
		if (!source->active ||
		    (source->roles & INPUT_ROLE_KEYBOARD) ==
			0U)
			continue;

		/* Handles the next source_key_state decision. */
		if (bit_is_set(source->capabilities.key_bits, KEY_MAX, code)) {
			supported = 1;

			/* Handles the next source_key_state decision. */
			if (bit_is_set(source->keys, KEY_MAX, code))
				return 1;
		}

		/* Checks the optional alternate key code. */
		if (alternate_code >= 0) {
			/* Requires source support for the alternate code. */
			if (bit_is_set(
				source->capabilities.key_bits,
				KEY_MAX,
				(unsigned)alternate_code)) {
				supported = 1;

				/* Reports a pressed alternate key. */
				if (bit_is_set(source->keys,
					       KEY_MAX,
					       (unsigned)alternate_code))
					return 1;
			}
		}
	}

	/* Reports the source_key_state result. */
	return supported ? 0 : -1;
}

/* Implements input_is_key_down(). */
static int
input_is_key_down(
	const struct input_state *input,
	int beui_key)
{
	int call_result;
	int code;

	/* Handles the next input_is_key_down decision. */
	if (input == NULL)
		return -1;

	/* Handles the next input_is_key_down decision. */
	if (beui_key == BEUI_KEY_SHIFT) {
		call_result = source_key_state(input, KEY_LEFTSHIFT, KEY_RIGHTSHIFT);

		/* Reports the input_is_key_down result. */
		return call_result;
	}
	code = key_to_evdev(beui_key);

	/* Handles the next input_is_key_down decision. */
	if (code < 0)
		return -1;

	/* Reports the input_is_key_down result. */
	call_result = source_key_state(input, (unsigned)code, -1);

	/* Reports the input_is_key_down result. */
	return call_result;
}

/* Implements input_poll_pointer(). */
static int
input_poll_pointer(
	struct input_state *input,
	struct pointer_event *event)
{
	/* Handles the next input_poll_pointer decision. */
	if (input == NULL ||
	    event == NULL ||
	    !input->pointer_changed)
		return 0;

	event->x = input->pointer_x;
	event->y = input->pointer_y;
	event->buttons = input->pointer_buttons;
	input->pointer_changed = 0;

	/* Reports the input_poll_pointer result. */
	return 1;
}

/* Implements init_ui(). */
static int
init_ui(
	void)
{
	int initialized;

	/* Opens the display without a preferred pixel depth. */
	initialized = init_ui_with_hint(0);

	/* Reports whether the display was opened. */
	return initialized;
}

/* Implements init_ui_with_hint(). */
static int
init_ui_with_hint(
	unsigned preferred_bits_per_pixel)
{
	int entered;
	int pointer_started;

	/* Preserves an already open display session. */
	if (state.display_open)
		return 1;

	/* Initializes the requested display description. */
	memset(&state.display, 0, sizeof(state.display));
	state.display.preferred_bits_per_pixel = preferred_bits_per_pixel;

	/* Enters the zedBSD graphics mode. */
	entered = zedbsd_display_enter(&backend, &state.display);
	if (!entered)
		return 0;

	/* Publishes the active display state. */
	state.display_open = 1;
	state.close_requested = 0;
	state.pointer_buttons = 0;

	/* Discards input that predates the graphics session. */
	drain_input();

	/* Rejects an unusable display mode. */
	if (state.display.width == 0 || state.display.height == 0) {
		close_ui();
		return 0;
	}

	/* Centers the initial pointer state. */
	state.pointer_x = state.display.width / 2U;
	state.pointer_y = state.display.height / 2U;

	/* Starts the zedBSD input service for this display. */
	pointer_started = zedbsd_pointer_start(&backend, &state.display);
	if (!pointer_started) {
		close_ui();
		return 0;
	}
	state.pointer_open = 1;

	/* Reports a successfully opened display. */
	return 1;
}

/* Implements close_ui(). */
static void
close_ui(
	void)
{
	/* Leaves an already closed display unchanged. */
	if (!state.display_open && !state.pointer_open)
		return;

	/* Discards input held or buffered during the session. */
	drain_input();

	/* Releases ownership of the input service. */
	state.pointer_open = 0;

	/* Leaves the zedBSD graphics mode. */
	if (state.display_open)
		zedbsd_display_leave(&backend);
	state.display_open = 0;

	/* Clears geometry that no longer describes an active display. */
	memset(&state.display, 0, sizeof(state.display));
}

/* Implements cleanup_ui(). */
static void
cleanup_ui(
	void)
{
	struct image_entry *entry;
	struct image_entry *next;

	/* Captures the image registry before clearing the UI state. */
	entry = state.images;

	/* Releases all active zedBSD resources. */
	close_ui();

	/* Releases every decoded image. */
	while (entry != NULL) {
		next = entry->next;
		free(entry);
		entry = next;
	}

	/* Clears the complete UI state. */
	memset(&state, 0, sizeof(state));
}

/* Implements is_ui_open(). */
static int
is_ui_open(
	void)
{
	/* Reports the is_ui_open result. */
	return state.display_open;
}

/* Implements get_display_info(). */
static int
get_display_info(
	struct display_info *info)
{
	/* Handles the next get_display_info decision. */
	if (!state.display_open || info == NULL)
		return 0;
	*info = state.display;

	/* Reports the get_display_info result. */
	return 1;
}

/* Implements fill(). */
static int
fill(
	const struct rect *rect,
	uint32_t color)
{
	int call_result;

	/* Requires a visible rectangle supported by the active display. */
	if (!state.display_open ||
	    rect == NULL ||
	    rect->width == 0 ||
	    rect->height == 0 ||
	    rect->x >= state.display.width ||
	    rect->y >= state.display.height ||
	    rect->width > state.display.width - rect->x ||
	    rect->height > state.display.height - rect->y)
		return 0;

	/* Fills the validated rectangle through the graphics driver. */
	call_result = zedbsd_display_fill(&backend, rect, color);

	/* Reports the fill result. */
	return call_result;
}

/* Implements draw_line(). */
static int
draw_line(
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

	/* Draws the validated line through the graphics driver. */
	call_result = zedbsd_display_line(&backend,
					  x0,
					  y0,
					  x1,
					  y1,
					  color);

	/* Reports the draw_line result. */
	return call_result;
}

/* Implements pattern_fill(). */
static int
pattern_fill(
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

	/* Fills the validated pattern through the graphics driver. */
	call_result = zedbsd_display_pattern(&backend,
					     rect,
					     color,
					     pattern);

	/* Reports the pattern_fill result. */
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
	return image->stride >= image->width;
}

/* Implements draw_image(). */
static int
draw_image(
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

	/* Draws the validated image through the graphics driver. */
	call_result = zedbsd_display_image(&backend,
					   x,
					   y,
					   image);

	/* Reports the draw_image result. */
	return call_result;
}

/* Implements draw_image_region(). */
static int
draw_image_region(
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

	/* Handles the next draw_image_region decision. */
	if (source_x > ((size_t)-1 - offset) / pixel_size)
		return 0;

	offset += (size_t)source_x * pixel_size;
	region = *image;
	region.width = width;
	region.height = height;
	region.pixels += offset;

	/* Reports the draw_image_region result. */
	call_result = draw_image(destination_x, destination_y, &region);

	/* Reports the draw_image_region result. */
	return call_result;
}

/* Implements draw_image_pattern(). */
static int
draw_image_pattern(
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

	/* Draws the validated pattern through the graphics driver. */
	call_result = zedbsd_display_image_pattern(&backend,
						   x,
						   y,
						   image,
						   pattern);

	/* Reports the draw_image_pattern result. */
	return call_result;
}

/* Implements decode_utf8(). */
static uint32_t
decode_utf8(
	const char **cursor)
{
	const unsigned char *text = (const unsigned char *)*cursor;
	uint32_t codepoint;
	unsigned length;
	unsigned index;

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
	if ((length == 2 &&
	     codepoint < 0x80U) ||
	    (length == 3 &&
	     codepoint < 0x800U) ||
	    (length == 4 &&
	     codepoint < 0x10000U) ||
	    codepoint > 0x10ffffU ||
	    (codepoint >= 0xd800U &&
	     codepoint <= 0xdfffU)) {
		(*cursor)++;

		/* Reports the decode_utf8 result. */
		return 0xfffdU;
	}
	*cursor += length;

	/* Reports the decode_utf8 result. */
	return codepoint;
}

/* Implements measure_text(). */
static int
measure_text(
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

	/* Continues measure_text processing while work remains. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Handles the next measure_text decision. */
		if (codepoint == '\r')
			continue;

		/* Handles the next measure_text decision. */
		if (codepoint == '\n') {
			/* Handles the next measure_text decision. */
			if (line_width > maximum_width)
				maximum_width = line_width;
			line_width = 0;

			/* Handles the next measure_text decision. */
			if (total_height > (unsigned)-1 - 16U)
				return 0;
			total_height += 16U;
			continue;
		}

		/* Measures the next printable glyph. */
		if (!zedbsd_glyph_measure(
			&backend,
			codepoint,
			&glyph_width,
			&glyph_height))
			return 0;

		/* Rejects unsupported height or an overflowing line width. */
		if (glyph_height > 16U ||
		    line_width > (unsigned)-1 - glyph_width)
			return 0;
		line_width += glyph_width;
	}

	/* Handles the next measure_text decision. */
	if (line_width > maximum_width)
		maximum_width = line_width;

	*width = maximum_width;
	*height = total_height;

	/* Reports the measure_text result. */
	return 1;
}

/* Implements draw_text(). */
static int
draw_text(
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
	if (!measure_text(text, &width, &height))
		return 0;

	/* Requires the complete text block to fit the display. */
	if (x > state.display.width ||
	    y > state.display.height ||
	    width > state.display.width - x ||
	    height > state.display.height - y)
		return 0;

	/* Continues draw_text processing while work remains. */
	while (*cursor != '\0') {
		codepoint = decode_utf8(&cursor);

		/* Handles the next draw_text decision. */
		if (codepoint == '\r')
			continue;

		/* Handles the next draw_text decision. */
		if (codepoint == '\n') {
			x = origin_x;
			y += 16U;
			continue;
		}

		/* Measures the next printable glyph. */
		if (!zedbsd_glyph_measure(&backend,
					  codepoint,
					  &glyph_width,
					  &glyph_height))
			return 0;

		/* Draws the measured glyph. */
		if (!zedbsd_glyph_draw(&backend,
				       x,
				       y,
				       codepoint,
				       foreground,
				       background))
			return 0;

		x += glyph_width;
	}

	/* Reports the draw_text result. */
	return 1;
}

/* Implements poll_ui(). */
static int
poll_ui(
	void)
{
	struct pointer_event event;
	int updated;

	/* Handles the next poll_ui decision. */
	if (!state.display_open || state.close_requested)
		return 0;

	/* Checks whether the graphics descriptor remains open. */
	updated = zedbsd_display_poll(&backend);
	if (updated != 1) {
		/* Makes a closed display sticky for every later poll. */
		state.close_requested = 1;
		return 0;
	}

	/* Services pending input and device discovery. */
	drain_input();

	/* Captures a changed absolute pointer state. */
	if (state.pointer_open) {
		memset(&event, 0, sizeof(event));
		updated = zedbsd_pointer_poll(&backend, &event);

		/* Makes an input failure close the UI session. */
		if (updated < 0) {
			state.close_requested = 1;
			return 0;
		}

		/* Clamps a changed pointer to the active display. */
		if (updated > 0) {
			state.pointer_x = event.x < state.display.width ?
				event.x :
				state.display.width - 1U;
			state.pointer_y = event.y < state.display.height ?
				event.y :
				state.display.height - 1U;
			state.pointer_buttons = event.buttons;
		}
	}

	/* Reports a live display after servicing its input. */
	return 1;
}

/* Implements get_pointer(). */
static int
get_pointer(
	unsigned *x,
	unsigned *y,
	unsigned *buttons)
{
	/* Handles the next get_pointer decision. */
	if (!state.display_open || !state.pointer_open)
		return 0;

	/* Handles the next get_pointer decision. */
	if (x != NULL)
		*x = state.pointer_x;

	/* Handles the next get_pointer decision. */
	if (y != NULL)
		*y = state.pointer_y;

	/* Handles the next get_pointer decision. */
	if (buttons != NULL)
		*buttons = state.pointer_buttons;

	/* Reports the get_pointer result. */
	return 1;
}

/* Implements flush(). */
static int
flush(
	void)
{
	int call_result;

	/* Handles the next flush decision. */
	if (!state.display_open)
		return 0;

	/* Flushes all pending graphics operations. */
	call_result = zedbsd_display_flush(&backend, NULL, 0);

	/* Reports the flush result. */
	return call_result;
}

/* Implements get_milliseconds(). */
static int
get_milliseconds(
	uint64_t *milliseconds)
{
	/* Requires a clock destination. */
	if (milliseconds == NULL)
		return 0;

	/* Reads the zedBSD monotonic clock. */
	*milliseconds = zedbsd_milliseconds(&backend);

	/* Reports the get_milliseconds result. */
	return 1;
}

/* Implements sleep_milliseconds(). */
static int
sleep_milliseconds(
	unsigned milliseconds)
{
	uint64_t start;
	uint64_t now;

	/* Handles the next sleep_milliseconds decision. */
	if (!get_milliseconds(&start))
		return 0;

	/*
	 * The busy loop doubles as the clock poll and keeps input and
	 * device discovery serviced while the script idles.
	 */
	do {
		drain_input();

		/*
		 * A window closed mid-sleep ends the wait; the script
		 * sees it on its next BeUI.poll().
		 */
		if (state.display_open) {
			/* Ends the wait when display polling requests closure. */
			if (!poll_ui())
				break;
		}

		/* Handles the next sleep_milliseconds decision. */
		if (!get_milliseconds(&now))
			return 0;
	} while (now - start < milliseconds);

	/* Reports the sleep_milliseconds result. */
	return 1;
}

/* Implements is_key_down(). */
static int
is_key_down(
	int key)
{
	int call_result;

	/* Reads the key state from the zedBSD input service. */
	call_result = zedbsd_key_state(&backend, key);

	/* Reports the is_key_down result. */
	return call_result;
}

/* Implements drain_input(). */
static void
drain_input(
	void)
{
	/* Services and drains the zedBSD input source. */
	zedbsd_input_drain(&backend);
}

/* Implements image_load_bmp(). */
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
	if (!bmp_measure(
		data,
		size,
		&format,
		&width,
		&height,
		&pixel_size))
		return 0;

	/* Requires a bounded decoded pixel buffer. */
	if (pixel_size == 0 || pixel_size > IMAGE_PIXELS_MAX)
		return 0;
	entry = malloc(
		offsetof(struct image_entry, pixels) + pixel_size);

	/* Handles the next image_load_bmp decision. */
	if (entry == NULL)
		return 0;

	/* Handles the next image_load_bmp decision. */
	if (!bmp_decode(
		data,
		size,
		entry->pixels,
		pixel_size,
		&entry->image)) {
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

/* Implements image_get(). */
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

/* Implements image_destroy(). */
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
	const uint8_t *bytes = data;
	uint32_t dib_size;
	uint32_t data_offset;
	uint32_t colors_used;
	int32_t signed_width;
	int32_t signed_height;
	size_t row_bits;
	size_t palette_end;
	size_t source_bytes;
	unsigned bytes_per_pixel;

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

/* Implements bmp_measure(). */
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

/* Implements bmp_decode(). */
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
	uint8_t *output = pixel_storage;
	uint8_t *destination;
	unsigned source_y;
	unsigned x;
	unsigned y;

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

	/* Initializes the zedBSD BeUI implementation. */
	api_result = init_ui() ? 1 : 0;

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
	api_result = init_ui_with_hint((unsigned)bits_per_pixel) ? 1 : 0;

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

	close_ui();

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
	api_result = is_ui_open() ? 1 : 0;

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
	if (!get_display_info(&info)) {
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
	if (!get_display_info(&info)) {
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
 * canonical loop is "while (BeUI.poll()) { ... }".
 */
static bool
cfunc_BeUI_poll(
	NoctEnv *env)
{
	bool call_result;
	int api_result;

	/* Services the zedBSD display and input sources. */
	api_result = poll_ui() ? 1 : 0;

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
	if (!flush()) {
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
	if (!fill(&rectangle, (uint32_t)color)) {
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
	if (!draw_line((unsigned)x0,
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
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the vertical patterned-fill origin. */
	if (!get_int_arg(env, 1, &y)) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned fill extent. */
	if (!get_int_arg(env, 2, &width)) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned-fill height. */
	if (!get_int_arg(env, 3, &height)) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports an invalid patterned fill argument. */
		return false;
	}

	/* Reads the patterned fill color. */
	if (!get_int_arg(env, 4, &color)) {
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

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
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

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
		noct_error(env, N_TR("BeUI.patternFill received an invalid argument."));

		/* Reports the cfunc_BeUI_patternFill result. */
		return false;
	}
	rectangle.x = (unsigned)x;
	rectangle.y = (unsigned)y;
	rectangle.width = (unsigned)width;
	rectangle.height = (unsigned)height;

	/* Handles the next cfunc_BeUI_patternFill decision. */
	if (!pattern_fill(&rectangle,
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
	if (!measure_text(text, width, height)) {
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
	if (!draw_text(text,
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
	if (!get_milliseconds(&milliseconds)) {
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
	if (!sleep_milliseconds((unsigned)milliseconds)) {
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
	/* Keys unavailable from zedBSD input read as released. */
	key_down = is_key_down(key) == 1;

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
	if (!get_pointer(x, y, buttons)) {
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
		noct_error(env, N_TR("BeUI.getImageWidth received an invalid handle."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.getImageWidth received an invalid handle."));

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
		noct_error(env, N_TR("BeUI.getImageHeight received an invalid handle."));

		/* Reports an invalid image handle. */
		return false;
	}

	/* Resolves the image handle. */
	image = image_get(handle);
	if (image == NULL) {
		noct_error(env, N_TR("BeUI.getImageHeight received an invalid handle."));

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
	if (!draw_image((unsigned)x, (unsigned)y, image)) {
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
	if (!draw_image_region(
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
		noct_error(env, N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Reads the patterned image destination. */
	if (!get_int_arg(env, 1, &x)) {
		noct_error(env, N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Reads the vertical patterned-image destination. */
	if (!get_int_arg(env, 2, &y)) {
		noct_error(env, N_TR("BeUI.drawImagePattern received an invalid argument."));

		/* Reports an invalid patterned image argument. */
		return false;
	}

	/* Validates the patterned image destination. */
	if (x < 0 || y < 0) {
		noct_error(env, N_TR("BeUI.drawImagePattern received an invalid argument."));

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

	/* Draws the image with the requested pattern. */
	if (!draw_image_pattern((unsigned)x,
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
		if (!noct_set_dict_elem_make_int(env,
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

/* Implements register_api(). */
static bool
register_api(
	NoctEnv *env)
{
	static struct ffi_item ffi_items[] = {
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
		{"BeUI.drawImageRegion", "drawImageRegion", 7, {"image", "sourceX", "sourceY", "width", "height", "x", "y"},cfunc_BeUI_drawImageRegion},
		{"BeUI.drawImagePattern", "drawImagePattern", 4, {"image", "x", "y", "pattern"}, cfunc_BeUI_drawImagePattern},
		{"BeUI.destroyImage", "destroyImage", 1, {"image"}, cfunc_BeUI_destroyImage},
	};

	NoctValue beui_dict;
	NoctValue function;
	struct ffi_item *item;
	size_t index;

	/* Initializes the pinned registration values. */
	memset(&beui_dict, 0, sizeof(beui_dict));
	memset(&function, 0, sizeof(function));

	/* Handles the next register_api decision. */
	if (!noct_pin_local(env, 2, &beui_dict, &function))
		return false;

	/* Handles the next register_api decision. */
	if (!noct_make_empty_dict(env, &beui_dict)) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);

		/* Reports the register_api result. */
		return false;
	}

	/* Handles the next register_api decision. */
	if (!noct_set_global(env, "BeUI", &beui_dict)) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);

		/* Reports the register_api result. */
		return false;
	}

	/* Processes each register_api item. */
	for (index = 0; index < sizeof(ffi_items) / sizeof(ffi_items[0]); index++) {
		item = &ffi_items[index];

		/* Handles the next register_api decision. */
		if (!noct_register_cfunc(env,
					 item->global_name,
					 item->param_count,
					 item->param,
					 item->cfunc,
					 NULL)) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);

			/* Reports the register_api result. */
			return false;
		}

		/* Handles the next register_api decision. */
		if (!noct_get_global(env, item->global_name, &function)) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);

			/* Reports the register_api result. */
			return false;
		}

		/* Handles the next register_api decision. */
		if (!noct_set_dict_elem_cstr(env,
					     &beui_dict,
					     item->field_name,
					     &function)) {
			(void)noct_unpin_local(env, 2, &beui_dict, &function);

			/* Reports the register_api result. */
			return false;
		}
	}

	/* Handles the next register_api decision. */
	if (!register_int_dictionary(env,
				     "Key",
				     keys,
				     sizeof(keys) / sizeof(keys[0]))) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);

		/* Reports the register_api result. */
		return false;
	}

	/* Handles the next register_api decision. */
	if (!register_int_dictionary(env,
				     "Button",
				     buttons,
				     sizeof(buttons) / sizeof(buttons[0]))) {
		(void)noct_unpin_local(env, 2, &beui_dict, &function);

		/* Reports the register_api result. */
		return false;
	}
	(void)noct_unpin_local(env, 2, &beui_dict, &function);

	/* Reports the register_api result. */
	return true;
}

/* Implements uapi_bit_is_set(). */
static int
uapi_bit_is_set(
	const unsigned long *bits,
	unsigned bit)
{
	/* Reports the uapi_bit_is_set result. */
	return (bits[bit / BITS_PER_ULONG] & (1UL << (bit % BITS_PER_ULONG))) != 0;
}

/* Implements graphics_has(). */
static int
graphics_has(
	const struct backend_context *context,
	uint32_t capability)
{
	/* Requires an open graphics context. */
	if (context == NULL || context->graphics_fd < 0)
		return 0;

	/* Reports whether the driver exposes the requested operation. */
	return (context->graphics_capabilities & capability) != 0;
}

/* Implements copy_rect(). */
static void
copy_rect(
	struct graphics_rect *to,
	const struct rect *from)
{
	to->x = from->x;
	to->y = from->y;
	to->width = from->width;
	to->height = from->height;
}

/* Implements zedbsd_display_enter(). */
static int
zedbsd_display_enter(
	struct backend_context *context,
	struct display_info *info)
{
	struct graphics_mode request;
	int ioctl_result;

	/* Requires a fresh graphics context and display result. */
	if (context == NULL ||
	    info == NULL ||
	    context->graphics_fd >= 0)
		return 0;

	context->graphics_fd = open("/dev/graphics", O_RDWR);

	/* Handles the next zedbsd_display_enter decision. */
	if (context->graphics_fd < 0)
		return 0;

	memset(&request, 0, sizeof(request));
	request.preferred_bits_per_pixel = info->preferred_bits_per_pixel;

	/* Enters the requested graphics mode. */
	ioctl_result = ioctl(context->graphics_fd,
			     ZEDBSD_GRAPHICS_ENTER,
			     &request);
	if (ioctl_result != 0) {
		(void)close(context->graphics_fd);
		context->graphics_fd = -1;

		/* Reports a failed graphics-mode request. */
		return 0;
	}

	/* Rejects a graphics driver that returned empty geometry. */
	if (request.width == 0 || request.height == 0) {
		(void)close(context->graphics_fd);
		context->graphics_fd = -1;

		/* Reports the zedbsd_display_enter result. */
		return 0;
	}

	info->width = request.width;
	info->height = request.height;
	info->bits_per_pixel = request.bits_per_pixel;
	info->stride = request.stride;

	context->display = *info;
	context->graphics_capabilities = request.capabilities;

	input_init(&context->input,
		   request.width,
		   request.height);

	context->input_initialized = 1;

	/* Reports the zedbsd_display_enter result. */
	return 1;
}

/* Implements zedbsd_display_poll(). */
static int
zedbsd_display_poll(
	struct backend_context *context)
{
	/* Requires a display context before inspecting its descriptor. */
	if (context == NULL)
		return 0;

	/* Reports whether the graphics descriptor remains open. */
	return context->graphics_fd >= 0;
}

/* Implements zedbsd_display_fill(). */
static int
zedbsd_display_fill(
	struct backend_context *context,
	const struct rect *rect,
	uint32_t color)
{
	int call_result;
	struct graphics_fill request;

	/* Requires a fill rectangle. */
	if (rect == NULL)
		return 0;

	/* Requires driver support for solid fills. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_FILL))
		return 0;

	memset(&request, 0, sizeof(request));

	copy_rect(&request.rect, rect);

	request.color = color;

	/* Reports the zedbsd_display_fill result. */
	call_result = ioctl(
		context->graphics_fd,
		ZEDBSD_GRAPHICS_FILL_RECT,
		&request) == 0;

	/* Reports the zedbsd_display_fill result. */
	return call_result;
}

/* Implements zedbsd_display_line(). */
static int
zedbsd_display_line(
	struct backend_context *context,
	unsigned x0,
	unsigned y0,
	unsigned x1,
	unsigned y1,
	uint32_t color)
{
	int call_result;
	struct graphics_line request;

	/* Handles the next zedbsd_display_line decision. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_LINE))
		return 0;

	memset(&request, 0, sizeof(request));
	request.x0 = x0;
	request.y0 = y0;
	request.x1 = x1;
	request.y1 = y1;
	request.color = color;

	/* Reports the zedbsd_display_line result. */
	call_result = ioctl(context->graphics_fd,
			    ZEDBSD_GRAPHICS_DRAW_LINE,
			    &request) == 0;

	/* Reports the zedbsd_display_line result. */
	return call_result;
}

/* Implements zedbsd_display_pattern(). */
static int
zedbsd_display_pattern(
	struct backend_context *context,
	const struct rect *rect,
	uint32_t color,
	uint64_t pattern)
{
	int call_result;
	struct graphics_pattern_fill request;

	/* Requires a patterned-fill rectangle. */
	if (rect == NULL)
		return 0;

	/* Requires driver support for patterned fills. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_PATTERN))
		return 0;

	memset(&request, 0, sizeof(request));
	copy_rect(&request.rect, rect);
	request.color = color;
	request.pattern = pattern;

	/* Reports the zedbsd_display_pattern result. */
	call_result = ioctl(context->graphics_fd,
			    ZEDBSD_GRAPHICS_PATTERN_FILL,
			    &request) == 0;

	/* Reports the zedbsd_display_pattern result. */
	return call_result;
}

/* Implements zedbsd_display_image_common(). */
static int
zedbsd_display_image_common(
	struct backend_context *context,
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern,
	int patterned)
{
	int call_result;
	struct graphics_blit request;
	uint32_t capability;

	/* Requires a source image representable by the graphics ABI. */
	if (image == NULL ||
	    image->pixels == NULL ||
	    image->stride > UINT32_MAX)
		return 0;

	memset(&request, 0, sizeof(request));
	request.x = x;
	request.y = y;
	request.width = image->width;
	request.height = image->height;
	request.stride = (uint32_t)image->stride;
	request.pixels = (uapi_ptr_t)(uintptr_t)image->pixels;

	/* Handles the next zedbsd_display_image_common decision. */
	if (image->format == IMAGE_RGB24) {
		request.format = ZEDBSD_GRAPHICS_FORMAT_RGB24;
		capability = ZEDBSD_GRAPHICS_CAP_BLIT_RGB24;
	} else if (image->format == IMAGE_INDEX8 &&
		   image->palette_size <= 256U) {
		request.format = ZEDBSD_GRAPHICS_FORMAT_INDEX8;
		request.palette = (uapi_ptr_t)(uintptr_t)image->palette;
		request.palette_count = image->palette_size;
		capability = ZEDBSD_GRAPHICS_CAP_BLIT_INDEX8;
	} else {
		/* Reports the zedbsd_display_image_common result. */
		return 0;
	}

	/* Handles the next zedbsd_display_image_common decision. */
	if (!graphics_has(context, capability))
		return 0;

	request.pattern = pattern;

	/* Reports the zedbsd_display_image_common result. */
	call_result = ioctl(context->graphics_fd,
			    patterned ? ZEDBSD_GRAPHICS_BLIT_PATTERN : ZEDBSD_GRAPHICS_BLIT,
			    &request) == 0;

	/* Reports the zedbsd_display_image_common result. */
	return call_result;
}

/* Implements zedbsd_display_image(). */
static int
zedbsd_display_image(
	struct backend_context *context,
	unsigned x,
	unsigned y,
	const struct image *image)
{
	int call_result;

	/* Reports the zedbsd_display_image result. */
	call_result = zedbsd_display_image_common(context, x, y, image, 0, 0);

	/* Reports the zedbsd_display_image result. */
	return call_result;
}

/* Implements zedbsd_display_image_pattern(). */
static int
zedbsd_display_image_pattern(
	struct backend_context *context,
	unsigned x,
	unsigned y,
	const struct image *image,
	uint64_t pattern)
{
	int call_result;

	/* Reports the zedbsd_display_image_pattern result. */
	call_result = zedbsd_display_image_common(context,
						  x,
						  y,
						  image,
						  pattern,
						  1);

	/* Reports the zedbsd_display_image_pattern result. */
	return call_result;
}

/* Implements zedbsd_display_flush(). */
static int
zedbsd_display_flush(
	struct backend_context *context,
	const struct rect *rectangles,
	size_t rectangle_count)
{
	int call_result;
	struct graphics_rect converted[FLUSH_RECTS];
	struct graphics_flush request;
	size_t index;

	/* Requires a bounded flush request for an open graphics context. */
	if (context == NULL ||
	    context->graphics_fd < 0 ||
	    rectangle_count > FLUSH_RECTS ||
	    (rectangle_count != 0 && rectangles == NULL))
		return 0;

	/* A non-buffered driver has nothing to flush. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_FLUSH))
		return 1;

	/* Converts every dirty rectangle to the graphics ABI. */
	for (index = 0; index < rectangle_count; index++)
		copy_rect(&converted[index], &rectangles[index]);

	request.rectangles = rectangle_count == 0 ?
		0 :
		(uapi_ptr_t)(uintptr_t)converted;

	request.rectangle_count = (uint32_t)rectangle_count;

	/* Reports the zedbsd_display_flush result. */
	call_result = ioctl(context->graphics_fd, ZEDBSD_GRAPHICS_FLUSH, &request) == 0;

	/* Reports the zedbsd_display_flush result. */
	return call_result;
}

/* Implements zedbsd_glyph_get(). */
static int
zedbsd_glyph_get(
	struct backend_context *context,
	uint32_t codepoint,
	struct graphics_glyph *glyph,
	uint8_t *bitmap,
	size_t capacity)
{
	int call_result;

	/* Requires glyph destinations before consulting the driver. */
	if (glyph == NULL || bitmap == NULL)
		return 0;

	/* Requires driver support for glyph retrieval. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_GLYPH))
		return 0;

	/* Requires a bitmap capacity representable by the graphics ABI. */
	if (capacity > UINT32_MAX)
		return 0;

	memset(glyph, 0, sizeof(*glyph));
	glyph->codepoint = codepoint;
	glyph->bitmap = (uapi_ptr_t)(uintptr_t)bitmap;
	glyph->bitmap_capacity = (uint32_t)capacity;

	/* Retrieves the requested glyph. */
	call_result = ioctl(context->graphics_fd,
			    ZEDBSD_GRAPHICS_GET_GLYPH,
			    glyph) == 0;
	if (!call_result)
		return 0;

	/* Requires the driver to honor the supplied bitmap capacity. */
	if (glyph->bitmap_size > capacity)
		return 0;

	/* Reports a complete glyph result. */
	return 1;
}

/* Implements zedbsd_glyph_measure(). */
static int
zedbsd_glyph_measure(
	struct backend_context *context,
	uint32_t codepoint,
	unsigned *width,
	unsigned *height)
{
	struct graphics_glyph glyph;
	uint8_t bitmap[GLYPH_BYTES];

	/* Requires both glyph measurement destinations. */
	if (width == NULL || height == NULL)
		return 0;

	/* Retrieves the glyph metrics. */
	if (!zedbsd_glyph_get(context,
			      codepoint,
			      &glyph,
			      bitmap,
			      sizeof(bitmap)))
		return 0;
	*width = glyph.advance != 0 ? glyph.advance : glyph.width;
	*height = glyph.height;

	/* Reports the zedbsd_glyph_measure result. */
	return 1;
}

/* Implements zedbsd_glyph_draw(). */
static int
zedbsd_glyph_draw(
	struct backend_context *context,
	unsigned x,
	unsigned y,
	uint32_t codepoint,
	uint32_t foreground,
	uint32_t background)
{
	int call_result;
	struct graphics_glyph glyph;
	struct graphics_blit blit;
	uint8_t bitmap[GLYPH_BYTES];

	/* Requires driver support for monochrome glyph blits. */
	if (!graphics_has(context, ZEDBSD_GRAPHICS_CAP_BLIT_MONO1))
		return 0;

	/* Retrieves the glyph bitmap. */
	if (!zedbsd_glyph_get(context,
			      codepoint,
			      &glyph,
			      bitmap,
			      sizeof(bitmap)))
		return 0;

	memset(&blit, 0, sizeof(blit));
	blit.x = x;
	blit.y = y;
	blit.width = glyph.width;
	blit.height = glyph.height;
	blit.format = ZEDBSD_GRAPHICS_FORMAT_MONO1;
	blit.stride = glyph.stride;
	blit.pixels = (uapi_ptr_t)(uintptr_t)bitmap;
	blit.foreground = foreground;
	blit.background = background;

	/* Reports the zedbsd_glyph_draw result. */
	call_result = ioctl(context->graphics_fd, ZEDBSD_GRAPHICS_BLIT, &blit) == 0;

	/* Reports the zedbsd_glyph_draw result. */
	return call_result;
}

/* Implements zedbsd_milliseconds(). */
static uint64_t
zedbsd_milliseconds(
	struct backend_context *context)
{
	struct timespec now;

	UNUSED_PARAMETER(context);

	/* Handles the next zedbsd_milliseconds decision. */
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;

	/* Reports the zedbsd_milliseconds result. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
}

/* Implements zedbsd_input_query_capabilities(). */
static int
zedbsd_input_query_capabilities(
	int fd,
	struct input_capabilities *capabilities)
{
	int call_result;
	unsigned long event_bits[BIT_WORDS(EV_MAX)];
	unsigned long key_bits[BIT_WORDS(KEY_MAX)];
	unsigned long relative_bits[BIT_WORDS(REL_MAX)];
	unsigned long absolute_bits[BIT_WORDS(ABS_MAX)];
	struct input_absinfo absolute;
	struct input_id identity;
	int version;
	unsigned code;

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (capabilities == NULL)
		return 0;

	memset(event_bits, 0, sizeof(event_bits));
	memset(&identity, 0, sizeof(identity));
	version = 0;

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (ioctl(fd, EVIOCGVERSION, &version) != 0)
		return 0;

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (ioctl(fd, EVIOCGID, &identity) != 0)
		return 0;

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (ioctl(fd, EVIOCGBIT(0, sizeof(event_bits)), event_bits) != 0)
		return 0;
	input_capabilities_clear(capabilities);

	/* Processes each zedbsd_input_query_capabilities item. */
	for (code = 0; code <= EV_MAX; code++) {
		/* Handles the next zedbsd_input_query_capabilities decision. */
			if (uapi_bit_is_set(event_bits, code)) {
				input_capabilities_set_event(
					capabilities,
					code);
			}
	}

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (uapi_bit_is_set(event_bits, EV_KEY)) {
		memset(key_bits, 0, sizeof(key_bits));

		/* Handles the next zedbsd_input_query_capabilities decision. */
		if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) != 0)
			return 0;

		/* Processes each zedbsd_input_query_capabilities item. */
		for (code = 0; code <= KEY_MAX; code++) {
			/* Handles the next zedbsd_input_query_capabilities decision. */
			if (uapi_bit_is_set(key_bits, code)) {
				input_capabilities_set_key(
					capabilities,
					code);
			}
		}
	}

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (uapi_bit_is_set(event_bits, EV_REL)) {
		memset(relative_bits, 0, sizeof(relative_bits));

		/* Handles the next zedbsd_input_query_capabilities decision. */
		if (ioctl(fd,
			  EVIOCGBIT(EV_REL, sizeof(relative_bits)),
			  relative_bits) != 0)
			return 0;

		/* Processes each zedbsd_input_query_capabilities item. */
		for (code = 0; code <= REL_MAX; code++) {
			/* Handles the next zedbsd_input_query_capabilities decision. */
			if (uapi_bit_is_set(relative_bits, code)) {
				input_capabilities_set_relative(
					capabilities,
					code);
			}
		}
	}

	/* Handles the next zedbsd_input_query_capabilities decision. */
	if (uapi_bit_is_set(event_bits, EV_ABS)) {
		memset(absolute_bits, 0, sizeof(absolute_bits));

		/* Handles the next zedbsd_input_query_capabilities decision. */
		if (ioctl(fd,
			  EVIOCGBIT(EV_ABS, sizeof(absolute_bits)),
			  absolute_bits) != 0)
			return 0;

		/* Processes each zedbsd_input_query_capabilities item. */
		for (code = 0; code <= ABS_MAX; code++) {
			/* Handles the next zedbsd_input_query_capabilities decision. */
			if (!uapi_bit_is_set(absolute_bits, code))
				continue;

			memset(&absolute, 0, sizeof(absolute));

			/* Handles the next zedbsd_input_query_capabilities decision. */
			if (ioctl(fd, EVIOCGABS(code), &absolute) == 0) {
				input_capabilities_set_absolute(
					capabilities,
					code,
					&absolute);
			}
		}
	}

	/* Reports the zedbsd_input_query_capabilities result. */
	call_result = input_classify(capabilities) != INPUT_ROLE_NONE;

	/* Reports the zedbsd_input_query_capabilities result. */
	return call_result;
}

/* Implements zedbsd_input_resync_source(). */
static unsigned
zedbsd_input_resync_source(
	struct backend_context *context,
	unsigned source_index)
{
	unsigned call_result;
	unsigned long key_bits[BIT_WORDS(KEY_MAX)];
	struct input_absinfo absolute_x;
	struct input_absinfo absolute_y;
	const struct input_absinfo *absolute_x_pointer;
	const struct input_absinfo *absolute_y_pointer;
	const void *key_pointer;
	int fd;

	/* Handles the next zedbsd_input_resync_source decision. */
	if (context == NULL ||
	    source_index >= INPUT_MAX_SOURCES ||
	    context->sources[source_index].fd < 0)
		return INPUT_UPDATE_NONE;

	fd = context->sources[source_index].fd;

	memset(key_bits, 0, sizeof(key_bits));

	key_pointer = ioctl(fd, EVIOCGKEY(sizeof(key_bits)), key_bits) == 0 ?
		(const void *)key_bits :
		NULL;

	memset(&absolute_x, 0, sizeof(absolute_x));
	absolute_x_pointer = ioctl(fd, EVIOCGABS(ABS_X), &absolute_x) == 0 ? &absolute_x : NULL;

	memset(&absolute_y, 0, sizeof(absolute_y));
	absolute_y_pointer = ioctl(fd, EVIOCGABS(ABS_Y), &absolute_y) == 0 ? &absolute_y : NULL;

	/* Reports the zedbsd_input_resync_source result. */
	call_result = input_resync(&context->input,
				   source_index,
				   key_pointer,
				   sizeof(key_bits),
				   absolute_x_pointer,
				   absolute_y_pointer);

	/* Reports the zedbsd_input_resync_source result. */
	return call_result;
}

/* Implements zedbsd_input_event_name(). */
static int
zedbsd_input_event_name(
	const char *name)
{
	const char *cursor;

	/* Requires an input directory entry name. */
	if (name == NULL)
		return 0;

	/* Requires the event-device prefix. */
	if (strncmp(name, "event", 5) != 0)
		return 0;

	/* Requires a numeric suffix after the prefix. */
	if (name[5] == '\0')
		return 0;

	/* Processes each zedbsd_input_event_name item. */
	for (cursor = name + 5; *cursor != '\0'; cursor++) {
		/* Handles the next zedbsd_input_event_name decision. */
		if (*cursor < '0' || *cursor > '9')
			return 0;
	}

	/* Reports the zedbsd_input_event_name result. */
	return 1;
}

/* Implements zedbsd_input_has_event_name(). */
static int
zedbsd_input_has_event_name(
	const struct backend_context *context,
	const char *event_name)
{
	unsigned index;

	/* Processes each zedbsd_input_has_event_name item. */
	for (index = 0; index < INPUT_MAX_SOURCES; index++) {
		/* Compares names only for an attached input source. */
		if (context->sources[index].fd >= 0) {
			/* Reports a matching attached event name. */
			if (strcmp(context->sources[index].event_name, event_name) == 0)
				return 1;
		}
	}

	/* Reports the zedbsd_input_has_event_name result. */
	return 0;
}

/* Implements zedbsd_input_discover(). */
static void
zedbsd_input_discover(
	struct backend_context *context,
	int force)
{
	struct input_capabilities capabilities;
	struct dirent *entry;
	DIR *directory;
	char path[32];
	uint64_t now;
	int fd;
	int slot;
	int length;

	/* Handles the next zedbsd_input_discover decision. */
	if (context == NULL || !context->input_initialized)
		return;

	now = zedbsd_milliseconds(context);

	/* Handles the next zedbsd_input_discover decision. */
	if (!force &&
	    context->next_rescan != 0 &&
	    now < context->next_rescan)
		return;

	context->next_rescan = now + RESCAN_MS;

	directory = opendir("/dev/input");

	/* Handles the next zedbsd_input_discover decision. */
	if (directory == NULL)
		return;

	/* Continues zedbsd_input_discover processing while work remains. */
	while ((entry = readdir(directory)) != NULL) {
		/* Filters non-event directory entries. */
		if (!zedbsd_input_event_name(entry->d_name))
			continue;

		/* Filters names that do not fit the source record. */
		if (strlen(entry->d_name) >=
		    sizeof(context->sources[0].event_name))
			continue;

		/* Filters devices that are already attached. */
		if (zedbsd_input_has_event_name(context, entry->d_name))
			continue;

		length = snprintf(
			path,
			sizeof(path),
			"/dev/input/%s",
			entry->d_name);

		/* Handles the next zedbsd_input_discover decision. */
		if (length < 0 || (size_t)length >= sizeof(path))
			continue;

		fd = open(path, O_RDONLY | O_NONBLOCK);

		/* Handles the next zedbsd_input_discover decision. */
		if (fd < 0)
			continue;

		/* Handles the next zedbsd_input_discover decision. */
		if (!zedbsd_input_query_capabilities(fd, &capabilities)) {
			(void)close(fd);
			continue;
		}

		slot = input_attach(&context->input, &capabilities);

		/* Handles the next zedbsd_input_discover decision. */
		if (slot < 0 ||
		    (unsigned)slot >= INPUT_MAX_SOURCES) {
			(void)close(fd);
			continue;
		}

		context->sources[slot].fd = fd;

		(void)strcpy(context->sources[slot].event_name, entry->d_name);

		context->sources[slot].engine_slot = slot;

		(void)zedbsd_input_resync_source(context, (unsigned)slot);
	}

	(void)closedir(directory);
}

/* Implements zedbsd_input_detach_source(). */
static void
zedbsd_input_detach_source(
	struct backend_context *context,
	unsigned source_index)
{
	/* Handles the next zedbsd_input_detach_source decision. */
	if (context == NULL ||
	    source_index >= INPUT_MAX_SOURCES)
		return;

	/* Handles the next zedbsd_input_detach_source decision. */
	if (context->sources[source_index].fd >= 0)
		(void)close(context->sources[source_index].fd);

	context->sources[source_index].fd = -1;
	context->sources[source_index].event_name[0] = '\0';
	context->sources[source_index].engine_slot = -1;

	(void)input_detach(&context->input, source_index);

	context->next_rescan = zedbsd_milliseconds(context) + RESCAN_MS;
}

/* Implements zedbsd_input_service_source(). */
static void
zedbsd_input_service_source(
	struct backend_context *context,
	unsigned source_index)
{
	unsigned char buffer[sizeof(struct input_event) * READ_EVENTS];
	ssize_t count;
	unsigned update;
	unsigned iteration;

	/* Processes each zedbsd_input_service_source item. */
	for (iteration = 0; iteration < READ_EVENTS; iteration++) {
		count = read(context->sources[source_index].fd,
			     buffer,
			     sizeof(buffer));

		/* Handles the next zedbsd_input_service_source decision. */
		if (count > 0) {
			update = input_feed(&context->input,
					    source_index,
					    buffer,
					    (size_t)count);

			/* Handles the next zedbsd_input_service_source decision. */
			if ((update & INPUT_UPDATE_RESYNC) != 0) {
				(void)zedbsd_input_resync_source(context, source_index);
			}
			continue;
		}

		/* Handles the next zedbsd_input_service_source decision. */
		if (count == 0) {
			zedbsd_input_detach_source(context, source_index);

			/* Finishes zedbsd_input_service_source processing. */
			return;
		}

		/* Handles the next zedbsd_input_service_source decision. */
		if (errno == EINTR)
			continue;

		/* Handles the next zedbsd_input_service_source decision. */
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			zedbsd_input_detach_source(context, source_index);

		/* Finishes zedbsd_input_service_source processing. */
		return;
	}
}

/* Implements zedbsd_input_service(). */
static void
zedbsd_input_service(
	struct backend_context *context)
{
	unsigned index;

	/* Handles the next zedbsd_input_service decision. */
	if (context == NULL || !context->input_initialized)
		return;

	/* Processes each zedbsd_input_service item. */
	for (index = 0; index < INPUT_MAX_SOURCES; index++) {
		/* Handles the next zedbsd_input_service decision. */
		if (context->sources[index].fd >= 0)
			zedbsd_input_service_source(context, index);
	}

	zedbsd_input_discover(context, 0);
}

/* Implements zedbsd_input_close(). */
static void
zedbsd_input_close(
	struct backend_context *context)
{
	unsigned index;

	/* Handles the next zedbsd_input_close decision. */
	if (context == NULL)
		return;

	/* Processes each zedbsd_input_close item. */
	for (index = 0; index < INPUT_MAX_SOURCES; index++) {
		/* Handles the next zedbsd_input_close decision. */
		if (context->sources[index].fd >= 0)
			zedbsd_input_detach_source(context, index);
	}

	input_reset(&context->input);

	context->input_initialized = 0;
	context->next_rescan = 0;
}

/* Implements zedbsd_display_leave(). */
static void
zedbsd_display_leave(
	struct backend_context *context)
{
	/* Handles the next zedbsd_display_leave decision. */
	if (context == NULL)
		return;

	zedbsd_input_close(context);

	/* Handles the next zedbsd_display_leave decision. */
	if (context->graphics_fd >= 0)
		(void)close(context->graphics_fd);

	context->graphics_fd = -1;
	context->graphics_capabilities = 0;

	memset(&context->display, 0, sizeof(context->display));
}

/* Implements zedbsd_pointer_start(). */
static int
zedbsd_pointer_start(
	struct backend_context *context,
	const struct display_info *display)
{
	/* Handles the next zedbsd_pointer_start decision. */
	if (context == NULL ||
	    display == NULL ||
	    !context->input_initialized)
		return 0;

	input_set_display(
		&context->input,
		display->width,
		display->height);

	zedbsd_input_discover(context, 1);

	/* Reports the zedbsd_pointer_start result. */
	return 1;
}

/* Implements zedbsd_pointer_poll(). */
static int
zedbsd_pointer_poll(
	struct backend_context *context,
	struct pointer_event *event)
{
	int call_result;

	/* Handles the next zedbsd_pointer_poll decision. */
	if (context == NULL || !context->input_initialized)
		return -1;

	zedbsd_input_service(context);

	/* Reports the zedbsd_pointer_poll result. */
	call_result = input_poll_pointer(&context->input, event);

	/* Reports the zedbsd_pointer_poll result. */
	return call_result;
}

/* Implements zedbsd_key_state(). */
static int
zedbsd_key_state(
	struct backend_context *context,
	int key)
{
	int call_result;

	/* Handles the next zedbsd_key_state decision. */
	if (context == NULL || !context->input_initialized)
		return -1;

	zedbsd_input_service(context);

	/* Reports the zedbsd_key_state result. */
	call_result = input_is_key_down(&context->input, key);

	/* Reports the zedbsd_key_state result. */
	return call_result;
}

/* Implements zedbsd_input_drain(). */
static void
zedbsd_input_drain(
	struct backend_context *context)
{
	zedbsd_input_service(context);
}
