/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Regex API and its regular expression engine.
 *
 * The dialect is the commonly used POSIX/Java subset:
 *
 *   literals  a ...           (any non-special character)
 *   .                         (any character except newline)
 *   [abc] [a-z] [^a-z]        (character classes; ] literal if first)
 *   \d \D \w \W \s \S         (predefined classes, ASCII + '_' for \w)
 *   \b \B                     (word boundary / non-boundary)
 *   ^ $                       (line anchors; multiline behavior)
 *   ( ... )  (?: ... )        (capturing / non-capturing groups)
 *   a|b                       (alternation)
 *   * + ? {m} {m,} {m,n}      (greedy repetition)
 *   *? +? ?? {m,n}?           (lazy repetition)
 *   \n \t \r \f \\ \. etc.    (escapes; unknown escapes are literal)
 *
 * Matching is leftmost, backtracking, with capture groups 1-9.
 * Positions are in characters (Unicode codepoints), consistent with
 * the String.* APIs. Runaway patterns are stopped by a step
 * budget and a recursion depth cap.
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compiled program limits. */
#define RX_MAX_INSTS	8192
#define RX_MAX_CLASSES	64
#define RX_MAX_RANGES	48
#define RX_MAX_NODES	4096
#define RX_MAX_REPEAT	100
#define RX_STEP_BUDGET	2000000
#define RX_DEPTH_MAX	8000

/* Regular-expression instruction types. */
enum rx_instruction_type {
	RX_I_CHAR,
	RX_I_ANY,
	RX_I_CLASS,
	RX_I_BOL,
	RX_I_EOL,
	RX_I_EOS,
	RX_I_WB,
	RX_I_NWB,
	RX_I_SPLIT,
	RX_I_JMP,
	RX_I_SAVE,
	RX_I_MATCH
};

/* Parse-tree node types. */
enum rx_node_type {
	RX_N_EMPTY,
	RX_N_CHAR,
	RX_N_ANY,
	RX_N_CLASS,
	RX_N_CAT,
	RX_N_ALT,
	RX_N_REP,
	RX_N_GROUP,
	RX_N_BOL,
	RX_N_EOL,
	RX_N_WB,
	RX_N_NWB
};

/* An engine match result whose positions are character indices. */
struct rx_match {
	int start;
	int end;
	int ngroups;
	int group_start[10];
	int group_end[10];
};

/* A compiled regular-expression instruction. */
struct rx_inst {
	uint8_t op;
	uint32_t cp;		/* RX_I_CHAR */
	int16_t cls;		/* RX_I_CLASS */
	int16_t save;		/* RX_I_SAVE */
	int32_t x, y;		/* RX_I_SPLIT / RX_I_JMP */
};

/* A compiled regular-expression character class. */
struct rx_class {
	bool negate;
	int nranges;
	uint32_t lo[RX_MAX_RANGES];
	uint32_t hi[RX_MAX_RANGES];
};

/* A regular-expression parse-tree node. */
struct rx_node {
	int type;
	uint32_t cp;
	int cls;
	struct rx_node *a, *b;
	int min, max;		/* RX_N_REP; max < 0 means unbounded */
	bool lazy;
	int gidx;		/* RX_N_GROUP */
};

/* A compiled regular-expression program. */
struct rx_prog {
	struct rx_inst ins[RX_MAX_INSTS];
	int ninst;
	struct rx_class cls[RX_MAX_CLASSES];
	int ncls;
	int ngroups;		/* highest capture index used */
};

/* Regular-expression compiler state. */
struct rx_comp {
	const uint32_t *pat;
	int patlen;
	int pos;
	struct rx_prog *prog;
	struct rx_node nodes[RX_MAX_NODES];
	int nnodes;
	int ngroups;
	char *errbuf;
	size_t errsize;
	bool failed;
};

/* Regular-expression execution state. */
struct rx_exec {
	const struct rx_prog *prog;
	const uint32_t *str;
	int len;
	int budget;
	bool overflow;
};

/* A dynamically growing replacement buffer. */
struct regex_output {
	uint32_t *codepoint;
	int length;
	int capacity;
};

/* Native allocations owned by one replacement operation. */
struct regex_replace_state {
	struct rx_prog *prog;
	uint32_t *codepoint;
	uint32_t *replacement;
	struct regex_output output;
	char *utf8;
};

/* Parameter names published by the three Regex functions. */
static const char *regex_search_param[] = {"pat", "s", "from"};
static const char *regex_matches_param[] = {"pat", "s"};
static const char *regex_replace_all_param[] = {"pat", "s", "repl"};

static int noct_rx_utf8_len(const char *s);
static void noct_rx_utf8_decode(const char *s, uint32_t *out);
static int noct_rx_utf8_encode(uint32_t cp, char *out);
static void rx_error(struct rx_comp *c, const char *msg);
static struct rx_node *rx_new_node(struct rx_comp *c, int type);
static uint32_t rx_peek(struct rx_comp *c);
static uint32_t rx_next(struct rx_comp *c);
static bool rx_is_word_cp(uint32_t cp);
static int rx_class_add(struct rx_class *cl, uint32_t lo, uint32_t hi);
static void rx_class_add_predef(struct rx_class *cl, uint32_t esc);
static uint32_t rx_escape_char(uint32_t esc);
static int rx_alloc_class(struct rx_comp *c);
static struct rx_node *rx_predef_node(struct rx_comp *c, uint32_t esc);
static struct rx_node *rx_parse_class(struct rx_comp *c);
static struct rx_node *rx_parse_alt(struct rx_comp *c);
static struct rx_node *rx_parse_atom(struct rx_comp *c);
static bool rx_parse_braces(struct rx_comp *c, int *min, int *max);
static struct rx_node *rx_parse_rep(struct rx_comp *c);
static struct rx_node *rx_parse_cat(struct rx_comp *c);
static int rx_emit(struct rx_comp *c, int op);
static void rx_gen(struct rx_comp *c, struct rx_node *n);
static int noct_rx_compile(struct rx_prog *prog, const uint32_t *pat, int patlen, bool anchor_end, char *errbuf, size_t errsize);
static bool rx_class_test(const struct rx_class *cl, uint32_t cp);
static int rx_run(struct rx_exec *e, int pc, int pos, int *saves, int depth);
static int noct_rx_search(const struct rx_prog *prog, const uint32_t *str, int len, int from, struct rx_match *m);
static size_t noct_rx_prog_size(void);
static bool cfunc_Regex_search(NoctEnv *env);
static bool cfunc_Regex_matches(NoctEnv *env);
static bool cfunc_Regex_replaceAll(NoctEnv *env);
static bool regex_register_pinned(NoctEnv *env, NoctValue *dict, NoctValue *func_value);
static bool regex_register_function(NoctEnv *env, NoctValue *dict, NoctValue *func_value, const char *global_name, const char *field_name, size_t param_count, const char *param[], bool (*cfunc)(NoctEnv *env));
static bool regex_prepare(NoctEnv *env, const char *pat_s, const char *str_s, bool anchor_end, struct rx_prog **prog, uint32_t **str, int *str_len);
static bool regex_append(NoctEnv *env, struct regex_output *output, uint32_t codepoint);
static bool regex_search_pinned(NoctEnv *env, NoctValue *pat, NoctValue *str, NoctValue *from, NoctValue *ret, NoctValue *groups, NoctValue *group, NoctValue *tmp);
static bool regex_matches_pinned(NoctEnv *env, NoctValue *pat, NoctValue *str, NoctValue *ret);
static void regex_replace_cleanup(struct regex_replace_state *state);
static bool regex_replace_pinned(NoctEnv *env, NoctValue *pat, NoctValue *str, NoctValue *repl, NoctValue *ret);

