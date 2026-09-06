/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Win32 terminal API.
 *
 * Implements Term.* directly with the classic Win32 Console API (no
 * VT-mode dependency), so it works on every
 * desktop Windows from the NT line onward, including consoles where
 * ENABLE_VIRTUAL_TERMINAL_PROCESSING is unavailable.
 *
 *  - Output goes through WriteConsoleW; UTF-8 from the VM is
 *    converted to UTF-16 here. The console handles double-width CJK
 *    cells by itself.
 *  - write() interprets the small in-band SGR subset (ESC [ ... m)
 *    that callers may embed in row text (remacs uses reverse video
 *    in-band so a row diffs as one string); other escape sequences
 *    are swallowed.
 *  - Input uses ReadConsoleInputW and translates KEY_EVENT records
 *    into the Emacs-style event integers of the POSIX backend:
 *    Alt maps to META, Ctrl to CTRL, arrows and friends to the
 *    private TERM_KEY_* constants. AltGr (right Alt + left Ctrl) is
 *    plain character input, not Meta.
 *  - The session runs on a private screen buffer, which doubles as
 *    the alternate screen: closing restores the original console
 *    contents like the POSIX ?1049 sequence.
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * These values are part of the Term.* language API, not the public C API.
 * Keep the platform implementation self-contained instead of exposing a
 * backend interface through noct.h.
 */
#define TERM_MOD_META	(1 << 27)
#define TERM_MOD_CTRL	(1 << 26)
#define TERM_MOD_SHIFT	(1 << 25)
#define TERM_KEY_BASE	0xe000
#define TERM_KEY_UP	(TERM_KEY_BASE + 0)
#define TERM_KEY_DOWN	(TERM_KEY_BASE + 1)
#define TERM_KEY_RIGHT	(TERM_KEY_BASE + 2)
#define TERM_KEY_LEFT	(TERM_KEY_BASE + 3)
#define TERM_KEY_HOME	(TERM_KEY_BASE + 4)
#define TERM_KEY_END	(TERM_KEY_BASE + 5)
#define TERM_KEY_PGUP	(TERM_KEY_BASE + 6)
#define TERM_KEY_PGDN	(TERM_KEY_BASE + 7)
#define TERM_KEY_INSERT	(TERM_KEY_BASE + 8)
#define TERM_KEY_DELETE	(TERM_KEY_BASE + 9)
#define TERM_KEY_F1	(TERM_KEY_BASE + 11)

#ifndef COMMON_LVB_UNDERSCORE
#define COMMON_LVB_UNDERSCORE	0x8000
#endif

/* Decoded key events waiting to be delivered. */
#define EVENT_QUEUE_SIZE	1024

/* Records pulled per ReadConsoleInputW call. */
#define READ_CHUNK		32

struct win32_term_style {
	int foreground;
	int background;
	bool bold;
	bool reverse;
	bool underline;
};

struct win32_term_state {
	int open;
	HANDLE input;
	HANDLE screen;		/* Our private buffer while open. */
	HANDLE original;	/* The buffer that was active before. */
	DWORD saved_input_mode;
	int saved_mode_valid;

	WORD base_attr;		/* Attributes to reset to (SGR 0). */
	WORD cur_attr;		/* Current fg/bg/bold/underline bits. */
	int reverse;

	volatile int resized;

	int queue[EVENT_QUEUE_SIZE];
	int queue_head;
	int queue_len;
	WCHAR pending_high;	/* Pending high surrogate, or 0. */

	/* Ctrl+C delivered through the console ctrl handler (see
	 * ctrl_handler below); read on the read_key thread. */
	volatile LONG ctrl_c_count;
	HANDLE wake;		/* Signaled by the ctrl handler. */

	/* Used only to bracket blocking waits for the MT runtime. */
	NoctEnv *env;
};

struct term_const {
	const char *name;
	int value;
};

static struct win32_term_state term;

static const char *term_no_param[NOCT_ARG_MAX] = {
	NULL
};

static const char *term_move_to_param[NOCT_ARG_MAX] = {
	"row",
	"col"
};

static const char *term_write_param[NOCT_ARG_MAX] = {
	"text"
};

static const char *term_style_param[NOCT_ARG_MAX] = {
	"style"
};

static const char *term_visible_param[NOCT_ARG_MAX] = {
	"visible"
};

static const char *term_timeout_param[NOCT_ARG_MAX] = {
	"timeoutMs"
};

static const struct term_const term_consts[] = {
	{"META", TERM_MOD_META},
	{"CTRL", TERM_MOD_CTRL},
	{"SHIFT", TERM_MOD_SHIFT},
	{"KEY_UP", TERM_KEY_UP},
	{"KEY_DOWN", TERM_KEY_DOWN},
	{"KEY_RIGHT", TERM_KEY_RIGHT},
	{"KEY_LEFT", TERM_KEY_LEFT},
	{"KEY_HOME", TERM_KEY_HOME},
	{"KEY_END", TERM_KEY_END},
	{"KEY_PGUP", TERM_KEY_PGUP},
	{"KEY_PGDN", TERM_KEY_PGDN},
	{"KEY_INSERT", TERM_KEY_INSERT},
	{"KEY_DELETE", TERM_KEY_DELETE},
	{"KEY_F1", TERM_KEY_F1},
	{"KEY_TAB", '\t'},
	{"KEY_RET", '\r'},
	{"KEY_ESC", 0x1b},
	{"KEY_BS", 0x7f},
};

static bool register_term_functions(NoctEnv *env, NoctValue *term_dict, NoctValue *function);
static bool register_term_function(NoctEnv *env, NoctValue *term_dict, NoctValue *function, const char *global_name, const char *field_name, size_t param_count, const char *param[], bool (*cfunc)(NoctEnv *env));
static bool register_term_constants(NoctEnv *env, NoctValue *term_dict, NoctValue *temporary);
static BOOL WINAPI ctrl_handler(DWORD type);
static void queue_push(int event);
static int queue_pop(void);
static WORD ansi_to_rgb_bits(int color);
static int color256_to_16(int color);
static WORD attr_with_fg(WORD attr, int color);
static WORD attr_with_bg(WORD attr, int color);
static WORD effective_attr(void);
static void write_wide(const WCHAR *text, DWORD length);
static void write_utf8_run(const char *utf8, int length);
static void apply_sgr(const int *params, int param_count);
static int win32_is_tty(void);
static int win32_open(void);
static void win32_close(void);
static int win32_size(unsigned *rows, unsigned *columns);
static int win32_resized(void);
static int win32_move_to(unsigned row, unsigned column);
static int win32_write(const char *utf8, size_t length);
static int win32_clear(void);
static int win32_clear_to_eol(void);
static int win32_set_style(const struct win32_term_style *style);
static int win32_show_cursor(int visible);
static int win32_flush(void);
static int vk_to_special(WORD virtual_key);
static void process_key_event(const KEY_EVENT_RECORD *key);
static void drain_input(void);
static int win32_read_key(int timeout_ms);
static int win32_pending_input(void);
static void term_restore(void);
static bool return_int(NoctEnv *env, int value);
static bool get_int_arg(NoctEnv *env, uint32_t index, int *result);
static bool read_style_field(NoctEnv *env, NoctValue *style, NoctValue *temporary, const char *name, int *value);
static bool cfunc_Term_open(NoctEnv *env);
static bool cfunc_Term_close(NoctEnv *env);
static bool cfunc_Term_isTTY(NoctEnv *env);
static bool cfunc_Term_size(NoctEnv *env);
static bool cfunc_Term_resized(NoctEnv *env);
static bool cfunc_Term_moveTo(NoctEnv *env);
static bool cfunc_Term_write(NoctEnv *env);
static bool cfunc_Term_clear(NoctEnv *env);
static bool cfunc_Term_clearToEol(NoctEnv *env);
static bool cfunc_Term_setStyle(NoctEnv *env);
static bool cfunc_Term_showCursor(NoctEnv *env);
static bool cfunc_Term_flush(NoctEnv *env);
static bool cfunc_Term_syncBegin(NoctEnv *env);
static bool cfunc_Term_syncEnd(NoctEnv *env);
static bool cfunc_Term_readKey(NoctEnv *env);
static bool cfunc_Term_pendingInput(NoctEnv *env);

/*
 * Registers the Win32 Term API functions and constants.
 */
NOCT_DLL
bool
noct_register_api_term(
	NoctEnv *env)
{
	NoctValue term_dict;
	NoctValue temporary;
	NoctValue function;
	bool registered;

	/* Associates blocking console waits with the current VM environment. */
	term.env = env;

	/* Initializes the values before they become GC roots. */
	memset(&term_dict, 0, sizeof(term_dict));
	memset(&temporary, 0, sizeof(temporary));
	memset(&function, 0, sizeof(function));

	/* Pins the dictionary and registration temporaries. */
	if (!noct_pin_local(env, 3, &term_dict, &temporary, &function))
		return false;

	/* Creates the global Term dictionary. */
	if (!noct_make_empty_dict(env, &term_dict)) {
		(void)noct_unpin_local(env, 3, &term_dict, &temporary, &function);
		return false;
	}

	/* Publishes the empty Term dictionary. */
	if (!noct_set_global(env, "Term", &term_dict)) {
		(void)noct_unpin_local(env, 3, &term_dict, &temporary, &function);
		return false;
	}

	/* Registers every native Term function. */
	if (!register_term_functions(env, &term_dict, &function)) {
		(void)noct_unpin_local(env, 3, &term_dict, &temporary, &function);
		return false;
	}

	/* Publishes every Term key constant. */
	if (!register_term_constants(env, &term_dict, &temporary)) {
		(void)noct_unpin_local(env, 3, &term_dict, &temporary, &function);
		return false;
	}

	/* Installs console restoration at process exit. */
	registered = atexit(term_restore) == 0;

	/* Releases the registration roots. */
	(void)noct_unpin_local(env, 3, &term_dict, &temporary, &function);

	/* Reports whether exit restoration was installed. */
	return registered;
}

/* Registers all native Term functions. */
static bool
register_term_functions(
	NoctEnv *env,
	NoctValue *term_dict,
	NoctValue *function)
{
	/* Registers Term.open(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.open",
		"open",
		0,
		term_no_param,
		cfunc_Term_open)) {
		return false;
	}

	/* Registers Term.close(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.close",
		"close",
		0,
		term_no_param,
		cfunc_Term_close)) {
		return false;
	}

	/* Registers Term.isTTY(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.isTTY",
		"isTTY",
		0,
		term_no_param,
		cfunc_Term_isTTY)) {
		return false;
	}

	/* Registers Term.size(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.size",
		"size",
		0,
		term_no_param,
		cfunc_Term_size)) {
		return false;
	}

	/* Registers Term.resized(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.resized",
		"resized",
		0,
		term_no_param,
		cfunc_Term_resized)) {
		return false;
	}

	/* Registers Term.moveTo(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.moveTo",
		"moveTo",
		2,
		term_move_to_param,
		cfunc_Term_moveTo)) {
		return false;
	}

	/* Registers Term.write(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.write",
		"write",
		1,
		term_write_param,
		cfunc_Term_write)) {
		return false;
	}

	/* Registers Term.clear(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.clear",
		"clear",
		0,
		term_no_param,
		cfunc_Term_clear)) {
		return false;
	}

	/* Registers Term.clearToEol(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.clearToEol",
		"clearToEol",
		0,
		term_no_param,
		cfunc_Term_clearToEol)) {
		return false;
	}

	/* Registers Term.setStyle(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.setStyle",
		"setStyle",
		1,
		term_style_param,
		cfunc_Term_setStyle)) {
		return false;
	}

	/* Registers Term.showCursor(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.showCursor",
		"showCursor",
		1,
		term_visible_param,
		cfunc_Term_showCursor)) {
		return false;
	}

	/* Registers Term.flush(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.flush",
		"flush",
		0,
		term_no_param,
		cfunc_Term_flush)) {
		return false;
	}

	/* Registers Term.syncBegin(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.syncBegin",
		"syncBegin",
		0,
		term_no_param,
		cfunc_Term_syncBegin)) {
		return false;
	}

	/* Registers Term.syncEnd(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.syncEnd",
		"syncEnd",
		0,
		term_no_param,
		cfunc_Term_syncEnd)) {
		return false;
	}

	/* Registers Term.readKey(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.readKey",
		"readKey",
		1,
		term_timeout_param,
		cfunc_Term_readKey)) {
		return false;
	}

	/* Registers Term.pendingInput(). */
	if (!register_term_function(
		env,
		term_dict,
		function,
		"Term.pendingInput",
		"pendingInput",
		0,
		term_no_param,
		cfunc_Term_pendingInput)) {
		return false;
	}

	/* Reports successful native function registration. */
	return true;
}

/* Registers one native Term function. */
static bool
register_term_function(
	NoctEnv *env,
	NoctValue *term_dict,
	NoctValue *function,
	const char *global_name,
	const char *field_name,
	size_t param_count,
	const char *param[],
	bool (*cfunc)(NoctEnv *env))
{
	/* Registers the native global function. */
	if (!noct_register_cfunc(
		env,
		global_name,
		param_count,
		param,
		cfunc,
		NULL)) {
		return false;
	}

	/* Reads the newly registered function value. */
	if (!noct_get_global(env, global_name, function))
		return false;

	/* Publishes the function in the Term dictionary. */
	if (!noct_set_dict_elem_cstr(
		env,
		term_dict,
		field_name,
		function)) {
		return false;
	}

	/* Reports successful function registration. */
	return true;
}

/* Publishes every key constant in the Term dictionary. */
static bool
register_term_constants(
	NoctEnv *env,
	NoctValue *term_dict,
	NoctValue *temporary)
{
	size_t index;

	/* Publishes each constant in table order. */
	for (index = 0;
	     index < sizeof(term_consts) / sizeof(term_consts[0]);
	     index++) {
		/* Creates the next integer dictionary element. */
		if (!noct_set_dict_elem_make_int(
			env,
			term_dict,
			term_consts[index].name,
			temporary,
			term_consts[index].value)) {
			return false;
		}
	}

	/* Reports successful constant publication. */
	return true;
}

/*
 * With ENABLE_PROCESSED_INPUT off, Ctrl+C is an ordinary key event on
 * a real Windows console and this handler stays dormant. Some hosts
 * (Wine's tty conhost among them) still route ^C through the console
 * ctrl event; translate it back into a C-c key so the binding works
 * everywhere. Runs on its own thread.
 */
/* Handles console control notifications from the system thread. */
static BOOL WINAPI
ctrl_handler(
	DWORD type)
{
	/* Converts an active Ctrl+C notification into a queued wakeup. */
	if (type == CTRL_C_EVENT && term.open) {
		(void)InterlockedIncrement(&term.ctrl_c_count);

		/* Wakes a key reader when its event object exists. */
		if (term.wake != NULL)
			(void)SetEvent(term.wake);

		/* Reports that the notification was handled. */
		return TRUE;
	}

	/* Leaves unrelated console notifications unhandled. */
	return FALSE;
}

/* Appends one decoded event to the bounded queue. */
static void
queue_push(
	int event)
{
	/* Drops an event when the queue has reached its fixed capacity. */
	if (term.queue_len >= EVENT_QUEUE_SIZE)
		return;

	/* Appends the event at the logical queue tail. */
	term.queue[(term.queue_head + term.queue_len) % EVENT_QUEUE_SIZE] =
		event;
	term.queue_len++;
}

/* Removes one decoded event from the bounded queue. */
static int
queue_pop(
	void)
{
	int event;

	/* Reports an empty queue. */
	if (term.queue_len == 0)
		return -1;

	/* Removes the event at the logical queue head. */
	event = term.queue[term.queue_head];
	term.queue_head = (term.queue_head + 1) % EVENT_QUEUE_SIZE;
	term.queue_len--;

	/* Returns the removed event. */
	return event;
}

/* Converts an ANSI color number to Win32 foreground RGB bits. */
static WORD
ansi_to_rgb_bits(
	int color)
{
	WORD bits;

	/* Starts with every color component disabled. */
	bits = 0;

	/* Maps the ANSI red component. */
	if (color & 1)
		bits |= FOREGROUND_RED;

	/* Maps the ANSI green component. */
	if (color & 2)
		bits |= FOREGROUND_GREEN;

	/* Maps the ANSI blue component. */
	if (color & 4)
		bits |= FOREGROUND_BLUE;

	/* Returns the combined Win32 color bits. */
	return bits;
}

/* Maps a 256-color index to the 16-color console palette. */
static int
color256_to_16(
	int color)
{
	int red;
	int green;
	int blue;
	int bright;
	int base;

	/* Preserves the sentinel for an unspecified color. */
	if (color < 0)
		return -1;

	/* Preserves an existing 16-color palette index. */
	if (color < 16)
		return color;

	/* Maps the grayscale ramp to four console intensities. */
	if (color >= 232) {
		/* Maps the darkest grayscale values to black. */
		if (color < 238)
			return 0;

		/* Maps dark grayscale values to bright black. */
		if (color < 244)
			return 8;

		/* Maps light grayscale values to gray. */
		if (color < 250)
			return 7;

		/* Maps the brightest grayscale values to white. */
		return 15;
	}

	/* Decomposes a 6x6x6 color-cube index. */
	color -= 16;
	red = color / 36;
	green = (color / 6) % 6;
	blue = color % 6;

	/* Selects the console intensity bit. */
	if (red >= 4 ||
	    green >= 4 ||
	    blue >= 4) {
		bright = 8;
	} else {
		bright = 0;
	}

	/* Selects the nearest console RGB components. */
	base = 0;

	/* Selects the red console component. */
	if (red >= 2)
		base |= 1;

	/* Selects the green console component. */
	if (green >= 2)
		base |= 2;

	/* Selects the blue console component. */
	if (blue >= 2)
		base |= 4;

	/* Returns the combined palette index and intensity. */
	return base | bright;
}

/* Replaces the foreground portion of a console attribute. */
static WORD
attr_with_fg(
	WORD attr,
	int color)
{
	WORD color_bits;

	/* Removes the previous foreground components. */
	attr &= (WORD)~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
			FOREGROUND_INTENSITY);

	/* Adds the requested foreground components. */
	color_bits = ansi_to_rgb_bits(color & 7);
	attr |= color_bits;

	/* Adds the requested foreground intensity. */
	if (color & 8)
		attr |= FOREGROUND_INTENSITY;

	/* Returns the updated console attribute. */
	return attr;
}

