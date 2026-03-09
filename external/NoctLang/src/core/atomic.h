/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Atomic Operations
 */

#ifndef NOCT_ATOMIC_H
#define NOCT_ATOMIC_H

#if defined(__GNUC__)

static INLINE void *atomic_load_relaxed_ptr(void **p)
{
	return __atomic_load_n(p, __ATOMIC_RELAXED);
}

static INLINE int atomic_load_acquire(int *v)
{
	return __atomic_load_n(v, __ATOMIC_ACQUIRE);
}

static INLINE void *atomic_load_acquire_ptr(void **pp)
{
	return __atomic_load_n(pp, __ATOMIC_ACQUIRE);
}

static INLINE void atomic_store_release_ptr(void **pp, void *v)
{
	__atomic_store_n(pp, v, __ATOMIC_RELEASE);
}

static INLINE int atomic_fetch_add_acquire(int *v, int add)
{
	unsigned old = __atomic_fetch_add(v, add, __ATOMIC_ACQUIRE);
	return old;
}

static INLINE int atomic_fetch_sub_release(int *v, int sub)
{
	unsigned old = __atomic_fetch_sub(v, sub, __ATOMIC_RELEASE);
	return old;
}

static INLINE void cpu_relax(void)
{
#if defined(__i386__) || defined(__x86_64__)
	// PAUSE
	__asm__ __volatile__("pause");
#elif defined(__arm__) || defined(__aarch64__)
	// YIELD
	__asm__ __volatile__("yield");
#elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
	// Spin Loop Hint
	__asm__ __volatile__("or 27,27,27");
#endif
}

#elif defined(_MSC_VER)

#include <intrin.h>

static INLINE void *atomic_load_relaxed_ptr(void **p)
{
	MemoryBarrier();
	return __atomic_load_n(p, __ATOMIC_RELAXED);
}

static INLINE int atomic_load_acquire(int *v)
{
	return _InterlockedExchangeAdd((volatile long *)v, 0);
}

static INLINE void *atomic_load_acquire_ptr(void **pp)
{
	return (void *)_InterlockedExchangeAdd((volatile long *)pp, 0);
}

static INLINE void atomic_store_release_ptr(void **p, void *v)
{
	_InterlockedExchangePointer((void* volatile *)p, v);
}

static INLINE int atomic_fetch_add_acquire(int *v, int add)
{
	return _InterlockedExchangeAdd((volatile long *)v, 1);
}

static INLINE int atomic_fetch_sub_release(int *v, int sub)
{
	return _InterlockedExchangeAdd((volatile long *)v, -1);
}

#if defined(_M_IX86) || defined(_M_X64)
#include <immintrin.h>
static INLINE void cpu_relax(void)
{
	_mm_pause();
}
#elif defined(_M_ARM64)
static INLINE void cpu_relax(void)
{
	// yield
	__emit(0xD503207F);
}
#endif

#endif

#endif
