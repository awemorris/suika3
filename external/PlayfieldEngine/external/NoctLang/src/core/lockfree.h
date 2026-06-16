/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Lock-Free Macros
 */

#include <noct/noct.h>
#include "runtime.h"
#include "gc.h"

/*
 * Acquire an array for a read access.
 */
static INLINE void
rt_acquire_array_read(
	struct rt_env *env,
	struct rt_value *val,
	struct rt_array *real_arr)
{
	struct rt_array *next;
	int old;
	uint64_t relax_base_time;

retry:
	/* If there is a GC request. */
	if (atomic_load_acquire(&env->vm->gc_stw_counter) > 0) {
		/* Make this thread non-inflight. */
		atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);

		/* Wait for GC to finish. */
		relax_base_time = cpu_relax(0);
		while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0) {
			/* Do adaptive yield and sleep using the spin time. */
			cpu_relax(relax_base_time);
		}

		/* Make this thread in-flight. */
		atomic_fetch_add_release(&env->vm->in_flight_counter, 1);
	}



	/*
	 * Read "val->val.arr" atomically because the array may be moved
	 * by the Copy-GC or Compaction-GC in the previous iteration of
	 * this while loop.
	 */
	real_arr = atomic_load_acquire_ptr((void**)&(val->val.arr));

	/*
	 * Get "newer" forwardings if exist. Note that array expansion
	 * is done by creating a new array and linking it to "newer"
	 * field in the old array. This is similar to RCU mechanism.
	 */
	while ((next = atomic_load_acquire_ptr((void **)&real_arr->newer)) != NULL) {
		real_arr = next;

		/* If there is a writer thread, retry. */
		if (atomic_load_acquire(&real_arr->write_counter) > 0)
			goto retry;
	}

	/* Acquire. */
	atomic_fetch_add_acquire(&real_arr->read_counter, 1);

	/* If the array was expanded by a race writer thread. */
	if (atomic_load_acquire_ptr((void **)&real_arr->newer) != NULL) {
		/* Release and retry. */
		atomic_fetch_sub_acquire(&real_arr->counter, 1);
		goto retry;
	}

	/* If  */

	return real_arr;
}

/*
 * Acquire an array for a write access.
 */
static INLINE void
rt_acquire_array_write(
	struct rt_env *env,
	struct rt_value *val,
	struct rt_array *real_arr)
{
	struct rt_array *next;
	int old;

retry:
	while (1) {
		/*
		 * Read "val->val.arr" atomically because the array may be moved
		 * by the Copy-GC or Compaction-GC in the previous iteration of
		 * this while loop.
		 */
		real_arr = atomic_load_acquire_ptr((void**)&(val->val.arr));

		/*
		 * Get "newer" forwardings if exist. Note that array expansion
		 * is done by creating a new array and linking it to "newer"
		 * field in the old array. This is similar to RCU mechanism.
		 */
		while ((next = atomic_load_acquire_ptr((void **)&real_arr->newer)) != NULL) {
			real_arr = next;

			/* If there is reader or write */
			if (atomic_load_acquire(&real_arr->counter) > 0)
				goto retry;

			    (void **)&real_arr->newer) == NULL) {
		atomic_fetch_add_acquire(&real_arr->counter, 1);

		/* If the array was not expanded by a race thread. */
		}

		/*
		 * Try acquire the array by atomic read-add-write to the access
		 * exclusion counter of the array.
		 */
		old = atomic_fetch_add_acquire(&real_arr->counter, 1);
		if (old == 0) {
			/*
			 * Succeeded to increment the counter. Also check if the
			 * array is not expanded before the last capture.
			 */
			if (atomic_load_acquire_ptr((void **)&real_arr->newer) == NULL) {
				/* Succeeded. */
				break;
			}
		}

		/* Failed, release the array. */
		atomic_fetch_sub_release(&real_arr->counter, 1);

