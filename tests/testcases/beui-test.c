/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 *
 * BeUI core host test: lifecycle against a mock HAL, the absolute
 * pointer contract, window-close reporting, the BMP decoder, and the
 * image handle registry.  Imported from the Boots host tests when BeUI
 * was promoted to a Noct non-standard API.
 */

#include "../../src/api/api-beui-pc98.c"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(expression)						\
	do {								\
		if (!(expression)) {					\
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__,	\
			       #expression);				\
			failures++;					\
		}							\
	} while (0)

/* --------------------------------------------------------------- */
/* Mock HAL.                                                       */

struct mock_hal {
	int enter_result;
	int pointer_result;
	int poll_events_result;
	unsigned enter_count;
	unsigned preferred_bits_per_pixel;
	unsigned leave_count;
	unsigned pointer_start_count;
	unsigned pointer_stop_count;
	unsigned pointer_poll_count;
	unsigned poll_events_count;
	unsigned flush_count;
	unsigned draw_image_count;
	unsigned drain_count;
	unsigned image_x;
	unsigned image_y;
	unsigned image_width;
	unsigned image_height;
	const uint8_t *image_pixels;
	unsigned pointer_x;
	unsigned pointer_y;
	unsigned pointer_buttons;
	uint64_t clock_now;
	unsigned clock_step;
	unsigned clock_count;
};

static int
display_enter(void *context, struct noct_beui_display_info *info)
{
	struct mock_hal *mock = context;

	mock->enter_count++;
	mock->preferred_bits_per_pixel = info->preferred_bits_per_pixel;
	if (!mock->enter_result)
		return 0;
	info->width = 640;
	info->height = 400;
	info->bits_per_pixel = 4;
	info->stride = 80;
	return 1;
}

static void
display_leave(void *context)
{
	((struct mock_hal *)context)->leave_count++;
}

static int
display_poll_events(void *context)
{
	struct mock_hal *mock = context;

	mock->poll_events_count++;
	return mock->poll_events_result;
}

static int
display_flush(void *context, const struct noct_beui_rect *rectangles,
	      size_t rectangle_count)
{
	struct mock_hal *mock = context;

	if (rectangles != NULL || rectangle_count != 0)
		return 0;
	mock->flush_count++;
	return 1;
}

static int
display_draw_image(void *context, unsigned x, unsigned y,
		   const struct noct_beui_image *image)
{
	struct mock_hal *mock = context;

	mock->draw_image_count++;
	mock->image_x = x;
	mock->image_y = y;
	mock->image_width = image->width;
	mock->image_height = image->height;
	mock->image_pixels = image->pixels;
	return 1;
}

static int
pointer_start(void *context, const struct noct_beui_display_info *display)
{
	struct mock_hal *mock = context;

	mock->pointer_start_count++;
	return mock->pointer_result && display->width == 640 &&
		display->height == 400;
}

static void
pointer_stop(void *context)
{
	((struct mock_hal *)context)->pointer_stop_count++;
}

static int
pointer_poll(void *context, struct noct_beui_pointer_event *event)
{
	struct mock_hal *mock = context;

	mock->pointer_poll_count++;
	event->x = mock->pointer_x;
	event->y = mock->pointer_y;
	event->buttons = mock->pointer_buttons;
	return 1;
}

static uint64_t
clock_milliseconds(void *context)
{
	struct mock_hal *mock = context;

	mock->clock_count++;
	mock->clock_now += mock->clock_step;
	return mock->clock_now;
}

static void
input_drain(void *context)
{
	((struct mock_hal *)context)->drain_count++;
}