/* Replaces the background portion of a console attribute. */
static WORD
attr_with_bg(
	WORD attr,
	int color)
{
	WORD color_bits;

	/* Removes the previous background components. */
	attr &= (WORD)~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE |
			BACKGROUND_INTENSITY);

	/* Adds the requested background components. */
	color_bits = ansi_to_rgb_bits(color & 7);
	attr |= (WORD)(color_bits << 4);

	/* Adds the requested background intensity. */
	if (color & 8)
		attr |= BACKGROUND_INTENSITY;

	/* Returns the updated console attribute. */
	return attr;
}

/* Resolves the console attribute after reverse-video processing. */
static WORD
effective_attr(
	void)
{
	WORD attr;
	WORD foreground;
	WORD background;
	WORD remaining;

	/* Reads the currently selected native attribute. */
	attr = term.cur_attr;

	/* Returns the native attribute when reverse video is disabled. */
	if (!term.reverse)
		return attr;

	/* Swaps the foreground and background color nibbles. */
	foreground = attr & 0x0F;
	background = (WORD)((attr >> 4) & 0x0F);
	remaining = attr & (WORD)~0xFF;

	/* Returns the reverse-video console attribute. */
	return (WORD)(remaining | (foreground << 4) | background);
}

/* Writes one UTF-16 run with the current console attribute. */
static void
write_wide(
	const WCHAR *text,
	DWORD length)
{
	DWORD written;
	WORD attr;

	/* Ignores an empty output run. */
	if (length == 0)
		return;

	/* Applies the effective console attribute. */
	attr = effective_attr();
	(void)SetConsoleTextAttribute(term.screen, attr);

	/* Writes the complete UTF-16 run. */
	(void)WriteConsoleW(term.screen, text, length, &written, NULL);
}

