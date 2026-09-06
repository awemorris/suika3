/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include <stddef.h>

static bool
partial_function(NoctEnv *env, void *userdata)
{
	(void)env;
	(void)userdata;
	return true;
}

NOCT_LIBRARY_EXPORT bool CDECL
noct_library_init(const NoctAPI *api, NoctEnv *env)
{
	size_t required_size;

	required_size = offsetof(NoctAPI, noct_register_cfunc_with_data) +
			sizeof(api->noct_register_cfunc_with_data);
	if (api == NULL || api->abi_version != NOCT_API_ABI_VERSION_1 ||
	    api->struct_size < required_size)
		return false;
	if (!api->noct_register_cfunc_with_data(env, "dynlib_partial", 0,
						NULL, partial_function,
						NULL, NULL))
		return false;
	api->noct_error(env, "intentional native-library initialization failure");
	return false;
}
