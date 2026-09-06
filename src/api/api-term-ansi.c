/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The ANSI terminal API.
 *
 * A terminal abstraction for writing full-screen console programs.
 *
 * A direct ANSI/VT100 implementation with no curses dependency.
 *
 *  - Output is buffered; Term.flush() writes the whole frame in one
 *    write(2). Redisplay builds a frame and flushes once.
 *  - Input is decoded by a state machine into key events. A key event
 *    is an integer in the GNU Emacs representation: the Unicode
 *    codepoint (or a special-key constant in the private use area)
 *    OR-ed with modifier bits (meta = 1<<27, control = 1<<26,
 *    shift = 1<<25).
 *  - Meta is the ESC prefix (metaSendsEscape). 8-bit meta is not
 *    supported: it is ambiguous with UTF-8 lead bytes.
 *
 * The interface deliberately exposes no escape sequences, so other
 * backends (Win32 console, DOS text mode, a GUI) can replace this
 * implementation without touching callers. Non-POSIX builds get a
 * stub in which Term.isTTY() returns 0 and Term.open() fails.
 */

#include <noct/noct.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(NOCT_TARGET_POSIX)
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(NOCT_TARGET_WINDOWS)

/* The Win32 console backend lives in api-term-win32.c. */

#elif !defined(NOCT_TARGET_POSIX)

/*
 * Registers an empty Term API on unsupported platforms.
 */
NOCT_DLL
bool
noct_register_api_term(
	NoctEnv *env)
{
	NoctValue term_dict;

	/* Initializes the dictionary before it becomes a GC root. */
	memset(&term_dict, 0, sizeof(term_dict));

	/* Pins the Term dictionary during publication. */
	if (!noct_pin_local(env, 1, &term_dict))
		return false;

	/* Creates the empty Term dictionary. */
	if (!noct_make_empty_dict(env, &term_dict)) {
		(void)noct_unpin_local(env, 1, &term_dict);
		return false;
	}

	/* Publishes the unsupported-platform dictionary. */
	if (!noct_set_global(env, "Term", &term_dict)) {
		(void)noct_unpin_local(env, 1, &term_dict);
		return false;
	}

	/* Releases the published dictionary root. */
	(void)noct_unpin_local(env, 1, &term_dict);

	/* Reports successful stub registration. */
	return true;
}

#else /* NOCT_TARGET_POSIX */

/* Emacs-compatible key modifier bits. */
#define MOD_META	(1 << 27)
#define MOD_CTRL	(1 << 26)
#define MOD_SHIFT	(1 << 25)

/* Special keys live in the Unicode private use area. */
#define KEY_BASE	0xE000
#define KEY_UP		(KEY_BASE + 0)
#define KEY_DOWN	(KEY_BASE + 1)
#define KEY_RIGHT	(KEY_BASE + 2)
#define KEY_LEFT	(KEY_BASE + 3)
#define KEY_HOME	(KEY_BASE + 4)
#define KEY_END		(KEY_BASE + 5)
#define KEY_PGUP	(KEY_BASE + 6)
#define KEY_PGDN	(KEY_BASE + 7)
#define KEY_INSERT	(KEY_BASE + 8)
#define KEY_DELETE	(KEY_BASE + 9)
#define KEY_F1		(KEY_BASE + 11)	/* F1..F12 are KEY_F1 + n */

/* ESC disambiguation timeout. */
#define ESC_TIMEOUT_MS	50

/* A target without input parity checking has no flag to clear. */
#if !defined(INPCK)
#define INPCK		0
#endif

struct term_state {
	bool open;
	struct termios saved;
	bool saved_valid;

	/* Output frame buffer. */
	char *out;
	size_t out_len;
	size_t out_alloc;

	/* Input ring. */
	unsigned char in[8192];
	size_t in_head;
	size_t in_len;

	/* The tty went away (read() returned 0). */
	bool eof;

	/* SIGWINCH flag. */
	volatile sig_atomic_t resized;
};

struct term_const {
	const char *name;
	int value;
};

static struct term_state term;

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
	{"META",	MOD_META},
	{"CTRL",	MOD_CTRL},
	{"SHIFT",	MOD_SHIFT},
	{"KEY_UP",	KEY_UP},
	{"KEY_DOWN",	KEY_DOWN},
	{"KEY_RIGHT",	KEY_RIGHT},
	{"KEY_LEFT",	KEY_LEFT},
	{"KEY_HOME",	KEY_HOME},
	{"KEY_END",	KEY_END},
	{"KEY_PGUP",	KEY_PGUP},
	{"KEY_PGDN",	KEY_PGDN},
	{"KEY_INSERT",	KEY_INSERT},
	{"KEY_DELETE",	KEY_DELETE},
	{"KEY_F1",	KEY_F1},
	{"KEY_TAB",	'\t'},
	{"KEY_RET",	'\r'},
	{"KEY_ESC",	0x1B},
	{"KEY_BS",	0x7F},
};