/*
 * Registers the Regex API functions.
 */
NOCT_DLL
bool
noct_register_api_regex(
	NoctEnv *env)
{
	NoctValue dict;
	NoctValue func_value;
	bool result;

	/* Initializes both values before exposing them as roots. */
	memset(&dict, 0, sizeof(dict));
	memset(&func_value, 0, sizeof(func_value));

	/* Roots the package values during registration. */
	if (!noct_pin_local(env, 2, &dict, &func_value))
		return false;

	/* Registers every function while the package values remain rooted. */
	result = regex_register_pinned(env, &dict, &func_value);

	/* Releases the package roots after registration. */
	(void)noct_unpin_local(env, 2, &dict, &func_value);

	/* Reports the registration result. */
	return result;
}

/*
 * UTF-8 decoding
 */

/* Count Unicode codepoints in a valid UTF-8 string. */
static int
noct_rx_utf8_len(
	const char *s)
{
	unsigned char c;
	int n;

	/* Count Unicode codepoints in the UTF-8 string. */
	n = 0;
	while (*s != '\0') {
		/* Advances across the current UTF-8 sequence. */
		c = (unsigned char)*s;
		if (c < 0x80)
			s += 1;
		else if (c < 0xE0)
			s += 2;
		else if (c < 0xF0)
			s += 3;
		else
			s += 4;
		n++;
	}

	/* Returns the counted codepoint extent. */
	return n;
}

/* Decode a valid UTF-8 string into codepoints. */
static void
noct_rx_utf8_decode(
	const char *s,
	uint32_t *out)
{
	unsigned char c;
	uint32_t cp;
	int n;

	/* Decode every UTF-8 sequence into one codepoint. */
	while (*s != '\0') {
		/* Decodes the leading byte and selects its sequence length. */
		c = (unsigned char)*s;
		if (c < 0x80) {
			cp = c;
			n = 1;
		} else if (c < 0xE0) {
			cp = c & 0x1F;
			n = 2;
		} else if (c < 0xF0) {
			cp = c & 0x0F;
			n = 3;
		} else {
			cp = c & 0x07;
			n = 4;
		}
		s++;

		/* Consume the continuation bytes of this sequence. */
		while (n > 1) {
			cp = (cp << 6) | ((unsigned char)*s & 0x3F);
			s++;
			n--;
		}

		/* Appends the decoded codepoint. */
		*out++ = cp;
	}
}

/* Encode one Unicode codepoint as UTF-8. */
static int
noct_rx_utf8_encode(
	uint32_t cp,
	char *out)
{
	/* Emits one-byte UTF-8. */
	if (cp < 0x80) {
		out[0] = (char)cp;

		/* Reports the one-byte encoding extent. */
		return 1;
	}

	/* Emits two-byte UTF-8. */
	if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));

		/* Reports the two-byte encoding extent. */
		return 2;
	}

	/* Emits three-byte UTF-8. */
	if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));

		/* Reports the three-byte encoding extent. */
		return 3;
	}

	/* Emits four-byte UTF-8. */
	out[0] = (char)(0xF0 | (cp >> 18));
	out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
	out[3] = (char)(0x80 | (cp & 0x3F));

	/* Returns the encoded byte count. */
	return 4;
}

/*
 * Parser
 */

/* Record the first pattern compilation error. */
static void
rx_error(
	struct rx_comp *c,
	const char *msg)
{
	/* Records only the first detailed compiler error. */
	if (!c->failed && c->errbuf != NULL)
		snprintf(c->errbuf, c->errsize, "%s", msg);

	/* Marks compilation as failed. */
	c->failed = true;
}

/* Allocate and initialize one parse-tree node. */
static struct rx_node *
rx_new_node(
	struct rx_comp *c,
	int type)
{
	struct rx_node *n;

	/* Rejects a parse tree beyond the fixed node budget. */
	if (c->nnodes >= RX_MAX_NODES) {
		rx_error(c, N_TR("Regex too large."));
		return &c->nodes[0];
	}

	/* Allocates and initializes the next parse-tree slot. */
	n = &c->nodes[c->nnodes];
	c->nnodes++;
	memset(n, 0, sizeof(*n));
	n->type = type;

	/* Returns the initialized parse-tree node. */
	return n;
}

/* Peek at the next pattern codepoint. */
static uint32_t
rx_peek(
	struct rx_comp *c)
{
	/* Returns zero at the end of the pattern. */
	if (c->pos >= c->patlen)
		return 0;

	/* Returns the unconsumed codepoint. */
	return c->pat[c->pos];
}

/* Consume the next pattern codepoint. */
static uint32_t
rx_next(
	struct rx_comp *c)
{
	uint32_t codepoint;

	/* Returns zero at the end of the pattern. */
	if (c->pos >= c->patlen)
		return 0;

	/* Reads and consumes the next codepoint. */
	codepoint = c->pat[c->pos];
	c->pos++;

	/* Returns the consumed codepoint. */
	return codepoint;
}

/* Test whether a codepoint belongs to the ASCII word class. */
static bool
rx_is_word_cp(
	uint32_t cp)
{
	/* Accepts an ASCII decimal digit. */
	if (cp >= '0' && cp <= '9')
		return true;

	/* Accepts an ASCII uppercase letter. */
	if (cp >= 'A' && cp <= 'Z')
		return true;

	/* Accepts an ASCII lowercase letter. */
	if (cp >= 'a' && cp <= 'z')
		return true;

	/* Accepts the underscore word character. */
	if (cp == '_')
		return true;

	/* Rejects every other codepoint. */
	return false;
}

/* Append one inclusive range to a character class. */
static int
rx_class_add(
	struct rx_class *cl,
	uint32_t lo,
	uint32_t hi)
{
	/* Rejects a range beyond the fixed class budget. */
	if (cl->nranges >= RX_MAX_RANGES)
		return -1;

	/* Appends the inclusive character range. */
	cl->lo[cl->nranges] = lo;
	cl->hi[cl->nranges] = hi;
	cl->nranges++;

	/* Reports successful range insertion. */
	return 0;
}

/* Add the ranges of a predefined class (\d, \w, \s). */
static void
rx_class_add_predef(
	struct rx_class *cl,
	uint32_t esc)
{
	/* Add the ranges selected by the predefined class escape. */
	switch (esc) {
	case 'd':
		rx_class_add(cl, '0', '9');
		break;
	case 'w':
		rx_class_add(cl, '0', '9');
		rx_class_add(cl, 'A', 'Z');
		rx_class_add(cl, 'a', 'z');
		rx_class_add(cl, '_', '_');
		break;
	case 's':
		rx_class_add(cl, ' ', ' ');
		rx_class_add(cl, '\t', '\t');
		rx_class_add(cl, '\n', '\n');
		rx_class_add(cl, '\r', '\r');
		rx_class_add(cl, '\f', '\f');
		rx_class_add(cl, 0x0B, 0x0B);
		break;
	default:
		break;
	}
}