/* Converts and writes one escape-free UTF-8 run. */
static void
write_utf8_run(
	const char *utf8,
	int length)
{
	WCHAR stack_buf[256];
	WCHAR *wide;
	int wide_length;

	/* Ignores an empty output run. */
	if (length <= 0)
		return;

	/* Measures the converted UTF-16 run. */
	wide_length = MultiByteToWideChar(
		CP_UTF8,
		0,
		utf8,
		length,
		NULL,
		0);

	/* Ignores an invalid UTF-8 run. */
	if (wide_length <= 0)
		return;

	/* Selects stack storage or allocates a larger conversion buffer. */
	if (wide_length <= (int)(sizeof(stack_buf) / sizeof(stack_buf[0]))) {
		wide = stack_buf;
	} else {
		/* Allocates a conversion buffer for a large UTF-16 run. */
		wide = malloc((size_t)wide_length * sizeof(WCHAR));
		if (wide == NULL)
			return;
	}

	/* Converts the UTF-8 bytes into the selected buffer. */
	(void)MultiByteToWideChar(
		CP_UTF8,
		0,
		utf8,
		length,
		wide,
		wide_length);

	/* Writes the converted console run. */
	write_wide(wide, (DWORD)wide_length);

	/* Releases a dynamically allocated conversion buffer. */
	if (wide != stack_buf)
		free(wide);
}

