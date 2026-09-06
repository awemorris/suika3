/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral typed accelerator IR.
 */

#include "accel_ir.h"

#include <stdlib.h>
#include <string.h>

static bool accel_ir_error(char *error, size_t error_size, const char *message);
static bool accel_ir_value_type_valid(int type);
static bool accel_ir_scalar_type_valid(int type);
static bool accel_ir_unused_operand(const struct accel_ir_instruction *instruction, uint32_t first);
static bool accel_ir_operand(const struct accel_ir_instruction *instruction, uint32_t operand_index, uint32_t defined_value_count, const int value_type[], int required_type);
static bool accel_ir_result(const struct accel_ir_instruction *instruction, uint32_t expected_value, bool has_result);
static bool accel_ir_validate_input(const struct accel_ir_kernel *kernel, const struct accel_ir_instruction *instruction, uint32_t defined_value_count, const int value_type[]);
static bool accel_ir_validate_arithmetic(const struct accel_ir_instruction *instruction, uint32_t defined_value_count, const int value_type[]);
static bool accel_ir_validate_comparison(const struct accel_ir_instruction *instruction, uint32_t defined_value_count, const int value_type[]);
static bool accel_ir_grow(struct accel_ir_builder *builder);

/*
 * Allocates an empty typed kernel.
 */
struct accel_ir_kernel *
accel_ir_kernel_create(
	const char *name,
	int source_line,
	int loop_block_id,
	uint32_t scalar_binding_count,
	uint32_t buffer_binding_count)
{
	struct accel_ir_kernel *kernel;
	uint32_t i;

	if (name == NULL)
		return NULL;
	if (scalar_binding_count > ACCEL_MAX_SCALAR_BINDINGS)
		return NULL;
	if (buffer_binding_count > ACCEL_MAX_BUFFER_BINDINGS)
		return NULL;

	kernel = noct_calloc(1, sizeof(*kernel));
	if (kernel == NULL)
		return NULL;

	kernel->name = noct_strdup(name);
	if (kernel->name == NULL) {
		noct_free(kernel);
		return NULL;
	}

	kernel->source_line = source_line;
	kernel->loop_block_id = loop_block_id;
	kernel->scalar_binding_count = scalar_binding_count;
	kernel->buffer_binding_count = buffer_binding_count;

	/* Mark every buffer type as unset until the program binds it. */
	for (i = 0; i < ACCEL_MAX_BUFFER_BINDINGS; i++)
		kernel->buffer_value_type[i] = ACCEL_IR_VOID;

	return kernel;
}

/*
 * Clones a typed kernel without retaining borrowed input storage.
 */
struct accel_ir_kernel *
accel_ir_kernel_clone(
	const struct accel_ir_kernel *kernel)
{
	struct accel_ir_kernel *result;
	size_t size;

	if (kernel == NULL)
		return NULL;
	if (kernel->name == NULL)
		return NULL;
	if (kernel->instruction_count > ACCEL_MAX_IR_INSTRUCTIONS)
		return NULL;
	if (kernel->instruction_count != 0 && kernel->instruction == NULL)
		return NULL;

	result = accel_ir_kernel_create(
		kernel->name,
		kernel->source_line,
		kernel->loop_block_id,
		kernel->scalar_binding_count,
		kernel->buffer_binding_count);
	if (result == NULL)
		return NULL;

	memcpy(
		result->buffer_value_type,
		kernel->buffer_value_type,
		sizeof(result->buffer_value_type));
	result->value_count = kernel->value_count;

	if (kernel->instruction_count == 0)
		return result;

	size = sizeof(*result->instruction) * kernel->instruction_count;
	result->instruction = noct_malloc(size);
	if (result->instruction == NULL) {
		accel_ir_kernel_destroy(result);
		return NULL;
	}

	memcpy(result->instruction, kernel->instruction, size);
	result->instruction_count = kernel->instruction_count;
	result->instruction_capacity = kernel->instruction_count;

	return result;
}

/*
 * Destroys a typed kernel and all storage it owns.
 */
void
accel_ir_kernel_destroy(
	struct accel_ir_kernel *kernel)
{
	if (kernel == NULL)
		return;

	noct_free(kernel->instruction);
	noct_free(kernel->name);
	noct_free(kernel);
}

/*
 * Initializes an instruction builder for one kernel.
 */