/* Map a control escape to its character, or return the char itself. */
static uint32_t
rx_escape_char(
	uint32_t esc)
{
	uint32_t codepoint;

	/* Translate supported control escapes to codepoints. */
	switch (esc) {
	case 'n':
		codepoint = '\n';
		break;
	case 't':
		codepoint = '\t';
		break;
	case 'r':
		codepoint = '\r';
		break;
	case 'f':
		codepoint = '\f';
		break;
	case '0':
		codepoint = '\0';
		break;
	default:
		codepoint = esc;
		break;
	}

	/* Returns the translated or literal codepoint. */
	return codepoint;
}

/* Allocate a class slot in the program. */
static int
rx_alloc_class(
	struct rx_comp *c)
{
	int index;

	/* Rejects a class beyond the fixed program budget. */
	if (c->prog->ncls >= RX_MAX_CLASSES) {
		rx_error(c, N_TR("Too many character classes."));
		return 0;
	}

	/* Initializes and consumes the next class slot. */
	index = c->prog->ncls;
	memset(&c->prog->cls[index], 0, sizeof(struct rx_class));
	c->prog->ncls++;

	/* Returns the initialized class index. */
	return index;
}

/* Build a node for a predefined class escape, or NULL if not one. */
static struct rx_node *
rx_predef_node(
	struct rx_comp *c,
	uint32_t esc)
{
	struct rx_node *n;
	struct rx_class *cl;
	int idx;

	/* Rejects an escape that is not a predefined class. */
	if (esc != 'd' &&
	    esc != 'D' &&
	    esc != 'w' &&
	    esc != 'W' &&
	    esc != 's' &&
	    esc != 'S')
		return NULL;

	/* Allocates and configures the predefined class. */
	idx = rx_alloc_class(c);
	cl = &c->prog->cls[idx];

	/* Negates uppercase predefined-class escapes. */
	if (esc == 'D' ||
	    esc == 'W' ||
	    esc == 'S')
		cl->negate = true;
	rx_class_add_predef(cl, (uint32_t)(esc | 0x20));

	/* Wraps the class in a parse-tree node. */
	n = rx_new_node(c, RX_N_CLASS);
	n->cls = idx;

	/* Returns the predefined-class node. */
	return n;
}

/* Parse a [...] character class. The '[' is already consumed. */
static struct rx_node *
rx_parse_class(
	struct rx_comp *c)
{
	struct rx_node *n;
	struct rx_class *cl;
	uint32_t lo;
	uint32_t esc;
	uint32_t hi;
	uint32_t peek;
	int idx;
	int add_result;
	bool first;

	/* Allocates the compiled class and selects its storage. */
	idx = rx_alloc_class(c);
	cl = &c->prog->cls[idx];

	/* Consumes an optional leading negation marker. */
	peek = rx_peek(c);
	if (peek == '^') {
		rx_next(c);
		cl->negate = true;
	}

	/* Parse entries until the closing bracket. */
	first = true;
	while (true) {
		/* Reports an unterminated character class. */
		if (c->pos >= c->patlen) {
			rx_error(c, N_TR("Unterminated character class."));
			break;
		}

		/* Reads the next class entry. */
		lo = rx_next(c);

		/* Stops at a non-leading closing bracket. */
		if (lo == ']' && !first)
			break;
		first = false;

		/* Decodes an escaped or predefined class entry. */
		if (lo == '\\') {
			esc = rx_next(c);

			/* Adds a predefined class as a complete entry. */
			if (esc == 'd' ||
			    esc == 'w' ||
			    esc == 's') {
				rx_class_add_predef(cl, esc);
				continue;
			}
			lo = rx_escape_char(esc);
		}

		/* Detects a nonterminal range separator. */
		peek = rx_peek(c);
		if (peek == '-' &&
		    c->pos + 1 < c->patlen &&
		    c->pat[c->pos + 1] != ']') {
			/* Reads and normalizes the range endpoint. */
			rx_next(c);	/* '-' */
			hi = rx_next(c);

			/* Decodes an escaped range endpoint. */
			if (hi == '\\') {
				esc = rx_next(c);
				hi = rx_escape_char(esc);
			}

			/* Reports and normalizes a descending range. */
			if (hi < lo) {
				rx_error(c, N_TR("Invalid character range."));
				hi = lo;
			}

			/* Appends the completed range. */
			add_result = rx_class_add(cl, lo, hi);
			if (add_result < 0)
				rx_error(c, N_TR("Character class too large."));
		} else {
			/* Appends one literal class codepoint. */
			add_result = rx_class_add(cl, lo, lo);
			if (add_result < 0)
				rx_error(c, N_TR("Character class too large."));
		}
	}

	/* Wraps the compiled class in a parse-tree node. */
	n = rx_new_node(c, RX_N_CLASS);
	n->cls = idx;

	/* Returns the completed class node. */
	return n;
}

/* Parse one atom. */
static struct rx_node *
rx_parse_atom(
	struct rx_comp *c)
{
	struct rx_node *n;
	uint32_t cp;
	uint32_t esc;
	bool capture;
	int gidx;

	/* Reads the atom-leading codepoint. */
	cp = rx_next(c);

	/* Parse the atom selected by the next pattern codepoint. */
	switch (cp) {
	case '.':
		n = rx_new_node(c, RX_N_ANY);
		break;
	case '^':
		n = rx_new_node(c, RX_N_BOL);
		break;
	case '$':
		n = rx_new_node(c, RX_N_EOL);
		break;
	case '[':
		n = rx_parse_class(c);
		break;
	case '(':
		/* Initializes capture state for the parenthesized atom. */
		capture = true;
		gidx = 0;

		/* Recognizes the supported noncapturing prefix. */
		cp = rx_peek(c);
		if (cp == '?') {
			rx_next(c);
			cp = rx_next(c);
			if (cp != ':') {
				rx_error(c, N_TR("Unsupported (?...) construct."));
				n = rx_new_node(c, RX_N_EMPTY);
				break;
			}
			capture = false;
		}

		/* Allocates one of the supported capture slots. */
		if (capture) {
			/* Rejects a group beyond the supported capture budget. */
			if (c->ngroups >= 9) {
				rx_error(c, N_TR("Too many capture groups."));
				n = rx_new_node(c, RX_N_EMPTY);
				break;
			}
			gidx = ++c->ngroups;
		}

		/* Parses the parenthesized alternative. */
		n = rx_new_node(c, RX_N_GROUP);
		n->gidx = capture ? gidx : 0;
		n->a = rx_parse_alt(c);

		/* Requires the closing parenthesis. */
		cp = rx_next(c);
		if (cp != ')')
			rx_error(c, N_TR("Unmatched parenthesis."));

		break;
	case ')':
		rx_error(c, N_TR("Unmatched parenthesis."));
		n = rx_new_node(c, RX_N_EMPTY);
		break;
	case '\\':
		/* Reads the escaped codepoint. */
		esc = rx_next(c);

		/* Builds the node selected by the escape. */
		if (esc == 'b') {
			n = rx_new_node(c, RX_N_WB);
		} else if (esc == 'B') {
			n = rx_new_node(c, RX_N_NWB);
		} else {
			n = rx_predef_node(c, esc);

			/* Builds a literal when the escape is not predefined. */
			if (n == NULL) {
				n = rx_new_node(c, RX_N_CHAR);
				n->cp = rx_escape_char(esc);
			}
		}
		break;
	default:
		/* Builds an ordinary literal node. */
		n = rx_new_node(c, RX_N_CHAR);
		n->cp = cp;
		break;
	}

	/* Returns the completed atom node. */
	return n;
}

