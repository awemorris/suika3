/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>

#include <stdio.h>
#include <stdlib.h>

struct owner {
	int *count;
	void *buffer;
};

static void
owner_finalizer(void *pointer)
{
	struct owner *owner = (struct owner *)pointer;
	(*owner->count)++;
	free(owner->buffer);
	free(owner);
}

static struct owner *
make_owner(int *count)
{
	struct owner *owner = malloc(sizeof(*owner));
	if (owner == NULL)
		return NULL;
	owner->buffer = malloc(64);
	if (owner->buffer == NULL) {
		free(owner);
		return NULL;
	}
	owner->count = count;
	return owner;
}

static int
test_vm_destroy(void)
{
	NoctVM *vm;
	NoctEnv *env;
	NoctValue packed;
	struct owner *owner;
	int count = 0;

	if (!noct_create_vm(&vm, &env, NULL))
		return 0;
	owner = make_owner(&count);
	if (owner == NULL || !noct_pin_local(env, 1, &packed) ||
	    !noct_make_packed(env, &packed, NOCT_PACKED_UINT8, 64, 64,
			      owner->buffer, owner, owner_finalizer))
		return 0;
	if (!noct_destroy_vm(vm))
		return 0;
	return count == 1;
}

static int
test_explicit_finalize(void)
{
	NoctVM *vm;
	NoctEnv *env;
	NoctValue packed;
	struct owner *owner;
	int count = 0;

	if (!noct_create_vm(&vm, &env, NULL))
		return 0;
	owner = make_owner(&count);
	if (owner == NULL || !noct_pin_local(env, 1, &packed) ||
	    !noct_make_packed(env, &packed, NOCT_PACKED_UINT8, 64, 64,
			      owner->buffer, owner, owner_finalizer) ||
	    !noct_finalize_packed(env, &packed) || count != 1 ||
	    !noct_destroy_vm(vm))
		return 0;
	return count == 1;
}

static int
test_dict_destroy(void)
{
	NoctVM *vm;
	NoctEnv *env;
	NoctValue dict, scratch;
	struct owner *owner;
	int count = 0;
	int i;
	char key[32];

	if (!noct_create_vm(&vm, &env, NULL))
		return 0;
	owner = make_owner(&count);
	if (owner == NULL || !noct_pin_local(env, 2, &dict, &scratch) ||
	    !noct_make_empty_dict(env, &dict) ||
	    !noct_set_dict_native_pointer(env, &dict, owner, owner_finalizer))
		return 0;
	for (i = 0; i < 100; i++) {
		(void)snprintf(key, sizeof(key), "key-%d", i);
		if (!noct_set_dict_elem_make_int(env, &dict, key, &scratch, i))
			return 0;
	}
	if (!noct_destroy_vm(vm))
		return 0;
	return count == 1;
}

int
main(void)
{
	if (!test_vm_destroy()) {
		fprintf(stderr, "owned Packed was not finalized at VM destroy\n");
		return 1;
	}
	if (!test_explicit_finalize()) {
		fprintf(stderr, "explicit Packed finalization was not exactly once\n");
		return 1;
	}
	if (!test_dict_destroy()) {
		fprintf(stderr, "Dict was not finalized at VM destroy\n");
		return 1;
	}
	puts("Packed/Dict finalizer tests passed.");
	return 0;
}
