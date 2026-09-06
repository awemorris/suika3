/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Host-independent REPL session engine.
 */

#include "repl.h"

#include <stdlib.h>
#include <string.h>

#define REPL_SOURCE_NAME	"REPL"
#define REPL_FUNC_NAME		"__noct_repl_eval"
#define REPL_MAX_NESTING	256
#define REPL_WORD_SIZE		16

struct noct_repl_session {
	NoctEnv *env;
	char *source;
	size_t capacity;
	size_t length;
};

struct repl_scan {
	char first_word[REPL_WORD_SIZE];
	size_t first_word_length;
	bool saw_token;
	bool saw_open_brace;
	bool in_string;
	bool invalid;
	char stack[REPL_MAX_NESTING];
	size_t depth;
};

static void repl_reset(NoctReplSession *session);
static void repl_scan_source(const char *source, struct repl_scan *scan);
static bool repl_is_ident_start(char c);
static bool repl_is_ident_char(char c);
static bool repl_is_space(char c);
static bool repl_word_is(const struct repl_scan *scan, const char *word);
static bool repl_is_block_statement(const struct repl_scan *scan);
static bool repl_needs_more(const struct repl_scan *scan);
static bool repl_append(NoctReplSession *session, const char *line);
static bool repl_prepare_statement(NoctReplSession *session,
				   const struct repl_scan *scan);
static bool repl_clear_synthetic_function(NoctEnv *env);

NOCT_DLL
NoctReplSession *
noct_repl_create(NoctEnv *env, size_t max_source_size)
{
	NoctReplSession *session;

	if (env == NULL || max_source_size == 0 ||
	    max_source_size == (size_t)-1)
		return NULL;

	session = noct_malloc(sizeof(*session));
	if (session == NULL) {
		noct_out_of_memory(env);
		return NULL;
	}
	memset(session, 0, sizeof(*session));

	session->source = noct_malloc(max_source_size + 1);
	if (session->source == NULL) {
		noct_free(session);
		noct_out_of_memory(env);
		return NULL;
	}

	session->env = env;
	session->capacity = max_source_size;
	repl_reset(session);
	return session;
}

NOCT_DLL
void
noct_repl_destroy(NoctReplSession *session)
{
	if (session == NULL)
		return;

	if (session->source != NULL) {
		memset(session->source, 0, session->capacity + 1);
		noct_free(session->source);
	}
	memset(session, 0, sizeof(*session));
	noct_free(session);
}

NOCT_DLL
enum NoctReplResult
noct_repl_cancel(NoctReplSession *session)
{
	if (session == NULL)
		return NOCT_REPL_ERROR;

	repl_reset(session);
	return NOCT_REPL_READY;
}

NOCT_DLL
enum NoctReplResult
noct_repl_submit(NoctReplSession *session, const char *line)
{
	struct repl_scan scan;
	NoctValue ret;
	bool is_function;
	bool ok;

	if (session == NULL)
		return NOCT_REPL_ERROR;
	if (line == NULL) {
		repl_reset(session);
		return NOCT_REPL_EXIT;
	}

	if (!repl_append(session, line)) {
		noct_error(session->env, N_TR("REPL input is too large."));
		repl_reset(session);
		return NOCT_REPL_ERROR;
	}

	repl_scan_source(session->source, &scan);
	if (scan.invalid) {
		noct_error(session->env, N_TR("REPL input has unmatched delimiters."));
		repl_reset(session);
		return NOCT_REPL_ERROR;
	}
	if (!scan.saw_token) {
		repl_reset(session);
		return NOCT_REPL_READY;
	}
	if (repl_needs_more(&scan))
		return NOCT_REPL_NEED_MORE;

	is_function = repl_word_is(&scan, "func");
	if (!is_function && !repl_prepare_statement(session, &scan)) {
		noct_error(session->env, N_TR("REPL input is too large."));
		repl_reset(session);
		return NOCT_REPL_ERROR;
	}

	ok = noct_register_source(session->env, REPL_SOURCE_NAME,
				  session->source);
	if (!ok) {
		if (!is_function)
			repl_clear_synthetic_function(session->env);
		repl_reset(session);
		return NOCT_REPL_ERROR;
	}

	if (!is_function) {
		memset(&ret, 0, sizeof(ret));
		ok = noct_enter_vm(session->env, REPL_FUNC_NAME, 0, NULL, &ret);
		if (!repl_clear_synthetic_function(session->env))
			ok = false;
	}

	repl_reset(session);
	return ok ? NOCT_REPL_EXECUTED : NOCT_REPL_ERROR;
}

static void repl_reset(NoctReplSession *session)
{
	session->length = 0;
	session->source[0] = '\0';
}

static bool repl_is_ident_start(char c)
{
	return (c >= 'A' && c <= 'Z') ||
	       (c >= 'a' && c <= 'z') || c == '_';
}

static bool repl_is_ident_char(char c)
{
	return repl_is_ident_start(c) || (c >= '0' && c <= '9');
}

