/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Transactional bytecode backend output.
 */

#ifndef NOCT_BCBACK_FILE_H
#define NOCT_BCBACK_FILE_H

#include <noct/c89compat.h>

#include <stdio.h>

struct bcback_output {
	FILE *stream;
	char *final_path;
	char *temporary_path;
};

bool bcback_output_open(
	struct bcback_output *output,
	const char *final_path);
FILE *bcback_output_get_stream(
	struct bcback_output *output);
bool bcback_output_commit(
	struct bcback_output *output);
void bcback_output_abort(
	struct bcback_output *output);

#endif