/* Applies a sequence of SGR parameters to the console attributes. */
static void
apply_sgr(
	const int *params,
	int param_count)
{
	int i;
	int parameter;
	int color;

	/* Treats an empty SGR sequence as a complete reset. */
	if (param_count == 0) {
		term.cur_attr = term.base_attr;
		term.reverse = 0;
		return;
	}

	/* Applies each SGR parameter in source order. */
	for (i = 0; i < param_count; i++) {
		parameter = params[i];

		/* Applies the selected SGR operation. */
		if (parameter == 0) {
			term.cur_attr = term.base_attr;
			term.reverse = 0;
		} else if (parameter == 1) {
			term.cur_attr |= FOREGROUND_INTENSITY;
		} else if (parameter == 22) {
			term.cur_attr &= (WORD)~FOREGROUND_INTENSITY;
		} else if (parameter == 4) {
			term.cur_attr |= COMMON_LVB_UNDERSCORE;
		} else if (parameter == 24) {
			term.cur_attr &= (WORD)~COMMON_LVB_UNDERSCORE;
		} else if (parameter == 7) {
			term.reverse = 1;
		} else if (parameter == 27) {
			term.reverse = 0;
		} else if (parameter >= 30 && parameter <= 37) {
			term.cur_attr = attr_with_fg(
				term.cur_attr,
				parameter - 30);
		} else if (parameter >= 90 && parameter <= 97) {
			term.cur_attr = attr_with_fg(
				term.cur_attr,
				(parameter - 90) | 8);
		} else if (parameter == 39) {
			term.cur_attr = (WORD)((term.cur_attr & (WORD)~0x0F) |
					       (term.base_attr & 0x0F));
		} else if (parameter >= 40 && parameter <= 47) {
			term.cur_attr = attr_with_bg(
				term.cur_attr,
				parameter - 40);
		} else if (parameter >= 100 && parameter <= 107) {
			term.cur_attr = attr_with_bg(
				term.cur_attr,
				(parameter - 100) | 8);
		} else if (parameter == 49) {
			term.cur_attr = (WORD)((term.cur_attr & (WORD)~0xF0) |
					       (term.base_attr & 0xF0));
		} else if ((parameter == 38 || parameter == 48) &&
			   i + 2 < param_count &&
			   params[i + 1] == 5) {
			/* Maps the extended color before selecting its target. */
			color = color256_to_16(params[i + 2]);
			if (color >= 0) {
				/* Applies the mapped foreground or background color. */
				if (parameter == 38) {
					term.cur_attr = attr_with_fg(
						term.cur_attr,
						color);
				} else {
					term.cur_attr = attr_with_bg(
						term.cur_attr,
						color);
				}
			}
			i += 2;
		}
	}
}

/* Tests whether both standard handles are Win32 console handles. */
static int
win32_is_tty(
	void)
{
	DWORD mode;
	HANDLE input;
	HANDLE output;
	BOOL mode_available;

	/* Resolves the standard input and output handles. */
	input = GetStdHandle(STD_INPUT_HANDLE);
	output = GetStdHandle(STD_OUTPUT_HANDLE);

	/* Rejects unavailable standard handles. */
	if (input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE)
		return 0;

	/* Verifies the input console handle first. */
	mode_available = GetConsoleMode(input, &mode);
	if (!mode_available)
		return 0;

	/* Verifies the output console handle second. */
	mode_available = GetConsoleMode(output, &mode);
	if (!mode_available)
		return 0;

	/* Reports two usable console handles. */
	return 1;
}

/* Opens a private Win32 console screen buffer. */
static int
win32_open(
	void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD size;
	SMALL_RECT window;
	DWORD mode;
	BOOL mode_available;
	BOOL info_available;

	/* Preserves an already open console session. */
	if (term.open)
		return 1;

	/* Rejects standard handles that are not Win32 console handles. */
	if (!win32_is_tty())
		return 0;

	/* Captures the standard console handles for this session. */
	term.input = GetStdHandle(STD_INPUT_HANDLE);
	term.original = GetStdHandle(STD_OUTPUT_HANDLE);

	/* Saves the original input mode when the console exposes it. */
	mode_available = GetConsoleMode(term.input, &term.saved_input_mode);
	if (mode_available)
		term.saved_mode_valid = 1;

	/*
	 * Selects raw key and resize events without line input, echo, Ctrl+C
	 * processing, or quick edit.
	 */
	mode = ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
	(void)SetConsoleMode(term.input, mode);

	/* Reads the original visible console dimensions and attributes. */
	info_available = GetConsoleScreenBufferInfo(term.original, &info);
	if (!info_available)
		return 0;

	/* Derives the new buffer size from the original visible window. */
	size.X = (SHORT)(info.srWindow.Right - info.srWindow.Left + 1);
	size.Y = (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1);

	/* Creates the private screen buffer used by the terminal session. */
	term.screen = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL);

	/* Restores the saved input mode after a failed buffer creation. */
	if (term.screen == INVALID_HANDLE_VALUE) {
		/* Restores only a mode that was captured successfully. */
		if (term.saved_mode_valid)
			(void)SetConsoleMode(term.input, term.saved_input_mode);

		/* Reports the failed terminal open. */
		return 0;
	}

	/*
	 * Shrinks the fresh buffer to the visible window so buffer coordinates
	 * are screen coordinates. Applying the window both before and after the
	 * buffer size tolerates either the old or new dimensions being larger.
	 */
	window.Left = 0;
	window.Top = 0;
	window.Right = (SHORT)(size.X - 1);
	window.Bottom = (SHORT)(size.Y - 1);
	(void)SetConsoleWindowInfo(term.screen, TRUE, &window);
	(void)SetConsoleScreenBufferSize(term.screen, size);
	(void)SetConsoleWindowInfo(term.screen, TRUE, &window);

	/* Captures a usable base console attribute. */
	term.base_attr = (WORD)(info.wAttributes & 0xFF);

	/* Substitutes the conventional gray foreground for black on black. */
	if ((term.base_attr & 0x0F) == 0)
		term.base_attr = (WORD)((term.base_attr & 0xF0) | 0x07);

	/* Initializes the output and input state for the new session. */
	term.cur_attr = term.base_attr;
	term.reverse = 0;
	term.queue_head = 0;
	term.queue_len = 0;
	term.pending_high = 0;
	term.resized = 0;
	term.ctrl_c_count = 0;

	/* Creates the persistent control-handler wake event when necessary. */
	if (term.wake == NULL)
		term.wake = CreateEventW(NULL, FALSE, FALSE, NULL);

	/* Installs the Ctrl+C compatibility handler for the active session. */
	(void)SetConsoleCtrlHandler(ctrl_handler, TRUE);

	/* Activates the private screen buffer and publishes the open state. */
	(void)SetConsoleActiveScreenBuffer(term.screen);
	term.open = 1;

	/* Reports a successfully initialized terminal session. */
	return 1;
}

/* Closes the private Win32 console screen buffer. */
static void
win32_close(
	void)
{
	/* Leaves an already closed console session unchanged. */
	if (!term.open)
		return;

	/* Removes the console control handler before restoring the screen. */
	(void)SetConsoleCtrlHandler(ctrl_handler, FALSE);

	/* Restores the original console buffer and releases the private one. */
	(void)SetConsoleActiveScreenBuffer(term.original);
	(void)CloseHandle(term.screen);
	term.screen = INVALID_HANDLE_VALUE;

	/* Restores a saved input mode after the private buffer is gone. */
	if (term.saved_mode_valid)
		(void)SetConsoleMode(term.input, term.saved_input_mode);

	/* Publishes the completed close operation. */
	term.open = 0;
}

/* Reads the visible Win32 console dimensions. */
static int
win32_size(
	unsigned *rows,
	unsigned *columns)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE target;
	BOOL info_available;

	/* Selects the private buffer or the current standard output handle. */
	if (term.open)
		target = term.screen;
	else
		target = GetStdHandle(STD_OUTPUT_HANDLE);

	/* Reads the selected console screen buffer information. */
	info_available = GetConsoleScreenBufferInfo(target, &info);
	if (!info_available)
		return 0;

	/* Publishes the visible window dimensions. */
	*rows = (unsigned)(info.srWindow.Bottom - info.srWindow.Top + 1);
	*columns = (unsigned)(info.srWindow.Right - info.srWindow.Left + 1);

	/* Reports that both dimensions were read. */
	return 1;
}

