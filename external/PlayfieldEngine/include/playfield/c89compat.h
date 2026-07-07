/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * C89 compatibility header
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef PLAYFIELD_C89COMPAT_H
#define PLAYFIELD_C89COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Here, we define two macros that indicates the target architecture
 * and the byte order.
 *
 * | Macro               | Architecture           |
 * |---------------------|------------------------|
 * | PF_ARCH_X86         | x86 32-bit             |
 * | PF_ARCH_X86_64      | x86 64-bit             |
 * | PF_ARCH_ARM32       | ARMv7                  |
 * | PF_ARCH_ARM64       | Armv8 / Armv9          |
 * | PF_ARCH_PPC32       | PowerPC 32-bit         |
 * | PF_ARCH_PPC64       | PowerPC 64-bit / POWER |
 * | PF_ARCH_MIPS32      | MIPS 32-bit            |
 * | PF_ARCH_MIPS64      | MIPS 64-bit            |
 * | PF_ARCH_RISCV32     | RISC-V 32-bit          |
 * | PF_ARCH_RISCV64     | RISC-V 64-bit          |
 *
 * | Macro               | Byte-Order             |
 * |---------------------|------------------------|
 * | PF_ARCH_LE          | Little Endian          |
 * | PF_ARCH_BE          | Big Endian             |
 */

#if (defined(__i386__) && !defined(__x86_64__)) || defined(_M_IX86)

/* x86 */
#define PF_ARCH_X86
#define PF_ARCH_LE		/* Always LE */

#elif defined(__x86_64__) || defined(_M_X64)

/* x86_64 */
#define PF_ARCH_X86_64
#define PF_ARCH_LE		/* Always LE */

#elif defined(__aarch64__) || defined(_M_ARM64)

/* Arm64 */
#define PF_ARCH_ARM64
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PF_ARCH_BE		/* May be BE*/
#else
#define PF_ARCH_LE		/* Default, always LE on MSVC */
#endif

#elif defined(__arm__)

/* Arm32 */
#define PF_ARCH_ARM32
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PF_ARCH_BE		/* May be BE */
#else
#define PF_ARCH_LE		/* Default, always LE on MSVC */
#endif

#elif defined(_ARCH_PPC64)

/* PowerPC 64 / POWER */
#define PF_ARCH_PPC64
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PF_ARCH_BE		/* May be BE optionally */
#else
#define PF_ARCH_LE		/* Default, no MSVC support */
#endif

#elif defined(_ARCH_PPC)

/* PowerPC */
#define PF_ARCH_PPC32
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PF_ARCH_LE		/* May be LE optionally */
#else
#define PF_ARCH_BE		/* Default, always BE on MSVC */
#endif

#elif defined(__mips64) || defined(__mips64__)

/* MIPS64 */
#define PF_ARCH_MIPS64
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PF_ARCH_LE		/* May be LE */
#else
#define PF_ARCH_BE		/* Default, no MSVC support */
#endif

#elif defined(__mips__)

/* MIPS32 */
#define PF_ARCH_MIPS32
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PF_ARCH_LE		/* May be BE optionally */
#else
#define PF_ARCH_BE		/* Default, always BE on MSVC */
#endif

#elif defined(__riscv) && (__riscv_xlen == 64)

/* RISC-V 64bit */
#define PF_ARCH_RISCV64
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PF_ARCH_BE		/* May be BE optionally */
#else
#define PF_ARCH_LE		/* Default, no MSVC support yet */
#endif

#elif defined(__riscv) && (__riscv_xlen == 32)

/* RISC-V 32bit */
#define PF_ARCH_RISCV32
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PF_ARCH_BE		/* May be BE optionally */
#else
#define PF_ARCH_LE		/* Default, no MSVC support yet */
#endif

#endif

/*
 * Here, we define a macro that indicates the target platform and
 * its characteristics.
 *
 * | Macr  o               | Platform          | Description                        |
 * |-----------------------|-------------------|------------------------------------|
 * | PF_TARGET_WINDOWS     | Windows           | Win32 and Win64                    |
 * | PF_TARGET_MACOS       | macOS             | Mac OS 10.0+                       |
 * | PF_TARGET_LINUX       | Linux             | Excluding Android                  |
 * | PF_TARGET_IOS         | iOS               | iOS/iPadOS/visionOS/watchOS/tvOS   |
 * | PF_TARGET_ANDROID     | Android NDK       | Android 4.4+                       |
 * | PF_TARGET_OPENHARMONY | OpenHarmony SDK   | HarmonyOS NEXT and compatible OSes |
 * | PF_TARGET_WASM        | Wasm              | Emscripten                         |
 * | PF_TARGET_UNITY       | Unity             | Gaming Consoles                    |
 * | PF_TARGET_FREEBSD     | FreeBSD           | Excluding PS4/5                    |
 * | PF_TARGET_NETBSD      | NetBSD            |                                    |
 * | PF_TARGET_OPENBSD     | OpenBSD           |                                    |
 * | PF_TARGET_SOLARIS11   | Solaris 11        |                                    |
 * | PF_TARGET_SOLARIS10   | Solaris 10        |                                    |
 * | PF_TARGET_GENERICUNIX | Old UNIX          |                                    |
 * | PF_TARGET_HAIKU       | BeOS and Haiku    |                                    |
 * | PF_TARGET_PC98        | NEC PC-9801 DOS4G | Watcom                             |
 * | PF_TARGET_PCAT        | PC/AT DOS4G       | Watcom                             |
 *
 * In addition, the following is defined for Linux and *BSD (excluding Apple)
 *
 * | Macro              | Description             |
 * |--------------------|-------------------------|
 * | PF_TARGET_POSIX    | POSIX compliant systems |
 */

