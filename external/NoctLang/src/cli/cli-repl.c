/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: REPL Mode
 */

#include <noct/noct.h>
#include "cli-main.h"
#include "repl.h"

#include <errno.h>
#include <signal.h>

#define REPL_SOURCE_SIZE	32768
#define REPL_LINE_SIZE		(REPL_SOURCE_SIZE + 2)

enum repl_read_result {
	REPL_READ_LINE,
	REPL_READ_EOF,
	REPL_READ_INTERRUPT,
	REPL_READ_TOO_LONG,
};

static volatile sig_atomic_t repl_interrupted;

#if defined(NOCT_TARGET_POSIX)
static struct sigaction repl_old_action;
#else
static void (*repl_old_handler)(int);
#endif
static bool repl_handler_installed;

static bool run_repl(void);
static bool register_repl_libraries(NoctEnv *env);
static void print_repl_error(NoctEnv *env);
static enum repl_read_result read_repl_line(char *line, size_t size);
static bool install_repl_interrupt_handler(void);
static void restore_repl_interrupt_handler(void);
static void repl_interrupt_handler(int signal_number);

int
command_repl(void)
{
	return run_repl() ? 0 : 1;
}

static bool
run_repl(void)
{
	NoctVM *vm;
	NoctEnv *env;
	NoctReplSession *session;
	bool continuation;
	bool success;

	vm = NULL;
	env = NULL;
	session = NULL;
	continuation = false;
	success = false;

	wide_printf(N_TR("Noct Programming Language\n"));
	wide_printf(N_TR("Entering REPL mode.\n"));
#if defined(NOCT_USE_JIT) &&                                                   \
    (defined(NOCT_ARCH_X86) || defined(NOCT_ARCH_X86_64) ||                    \
     defined(NOCT_ARCH_ARM32) || defined(NOCT_ARCH_ARM64) ||                   \
     defined(NOCT_ARCH_MIPS32) || defined(NOCT_ARCH_MIPS64) ||                 \
     defined(NOCT_ARCH_PPC32) || defined(NOCT_ARCH_PPC64) ||                   \
     defined(NOCT_ARCH_RISCV32) || defined(NOCT_ARCH_RISCV64))
	wide_printf(
	    N_TR("JIT compilation is enabled. Starting the fast VM...\n"));
#endif
	wide_printf("\n");

	if (!noct_create_vm(&vm, &env, NULL)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!register_repl_libraries(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}

	session = noct_repl_create(env, REPL_SOURCE_SIZE);
	if (session == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!install_repl_interrupt_handler()) {
		wide_printf(
		    N_TR("Cannot install the REPL interrupt handler.\n"));
		goto cleanup;
	}

	for (;;) {
		char line[REPL_LINE_SIZE];
		enum repl_read_result read_result;
		enum NoctReplResult result;

		if (repl_interrupted) {
			wide_printf("\n");
			noct_repl_submit(session, NULL);
			break;
		}

		wide_printf(continuation ? ". " : "> ");
		fflush(stdout);
		read_result = read_repl_line(line, sizeof(line));
		if (read_result == REPL_READ_INTERRUPT) {
			wide_printf("\n");
			noct_repl_submit(session, NULL);
			break;
		}
		if (read_result == REPL_READ_EOF) {
			noct_repl_submit(session, NULL);
			break;
		}
		if (read_result == REPL_READ_TOO_LONG) {
			wide_printf(N_TR("Input line is too long.\n"));
			noct_repl_cancel(session);
			continuation = false;
			continue;
		}

		result = noct_repl_submit(session, line);
		switch (result) {
		case NOCT_REPL_READY:
		case NOCT_REPL_EXECUTED:
			continuation = false;
			break;
		case NOCT_REPL_NEED_MORE:
			continuation = true;
			break;
		case NOCT_REPL_ERROR:
			print_repl_error(env);
			continuation = false;
			break;
		case NOCT_REPL_EXIT:
			goto done;
		default:
			wide_printf(N_TR("Invalid REPL state.\n"));
			goto cleanup;
		}
	}

done:
	success = true;

cleanup:
	restore_repl_interrupt_handler();
	noct_repl_destroy(session);
	if (vm != NULL && !noct_destroy_vm(vm))
		success = false;
	return success;
}

static bool
register_repl_libraries(NoctEnv *env)
{
	if (!noct_register_api_system(env))
		return false;
	if (!noct_register_api_console(env))
		return false;
	if (!noct_register_api_file(env))
		return false;
	if (!noct_register_api_regex(env))
		return false;
#if defined(NOCT_USE_MULTITHREAD)
	if (!noct_register_api_thread(env))
		return false;
#endif
#if defined(NOCT_USE_HTTPSERVER)
	if (!noct_register_api_httpserver(env))
		return false;
#endif
#if defined(NOCT_USE_TERM)
	if (!noct_register_api_term(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
#endif
#if defined(NOCT_USE_BEUI)
	if (!noct_register_api_beui(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
#endif
	return register_cli_cfunc(env);
}

static void
print_repl_error(NoctEnv *env)
{
	const char *file;
	const char *message;
	int line;

	file = NULL;
	message = NULL;
	line = 0;
	noct_get_error_file(env, &file);
	noct_get_error_line(env, &line);
	noct_get_error_message(env, &message);
	if (message == NULL || message[0] == '\0')
		message = N_TR("Unknown error.");
	if (file != NULL && file[0] != '\0')
		wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, message);
	else
		wide_printf(N_TR("Error: %s\n"), message);
}

static enum repl_read_result
read_repl_line(char *line, size_t size)
{
	int c;

	errno = 0;
	if (fgets(line, (int)size, stdin) != NULL) {
		if (strchr(line, '\n') != NULL || feof(stdin))
			return REPL_READ_LINE;

		/* Discard the rest rather than executing a partial source line.
		 */
		do {
			c = fgetc(stdin);
		} while (c != '\n' && c != EOF && !repl_interrupted);
		if (repl_interrupted || errno == EINTR) {
			clearerr(stdin);
			return REPL_READ_INTERRUPT;
		}
		return REPL_READ_TOO_LONG;
	}
	if (repl_interrupted || errno == EINTR) {
		clearerr(stdin);
		return REPL_READ_INTERRUPT;
	}
	return REPL_READ_EOF;
}

static bool
install_repl_interrupt_handler(void)
{
	repl_interrupted = 0;
#if defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
	{
		struct sigaction action;

		memset(&action, 0, sizeof(action));
#if defined(NOCT_TARGET_ZEDBSD)
		action.sa_handler = (uint64_t)(uintptr_t)repl_interrupt_handler;
#else
		action.sa_handler = repl_interrupt_handler;
#endif
		sigemptyset(&action.sa_mask);
		if (sigaction(SIGINT, &action, &repl_old_action) != 0)
			return false;
	}
#else
	repl_old_handler = signal(SIGINT, repl_interrupt_handler);
	if (repl_old_handler == SIG_ERR)
		return false;
#endif
	repl_handler_installed = true;
	return true;
}

static void
restore_repl_interrupt_handler(void)
{
	if (!repl_handler_installed)
		return;
#if defined(NOCT_TARGET_POSIX) || defined(NOCT_TARGET_MACOS)
	sigaction(SIGINT, &repl_old_action, NULL);
#else
	signal(SIGINT, repl_old_handler);
#endif
	repl_handler_installed = false;
	repl_interrupted = 0;
}

static void
repl_interrupt_handler(int signal_number)
{
	(void)signal_number;
	repl_interrupted = 1;
}