static struct noct_beui_hal
make_hal(struct mock_hal *mock)
{
	struct noct_beui_hal hal;

	memset(&hal, 0, sizeof(hal));
	hal.display.context = mock;
	hal.display.enter = display_enter;
	hal.display.leave = display_leave;
	hal.display.draw_image = display_draw_image;
	hal.display.flush = display_flush;
	hal.pointer.context = mock;
	hal.pointer.start = pointer_start;
	hal.pointer.stop = pointer_stop;
	hal.pointer.poll = pointer_poll;
	hal.clock.context = mock;
	hal.clock.milliseconds = clock_milliseconds;
	hal.input.context = mock;
	hal.input.drain = input_drain;
	return hal;
}

/* --------------------------------------------------------------- */
/* Lifecycle.                                                      */

static void
test_lifecycle(void)
{
	struct noct_beui_display_info info;
	struct mock_hal mock;
	struct noct_beui_hal hal;
	uint64_t milliseconds;

	memset(&mock, 0, sizeof(mock));
	mock.enter_result = 1;
	mock.pointer_result = 1;
	hal = make_hal(&mock);

	/* Nothing works before a HAL is bound; init is idempotent after. */
	CHECK(!noct_beui_init());
	CHECK(noct_beui_bind(&hal));
	CHECK(noct_beui_init());
	CHECK(mock.drain_count == 1);
	CHECK(noct_beui_init());
	CHECK(noct_beui_is_open());
	CHECK(noct_beui_get_display_info(&info));
	CHECK(info.width == 640 && info.height == 400);
	CHECK(noct_beui_poll());
	CHECK(noct_beui_flush());
	CHECK(mock.enter_count == 1);
	CHECK(mock.pointer_start_count == 1);
	CHECK(mock.pointer_poll_count == 1);
	CHECK(mock.flush_count == 1);
	noct_beui_close();
	/* Initialization discards type-ahead from the previous screen and
	 * closing drains held keys so neither direction leaks input. */
	CHECK(mock.drain_count >= 3);
	noct_beui_close();
	CHECK(!noct_beui_is_open());
	CHECK(mock.pointer_stop_count == 1);
	CHECK(mock.leave_count == 1);
	CHECK(noct_beui_init_with_hint(24));
	CHECK(mock.preferred_bits_per_pixel == 24);
	CHECK(noct_beui_get_display_info(&info));
	CHECK(info.preferred_bits_per_pixel == 24);
	noct_beui_close();
	CHECK(mock.enter_count == 2 && mock.leave_count == 2);

	/* The clock works with the display closed, and sleep polls it until
	 * the requested time has elapsed. */
	mock.clock_now = 0;
	mock.clock_step = 10;
	CHECK(noct_beui_get_milliseconds(&milliseconds));
	CHECK(milliseconds == 10);
	CHECK(noct_beui_sleep(25));
	CHECK(mock.clock_now >= 35);
	CHECK(mock.clock_count >= 4);

	noct_beui_cleanup();
	CHECK(!noct_beui_get_milliseconds(&milliseconds));

	/* A pointer that refuses to start unwinds the whole display. */
	memset(&mock, 0, sizeof(mock));
	mock.enter_result = 1;
	hal = make_hal(&mock);
	CHECK(noct_beui_bind(&hal));
	CHECK(!noct_beui_init());
	CHECK(mock.enter_count == 1);
	CHECK(mock.pointer_start_count == 1);
	CHECK(mock.pointer_stop_count == 0);
	CHECK(mock.leave_count == 1);
	noct_beui_cleanup();
}

/* --------------------------------------------------------------- */
/* Absolute pointer.                                               */

