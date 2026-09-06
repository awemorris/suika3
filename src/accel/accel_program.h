/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Owned target-neutral accelerator program plans.
 */

#ifndef NOCT_ACCEL_PROGRAM_H
#define NOCT_ACCEL_PROGRAM_H

#include "accel_ir.h"

#define ACCEL_PROGRAM_INDEX_NONE	((uint32_t)-1)
#define ACCEL_ARGS_SLOT_NONE		((uint32_t)-1)

enum accel_size_opcode {
	ACCEL_SIZE_CONSTANT,
	ACCEL_SIZE_SCALAR,
	ACCEL_SIZE_ADD,
	ACCEL_SIZE_SUB,
	ACCEL_SIZE_MUL_CONSTANT,
	ACCEL_SIZE_MIN,
	ACCEL_SIZE_MAX,
	ACCEL_SIZE_MAX_ZERO
};

enum accel_buffer_origin {
	ACCEL_BUFFER_PARAMETER,
	ACCEL_BUFFER_LOCAL_HOST,
	ACCEL_BUFFER_LOCAL_DEVICE
};

struct accel_size_expression {
	int opcode;
	uint32_t left;
	uint32_t right;
	uint32_t reference;
	int64_t value;
};

struct accel_scalar_binding {
	char *name;
	uint32_t args_slot;
	int value_type;
};

struct accel_scalar_result {
	char *name;
	uint32_t result_entry_id;
	uint32_t args_slot;
	int value_type;
	uint32_t identity_bits;
	uint32_t producer_kernel;
	uint32_t gpu_consumer_mask;
	bool cpu_publication;
};

struct accel_buffer_effect {
	bool read;
	bool write;
	bool read_before_write;
	bool full_overwrite;
};

struct accel_buffer_binding {
	char *name;
	int source_line;
	int element_kind;
	uint32_t element_width;
	int origin;
	uint32_t args_slot;
	uint32_t device_binding;
	uint32_t required_first_expression;
	uint32_t required_end_expression;
	uint32_t required_byte_end_expression;
	uint32_t extent_expression;
	uint32_t kernel_required_first_expression[ACCEL_MAX_KERNELS];
	uint32_t kernel_required_end_expression[ACCEL_MAX_KERNELS];
	bool host_visible;
	bool cpu_read;
	bool cpu_write;
	bool returned;
	bool escaped;
	bool unknown_call;
	bool reassigned;
	bool upload_required;
	bool download_required;
	bool materialization_required;
	struct accel_buffer_effect effect[ACCEL_MAX_KERNELS];
};

struct accel_kernel_plan {
	uint32_t kernel_index;
	int source_line;
	int loop_block_id;
	uint32_t start_expression;
	uint32_t stop_expression;
	uint32_t trip_expression;
	struct accel_ir_kernel *ir;
};

struct accel_program {
	char *source_name;
	char *function_name;
	int source_line;
	uint32_t source_function_index;
	uint32_t parameter_count;
	uint32_t region_index;
	int first_block_id;
	int last_block_id;
	uint32_t scalar_count;
	uint32_t scalar_capacity;
	struct accel_scalar_binding *scalar;
	uint32_t size_expression_count;
	uint32_t size_expression_capacity;
	struct accel_size_expression *size_expression;
	uint32_t buffer_count;
	uint32_t buffer_capacity;
	struct accel_buffer_binding *buffer;
	uint32_t scalar_result_count;
	uint32_t scalar_result_capacity;
	struct accel_scalar_result *scalar_result;
	uint32_t kernel_count;
	uint32_t kernel_capacity;
	struct accel_kernel_plan *kernel;
	void *backend_payload;
	void (*destroy_backend_payload)(void *payload);
};

struct accel_function_plan {
	uint32_t generated_local_count;
	uint32_t region_count;
	uint32_t region_capacity;
	struct accel_program **region;
};

/*
 * Allocates an empty accelerator program for one consecutive loop region.
 */
struct accel_program *
accel_program_create(
	const char *source_name,
	const char *function_name,
	int source_line,
	uint32_t source_function_index,
	uint32_t parameter_count,
	uint32_t region_index,
	int first_block_id,
	int last_block_id);

/*
 * Clones an accelerator program without cloning backend-owned payload.
 */
struct accel_program *
accel_program_clone(
	const struct accel_program *program);

/*
 * Destroys an accelerator program and every object it owns.
 */
void
accel_program_destroy(
	struct accel_program *program);

/*
 * Appends a deep-copied scalar binding.
 */
bool
accel_program_add_scalar(
	struct accel_program *program,
	const struct accel_scalar_binding *binding,
	uint32_t *binding_index);

/*
 * Appends a deep-copied scalar result in deterministic entry order.
 */
bool
accel_program_add_scalar_result(
	struct accel_program *program,
	const struct accel_scalar_result *result,
	uint32_t *result_entry_id);

/*
 * Appends a checked size-expression node.
 */
bool
accel_program_add_size_expression(
	struct accel_program *program,
	const struct accel_size_expression *expression,
	uint32_t *expression_index);

/*
 * Appends a deep-copied buffer binding.
 */
bool
accel_program_add_buffer(
	struct accel_program *program,
	const struct accel_buffer_binding *binding,
	uint32_t *binding_index);

/*
 * Transfers one typed kernel into a program.
 */
bool
accel_program_add_kernel(
	struct accel_program *program,
	const struct accel_kernel_plan *kernel,
	uint32_t *kernel_index);

/*
 * Evaluates one checked size expression with signed scalar inputs.
 */
bool
accel_program_evaluate_size(
	const struct accel_program *program,
	uint32_t expression_index,
	uint32_t scalar_count,
	const int64_t scalar_value[],
	int64_t *result);

/*
 * Validates an owned program and every contained typed kernel.
 */
bool
accel_program_validate(
	const struct accel_program *program,
	char *error,
	size_t error_size);

/*
 * Allocates an empty function-level compilation plan.
 */
struct accel_function_plan *
accel_function_plan_create(void);

/*
 * Transfers one region program into a function plan.
 */
bool
accel_function_plan_add_region(
	struct accel_function_plan *plan,
	struct accel_program *program);

/*
 * Returns the number of region programs in a function plan.
 */
uint32_t
accel_function_plan_get_region_count(
	const struct accel_function_plan *plan);

/*
 * Returns the number of locals needed by the planned rewrite.
 */
uint32_t
accel_function_plan_get_generated_local_count(
	const struct accel_function_plan *plan);

/*
 * Borrows one region program from a function plan.
 */
const struct accel_program *
accel_function_plan_get_region(
	const struct accel_function_plan *plan,
	uint32_t region_index);

/*
 * Destroys a function plan and every region program it owns.
 */
void
accel_function_plan_destroy(
	struct accel_function_plan *plan);

#endif