void
accel_ir_builder_init(
	struct accel_ir_builder *builder,
	struct accel_ir_kernel *kernel)
{
	if (builder == NULL)
		return;

	memset(builder, 0, sizeof(*builder));
	builder->kernel = kernel;
}

/*
 * Records the scalar value type stored by one buffer binding.
 */
bool
accel_ir_kernel_set_buffer_type(
	struct accel_ir_kernel *kernel,
	uint32_t buffer_binding,
	int value_type)
{
	if (kernel == NULL)
		return false;
	if (buffer_binding >= kernel->buffer_binding_count)
		return false;
	if (value_type != ACCEL_IR_I32 && value_type != ACCEL_IR_F32)
		return false;

	kernel->buffer_value_type[buffer_binding] = value_type;

	return true;
}

/*
 * Appends one instruction and assigns its deterministic result value.
 */
bool
accel_ir_builder_append(
	struct accel_ir_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t *result_value)
{
	struct accel_ir_instruction *destination;
	struct accel_ir_kernel *kernel;
	bool has_result;

	if (result_value != NULL)
		*result_value = ACCEL_IR_VALUE_NONE;
	if (builder == NULL)
		return false;
	if (instruction == NULL)
		return false;
	if (builder->kernel == NULL)
		return false;
	if (builder->limit_exceeded || builder->out_of_memory)
		return false;

	kernel = builder->kernel;
	if (kernel->instruction_count >= ACCEL_MAX_IR_INSTRUCTIONS) {
		builder->limit_exceeded = true;
		return false;
	}

	has_result = instruction->result_type != ACCEL_IR_VOID;
	if (has_result && kernel->value_count >= ACCEL_MAX_IR_VALUES) {
		builder->limit_exceeded = true;
		return false;
	}

	if (kernel->instruction_count == kernel->instruction_capacity) {
		if (!accel_ir_grow(builder))
			return false;
	}

	destination = &kernel->instruction[kernel->instruction_count];
	*destination = *instruction;
	if (has_result) {
		destination->result = kernel->value_count;
		if (result_value != NULL)
			*result_value = kernel->value_count;
		kernel->value_count++;
	} else {
		destination->result = ACCEL_IR_VALUE_NONE;
	}
	kernel->instruction_count++;

	return true;
}

/*
 * Validates one kernel's typed, use-before-definition-safe instruction stream.
 */
bool
accel_ir_kernel_validate(
	const struct accel_ir_kernel *kernel,
	char *error,
	size_t error_size)
{
	int value_type[ACCEL_MAX_IR_VALUES];
	bool constant_i32[ACCEL_MAX_IR_VALUES];
	int32_t constant_value[ACCEL_MAX_IR_VALUES];
	const struct accel_ir_instruction *instruction;
	uint32_t defined_value_count;
	uint32_t i;
	bool valid;

	if (kernel == NULL)
		return accel_ir_error(error, error_size, N_TR("null kernel"));
	if (kernel->name == NULL)
		return accel_ir_error(error, error_size, N_TR("missing kernel name"));
	if (kernel->scalar_binding_count > ACCEL_MAX_SCALAR_BINDINGS)
		return accel_ir_error(error, error_size, N_TR("scalar binding limit exceeded"));
	if (kernel->buffer_binding_count > ACCEL_MAX_BUFFER_BINDINGS)
		return accel_ir_error(error, error_size, N_TR("buffer binding limit exceeded"));
	if (kernel->instruction_count > ACCEL_MAX_IR_INSTRUCTIONS)
		return accel_ir_error(error, error_size, N_TR("instruction limit exceeded"));
	if (kernel->value_count > ACCEL_MAX_IR_VALUES)
		return accel_ir_error(error, error_size, N_TR("value limit exceeded"));
	if (kernel->instruction_count != 0 && kernel->instruction == NULL)
		return accel_ir_error(error, error_size, N_TR("missing instruction table"));

	/* Validate every declared buffer element type. */
	for (i = 0; i < kernel->buffer_binding_count; i++) {
		if (kernel->buffer_value_type[i] != ACCEL_IR_I32 &&
		    kernel->buffer_value_type[i] != ACCEL_IR_F32) {
			return accel_ir_error(
				error,
				error_size,
				N_TR("invalid buffer value type"));
		}
	}

	memset(constant_i32, 0, sizeof(constant_i32));
	defined_value_count = 0;

	/* Validate instructions in definition order. */
	for (i = 0; i < kernel->instruction_count; i++) {
		instruction = &kernel->instruction[i];
		if (!accel_ir_value_type_valid(instruction->result_type)) {
			return accel_ir_error(
				error,
				error_size,
				N_TR("invalid instruction result type"));
		}

		valid = accel_ir_validate_input(
			kernel,
			instruction,
			defined_value_count,
			value_type);
		if (!valid)
			return accel_ir_error(error, error_size, N_TR("invalid instruction"));

		if (instruction->opcode == ACCEL_IR_DIV_I32 ||
		    instruction->opcode == ACCEL_IR_MOD_I32) {
			if (!constant_i32[instruction->operand[1]]) {
				return accel_ir_error(
					error,
					error_size,
					N_TR("nonconstant integer divisor"));
			}
			if (constant_value[instruction->operand[1]] <= 0) {
				return accel_ir_error(
					error,
					error_size,
					N_TR("nonpositive integer divisor"));
			}
		}
		if (instruction->opcode == ACCEL_IR_SHIFT_LEFT ||
		    instruction->opcode == ACCEL_IR_SHIFT_RIGHT_LOGICAL) {
			if (!constant_i32[instruction->operand[1]]) {
				return accel_ir_error(
					error,
					error_size,
					N_TR("nonconstant shift count"));
			}
			if (constant_value[instruction->operand[1]] < 0 ||
			    constant_value[instruction->operand[1]] > 31) {
				return accel_ir_error(
					error,
					error_size,
					N_TR("shift count out of range"));
			}
		}

		if (instruction->result_type == ACCEL_IR_VOID)
			continue;

		value_type[defined_value_count] = instruction->result_type;
		if (instruction->opcode == ACCEL_IR_CONST_I32) {
			constant_i32[defined_value_count] = true;
			constant_value[defined_value_count] =
				(int32_t)instruction->literal_bits;
		}
		defined_value_count++;
	}

	if (defined_value_count != kernel->value_count)
		return accel_ir_error(error, error_size, N_TR("incorrect value count"));

	if (error != NULL && error_size != 0)
		error[0] = '\0';

	return true;
}