/* Parse {m}, {m,}, {m,n}. Returns false if not a valid repeat form. */
static bool
rx_parse_braces(
	struct rx_comp *c,
	int *pmin,
	int *pmax)
{
	int save;
	int m;
	int n;
	uint32_t peek;
	uint32_t digit;
	bool has_m;
	bool has_n;
	bool comma;

	/* Initializes the tentative repetition parse. */
	save = c->pos;
	m = 0;
	n = 0;
	has_m = false;
	has_n = false;
	comma = false;

	/* Consumes the opening brace. */
	rx_next(c);

	/* Parses the lower repetition bound. */
	peek = rx_peek(c);
	while (peek >= '0' && peek <= '9') {
		/* Consumes and incorporates the next lower-bound digit. */
		digit = rx_next(c);
		m = m * 10 + (int)(digit - '0');
		has_m = true;

		/* Stops once the lower bound exceeds the supported limit. */
		if (m > RX_MAX_REPEAT)
			break;
		peek = rx_peek(c);
	}

	/* Parses an optional comma and upper bound. */
	peek = rx_peek(c);
	if (peek == ',') {
		rx_next(c);
		comma = true;

		/* Parses the optional upper repetition bound. */
		peek = rx_peek(c);
		while (peek >= '0' && peek <= '9') {
			/* Consumes and incorporates the next upper-bound digit. */
			digit = rx_next(c);
			n = n * 10 + (int)(digit - '0');
			has_n = true;

			/* Stops once the upper bound exceeds the supported limit. */
			if (n > RX_MAX_REPEAT)
				break;
			peek = rx_peek(c);
		}
	}

	/* Validates the complete repetition suffix. */
	peek = rx_peek(c);
	if (!has_m ||
	    peek != '}' ||
	    m > RX_MAX_REPEAT ||
	    (has_n &&
	     (n > RX_MAX_REPEAT ||
	      n < m))) {
		/* Restores the parser to treat the brace as a literal. */
		c->pos = save;
		return false;
	}

	/* Consumes the closing brace and publishes the lower bound. */
	rx_next(c);
	*pmin = m;

	/* Publishes the fixed, bounded, or unbounded upper limit. */
	if (!comma)
		*pmax = m;
	else if (has_n)
		*pmax = n;
	else
		*pmax = -1;

	/* Reports a valid repetition suffix. */
	return true;
}

/* Parse an atom with a possible repetition suffix. */
static struct rx_node *
rx_parse_rep(
	struct rx_comp *c)
{
	struct rx_node *atom;
	struct rx_node *n;
	int min;
	int max;
	uint32_t cp;

	/* Parses the repeated atom and peeks at its suffix. */
	atom = rx_parse_atom(c);
	cp = rx_peek(c);

	/* Decodes the supported repetition suffix. */
	if (cp == '*') {
		min = 0;
		max = -1;
		rx_next(c);
	} else if (cp == '+') {
		min = 1;
		max = -1;
		rx_next(c);
	} else if (cp == '?') {
		min = 0;
		max = 1;
		rx_next(c);
	} else if (cp == '{') {
		/* Returns the atom when the brace is not a valid suffix. */
		if (!rx_parse_braces(c, &min, &max))
			return atom;
	} else {
		/* Returns an atom without a repetition suffix. */
		return atom;
	}

	/* Builds the repetition node. */
	n = rx_new_node(c, RX_N_REP);
	n->a = atom;
	n->min = min;
	n->max = max;

	/* Consumes an optional lazy marker. */
	cp = rx_peek(c);
	if (cp == '?') {
		rx_next(c);
		n->lazy = true;
	}

	/* Returns the completed repetition node. */
	return n;
}

/* Parse a concatenation. */
static struct rx_node *
rx_parse_cat(
	struct rx_comp *c)
{
	struct rx_node *left;
	struct rx_node *n;
	struct rx_node *cat;
	uint32_t cp;

	/* Combine adjacent atoms into concatenation nodes. */
	left = NULL;
	while (c->pos < c->patlen && !c->failed) {
		/* Stops before an alternative or group terminator. */
		cp = rx_peek(c);
		if (cp == '|' || cp == ')')
			break;

		/* Parses and appends the next repeated atom. */
		n = rx_parse_rep(c);
		if (left == NULL) {
			left = n;
		} else {
			cat = rx_new_node(c, RX_N_CAT);
			cat->a = left;
			cat->b = n;
			left = cat;
		}
	}

	/* Represents an empty concatenation explicitly. */
	if (left == NULL)
		left = rx_new_node(c, RX_N_EMPTY);

	/* Returns the completed concatenation tree. */
	return left;
}

/* Parse an alternation. */
static struct rx_node *
rx_parse_alt(
	struct rx_comp *c)
{
	struct rx_node *left;
	struct rx_node *alt;
	struct rx_node *right;
	uint32_t cp;

	/* Parses the first alternative. */
	left = rx_parse_cat(c);

	/* Fold every alternative into the parse tree. */
	cp = rx_peek(c);
	while (cp == '|' && !c->failed) {
		/* Parses and appends the next alternative. */
		rx_next(c);
		right = rx_parse_cat(c);
		alt = rx_new_node(c, RX_N_ALT);
		alt->a = left;
		alt->b = right;
		left = alt;
		cp = rx_peek(c);
	}

	/* Returns the completed alternation tree. */
	return left;
}

/*
 * Code generation
 */

/* Append one initialized instruction to the compiled program. */
static int
rx_emit(
	struct rx_comp *c,
	int op)
{
	struct rx_prog *p;
	int index;

	/* Selects the destination program. */
	p = c->prog;

	/* Rejects an instruction beyond the fixed program budget. */
	if (p->ninst >= RX_MAX_INSTS) {
		rx_error(c, N_TR("Regex too large."));
		return p->ninst - 1;
	}

	/* Initializes the next instruction slot. */
	index = p->ninst;
	memset(&p->ins[index], 0, sizeof(struct rx_inst));
	p->ins[index].op = (uint8_t)op;
	p->ninst++;

	/* Returns the initialized instruction index. */
	return index;
}

/* Emit instructions for one parse-tree node. */
static void
rx_gen(
	struct rx_comp *c,
	struct rx_node *n)
{
	struct rx_prog *p;
	int splits[RX_MAX_REPEAT];
	int i;
	int sp;
	int jmp;
	int k;
	int nsplits;

	/* Selects the destination program. */
	p = c->prog;

	/* Stops generation after the compiler has failed. */
	if (c->failed)
		return;