/* Windows */
#if defined(_WIN32) && !defined(PF_TARGET_UNITY)
#define PF_TARGET_WINDOWS
#endif

/* macOS / iOS */
#if !defined(PF_TARGET_UNITY)
#if defined(__GNUC__)
#if defined(__APPLE__) && __has_include(<TargetConditionals.h>)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define PF_TARGET_IOS
#elif TARGET_OS_OSX
#define PF_TARGET_MACOS
#endif
#endif
#endif
#endif

/* Linux */
#if defined(__linux) && \
    !defined(__ANDROID__) && \
    !defined(PF_TARGET_OPENHARMONY) && \
    !defined(PF_TARGET_UNITY)
#define PF_TARGET_LINUX
#ifndef PF_TARGET_POSIX
#define PF_TARGET_POSIX
#endif
#endif

/* FreeBSD */
#if defined(__FreeBSD__) && \
    !defined(PF_TARGET_UNITY)
#define PF_TARGET_FREEBSD
#ifndef PF_TARGET_POSIX
#define PF_TARGET_POSIX
#endif
#endif

/* NetBSD */
#if defined(__NetBSD__)
#define PF_TARGET_NETBSD
#ifndef PF_TARGET_POSIX
#define PF_TARGET_POSIX
#endif
#endif

/* OpenBSD */
#if defined(__OpenBSD__)
#define PF_TARGET_OPENBSD
#ifndef PF_TARGET_POSIX
#define PF_TARGET_POSIX
#endif
#endif

/* SunOS/Solaris */
#if defined(__sun) 
#if defined(__SunOS_5_11)
#define PF_TARGET_SOLARIS11
#else
#define PF_TARGET_SOLARIS10
#endif
#ifndef PF_TARGET_POSIX
#define PF_TARGET_POSIX
#endif
#endif

/* Android */
#if defined(__ANDROID__)
#define PF_TARGET_ANDROID
#endif

/* Wasm (Emscripten) */
#if defined(__EMSCRIPTEN__)
#define PF_TARGET_WASM
#endif

/* BeOS / Haiku */
#if defined(__BEOS__) || defined(__HAIKU__)
#define PF_TARGET_BEOS
#endif

/* Unity */
#if defined(PF_TARGET_UNITY)
#endif

/* OpenHarmony */
#if defined(PF_TARGET_OPENHARMONY)
#endif

/* PC98 */
#if defined(PF_TARGET_PC98)
#endif

/* PC/AT */
#if defined(PF_TARGET_PCAT)
#endif

/* Error: No target detected. */
#if !defined(PF_TARGET_WINDOWS) &&              \
    !defined(PF_TARGET_MACOS) &&                \
    !defined(PF_TARGET_MACOS7) &&                \
    !defined(PF_TARGET_LINUX) &&                \
    !defined(PF_TARGET_FREEBSD) &&              \
    !defined(PF_TARGET_NETBSD) &&               \
    !defined(PF_TARGET_OPENBSD) &&              \
    !defined(PF_TARGET_SOLARIS11) &&            \
    !defined(PF_TARGET_SOLARIS10) &&            \
    !defined(PF_TARGET_PIOSIX) &&               \
    !defined(PF_TARGET_IOS) &&                  \
    !defined(PF_TARGET_ANDROID) &&              \
    !defined(PF_TARGET_OPENHARMONY) &&          \
    !defined(PF_TARGET_WASM) &&                 \
    !defined(PF_TARGET_BEOS) &&                 \
    !defined(PF_TARGET_UNITY) &&                \
    !defined(PF_TARGET_PCAT) &&                 \
    !defined(PF_TARGET_PC98)
#error "No target detected."
#endif

/*
 * Definition of the bool type.
 */
#ifndef __cplusplus
#if defined(HAVE_STDBOOL_H)
#include <stdbool.h>
#elif defined(_MSC_VER) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#else
#if !defined(bool) && !defined(BOOL_DEF)
#define BOOL_DEF
typedef int bool;
enum { false = 0, true = 1 };
#endif
#endif
#endif