static void
test_pointer(void)
{
	struct mock_hal mock;
	struct noct_beui_hal hal;
	unsigned x;
	unsigned y;
	unsigned buttons;

	memset(&mock, 0, sizeof(mock));
	mock.enter_result = 1;
	mock.pointer_result = 1;
	hal = make_hal(&mock);

	/* No pointer state exists before the display opens. */
	CHECK(!noct_beui_get_pointer(&x, &y, &buttons));
	CHECK(noct_beui_bind(&hal));
	CHECK(noct_beui_init());

	/* The pointer starts centred rather than at a stale origin. */
	CHECK(noct_beui_get_pointer(&x, &y, &buttons));
	CHECK(x == 320 && y == 200 && buttons == 0);

	mock.pointer_x = 100;
	mock.pointer_y = 50;
	mock.pointer_buttons = NOCT_BEUI_BUTTON_LEFT | NOCT_BEUI_BUTTON_MIDDLE;
	CHECK(noct_beui_poll());
	CHECK(noct_beui_get_pointer(&x, &y, &buttons));
	CHECK(x == 100 && y == 50);
	CHECK(buttons == (NOCT_BEUI_BUTTON_LEFT | NOCT_BEUI_BUTTON_MIDDLE));

	/* A backend that reports past the edge is clamped, not wrapped. */
	mock.pointer_x = 5000;
	mock.pointer_y = 5000;
	CHECK(noct_beui_poll());
	CHECK(noct_beui_get_pointer(&x, &y, &buttons));
	CHECK(x == 639 && y == 399);

	noct_beui_cleanup();
}

/* --------------------------------------------------------------- */
/* Window close.                                                   */

static void
test_close_reporting(void)
{
	struct mock_hal mock;
	struct noct_beui_hal hal;

	memset(&mock, 0, sizeof(mock));
	mock.enter_result = 1;
	mock.pointer_result = 1;
	mock.poll_events_result = 1;
	hal = make_hal(&mock);
	hal.display.poll_events = display_poll_events;
	CHECK(noct_beui_bind(&hal));
	CHECK(noct_beui_init());
	CHECK(noct_beui_poll());
	CHECK(mock.poll_events_count == 1);

	/* Once the host reports a close it stays reported, so a script loop
	 * ends instead of spinning on a dead window. */
	mock.poll_events_result = 0;
	CHECK(!noct_beui_poll());
	mock.poll_events_result = 1;
	CHECK(!noct_beui_poll());
	/* A closed display stops calling into the backend entirely. */
	CHECK(mock.poll_events_count == 2);

	/* Reopening clears it. */
	noct_beui_close();
	CHECK(noct_beui_init());
	CHECK(noct_beui_poll());
	noct_beui_cleanup();
}

/* --------------------------------------------------------------- */
/* Image drawing.                                                   */

static void
test_image_drawing(void)
{
	struct mock_hal mock;
	struct noct_beui_hal hal;
	uint8_t pixels[5U * 4U * 3U];
	struct noct_beui_image image;

	memset(&mock, 0, sizeof(mock));
	mock.enter_result = 1;
	mock.pointer_result = 1;
	hal = make_hal(&mock);
	CHECK(noct_beui_bind(&hal));
	CHECK(noct_beui_init());

	memset(&image, 0, sizeof(image));
	image.format = NOCT_BEUI_IMAGE_RGB24;
	image.width = 5;
	image.height = 4;
	image.stride = 5U * 3U;
	image.pixels = pixels;
	CHECK(noct_beui_draw_image_region(&image, 2, 1, 3, 2, 10, 20));
	CHECK(mock.draw_image_count == 1);
	CHECK(mock.image_x == 10 && mock.image_y == 20);
	CHECK(mock.image_width == 3 && mock.image_height == 2);
	CHECK(mock.image_pixels == pixels + 1U * 15U + 2U * 3U);

	/* Empty, source-overflow, and destination-overflow rectangles are
	 * rejected before the backend sees them. */
	CHECK(!noct_beui_draw_image_region(&image, 0, 0, 0, 1, 0, 0));
	CHECK(!noct_beui_draw_image_region(&image, 4, 0, 2, 1, 0, 0));
	CHECK(!noct_beui_draw_image_region(&image, 0, 0, 1, 1, 640, 0));
	CHECK(mock.draw_image_count == 1);
	noct_beui_cleanup();
}

/* --------------------------------------------------------------- */
/* BMP decoding.                                                   */

