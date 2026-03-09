/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Interpreter
 */

#ifndef NOCT_INTERPRETER_H
#define NOCT_INTERPRETER_H

#include <noct/c89compat.h>

/* Visit bytecode. */
bool
rt_visit_bytecode(struct rt_env *rt, struct rt_func *func);

#endif

