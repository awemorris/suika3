/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

#ifndef NOCT_CLI_MAIN_H
#define NOCT_CLI_MAIN_H

#include <noct/noct.h>
#include "runtime.h"
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "cback.h"
#include "elback.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

void show_usage(void);
bool load_file_content(const char *fname, char **data, size_t *size);
int wide_printf(const char *format, ...);
bool add_file(const char *fname, bool (*add_file_hook)(const char *));

int command_compile(int argc, char *argv[]);
int command_transpile_c(int argc, char *argv[]);
int command_transpile_elisp(int argc, char *argv[]);
int command_run(int argc, char *argv[]);
int command_repl(void);

bool register_cli_ffi(NoctEnv *env);

#endif
