/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include <noct/repl.h>

#include <stdio.h>
#include <stdlib.h>

static void expect_result(NoctReplSession *session, const char *line,
			  enum NoctReplResult expected, const char *label)
{
	enum NoctReplResult actual;

	actual = noct_repl_submit(session, line);
	if (actual != expected) {
		fprintf(stderr, "%s: result %d, expected %d\n",
			label, (int)actual, (int)expected);
		exit(1);
	}
}

int main(void)
{
	NoctVM *vm;
	NoctEnv *env;
	NoctReplSession *session;
	NoctReplSession *small;

	vm = NULL;
	env = NULL;
	if (!noct_create_vm(&vm, &env, NULL)) {
		fprintf(stderr, "cannot create VM\n");
		return 1;
	}
	session = noct_repl_create(env, 4096);
	if (session == NULL) {
		fprintf(stderr, "cannot create REPL session\n");
		noct_destroy_vm(vm);
		return 1;
	}

	expect_result(session, "\n", NOCT_REPL_READY, "blank");
	expect_result(session, "// { ignored }\n", NOCT_REPL_READY,
		      "line comment");
	expect_result(session, "var text = \"{ ignored }\"\n",
		      NOCT_REPL_EXECUTED, "string braces");
	expect_result(session, "if (1) {", NOCT_REPL_NEED_MORE,
		      "line without newline begin");
	expect_result(session, "1 + 1;", NOCT_REPL_NEED_MORE,
		      "line without newline body");
	expect_result(session, "}", NOCT_REPL_EXECUTED,
		      "line without newline end");

	expect_result(session, "if (1) {\n", NOCT_REPL_NEED_MORE,
		      "if begin");
	expect_result(session, "// } ignored\n", NOCT_REPL_NEED_MORE,
		      "if comment");
	expect_result(session, "var value = 1;\n", NOCT_REPL_NEED_MORE,
		      "if body");
	expect_result(session, "}\n", NOCT_REPL_EXECUTED, "if end");

	expect_result(session, "func add(a, b)\n", NOCT_REPL_NEED_MORE,
		      "function before brace");
	expect_result(session, "{\n", NOCT_REPL_NEED_MORE,
		      "function open");
	expect_result(session, "return a + b;\n", NOCT_REPL_NEED_MORE,
		      "function body");
	expect_result(session, "}\n", NOCT_REPL_EXECUTED,
		      "function definition");
	expect_result(session, "add(2, 3)\n", NOCT_REPL_EXECUTED,
		      "function call");
	expect_result(session, "func repl() { return 7; }\n",
		      NOCT_REPL_EXECUTED, "user repl function definition");
	expect_result(session, "1 + 1\n", NOCT_REPL_EXECUTED,
		      "statement does not replace user repl function");
	expect_result(session, "repl()\n", NOCT_REPL_EXECUTED,
		      "user repl function survives statements");

	expect_result(session, "var values = [\n", NOCT_REPL_NEED_MORE,
		      "array begin");
	expect_result(session, "1, 2\n", NOCT_REPL_NEED_MORE,
		      "array body");
	expect_result(session, "]\n", NOCT_REPL_EXECUTED, "array end");

	expect_result(session, "([)]\n", NOCT_REPL_ERROR,
		      "mismatched delimiters");
	expect_result(session, "var = 1\n", NOCT_REPL_ERROR,
		      "syntax error");
	expect_result(session, "1 + 1\n", NOCT_REPL_EXECUTED,
		      "recovery after error");

	expect_result(session, "while (1)\n", NOCT_REPL_NEED_MORE,
		      "cancel begin");
	if (noct_repl_cancel(session) != NOCT_REPL_READY) {
		fprintf(stderr, "cancel did not return READY\n");
		return 1;
	}
	expect_result(session, "2 + 2\n", NOCT_REPL_EXECUTED,
		      "recovery after cancel");

	small = noct_repl_create(env, 12);
	if (small == NULL) {
		fprintf(stderr, "cannot create small REPL session\n");
		return 1;
	}
	expect_result(small, "this input is too large\n", NOCT_REPL_ERROR,
		      "bounded input");
	noct_repl_destroy(small);

	expect_result(session, NULL, NOCT_REPL_EXIT, "orderly exit");
	noct_repl_destroy(session);
	if (!noct_destroy_vm(vm)) {
		fprintf(stderr, "cannot destroy VM\n");
		return 1;
	}

	puts("Noct reusable REPL session tests: PASS");
	return 0;
}
