/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Accelerator: Private Mutex
 */

#include "accel_mutex.h"

#include <assert.h>
#include <stdlib.h>

/*
 * Initializes an accelerator mutex.
 */
bool
accel_mutex_init(
	struct accel_mutex *mutex)
{
#if !defined(NOCT_TARGET_WINDOWS)
	pthread_mutexattr_t attr;
	int result;
	int destroy_result;

#endif
	assert(mutex != NULL);
	assert(!mutex->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	InitializeSRWLock(&mutex->native);
	mutex->initialized = true;
#else
	result = pthread_mutexattr_init(&attr);
	if (result != 0)
		return false;

	result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	if (result != 0) {
		(void)pthread_mutexattr_destroy(&attr);
		return false;
	}

	result = pthread_mutex_init(&mutex->native, &attr);
	destroy_result = pthread_mutexattr_destroy(&attr);
	if (result != 0)
		return false;
	if (destroy_result != 0) {
		(void)pthread_mutex_destroy(&mutex->native);
		return false;
	}

	mutex->initialized = true;
#endif

	return true;
}

/*
 * Locks an accelerator mutex.
 */
void
accel_mutex_lock(
	struct accel_mutex *mutex)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	assert(mutex != NULL);
	assert(mutex->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	AcquireSRWLockExclusive(&mutex->native);
#else
	result = pthread_mutex_lock(&mutex->native);
	if (result != 0)
		abort();
#endif
}

/*
 * Unlocks an accelerator mutex.
 */
void
accel_mutex_unlock(
	struct accel_mutex *mutex)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	assert(mutex != NULL);
	assert(mutex->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	ReleaseSRWLockExclusive(&mutex->native);
#else
	result = pthread_mutex_unlock(&mutex->native);
	if (result != 0)
		abort();
#endif
}

/*
 * Destroys an accelerator mutex.
 */
void
accel_mutex_destroy(
	struct accel_mutex *mutex)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	if (mutex == NULL)
		return;
	if (!mutex->initialized)
		return;

#if !defined(NOCT_TARGET_WINDOWS)
	result = pthread_mutex_destroy(&mutex->native);
	if (result != 0)
		abort();
#endif

	mutex->initialized = false;
}

/*
 * Initializes one accelerator condition variable.
 */
bool
accel_condition_init(
	struct accel_condition *condition)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	assert(condition != NULL);
	assert(!condition->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	InitializeConditionVariable(&condition->native);
	condition->initialized = true;
#else
	/* Initializes the native condition variable without transferred ownership. */
	result = pthread_cond_init(&condition->native, NULL);
	if (result != 0)
		return false;

	condition->initialized = true;
#endif

	return true;
}

/*
 * Waits atomically after releasing and before reacquiring one mutex.
 */
void
accel_condition_wait(
	struct accel_condition *condition,
	struct accel_mutex *mutex)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	assert(condition != NULL);
	assert(condition->initialized);
	assert(mutex != NULL);
	assert(mutex->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	/* Waits indefinitely while atomically releasing the exclusive SRW lock. */
	if (!SleepConditionVariableSRW(
		&condition->native,
		&mutex->native,
		INFINITE,
		0)) {
		abort();
	}
#else
	/* Waits indefinitely while atomically releasing the POSIX mutex. */
	result = pthread_cond_wait(&condition->native, &mutex->native);
	if (result != 0)
		abort();
#endif
}

/*
 * Wakes every waiter blocked on one accelerator condition.
 */
void
accel_condition_wake_all(
	struct accel_condition *condition)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	assert(condition != NULL);
	assert(condition->initialized);

#if defined(NOCT_TARGET_WINDOWS)
	WakeAllConditionVariable(&condition->native);
#else
	/* Wakes every POSIX waiter and treats synchronization failure as fatal. */
	result = pthread_cond_broadcast(&condition->native);
	if (result != 0)
		abort();
#endif
}

/*
 * Destroys one initialized accelerator condition variable.
 */
void
accel_condition_destroy(
	struct accel_condition *condition)
{
#if !defined(NOCT_TARGET_WINDOWS)
	int result;

#endif
	/* Accepts cleanup of an optional or uninitialized condition variable. */
	if (condition == NULL)
		return;
	if (!condition->initialized)
		return;

#if !defined(NOCT_TARGET_WINDOWS)
	result = pthread_cond_destroy(&condition->native);
	if (result != 0)
		abort();
#endif

	condition->initialized = false;
}