	/* Emit the instruction sequence for this parse-tree node. */
	switch (n->type) {
	case RX_N_EMPTY:
		break;
	case RX_N_CHAR:
		i = rx_emit(c, RX_I_CHAR);
		p->ins[i].cp = n->cp;
		break;
	case RX_N_ANY:
		rx_emit(c, RX_I_ANY);
		break;
	case RX_N_CLASS:
		i = rx_emit(c, RX_I_CLASS);
		p->ins[i].cls = (int16_t)n->cls;
		break;
	case RX_N_BOL:
		rx_emit(c, RX_I_BOL);
		break;
	case RX_N_EOL:
		rx_emit(c, RX_I_EOL);
		break;
	case RX_N_WB:
		rx_emit(c, RX_I_WB);
		break;
	case RX_N_NWB:
		rx_emit(c, RX_I_NWB);
		break;
	case RX_N_CAT:
		rx_gen(c, n->a);
		rx_gen(c, n->b);
		break;
	case RX_N_ALT:
		/* Emits and patches both alternatives. */
		sp = rx_emit(c, RX_I_SPLIT);
		rx_gen(c, n->a);
		jmp = rx_emit(c, RX_I_JMP);
		p->ins[sp].x = sp + 1;
		p->ins[sp].y = p->ninst;
		rx_gen(c, n->b);
		p->ins[jmp].x = p->ninst;
		break;
	case RX_N_GROUP:
		/* Emits a leading capture boundary when requested. */
		if (n->gidx > 0) {
			i = rx_emit(c, RX_I_SAVE);
			p->ins[i].save = (int16_t)(n->gidx * 2);
		}

		/* Emits the group body. */
		rx_gen(c, n->a);

		/* Emits a trailing capture boundary when requested. */
		if (n->gidx > 0) {
			i = rx_emit(c, RX_I_SAVE);
			p->ins[i].save = (int16_t)(n->gidx * 2 + 1);
		}
		break;
	case RX_N_REP:
		/* Emits the mandatory repetition copies. */
		for (k = 0; k < n->min; k++) {
			rx_gen(c, n->a);
		}

		/* Emits the unbounded or bounded repetition tail. */
		if (n->max < 0) {
			/* Emits an unbounded split-body-jump tail. */
			sp = rx_emit(c, RX_I_SPLIT);
			rx_gen(c, n->a);
			i = rx_emit(c, RX_I_JMP);
			p->ins[i].x = sp;

			/* Orders the split according to repetition greediness. */
			if (n->lazy) {
				p->ins[sp].x = p->ninst;
				p->ins[sp].y = sp + 1;
			} else {
				p->ins[sp].x = sp + 1;
				p->ins[sp].y = p->ninst;
			}
		} else {
			/* Emits one optional copy for each remaining repeat. */
			nsplits = 0;
			for (k = 0; k < n->max - n->min; k++) {
				splits[nsplits++] = rx_emit(c, RX_I_SPLIT);
				rx_gen(c, n->a);
			}

			/* Patches the optional branches after emission. */
			for (k = 0; k < nsplits; k++) {
				/* Orders this split according to repetition greediness. */
				if (n->lazy) {
					p->ins[splits[k]].x = p->ninst;
					p->ins[splits[k]].y = splits[k] + 1;
				} else {
					p->ins[splits[k]].x = splits[k] + 1;
					p->ins[splits[k]].y = p->ninst;
				}
			}
		}
		break;
	default:
		break;
	}
}

/* Compile a pattern, returning zero on success and -1 on error. */
static int
noct_rx_compile(
	struct rx_prog *prog,
	const uint32_t *pat,
	int patlen,
	bool anchor_end,
	char *errbuf,
	size_t errsize)
{
	struct rx_comp *c;
	struct rx_node *root;
	int i;

	/* Allocates the temporary compiler state. */
	c = noct_malloc(sizeof(struct rx_comp));
	if (c == NULL) {
		snprintf(errbuf, errsize, "Out of memory.");
		return -1;
	}

	/* Initializes the compiler and destination program. */
	memset(c, 0, sizeof(*c));
	memset(prog, 0, sizeof(*prog));
	c->pat = pat;
	c->patlen = patlen;
	c->prog = prog;
	c->errbuf = errbuf;
	c->errsize = errsize;

	/* Emits the leading match boundary and parses the pattern. */
	i = rx_emit(c, RX_I_SAVE);
	prog->ins[i].save = 0;
	root = rx_parse_alt(c);

	/* Reports pattern text left after the top-level parse. */
	if (c->pos < c->patlen && !c->failed)
		rx_error(c, N_TR("Unmatched parenthesis."));

	/* Emits the parsed expression body. */
	rx_gen(c, root);

	/* Emits an optional end-of-subject anchor. */
	if (anchor_end)
		rx_emit(c, RX_I_EOS);

	/* Emits the trailing match boundary and acceptance instruction. */
	i = rx_emit(c, RX_I_SAVE);
	prog->ins[i].save = 1;
	rx_emit(c, RX_I_MATCH);
	prog->ngroups = c->ngroups;

	/* Releases a failed compiler state and reports failure. */
	if (c->failed) {
		noct_free(c);
		return -1;
	}

	/* Releases the successful compiler state. */
	noct_free(c);

	/* Reports successful compilation. */
	return 0;
}

/*
 * Execution
 */

/* Test one codepoint against a compiled character class. */
static bool
rx_class_test(
	const struct rx_class *cl,
	uint32_t cp)
{
	int i;
	bool in;

	/* Test the codepoint against every range in the class. */
	in = false;
	for (i = 0; i < cl->nranges; i++) {
		/* Stops after finding a containing range. */
		if (cp >= cl->lo[i] && cp <= cl->hi[i]) {
			in = true;
			break;
		}
	}

	/* Applies the class negation flag. */
	if (cl->negate)
		return !in;

	/* Returns the positive class result. */
	return in;
}

