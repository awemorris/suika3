/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Hash
 */

#include "hash.h"

/*
 * FNV-1a
 */
uint32_t string_hash(const char *s)
{
	uint32_t hash = 2166136261u;
	while (*s) {
		hash ^= (uint8_t)*s++;
		hash *= 16777619u;
	}
	return hash;
}

/*
 * FNV-1a
 */
void string_hash_and_len(const char *s, uint32_t *hash, uint32_t *len)
{
	*len = 0;
	*hash = 2166136261u;
	while (*s) {
		*hash ^= (uint8_t)*s++;
		*hash *= 16777619u;
		*len = *len + 1;
	}
}
