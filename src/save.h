/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Save Subsystem
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

#ifndef SUIKA3_SAVE_H
#define SUIKA3_SAVE_H

#include <suika3/suika3.h>

/*
 * File name for the global save data.
 */
#define GLOBAL_SAVE_FILE	"g000.sav"

/*
 * File name for the quick save data.
 */
#define QUICK_SAVE_FILE		"q000.sav"

/*
 * File name for the quick save data. (extra)
 */
#define QUICK_SAVE_EXTRA_FILE	"q001.sav"

/*
 * Initialize the save subsystem.
 */
bool
s3i_init_save(void);

/*
 * Cleanup the save subsystem.
 */
void
s3i_cleanup_save(void);

/*
 * Is loading in progress?
 */
bool
s3i_is_load_in_progress(void);

#endif
