/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 *
 * BeUI PC-98 Core-Graph / Cirrus backend host test.
 * Imported from the Boots host tests when the BeUI PC-98 backends were
 * promoted to Noct.  Port I/O is mocked, so the register sequences are
 * checked on any host.
 */

#include "../../src/api/api-beui-pc98.c"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct mock_io {
	uint8_t wab_index;
	uint8_t wab[5];
	uint8_t seq_index;
	uint8_t seq[256];
	uint8_t gfx_index;
	uint8_t gfx[256];
	uint8_t crtc_index;
	uint8_t crtc[256];
	uint8_t misc;
	uint8_t sleep;
	uint8_t *framebuffer;
	size_t clear_bytes;
	unsigned clear_before_relay_count;
};

static uint8_t
mock_in8(void *context, uint16_t port)
{
	struct mock_io *io = context;

	switch (port) {
	case 0x0fab:
		return io->wab_index < sizeof(io->wab) ?
			io->wab[io->wab_index] : 0xff;
	case 0x0ca3:
		return io->sleep;
	case 0x0ca5:
		return io->seq[io->seq_index];
	case 0x0caf:
		return io->gfx[io->gfx_index];
	case 0x0da5:
	case 0x0ba5:
		return io->crtc[io->crtc_index];
	case 0x0cac:
		return io->misc;
	default:
		return 0;
	}
}

static void
mock_out8(void *context, uint16_t port, uint8_t value)
{
	struct mock_io *io = context;

	switch (port) {
	case 0x0faa:
		io->wab_index = value;
		break;
	case 0x0fab:
		if (io->wab_index == 3 && (value & 0x02) != 0 &&
		    io->framebuffer != NULL && io->clear_bytes != 0) {
			size_t i;

			for (i = 0; i < io->clear_bytes; i++)
				assert(io->framebuffer[i] == 0);
			io->clear_before_relay_count++;
		}
		if (io->wab_index != 0 && io->wab_index < sizeof(io->wab))
			io->wab[io->wab_index] = value;
		break;
	case 0x0ca3:
		io->sleep = value;
		break;
	case 0x0ca4:
		io->seq_index = value;
		break;
	case 0x0ca5:
		io->seq[io->seq_index] = value;
		break;
	case 0x0cae:
		io->gfx_index = value;
		break;
	case 0x0caf:
		io->gfx[io->gfx_index] = value;
		break;
	case 0x0da4:
	case 0x0ba4:
		io->crtc_index = value;
		break;
	case 0x0da5:
	case 0x0ba5:
		io->crtc[io->crtc_index] = value;
		break;
	case 0x0ca2:
		io->misc = value;
		break;
	default:
		break;
	}
}

int
main(void)
{
	struct mock_io io;
	struct noct_beui_pc98_cirrus backend;
	struct noct_beui_hal hal;
	struct noct_beui_display_info info;
	struct noct_beui_rect rect = { 7, 9, 3, 2 };
	uint8_t framebuffer[1024 * 1024];
	uint8_t image_pixels[] = { 0, 1 };
	struct noct_beui_image image;
	uint8_t saved_window = 0x80;
	uint8_t saved_relay = 0x02;
	uint8_t saved_sleep = 0x40;

	memset(&io, 0, sizeof(io));
	memset(framebuffer, 0x55, sizeof(framebuffer));
	io.wab[0] = 0x5b;
	io.wab[1] = saved_window;
	io.wab[2] = 0;
	io.wab[3] = saved_relay;
	io.sleep = saved_sleep;
	io.crtc[0x27] = 0xa0;
	io.misc = 1;
	io.framebuffer = framebuffer;
	io.clear_bytes = 640 * 480;
	noct_beui_pc98_cirrus_default(&backend, mock_in8, mock_out8, &io,
				       framebuffer);
	assert(noct_beui_pc98_cirrus_make_hal(&hal, &backend));

	/* With no hint, Cirrus retains the existing 8bpp mode. */
	memset(&info, 0, sizeof(info));
	assert(hal.display.enter(hal.display.context, &info));
	assert(info.width == 640 && info.height == 480);
	assert(info.bits_per_pixel == 8 && info.stride == 640);
	assert(io.seq[0x07] == 0x11 && io.crtc[0x13] == 0x50);
	assert(framebuffer[0] == 0 && framebuffer[640 * 480 - 1] == 0);
	assert(io.clear_before_relay_count == 1);
	assert(hal.display.fill(hal.display.context, &rect, 0x00ff0000));
	assert(framebuffer[9 * 640 + 7] == 0xe0);
	assert(framebuffer[10 * 640 + 9] == 0xe0);
	assert(framebuffer[9 * 640 + 10] == 0);
	hal.display.leave(hal.display.context);

	/* A 24bpp hint selects packed BGR without changing the default. */
	memset(framebuffer, 0x55, sizeof(framebuffer));
	memset(&info, 0, sizeof(info));
	info.preferred_bits_per_pixel = 24;
	io.clear_bytes = 640 * 480 * 3;
	assert(hal.display.enter(hal.display.context, &info));
	assert(info.width == 640 && info.height == 480);
	assert(info.bits_per_pixel == 24 && info.stride == 640 * 3);
	assert(io.seq[0x07] == 0x15 && io.crtc[0x13] == 0xf0);
	assert(framebuffer[0] == 0 && framebuffer[640 * 480 * 3 - 1] == 0);
	assert(io.clear_before_relay_count == 2);
	assert(hal.display.fill(hal.display.context, &rect, 0x00ff0000));
	assert(framebuffer[(9 * 640 + 7) * 3] == 0x00);
	assert(framebuffer[(9 * 640 + 7) * 3 + 1] == 0x00);
	assert(framebuffer[(9 * 640 + 7) * 3 + 2] == 0xff);
	assert(framebuffer[(10 * 640 + 9) * 3 + 2] == 0xff);
	assert(framebuffer[(9 * 640 + 10) * 3] == 0);
	assert(hal.display.line(hal.display.context, 0, 0, 1, 0,
				0x0000ff00));
	assert(framebuffer[0] == 0 && framebuffer[1] == 0xff &&
	       framebuffer[2] == 0);
	assert(framebuffer[3] == 0 && framebuffer[4] == 0xff &&
	       framebuffer[5] == 0);

	memset(&image, 0, sizeof(image));
	image.format = NOCT_BEUI_IMAGE_INDEX8;
	image.width = 2;
	image.height = 1;
	image.stride = 2;
	image.pixels = image_pixels;
	image.palette_size = 2;
	image.palette[0] = 0x000000ff;
	image.palette[1] = 0x00ffffff;
	assert(hal.display.draw_image(hal.display.context, 20, 30, &image));
	assert(framebuffer[(30 * 640 + 20) * 3] == 0xff);
	assert(framebuffer[(30 * 640 + 20) * 3 + 1] == 0x00);
	assert(framebuffer[(30 * 640 + 20) * 3 + 2] == 0x00);
	assert(framebuffer[(30 * 640 + 21) * 3] == 0xff);
	assert(framebuffer[(30 * 640 + 21) * 3 + 1] == 0xff);
	assert(framebuffer[(30 * 640 + 21) * 3 + 2] == 0xff);

	hal.display.leave(hal.display.context);
	assert(io.wab[1] == saved_window);
	assert(io.wab[2] == 0);
	assert(io.wab[3] == 0);
	assert(io.sleep == 0);
	puts("Core-Graph/Cirrus register sequence: PASS");
	return 0;
}
