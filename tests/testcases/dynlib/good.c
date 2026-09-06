/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct library_state {
	const NoctAPI *api;
	int value;
	char *finalizer_marker;
};

static bool
dynlib_value(NoctEnv *env, void *userdata)
{
	struct library_state *state;
	NoctValue ret;

	state = userdata;
	memset(&ret, 0, sizeof(ret));
	if (!state->api->noct_make_int(env, &ret, state->value))
		return false;
	return state->api->noct_set_return(env, &ret);
}

static void
library_finalizer(void *userdata)
{
	struct library_state *state;
	FILE *fp;

	state = userdata;
	if (state->finalizer_marker != NULL) {
		fp = fopen(state->finalizer_marker, "ab");
		if (fp != NULL) {
			fputs("finalized\n", fp);
			fclose(fp);
		}
	}
	free(state->finalizer_marker);
	free(state);
}

NOCT_LIBRARY_EXPORT bool CDECL
noct_library_init(const NoctAPI *api, NoctEnv *env)
{
	struct library_state *state;
	const char *marker;
	size_t required_size;

	required_size = offsetof(NoctAPI, noct_register_vm_finalizer) +
			sizeof(api->noct_register_vm_finalizer);
	if (api == NULL || api->abi_version != NOCT_API_ABI_VERSION_1 ||
	    api->struct_size < required_size ||
	    (api->feature_bits & (NOCT_API_FEATURE_CFUNC_DATA |
				  NOCT_API_FEATURE_VM_FINALIZER)) !=
			(NOCT_API_FEATURE_CFUNC_DATA |
			 NOCT_API_FEATURE_VM_FINALIZER))
		return false;
	state = calloc(1, sizeof(*state));
	if (state == NULL)
		return false;
	state->api = api;
	state->value = 42;
	marker = getenv("NOCT_DYNLIB_FINALIZER_MARKER");
	if (marker != NULL) {
		state->finalizer_marker = malloc(strlen(marker) + 1);
		if (state->finalizer_marker == NULL) {
			free(state);
			return false;
		}
		strcpy(state->finalizer_marker, marker);
	}
	if (!api->noct_register_cfunc_with_data(env, "dynlib_value", 0,
						NULL, dynlib_value, state, NULL) ||
	    !api->noct_register_vm_finalizer(env, library_finalizer, state)) {
		free(state->finalizer_marker);
		free(state);
		return false;
	}
	return true;
}