static void
put16(uint8_t *destination, uint16_t value)
{
	destination[0] = (uint8_t)value;
	destination[1] = (uint8_t)(value >> 8);
}

static void
put32(uint8_t *destination, uint32_t value)
{
	destination[0] = (uint8_t)value;
	destination[1] = (uint8_t)(value >> 8);
	destination[2] = (uint8_t)(value >> 16);
	destination[3] = (uint8_t)(value >> 24);
}

static size_t
make_bmp(uint8_t *bmp, unsigned bits_per_pixel, int top_down)
{
	unsigned colors = bits_per_pixel <= 8 ? 1U << bits_per_pixel : 0;
	unsigned data_offset = 54U + colors * 4U;
	unsigned stride = ((3U * bits_per_pixel + 31U) / 32U) * 4U;
	unsigned source_y;

	memset(bmp, 0, 2048);
	bmp[0] = 'B';
	bmp[1] = 'M';
	put32(bmp + 2, data_offset + stride * 2U);
	put32(bmp + 10, data_offset);
	put32(bmp + 14, 40);
	put32(bmp + 18, 3);
	put32(bmp + 22, top_down ? (uint32_t)-2 : 2);
	put16(bmp + 26, 1);
	put16(bmp + 28, (uint16_t)bits_per_pixel);
	put32(bmp + 34, stride * 2U);
	put32(bmp + 46, colors);
	for (source_y = 0; source_y < colors; source_y++) {
		uint8_t *entry = bmp + 54U + source_y * 4U;

		entry[0] = (uint8_t)(source_y * 17U);
		entry[1] = (uint8_t)(source_y * 11U);
		entry[2] = (uint8_t)(source_y * 7U);
	}
	for (source_y = 0; source_y < 2; source_y++) {
		unsigned logical_y = top_down ? source_y : 1U - source_y;
		uint8_t *row = bmp + data_offset + source_y * stride;
		unsigned values[3];

		if (bits_per_pixel == 1U) {
			values[0] = logical_y == 0 ? 1U : 0U;
			values[1] = logical_y == 0 ? 0U : 1U;
			values[2] = logical_y == 0 ? 1U : 0U;
			row[0] = (uint8_t)(values[0] << 7 | values[1] << 6 |
					 values[2] << 5);
		} else if (bits_per_pixel == 4U) {
			values[0] = logical_y == 0 ? 1U : 3U;
			values[1] = 2U;
			values[2] = logical_y == 0 ? 3U : 1U;
			row[0] = (uint8_t)(values[0] << 4 | values[1]);
			row[1] = (uint8_t)(values[2] << 4);
		} else if (bits_per_pixel == 8U) {
			row[0] = (uint8_t)(logical_y == 0 ? 1U : 3U);
			row[1] = 2;
			row[2] = (uint8_t)(logical_y == 0 ? 3U : 1U);
		} else {
			static const uint8_t top[9] = {
				0, 0, 255, 0, 255, 0, 255, 0, 0,
			};
			static const uint8_t bottom[9] = {
				255, 255, 255, 0, 0, 0, 0x33, 0x22, 0x11,
			};

			memcpy(row, logical_y == 0 ? top : bottom, 9);
		}
	}
	return data_offset + stride * 2U;
}

static void
test_indexed(unsigned bits_per_pixel)
{
	uint8_t bmp[2048];
	uint8_t pixels[6];
	struct noct_beui_image image;
	enum noct_beui_image_format format;
	unsigned width;
	unsigned height;
	size_t pixel_bytes;
	size_t size = make_bmp(bmp, bits_per_pixel, 0);
	uint8_t expected[6];

	if (bits_per_pixel == 1U) {
		uint8_t values[6] = { 1, 0, 1, 0, 1, 0 };
		memcpy(expected, values, sizeof(expected));
	} else {
		uint8_t values[6] = { 1, 2, 3, 3, 2, 1 };
		memcpy(expected, values, sizeof(expected));
	}
	CHECK(noct_beui_bmp_measure(bmp, size, &format, &width, &height,
				    &pixel_bytes));
	CHECK(format == NOCT_BEUI_IMAGE_INDEX8);
	CHECK(width == 3 && height == 2);
	CHECK(pixel_bytes == sizeof(pixels));
	CHECK(noct_beui_bmp_decode(bmp, size, pixels, sizeof(pixels), &image));
	CHECK(image.stride == 3);
	CHECK(image.palette_size == (1U << bits_per_pixel));
	CHECK(memcmp(pixels, expected, sizeof(pixels)) == 0);
	CHECK(image.palette[1] == 0x00070b11U);
}

