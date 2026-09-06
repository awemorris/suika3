/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_REPL_H
#define NOCT_REPL_H

#include <noct/noct.h>

/* Result of one REPL input submission. */
enum NoctReplResult {
	NOCT_REPL_READY = 0,
	NOCT_REPL_NEED_MORE,
	NOCT_REPL_EXECUTED,
	NOCT_REPL_ERROR,
	NOCT_REPL_EXIT,
};

/* A host-owned REPL session using an existing Noct environment. */
typedef struct noct_repl_session NoctReplSession;

/*
 * Creates a REPL session.
 *
 * The VM and environment remain owned by the host. The source buffer is
 * bounded by max_source_size and is released by noct_repl_destroy().
 */
NOCT_DLL
NoctReplSession *
noct_repl_create(
	NoctEnv *env,
	size_t max_source_size);

/* Releases the REPL source buffer and session object. */
NOCT_DLL
void
noct_repl_destroy(
	NoctReplSession *session);

/*
 * Submits one physical input line. A missing trailing newline is supplied by
 * the session, so terminal hosts may pass their line editor's plain text.
 *
 * Pass NULL to request an orderly REPL exit. The caller can retrieve compile,
 * runtime, and session errors through the normal noct_get_error_*() API.
 */
NOCT_DLL
enum NoctReplResult
noct_repl_submit(
	NoctReplSession *session,
	const char *line);

/* Discards a pending multiline input without destroying the VM. */
NOCT_DLL
enum NoctReplResult
noct_repl_cancel(
	NoctReplSession *session);

#endif