		/* Make this thread non-inflight. */
		atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);

		/*
		 * If another thread is waiting for all other threads to reach
		 * GC-safe points. Note that "another thread" includes cases
		 * where a race thread on the array wanted to expand the array
		 * and the allocation of the new array triggered a
		 * GC. Additionally, "another thread" also naturally includes
		 * cases where threads unrelated to the array randomly trigger a
		 * GC.
		 */
		if (atomic_load_acquire(&env->vm->gc_stw_counter) > 0) {
			/* Wait for GC to finish. */
			while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)
				cpu_relax();

		}

		/* Make this thread in-flight. */
		atomic_fetch_add_release(&env->vm->in_flight_counter, 1);
	}

	return real_arr;
}

/*
 * Acquire two arrays for the access rights.
 */
static INLINE void
rt_acquire_array_dual(
	struct rt_env *env,
	struct rt_value *val1,
	struct rt_value *val2,
	struct rt_array *real_arr1,
	struct rt_array *real_arr2)
{
	struct rt_array *next;
	int old;

	while (1) {
		/*
		 * Read "val1->val.arr" atomically because the array may be
		 * moved by the Copy-GC or Compaction-GC in the previous
		 * iteration of this while loop.
		 */
		real_arr1 = atomic_load_acquire_ptr((void**)&(val->val1.arr));

		/*
		 * Get "newer" forwardings if exist. Note that array expansion
		 * is done by creating a new array and linking it to "newer"
		 * field in the old array. This is similar to RCU mechanism.
		 */
		while ((next = atomic_load_acquire_ptr((void **)&real_arr1->newer)) != NULL)
			real_arr1 = next;

		/*
		 * Try acquire the array by atomic read-add-write to the access
		 * exclusion counter of the array.
		 */
		old = atomic_fetch_add_acquire(&real_arr1->counter, 1);
		if (old == 0) {
			/*
			 * Succeeded to increment the counter. Also check if the
			 * array was not expanded after the last capture.
			 */
			if (atomic_load_acquire_ptr((void **)&real_arr1->newer) == NULL) {
				/*
				 * Suceeded, val1 was acquired. Now acquire val2
				 * by the same sequence.
				 */
				real_arr2 = atomic_load_acquire_ptr((void**)&(val->val2.arr));
				while ((next = atomic_load_acquire_ptr((void **)&real_arr2->newer)) != NULL)
					real_arr2 = next;
				old = atomic_fetch_add_acquire(&real_arr2->counter, 1);
				if (old == 0) {
					if (atomic_load_acquire_ptr((void **)&real_arr2->newer) == NULL) {
						/* Succeeded, both val1 and val2 acquired. */
						break;
					}
				}

				/* Failed, release the val2 array. */
				atomic_fetch_sub_release(&real_arr2->counter, 1);
			}
		}

		/* Failed, release the val1 array. */
		atomic_fetch_sub_release(&real_arr1->counter, 1);

		/* Make this thread non-inflight. */
		atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);

		/*
		 * If another thread is waiting for all other threads to reach
		 * GC-safe points. Note that "another thread" includes cases
		 * where a race thread on one of the arrays wanted to expand the
		 * array and the allocation of the new array triggered a
		 * GC. Additionally, "another thread" also naturally includes
		 * cases where threads unrelated to the arrayes randomly trigger
		 * a GC.
		 */
		if (atomic_load_acquire(&env->vm->gc_stw_counter) > 0) {
			/* Wait for GC to finish. */
			while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)
				cpu_relax();
		}

		/* Make this thread in-flight. */
		atomic_fetch_add_release(&env->vm->in_flight_counter, 1);
	}

	return real_arr;
}

/*
 * Get the latest "struct rt_array *" pointer after a possible GC execution.
 */
static INLINE struct rt_array *
rt_recapture_array(
	struct rt_env *env,
	struct rt_value *val)
{
	struct rt_array *real_arr;

