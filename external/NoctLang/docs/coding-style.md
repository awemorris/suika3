C Coding Style
==============

## 1. Purpose

A C source is written for human readers and debuggers first.  Compact
expressions are not a goal.  A reader should be able to follow a
function from top to bottom, understand each processing phase from its
surrounding whitespace and comments, and stop a debugger on each
meaningful decision.

ANSI C-compatible declarations first, small guarded steps, explicit error exits, and
semantic paragraphs separated by blank lines.  It still contains
historical exceptions.  Those exceptions are not precedents; the rules
below are the canonical form for new and refactored code.

Style-only changes must preserve evaluation order, ownership,
lifetime, error reporting, and observable behavior.

The language baseline is ANSI C (C89/C90).  Do not use ordinary C99-or-later
syntax where ANSI C expresses the same program.  Implementation-reserved
extensions that are reliably supported by every configured compiler may be
used when the platform needs them.  Examples include `__inline`, `__attribute__`,
and `__asm__`.  Spell such extensions explicitly with their reserved names;
an extension is not permission to use unrelated newer-language syntax.

## 2. File organization

C source files use the following basic order after the include directives:

1. file-wide macros and constants
2. enums
3. structures and other type definitions
4. file-scope variables
5. forward declarations
6. public function definitions
7. static function definitions

An implementation-local macro may remain next to the `static inline` functions
that use it when moving it to the top would make the implementation harder to
read.  This is a narrow exception for macros that are inseparable from the
local implementation.

Put the Emacs modeline on the first physical line of the file.  Follow it with
one blank line and then the copyright header.  Indentation uses tabs with a tab
width of eight.

## 3. Forward declarations and function definitions

Every non-`static inline` static function has a forward declaration.  A forward
declaration is written on one physical line even when it is long:

```c
static bool parse_index(struct parse_ctx *ctx, const struct hir_expr *expr, int *offset);
```

Function definitions put the return type, function name, and every argument on
separate lines:

```c
static bool
parse_index(
	struct parse_ctx *ctx,
	const struct hir_expr *expr,
	int *offset)
{
	/* ... */
}
```

Public function definitions appear before static function definitions.  Put a
multiple-line comment before every public function definition.  Its first
sentence is a one-line verb-and-object description.  Additional explanation
may follow, but the comment does not cite design-document definition numbers.

```c
/*
 * Unrolls eligible scalar Packed loops.
 *
 * The transformed HIR remains valid for every backend.
 */
bool
hir_opt_unroll_func(
	struct hir_block *func_block)
```

A short one-line header comment is required for any static function.

## 4. Local declarations

Use ANSI C declaration placement.  Declare every automatic variable at the
beginning of its function, before the first executable statement.  ANSI C
initializers are permitted, including aggregate and `static const` table
initializers.  Keep an initializer when splitting it into later assignments
would change constness, aggregate representation, evaluation order, or
readability.  Prefer a separate assignment for runtime initialization that
forms a semantic paragraph:

```c
int count;

count = 0;
```

Move variables needed only by a nested control-flow path to the function
declaration group as well.  Choose a name that remains unambiguous at function
scope.  Do not declare a variable in a `for` initializer:

```c
int i;

/* Processes every input item. */
for (i = 0; i < item_count; i++)
	process_item(&items[i]);
```

Do not introduce a standalone compound statement merely to restrict a
variable's scope.  Braced bodies owned by a function, `if`, `else`, `for`,
`while`, `do`, or `switch` are normal control-flow bodies and are not standalone
scope blocks.  If moving declarations to function scope makes a function hard
to understand or creates excessive nesting, split the function along a
semantic boundary instead of adding a scope-only block.

Put exactly one blank line between the function declaration group and the first
executable statement.  Then write
`UNUSED_PARAMETER()` entries together, one per line and in argument order.  Put
assertions below them after another blank line:

```c
struct item *item;

UNUSED_PARAMETER(env);
UNUSED_PARAMETER(flags);

assert(source != NULL);
assert(result != NULL);
```

## 5. Semantic paragraphs and whitespace

Treat consecutive assignments and function calls that perform one operation as
a semantic paragraph.  Precede every semantic paragraph with a comment that
explains what the paragraph does, normally as a one-line verb-and-object English
sentence.  Use a natural multi-line explanation when one sentence cannot state
the purpose clearly.  Allocation and its check, initialization of one object,
traversal of one collection, and publication of a result are separate semantic
paragraphs.  Put one blank line between semantic paragraphs and before each
paragraph comment.

A purpose comment introduces the entire operation that follows it, including
the assignments that prepare a loop or decision.  Put the comment before those
assignments, not between the preparation and the control statement.  Keep the
preparation and the control statement adjacent when they are one operation:

