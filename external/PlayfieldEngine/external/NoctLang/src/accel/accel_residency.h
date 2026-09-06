/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral accelerator buffer residency classification.
 */

#ifndef NOCT_ACCEL_RESIDENCY_H
#define NOCT_ACCEL_RESIDENCY_H

#include "hir_opt_parallel.h"

enum accel_residency_class {
	ACCEL_RESIDENCY_PARAMETER_HOST,
	ACCEL_RESIDENCY_LOCAL_HOST,
	ACCEL_RESIDENCY_LOCAL_DEVICE,
	ACCEL_RESIDENCY_LOCAL_DEVICE_RETURN,
	ACCEL_RESIDENCY_UNSUPPORTED
};

struct accel_device_local_facts {
	bool exact_constructor;
	bool declaration_adjacent;
	bool unique_region;
	bool immutable_extent;
	bool cpu_read;
	bool cpu_write;
	bool returned;
	bool escaped;
	bool unknown_call;
	bool reassigned;
	bool first_kernel_reads;
	bool first_kernel_writes;
	bool first_kernel_full_overwrite;
	bool first_kernel_exact_extent;
};

/*
 * Classifies one GPU-visible logical buffer without changing its HIR.
 */
int
accel_residency_classify_buffer(
	const struct hir_memory_object *object,
	bool reassigned);

/*
 * Classifies a proven single-session local for device-only residency.
 */
int
accel_residency_classify_device_local(
	const struct hir_memory_object *object,
	const struct accel_device_local_facts *facts);

#endif
