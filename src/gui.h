/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * GUI Subsystem
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 1996-2026 Awe Morris / SCHOLA SUIKAE
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

#ifndef SUIKA3_GUI_H
#define SUIKA3_GUI_H

#include <suika3/suika3.h>

/*
 * Initialize the GUI subsystem.
 */
bool
s3i_init_gui(void);

/*
 * Cleanup the GUI subsytem.
 */
void
s3i_cleanup_gui(void);

/*
 * Run a GUI frame update.
 */
bool
s3i_run_gui_update(void);

/*
 * Run a GUI frame rendering.
 */
bool
s3i_run_gui_render(void);

/*
 * Run a GUI postprocess.
 */
bool
s3i_run_gui_postprocess(void);

/*
 * Check if the current GUI is a system GUI.
 */
bool 
s3_is_system_gui(void);

#endif
