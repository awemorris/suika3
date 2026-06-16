/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Atomic Operations
 */

#ifndef NOCT_ATOMIC_H
#define NOCT_ATOMIC_H

#define CPU_RELAX_BASE_INITIALIZER	((uint64_t)(int64_t)-1)

#if defined(__GNUC__)

static INLINE int atomic_load_relaxed_int(int *v)
{
	return __atomic_load_n(v, __ATOMIC_RELAXED);
}

static INLINE void *atomic_load_relaxed_ptr(void **p)
{
	return __atomic_load_n(p, __ATOMIC_RELAXED);
}

static INLINE int atomic_load_acquire_int(int *v)
{
	return __atomic_load_n(v, __ATOMIC_ACQUIRE);
}

static INLINE void *atomic_load_acquire_ptr(void **pp)
{
	return __atomic_load_n(pp, __ATOMIC_ACQUIRE);
}

static INLINE void atomic_store_release_int(int *p, int v)
{
	__atomic_store_n(p, v, __ATOMIC_RELEASE);
}

static INLINE void atomic_store_release_ptr(void **pp, void *v)
{
	__atomic_store_n(pp, v, __ATOMIC_RELEASE);
}

static INLINE int atomic_fetch_add_acquire_int(int *v, int add)
{
	int old = __atomic_fetch_add(v, add, __ATOMIC_ACQUIRE);
	return old;
}

static INLINE int atomic_fetch_add_release_int(int *v, int add)
{
	int old = __atomic_fetch_add(v, add, __ATOMIC_RELEASE);
	return old;
}

static INLINE int atomic_fetch_sub_acquire_int(int *v, int sub)
{
	int old = __atomic_fetch_sub(v, sub, __ATOMIC_ACQUIRE);
	return old;
}

static INLINE int atomic_fetch_sub_release_int(int *v, int sub)
{
	int old = __atomic_fetch_sub(v, sub, __ATOMIC_RELEASE);
	return old;
}

static INLINE void cpu_relax(uint64_t *t)
{
	UNUSED_PARAMETER(t);

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

static INLINE int atomic_load_relaxed_int(int* v)
{
	return *(volatile int*)v;
}

static INLINE void* atomic_load_relaxed_ptr(void** p)
{
	return *(void* volatile*)p;
}

static INLINE int atomic_load_acquire_int(int* v)
{
#if defined(_M_ARM64)
	return __iso_volatile_load32((const volatile __int32*)v);
#else
	int value = *(volatile int*)v;
	_ReadWriteBarrier();
	return value;
#endif
}

static INLINE void* atomic_load_acquire_ptr(void** pp)
{
#if defined(_M_ARM64)
	return (void*)__iso_volatile_load64((const volatile __int64*)pp);
#else
	void* value = *(void* volatile*)pp;
	_ReadWriteBarrier();
	return value;
#endif
}

static INLINE int atomic_load_release_int(int* v)
{
	_ReadWriteBarrier();
	return *(volatile int*)v;
}

static INLINE void* atomic_load_release_ptr(void** pp)
{
	_ReadWriteBarrier();
	return *(void* volatile*)pp;
}

static INLINE void atomic_store_release_int(int* p, int v)
{
#if defined(_M_ARM64)
	__iso_volatile_store32((volatile __int32*)p, v);
#else
	_ReadWriteBarrier();
	*(volatile int*)p = v;
#endif
}

static INLINE void atomic_store_release_ptr(void** pp, void* v)
{
#if defined(_M_ARM64)
	__iso_volatile_store64((volatile __int64*)pp, (__int64)v);
#else
	_ReadWriteBarrier();
	*(void* volatile*)pp = v;
#endif
}

static INLINE int atomic_fetch_add_acquire_int(int* v, int add)
{
	return _InterlockedExchangeAdd((volatile long*)v, add);
}

static INLINE int atomic_fetch_add_release_int(int* v, int add)
{
	return _InterlockedExchangeAdd((volatile long*)v, add);
}

static INLINE int atomic_fetch_sub_acquire_int(int* v, int sub)
{
	return _InterlockedExchangeAdd((volatile long*)v, -sub);
}

static INLINE int atomic_fetch_sub_release_int(int* v, int sub)
{
	return _InterlockedExchangeAdd((volatile long*)v, -sub);
}

static INLINE void cpu_relax(uint64_t* t)
{
	UNUSED_PARAMETER(t);
#if defined(_M_IX86) || defined(_M_X64)
	_mm_pause();
#elif defined(_M_ARM64)
	__yield();
#endif
}

#endif

#endif