/* Copy a stable validation error into the caller's optional buffer. */
static bool
accel_ir_error(
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

/* Test whether a value type belongs to the private IR type set. */
static bool
accel_ir_value_type_valid(
	int type)
{
	if (type == ACCEL_IR_VOID)
		return true;
	if (type == ACCEL_IR_BOOL)
		return true;
	if (type == ACCEL_IR_I32)
		return true;
	if (type == ACCEL_IR_F32)
		return true;
	if (type == ACCEL_IR_INDEX_U32)
		return true;

	return false;
}

/* Test whether a value type can represent a source scalar. */
static bool
accel_ir_scalar_type_valid(
	int type)
{
	if (type == ACCEL_IR_BOOL)
		return true;
	if (type == ACCEL_IR_I32)
		return true;
	if (type == ACCEL_IR_F32)
		return true;

	return false;
}

/* Verify that every operand from the requested slot is unused. */
static bool
accel_ir_unused_operand(
	const struct accel_ir_instruction *instruction,
	uint32_t first)
{
	uint32_t i;

	/* Check the remaining fixed operand slots. */
	for (i = first; i < 3; i++) {
		if (instruction->operand[i] != ACCEL_IR_VALUE_NONE)
			return false;
	}

	return true;
}

/* Validate one operand reference and its required type. */
static bool
accel_ir_operand(
	const struct accel_ir_instruction *instruction,
	uint32_t operand_index,
	uint32_t defined_value_count,
	const int value_type[],
	int required_type)
{
	uint32_t value;

	if (operand_index >= 3)
		return false;

	value = instruction->operand[operand_index];
	if (value >= defined_value_count)
		return false;
	if (required_type != ACCEL_IR_VOID) {
		if (value_type[value] != required_type)
			return false;
	}

	return true;
}

/* Validate whether an instruction defines exactly the next value. */
static bool
accel_ir_result(
	const struct accel_ir_instruction *instruction,
	uint32_t expected_value,
	bool has_result)
{
	if (has_result) {
		if (instruction->result_type == ACCEL_IR_VOID)
			return false;
		if (instruction->result != expected_value)
			return false;

		return true;
	}

	if (instruction->result_type != ACCEL_IR_VOID)
		return false;
	if (instruction->result != ACCEL_IR_VALUE_NONE)
		return false;

	return true;
}

/* Validate the operands and result contract of one instruction. */
static bool
accel_ir_validate_input(
	const struct accel_ir_kernel *kernel,
	const struct accel_ir_instruction *instruction,
	uint32_t defined_value_count,
	const int value_type[])
{
	int buffer_type;

	/* Validate the instruction family and its exact operand shape. */
	switch (instruction->opcode) {
	case ACCEL_IR_PARAMETER:
	case ACCEL_IR_UNIFORM:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_I32 &&
		    instruction->result_type != ACCEL_IR_F32) {
			return false;
		}
		if (instruction->reference >= kernel->scalar_binding_count)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_CONST_BOOL:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_BOOL)
			return false;
		if (instruction->literal_bits > 1)
			return false;
		if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_CONST_I32:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_I32)
			return false;
		if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_CONST_F32:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_F32)
			return false;
		if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_GLOBAL_INDEX:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_INDEX_U32)
			return false;
		if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_BUFFER_LOAD:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->reference >= kernel->buffer_binding_count)
			return false;
		buffer_type = kernel->buffer_value_type[instruction->reference];
		if (instruction->result_type != buffer_type)
			return false;
		if (!accel_ir_operand(
			instruction,
			0,
			defined_value_count,
			value_type,
			ACCEL_IR_INDEX_U32)) {
			return false;
		}

		return accel_ir_unused_operand(instruction, 1);
	case ACCEL_IR_BUFFER_STORE:
		if (!accel_ir_result(instruction, defined_value_count, false))
			return false;
		if (instruction->reference >= kernel->buffer_binding_count)
			return false;
		buffer_type = kernel->buffer_value_type[instruction->reference];
		if (!accel_ir_operand(
			instruction,
			0,
			defined_value_count,
			value_type,
			ACCEL_IR_INDEX_U32)) {
			return false;
		}
		if (!accel_ir_operand(
			instruction,
			1,
			defined_value_count,
			value_type,
			buffer_type)) {
			return false;
		}

		return accel_ir_unused_operand(instruction, 2);
	case ACCEL_IR_ATOMIC_ADD_I32:
		if (!accel_ir_result(instruction, defined_value_count, false))
			return false;
		if (instruction->reference == ACCEL_IR_REFERENCE_NONE)
			return false;
		if (!accel_ir_operand(
			instruction,
			0,
			defined_value_count,
			value_type,
			ACCEL_IR_I32)) {
			return false;
		}

		return accel_ir_unused_operand(instruction, 1);
	case ACCEL_IR_LOAD_RESULT_I32:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (instruction->result_type != ACCEL_IR_I32)
			return false;
		if (instruction->reference == ACCEL_IR_REFERENCE_NONE)
			return false;

		return accel_ir_unused_operand(instruction, 0);
	case ACCEL_IR_ADD:
	case ACCEL_IR_SUB:
	case ACCEL_IR_MUL:
	case ACCEL_IR_DIV_I32:
	case ACCEL_IR_MOD_I32:
	case ACCEL_IR_BIT_AND:
	case ACCEL_IR_BIT_OR:
	case ACCEL_IR_BIT_XOR:
	case ACCEL_IR_SHIFT_LEFT:
	case ACCEL_IR_SHIFT_RIGHT_LOGICAL:
		return accel_ir_validate_arithmetic(
			instruction,
			defined_value_count,
			value_type);
	case ACCEL_IR_COMPARE_EQ:
	case ACCEL_IR_COMPARE_NE:
	case ACCEL_IR_COMPARE_LT:
	case ACCEL_IR_COMPARE_LTE:
	case ACCEL_IR_COMPARE_GT:
	case ACCEL_IR_COMPARE_GTE:
		return accel_ir_validate_comparison(
			instruction,
			defined_value_count,
			value_type);
	case ACCEL_IR_SELECT:
		if (!accel_ir_result(instruction, defined_value_count, true))
			return false;
		if (!accel_ir_scalar_type_valid(instruction->result_type))
			return false;
		if (!accel_ir_operand(
			instruction,
			0,
			defined_value_count,
			value_type,
			ACCEL_IR_BOOL)) {
			return false;
		}
		if (!accel_ir_operand(
			instruction,
			1,
			defined_value_count,
			value_type,
			instruction->result_type)) {
			return false;
		}
		if (!accel_ir_operand(
			instruction,
			2,
			defined_value_count,
			value_type,
			instruction->result_type)) {
			return false;
		}
		if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
			return false;

		return true;
	default:
		return false;
	}
}

