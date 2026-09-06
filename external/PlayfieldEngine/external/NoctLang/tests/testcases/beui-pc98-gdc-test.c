/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 *
 * BeUI PC-98 GDC backend host test.
 * Imported from the Boots host tests when the BeUI PC-98 backends were
 * promoted to Noct.  Port I/O is mocked, so the register sequences are
 * checked on any host.
 */

#include "../../src/api/api-beui-pc98.c"

#include <string.h>

struct mock_gdc {
	uint8_t planes[4][NOCT_BEUI_GDC_PLANE_BYTES];
	unsigned reset_count;
	unsigned reset_saw_clear;
	unsigned stop_count;
	uint8_t commands[8];
	unsigned command_count;
	unsigned mode_count;
};

static int
display_reset(void *context)
{
	struct mock_gdc *mock = context;
	unsigned plane;

	mock->reset_count++;
	mock->reset_saw_clear = 1;
	for (plane = 0; plane < 4; plane++)
		if (mock->planes[plane][0] != 0 ||
		    mock->planes[plane][NOCT_BEUI_GDC_PLANE_BYTES - 1U] != 0)
			mock->reset_saw_clear = 0;
	return 1;
}

static int
display_stop(void *context)
{
	((struct mock_gdc *)context)->stop_count++;
	return 1;
}

static uint8_t
port_in8(void *context, uint16_t port)
{
	(void)context;
	return port == 0x60 ? 0 : 0xff;
}

static void
port_out8(void *context, uint16_t port, uint8_t value)
{
	struct mock_gdc *mock = context;

	if (port == 0x62 && mock->command_count < sizeof(mock->commands))
		mock->commands[mock->command_count++] = value;
	if (port == 0x6a && value == 1)
		mock->mode_count++;
}

int
main(void)
{
	struct mock_gdc mock;
	struct noct_beui_pc98_gdc backend;
	struct noct_beui_hal hal;
	struct noct_beui_rect rectangle = { 0, 0, 1, 1 };
	struct noct_beui_image image;
	uint8_t rgb_pixels[6] = { 0, 0, 255, 0, 255, 0 };
	unsigned plane;

	memset(&mock, 0xa5, sizeof(mock));
	memset(&backend, 0, sizeof(backend));
	backend.bios_context = &mock;
	backend.display_reset = display_reset;
	backend.display_stop = display_stop;
	backend.io_context = &mock;
	backend.port_in8 = port_in8;
	backend.port_out8 = port_out8;
	for (plane = 0; plane < 4; plane++)
		backend.planes[plane] = mock.planes[plane];
	memset(mock.commands, 0, sizeof(mock.commands));
	mock.command_count = 0;
	mock.reset_count = 0;
	mock.reset_saw_clear = 0;
	mock.stop_count = 0;
	mock.mode_count = 0;
	if (!noct_beui_pc98_gdc_make_hal(&hal, &backend) ||
	    !noct_beui_bind(&hal) || !noct_beui_init() ||
	    mock.reset_count != 1 || mock.reset_saw_clear != 1 ||
	    mock.mode_count != 1 ||
	    mock.command_count != 1 || mock.commands[0] != 0x0c ||
	    !noct_beui_fill(&rectangle, 0x00ff0000U))
		return 1;
	/* Red crosses the R threshold but not the weighted-luminance threshold. */
	if (mock.planes[0][0] != 0 || mock.planes[1][0] != 0x80 ||
	    mock.planes[2][0] != 0 || mock.planes[3][0] != 0)
		return 2;
	memset(&image, 0, sizeof(image));
	image.format = NOCT_BEUI_IMAGE_RGB24;
	image.width = 2;
	image.height = 1;
	image.stride = 6;
	image.pixels = rgb_pixels;
	if (!noct_beui_draw_image(8, 0, &image))
		return 3;
	/* Blue is B at x=8 and green is G at x=9. */
	if (mock.planes[0][1] != 0x80 || mock.planes[1][1] != 0 ||
	    mock.planes[2][1] != 0x40 || mock.planes[3][1] != 0)
		return 4;
	if (!noct_beui_line(16, 0, 17, 0, 0x00ff0000U) ||
	    mock.planes[1][2] != 0xc0 || mock.planes[3][2] != 0)
		return 5;
	rectangle.x = 24;
	rectangle.width = 2;
	if (!noct_beui_pattern_fill(&rectangle, 0x0000ff00U, 0x80U) ||
	    mock.planes[2][3] != 0x80 || mock.planes[3][3] != 0)
		return 6;
	if (!noct_beui_draw_image_pattern(32, 0, &image, 0x40U) ||
	    mock.planes[0][4] != 0 || mock.planes[2][4] != 0x40 ||
	    mock.planes[3][4] != 0)
		return 7;
	/* Cyan crosses B and G and its green-weighted luminance enables I. */
	rectangle.x = 40;
	rectangle.width = 1;
	if (!noct_beui_fill(&rectangle, 0x0000ffffU) ||
	    mock.planes[0][5] != 0x80 || mock.planes[1][5] != 0 ||
	    mock.planes[2][5] != 0x80 || mock.planes[3][5] != 0x80)
		return 10;
	noct_beui_close();
	if (mock.stop_count != 1 || mock.command_count != 2 ||
	    mock.commands[1] != 0x0d)
		return 8;
	for (plane = 0; plane < 4; plane++)
		if (mock.planes[plane][0] != 0 || mock.planes[plane][1] != 0)
			return 9;
	for (plane = 0; plane < 4; plane++)
		memset(mock.planes[plane], 0xa5, sizeof(mock.planes[plane]));
	if (!noct_beui_pc98_gdc_clear_graphics(&backend) ||
	    mock.mode_count != 2)
		return 11;
	for (plane = 0; plane < 4; plane++)
		if (mock.planes[plane][0] != 0 ||
		    mock.planes[plane][NOCT_BEUI_GDC_PLANE_BYTES - 1U] != 0)
			return 12;
	noct_beui_cleanup();
	return 0;
}