static void
test_rgb24(void)
{
	uint8_t bmp[2048];
	uint8_t pixels[18];
	struct noct_beui_image image;
	static const uint8_t expected[18] = {
		255, 0, 0, 0, 255, 0, 0, 0, 255,
		255, 255, 255, 0, 0, 0, 0x11, 0x22, 0x33,
	};
	size_t size = make_bmp(bmp, 24, 1);

	CHECK(noct_beui_bmp_decode(bmp, size, pixels, sizeof(pixels), &image));
	CHECK(image.format == NOCT_BEUI_IMAGE_RGB24);
	CHECK(image.stride == 9);
	CHECK(image.palette_size == 0);
	CHECK(memcmp(pixels, expected, sizeof(pixels)) == 0);
}

static void
test_bmp_rejects_malformed(void)
{
	uint8_t invalid[2048];
	enum noct_beui_image_format format;
	unsigned width;
	unsigned height;
	size_t pixel_bytes;
	size_t size = make_bmp(invalid, 8, 0);

	/* Compressed BMP is out of scope. */
	put32(invalid + 30, 1);
	CHECK(!noct_beui_bmp_measure(invalid, size, &format, &width, &height,
				     &pixel_bytes));
	/* Truncated pixel data must not be read past the end. */
	put32(invalid + 30, 0);
	CHECK(!noct_beui_bmp_measure(invalid, size - 1U, &format, &width,
				     &height, &pixel_bytes));
}

/* --------------------------------------------------------------- */
/* Image registry.                                                 */

static void
test_image_registry(void)
{
	uint8_t bmp[2048];
	size_t size = make_bmp(bmp, 8, 0);
	const struct noct_beui_image *image;
	int first;
	int second;

	first = noct_beui_image_load_bmp(bmp, size);
	second = noct_beui_image_load_bmp(bmp, size);
	CHECK(first > 0 && second > 0 && first != second);

	image = noct_beui_image_get(first);
	CHECK(image != NULL);
	CHECK(image != NULL && image->width == 3 && image->height == 2);
	CHECK(noct_beui_image_get(0) == NULL);
	CHECK(noct_beui_image_get(first + second + 1) == NULL);

	CHECK(noct_beui_image_destroy(first));
	CHECK(!noct_beui_image_destroy(first));
	CHECK(noct_beui_image_get(first) == NULL);
	CHECK(noct_beui_image_get(second) != NULL);

	/* Garbage is rejected rather than allocated. */
	CHECK(noct_beui_image_load_bmp(bmp, 4) == 0);
	CHECK(noct_beui_image_load_bmp(NULL, size) == 0);

	/* Cleanup frees whatever the script left behind; run under a leak
	 * checker to see it. */
	noct_beui_cleanup();
	CHECK(noct_beui_image_get(second) == NULL);
}

int
main(void)
{
	test_lifecycle();
	test_pointer();
	test_close_reporting();
	test_image_drawing();
	test_indexed(1);
	test_indexed(4);
	test_indexed(8);
	test_rgb24();
	test_bmp_rejects_malformed();
	test_image_registry();

	if (failures != 0) {
		printf("BeUI tests: %d failure(s)\n", failures);
		return 1;
	}
	printf("BeUI tests: OK\n");
	return 0;
}
