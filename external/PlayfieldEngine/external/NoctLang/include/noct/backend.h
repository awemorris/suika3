/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Backend Interface
 */

#ifndef NOCT_BACKEND_H
#define NOCT_BACKEND_H

#include <noct/c89compat.h>

/*
 * Bytecode Backend
 */

/* Start the BC backend. */
NOCT_DLL
bool
noct_bcback_start(
	const char *out_file_name);

/* Translate a file using BC backend. */
NOCT_DLL
bool
noct_bcback_translate(
	const char *source_file_name,
	const char *source_data);

/* Finalize the BC backend. */
NOCT_DLL
bool
noct_bcback_finalize(void);

/*
 * C Backend
 */

/* Start the C backend. */
NOCT_DLL
bool
noct_cback_start(
	const char *fname);

/* Translate a file using C backend. */
NOCT_DLL
bool
noct_cback_translate(
	const char *fname,
	const char *data);

/* Finalize the C backend. */
NOCT_DLL
bool
noct_cback_finalize(void);

/*
 * Emacs Lisp Backend
 */

/* Start EL backend. */
NOCT_DLL
bool
noct_elback_start(
	const char *fname);

/* Translate a file using EL backend. */
NOCT_DLL
bool
noct_elback_translate(
	const char *fname,
	const char *data);

/* Finalize the EL backend. */
NOCT_DLL
bool
noct_elback_finalize(void);

/*
 * Scheme Backend
 */

/* Start Scheme backend. */
NOCT_DLL
bool
noct_scmback_start(
	const char *fname);

/* Translate a file using Scheme backend. */
NOCT_DLL
bool
noct_scmback_translate(
	const char *fname,
	const char *data);

/* Finalize the Scheme backend. */
NOCT_DLL
bool
noct_scmback_finalize(void);

#endif