	/*
	 * Read "val->val.arr" atomically because the array may be moved by the
	 * Copy-GC or Compaction-GC.
	 */
	real_arr = atomic_load_acquire_ptr((void**)&(val->val.arr));

	/*
	 * Get "newer" forwardings if exist. Note that array expansion is done
	 * by creating a new array and linking it to "newer" field in the old
	 * array. This is similar to RCU mechanism.
	 */
	while (atomic_load_acquire_ptr((void **)&real_arr->newer) != NULL)
		real_arr = atomic_load_acquire_ptr((void **)&real_arr->newer);

	return real_arr;
}

/*
 * Release an array.
 */
static INLINE void
rt_release_array(
	struct rt_array *arr)
{
	atomic_fetch_sub_release(&arr->counter, 1);
}

/*
 * Make a GC safe point.
 *
 * Note that:
 * - "in-flight" means a thread is executing and not waiting at a
 *   GC-safe point.
 * - "non-inflight" means a thread is waiting for another thread to finish GC
 *   at a GC-safe point.
 */
static INLINE void
rt_enter_gc_safe_point(
	struct rt_env *env)
{
	/*
	 * If another thread is waiting for all other threads to reach GC-safe
	 * points.
	 */
	if (atomic_load_acquire(&env->vm->gc_stw_counter) > 0) {
		/* Make this thread non-inflight. */
		atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);

		/* Wait for GC to finish. */
		while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)
			cpu_relax();

		/* Make this thread inflight again. */
		atomic_fetch_add_acquire(&env->vm->in_flight_counter, 1);
	}
}

/*
 * Run a GC function in a multithreaded manner.
 */
static bool
rt_gc_multithread_call_wrapper(
	struct rt_env *env,
	bool (*gc)(struct rt_env *env))
{
	bool is_executor = false;
	bool result;

	/* If this is not a recursive GC call. */
	if (atomic_load_acquire(&env->gc_in_progress_counter) == 0) {
		/* Make this thread non-inflight. (Enter a GC safe point.) */
		atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);

		/*
		 * Wait for all other threads entering GC safe points,
		 * and this thread getting the right to enter GC section.
		 */
		while (1) {
			/* Try acquire the GC right. */
			int old = atomic_fetch_add_acquire(&env->vm->gc_stw_counter, 1);
			if (old == 0) {
				/* Wait for all other threads get non-inflight. */
				while (atomic_load_acquire(&env->vm->in_flight_counter) > 0)
					cpu_relax();

				/* Succeeded, entering the GC section. */
				is_executor = true;
				break;
			}

			/* Failed, release the GC right. */
			atomic_fetch_sub_release(&env->vm->gc_stw_counter, 1);

			/* Wait for another thread to finish the GC. */
			while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)
				cpu_relax();
		}
	} else {
		/*
		 * Recursive call, this thread is already non-inflight and in
		 * the GC section.
		 */
	}

	/* Count-up for recursive GC calls. */
	atomic_fetch_add_acquire(&env->gc_in_progress_counter, 1);

	/* Do GC. */
	result = gc(env);

	/* Count-down for recursive GC calls. */
	atomic_fetch_sub_release(&env->gc_in_progress_counter, 1);

back_to_inflight:
	/* If this is not a recursive GC call. */
	if (atomic_load_acquire(&env->gc_in_progress_counter) == 0) {
		if (is_executor) {
			/* Exit the GC section. */
			atomic_fetch_sub_release(&env->vm->gc_stw_counter, 1);
		}

		/*
		 * Wait for other threads to finish GC.
		 * This is not mandatory, but we do cooperation here.
		 */
		while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)
			cpu_relax();

		/* Make this thread in-flight. (Leave the GC safe point.) */
		atomic_fetch_add_acquire(&env->vm->in_flight_counter, 1);
	} else {
		/*
		 * Recursive call, this thread is still non-inflight and in the
		 * GC section.
		 */
	}

	return result;
}