/* Validate arithmetic, bitwise, shift, and dispatch-index operations. */
static bool
accel_ir_validate_arithmetic(
	const struct accel_ir_instruction *instruction,
	uint32_t defined_value_count,
	const int value_type[])
{
	int first_type;
	int second_type;

	if (!accel_ir_result(instruction, defined_value_count, true))
		return false;
	if (!accel_ir_operand(
		instruction,
		0,
		defined_value_count,
		value_type,
		ACCEL_IR_VOID)) {
		return false;
	}
	if (!accel_ir_operand(
		instruction,
		1,
		defined_value_count,
		value_type,
		ACCEL_IR_VOID)) {
		return false;
	}
	if (!accel_ir_unused_operand(instruction, 2))
		return false;
	if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
		return false;

	first_type = value_type[instruction->operand[0]];
	second_type = value_type[instruction->operand[1]];

	if (instruction->result_type == ACCEL_IR_INDEX_U32) {
		if (instruction->opcode != ACCEL_IR_ADD &&
		    instruction->opcode != ACCEL_IR_SUB) {
			return false;
		}
		if (first_type != ACCEL_IR_INDEX_U32)
			return false;
		if (second_type != ACCEL_IR_I32)
			return false;

		return true;
	}

	if (first_type != instruction->result_type)
		return false;
	if (second_type != instruction->result_type)
		return false;

	if (instruction->opcode == ACCEL_IR_ADD ||
	    instruction->opcode == ACCEL_IR_SUB ||
	    instruction->opcode == ACCEL_IR_MUL) {
		if (instruction->result_type == ACCEL_IR_I32)
			return true;
		if (instruction->result_type == ACCEL_IR_F32)
			return true;

		return false;
	}

	if (instruction->result_type != ACCEL_IR_I32)
		return false;

	return true;
}