static bool register_term_functions(NoctEnv *env, NoctValue *term_dict);
static bool register_term_function(NoctEnv *env, NoctValue *term_dict, const char *global_name, const char *field_name, size_t param_count, const char *param[], bool (*cfunc)(NoctEnv *env));
static bool register_term_constants(NoctEnv *env, NoctValue *term_dict, NoctValue *temporary);
static void on_sigwinch(int signal_number);
static void term_restore(void);
static bool out_put(NoctEnv *env, const char *string, size_t length);
static bool out_put_cstr(NoctEnv *env, const char *string);
static bool return_int(NoctEnv *env, int64_t value);
static bool cfunc_Term_isTTY(NoctEnv *env);
static bool cfunc_Term_open(NoctEnv *env);
static bool cfunc_Term_close(NoctEnv *env);
static bool cfunc_Term_size(NoctEnv *env);
static bool cfunc_Term_resized(NoctEnv *env);
static bool get_int_arg(NoctEnv *env, uint32_t index, int64_t *result);
static bool cfunc_Term_moveTo(NoctEnv *env);
static bool cfunc_Term_syncBegin(NoctEnv *env);
static bool cfunc_Term_syncEnd(NoctEnv *env);
static bool cfunc_Term_write(NoctEnv *env);
static bool cfunc_Term_clear(NoctEnv *env);
static bool cfunc_Term_clearToEol(NoctEnv *env);
static bool cfunc_Term_setStyle(NoctEnv *env);
static bool cfunc_Term_showCursor(NoctEnv *env);
static bool cfunc_Term_flush(NoctEnv *env);
static int in_fill(NoctEnv *env, int timeout_ms);
static int in_peek(size_t offset);
static void in_consume(size_t length);
static int decode_csi(size_t start, size_t *consumed);
static int decode_event(NoctEnv *env, bool esc_timed_out);
static bool cfunc_Term_readKey(NoctEnv *env);
static bool cfunc_Term_pendingInput(NoctEnv *env);

/*
 * Registers the ANSI Term API functions and constants.
 */
NOCT_DLL
bool
noct_register_api_term(
	NoctEnv *env)
{
	NoctValue term_dict;
	NoctValue temporary;

	/* Initializes the values before they become GC roots. */
	memset(&term_dict, 0, sizeof(term_dict));
	memset(&temporary, 0, sizeof(temporary));

	/* Pins the dictionary and constant temporary during registration. */
	if (!noct_pin_local(env, 2, &term_dict, &temporary))
		return false;

	/* Creates the global Term dictionary. */
	if (!noct_make_empty_dict(env, &term_dict)) {
		(void)noct_unpin_local(env, 2, &term_dict, &temporary);
		return false;
	}

	/* Publishes the empty Term dictionary. */
	if (!noct_set_global(env, "Term", &term_dict)) {
		(void)noct_unpin_local(env, 2, &term_dict, &temporary);
		return false;
	}

	/* Registers every native Term function. */
	if (!register_term_functions(env, &term_dict)) {
		(void)noct_unpin_local(env, 2, &term_dict, &temporary);
		return false;
	}

	/* Publishes every Term key constant. */
	if (!register_term_constants(env, &term_dict, &temporary)) {
		(void)noct_unpin_local(env, 2, &term_dict, &temporary);
		return false;
	}

	/* Installs best-effort terminal restoration at process exit. */
	(void)atexit(term_restore);

	/* Releases the registration roots. */
	(void)noct_unpin_local(env, 2, &term_dict, &temporary);

	/* Reports successful Term API registration. */
	return true;
}

