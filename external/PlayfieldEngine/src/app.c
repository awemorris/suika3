/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Copyright (c) 2025, 2026, Awe Morris. All rights reserved.
 */

/*
 * Default executable main.
 */

#include <playfield/playfield.h>
#include <noct/noct.h>

/*
 * init_aot_code() is defined in the library.c file.
 * To use AOT code, replace the file by the output of playfield-aotc.
 */
bool init_aot_code(struct rt_env *env);

/*
 * This function will be defined by customized engines to inject their APIs.
 * We define an empty implementation for the non-customized executable here.
 */
static bool
init_hook(
	int width,
	int height)
{
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);

	return true;
}

/*
 * Define the entrypoint.
 */
PF_DEFINE_MAIN(init_hook, init_aot_code)
