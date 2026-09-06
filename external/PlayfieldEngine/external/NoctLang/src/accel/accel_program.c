/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Owned target-neutral accelerator program plans.
 */

#include "accel_program.h"

#include <stdlib.h>
#include <string.h>

#define ACCEL_INT64_MAX_VALUE	((int64_t)(((uint64_t)-1) >> 1))
#define ACCEL_INT64_MIN_VALUE	(-ACCEL_INT64_MAX_VALUE - 1)

static bool accel_program_error(char *error, size_t error_size, const char *message);
static bool accel_program_add_checked(int64_t left, int64_t right, int64_t *result);
static bool accel_program_sub_checked(int64_t left, int64_t right, int64_t *result);
static bool accel_program_mul_checked(int64_t left, int64_t right, int64_t *result);
static bool accel_program_grow_scalars(struct accel_program *program);
static bool accel_program_grow_size_expressions(struct accel_program *program);
static bool accel_program_grow_buffers(struct accel_program *program);
static bool accel_program_grow_scalar_results(struct accel_program *program);
static bool accel_program_grow_kernels(struct accel_program *program);
static bool accel_function_plan_grow(struct accel_function_plan *plan);
static bool accel_program_validate_size_expressions(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_scalars(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_buffers(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_kernels(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_scalar_results(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_argument_namespace(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_device_buffers(const struct accel_program *program, char *error, size_t error_size);
static bool accel_program_validate_device_buffer(const struct accel_program *program, uint32_t buffer_index, char *error, size_t error_size);
static bool accel_program_validate_device_producer(const struct accel_program *program, uint32_t buffer_index, char *error, size_t error_size);
static int accel_program_buffer_value_type(int element_kind);

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
	int last_block_id)
{
	struct accel_program *program;

	if (source_name == NULL)
		return NULL;
	if (function_name == NULL)
		return NULL;
	if (first_block_id < 0)
		return NULL;
	if (last_block_id < 0)
		return NULL;

	program = noct_calloc(1, sizeof(*program));
	if (program == NULL)
		return NULL;

	program->source_name = noct_strdup(source_name);
	if (program->source_name == NULL) {
		accel_program_destroy(program);
		return NULL;
	}

	program->function_name = noct_strdup(function_name);
	if (program->function_name == NULL) {
		accel_program_destroy(program);
		return NULL;
	}

	program->source_line = source_line;
	program->source_function_index = source_function_index;
	program->parameter_count = parameter_count;
	program->region_index = region_index;
	program->first_block_id = first_block_id;
	program->last_block_id = last_block_id;

	return program;
}

/*
 * Clones an accelerator program without cloning backend-owned payload.
 */
struct accel_program *
accel_program_clone(
	const struct accel_program *program)
{
	struct accel_program *result;
	struct accel_scalar_binding scalar;
	struct accel_buffer_binding buffer;
	struct accel_scalar_result scalar_result;
	struct accel_kernel_plan kernel;
	uint32_t ignored;
	uint32_t i;

	if (program == NULL)
		return NULL;

	result = accel_program_create(
		program->source_name,
		program->function_name,
		program->source_line,
		program->source_function_index,
		program->parameter_count,
		program->region_index,
		program->first_block_id,
		program->last_block_id);
	if (result == NULL)
		return NULL;

	/* Clone scalar bindings in their deterministic order. */
	for (i = 0; i < program->scalar_count; i++) {
		scalar = program->scalar[i];
		if (!accel_program_add_scalar(result, &scalar, &ignored)) {
			accel_program_destroy(result);
			return NULL;
		}
	}

	/* Clone size expressions without retaining source pointers. */
	for (i = 0; i < program->size_expression_count; i++) {
		if (!accel_program_add_size_expression(
			result,
			&program->size_expression[i],
			&ignored)) {
			accel_program_destroy(result);
			return NULL;
		}
	}

	/* Clone every buffer descriptor and its fixed effect table. */
	for (i = 0; i < program->buffer_count; i++) {
		buffer = program->buffer[i];
		if (!accel_program_add_buffer(result, &buffer, &ignored)) {
			accel_program_destroy(result);
			return NULL;
		}
	}

	/* Clone scalar-result descriptors in their deterministic entry order. */
	for (i = 0; i < program->scalar_result_count; i++) {
		scalar_result = program->scalar_result[i];
		if (!accel_program_add_scalar_result(
			result,
			&scalar_result,
			&ignored)) {
			accel_program_destroy(result);
			return NULL;
		}
	}

	/* Clone every typed kernel before transferring it to the result. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = program->kernel[i];
		kernel.ir = accel_ir_kernel_clone(program->kernel[i].ir);
		if (kernel.ir == NULL) {
			accel_program_destroy(result);
			return NULL;
		}

		if (!accel_program_add_kernel(result, &kernel, &ignored)) {
			accel_ir_kernel_destroy(kernel.ir);
			accel_program_destroy(result);
			return NULL;
		}
	}

	return result;
}

/*
 * Destroys an accelerator program and every object it owns.
 */
void
accel_program_destroy(
	struct accel_program *program)
{
	uint32_t i;

	if (program == NULL)
		return;

	if (program->backend_payload != NULL &&
	    program->destroy_backend_payload != NULL) {
		program->destroy_backend_payload(program->backend_payload);
	}

	/* Release every deep-copied scalar name. */
	for (i = 0; i < program->scalar_count; i++)
		noct_free(program->scalar[i].name);

	/* Release every deep-copied buffer name. */
	for (i = 0; i < program->buffer_count; i++)
		noct_free(program->buffer[i].name);

	/* Release every deep-copied scalar-result name. */
	for (i = 0; i < program->scalar_result_count; i++)
		noct_free(program->scalar_result[i].name);

	/* Release every typed kernel owned by the program. */
	for (i = 0; i < program->kernel_count; i++)
		accel_ir_kernel_destroy(program->kernel[i].ir);

	noct_free(program->kernel);
	noct_free(program->scalar_result);
	noct_free(program->buffer);
	noct_free(program->size_expression);
	noct_free(program->scalar);
	noct_free(program->function_name);
	noct_free(program->source_name);
	noct_free(program);
}

/*
 * Appends a deep-copied scalar binding.
 */
bool
accel_program_add_scalar(
	struct accel_program *program,
	const struct accel_scalar_binding *binding,
	uint32_t *binding_index)
{
	struct accel_scalar_binding *destination;

	if (binding_index != NULL)
		*binding_index = ACCEL_PROGRAM_INDEX_NONE;
	if (program == NULL)
		return false;
	if (binding == NULL)
		return false;
	if (binding->name == NULL)
		return false;
	if (program->scalar_count >= ACCEL_MAX_SCALAR_BINDINGS)
		return false;

	if (program->scalar_count == program->scalar_capacity) {
		if (!accel_program_grow_scalars(program))
			return false;
	}

	destination = &program->scalar[program->scalar_count];
	memset(destination, 0, sizeof(*destination));
	destination->name = noct_strdup(binding->name);
	if (destination->name == NULL)
		return false;

	destination->args_slot = binding->args_slot;
	destination->value_type = binding->value_type;
	if (binding_index != NULL)
		*binding_index = program->scalar_count;
	program->scalar_count++;

	return true;
}

/*
 * Appends a deep-copied scalar result in deterministic entry order.
 */
bool
accel_program_add_scalar_result(
	struct accel_program *program,
	const struct accel_scalar_result *result,
	uint32_t *result_entry_id)
{
	struct accel_scalar_result *destination;

	if (result_entry_id != NULL)
		*result_entry_id = ACCEL_PROGRAM_INDEX_NONE;
	if (program == NULL)
		return false;
	if (result == NULL)
		return false;
	if (result->name == NULL)
		return false;
	if (program->scalar_result_count >= ACCEL_MAX_SCALAR_BINDINGS)
		return false;

	if (program->scalar_result_count == program->scalar_result_capacity) {
		if (!accel_program_grow_scalar_results(program))
			return false;
	}

	destination = &program->scalar_result[program->scalar_result_count];
	*destination = *result;
	destination->name = noct_strdup(result->name);
	if (destination->name == NULL) {
		memset(destination, 0, sizeof(*destination));
		return false;
	}

	destination->result_entry_id = program->scalar_result_count;
	if (result_entry_id != NULL)
		*result_entry_id = program->scalar_result_count;
	program->scalar_result_count++;

	return true;
}

/*
 * Appends a checked size-expression node.
 */
bool
accel_program_add_size_expression(
	struct accel_program *program,
	const struct accel_size_expression *expression,
	uint32_t *expression_index)
{
	if (expression_index != NULL)
		*expression_index = ACCEL_PROGRAM_INDEX_NONE;
	if (program == NULL)
		return false;
	if (expression == NULL)
		return false;
	if (program->size_expression_count >= ACCEL_MAX_SIZE_EXPRESSIONS)
		return false;

	if (program->size_expression_count ==
	    program->size_expression_capacity) {
		if (!accel_program_grow_size_expressions(program))
			return false;
	}

	program->size_expression[program->size_expression_count] = *expression;
	if (expression_index != NULL)
		*expression_index = program->size_expression_count;
	program->size_expression_count++;

	return true;
}

/*
 * Appends a deep-copied buffer binding.
 */
bool
accel_program_add_buffer(
	struct accel_program *program,
	const struct accel_buffer_binding *binding,
	uint32_t *binding_index)
{
	struct accel_buffer_binding *destination;

	if (binding_index != NULL)
		*binding_index = ACCEL_PROGRAM_INDEX_NONE;
	if (program == NULL)
		return false;
	if (binding == NULL)
		return false;
	if (binding->name == NULL)
		return false;
	if (program->buffer_count >= ACCEL_MAX_BUFFER_BINDINGS)
		return false;

	if (program->buffer_count == program->buffer_capacity) {
		if (!accel_program_grow_buffers(program))
			return false;
	}

	destination = &program->buffer[program->buffer_count];
	*destination = *binding;
	destination->name = noct_strdup(binding->name);
	if (destination->name == NULL) {
		memset(destination, 0, sizeof(*destination));
		return false;
	}

	if (binding_index != NULL)
		*binding_index = program->buffer_count;
	program->buffer_count++;

	return true;
}

/*
 * Transfers one typed kernel into a program.
 */
bool
accel_program_add_kernel(
	struct accel_program *program,
	const struct accel_kernel_plan *kernel,
	uint32_t *kernel_index)
{
	struct accel_kernel_plan *destination;

	if (kernel_index != NULL)
		*kernel_index = ACCEL_PROGRAM_INDEX_NONE;
	if (program == NULL)
		return false;
	if (kernel == NULL)
		return false;
	if (kernel->ir == NULL)
		return false;
	if (program->kernel_count >= ACCEL_MAX_KERNELS)
		return false;

	if (program->kernel_count == program->kernel_capacity) {
		if (!accel_program_grow_kernels(program))
			return false;
	}

	destination = &program->kernel[program->kernel_count];
	*destination = *kernel;
	destination->kernel_index = program->kernel_count;
	if (kernel_index != NULL)
		*kernel_index = program->kernel_count;
	program->kernel_count++;

	return true;
}

/*
 * Evaluates one checked size expression with signed scalar inputs.
 */
bool
accel_program_evaluate_size(
	const struct accel_program *program,
	uint32_t expression_index,
	uint32_t scalar_count,
	const int64_t scalar_value[],
	int64_t *result)
{
	int64_t value[ACCEL_MAX_SIZE_EXPRESSIONS];
	const struct accel_size_expression *expression;
	uint32_t i;

	if (program == NULL)
		return false;
	if (result == NULL)
		return false;
	if (program->size_expression_count > ACCEL_MAX_SIZE_EXPRESSIONS)
		return false;
	if (expression_index >= program->size_expression_count)
		return false;
	if (program->size_expression_count != 0 &&
	    program->size_expression == NULL) {
		return false;
	}

	/* Evaluate the topologically ordered DAG through the requested node. */
	for (i = 0; i <= expression_index; i++) {
		expression = &program->size_expression[i];

		/* Evaluate the checked operation represented by this node. */
		switch (expression->opcode) {
		case ACCEL_SIZE_CONSTANT:
			if (expression->value < 0)
				return false;
			value[i] = expression->value;
			break;
		case ACCEL_SIZE_SCALAR:
			if (expression->reference >= scalar_count)
				return false;
			if (scalar_value == NULL)
				return false;
			value[i] = scalar_value[expression->reference];
			break;
		case ACCEL_SIZE_ADD:
			if (expression->left >= i || expression->right >= i)
				return false;
			if (!accel_program_add_checked(
				value[expression->left],
				value[expression->right],
				&value[i])) {
				return false;
			}
			break;
		case ACCEL_SIZE_SUB:
			if (expression->left >= i || expression->right >= i)
				return false;
			if (!accel_program_sub_checked(
				value[expression->left],
				value[expression->right],
				&value[i])) {
				return false;
			}
			break;
		case ACCEL_SIZE_MUL_CONSTANT:
			if (expression->left >= i)
				return false;
			if (!accel_program_mul_checked(
				value[expression->left],
				expression->value,
				&value[i])) {
				return false;
			}
			break;
		case ACCEL_SIZE_MIN:
			if (expression->left >= i || expression->right >= i)
				return false;
			if (value[expression->left] < value[expression->right])
				value[i] = value[expression->left];
			else
				value[i] = value[expression->right];
			break;
		case ACCEL_SIZE_MAX:
			if (expression->left >= i || expression->right >= i)
				return false;
			if (value[expression->left] > value[expression->right])
				value[i] = value[expression->left];
			else
				value[i] = value[expression->right];
			break;
		case ACCEL_SIZE_MAX_ZERO:
			if (expression->left >= i)
				return false;
			if (value[expression->left] > 0)
				value[i] = value[expression->left];
			else
				value[i] = 0;
			break;
		default:
			return false;
		}
	}

	*result = value[expression_index];

	return true;
}

/*
 * Validates an owned program and every contained typed kernel.
 */
bool
accel_program_validate(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	if (program == NULL)
		return accel_program_error(error, error_size, N_TR("null program"));
	if (program->source_name == NULL)
		return accel_program_error(error, error_size, N_TR("missing source name"));
	if (program->function_name == NULL)
		return accel_program_error(error, error_size, N_TR("missing function name"));
	if (program->first_block_id < 0 || program->last_block_id < 0)
		return accel_program_error(error, error_size, N_TR("invalid region block id"));
	if (program->kernel_count == 0)
		return accel_program_error(error, error_size, N_TR("empty accelerator region"));
	if (program->parameter_count > HIR_PARAM_SIZE)
		return accel_program_error(error, error_size, N_TR("parameter count limit exceeded"));
	if (program->scalar_count > ACCEL_MAX_SCALAR_BINDINGS)
		return accel_program_error(error, error_size, N_TR("scalar binding limit exceeded"));
	if (program->buffer_count > ACCEL_MAX_BUFFER_BINDINGS)
		return accel_program_error(error, error_size, N_TR("buffer binding limit exceeded"));
	if (program->scalar_result_count > ACCEL_MAX_SCALAR_BINDINGS)
		return accel_program_error(error, error_size, N_TR("scalar result limit exceeded"));
	if (program->kernel_count > ACCEL_MAX_KERNELS)
		return accel_program_error(error, error_size, N_TR("kernel limit exceeded"));
	if (program->size_expression_count > ACCEL_MAX_SIZE_EXPRESSIONS) {
		return accel_program_error(
			error,
			error_size,
			N_TR("size expression limit exceeded"));
	}
	if ((program->backend_payload == NULL) !=
	    (program->destroy_backend_payload == NULL)) {
		return accel_program_error(error, error_size, N_TR("invalid backend payload owner"));
	}

	if (!accel_program_validate_size_expressions(program, error, error_size))
		return false;
	if (!accel_program_validate_scalars(program, error, error_size))
		return false;
	if (!accel_program_validate_buffers(program, error, error_size))
		return false;
	if (!accel_program_validate_kernels(program, error, error_size))
		return false;
	if (!accel_program_validate_scalar_results(program, error, error_size))
		return false;
	if (!accel_program_validate_argument_namespace(program, error, error_size))
		return false;
	if (!accel_program_validate_device_buffers(program, error, error_size))
		return false;

	if (error != NULL && error_size != 0)
		error[0] = '\0';

	return true;
}

/*
 * Allocates an empty function-level compilation plan.
 */
struct accel_function_plan *
accel_function_plan_create(void)
{
	return noct_calloc(1, sizeof(struct accel_function_plan));
}

/*
 * Transfers one region program into a function plan.
 */
bool
accel_function_plan_add_region(
	struct accel_function_plan *plan,
	struct accel_program *program)
{
	if (plan == NULL)
		return false;
	if (program == NULL)
		return false;
	if (plan->region_count >= ACCEL_MAX_KERNELS)
		return false;
	if (plan->generated_local_count > ACCEL_PROGRAM_INDEX_NONE - 2)
		return false;

	if (plan->region_count == plan->region_capacity) {
		if (!accel_function_plan_grow(plan))
			return false;
	}

	plan->region[plan->region_count] = program;
	plan->region_count++;
	plan->generated_local_count += 2;

	return true;
}

/*
 * Returns the number of region programs in a function plan.
 */
uint32_t
accel_function_plan_get_region_count(
	const struct accel_function_plan *plan)
{
	if (plan == NULL)
		return 0;

	return plan->region_count;
}

/*
 * Returns the number of locals needed by the planned rewrite.
 */
uint32_t
accel_function_plan_get_generated_local_count(
	const struct accel_function_plan *plan)
{
	if (plan == NULL)
		return 0;

	return plan->generated_local_count;
}

/*
 * Borrows one region program from a function plan.
 */
const struct accel_program *
accel_function_plan_get_region(
	const struct accel_function_plan *plan,
	uint32_t region_index)
{
	if (plan == NULL)
		return NULL;
	if (region_index >= plan->region_count)
		return NULL;

	return plan->region[region_index];
}

/*
 * Destroys a function plan and every region program it owns.
 */
void
accel_function_plan_destroy(
	struct accel_function_plan *plan)
{
	uint32_t i;

	if (plan == NULL)
		return;

	/* Release every region program owned by this transaction plan. */
	for (i = 0; i < plan->region_count; i++)
		accel_program_destroy(plan->region[i]);

	noct_free(plan->region);
	noct_free(plan);
}

/* Copy a stable validation error into the caller's optional buffer. */
static bool
accel_program_error(
	char *error,
	size_t error_size,
	const char *message)
{
	if (error != NULL && error_size != 0) {
		strncpy(error, message, error_size - 1);
		error[error_size - 1] = '\0';
	}

	return false;
}

/* Add two signed values without invoking signed overflow. */
static bool
accel_program_add_checked(
	int64_t left,
	int64_t right,
	int64_t *result)
{
	if (right > 0 && left > ACCEL_INT64_MAX_VALUE - right)
		return false;
	if (right < 0 && left < ACCEL_INT64_MIN_VALUE - right)
		return false;

	*result = left + right;

	return true;
}

/* Subtract two signed values without invoking signed overflow. */
static bool
accel_program_sub_checked(
	int64_t left,
	int64_t right,
	int64_t *result)
{
	if (right > 0 && left < ACCEL_INT64_MIN_VALUE + right)
		return false;
	if (right < 0 && left > ACCEL_INT64_MAX_VALUE + right)
		return false;

	*result = left - right;

	return true;
}

/* Multiply two nonnegative size values without overflow. */
static bool
accel_program_mul_checked(
	int64_t left,
	int64_t right,
	int64_t *result)
{
	if (left < 0)
		return false;
	if (right <= 0)
		return false;
	if (left != 0 && right > ACCEL_INT64_MAX_VALUE / left)
		return false;

	*result = left * right;

	return true;
}

/* Grows the scalar table within its private hard limit. */
static bool
accel_program_grow_scalars(
	struct accel_program *program)
{
	struct accel_scalar_binding *scalar;
	uint32_t capacity;
	size_t size;

	if (program->scalar_capacity == 0)
		capacity = 8;
	else
		capacity = program->scalar_capacity * 2;
	if (capacity > ACCEL_MAX_SCALAR_BINDINGS)
		capacity = ACCEL_MAX_SCALAR_BINDINGS;
	if (capacity <= program->scalar_capacity)
		return false;

	size = sizeof(*scalar) * capacity;
	scalar = noct_realloc(program->scalar, size);
	if (scalar == NULL)
		return false;

	program->scalar = scalar;
	program->scalar_capacity = capacity;

	return true;
}

/* Grows the checked size-expression table within its hard limit. */
static bool
accel_program_grow_size_expressions(
	struct accel_program *program)
{
	struct accel_size_expression *expression;
	uint32_t capacity;
	size_t size;

	if (program->size_expression_capacity == 0)
		capacity = 16;
	else
		capacity = program->size_expression_capacity * 2;
	if (capacity > ACCEL_MAX_SIZE_EXPRESSIONS)
		capacity = ACCEL_MAX_SIZE_EXPRESSIONS;
	if (capacity <= program->size_expression_capacity)
		return false;

	size = sizeof(*expression) * capacity;
	expression = noct_realloc(program->size_expression, size);
	if (expression == NULL)
		return false;

	program->size_expression = expression;
	program->size_expression_capacity = capacity;

	return true;
}

/* Grows the buffer table within its private hard limit. */
static bool
accel_program_grow_buffers(
	struct accel_program *program)
{
	struct accel_buffer_binding *buffer;
	uint32_t capacity;
	size_t size;

	if (program->buffer_capacity == 0)
		capacity = 8;
	else
		capacity = program->buffer_capacity * 2;
	if (capacity > ACCEL_MAX_BUFFER_BINDINGS)
		capacity = ACCEL_MAX_BUFFER_BINDINGS;
	if (capacity <= program->buffer_capacity)
		return false;

	size = sizeof(*buffer) * capacity;
	buffer = noct_realloc(program->buffer, size);
	if (buffer == NULL)
		return false;

	program->buffer = buffer;
	program->buffer_capacity = capacity;

	return true;
}

/* Grows the scalar-result table within its private hard limit. */
static bool
accel_program_grow_scalar_results(
	struct accel_program *program)
{
	struct accel_scalar_result *result;
	uint32_t capacity;
	size_t size;

	if (program->scalar_result_capacity == 0)
		capacity = 4;
	else
		capacity = program->scalar_result_capacity * 2;
	if (capacity > ACCEL_MAX_SCALAR_BINDINGS)
		capacity = ACCEL_MAX_SCALAR_BINDINGS;
	if (capacity <= program->scalar_result_capacity)
		return false;

	size = sizeof(*result) * capacity;
	result = noct_realloc(program->scalar_result, size);
	if (result == NULL)
		return false;

	program->scalar_result = result;
	program->scalar_result_capacity = capacity;

	return true;
}

/* Grows the kernel table within its private hard limit. */
static bool
accel_program_grow_kernels(
	struct accel_program *program)
{
	struct accel_kernel_plan *kernel;
	uint32_t capacity;
	size_t size;

	if (program->kernel_capacity == 0)
		capacity = 4;
	else
		capacity = program->kernel_capacity * 2;
	if (capacity > ACCEL_MAX_KERNELS)
		capacity = ACCEL_MAX_KERNELS;
	if (capacity <= program->kernel_capacity)
		return false;

	size = sizeof(*kernel) * capacity;
	kernel = noct_realloc(program->kernel, size);
	if (kernel == NULL)
		return false;

	program->kernel = kernel;
	program->kernel_capacity = capacity;

	return true;
}

/* Grows a function plan's region-owner table. */
static bool
accel_function_plan_grow(
	struct accel_function_plan *plan)
{
	struct accel_program **region;
	uint32_t capacity;
	size_t size;

	if (plan->region_capacity == 0)
		capacity = 2;
	else
		capacity = plan->region_capacity * 2;
	if (capacity > ACCEL_MAX_KERNELS)
		capacity = ACCEL_MAX_KERNELS;
	if (capacity <= plan->region_capacity)
		return false;

	size = sizeof(*region) * capacity;
	region = noct_realloc(plan->region, size);
	if (region == NULL)
		return false;

	plan->region = region;
	plan->region_capacity = capacity;

	return true;
}

/* Validate the topologically ordered checked size-expression DAG. */
static bool
accel_program_validate_size_expressions(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	const struct accel_size_expression *expression;
	uint32_t i;

	if (program->size_expression_count != 0 &&
	    program->size_expression == NULL) {
		return accel_program_error(error, error_size, N_TR("missing size expression table"));
	}

	/* Validate every expression against only earlier nodes. */
	for (i = 0; i < program->size_expression_count; i++) {
		expression = &program->size_expression[i];

		/* Validate the operand form selected by the size opcode. */
		switch (expression->opcode) {
		case ACCEL_SIZE_CONSTANT:
			if (expression->value < 0) {
				return accel_program_error(
					error,
					error_size,
					N_TR("negative size constant"));
			}
			break;
		case ACCEL_SIZE_SCALAR:
			if (expression->reference >= program->scalar_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid size scalar reference"));
			}
			if (program->scalar[expression->reference].value_type !=
			    ACCEL_IR_I32) {
				return accel_program_error(
					error,
					error_size,
					N_TR("noninteger size scalar"));
			}
			break;
		case ACCEL_SIZE_ADD:
		case ACCEL_SIZE_SUB:
		case ACCEL_SIZE_MIN:
		case ACCEL_SIZE_MAX:
			if (expression->left >= i || expression->right >= i) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid binary size expression"));
			}
			break;
		case ACCEL_SIZE_MUL_CONSTANT:
			if (expression->left >= i || expression->value <= 0) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid size multiplication"));
			}
			break;
		case ACCEL_SIZE_MAX_ZERO:
			if (expression->left >= i) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid zero-clamped size expression"));
			}
			break;
		default:
			return accel_program_error(error, error_size, N_TR("invalid size opcode"));
		}
	}

	return true;
}

/* Validate scalar names, types, and distinct runtime argument slots. */
static bool
accel_program_validate_scalars(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	uint32_t i;
	uint32_t j;

	if (program->scalar_count != 0 && program->scalar == NULL)
		return accel_program_error(error, error_size, N_TR("missing scalar table"));

	/* Validate every scalar descriptor and compare prior slots. */
	for (i = 0; i < program->scalar_count; i++) {
		if (program->scalar[i].name == NULL)
			return accel_program_error(error, error_size, N_TR("missing scalar name"));
		if (program->scalar[i].args_slot >= program->parameter_count)
			return accel_program_error(error, error_size, N_TR("invalid scalar argument slot"));
		if (program->scalar[i].value_type != ACCEL_IR_I32 &&
		    program->scalar[i].value_type != ACCEL_IR_F32) {
			return accel_program_error(error, error_size, N_TR("invalid scalar type"));
		}

		/* Reject a second descriptor for the same runtime argument. */
		for (j = 0; j < i; j++) {
			if (program->scalar[j].args_slot ==
			    program->scalar[i].args_slot) {
				return accel_program_error(
					error,
					error_size,
					N_TR("duplicate scalar argument slot"));
			}
		}
	}

	return true;
}

/* Validate buffer names, ranges, effects, and binding namespaces. */
static bool
accel_program_validate_buffers(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *buffer;
	uint32_t i;
	uint32_t j;
	int value_type;

	if (program->buffer_count != 0 && program->buffer == NULL)
		return accel_program_error(error, error_size, N_TR("missing buffer table"));

	/* Validate every buffer descriptor and compare prior namespaces. */
	for (i = 0; i < program->buffer_count; i++) {
		buffer = &program->buffer[i];
		if (buffer->name == NULL)
			return accel_program_error(error, error_size, N_TR("missing buffer name"));
		if (buffer->origin != ACCEL_BUFFER_PARAMETER &&
		    buffer->origin != ACCEL_BUFFER_LOCAL_HOST &&
		    buffer->origin != ACCEL_BUFFER_LOCAL_DEVICE) {
			return accel_program_error(error, error_size, N_TR("unsupported buffer origin"));
		}

		/* Enforce the host representation selected by the buffer origin. */
		if (buffer->origin == ACCEL_BUFFER_LOCAL_DEVICE) {
			if (buffer->args_slot != ACCEL_ARGS_SLOT_NONE) {
				return accel_program_error(
					error,
					error_size,
					N_TR("device buffer has a host argument slot"));
			}
			if (buffer->host_visible ||
			    buffer->cpu_read ||
			    buffer->cpu_write ||
			    buffer->returned ||
			    buffer->escaped ||
			    buffer->unknown_call ||
			    buffer->reassigned) {
				return accel_program_error(
					error,
					error_size,
					N_TR("device buffer escapes its GPU session"));
			}
			if (buffer->upload_required ||
			    buffer->download_required ||
			    buffer->materialization_required) {
				return accel_program_error(
					error,
					error_size,
					N_TR("device buffer requires host materialization"));
			}
			if (buffer->extent_expression >=
			    program->size_expression_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid device buffer extent"));
			}
		} else {
			if (buffer->origin == ACCEL_BUFFER_PARAMETER &&
			    buffer->args_slot >= program->parameter_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid buffer argument slot"));
			}
			if (buffer->origin == ACCEL_BUFFER_LOCAL_HOST &&
			    (buffer->args_slot < program->parameter_count ||
			     buffer->args_slot >= HIR_PARAM_SIZE)) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid local buffer argument slot"));
			}
			if (!buffer->host_visible) {
				return accel_program_error(
					error,
					error_size,
					N_TR("host buffer is not host visible"));
			}
		}
		if (buffer->device_binding != i)
			return accel_program_error(error, error_size, N_TR("nondeterministic buffer binding"));
		if (buffer->element_width != 4)
			return accel_program_error(error, error_size, N_TR("invalid buffer element width"));

		value_type = accel_program_buffer_value_type(buffer->element_kind);
		if (value_type == ACCEL_IR_VOID)
			return accel_program_error(error, error_size, N_TR("invalid buffer element kind"));
		if (buffer->required_first_expression >=
		    program->size_expression_count) {
			return accel_program_error(error, error_size, N_TR("invalid buffer range start"));
		}
		if (buffer->required_end_expression >=
		    program->size_expression_count) {
			return accel_program_error(error, error_size, N_TR("invalid buffer range end"));
		}
		if (buffer->required_byte_end_expression >=
		    program->size_expression_count) {
			return accel_program_error(error, error_size, N_TR("invalid buffer byte range"));
		}

		/* Validate the exact range owned by every kernel effect. */
		for (j = 0; j < program->kernel_count; j++) {
			if (!buffer->effect[j].read && !buffer->effect[j].write) {
				if (buffer->kernel_required_first_expression[j] !=
				    ACCEL_PROGRAM_INDEX_NONE ||
				    buffer->kernel_required_end_expression[j] !=
				    ACCEL_PROGRAM_INDEX_NONE) {
					return accel_program_error(
						error,
						error_size,
						N_TR("unused kernel has a buffer range"));
				}
				continue;
			}
			if (buffer->kernel_required_first_expression[j] >=
			    program->size_expression_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid kernel buffer range start"));
			}
			if (buffer->kernel_required_end_expression[j] >=
			    program->size_expression_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid kernel buffer range end"));
			}
		}
		/* Reject collisions in either runtime or device binding namespace. */
		for (j = 0; j < i; j++) {
			if (buffer->args_slot != ACCEL_ARGS_SLOT_NONE &&
			    program->buffer[j].args_slot == buffer->args_slot) {
				return accel_program_error(
					error,
					error_size,
					N_TR("duplicate buffer argument slot"));
			}
			if (program->buffer[j].device_binding ==
			    buffer->device_binding) {
				return accel_program_error(
					error,
					error_size,
					N_TR("duplicate device binding"));
			}
		}

		/* Ensure one argument is not both scalar and buffer typed. */
		if (buffer->args_slot != ACCEL_ARGS_SLOT_NONE) {
			for (j = 0; j < program->scalar_count; j++) {
				if (program->scalar[j].args_slot != buffer->args_slot)
					continue;

				return accel_program_error(
					error,
					error_size,
					N_TR("argument has two binding types"));
			}
		}
	}

	return true;
}

/* Validate kernel metadata and every contained typed IR stream. */
static bool
accel_program_validate_kernels(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	const struct accel_kernel_plan *kernel;
	char ir_error[128];
	uint32_t i;
	uint32_t j;
	int value_type;

	if (program->kernel_count != 0 && program->kernel == NULL)
		return accel_program_error(error, error_size, N_TR("missing kernel table"));

	/* Validate every kernel and its program-level range expressions. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = &program->kernel[i];
		if (kernel->kernel_index != i)
			return accel_program_error(error, error_size, N_TR("nondeterministic kernel index"));
		if (kernel->loop_block_id < 0)
			return accel_program_error(error, error_size, N_TR("invalid loop block id"));
		if (kernel->start_expression >= program->size_expression_count)
			return accel_program_error(error, error_size, N_TR("invalid kernel start"));
		if (kernel->stop_expression >= program->size_expression_count)
			return accel_program_error(error, error_size, N_TR("invalid kernel stop"));
		if (kernel->trip_expression >= program->size_expression_count)
			return accel_program_error(error, error_size, N_TR("invalid kernel trip"));
		if (kernel->ir == NULL)
			return accel_program_error(error, error_size, N_TR("missing typed kernel"));
		if (kernel->ir->scalar_binding_count != program->scalar_count)
			return accel_program_error(error, error_size, N_TR("kernel scalar table mismatch"));
		if (kernel->ir->buffer_binding_count != program->buffer_count)
			return accel_program_error(error, error_size, N_TR("kernel buffer table mismatch"));

		/* Match each kernel buffer type to the program descriptor. */
		for (j = 0; j < program->buffer_count; j++) {
			value_type = accel_program_buffer_value_type(
				program->buffer[j].element_kind);
			if (kernel->ir->buffer_value_type[j] != value_type) {
				return accel_program_error(
					error,
					error_size,
					N_TR("kernel buffer type mismatch"));
			}
		}

		if (!accel_ir_kernel_validate(kernel->ir, ir_error, sizeof(ir_error)))
			return accel_program_error(error, error_size, ir_error);
	}

	return true;
}

/* Validate scalar-result ownership, producers, and consumer metadata. */
static bool
accel_program_validate_scalar_results(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	const struct accel_scalar_result *result;
	const struct accel_ir_instruction *instruction;
	uint32_t producer_count[ACCEL_MAX_SCALAR_BINDINGS];
	uint32_t consumer_mask[ACCEL_MAX_SCALAR_BINDINGS];
	uint32_t result_entry_id;
	uint32_t i;
	uint32_t j;
	uint32_t k;

	if (program->scalar_result_count != 0 &&
	    program->scalar_result == NULL) {
		return accel_program_error(error, error_size, N_TR("missing scalar result table"));
	}

	memset(producer_count, 0, sizeof(producer_count));
	memset(consumer_mask, 0, sizeof(consumer_mask));

	/* Validate every deterministic scalar-result descriptor. */
	for (i = 0; i < program->scalar_result_count; i++) {
		result = &program->scalar_result[i];
		if (result->name == NULL)
			return accel_program_error(error, error_size, N_TR("missing scalar result name"));
		if (result->result_entry_id != i)
			return accel_program_error(error, error_size, N_TR("nondeterministic scalar result entry"));
		if (result->args_slot < program->parameter_count ||
		    result->args_slot >= HIR_PARAM_SIZE) {
			return accel_program_error(error, error_size, N_TR("invalid scalar result argument slot"));
		}
		if (result->value_type != ACCEL_IR_I32)
			return accel_program_error(error, error_size, N_TR("invalid scalar result type"));
		if (result->identity_bits != 0)
			return accel_program_error(error, error_size, N_TR("invalid scalar result identity"));
		if (result->producer_kernel >= program->kernel_count)
			return accel_program_error(error, error_size, N_TR("invalid scalar result producer"));

		/* Reject a reused result argument slot. */
		for (j = 0; j < i; j++) {
			if (program->scalar_result[j].args_slot == result->args_slot) {
				return accel_program_error(
					error,
					error_size,
					N_TR("duplicate scalar result argument slot"));
			}
		}

		/* Keep scalar-result output slots distinct from every input. */
		for (j = 0; j < program->scalar_count; j++) {
			if (program->scalar[j].args_slot == result->args_slot) {
				return accel_program_error(
					error,
					error_size,
					N_TR("scalar result collides with scalar input"));
			}
		}
		for (j = 0; j < program->buffer_count; j++) {
			if (program->buffer[j].args_slot == ACCEL_ARGS_SLOT_NONE)
				continue;
			if (program->buffer[j].args_slot == result->args_slot) {
				return accel_program_error(
					error,
					error_size,
					N_TR("scalar result collides with buffer input"));
			}
		}
	}

	/* Account for every static result producer and GPU consumer. */
	for (i = 0; i < program->kernel_count; i++) {
		for (j = 0; j < program->kernel[i].ir->instruction_count; j++) {
			instruction = &program->kernel[i].ir->instruction[j];
			if (instruction->opcode != ACCEL_IR_ATOMIC_ADD_I32 &&
			    instruction->opcode != ACCEL_IR_LOAD_RESULT_I32) {
				continue;
			}

			result_entry_id = instruction->reference;
			if (result_entry_id >= program->scalar_result_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("scalar result reference out of range"));
			}

			result = &program->scalar_result[result_entry_id];
			if (instruction->opcode == ACCEL_IR_ATOMIC_ADD_I32) {
				if (i != result->producer_kernel) {
					return accel_program_error(
						error,
						error_size,
						N_TR("scalar result has a foreign producer"));
				}
				producer_count[result_entry_id]++;
				continue;
			}

			if (i <= result->producer_kernel) {
				return accel_program_error(
					error,
					error_size,
					N_TR("scalar result load precedes its producer"));
			}
			consumer_mask[result_entry_id] |= (uint32_t)1U << i;
		}
	}

	/* Match the recorded producer and consumer contracts exactly. */
	for (k = 0; k < program->scalar_result_count; k++) {
		if (producer_count[k] != 1)
			return accel_program_error(error, error_size, N_TR("scalar result producer is not unique"));
		if (consumer_mask[k] != program->scalar_result[k].gpu_consumer_mask) {
			return accel_program_error(
				error,
				error_size,
				N_TR("scalar result consumer metadata mismatch"));
		}
	}

	return true;
}