/*
 * Definitions of the intN_t and uintN_t types.
 */
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
#elif !defined(INTN_DEF)
#define INTN_DEF
#if defined(_MSC_VER)
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#endif
#endif

/*
 * Definition of the inline keyword.
 */
#if !defined(INLINE)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define INLINE			inline		/* C99 standard */
#elif defined(__GNUC__)
#define INLINE			__inline__	/* GCC extension */
#elif defined(__xlc__)
#define INLINE			__inline	/* IBM XLC extension */
#elif defined(_MSC_VER)
#define INLINE			__inline	/* MSVC extension */
#elif defined(__WATCOMC__)
#define INLINE			__inline	/* Watcom extension */
#pragma warning 202 9
#else
#define INLINE					/* Not supported */
#endif
#endif

/*
 * Definition of the restrict keyword.
 */
#if !defined(RESTRICT)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define RESTRICT		restrict	/* C99 standard */
#elif defined(__GNUC__)
#define RESTRICT		__restrict	/* GCC extension */
#elif defined(_MSC_VER)
#define RESTRICT		__restrict	/* MSVC extension */
#else
#define RESTRICT				/* Not supported */
#endif
#endif

/*
 * Definition of the CDECL keyword
 */
#if !defined(CDECL)
#if defined(PF_TARGET_PC98) || defined(PD_TARGET_PCAT)
#define CDECL __cdecl
#else
#define CDECL
#endif
#endif

/*
 * Definition of the import/export keyword.
 */
#if defined(PF_USE_DLL)
  #if defined(__GNUC__)
    #define PF_DLL		__attribute__((visibility("default")))
  #elif defined(_MSC_VER)
    #if defined(DLL_IMPL)
      #define PF_DLL		__declspec(dllexport)
    #else
      #define PF_DLL		__declspec(dllimport)
    #endif
  #endif
#else
  #define PF_DLL
#endif

/*
 * size_t
 */
#include <stddef.h>

/*
 * Suppress unused warnings.
 */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x)	(void)(x)
#endif

/*
 * UTF-8 string literal.
 */
#ifndef U8
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define U8(s)			u8##s		/* C23 standard */
#elif defined(__GNUC__) || defined(_MSC_VER)
#define U8(s)			u8##s		/* GCC and MSVC extensions */
#else
#define U8(s)			s		/* Not supported */
#endif
#endif

/*
 * UTF-32 character literal.
 *  U32C('A', 65) --> U'A' or 65
 */
#ifndef U32C
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define U32C(literal, unicode)	U##literal	/* C23 standard */
#elif defined(__GNUC__) || defined(_MSC_VER)
#define U32C(literal, unicode)	U##literal	/* GCC and MSVC extensions */
#else
#define U32C(literal, unicode)	(unicode)	/* C89 fallback */
#endif
#endif

/*
 * strdup()
 */
#if defined(_MSC_VER)
#if !defined(strdup)
#define strdup _strdup
#endif
#endif

/*
 * strcasecmp()
 */
#if defined(_MSC_VER)
#if !defined(strcasecmp)
#define strcasecmp _stricmp
#endif
#endif
#if defined(__WATCOMC__)
#if !defined(strcasecmp)
#define strcasecmp stricmp
#endif
#endif

/*
 * math
 */
#if defined(__WATCOMC__)
#if !defined(lroundf)
#define lroundf round
static INLINE double round(double x)
{
    if (x >= 0.0)
	    return (double)(int)(x + 0.5);
    else
	    return (double)(int)(x - 0.5);
}
#endif
#if !defined(floorf)
#define floorf floor
#endif
#if !defined(ceilf)
#define ceilf ceil
#endif
#if !defined(sqrtf)
#define sqrtf sqrt
#endif
#if !defined(sinf)
#define sinf sin
#endif
#if !defined(cosf)
#define cosf cos
#endif
#if !defined(tanf)
#define tanf tan
#endif
#endif

/*
 * Do not get warnings for usages of string.h functions.
 */
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

/*
 * Byteorder
 */
#if defined(PF_ARCH_LE)