/* Validate signed/ordered comparison operands and their Boolean result. */
static bool
accel_ir_validate_comparison(
	const struct accel_ir_instruction *instruction,
	uint32_t defined_value_count,
	const int value_type[])
{
	int first_type;
	int second_type;

	if (!accel_ir_result(instruction, defined_value_count, true))
		return false;
	if (instruction->result_type != ACCEL_IR_BOOL)
		return false;
	if (!accel_ir_operand(
		instruction,
		0,
		defined_value_count,
		value_type,
		ACCEL_IR_VOID)) {
		return false;
	}
	if (!accel_ir_operand(
		instruction,
		1,
		defined_value_count,
		value_type,
		ACCEL_IR_VOID)) {
		return false;
	}
	if (!accel_ir_unused_operand(instruction, 2))
		return false;
	if (instruction->reference != ACCEL_IR_REFERENCE_NONE)
		return false;

	first_type = value_type[instruction->operand[0]];
	second_type = value_type[instruction->operand[1]];
	if (first_type != second_type)
		return false;
	if (first_type != ACCEL_IR_I32 &&
	    first_type != ACCEL_IR_F32 &&
	    first_type != ACCEL_IR_BOOL) {
		return false;
	}

	if (first_type == ACCEL_IR_BOOL) {
		if (instruction->opcode != ACCEL_IR_COMPARE_EQ &&
		    instruction->opcode != ACCEL_IR_COMPARE_NE) {
			return false;
		}
	}

	return true;
}

/* Grows the kernel's instruction table within the private hard limit. */
static bool
accel_ir_grow(
	struct accel_ir_builder *builder)
{
	struct accel_ir_instruction *instruction;
	struct accel_ir_kernel *kernel;
	uint32_t capacity;
	size_t size;

	kernel = builder->kernel;
	if (kernel->instruction_capacity == 0)
		capacity = 16;
	else
		capacity = kernel->instruction_capacity * 2;

	if (capacity > ACCEL_MAX_IR_INSTRUCTIONS)
		capacity = ACCEL_MAX_IR_INSTRUCTIONS;
	if (capacity <= kernel->instruction_capacity) {
		builder->limit_exceeded = true;
		return false;
	}

	size = sizeof(*instruction) * capacity;
	instruction = noct_realloc(kernel->instruction, size);
	if (instruction == NULL) {
		builder->out_of_memory = true;
		return false;
	}

	kernel->instruction = instruction;
	kernel->instruction_capacity = capacity;

	return true;
}
