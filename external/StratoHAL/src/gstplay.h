/* -*- c-basic-offset: 2; indent-tabs-mode: nil; -*- */

/*
 * Playfield Engine
 * Video playback HAL for GStreamer on X11
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
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef STRATOHAL_GSTPLAY_H
#define STRATOHAL_GSTPLAY_H

#include <X11/Xlib.h>

void
gstplay_init (int argc, char *argv[]);

void
gstplay_play (const char *fname, Window window);

void
gstplay_stop (void);

int
gstplay_is_playing (void);

void
gstplay_loop_iteration (void);

#endif
