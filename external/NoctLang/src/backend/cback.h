/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * cback: C translation backend
 */

#ifndef NOCT_CBACK_H
#define NOCT_CBACK_H

#include <noct/c89compat.h>
#include <stdio.h>

struct lir_func;

/* Clear translator states. */
bool cback_init(const char *fname);

/* Translate LIR to C. */
bool cback_translate_func(struct lir_func *func);

/* Put a finalization code for a standalone app. */
bool cback_finalize_standalone(void);

/* Put a finalization code for a plugin. */
bool cback_finalize_dll(void);

#endif