static INLINE uint64_t pf_host_to_le_64(uint64_t d) {
	return d;
}
static INLINE uint32_t pf_host_to_le_32(uint32_t d) {
	return d;
}
static INLINE uint16_t pf_host_to_le_16(uint16_t d) {
	return d;
}
static INLINE uint64_t pf_le_to_host_64(uint64_t d) {
	return d;
}
static INLINE uint32_t pf_le_to_host_32(uint32_t d) {
	return d;
}
static INLINE uint16_t pf_le_to_host_16(uint16_t d) {
	return d;
}
static INLINE uint64_t pf_be_to_host_64(uint64_t d) {
	return ((d & 0xff) << 56) |
		(((d >> 8) & 0xff) << 48) |
		(((d >> 16) & 0xff) << 40) |
		(((d >> 24) & 0xff) << 32) |
		(((d >> 32) & 0xff) << 24) |
		(((d >> 40) & 0xff) << 16) |
		(((d >> 48) & 0xff) << 8) |
		(((d >> 56) & 0xff));
}
static INLINE uint32_t pf_be_to_host_32(uint32_t d) {
	return ((d & 0xff) << 24) |
		(((d >> 8) & 0xff) << 16) |
		(((d >> 16) & 0xff) << 8) |
		((d >> 24) & 0xff);
}
static INLINE uint16_t pf_be_to_host_16(uint16_t d) {
	return (uint16_t)(((uint32_t)(d & 0xff) << 8) |
		          ((d >> 8) & 0xff));
}
static INLINE uint64_t pf_host_to_be_64(uint64_t d) {
	return ((d & 0xff) << 56) |
		(((d >> 8) & 0xff) << 48) |
		(((d >> 16) & 0xff) << 40) |
		(((d >> 24) & 0xff) << 32) |
		(((d >> 32) & 0xff) << 24) |
		(((d >> 40) & 0xff) << 16) |
		(((d >> 48) & 0xff) << 8) |
		(((d >> 56) & 0xff));
}
static INLINE uint32_t pf_host_to_be_32(uint32_t d) {
	return ((d & 0xff) << 24) |
		(((d >> 8) & 0xff) << 16) |
		(((d >> 16) & 0xff) << 8) |
		((d >> 24) & 0xff);
}
static INLINE uint16_t pf_host_to_be_16(uint16_t d) {
	return (uint16_t)(((uint32_t)(d & 0xff) << 8) |
			  ((d >> 8) & 0xff));
}

#else

static INLINE uint64_t pf_host_to_be_64(uint64_t d) {
	return d;
}
static INLINE uint32_t pf_host_to_be_32(uint32_t d) {
	return d;
}
static INLINE uint16_t pf_host_to_be_16(uint16_t d) {
	return d;
}
static INLINE uint64_t pf_be_to_host_64(uint64_t d) {
	return d;
}
static INLINE uint32_t pf_be_to_host_32(uint32_t d) {
	return d;
}
static INLINE uint16_t pf_be_to_host_16(uint16_t d) {
	return d;
}
static INLINE uint64_t pf_le_to_host_64(uint64_t d) {
	return ((d & 0xff) << 56) |
		(((d >> 8) & 0xff) << 48) |
		(((d >> 16) & 0xff) << 40) |
		(((d >> 24) & 0xff) << 32) |
		(((d >> 32) & 0xff) << 24) |
		(((d >> 40) & 0xff) << 16) |
		(((d >> 48) & 0xff) << 8) |
		(((d >> 56) & 0xff));
}
static INLINE uint32_t pf_le_to_host_32(uint32_t d) {
	return ((d & 0xff) << 24) |
		(((d >> 8) & 0xff) << 16) |
		(((d >> 16) & 0xff) << 8) |
		((d >> 24) & 0xff);
}
static INLINE uint16_t pf_le_to_host_16(uint16_t d) {
	return ((d & 0xff) << 8) |
		((d >> 8) & 0xff);
}
static INLINE uint64_t pf_host_to_le_64(uint64_t d) {
	return ((d & 0xff) << 56) |
		(((d >> 8) & 0xff) << 48) |
		(((d >> 16) & 0xff) << 40) |
		(((d >> 24) & 0xff) << 32) |
		(((d >> 32) & 0xff) << 24) |
		(((d >> 40) & 0xff) << 16) |
		(((d >> 48) & 0xff) << 8) |
		(((d >> 56) & 0xff));
}
static INLINE uint32_t pf_host_to_le_32(uint32_t d) {
	return ((d & 0xff) << 24) |
		(((d >> 8) & 0xff) << 16) |
		(((d >> 16) & 0xff) << 8) |
		((d >> 24) & 0xff);
}
static INLINE uint16_t pf_host_to_le_16(uint16_t d) {
	return ((d & 0xff) << 8) |
		((d >> 8) & 0xff);
}

#endif

/*
 * Message Translation
 */
#if defined(PF_USE_TRANSLATION) && !defined(PF_USE_LIBINTL)

/* Translate messages. */
#define PF_TR(s)	pf_gettext(s)

/* Translator. */
const char *pf_gettext(const char *s);

#elif defined(PF_USE_TRANSLATION) && defined(PF_USE_LIBINTL)

#include <libintl.h>

#define PF_TR(s)	dgettext("libplayfield", s)

#else

/* No translation. */
#define PF_TR(s)	(s)

#endif

#ifdef __cplusplus
}
#endif

#endif
