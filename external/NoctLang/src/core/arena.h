/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Arena Allocator
 */

#ifndef NOCT_ARENA_H
#define NOCT_ARENA_H

#include <noct/noct.h>

#include <stdlib.h>
#include <string.h>

#define ARENA_ALIGN	(sizeof(void *))

struct arena_info {
	char *top;
	char *cur;
	size_t size;
};

/*
 * Initialize the arena allocator.
 */
static INLINE bool
arena_init(
	struct arena_info *arena,
	size_t size)
{
	arena->top = noct_calloc(1, size);
	if (arena->top == NULL)
		return false;
	arena->cur = arena->top;
	arena->size = size;
	return true;
}

/*
 * Cleanup the arena allocator.
 */
static INLINE void
arena_cleanup(
	struct arena_info *arena)
{
	free(arena->top);
}

/*
 * Allocate a block.
 */
static INLINE void *
arena_alloc(
	struct arena_info *arena,
	size_t size)
{
	void *p;
	size = (size + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
	if (arena->cur + sizeof(size_t) + size >= arena->top + arena->size) {
		return NULL;
	}
	p = arena->cur + sizeof(size_t);
	*(size_t *)arena->cur = size;
	arena->cur += sizeof(size_t) + size;
	return p;
}

/*
 * Get the size of a block.
 */
static INLINE size_t
arena_get_block_size(
	const void *p)
{
	return *(size_t *)((const char *)p - sizeof(size_t));
}

/*
 * Unwind the arena.
 */
static INLINE void
arena_unwind(
	struct arena_info *arena)
{
	arena->cur = arena->top;
	memset(arena->top, 0, arena->size);
}

#endif /* NOCT_ARENA_H */