static bool repl_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
	       c == '\f' || c == '\v';
}

static void repl_scan_source(const char *source, struct repl_scan *scan)
{
	const char *s;
	bool escaped;
	bool line_comment;

	memset(scan, 0, sizeof(*scan));
	escaped = false;
	line_comment = false;

	for (s = source; *s != '\0'; s++) {
		char c;

		c = *s;
		if (line_comment) {
			if (c == '\n')
				line_comment = false;
			continue;
		}
		if (scan->in_string) {
			if (escaped) {
				escaped = false;
				continue;
			}
			if (c == '\\') {
				escaped = true;
				continue;
			}
			if (c == '"') {
				scan->in_string = false;
				continue;
			}
			if (c == '\n') {
				scan->invalid = true;
				return;
			}
			continue;
		}

		if (c == '/' && s[1] == '/') {
			line_comment = true;
			s++;
			continue;
		}
		if (c == '"') {
			scan->in_string = true;
			scan->saw_token = true;
			continue;
		}

		if (!scan->saw_token && repl_is_ident_start(c)) {
			size_t length;

			length = 0;
			while (repl_is_ident_char(s[length])) {
				if (length + 1 < sizeof(scan->first_word))
					scan->first_word[length] = s[length];
				length++;
			}
			scan->first_word_length = length;
			if (length >= sizeof(scan->first_word))
				scan->first_word[0] = '\0';
			else
				scan->first_word[length] = '\0';
			scan->saw_token = true;
			s += length - 1;
			continue;
		}
		if (!scan->saw_token && !repl_is_space(c))
			scan->saw_token = true;

		if (c == '(' || c == '[' || c == '{') {
			if (scan->depth >= REPL_MAX_NESTING) {
				scan->invalid = true;
				return;
			}
			scan->stack[scan->depth++] = c;
			if (c == '{')
				scan->saw_open_brace = true;
		} else if (c == ')' || c == ']' || c == '}') {
			char expected;

			expected = c == ')' ? '(' : (c == ']' ? '[' : '{');
			if (scan->depth == 0 ||
			    scan->stack[scan->depth - 1] != expected) {
				scan->invalid = true;
				return;
			}
			scan->depth--;
		}
	}

	if (escaped)
		scan->in_string = true;
}

static bool repl_word_is(const struct repl_scan *scan, const char *word)
{
	size_t length;

	length = strlen(word);
	return scan->first_word_length == length &&
	       strcmp(scan->first_word, word) == 0;
}

static bool repl_is_block_statement(const struct repl_scan *scan)
{
	return repl_word_is(scan, "if") || repl_word_is(scan, "for") ||
	       repl_word_is(scan, "while");
}

static bool repl_needs_more(const struct repl_scan *scan)
{
	bool block_start;

	block_start = repl_word_is(scan, "func") ||
		      repl_is_block_statement(scan);
	return scan->depth != 0 || scan->in_string ||
	       (block_start && !scan->saw_open_brace);
}

static bool repl_append(NoctReplSession *session, const char *line)
{
	size_t length;
	bool add_newline;

	length = strlen(line);
	add_newline = length == 0 || line[length - 1] != '\n';
	if (length > session->capacity - session->length)
		return false;
	if (add_newline && session->length + length == session->capacity)
		return false;

	memcpy(session->source + session->length, line, length);
	session->length += length;
	if (add_newline)
		session->source[session->length++] = '\n';
	session->source[session->length] = '\0';
	return true;
}

static bool repl_prepare_statement(NoctReplSession *session,
				   const struct repl_scan *scan)
{
	static const char prefix[] = "func __noct_repl_eval() {\n";
	static const char block_suffix[] = "\n}\n";
	static const char statement_suffix[] = ";\n}\n";
	const char *suffix;
	size_t prefix_length;
	size_t suffix_length;
	size_t end;
	bool has_semicolon;

	end = session->length;
	while (end > 0 && repl_is_space(session->source[end - 1]))
		end--;
	has_semicolon = end > 0 && session->source[end - 1] == ';';

	if (repl_is_block_statement(scan) || has_semicolon)
		suffix = block_suffix;
	else
		suffix = statement_suffix;
	prefix_length = sizeof(prefix) - 1;
	suffix_length = strlen(suffix);

	if (prefix_length > session->capacity - session->length)
		return false;
	if (suffix_length >
	    session->capacity - session->length - prefix_length)
		return false;

	memmove(session->source + prefix_length, session->source,
		session->length + 1);
	memcpy(session->source, prefix, prefix_length);
	memcpy(session->source + prefix_length + session->length,
	       suffix, suffix_length + 1);
	session->length += prefix_length + suffix_length;
	return true;
}

static bool repl_clear_synthetic_function(NoctEnv *env)
{
	NoctValue zero;

	memset(&zero, 0, sizeof(zero));
	if (!noct_make_int(env, &zero, 0))
		return false;
	return noct_set_global(env, REPL_FUNC_NAME, &zero);
}