/* Validate the dense generated-argument namespace across every binding kind. */
static bool
accel_program_validate_argument_namespace(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	bool occupied[HIR_PARAM_SIZE];
	uint32_t argument_count;
	uint32_t slot;
	uint32_t i;

	memset(occupied, 0, sizeof(occupied));
	argument_count = program->parameter_count;

	/* Reserve the exact generated Array prefix for every source parameter. */
	for (i = 0; i < program->parameter_count; i++)
		occupied[i] = true;

	/* Verify that every scalar descriptor points into the parameter prefix. */
	for (i = 0; i < program->scalar_count; i++) {
		slot = program->scalar[i].args_slot;
		if (slot >= program->parameter_count)
			return accel_program_error(error, error_size, N_TR("invalid scalar argument slot"));
	}

	/* Claim host-local slots after accepting parameter references in the prefix. */
	for (i = 0; i < program->buffer_count; i++) {
		slot = program->buffer[i].args_slot;
		if (slot == ACCEL_ARGS_SLOT_NONE)
			continue;
		if (program->buffer[i].origin == ACCEL_BUFFER_PARAMETER) {
			if (slot >= program->parameter_count) {
				return accel_program_error(
					error,
					error_size,
					N_TR("invalid buffer argument slot"));
			}
			continue;
		}
		if (slot < program->parameter_count || slot >= HIR_PARAM_SIZE)
			return accel_program_error(error, error_size, N_TR("invalid buffer argument slot"));
		if (occupied[slot])
			return accel_program_error(error, error_size, N_TR("duplicate runtime argument slot"));
		occupied[slot] = true;
		argument_count++;
	}

	/* Claim scalar-result publication slots after every input binding. */
	for (i = 0; i < program->scalar_result_count; i++) {
		slot = program->scalar_result[i].args_slot;
		if (slot >= HIR_PARAM_SIZE)
			return accel_program_error(error, error_size, N_TR("invalid scalar result argument slot"));
		if (occupied[slot])
			return accel_program_error(error, error_size, N_TR("duplicate runtime argument slot"));
		occupied[slot] = true;
		argument_count++;
	}

	/* Reject a sparse namespace that would require a fake Array placeholder. */
	for (i = 0; i < argument_count; i++) {
		if (!occupied[i])
			return accel_program_error(error, error_size, N_TR("sparse runtime argument namespace"));
	}

	/* Reports a complete dense namespace. */
	return true;
}