/* Registers all native Term functions. */
static bool
register_term_functions(
	NoctEnv *env,
	NoctValue *term_dict)
{
	/* Registers Term.open(). */
	if (!register_term_function(
		env,
		term_dict,
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
		"Term.write",
		"write",
		1,
		term_write_param,
		cfunc_Term_write)) {
		return false;
	}

	/* Registers Term.syncBegin(). */
	if (!register_term_function(
		env,
		term_dict,
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
		"Term.syncEnd",
		"syncEnd",
		0,
		term_no_param,
		cfunc_Term_syncEnd)) {
		return false;
	}

	/* Registers Term.clear(). */
	if (!register_term_function(
		env,
		term_dict,
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
		"Term.flush",
		"flush",
		0,
		term_no_param,
		cfunc_Term_flush)) {
		return false;
	}

	/* Registers Term.readKey(). */
	if (!register_term_function(
		env,
		term_dict,
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
	const char *global_name,
	const char *field_name,
	size_t param_count,
	const char *param[],
	bool (*cfunc)(NoctEnv *env))
{
	NoctValue function;

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
	memset(&function, 0, sizeof(function));
	if (!noct_get_global(env, global_name, &function))
		return false;

	/* Publishes the function in the Term dictionary. */
	if (!noct_set_dict_elem_cstr(
		env,
		term_dict,
		field_name,
		&function)) {
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
	size_t i;

	/* Publishes each constant in table order. */
	for (i = 0; i < sizeof(term_consts) / sizeof(term_consts[0]); i++) {
		/* Creates the next integer dictionary element. */
		if (!noct_set_dict_elem_make_int(
			env,
			term_dict,
			term_consts[i].name,
			temporary,
			term_consts[i].value)) {
			return false;
		}
	}

	/* Reports successful constant publication. */
	return true;
}

/* Records a pending terminal resize signal. */
static void
on_sigwinch(
	int signal_number)
{
	UNUSED_PARAMETER(signal_number);

	/* Defers resize handling to the normal input path. */
	term.resized = 1;
}

/* Restores the terminal through an idempotent path. */
static void
term_restore(
	void)
{
	static const char restore_seq[] =
		"\x1B[?25h"	/* show cursor */
		"\x1B[0m"	/* reset attributes */
		"\x1B[>4;0m"	/* modifyOtherKeys off */
		"\x1B[?1049l";	/* leave the alternate screen */

	/* Leaves an already restored terminal unchanged. */
	if (!term.open)
		return;

	/* Restores the visible terminal modes. */
	(void)write(STDOUT_FILENO, restore_seq, sizeof(restore_seq) - 1);

	/* Restores the saved terminal attributes when available. */
	if (term.saved_valid)
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &term.saved);

	/* Marks the terminal session as closed. */
	term.open = false;
}

/* Appends bytes to the terminal output buffer. */
static bool
out_put(
	NoctEnv *env,
	const char *string,
	size_t length)
{
	size_t required;
	size_t new_alloc;
	char *new_buffer;

	/* Computes the extent required by the appended bytes. */
	required = term.out_len + length;

	/* Expands the output buffer when the bytes do not fit. */
	if (required > term.out_alloc) {
		if (term.out_alloc == 0)
			new_alloc = 8192;
		else
			new_alloc = term.out_alloc;

		/* Doubles the allocation until it can hold the result. */
		while (new_alloc < required)
			new_alloc *= 2;

		/* Reallocates the output storage. */
		new_buffer = noct_realloc(term.out, new_alloc);
		if (new_buffer == NULL) {
			noct_out_of_memory(env);
			return false;
		}

		/* Publishes the expanded output buffer. */
		term.out = new_buffer;
		term.out_alloc = new_alloc;
	}

	/* Appends the requested bytes to the current frame. */
	memcpy(term.out + term.out_len, string, length);
	term.out_len += length;

	/* Reports successful buffering. */
	return true;
}

/* Appends one C string to the terminal output buffer. */
static bool
out_put_cstr(
	NoctEnv *env,
	const char *string)
{
	size_t length;
	bool succeeded;

	/* Measures and appends the complete string. */
	length = strlen(string);
	succeeded = out_put(env, string, length);

	/* Reports whether the string was buffered. */
	return succeeded;
}

/* Returns one integer through the common VM integer path. */
static bool
return_int(
	NoctEnv *env,
	int64_t value)
{
	NoctValue result;
	bool succeeded;

	/* Initializes the result before it becomes a GC root. */
	memset(&result, 0, sizeof(result));

	/* Pins the return value during integer construction. */
	if (!noct_pin_local(env, 1, &result))
		return false;

	/*
	 * Return an int, not a long: conditions and arithmetic stay in
	 * the VM's common int path. Buffer positions beyond 2^31 are out
	 * of scope for v1.
	 */
	succeeded = noct_set_return_make_int(env, &result, (int)value);

	/* Releases the temporary return root. */
	(void)noct_unpin_local(env, 1, &result);

	/* Reports whether the integer was returned. */
	return succeeded;
}

/* Implements Term.isTTY(). */
static bool
cfunc_Term_isTTY(
	NoctEnv *env)
{
	int is_tty;
	bool succeeded;

	/* Checks the input terminal before checking the output terminal. */
	is_tty = 0;
	if (isatty(STDIN_FILENO)) {
		/* Requires both standard streams to be terminals. */
		if (isatty(STDOUT_FILENO))
			is_tty = 1;
	}

	/* Returns the normalized terminal flag. */
	succeeded = return_int(env, is_tty);

	/* Reports whether the flag was returned. */
	return succeeded;
}

/* Implements Term.open(). */
static bool
cfunc_Term_open(
	NoctEnv *env)
{
	struct termios raw;
	struct sigaction sa;
	static const char enter_seq[] =
		"\x1B[?1049h"	/* alternate screen */
		"\x1B[2J"	/* clear */
		"\x1B[H"	/* home */
		"\x1B[>4;2m";	/* modifyOtherKeys=2 */
	int input_is_tty;
	int output_is_tty;
	bool succeeded;

	/* Reports success for an already open terminal. */
	if (term.open) {
		succeeded = return_int(env, 1);
		return succeeded;
	}

	/* Checks both standard terminal streams in short-circuit order. */
	input_is_tty = isatty(STDIN_FILENO);
	if (input_is_tty)
		output_is_tty = isatty(STDOUT_FILENO);
	else
		output_is_tty = 0;

	/* Reports an unavailable interactive terminal. */
	if (!input_is_tty || !output_is_tty) {
		succeeded = return_int(env, 0);
		return succeeded;
	}

	/* Saves the current terminal attributes. */
	if (tcgetattr(STDIN_FILENO, &term.saved) != 0) {
		succeeded = return_int(env, 0);
		return succeeded;
	}
	term.saved_valid = true;

	/* Constructs the raw input attributes. */
	raw = term.saved;
	raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= (tcflag_t)~OPOST;
	raw.c_cflag |= CS8;
	raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;

	/* Activates the raw terminal attributes. */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
		succeeded = return_int(env, 0);
		return succeeded;
	}

	/* Installs the deferred resize handler. */
	memset(&sa, 0, sizeof(sa));
#if defined(NOCT_TARGET_ZEDBSD)
	sa.sa_handler = (uint64_t)(uintptr_t)on_sigwinch;
#else
	sa.sa_handler = on_sigwinch;
#endif
	(void)sigaction(SIGWINCH, &sa, NULL);

	/* Enters the alternate screen and extended key mode. */
	(void)write(STDOUT_FILENO, enter_seq, sizeof(enter_seq) - 1);

	/* Initializes the live terminal session state. */
	term.open = true;
	term.in_head = 0;
	term.in_len = 0;
	term.out_len = 0;
	term.eof = false;

	/* Returns the successful open status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.close(). */
static bool
cfunc_Term_close(
	NoctEnv *env)
{
	bool succeeded;

	/* Restores the terminal session. */
	term_restore();

	/* Returns the successful close status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.size(). */
static bool
cfunc_Term_size(
	NoctEnv *env)
{
	NoctValue result;
	NoctValue temporary;
	struct winsize ws;
	int rows;
	int columns;
	int ioctl_status;

	/* Initializes the portable fallback dimensions. */
	rows = 24;
	columns = 80;

	/* Reads the current terminal window dimensions. */
	ioctl_status = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

	/* Replaces the fallback when the terminal reports valid rows. */
	if (ioctl_status == 0 && ws.ws_row > 0) {
		rows = ws.ws_row;
		columns = ws.ws_col;
	}

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
		rows)) {
		(void)noct_unpin_local(env, 2, &result, &temporary);
		return false;
	}

	/* Publishes the column count. */
	if (!noct_set_dict_elem_make_int(
		env,
		&result,
		"cols",
		&temporary,
		columns)) {
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
	int resized;
	bool succeeded;

	/* Captures and clears the pending resize flag. */
	resized = term.resized ? 1 : 0;
	term.resized = 0;

	/* Returns the captured resize flag. */
	succeeded = return_int(env, resized);

	/* Reports whether the flag was returned. */
	return succeeded;
}

/* Reads one integer argument while accepting int and long values. */
static bool
get_int_arg(
	NoctEnv *env,
	uint32_t index,
	int64_t *result)
{
	NoctValue value;
	int64_t long_value;
	int int_value;
	bool is_long;
	bool is_int;

	/* Initializes the argument before it becomes a GC root. */
	memset(&value, 0, sizeof(value));

	/* Pins the argument during type conversion. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Tries the long representation first. */
	is_long = noct_get_arg_check_long(env, index, &value, &long_value);
	if (is_long) {
		*result = long_value;
	} else {
		/* Falls back to the common integer representation. */
		is_int = noct_get_arg_check_int(env, index, &value, &int_value);
		if (!is_int) {
			(void)noct_unpin_local(env, 1, &value);
			return false;
		}
		*result = int_value;
	}

	/* Releases the converted argument root. */
	(void)noct_unpin_local(env, 1, &value);

	/* Reports a converted integer argument. */
	return true;
}

/* Implements Term.moveTo(). */
static bool
cfunc_Term_moveTo(
	NoctEnv *env)
{
	int64_t row;
	int64_t column;
	char sequence[32];
	bool succeeded;

	/* Reads the target row. */
	if (!get_int_arg(env, 0, &row))
		return false;

	/* Reads the target column. */
	if (!get_int_arg(env, 1, &column))
		return false;

	/* Formats the ANSI cursor-position sequence. */
	snprintf(
		sequence,
		sizeof(sequence),
		"\x1B[%d;%dH",
		(int)row,
		(int)column);

	/* Buffers the cursor-position sequence. */
	if (!out_put_cstr(env, sequence))
		return false;

	/* Returns the successful movement status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/*
 * Term.syncBegin() / Term.syncEnd()
 *
 * Bracket a frame with DEC private mode 2026 (synchronized output):
 * capable terminals hold rendering until the frame completes, which
 * removes flicker; others ignore the sequence.
 */
/* Implements Term.syncBegin(). */
static bool
cfunc_Term_syncBegin(
	NoctEnv *env)
{
	bool succeeded;

	/* Buffers the synchronized-output begin sequence. */
	if (!out_put_cstr(env, "\033[?2026h"))
		return false;

	/* Returns the successful synchronization status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.syncEnd(). */
static bool
cfunc_Term_syncEnd(
	NoctEnv *env)
{
	bool succeeded;

	/* Buffers the synchronized-output end sequence. */
	if (!out_put_cstr(env, "\033[?2026l"))
		return false;

	/* Returns the successful synchronization status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.write(). */
static bool
cfunc_Term_write(
	NoctEnv *env)
{
	NoctValue text;
	const char *text_string;
	bool succeeded;

	/* Initializes the text before it becomes a GC root. */
	memset(&text, 0, sizeof(text));

	/* Pins the text while its bytes are copied. */
	if (!noct_pin_local(env, 1, &text))
		return false;

	/* Reads the string to write. */
	if (!noct_get_arg_check_string(env, 0, &text, &text_string)) {
		(void)noct_unpin_local(env, 1, &text);
		return false;
	}

	/* Copies the text into the frame buffer. */
	if (!out_put_cstr(env, text_string)) {
		(void)noct_unpin_local(env, 1, &text);
		return false;
	}

	/* Releases the copied text root. */
	(void)noct_unpin_local(env, 1, &text);

	/* Returns the successful write status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.clear(). */
static bool
cfunc_Term_clear(
	NoctEnv *env)
{
	bool succeeded;

	/* Buffers the clear-screen and home sequence. */
	if (!out_put_cstr(env, "\x1B[2J\x1B[H"))
		return false;

	/* Returns the successful clear status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.clearToEol(). */
static bool
cfunc_Term_clearToEol(
	NoctEnv *env)
{
	bool succeeded;

	/* Buffers the clear-to-end-of-line sequence. */
	if (!out_put_cstr(env, "\x1B[K"))
		return false;

	/* Returns the successful clear status. */
	succeeded = return_int(env, 1);

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
	char sequence[64];
	int foreground;
	int background;
	int bold;
	int reverse;
	int underline;
	bool checked;
	bool has_value;
	bool succeeded;

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

	/* Initializes the optional style fields. */
	foreground = -1;
	background = -1;
	bold = 0;
	reverse = 0;
	underline = 0;

	/* Reads the optional foreground color. */
	checked = noct_check_dict_key_cstr(env, &style, "fg", &has_value);
	if (checked && has_value) {
		(void)noct_get_dict_elem_check_int(
			env,
			&style,
			"fg",
			&temporary,
			&foreground);
	}

	/* Reads the optional background color. */
	checked = noct_check_dict_key_cstr(env, &style, "bg", &has_value);
	if (checked && has_value) {
		(void)noct_get_dict_elem_check_int(
			env,
			&style,
			"bg",
			&temporary,
			&background);
	}

	/* Reads the optional bold flag. */
	checked = noct_check_dict_key_cstr(env, &style, "bold", &has_value);
	if (checked && has_value) {
		(void)noct_get_dict_elem_check_int(
			env,
			&style,
			"bold",
			&temporary,
			&bold);
	}

	/* Reads the optional reverse-video flag. */
	checked = noct_check_dict_key_cstr(env, &style, "reverse", &has_value);
	if (checked && has_value) {
		(void)noct_get_dict_elem_check_int(
			env,
			&style,
			"reverse",
			&temporary,
			&reverse);
	}

	/* Reads the optional underline flag. */
	checked = noct_check_dict_key_cstr(env, &style, "underline", &has_value);
	if (checked && has_value) {
		(void)noct_get_dict_elem_check_int(
			env,
			&style,
			"underline",
			&temporary,
			&underline);
	}

	/* Resets the current terminal style. */
	if (!out_put_cstr(env, "\x1B[0m"))
		succeeded = false;
	else
		succeeded = true;

	/* Applies the requested bold style. */
	if (succeeded && bold) {
		if (!out_put_cstr(env, "\x1B[1m"))
			succeeded = false;
	}

	/* Applies the requested underline style. */
	if (succeeded && underline) {
		if (!out_put_cstr(env, "\x1B[4m"))
			succeeded = false;
	}

	/* Applies the requested reverse-video style. */
	if (succeeded && reverse) {
		if (!out_put_cstr(env, "\x1B[7m"))
			succeeded = false;
	}

	/* Applies the requested foreground color. */
	if (succeeded && foreground >= 0) {
		snprintf(
			sequence,
			sizeof(sequence),
			"\x1B[38;5;%dm",
			foreground);
		if (!out_put_cstr(env, sequence))
			succeeded = false;
	}

	/* Applies the requested background color. */
	if (succeeded && background >= 0) {
		snprintf(
			sequence,
			sizeof(sequence),
			"\x1B[48;5;%dm",
			background);
		if (!out_put_cstr(env, sequence))
			succeeded = false;
	}

	/* Releases the style dictionary roots. */
	(void)noct_unpin_local(env, 2, &style, &temporary);

	/* Propagates a failed style sequence. */
	if (!succeeded)
		return false;

	/* Returns the successful style status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.showCursor(). */
static bool
cfunc_Term_showCursor(
	NoctEnv *env)
{
	const char *sequence;
	int64_t visible;
	bool succeeded;

	/* Reads the requested visibility flag. */
	if (!get_int_arg(env, 0, &visible))
		return false;

	/* Selects the matching ANSI cursor sequence. */
	if (visible)
		sequence = "\x1B[?25h";
	else
		sequence = "\x1B[?25l";

	/* Buffers the selected cursor sequence. */
	if (!out_put_cstr(env, sequence))
		return false;

	/* Returns the successful visibility status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Implements Term.flush(). */
static bool
cfunc_Term_flush(
	NoctEnv *env)
{
	size_t offset;
	ssize_t write_size;
	bool succeeded;

	/* Writes every buffered byte unless the terminal rejects it. */
	offset = 0;
	while (offset < term.out_len) {
		write_size = write(
			STDOUT_FILENO,
			term.out + offset,
			term.out_len - offset);

		/* Retries an interrupted write and stops on other errors. */
		if (write_size < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		/* Advances past the bytes accepted by the terminal. */
		offset += (size_t)write_size;
	}

	/* Starts the next frame with an empty output buffer. */
	term.out_len = 0;

	/* Returns the existing successful flush status. */
	succeeded = return_int(env, 1);

	/* Reports whether the status was returned. */
	return succeeded;
}

/* Pulls more bytes into the input ring in a blocking region. */
static int
in_fill(
	NoctEnv *env,
	int timeout_ms)
{
	struct pollfd descriptor;
	unsigned char buffer[4096];
	ssize_t read_size;
	size_t i;
	int poll_result;
	int wait_timeout;

	/* Reuses bytes already available in the input ring. */
	if (term.in_len > 0)
		return 1;

	/*
	 * Preserves caller timing after EOF because polling a dead terminal
	 * would otherwise return immediately and cause a busy loop.
	 */
	if (term.eof) {
		if (timeout_ms < 0)
			wait_timeout = 1000;
		else
			wait_timeout = timeout_ms;

		/* Waits without a descriptor in the VM blocking region. */
		noct_enter_blocking(env);
		(void)poll(NULL, 0, wait_timeout);
		noct_leave_blocking(env);

		/* Reports that EOF produced no input. */
		return 0;
	}

	/* Prepares the standard-input poll descriptor. */
	descriptor.fd = STDIN_FILENO;
	descriptor.events = POLLIN;

	/* Waits until input, resize, timeout, or a permanent error. */
	noct_enter_blocking(env);
	for (;;) {
		poll_result = poll(&descriptor, 1, timeout_ms);

		/* Stops after input or a normal timeout. */
		if (poll_result >= 0)
			break;

		/* Retries interrupted waits unless a resize caused the signal. */
		if (errno == EINTR) {
			if (term.resized) {
				poll_result = 0;
				break;
			}
			continue;
		}

		/* Stops after a permanent polling error. */
		break;
	}
	noct_leave_blocking(env);

	/* Reports a timeout, resize, or polling failure as no input. */
	if (poll_result <= 0)
		return 0;

	/* Reads the available terminal bytes. */
	read_size = read(STDIN_FILENO, buffer, sizeof(buffer));

	/* Records a terminal that reached EOF. */
	if (read_size == 0) {
		term.eof = true;
		return 0;
	}

	/* Reports a failed read as no input. */
	if (read_size < 0)
		return 0;

	/* Copies as many read bytes as fit in the input ring. */
	for (i = 0;
	     i < (size_t)read_size && term.in_len < sizeof(term.in);
	     i++) {
		term.in[(term.in_head + term.in_len) % sizeof(term.in)] =
			buffer[i];
		term.in_len++;
	}

	/* Reports newly available input. */
	return 1;
}

/* Peeks one byte from the input ring. */
static int
in_peek(
	size_t offset)
{
	/* Reports an unavailable input position. */
	if (offset >= term.in_len)
		return -1;

	/* Returns the requested ring byte without consuming it. */
	return term.in[(term.in_head + offset) % sizeof(term.in)];
}

/* Consumes bytes from the front of the input ring. */
static void
in_consume(
	size_t length)
{
	/* Verifies that the requested bytes are available. */
	assert(length <= term.in_len);

	/* Advances and shortens the ring contents. */
	term.in_head = (term.in_head + length) % sizeof(term.in);
	term.in_len -= length;
}

/* Decodes one CSI sequence already present in the input ring. */
static int
decode_csi(
	size_t start,
	size_t *consumed)
{
	int params[4];
	size_t index;
	int parameter_count;
	int final_byte;
	int accumulator;
	int has_digit;
	int modifier_bits;
	int base;
	int modifier;
	int code;
	int modified_modifier;
	int modified_bits;

	/* Initializes the parameter parser at the CSI payload. */
	parameter_count = 0;
	accumulator = 0;
	has_digit = 0;
	index = start;

	/* Parses numeric parameters until the final CSI byte. */
	for (;;) {
		final_byte = in_peek(index);

		/* Requests more input for an incomplete sequence. */
		if (final_byte < 0)
			return -1;

		/* Accumulates one decimal parameter digit. */
		if (final_byte >= '0' && final_byte <= '9') {
			accumulator = accumulator * 10 + (final_byte - '0');
			has_digit = 1;
			index++;
			continue;
		}

		/* Commits a parameter at each separator. */
		if (final_byte == ';') {
			if (parameter_count < 4) {
				params[parameter_count] =
					has_digit ? accumulator : 0;
				parameter_count++;
			}
			accumulator = 0;
			has_digit = 0;
			index++;
			continue;
		}

		/* Leaves the parser positioned on the final byte. */
		break;
	}

	/* Commits the final numeric parameter when storage remains. */
	if (parameter_count < 4) {
		params[parameter_count] = has_digit ? accumulator : 0;
		parameter_count++;
	}

	/* Reports the bytes consumed through the final CSI byte. */
	*consumed = index + 1 - start;

	/* Decodes the xterm modifier parameter into event bits. */
	modifier_bits = 0;
	modifier = 0;
	if (parameter_count >= 2)
		modifier = params[1] > 0 ? params[1] - 1 : 0;

	/* Adds the requested shift modifier. */
	if (modifier & 1)
		modifier_bits |= MOD_SHIFT;

	/* Adds the requested meta modifier. */
	if (modifier & 2)
		modifier_bits |= MOD_META;

	/* Adds the requested control modifier. */
	if (modifier & 4)
		modifier_bits |= MOD_CTRL;

	/* Decodes the final CSI byte. */
	base = -1;
	switch (final_byte) {
	case 'A':
		base = KEY_UP;
		break;
	case 'B':
		base = KEY_DOWN;
		break;
	case 'C':
		base = KEY_RIGHT;
		break;
	case 'D':
		base = KEY_LEFT;
		break;
	case 'H':
		base = KEY_HOME;
		break;
	case 'F':
		base = KEY_END;
		break;
	case '~':
		/* Decodes a numeric xterm key identifier. */
		switch (params[0]) {
		case 1:
			base = KEY_HOME;
			break;
		case 2:
			base = KEY_INSERT;
			break;
		case 3:
			base = KEY_DELETE;
			break;
		case 4:
			base = KEY_END;
			break;
		case 5:
			base = KEY_PGUP;
			break;
		case 6:
			base = KEY_PGDN;
			break;
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			base = KEY_F1 + params[0] - 11;
			break;
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
			base = KEY_F1 + params[0] - 12;
			break;
		case 23:
		case 24:
			base = KEY_F1 + params[0] - 13;
			break;
		case 27:
			/* Decodes a modifyOtherKeys character event. */
			if (parameter_count >= 3) {
				code = params[2];
				modified_modifier =
					params[1] > 0 ? params[1] - 1 : 0;
				modified_bits = 0;

				/* Adds the modified character's shift bit. */
				if (modified_modifier & 1)
					modified_bits |= MOD_SHIFT;

				/* Adds the modified character's meta bit. */
				if (modified_modifier & 2)
					modified_bits |= MOD_META;

				/* Adds the modified character's control bit. */
				if (modified_modifier & 4)
					modified_bits |= MOD_CTRL;

				/* Returns the modified character event. */
				return modified_bits | code;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	/* Swallows an unknown CSI sequence. */
	if (base < 0)
		return 0;

	/* Returns the decoded special-key event. */
	return modifier_bits | base;
}

/* Decodes one key event from the input ring. */
static int
decode_event(
	NoctEnv *env,
	bool esc_timed_out)
{
	size_t consumed;
	size_t length;
	size_t i;
	uint32_t codepoint;
	int first_byte;
	int second_byte;
	int third_byte;
	int continuation_byte;
	int event;

	/* Reads the first byte without consuming it. */
	first_byte = in_peek(0);

	/* Requests more input when the ring is empty. */
	if (first_byte < 0)
		return -1;

	/* Decodes a plain control character. */
	if (first_byte != 0x1B && first_byte < 0x20) {
		in_consume(1);

		/* Preserves the traditional tab and return identities. */
		if (first_byte == '\t' || first_byte == '\r')
			return first_byte;

		/* Represents line feed as Emacs control-j. */
		if (first_byte == '\n')
			return MOD_CTRL | 0x6A;

		/* Represents NUL as control-space. */
		if (first_byte == 0x00)
			return MOD_CTRL | 0x20;

		/* Maps control-a through control-z to lowercase key events. */
		if (first_byte <= 0x1A)
			return MOD_CTRL | (first_byte + 0x60);

		/* Maps the remaining control bytes to punctuation events. */
		return MOD_CTRL | (first_byte + 0x40);
	}

	/* Decodes the conventional backspace byte. */
	if (first_byte == 0x7F) {
		in_consume(1);
		return 0x7F;
	}

	/* Decodes one printable ASCII byte. */
	if (first_byte >= 0x20 &&
	    first_byte < 0x7F &&
	    first_byte != 0x1B) {
		in_consume(1);
		return first_byte;
	}

	/* Decodes one UTF-8 sequence beginning with a lead byte. */
	if (first_byte >= 0xC0) {
		/* Selects the encoded sequence length. */
		if ((first_byte & 0xE0) == 0xC0)
			length = 2;
		else if ((first_byte & 0xF0) == 0xE0)
			length = 3;
		else
			length = 4;

		/* Requests more bytes for an incomplete UTF-8 sequence. */
		if (term.in_len < length)
			return -1;

		/* Initializes the codepoint from the lead byte payload. */
		codepoint =
			(uint32_t)(first_byte & (0xFF >> (length + 1)));

		/* Incorporates every UTF-8 continuation byte. */
		for (i = 1; i < length; i++) {
			continuation_byte = in_peek(i);

			/* Swallows a broken lead byte for forward progress. */
			if ((continuation_byte & 0xC0) != 0x80) {
				in_consume(1);
				return 0;
			}

			/* Appends the next six codepoint bits. */
			codepoint = (codepoint << 6) |
				(uint32_t)(continuation_byte & 0x3F);
		}

		/* Consumes and returns the completed UTF-8 sequence. */
		in_consume(length);
		return (int)codepoint;
	}

	/* Reads the byte following an escape prefix. */
	second_byte = in_peek(1);

	/* Resolves an escape byte that is still alone. */
	if (second_byte < 0) {
		/* Returns a bare escape after the disambiguation timeout. */
		if (esc_timed_out) {
			in_consume(1);
			return 0x1B;
		}

		/* Requests another byte for a possible escape sequence. */
		return -1;
	}

	/* Decodes a control-sequence-introducer escape. */
	if (second_byte == '[') {
		event = decode_csi(2, &consumed);

		/* Requests more bytes for an incomplete CSI sequence. */
		if (event < 0)
			return -1;

		/* Consumes and returns the complete CSI event. */
		in_consume(2 + consumed);
		return event;
	}

	/* Decodes an SS3 special-key escape. */
	if (second_byte == 'O') {
		third_byte = in_peek(2);

		/* Requests the missing SS3 key byte. */
		if (third_byte < 0)
			return -1;

		/* Consumes the complete SS3 sequence. */
		in_consume(3);

		/* Maps the SS3 suffix to a special-key event. */
		switch (third_byte) {
		case 'A':
			event = KEY_UP;
			break;
		case 'B':
			event = KEY_DOWN;
			break;
		case 'C':
			event = KEY_RIGHT;
			break;
		case 'D':
			event = KEY_LEFT;
			break;
		case 'H':
			event = KEY_HOME;
			break;
		case 'F':
			event = KEY_END;
			break;
		case 'P':
			event = KEY_F1;
			break;
		case 'Q':
			event = KEY_F1 + 1;
			break;
		case 'R':
			event = KEY_F1 + 2;
			break;
		case 'S':
			event = KEY_F1 + 3;
			break;
		default:
			event = 0;
			break;
		}

		/* Returns the decoded SS3 event. */
		return event;
	}

	/* Decodes the byte after a meta escape prefix. */
	in_consume(1);
	event = decode_event(env, true);

	/* Falls back to a bare escape for an undecodable suffix. */
	if (event <= 0)
		return 0x1B;

	/* Returns the decoded event with its meta modifier. */
	return MOD_META | event;
}

/* Implements Term.readKey(). */
static bool
cfunc_Term_readKey(
	NoctEnv *env)
{
	int64_t timeout_ms;
	int event;
	int wait_timeout;
	int first_byte;
	int fill_result;
	bool escape_wait;
	bool succeeded;

	/* Reads the caller's input timeout. */
	if (!get_int_arg(env, 0, &timeout_ms))
		return false;

	/* Decodes input until one reportable event is available. */
	escape_wait = false;
	for (;;) {
		event = decode_event(env, escape_wait);

		/* Returns a complete, nonzero event. */
		if (event >= 0 && event != 0) {
			succeeded = return_int(env, event);
			return succeeded;
		}

		/* Continues after swallowing an unknown input sequence. */
		if (event == 0)
			continue;

		/* Selects the normal caller timeout by default. */
		wait_timeout = (int)timeout_ms;

		/* Uses a short timeout to disambiguate a pending escape byte. */
		if (term.in_len > 0) {
			first_byte = in_peek(0);
			if (first_byte == 0x1B)
				wait_timeout = ESC_TIMEOUT_MS;
		}

		/* Pulls more bytes into the input ring. */
		fill_result = in_fill(env, wait_timeout);

		/* Resolves timeout and EOF when no new input arrived. */
		if (!fill_result) {
			/* Marks a pending escape as timed out. */
			if (term.in_len > 0) {
				first_byte = in_peek(0);
				if (first_byte == 0x1B) {
					escape_wait = true;
					continue;
				}
			}

			/* Reports a terminal that is permanently gone. */
			if (term.eof && term.in_len == 0) {
				succeeded = return_int(env, -2);
				return succeeded;
			}

			/* Reports an ordinary input timeout. */
			succeeded = return_int(env, -1);
			return succeeded;
		}
	}
}

/* Implements Term.pendingInput(). */
static bool
cfunc_Term_pendingInput(
	NoctEnv *env)
{
	struct pollfd descriptor;
	int poll_result;
	int pending;
	bool succeeded;

	/* Reports bytes already buffered in the input ring. */
	if (term.in_len > 0) {
		succeeded = return_int(env, 1);
		return succeeded;
	}

	/* Polls standard input without waiting. */
	descriptor.fd = STDIN_FILENO;
	descriptor.events = POLLIN;
	poll_result = poll(&descriptor, 1, 0);

	/* Normalizes the polling result to an integer flag. */
	if (poll_result > 0)
		pending = 1;
	else
		pending = 0;

	/* Returns the pending-input flag. */
	succeeded = return_int(env, pending);

	/* Reports whether the flag was returned. */
	return succeeded;
}

#endif /* NOCT_TARGET_POSIX */