/* Captures and clears the Win32 console resize flag. */
static int
win32_resized(
	void)
{
	int resized;

	/* Captures and clears the normalized resize state. */
	resized = term.resized ? 1 : 0;
	term.resized = 0;

	/* Returns the resize state captured before clearing it. */
	return resized;
}

/* Moves the Win32 console cursor to one-based coordinates. */
static int
win32_move_to(
	unsigned row,
	unsigned column)
{
	COORD pos;

	/* Rejects movement without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Converts the one-based API coordinates to Win32 coordinates. */
	pos.X = (SHORT)(column - 1);
	pos.Y = (SHORT)(row - 1);

	/* Applies the requested cursor position to the private buffer. */
	(void)SetConsoleCursorPosition(term.screen, pos);

	/* Reports that the movement was issued. */
	return 1;
}

/* Writes UTF-8 text and interprets embedded SGR sequences. */
static int
win32_write(
	const char *utf8,
	size_t length)
{
	size_t i;
	size_t run_start;
	int params[8];
	int nparam;
	int acc;
	int has_digit;
	unsigned char c;

	/* Rejects output without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Scans plain UTF-8 runs and embedded escape sequences in order. */
	i = 0;
	run_start = 0;
	while (i < length) {
		/* Extends the current plain run through non-escape bytes. */
		if ((unsigned char)utf8[i] != 0x1B) {
			i++;

			/* Continues scanning the current plain run. */
			continue;
		}

		/* Writes the plain run that precedes this escape byte. */
		write_utf8_run(utf8 + run_start, (int)(i - run_start));

		/* Parses a CSI sequence or discards a lone escape byte. */
		if (i + 1 < length && utf8[i + 1] == '[') {
			/* Initializes the bounded SGR parameter parser. */
			nparam = 0;
			acc = 0;
			has_digit = 0;
			i += 2;

			/* Accumulates every decimal parameter before the final byte. */
			while (i < length) {
				c = (unsigned char)utf8[i];

				/* Accumulates the next decimal digit. */
				if (c >= '0' && c <= '9') {
					acc = acc * 10 + (c - '0');
					has_digit = 1;
					i++;

					/* Continues the current decimal parameter. */
					continue;
				}

				/* Commits one parameter at a semicolon separator. */
				if (c == ';') {
					/* Stores the parameter while capacity remains. */
					if (nparam < 8)
						params[nparam++] = has_digit ? acc : 0;
					acc = 0;
					has_digit = 0;
					i++;

					/* Continues with the next parameter. */
					continue;
				}

				/* Stops at the sequence's final or unsupported byte. */
				break;
			}

			/* Commits a final nonempty decimal parameter. */
			if (has_digit && nparam < 8)
				params[nparam++] = acc;

			/* Consumes a present final byte after applying SGR. */
			if (i < length) {
				/* Applies only the supported SGR final byte. */
				if (utf8[i] == 'm')
					apply_sgr(params, nparam);

				/* Swallows every recognized or unsupported final byte. */
				i++;
			}
		} else {
			/* Drops a lone escape byte or a non-CSI sequence prefix. */
			i++;
		}

		/* Starts the next plain run after the consumed escape bytes. */
		run_start = i;
	}

	/* Writes the trailing plain UTF-8 run. */
	write_utf8_run(utf8 + run_start, (int)(i - run_start));

	/* Reports that the complete input string was consumed. */
	return 1;
}

/* Clears the complete private Win32 console buffer. */
static int
win32_clear(
	void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD home;
	DWORD cells;
	DWORD written;
	WORD attr;
	BOOL info_available;

	/* Rejects clearing without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Reads the complete private screen buffer dimensions. */
	info_available = GetConsoleScreenBufferInfo(term.screen, &info);
	if (!info_available)
		return 0;

	/* Derives the home coordinate and total cell count. */
	home.X = 0;
	home.Y = 0;
	cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;

	/* Replaces every cell with a space. */
	(void)FillConsoleOutputCharacterW(
		term.screen,
		L' ',
		cells,
		home,
		&written);

	/* Applies the effective attribute to every cleared cell. */
	attr = effective_attr();
	(void)FillConsoleOutputAttribute(
		term.screen,
		attr,
		cells,
		home,
		&written);

	/* Returns the cursor to the screen origin. */
	(void)SetConsoleCursorPosition(term.screen, home);

	/* Reports that the clear operations were issued. */
	return 1;
}

/* Clears the private Win32 console buffer to the end of the line. */
static int
win32_clear_to_eol(
	void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	DWORD cells;
	DWORD written;
	WORD attr;
	BOOL info_available;

	/* Rejects clearing without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Reads the current cursor position and line width. */
	info_available = GetConsoleScreenBufferInfo(term.screen, &info);
	if (!info_available)
		return 0;

	/* Derives the number of cells from the cursor through line end. */
	cells = (DWORD)(info.dwSize.X - info.dwCursorPosition.X);

	/* Replaces the remaining line cells with spaces. */
	(void)FillConsoleOutputCharacterW(
		term.screen,
		L' ',
		cells,
		info.dwCursorPosition,
		&written);

	/* Applies the effective attribute to the cleared line cells. */
	attr = effective_attr();
	(void)FillConsoleOutputAttribute(
		term.screen,
		attr,
		cells,
		info.dwCursorPosition,
		&written);

	/* Reports that both line-clear operations were issued. */
	return 1;
}

/* Applies one Term style to subsequent Win32 console output. */
static int
win32_set_style(
	const struct win32_term_style *style)
{
	int color;

	/* Resets the native attributes before applying the requested style. */
	term.cur_attr = term.base_attr;
	term.reverse = style->reverse ? 1 : 0;

	/* Enables the requested foreground intensity. */
	if (style->bold)
		term.cur_attr |= FOREGROUND_INTENSITY;

	/* Enables the requested underline attribute. */
	if (style->underline)
		term.cur_attr |= COMMON_LVB_UNDERSCORE;

	/* Maps and applies an explicitly requested foreground color. */
	color = color256_to_16(style->foreground);
	if (color >= 0)
		term.cur_attr = attr_with_fg(term.cur_attr, color);

	/* Maps and applies an explicitly requested background color. */
	color = color256_to_16(style->background);
	if (color >= 0)
		term.cur_attr = attr_with_bg(term.cur_attr, color);

	/* Reports that the complete style was applied. */
	return 1;
}

/* Changes the private Win32 console cursor visibility. */
static int
win32_show_cursor(
	int visible)
{
	CONSOLE_CURSOR_INFO cursor;
	BOOL info_available;

	/* Rejects cursor changes without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Reads the current cursor configuration. */
	info_available = GetConsoleCursorInfo(term.screen, &cursor);
	if (!info_available)
		return 0;

	/* Applies the normalized visibility while preserving cursor size. */
	cursor.bVisible = visible ? TRUE : FALSE;
	(void)SetConsoleCursorInfo(term.screen, &cursor);

	/* Reports that the cursor update was issued. */
	return 1;
}

/* Flushes the unbuffered Win32 console backend. */
static int
win32_flush(
	void)
{
	/* Reports success because WriteConsoleW output is immediate. */
	return 1;
}

/*
 * Input
 */