/* Validate every device-only descriptor after its kernels are available. */
static bool
accel_program_validate_device_buffers(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Validate the stronger proof carried by each device-only binding. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_DEVICE)
			continue;
		if (!accel_program_validate_device_buffer(
			program,
			i,
			error,
			error_size)) {
			return false;
		}
	}

	/* Reports that all device descriptors preserve the private contract. */
	return true;
}

/* Validate one device extent and its exact first-kernel ownership range. */
static bool
accel_program_validate_device_buffer(
	const struct accel_program *program,
	uint32_t buffer_index,
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *buffer;
	const struct accel_size_expression *extent;
	const struct accel_size_expression *byte_end;

	buffer = &program->buffer[buffer_index];
	extent = &program->size_expression[buffer->extent_expression];

	/* Limit device allocation extents to one positive literal or I32 input. */
	if (extent->opcode == ACCEL_SIZE_CONSTANT) {
		if (extent->value <= 0)
			return accel_program_error(error, error_size, N_TR("nonpositive device buffer extent"));
	} else if (extent->opcode != ACCEL_SIZE_SCALAR) {
		return accel_program_error(error, error_size, N_TR("unsupported device buffer extent"));
	}

	/* Require byte sizing to derive directly from the validated extent. */
	if (buffer->required_byte_end_expression >= program->size_expression_count)
		return accel_program_error(error, error_size, N_TR("invalid device buffer byte extent"));
	byte_end = &program->size_expression[buffer->required_byte_end_expression];
	if (byte_end->opcode != ACCEL_SIZE_MUL_CONSTANT ||
	    byte_end->left != buffer->extent_expression ||
	    byte_end->value != (int64_t)buffer->element_width) {
		return accel_program_error(error, error_size, N_TR("invalid device buffer byte extent"));
	}

	/* Validate the unconditional exact producer carried by the first kernel. */
	if (!accel_program_validate_device_producer(
		program,
		buffer_index,
		error,
		error_size)) {
		return false;
	}

	/* Reports an exact single-session device allocation. */
	return true;
}