/* Return 1 on match, zero on failure, or -1 on budget exhaustion. */
static int
rx_run(
	struct rx_exec *e,
	int pc,
	int pos,
	int *saves,
	int depth)
{
	const struct rx_inst *ins;
	bool class_matches;
	bool left_word;
	bool right_word;
	bool boundary;
	int old;
	int run_result;

	/* Rejects an execution path beyond the recursion cap. */
	if (depth > RX_DEPTH_MAX) {
		e->overflow = true;
		return -1;
	}

	/* Execute instructions until this path accepts or fails. */
	for (;;) {
		/* Charges the next instruction against the execution budget. */
		if (--e->budget < 0) {
			e->overflow = true;
			return -1;
		}

		/* Selects the next compiled instruction. */
		ins = &e->prog->ins[pc];

		/* Execute the current regular-expression instruction. */
		switch (ins->op) {
		case RX_I_CHAR:
			/* Consumes the requested literal when it matches. */
			if (pos < e->len && e->str[pos] == ins->cp) {
				pc++;
				pos++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_ANY:
			/* Consumes one non-newline codepoint. */
			if (pos < e->len && e->str[pos] != '\n') {
				pc++;
				pos++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_CLASS:
			/* Tests an available codepoint against the class. */
			if (pos < e->len) {
				class_matches = rx_class_test(
					&e->prog->cls[ins->cls],
					e->str[pos]);
				if (class_matches) {
					pc++;
					pos++;
					continue;
				}
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_BOL:
			/* Accepts a beginning-of-line position. */
			if (pos == 0 || e->str[pos - 1] == '\n') {
				pc++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_EOL:
			/* Accepts an end-of-line position. */
			if (pos == e->len || e->str[pos] == '\n') {
				pc++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_EOS:
			/* Accepts the end-of-subject position. */
			if (pos == e->len) {
				pc++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_WB:
		case RX_I_NWB:
			/* Classifies the codepoint before the boundary. */
			left_word = false;
			if (pos > 0)
				left_word = rx_is_word_cp(e->str[pos - 1]);

			/* Classifies the codepoint after the boundary. */
			right_word = false;
			if (pos < e->len)
				right_word = rx_is_word_cp(e->str[pos]);

			/* Accepts the requested boundary polarity. */
			boundary = left_word != right_word;
			if (boundary == (ins->op == RX_I_WB)) {
				pc++;
				continue;
			}

			/* Rejects this execution path. */
			return 0;
		case RX_I_SAVE:
			/* Saves and tentatively updates one capture boundary. */
			old = saves[ins->save];
			saves[ins->save] = pos;

			/* Executes the path after the saved boundary. */
			run_result = rx_run(e, pc + 1, pos, saves, depth + 1);
			if (run_result != 0)
				return run_result;

			/* Restores the capture boundary after a rejected path. */
			saves[ins->save] = old;

			/* Rejects this execution path. */
			return 0;
		case RX_I_SPLIT:
			/* Executes the first split branch. */
			run_result = rx_run(e, ins->x, pos, saves, depth + 1);
			if (run_result != 0)
				return run_result;

			/* Continues with the second split branch. */
			pc = ins->y;
			continue;
		case RX_I_JMP:
			/* Continues at the jump target. */
			pc = ins->x;
			continue;
		case RX_I_MATCH:
			/* Accepts this execution path. */
			return 1;
		default:
			/* Rejects an unknown instruction. */
			return 0;
		}
	}
}

/* Search for the leftmost match at or after the requested position. */
static int
noct_rx_search(
	const struct rx_prog *prog,
	const uint32_t *str,
	int len,
	int from,
	struct rx_match *m)
{
	struct rx_exec e;
	int saves[20];
	int start;
	int i;
	int run_result;

	/* Initializes the shared execution budget and subject view. */
	e.prog = prog;
	e.str = str;
	e.len = len;
	e.budget = RX_STEP_BUDGET;
	e.overflow = false;

	/* Try every possible starting position from left to right. */
	for (start = from; start <= len; start++) {
		/* Clear all capture slots for this attempt. */
		for (i = 0; i < 20; i++)
			saves[i] = -1;

		/* Executes the program from this candidate position. */
		run_result = rx_run(&e, 0, start, saves, 0);

		/* Propagates budget or recursion exhaustion. */
		if (run_result < 0)
			return -1;

		/* Publishes the first accepted match. */
		if (run_result == 1) {
			m->start = saves[0];
			m->end = saves[1];
			m->ngroups = prog->ngroups;

			/* Copy the capture slots into the match result. */
			for (i = 0; i < 10; i++) {
				m->group_start[i] = saves[i * 2];
				m->group_end[i] = saves[i * 2 + 1];
			}

			/* Reports a successful match. */
			return 1;
		}
	}

	/* Reports that no starting position matched. */
	return 0;
}

/* Returns the opaque compiled-program allocation size. */
static size_t
noct_rx_prog_size(
	void)
{
	/* Returns the complete compiled-program size. */
	return sizeof(struct rx_prog);
}

/* Registers every function in the rooted Regex package. */
static bool
regex_register_pinned(
	NoctEnv *env,
	NoctValue *dict,
	NoctValue *func_value)
{
	/* Creates the Regex package dictionary. */
	if (!noct_make_empty_dict(env, dict))
		return false;

	/* Publishes the Regex package dictionary. */
	if (!noct_set_global(env, "Regex", dict))
		return false;

	/* Registers Regex.search. */
	if (!regex_register_function(
		env,
		dict,
		func_value,
		"Regex.search",
		"search",
		3,
		regex_search_param,
		cfunc_Regex_search)) {
		return false;
	}

	/* Registers Regex.matches. */
	if (!regex_register_function(
		env,
		dict,
		func_value,
		"Regex.matches",
		"matches",
		2,
		regex_matches_param,
		cfunc_Regex_matches)) {
		return false;
	}

	/* Registers Regex.replaceAll. */
	if (!regex_register_function(
		env,
		dict,
		func_value,
		"Regex.replaceAll",
		"replaceAll",
		3,
		regex_replace_all_param,
		cfunc_Regex_replaceAll)) {
		return false;
	}

	/* Reports successful package registration. */
	return true;
}

/* Registers one function in the Regex package dictionary. */
static bool
regex_register_function(
	NoctEnv *env,
	NoctValue *dict,
	NoctValue *func_value,
	const char *global_name,
	const char *field_name,
	size_t param_count,
	const char *param[],
	bool (*cfunc)(NoctEnv *env))
{
	/* Registers the native function as a global value. */
	if (!noct_register_cfunc(
		env,
		global_name,
		param_count,
		param,
		cfunc,
		NULL)) {
		return false;
	}

	/* Reads the registered function value. */
	if (!noct_get_global(env, global_name, func_value))
		return false;

	/* Publishes the function in the package dictionary. */
	if (!noct_set_dict_elem_cstr(
		env,
		dict,
		field_name,
		func_value)) {
		return false;
	}

	/* Reports successful function registration. */
	return true;
}

/* Decodes the input string and compiles the regular expression. */
static bool
regex_prepare(
	NoctEnv *env,
	const char *pat_s,
	const char *str_s,
	bool anchor_end,
	struct rx_prog **prog,
	uint32_t **str,
	int *str_len)
{
	char errbuf[128];
	uint32_t *pat;
	int compile_result;
	int pat_len;

	/* Initializes the returned ownership state. */
	*prog = NULL;
	*str = NULL;
	*str_len = 0;
	pat = NULL;

	/* Allocates the compiled program. */
	*prog = noct_malloc(noct_rx_prog_size());
	if (*prog == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Allocates and decodes the pattern codepoints. */
	pat_len = noct_rx_utf8_len(pat_s);
	pat = noct_malloc(sizeof(uint32_t) * (size_t)(pat_len + 1));
	if (pat == NULL) {
		noct_free(*prog);
		*prog = NULL;
		noct_out_of_memory(env);
		return false;
	}
	noct_rx_utf8_decode(pat_s, pat);

	/* Compiles the decoded regular expression. */
	compile_result = noct_rx_compile(
		*prog,
		pat,
		pat_len,
		anchor_end,
		errbuf,
		sizeof(errbuf));
	if (compile_result < 0) {
		noct_free(pat);
		noct_free(*prog);
		*prog = NULL;
		noct_error(env, N_TR("Regex error: %s"), errbuf);
		return false;
	}

	/* Releases the temporary decoded pattern. */
	noct_free(pat);

	/* Allocates and decodes the subject codepoints. */
	*str_len = noct_rx_utf8_len(str_s);
	*str = noct_malloc(sizeof(uint32_t) * (size_t)(*str_len + 1));
	if (*str == NULL) {
		noct_free(*prog);
		*prog = NULL;
		noct_out_of_memory(env);
		return false;
	}
	noct_rx_utf8_decode(str_s, *str);

	/* Reports successful preparation. */
	return true;
}

/* Appends one codepoint to a replacement buffer. */
static bool
regex_append(
	NoctEnv *env,
	struct regex_output *output,
	uint32_t codepoint)
{
	uint32_t *new_codepoint;
	int new_capacity;

	/* Grows a full replacement buffer. */
	if (output->length >= output->capacity) {
		/* Rejects a capacity that cannot be doubled. */
		if (output->capacity > INT_MAX / 2) {
			noct_out_of_memory(env);
			return false;
		}

		/* Reallocates the replacement buffer at twice its capacity. */
		new_capacity = output->capacity * 2;
		new_codepoint = noct_realloc(
			output->codepoint,
			sizeof(uint32_t) * (size_t)new_capacity);
		if (new_codepoint == NULL) {
			noct_out_of_memory(env);
			return false;
		}

		/* Publishes the enlarged replacement buffer. */
		output->codepoint = new_codepoint;
		output->capacity = new_capacity;
	}

	/* Appends and consumes the requested output slot. */
	output->codepoint[output->length] = codepoint;
	output->length++;

	/* Reports successful append. */
	return true;
}

/* Implements Regex.search(). */
static bool
cfunc_Regex_search(
	NoctEnv *env)
{
	NoctValue pat;
	NoctValue str;
	NoctValue from;
	NoctValue ret;
	NoctValue groups;
	NoctValue group;
	NoctValue tmp;
	bool result;

	/* Initializes all values before exposing them as roots. */
	memset(&pat, 0, sizeof(pat));
	memset(&str, 0, sizeof(str));
	memset(&from, 0, sizeof(from));
	memset(&ret, 0, sizeof(ret));
	memset(&groups, 0, sizeof(groups));
	memset(&group, 0, sizeof(group));
	memset(&tmp, 0, sizeof(tmp));

	/* Roots the values used by the search implementation. */
	if (!noct_pin_local(
		env,
		7,
		&pat,
		&str,
		&from,
		&ret,
		&groups,
		&group,
		&tmp)) {
		return false;
	}

	/* Executes the search while its Noct values remain rooted. */
	result = regex_search_pinned(
		env,
		&pat,
		&str,
		&from,
		&ret,
		&groups,
		&group,
		&tmp);

	/* Releases the search roots and propagates an unpin failure. */
	if (!noct_unpin_local(
		env,
		7,
		&pat,
		&str,
		&from,
		&ret,
		&groups,
		&group,
		&tmp)) {
		return false;
	}

	/* Reports the search result. */
	return result;
}

/* Executes Regex.search() with its Noct values rooted. */
static bool
regex_search_pinned(
	NoctEnv *env,
	NoctValue *pat,
	NoctValue *str,
	NoctValue *from,
	NoctValue *ret,
	NoctValue *groups,
	NoctValue *group,
	NoctValue *tmp)
{
	const char *pat_s;
	const char *str_s;
	struct rx_prog *prog;
	uint32_t *codepoint;
	struct rx_match match;
	int from_index;
	int length;
	int search_result;
	int i;
	bool result;

	/* Reads the pattern argument. */
	if (!noct_get_arg_check_string(env, 0, pat, &pat_s))
		return false;

	/* Reads the subject argument. */
	if (!noct_get_arg_check_string(env, 1, str, &str_s))
		return false;

	/* Reads the starting character index. */
	if (!noct_get_arg_check_int(env, 2, from, &from_index))
		return false;

	/* Compiles the pattern and decodes the subject. */
	if (!regex_prepare(
		env,
		pat_s,
		str_s,
		false,
		&prog,
		&codepoint,
		&length)) {
		return false;
	}

	/* Clamps a negative starting position to zero. */
	if (from_index < 0)
		from_index = 0;

	/* Clamps a starting position beyond the subject extent. */
	if (from_index > length)
		from_index = length;

	/* Searches the decoded subject. */
	search_result = noct_rx_search(
		prog,
		codepoint,
		length,
		from_index,
		&match);

	/* Releases the native search representation. */
	noct_free(codepoint);
	noct_free(prog);

	/* Reports an exhausted matching budget. */
	if (search_result < 0) {
		noct_error(env, N_TR("Regex too complex."));
		return false;
	}

	/* Returns zero when no match exists. */
	if (search_result == 0) {
		result = noct_set_return_make_int(env, ret, 0);

		/* Reports the return-value publication result. */
		return result;
	}

	/* Creates the successful match dictionary. */
	if (!noct_make_empty_dict(env, ret))
		return false;

	/* Publishes the match start position. */
	if (!noct_set_dict_elem_make_int(
		env,
		ret,
		"start",
		tmp,
		match.start)) {
		return false;
	}

	/* Publishes the match end position. */
	if (!noct_set_dict_elem_make_int(
		env,
		ret,
		"end",
		tmp,
		match.end)) {
		return false;
	}

	/* Creates the capture-group array. */
	if (!noct_make_empty_array(env, groups))
		return false;

	/* Builds one dictionary for each capture group. */
	for (i = 1; i <= match.ngroups; i++) {
		/* Creates the next capture-group dictionary. */
		if (!noct_make_empty_dict(env, group))
			return false;

		/* Publishes the capture start position. */
		if (!noct_set_dict_elem_make_int(
			env,
			group,
			"start",
			tmp,
			match.group_start[i])) {
			return false;
		}

		/* Publishes the capture end position. */
		if (!noct_set_dict_elem_make_int(
			env,
			group,
			"end",
			tmp,
			match.group_end[i])) {
			return false;
		}

		/* Appends the capture dictionary to the group array. */
		if (!noct_set_array_elem(
			env,
			groups,
			(size_t)(i - 1),
			group)) {
			return false;
		}
	}

	/* Publishes the complete capture-group array. */
	if (!noct_set_dict_elem_cstr(env, ret, "groups", groups))
		return false;

	/* Publishes the complete match dictionary. */
	result = noct_set_return(env, ret);

	/* Reports the return-value publication result. */
	return result;
}

/* Implements Regex.matches(). */
static bool
cfunc_Regex_matches(
	NoctEnv *env)
{
	NoctValue pat;
	NoctValue str;
	NoctValue ret;
	bool result;

	/* Initializes all values before exposing them as roots. */
	memset(&pat, 0, sizeof(pat));
	memset(&str, 0, sizeof(str));
	memset(&ret, 0, sizeof(ret));

	/* Roots the values used by the matching implementation. */
	if (!noct_pin_local(env, 3, &pat, &str, &ret))
		return false;

	/* Executes the match while its Noct values remain rooted. */
	result = regex_matches_pinned(env, &pat, &str, &ret);

	/* Releases the match roots and propagates an unpin failure. */
	if (!noct_unpin_local(env, 3, &pat, &str, &ret))
		return false;

	/* Reports the match result. */
	return result;
}

/* Executes Regex.matches() with its Noct values rooted. */
static bool
regex_matches_pinned(
	NoctEnv *env,
	NoctValue *pat,
	NoctValue *str,
	NoctValue *ret)
{
	const char *pat_s;
	const char *str_s;
	struct rx_prog *prog;
	uint32_t *codepoint;
	struct rx_match match;
	int length;
	int search_result;
	int return_value;
	bool result;

	/* Reads the pattern argument. */
	if (!noct_get_arg_check_string(env, 0, pat, &pat_s))
		return false;

	/* Reads the subject argument. */
	if (!noct_get_arg_check_string(env, 1, str, &str_s))
		return false;

	/* Compiles the end-anchored pattern and decodes the subject. */
	if (!regex_prepare(
		env,
		pat_s,
		str_s,
		true,
		&prog,
		&codepoint,
		&length)) {
		return false;
	}

	/* Searches from zero with the compiled end anchor. */
	search_result = noct_rx_search(
		prog,
		codepoint,
		length,
		0,
		&match);

	/* Releases the native matching representation. */
	noct_free(codepoint);
	noct_free(prog);

	/* Reports an exhausted matching budget. */
	if (search_result < 0) {
		noct_error(env, N_TR("Regex too complex."));
		return false;
	}

	/* Rejects a match that did not begin at the subject start. */
	if (search_result == 1 && match.start != 0)
		search_result = 0;

	/* Publishes the Boolean integer result. */
	return_value = search_result == 1 ? 1 : 0;
	result = noct_set_return_make_int(env, ret, return_value);

	/* Reports the return-value publication result. */
	return result;
}

/* Releases every native allocation owned by a replacement operation. */
static void
regex_replace_cleanup(
	struct regex_replace_state *state)
{
	/* Releases allocations in the historical cleanup order. */
	noct_free(state->utf8);
	noct_free(state->output.codepoint);
	noct_free(state->replacement);
	noct_free(state->codepoint);
	noct_free(state->prog);
}

/* Implements Regex.replaceAll(). */
static bool
cfunc_Regex_replaceAll(
	NoctEnv *env)
{
	NoctValue pat;
	NoctValue str;
	NoctValue repl;
	NoctValue ret;
	bool result;

	/* Initializes all values before exposing them as roots. */
	memset(&pat, 0, sizeof(pat));
	memset(&str, 0, sizeof(str));
	memset(&repl, 0, sizeof(repl));
	memset(&ret, 0, sizeof(ret));

	/* Roots the values used by the replacement implementation. */
	if (!noct_pin_local(env, 4, &pat, &str, &repl, &ret))
		return false;

	/* Executes replacement while its Noct values remain rooted. */
	result = regex_replace_pinned(env, &pat, &str, &repl, &ret);

	/* Releases the replacement roots and propagates an unpin failure. */
	if (!noct_unpin_local(env, 4, &pat, &str, &repl, &ret))
		return false;

	/* Reports the replacement result. */
	return result;
}

/* Executes Regex.replaceAll() with its Noct values rooted. */
static bool
regex_replace_pinned(
	NoctEnv *env,
	NoctValue *pat,
	NoctValue *str,
	NoctValue *repl,
	NoctValue *ret)
{
	const char *pat_s;
	const char *str_s;
	const char *repl_s;
	struct regex_replace_state state;
	struct rx_match match;
	uint32_t next;
	int length;
	int replacement_length;
	int utf8_length;
	int from;
	int group_index;
	int group_start;
	int group_end;
	int i;
	int k;
	int search_result;
	bool result;

	/* Initializes all native ownership state. */
	memset(&state, 0, sizeof(state));

	/* Reads the pattern argument. */
	if (!noct_get_arg_check_string(env, 0, pat, &pat_s))
		return false;

	/* Reads the subject argument. */
	if (!noct_get_arg_check_string(env, 1, str, &str_s))
		return false;

	/* Reads the replacement argument. */
	if (!noct_get_arg_check_string(env, 2, repl, &repl_s))
		return false;

	/* Compiles the pattern and decodes the subject. */
	if (!regex_prepare(
		env,
		pat_s,
		str_s,
		false,
		&state.prog,
		&state.codepoint,
		&length)) {
		return false;
	}

	/* Allocates and decodes the replacement codepoints. */
	replacement_length = noct_rx_utf8_len(repl_s);
	state.replacement = noct_malloc(
		sizeof(uint32_t) * (size_t)(replacement_length + 1));
	if (state.replacement == NULL) {
		noct_out_of_memory(env);
		regex_replace_cleanup(&state);
		return false;
	}
	noct_rx_utf8_decode(repl_s, state.replacement);

	/* Rejects a subject extent that cannot seed the output buffer. */
	if (length > INT_MAX - 64) {
		noct_out_of_memory(env);
		regex_replace_cleanup(&state);
		return false;
	}

	/* Allocates the initial replacement output buffer. */
	state.output.capacity = length + 64;
	state.output.codepoint = noct_malloc(
		sizeof(uint32_t) * (size_t)state.output.capacity);
	if (state.output.codepoint == NULL) {
		noct_out_of_memory(env);
		regex_replace_cleanup(&state);
		return false;
	}

	/* Finds and replaces every non-overlapping match. */
	from = 0;
	while (from <= length) {
		/* Searches for the next match. */
		search_result = noct_rx_search(
			state.prog,
			state.codepoint,
			length,
			from,
			&match);
		if (search_result < 0) {
			noct_error(env, N_TR("Regex too complex."));
			regex_replace_cleanup(&state);
			return false;
		}

		/* Stops after the last match. */
		if (search_result == 0)
			break;

		/* Copies the text preceding this match. */
		for (i = from; i < match.start; i++) {
			/* Appends the next unchanged codepoint. */
			if (!regex_append(
				env,
				&state.output,
				state.codepoint[i])) {
				regex_replace_cleanup(&state);
				return false;
			}
		}

		/* Expands capture references in the replacement text. */
		for (i = 0; i < replacement_length; i++) {
			/* Interprets a possible replacement escape. */
			if (state.replacement[i] == '$' &&
			    i + 1 < replacement_length) {
				next = state.replacement[i + 1];

				/* Replaces a doubled dollar sign with one literal. */
				if (next == '$') {
					/* Appends the replacement dollar sign. */
					if (!regex_append(
						env,
						&state.output,
						'$')) {
						regex_replace_cleanup(&state);
						return false;
					}
					i++;
					continue;
				}

				/* Replaces a numeric reference with its capture. */
				if (next >= '0' && next <= '9') {
					group_index = (int)(next - '0');
					group_start = match.group_start[group_index];
					group_end = match.group_end[group_index];

					/* Copies a participating capture group. */
					if (group_start >= 0) {
						/* Appends every captured codepoint. */
						for (k = group_start;
						     k < group_end;
						     k++) {
							/* Appends the next capture codepoint. */
							if (!regex_append(
								env,
								&state.output,
								state.codepoint[k])) {
								regex_replace_cleanup(&state);
								return false;
							}
						}
					}
					i++;
					continue;
				}
			}

			/* Appends an ordinary replacement codepoint. */
			if (!regex_append(
				env,
				&state.output,
				state.replacement[i])) {
				regex_replace_cleanup(&state);
				return false;
			}
		}

		/* Advances beyond a nonempty or empty match. */
		if (match.end > match.start) {
			from = match.end;
		} else {
			/* Preserves one codepoint after an empty match. */
			if (match.end < length) {
				if (!regex_append(
					env,
					&state.output,
					state.codepoint[match.end])) {
					regex_replace_cleanup(&state);
					return false;
				}
			}
			from = match.end + 1;
		}
	}

	/* Copies the suffix following the last match. */
	for (i = from; i < length; i++) {
		/* Appends the next unchanged suffix codepoint. */
		if (!regex_append(
			env,
			&state.output,
			state.codepoint[i])) {
			regex_replace_cleanup(&state);
			return false;
		}
	}

	/* Rejects an output extent that cannot fit a UTF-8 allocation. */
	if (state.output.length > (INT_MAX - 1) / 4) {
		noct_out_of_memory(env);
		regex_replace_cleanup(&state);
		return false;
	}

	/* Allocates the UTF-8 result buffer. */
	state.utf8 = noct_malloc((size_t)(state.output.length * 4 + 1));
	if (state.utf8 == NULL) {
		noct_out_of_memory(env);
		regex_replace_cleanup(&state);
		return false;
	}

	/* Encodes the replacement result as UTF-8. */
	utf8_length = 0;
	for (i = 0; i < state.output.length; i++) {
		utf8_length += noct_rx_utf8_encode(
			state.output.codepoint[i],
			state.utf8 + utf8_length);
	}
	state.utf8[utf8_length] = '\0';

	/* Publishes the completed replacement string. */
	result = noct_set_return_make_string(env, ret, state.utf8);

	/* Releases every native replacement allocation. */
	regex_replace_cleanup(&state);

	/* Reports the return-value publication result. */
	return result;
}
