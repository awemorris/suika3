/* -*- coding: utf-8; indent-tabs-mode: t; tab-width: 4; c-basic-offset: 4; -*- */

/*
 * StratoHAL
 * Gamepad HAL for Linux evdev
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
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

#if defined(__linux__) || defined(__FreeBSD__)

#include "stratohal/platform.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#if defined(__linux__)
#include <linux/input.h>
#else
#include <dev/evdev/input.h>
#endif

static int fd = -1;

static bool is_gamepad(int fd);
static void on_press_button(int button);
static void on_release_button(int button);
static void on_change_analog(int axis, int value);

void init_evgamepad(void)
{
	DIR *dir;
	struct dirent *ent;
	char path[256 + 32];
	char name[256];

	dir = opendir("/dev/input");
	if (!dir) {
		/* Ignore the error. */
		return;
	}

	while ((ent = readdir(dir)) != NULL) {
		if (strncmp(ent->d_name, "event", 5) == 0) {
			snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
			fd = open(path, O_RDONLY | O_NONBLOCK);
			if (fd < 0)
				continue;

			strcpy(name, "Unknown");
			if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
				/* printf("Checking %s (%s)\n", path, name); */
			}

			if (is_gamepad(fd)) {
				/* Found a gamepad! */
				closedir(dir);
				return;
			}

			close(fd);
			fd = -1;
		}
	}

	/* Not found. */
	fd = -1;
	closedir(dir);
	return;
}

static bool is_gamepad(int fd)
{
	unsigned long key_bits[(KEY_MAX + 1) / (sizeof(long) * 8)];

	memset(key_bits, 0, sizeof(key_bits));
	if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
		/* Ignore the error. */
		return false;
	}

	/* Check whether BTN_GAMEPAD (0x130) is available. */
	int byte = BTN_GAMEPAD / (sizeof(long) * 8);
	int bit  = BTN_GAMEPAD % (sizeof(long) * 8);
	if (key_bits[byte] & (1UL << bit)) {
		/* Gamepad! */
		return true;
	}
	return false;
}

void update_evgamepad(void)
{
	struct input_event ev;
	fd_set rfds;
        struct timeval tv;
	int ret;

	if (fd == -1)
		return;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	/* Timeout 0. */
	tv.tv_sec  = 0;
	tv.tv_usec = 0;

	ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(fd, &rfds)) {
		while (1) {
			ssize_t n = read(fd, &ev, sizeof(ev));
			if (n == (ssize_t)sizeof(ev)) {
				if (ev.type == EV_KEY) {
					if (ev.value)
						on_press_button(ev.code);
					else
						on_release_button(ev.code);
				} else if (ev.type == EV_ABS) {
					on_change_analog(ev.code, ev.value);
				}
			} else {
				break;
			}
		}
	}
}

static void on_press_button(int button)
{
	switch (button) {
	case 304:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_A);
		break;
	case 305:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_B);
		break;
	case 307:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_X);
		break;
	case 308:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_Y);
		break;
	case 310:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_L);
		break;
	case 311:
		hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_R);
		break;
	default:
		break;
	}
}

static void on_release_button(int button)
{
	switch (button) {
	case 304:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_A);
		break;
	case 305:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_B);
		break;
	case 307:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_X);
		break;
	case 308:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_Y);
		break;
	case 310:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_L);
		break;
	case 311:
		hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_R);
		break;
	default:
		break;
	}
}

static void on_change_analog(int axis, int value)
{
	switch (axis) {
	case 0:
		hal_callback_on_event_analog_input(HAL_ANALOG_X1, value);
		break;
	case 1:
		hal_callback_on_event_analog_input(HAL_ANALOG_Y1, value);
		break;
	case 2:
		hal_callback_on_event_analog_input(HAL_ANALOG_L, value * 256);
		break;
	case 3:
		hal_callback_on_event_analog_input(HAL_ANALOG_X2, value);
		break;
	case 4:
		hal_callback_on_event_analog_input(HAL_ANALOG_Y2, value);
		break;
	case 5:
		hal_callback_on_event_analog_input(HAL_ANALOG_R, value * 256);
		break;
	case 16:
		if (value == -1) {
			hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_LEFT);
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_RIGHT);
		} else if (value == 1) {
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_LEFT);
			hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_RIGHT);
		} else {
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_LEFT);
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_RIGHT);
		}
		break;
	case 17:
		if (value == -1) {
			hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_UP);
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_DOWN);
		} else if (value == 1) {
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_UP);
			hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_DOWN);
		} else {
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_UP);
			hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_DOWN);
		}
		break;
	default:
		break;
	}
}

#else

void init_evgamepad(void)
{
}

void update_evgamepad(void)
{
}

#endif
