/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * NoctLang virtrual machine
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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

#ifndef PLAYFIELD_VM_H
#define PLAYFIELD_VM_H

#include "playfield/playfield.h"

/*
 * Create the VM.
 */
bool
pfi_create_vm(
	char **title,
	int *width,
	int *height,
	bool *fullscreen);

/*
 * Destroy the VM.
 */
void
pfi_destroy_vm(void);

/*
 * Call a NoctLang function in the VM.
 */
bool
pfi_call_vm_function(
	const char *func_name);

/*
 * Call a NoctLang function that the current tag points to.
 */
bool
pfi_call_vm_tag_function(
	bool *tag_end);

/*
 * Set an integer properties in the VM. (Engine.*)
 */
bool
pfi_set_vm_int(
	const char *prop_name,
	int val);

/*
 * Get an integer properties from the VM. (Engine.*)
 */
bool
pfi_get_vm_int(
	const char *prop_name,
	int *val);

/*
 * Get the heap usage of the VM.
 */
size_t
pfi_get_heap_usage(void);

/*
 * Perform a fast garbage collection in the VM.
 */
void
pfi_fast_gc(void);

/*
 * Perform a full garbage collection in the VM.
 */
void
pfi_full_gc(void);

/*
 * Get the VM environment pointer.
 */
void *
pfi_get_vm_env(void);

#endif