/* Maps one non-character virtual key to a Term event. */
static int
vk_to_special(
	WORD vk)
{
	/* Maps each supported navigation key to its Term event. */
	switch (vk) {
	case VK_UP:
		/* Returns the upward navigation event. */
		return TERM_KEY_UP;
	case VK_DOWN:
		/* Returns the downward navigation event. */
		return TERM_KEY_DOWN;
	case VK_RIGHT:
		/* Returns the rightward navigation event. */
		return TERM_KEY_RIGHT;
	case VK_LEFT:
		/* Returns the leftward navigation event. */
		return TERM_KEY_LEFT;
	case VK_HOME:
		/* Returns the beginning navigation event. */
		return TERM_KEY_HOME;
	case VK_END:
		/* Returns the ending navigation event. */
		return TERM_KEY_END;
	case VK_PRIOR:
		/* Returns the page-up navigation event. */
		return TERM_KEY_PGUP;
	case VK_NEXT:
		/* Returns the page-down navigation event. */
		return TERM_KEY_PGDN;
	case VK_INSERT:
		/* Returns the insert-key event. */
		return TERM_KEY_INSERT;
	case VK_DELETE:
		/* Returns the delete-key event. */
		return TERM_KEY_DELETE;
	default:
		/* Maps the contiguous function-key virtual codes. */
		if (vk >= VK_F1 && vk <= VK_F12)
			return TERM_KEY_F1 + (vk - VK_F1);

		/* Reports an unsupported virtual key. */
		return -1;
	}
}

