/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Helper for CPUID instruction
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "stratohal/platform.h"

#include <intrin.h>

#if defined(USE_SSE_DISPATCH)

bool is_avx2_available;
bool is_avx_available;
bool is_sse42_available;
bool is_sse4_available;
bool is_sse3_available;
bool is_sse2_available;
bool is_sse_available;

void check_cpuid(void)
{
#if defined(__GNUC__)
	/*
	 * GCC
	 */
	is_avx2_available = __builtin_cpu_supports("avx2");
	is_avx_available = __builtin_cpu_supports("avx");
	is_sse42_available = __builtin_cpu_supports("sse4.2");
	is_sse4_available = __builtin_cpu_supports("sse4.1");
	is_sse3_available = __builtin_cpu_supports("sse3");
	is_sse2_available = __builtin_cpu_supports("sse2");
	is_sse_available = __builtin_cpu_supports("sse");
#else
	/*
	 * MSVC
	 */
	int cpuinfo[4] = {0};
	__cpuid(cpuinfo, 1);

	/* SSE */
	is_sse_available = (cpuinfo[3] & (1 << 25)) != 0;

	/* SSE2 */
	is_sse2_available = (cpuinfo[3] & (1 << 26)) != 0;

	/* MSVC doesn't generate SSE4.2/SSE4/SSE3/SSE2 code. */
#if 0
	/* SSE3 */
	is_sse3_available = (cpuInfo[2] & 1) != 0;

	/* SSE4 */
	is_sse4_available = (cpuinfo[2] & (1 << 19)) != 0;

	/* SSE4.2 */
	__cpuid(cpuinfo, 1);
	is_sse42_available = (cpuInfo[2] & (1 << 20)) != 0;
#endif

	/* AVX/AXV2 */
	bool os_uses_xsave = (cpuinfo[2] & (1 << 27)) != 0;
	if (os_uses_xsave) {
		bool os_supports_ymm =  (_xgetbv(0) & 0x6) == 0x6;
		if (os_supports_ymm) {
			/* AVX */
			bool cpu_supports_avx = (cpuinfo[2] & (1 << 28)) != 0;
			if (cpu_supports_avx)
				is_avx_available = true;

			/* AVX2 */
			__cpuid(cpuinfo, 0);
			if (cpuinfo[0] >= 7) {
				__cpuidex(cpuinfo, 7, 0);
				bool cpu_supports_avx2 = (cpuinfo[1] & (1 << 5)) != 0;
				if (cpu_supports_avx2)
					is_avx2_available = true;
			}
		}
	}
#endif
}

#endif
