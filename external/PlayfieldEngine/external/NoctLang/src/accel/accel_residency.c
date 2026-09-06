/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral accelerator buffer residency classification.
 */

#include "accel_residency.h"

/*
 * Classifies the initial host-backed residency subset conservatively.
 */
int
accel_residency_classify_buffer(
	const struct hir_memory_object *object,
	bool reassigned)
{
	if (object == NULL)
		return ACCEL_RESIDENCY_UNSUPPORTED;
	if (reassigned)
		return ACCEL_RESIDENCY_UNSUPPORTED;

	if (object->storage == HIR_MEMORY_STORAGE_PARAMETER)
		return ACCEL_RESIDENCY_PARAMETER_HOST;
	if (object->storage == HIR_MEMORY_STORAGE_LOCAL)
		return ACCEL_RESIDENCY_LOCAL_HOST;

	return ACCEL_RESIDENCY_UNSUPPORTED;
}

/*
 * Classifies a proven single-session local for device-only residency.
 *
 * Failed device-only proofs retain the ordinary CPU-backed local whenever
 * that representation is safe.  The caller separately validates that local's
 * exact Packed constructor before accepting the fallback.
 */
int
accel_residency_classify_device_local(
	const struct hir_memory_object *object,
	const struct accel_device_local_facts *facts)
{
	int residency;

	/* Establish the safe representation before considering promotion. */
	residency = accel_residency_classify_buffer(
		object,
		facts != NULL ? facts->reassigned : false);

	/* Preserve the ordinary result when no device proof is available. */
	if (residency != ACCEL_RESIDENCY_LOCAL_HOST)
		return residency;
	if (facts == NULL)
		return residency;

	/* Require the exact removable constructor and source-order boundary. */
	if (!facts->exact_constructor)
		return residency;
	if (!facts->declaration_adjacent)
		return residency;
	if (!facts->unique_region)
		return residency;
	if (!facts->immutable_extent)
		return residency;

	/* Reject every host-visible use or binding escape. */
	if (facts->cpu_read)
		return residency;
	if (facts->cpu_write)
		return residency;
	if (facts->returned)
		return residency;
	if (facts->escaped)
		return residency;
	if (facts->unknown_call)
		return residency;
	if (facts->reassigned)
		return residency;

	/* Require the first kernel to define the complete device allocation. */
	if (facts->first_kernel_reads)
		return residency;
	if (!facts->first_kernel_writes)
		return residency;
	if (!facts->first_kernel_full_overwrite)
		return residency;
	if (!facts->first_kernel_exact_extent)
		return residency;

	/* Select the allocation-free host representation. */
	return ACCEL_RESIDENCY_LOCAL_DEVICE;
}