```c
/* Processes each requested archive option. */
options = argv[1][0] == '-' ? argv[1] + 1 : argv[1];
for (; *options; options++) {
	/* ... */
}
```

Do not leave an uncommented initialization paragraph immediately followed by a
comment that describes only the loop which consumes it.

An `if` that immediately checks an operation belongs to the same semantic
paragraph.  Keep its purpose comment above the operation and do not put a blank
line between the operation and its check:

```c
/* Allocates the result item. */
item = hir_malloc(sizeof(*item));
if (item == NULL) {
	hir_out_of_memory();
	return NULL;
}

memset(item, 0, sizeof(*item));
item->type = HIR_ITEM_VALUE;
```

Precede every `if` with a purpose comment and a blank line.  One comment may
introduce a complete `if`-`else if`-`else` chain; do not insert blank lines or
repeated comments inside that chain.  An immediate result check uses the
semantic paragraph comment above the checked operation instead of a redundant
second comment.

The blank line before a paragraph comment is mandatory.  A comment must not be
attached to the preceding statement merely because it is adjacent to the
`if`, loop, `switch`, return, or assignment that it describes.  The only normal
exceptions are the first paragraph after an opening brace, a continued comment
block, and a comment directly following a `case` or other label where inserting
a blank line would not create a paragraph boundary.

Place a blank line before the purpose comment that introduces a `switch`,
`for`, or `while` block.  Keep the purpose comment adjacent to the paragraph;
loop-preparation assignments may appear between that comment and the loop as
described below.  The same blank-line rule applies before comments that
introduce returns and other semantic paragraphs, except at the beginning of a
control-flow body where the opening brace already provides the visual boundary.

## 6. Debugger-friendly control flow

Prefer guard clauses and explicit decisions to compound Boolean returns.  Each
meaningful condition should provide a debugger stop point.

Do not write:

```c
return is_left(expr) || is_right(expr);
```

Write:

```c
if (is_left(expr))
	return true;
if (is_right(expr))
	return true;

return false;
```

Do not place several fallible or significant function calls in one condition.
Evaluate them in short-circuit order and check them one at a time:

```c
if (!check_expr(ctx, left))
	return false;
if (!check_expr(ctx, right))
	return false;

return true;
```

When a condition contains three or more clauses, or mixes nested `&&` and
`||`, put the clauses on separate lines and use parentheses and indentation to
expose its structure:

```c
if (expr == NULL ||
    (expr->type != HIR_EXPR_PLUS &&
     expr->type != HIR_EXPR_MINUS))
	return false;
```

A short condition with two side-effect-free clauses may remain on one line when
their relationship is immediately clear.  The rule against combining
significant calls still applies.

Preserve the original short-circuit evaluation order when decomposing a
condition.

Do not use `goto`, including for error cleanup.  Keep ownership local, return
immediately after a failed operation, and release already acquired resources
in reverse order on that path.  When this would duplicate a large cleanup
sequence or obscure the main operation, extract a small cleanup helper or
split the function at an ownership boundary.

## 7. Loops and switches

Every `for` and `while` loop belongs to a paragraph introduced by a one-line
comment that states what the loop does.  The comment is immediately before the
loop when no preparation is required.  When assignments prepare the traversal,
put the comment before those assignments and keep the assignments adjacent to
the loop.  The comment describes intent rather than restating the syntax:

```c
/* Skip redundant parenthesized expressions. */
while (expr != NULL && expr->type == HIR_EXPR_PAR)
	expr = expr->val.unary.expr;
```

For a non-standard `for` loop, put each of its three control expressions on a
separate line:

```c
/* Append every remaining source statement. */
for (;
     source != NULL;
     source = source->next) {
	/* ... */
}
```

A `for` initializer is an expression, never a declaration.  Its control
variable is declared in the function's leading declaration group.

Put a one-line purpose comment immediately before every `switch`, just as for a
loop.  Separate the comment and switch from the preceding semantic paragraph
with a blank line.  A `case` label occupies its own line; never put a return,
assignment, or other statement on the same line as the label.

## 8. Function calls and control-statement bodies

Keep a simple call on one line.  If a call must be split, put the callee and
opening parenthesis on the first line and one argument on each following line:

```c
result = build_binary(
	HIR_EXPR_PLUS,
	left,
	right);
```

If the only statement controlled by `if`, `else`, `for`, or `while` spans
multiple physical lines, enclose that statement in braces:

```c
if (offset > 0) {
	return build_binary(
		HIR_EXPR_PLUS,
		counter,
		value);
}
```

Keep the two sides of an `if`-`else` decision structurally symmetric.  If the
`if` body or the terminal `else` body uses braces, the other body uses braces
too.  An `else if` remains part of the same decision chain and is not treated as
a terminal unbraced `else`:

```c
if (ready) {
	start_request();
} else {
	defer_request();
}
```

Use braces around a `for` body whose controlled statement is an `if`, even when
that `if` is grammatically one statement.  Also use braces around a `for` or
`while` body whenever its controlled statement occupies more than one physical
line.  Comments do not make a statement compound, but a comment plus a
controlled statement still belongs inside the loop's braces.

The first content line in an `if`, `else`, `for`, or `while` block is indented
exactly one tab beyond its controlling statement.  Do not add an extra
indentation level merely because the first content is a comment or because an
unbraced statement was converted to a block.  A block starts with content, not
an empty line; do not leave a blank line immediately after its opening brace.

Do not build deeply nested call expressions as arguments.  Store complex
intermediate results in named temporary variables, check each fallible result,
and then pass the completed values to the outer call.

## 9. Allocation and object initialization

Allocate and check one object at a time.  Do not perform several allocations
and then check them together:

```c
expr = hir_malloc(sizeof(*expr));
if (expr == NULL) {
	hir_out_of_memory();
	return NULL;
}

term = hir_malloc(sizeof(*term));
if (term == NULL) {
	hir_out_of_memory();
	return NULL;
}
```

Treat allocation plus its immediate check as one block.  Treat initialization
of one structure as another block.  Initialize one structure completely, then
separate it with a blank line before initializing another structure.

Check each fallible return value immediately and individually.  Do not execute
several fallible operations and test all their results afterward.

## 10. Comments

Use comments to expose why a processing phase or loop exists.  Describe a
meaningful group of assignments or calls, not each obvious individual
statement.  Prefer a verb-and-object sentence such as `Initializes the output
record.` or `Publishes the completed request.`.  Put a blank line before the
comment and keep the comment adjacent to the paragraph it introduces.

Use this form for a comment that spans multiple lines:

```c
/*
 * The scalar cache cannot keep four division lanes live cheaply.
 * Keep this guard until the allocator exposes enough parallelism.
 */
```

Do not start a multi-line comment with prose on the opening `/*` line.

## 11. Returns

Precede a return with a comment that explains the returned result or the reason
for the exit.  Put a blank line before that comment and keep the comment adjacent
to the return.  An early guard return remains adjacent to its controlling
condition and is explained by the condition's purpose comment instead of a
redundant return comment.

Do not directly return the result of a meaningful function call.  Store the
result or test it explicitly so that success and failure each provide a debugger
stop point and the function's own return convention is visible.  Preserve the
callee's actual success convention; do not assume that zero, nonzero, true, or
false means success without checking its contract:

```c
/* Submits the completed job. */
status = submit_job(input, when, queue);

/* Propagates a failed submission. */
if (status != 0)
	return status;

/* Reports a successful submission. */
return 0;
```

Literal, variable, field, and simple arithmetic returns may remain direct, with
their required result comment.  Early guard returns remain governed by the
preceding decision comment.

## 12. Test controls and environment variables

Do not put test-only environment-variable switches in production code.  The
implementation follows its default production behavior, selected by ordinary
configuration or compile-time architecture conditions.

An environment variable that implements an intentional diagnostic interface,
such as a supported debug trace, is not a test-only behavior switch and may
remain.

Tests must exercise the default production path.  They must not require a
hidden environment variable to enable or disable the implementation under
test.

## 13. Copyright header

Source code files start with the Emacs modeline, followed by one blank line.
The copyright header then begins with:

```
/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

```

Then add the file explanation:

```
/*
 * The cmp progmra.
 */

```

## 14. Review checklist

Before finishing a C-source change, verify that:

- file sections and public/static function order are correct
- every normal static function has a one-line forward declaration
- every public function definition has a verb-and-object comment
- every local declaration is in the function-leading ANSI C declaration group,
  no `for` initializer declares a variable, and no standalone scope-only block
  exists
- the declaration group, first statement, and assertions follow the required
  order and spacing
- compound decisions and fallible calls are individually debuggable
- no `goto` statement is used for cleanup or control flow
- every loop and `switch` has an immediately preceding intent comment
- split calls use one argument per line and split controlled statements use
  braces
- `if`/terminal-`else` braces are symmetric, loop bodies containing `if` or
  multi-line statements are braced, and block-entry indentation is exactly one
  level with no leading blank line
- allocations, checks, and per-object initialization form clear blocks
- every semantic paragraph, decision, loop, and return has an adjacent purpose
  comment and the expected blank lines
- every loop-preparation assignment is covered by the loop paragraph comment,
  every comment starts a new paragraph where required, and meaningful function
  results are not returned directly
- no test-only environment switch controls production behavior
- the build, focused tests, and `git diff --check` pass