/* Validate the first kernel's direct counter-to-buffer definition. */
static bool
accel_program_validate_device_producer(
	const struct accel_program *program,
	uint32_t buffer_index,
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *buffer;
	const struct accel_kernel_plan *kernel;
	const struct accel_ir_instruction *instruction;
	const struct accel_size_expression *start;
	const struct accel_size_expression *trip;
	const struct accel_size_expression *difference;
	uint32_t global_index_value;
	uint32_t store_count;
	uint32_t i;
	uint32_t j;
	bool later_read;

	/* Reject a malformed program before reading its first kernel. */
	if (program->kernel_count == 0 || program->kernel == NULL)
		return accel_program_error(error, error_size, N_TR("device buffer has no producer kernel"));

	buffer = &program->buffer[buffer_index];
	kernel = &program->kernel[0];
	global_index_value = ACCEL_IR_VALUE_NONE;
	store_count = 0;
	later_read = false;

	/* Require the first loop to iterate over the complete allocation. */
	start = &program->size_expression[kernel->start_expression];
	if (start->opcode != ACCEL_SIZE_CONSTANT || start->value != 0)
		return accel_program_error(error, error_size, N_TR("device producer does not start at zero"));
	if (kernel->stop_expression != buffer->extent_expression)
		return accel_program_error(error, error_size, N_TR("device producer does not cover its extent"));
	if (buffer->kernel_required_first_expression[0] !=
	    kernel->start_expression ||
	    buffer->kernel_required_end_expression[0] !=
	    kernel->stop_expression) {
		return accel_program_error(error, error_size, N_TR("device producer range is not exact"));
	}
	if (buffer->effect[0].read ||
	    buffer->effect[0].read_before_write ||
	    !buffer->effect[0].write ||
	    !buffer->effect[0].full_overwrite) {
		return accel_program_error(error, error_size, N_TR("device producer is not a full definition"));
	}

	/* Require the canonical positive-extent trip calculation. */
	trip = &program->size_expression[kernel->trip_expression];
	if (trip->opcode != ACCEL_SIZE_MAX_ZERO ||
	    trip->left >= kernel->trip_expression) {
		return accel_program_error(error, error_size, N_TR("device producer has an invalid trip expression"));
	}
	difference = &program->size_expression[trip->left];
	if (difference->opcode != ACCEL_SIZE_SUB ||
	    difference->left != kernel->stop_expression ||
	    difference->right != kernel->start_expression) {
		return accel_program_error(error, error_size, N_TR("device producer has an invalid trip difference"));
	}

	/* Find the invocation index and exact store for this binding. */
	for (i = 0; i < kernel->ir->instruction_count; i++) {
		instruction = &kernel->ir->instruction[i];
		if (instruction->opcode == ACCEL_IR_GLOBAL_INDEX) {
			if (global_index_value != ACCEL_IR_VALUE_NONE) {
				return accel_program_error(
					error,
					error_size,
					N_TR("device producer has multiple invocation indices"));
			}
			global_index_value = instruction->result;
			continue;
		}
		if (instruction->reference != buffer_index)
			continue;
		if (instruction->opcode == ACCEL_IR_BUFFER_LOAD) {
			return accel_program_error(error, error_size, N_TR("device producer reads before definition"));
		}
		if (instruction->opcode != ACCEL_IR_BUFFER_STORE)
			continue;
		store_count++;
		if (instruction->operand[0] != global_index_value) {
			return accel_program_error(error, error_size, N_TR("device producer index is not exact"));
		}
	}

	/* Require one unconditional direct store for every invocation. */
	if (global_index_value == ACCEL_IR_VALUE_NONE || store_count != 1)
		return accel_program_error(error, error_size, N_TR("device producer store is not unique"));

	/* Require an actual later kernel load from the private allocation. */
	for (i = 1; i < program->kernel_count; i++) {
		for (j = 0;
		     j < program->kernel[i].ir->instruction_count;
		     j++) {
			instruction = &program->kernel[i].ir->instruction[j];
			if (instruction->opcode == ACCEL_IR_BUFFER_LOAD &&
			    instruction->reference == buffer_index) {
				later_read = true;
			}
		}
	}
	if (!later_read)
		return accel_program_error(error, error_size, N_TR("device buffer has no later consumer"));

	/* Reports a complete first-kernel definition. */
	return true;
}

/* Map a supported Packed element kind to its source scalar IR type. */
static int
accel_program_buffer_value_type(
	int element_kind)
{
	if (element_kind == NOCT_PACKED_INT32)
		return ACCEL_IR_I32;
	if (element_kind == NOCT_PACKED_UINT32)
		return ACCEL_IR_I32;
	if (element_kind == NOCT_PACKED_FLOAT32)
		return ACCEL_IR_F32;

	return ACCEL_IR_VOID;
}