/* Translates one Win32 key record into queued Term events. */
static void
process_key_event(
	const KEY_EVENT_RECORD *key)
{
	DWORD state;
	int alt;
	int ctrl;
	int shift;
	int altgr;
	int special;
	int event;
	int repeat;
	int i;
	unsigned int ch;
	unsigned int virtual_character;

	/* Decodes the modifier state attached to this console record. */
	state = key->dwControlKeyState;
	alt = (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
	ctrl = (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
	shift = (state & SHIFT_PRESSED) != 0;

	/*
	 * Recognizes AltGr as right Alt plus left Ctrl only when Windows has
	 * already translated the combination into a character.
	 */
	altgr = (state & RIGHT_ALT_PRESSED) != 0 &&
		(state & LEFT_CTRL_PRESSED) != 0 &&
		key->uChar.UnicodeChar != 0;

	/* Reads the translated UTF-16 code unit. */
	ch = (unsigned int)key->uChar.UnicodeChar;

	/* Handles the one character-producing key-release convention. */
	if (!key->bKeyDown) {
		/* Queues Alt+numpad text delivered on the Alt key release. */
		if (key->wVirtualKeyCode == VK_MENU && ch != 0)
			queue_push((int)ch);

		/* Finishes processing the key-release record. */
		return;
	}

	/* Ignores key-down records for pure modifier and lock keys. */
	switch (key->wVirtualKeyCode) {
	case VK_SHIFT:
		/* Ignores a standalone Shift press. */
		return;
	case VK_CONTROL:
		/* Ignores a standalone Control press. */
		return;
	case VK_MENU:
		/* Ignores a standalone Alt press. */
		return;
	case VK_LWIN:
		/* Ignores a standalone left Windows-key press. */
		return;
	case VK_RWIN:
		/* Ignores a standalone right Windows-key press. */
		return;
	case VK_CAPITAL:
		/* Ignores a standalone Caps Lock press. */
		return;
	case VK_NUMLOCK:
		/* Ignores a standalone Num Lock press. */
		return;
	case VK_SCROLL:
		/* Ignores a standalone Scroll Lock press. */
		return;
	default:
		/* Continues translating every non-modifier key. */
		break;
	}

	/* Normalizes the console repeat count to at least one event. */
	if (key->wRepeatCount > 0)
		repeat = key->wRepeatCount;
	else
		repeat = 1;

	/* Starts with no decoded Term event. */
	event = -1;

	/* Translates the key according to its virtual key and character data. */
	special = vk_to_special(key->wVirtualKeyCode);
	if (special >= 0) {
		event = special;

		/* Preserves Meta on a special navigation key. */
		if (alt)
			event |= TERM_MOD_META;

		/* Preserves Ctrl on a special navigation key. */
		if (ctrl)
			event |= TERM_MOD_CTRL;

		/* Preserves Shift on a special navigation key. */
		if (shift)
			event |= TERM_MOD_SHIFT;
	} else if (key->wVirtualKeyCode == VK_BACK) {
		/* Maps Backspace to DEL like the POSIX terminal backend. */
		event = 0x7F;

		/* Preserves only the Meta modifier on Backspace. */
		if (alt)
			event |= TERM_MOD_META;
	} else if (ch == 0) {
		/* Derives uncooked Alt and Ctrl chords from the virtual key. */
		if (alt || ctrl) {
			/* Maps the virtual key without changing the console state. */
			virtual_character = MapVirtualKeyW(
				key->wVirtualKeyCode,
				MAPVK_VK_TO_CHAR);
			virtual_character &= 0x7FFFFFFF;

			/* Normalizes an unshifted ASCII letter to lowercase. */
			if (virtual_character >= 'A' &&
			    virtual_character <= 'Z' &&
			    !shift) {
				virtual_character = virtual_character - 'A' + 'a';
			}

			/* Publishes a printable virtual character and its modifiers. */
			if (virtual_character >= 0x20) {
				event = (int)virtual_character;

				/* Preserves Meta on the derived virtual character. */
				if (alt)
					event |= TERM_MOD_META;

				/* Preserves Ctrl on the derived virtual character. */
				if (ctrl)
					event |= TERM_MOD_CTRL;
			}
		}
	} else if (altgr) {
		/* Treats an AltGr-produced character as unmodified text. */
		event = (int)ch;
	} else if (ch < 0x20) {
		/* Maps translated control characters like the POSIX backend. */
		if (ch == 0x09 && !ctrl) {
			event = '\t';
		} else if (ch == 0x0D && !ctrl) {
			event = '\r';
		} else if (ch == 0x1B) {
			event = 0x1B;
		} else if (ch == 0x00) {
			event = TERM_MOD_CTRL | 0x20;	/* C-SPC / C-@ */
		} else if (ch == 0x0A) {
			event = TERM_MOD_CTRL | 'j';
		} else if (ch <= 0x1A) {
			event = TERM_MOD_CTRL | (int)(ch + 0x60);
		} else {
			event = TERM_MOD_CTRL | (int)(ch + 0x40);
		}

		/* Preserves Meta on a successfully decoded control character. */
		if (event >= 0 && alt)
			event |= TERM_MOD_META;
	} else if (ch >= 0xD800 && ch < 0xDC00) {
		/* Saves a high surrogate until the following console record. */
		term.pending_high = (WCHAR)ch;

		/* Defers the incomplete supplementary character. */
		return;
	} else if (ch >= 0xDC00 && ch < 0xE000) {
		/* Combines a low surrogate only with a pending high surrogate. */
		if (term.pending_high != 0) {
			event = 0x10000 +
				(((int)term.pending_high - 0xD800) << 10) +
				((int)ch - 0xDC00);
			term.pending_high = 0;
		}
	} else {
		/* Starts a printable event with Windows' translated character. */
		event = (int)ch;

		/* Preserves Ctrl on spaces, digits, and symbols alike. */
		if (ch == 0x20 && ctrl)
			event |= TERM_MOD_CTRL;
		else if (ctrl)
			event |= TERM_MOD_CTRL;

		/* Preserves Meta while Shift remains folded into the character. */
		if (alt)
			event |= TERM_MOD_META;
	}

	/* Ignores records that did not produce a complete Term event. */
	if (event < 0)
		return;

	/* Queues the decoded event once for every console repeat. */
	for (i = 0; i < repeat; i++)
		queue_push(event);
}

/* Drains available Win32 input records into the bounded event queue. */
static void
drain_input(
	void)
{
	INPUT_RECORD records[READ_CHUNK];
	DWORD count;
	DWORD available;
	DWORD i;
	BOOL count_available;
	BOOL read_succeeded;

	/* Drains bounded chunks while the application queue has capacity. */
	for (;;) {
		/* Leaves excess input in the console instead of dropping it. */
		if (term.queue_len >= EVENT_QUEUE_SIZE - READ_CHUNK * 4)
			return;

		/* Reads the number of console records currently available. */
		count_available = GetNumberOfConsoleInputEvents(
			term.input,
			&available);
		if (!count_available)
			return;

		/* Stops before a blocking read when the console queue is empty. */
		if (available == 0)
			return;

		/* Pulls the next bounded chunk from the console input queue. */
		read_succeeded = ReadConsoleInputW(
			term.input,
			records,
			READ_CHUNK,
			&count);
		if (!read_succeeded)
			return;

		/* Translates every record in the retrieved console chunk. */
		for (i = 0; i < count; i++) {
			/* Dispatches key records and records resize notifications. */
			if (records[i].EventType == KEY_EVENT)
				process_key_event(&records[i].Event.KeyEvent);
			else if (records[i].EventType ==
				 WINDOW_BUFFER_SIZE_EVENT)
				term.resized = 1;
		}

		/* Stops after a short read has exhausted available records. */
		if (count < READ_CHUNK)
			return;
	}
}

/* Waits for and returns one decoded Win32 input event. */
static int
win32_read_key(
	int timeout_ms)
{
	DWORD start;
	DWORD elapsed;
	DWORD wait;
	DWORD wait_result;
	DWORD handle_count;
	LONG pending_c;
	HANDLE handles[2];
	int event;

	/* Rejects input without an active private screen buffer. */
	if (!term.open)
		return -1;

	/* Captures the deadline origin before checking queued input. */
	start = GetTickCount();

	/* Rechecks asynchronous wakeups and the deadline after every wait. */
	for (;;) {
		/* Transfers every pending control-handler Ctrl+C notification. */
		pending_c = InterlockedExchange(&term.ctrl_c_count, 0);
		while (pending_c-- > 0)
			queue_push(TERM_MOD_CTRL | 'c');

		/* Returns the oldest event before entering a blocking wait. */
		event = queue_pop();
		if (event >= 0)
			return event;

		/* Derives an infinite or deadline-relative wait interval. */
		if (timeout_ms < 0) {
			wait = INFINITE;
		} else {
			elapsed = GetTickCount() - start;

			/* Reports an expired finite deadline. */
			if (elapsed > (DWORD)timeout_ms)
				return -1;

			wait = (DWORD)timeout_ms - elapsed;
		}

		/* Marks the runtime thread as blocked before the system wait. */
		if (term.env != NULL)
			noct_enter_blocking(term.env);

		/* Builds the console and optional control-wakeup wait set. */
		handles[0] = term.input;
		handles[1] = term.wake;
		if (term.wake != NULL)
			handle_count = 2;
		else
			handle_count = 1;

		/* Waits for console input, a control wakeup, or the deadline. */
		wait_result = WaitForMultipleObjects(
			handle_count,
			handles,
			FALSE,
			wait);

		/* Re-enters the runtime before interpreting every wait result. */
		if (term.env != NULL)
			noct_leave_blocking(term.env);

		/* Drains input or reports a failed or expired console wait. */
		if (wait_result == WAIT_OBJECT_0)
			drain_input();
		else if (wait_result == WAIT_FAILED)
			return -2;
		else if (wait_result != WAIT_OBJECT_0 + 1)
			return -1;

		/* Rechecks after a wakeup, resize, or swallowed input record. */
	}
}

/* Reports whether decoded or immediately available input is pending. */
static int
win32_pending_input(
	void)
{
	int pending;

	/* Rejects input without an active private screen buffer. */
	if (!term.open)
		return 0;

	/* Reports an asynchronous Ctrl+C notification immediately. */
	if (term.ctrl_c_count > 0)
		return 1;

	/* Reports an already decoded event immediately. */
	if (term.queue_len > 0)
		return 1;

	/* Pulls any immediately available console records into the queue. */
	drain_input();

	/* Normalizes the queue state after the nonblocking drain. */
	pending = term.queue_len > 0 ? 1 : 0;

	/* Returns the normalized pending-input state. */
	return pending;
}

/* Restores the console when the process exits. */
static void
term_restore(
	void)
{
	/* Closes the backend through its idempotent path. */
	win32_close();
}

/* Returns one integer through the common VM integer path. */
static bool
return_int(
	NoctEnv *env,
	int value)
{
	NoctValue result;
	bool succeeded;

	/* Initializes the result before it becomes a GC root. */
	memset(&result, 0, sizeof(result));

	/* Pins the return value during integer construction. */
	if (!noct_pin_local(env, 1, &result))
		return false;

	/* Constructs and publishes the requested integer. */
	succeeded = noct_set_return_make_int(env, &result, value);

	/* Releases the temporary return root. */
	(void)noct_unpin_local(env, 1, &result);

	/* Reports whether the integer was returned. */
	return succeeded;
}

/* Reads one integer argument while accepting int and long values. */
static bool
get_int_arg(
	NoctEnv *env,
	uint32_t index,
	int *result)
{
	NoctValue value;
	int64_t long_value;
	int int_value;
	bool succeeded;

	/* Initializes the argument before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the argument during type conversion. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Tries the long representation first. */
	succeeded = noct_get_arg_check_long(env, index, &value, &long_value);

	/* Falls back to the common integer representation. */
	if (!succeeded) {
			succeeded = noct_get_arg_check_int(
				env,
				index,
				&value,
				&int_value);

		/* Converts a successful integer fallback to the long form. */
		if (succeeded)
			long_value = int_value;
	}

	/* Publishes a successfully converted argument. */
	if (succeeded)
		*result = (int)long_value;

	/* Releases the converted argument root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports whether the argument was converted. */
	return succeeded;
}

/* Reads one optional integer from a style dictionary. */
static bool
read_style_field(
	NoctEnv *env,
	NoctValue *style,
	NoctValue *temporary,
	const char *name,
	int *value)
{
	bool has_value;

	/* Tests whether the optional field is present. */
	has_value = false;
	if (!noct_check_dict_key_cstr(env, style, name, &has_value))
		return false;

	/* Leaves the caller's default value unchanged when absent. */
	if (!has_value)
		return true;

	/* Reads the present integer field. */
	if (!noct_get_dict_elem_check_int(
		env,
		style,
		name,
		temporary,
		value)) {
		return false;
	}

	/* Reports a valid optional style field. */
	return true;
}

/* Implements Term.open(). */
static bool
cfunc_Term_open(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Opens the Win32 terminal backend. */
	result = win32_open();

	/* Returns the backend status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.close(). */
static bool
cfunc_Term_close(
	NoctEnv *env)
{
	bool succeeded;

	/* Restores and closes the Win32 terminal backend. */
	win32_close();

	/* Returns the successful close status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.isTTY(). */
static bool
cfunc_Term_isTTY(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Queries the Win32 console handles. */
	result = win32_is_tty();

	/* Returns the normalized console flag. */
	succeeded = return_int(env, result);

	/* Reports whether the flag was returned. */
	return succeeded;
}

/* Implements Term.size(). */
static bool
cfunc_Term_size(
	NoctEnv *env)
{
	NoctValue result;
	NoctValue temporary;
	unsigned rows;
	unsigned columns;

	/* Reads the backend dimensions with portable fallbacks. */
	rows = 24;
	columns = 80;
	(void)win32_size(&rows, &columns);

	/* Initializes the values before they become GC roots. */
	memset(&result, 0, sizeof(result));
	memset(&temporary, 0, sizeof(temporary));

	/* Pins the size dictionary and integer temporary. */
	if (!noct_pin_local(env, 2, &result, &temporary))
		return false;

	/* Creates the size dictionary. */
	if (!noct_make_empty_dict(env, &result)) {
		(void)noct_unpin_local(env, 2, &result, &temporary);
		return false;
	}

	/* Publishes the row count. */
	if (!noct_set_dict_elem_make_int(
		env,
		&result,
		"rows",
		&temporary,
		(int)rows)) {
		(void)noct_unpin_local(env, 2, &result, &temporary);
		return false;
	}

	/* Publishes the column count. */
	if (!noct_set_dict_elem_make_int(
		env,
		&result,
		"cols",
		&temporary,
		(int)columns)) {
		(void)noct_unpin_local(env, 2, &result, &temporary);
		return false;
	}

	/* Returns the completed size dictionary. */
	if (!noct_set_return(env, &result)) {
		(void)noct_unpin_local(env, 2, &result, &temporary);
		return false;
	}

	/* Releases the size dictionary roots. */
	(void)noct_unpin_local(env, 2, &result, &temporary);

	/* Reports successful size construction. */
	return true;
}

/* Implements Term.resized(). */
static bool
cfunc_Term_resized(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Captures and clears the backend resize flag. */
	result = win32_resized();

	/* Returns the captured resize flag. */
	succeeded = return_int(env, result);

	/* Reports whether the flag was returned. */
	return succeeded;
}

/* Implements Term.moveTo(). */
static bool
cfunc_Term_moveTo(
	NoctEnv *env)
{
	int row;
	int column;
	int result;
	bool succeeded;

	/* Reads the target row. */
	if (!get_int_arg(env, 0, &row))
		return false;

	/* Reads the target column. */
	if (!get_int_arg(env, 1, &column))
		return false;

	/* Moves only to valid one-based coordinates. */
	if (row > 0 && column > 0)
		result = win32_move_to((unsigned)row, (unsigned)column);
	else
		result = 0;

	/* Returns the backend movement status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.write(). */
static bool
cfunc_Term_write(
	NoctEnv *env)
{
	NoctValue value;
	const char *text;
	size_t length;
	int result;
	bool succeeded;

	/* Initializes the text before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the text while the backend consumes its bytes. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Reads the string to write. */
	succeeded = noct_get_arg_check_string(env, 0, &value, &text);
	if (!succeeded) {
		(void)noct_unpin_local(env, 1, &value);
		return false;
	}

	/* Writes the complete UTF-8 string through the backend. */
	length = strlen(text);
	result = win32_write(text, length);

	/* Releases the consumed text root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Returns the backend write status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.clear(). */
static bool
cfunc_Term_clear(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Clears the Win32 screen buffer. */
	result = win32_clear();

	/* Returns the backend clear status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.clearToEol(). */
static bool
cfunc_Term_clearToEol(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Clears the current line through the backend. */
	result = win32_clear_to_eol();

	/* Returns the backend clear status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.setStyle(). */
static bool
cfunc_Term_setStyle(
	NoctEnv *env)
{
	NoctValue style;
	NoctValue temporary;
	struct win32_term_style native;
	int result;
	int value;
	bool succeeded;

	/* Initializes the optional native style fields. */
	native.foreground = -1;
	native.background = -1;
	native.bold = false;
	native.reverse = false;
	native.underline = false;

	/* Initializes the values before they become GC roots. */
	memset(&style, 0, sizeof(style));
	memset(&temporary, 0, sizeof(temporary));

	/* Pins the style dictionary and lookup temporary. */
	if (!noct_pin_local(env, 2, &style, &temporary))
		return false;

	/* Reads the style dictionary. */
	if (!noct_get_arg_check_dict(env, 0, &style)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}

	/* Reads the optional foreground color. */
	if (!read_style_field(
		env,
		&style,
		&temporary,
		"fg",
		&native.foreground)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}

	/* Reads the optional background color. */
	if (!read_style_field(
		env,
		&style,
		&temporary,
		"bg",
		&native.background)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}

	/* Reads the optional bold flag. */
	value = native.bold;
	if (!read_style_field(
		env,
		&style,
		&temporary,
		"bold",
		&value)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}
	native.bold = value;

	/* Reads the optional reverse-video flag. */
	value = native.reverse;
	if (!read_style_field(
		env,
		&style,
		&temporary,
		"reverse",
		&value)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}
	native.reverse = value;

	/* Reads the optional underline flag. */
	value = native.underline;
	if (!read_style_field(
		env,
		&style,
		&temporary,
		"underline",
		&value)) {
		(void)noct_unpin_local(env, 2, &style, &temporary);
		return false;
	}
	native.underline = value;

	/* Applies the completed native style. */
	result = win32_set_style(&native);

	/* Returns the backend style status while roots remain valid. */
	succeeded = return_int(env, result);

	/* Releases the style dictionary roots. */
	(void)noct_unpin_local(env, 2, &style, &temporary);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.showCursor(). */
static bool
cfunc_Term_showCursor(
	NoctEnv *env)
{
	int visible;
	int result;
	bool succeeded;

	/* Reads the requested cursor visibility. */
	if (!get_int_arg(env, 0, &visible))
		return false;

	/* Applies the normalized visibility through the backend. */
	result = win32_show_cursor(visible != 0);

	/* Returns the backend visibility status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.flush(). */
static bool
cfunc_Term_flush(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Flushes the immediate Win32 backend. */
	result = win32_flush();

	/* Returns the backend flush status. */
	succeeded = return_int(env, result);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements the no-op Term.syncBegin(). */
static bool
cfunc_Term_syncBegin(
	NoctEnv *env)
{
	bool succeeded;

	/* Returns success because Win32 console writes are immediate. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements the no-op Term.syncEnd(). */
static bool
cfunc_Term_syncEnd(
	NoctEnv *env)
{
	bool succeeded;

	/* Returns success because Win32 console writes are immediate. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.readKey(). */
static bool
cfunc_Term_readKey(
	NoctEnv *env)
{
	int timeout;
	int result;
	bool succeeded;

	/* Reads the requested input timeout. */
	if (!get_int_arg(env, 0, &timeout))
		return false;

	/* Waits for one decoded Win32 key event. */
	result = win32_read_key(timeout);

	/* Returns the backend key event. */
	succeeded = return_int(env, result);

	/* Reports whether the event was returned. */
	return succeeded;
}

/* Implements Term.pendingInput(). */
static bool
cfunc_Term_pendingInput(
	NoctEnv *env)
{
	int result;
	bool succeeded;

	/* Queries the backend input queues. */
	result = win32_pending_input();

	/* Returns the pending-input flag. */
	succeeded = return_int(env, result);

	/* Reports whether the flag was returned. */
	return succeeded;
}

#else

/* Not Windows: this translation unit is empty. */
typedef int noct_api_term_win32_unused;

#endif /* NOCT_TARGET_WINDOWS */
