/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Tag Commands
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2026 The Suika3 Community
 * Copyright (c) 2025-2026 The Playfield Engine Project
 * Copyright (c) 2025-2026 The NoctVM Project
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 2016-2024 The Suika2 Development Team
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is derived from the codebase of Playfield Engine, NoctLang,
 * Suika2, Suika Studio, Wind Game Lib, and 98/AT Game Lib.
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

#include <suika3/suika3.h>
#include "cmd.h"

/* Tag function entry. */
struct tag_func {
	const char *name;
	bool (*func)(void *);
};

/* Forward declaration for tag functions. */
bool s3i_tag_bg(void *p);
bool s3i_tag_ch(void *p);
bool s3i_tag_choose(void *p);
bool s3i_tag_click(void *p);
bool s3i_tag_choose(void *p);
bool s3i_tag_else(void *p);
bool s3i_tag_elseif(void *p);
bool s3i_tag_endif(void *p);
bool s3i_tag_goto(void *p);
bool s3i_tag_gui(void *p);
bool s3i_tag_if(void *p);
bool s3i_tag_label(void *p);
bool s3i_tag_load(void *p);
bool s3i_tag_set(void *p);
bool s3i_tag_text(void *p);

/* Tag function table. */
static struct tag_func tag_func[] = {
	{"Tag_bg",		s3i_tag_bg},
	{"Tag_ch",		s3i_tag_ch},
	{"Tag_choose",		s3i_tag_choose},
	{"Tag_click",		s3i_tag_click},
	{"Tag_else",		s3i_tag_else},
	{"Tag_elseif",		s3i_tag_elseif},
	{"Tag_endif",		s3i_tag_endif},
	{"Tag_goto",		s3i_tag_goto},
	{"Tag_gui",		s3i_tag_gui},
	{"Tag_if",		s3i_tag_if},
	{"Tag_text",		s3i_tag_text},
	{"Tag_label",		s3i_tag_label},
	{"Tag_load",		s3i_tag_load},
	{"Tag_set",		s3i_tag_set},
};

/*
 * Install the tag functions.
 */
bool
s3i_install_tag_funcs(void)
{
	const char *params[] = {"param"};
	int i;

	/* Register functions. */
	for (i = 0; i < sizeof(tag_func) / sizeof(struct tag_func); i++) {
		if (!s3_install_tag(tag_func[i].name,
				    tag_func[i].func))
			return false;
	}

	return true;
}
