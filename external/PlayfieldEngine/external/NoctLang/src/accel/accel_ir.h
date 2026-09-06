/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral typed accelerator IR.
 */

#ifndef NOCT_ACCEL_IR_H
#define NOCT_ACCEL_IR_H

#include "accel_private.h"

#define ACCEL_IR_VALUE_NONE	((uint32_t)-1)
#define ACCEL_IR_REFERENCE_NONE	((uint32_t)-1)

enum accel_ir_value_type {
	ACCEL_IR_VOID,
	ACCEL_IR_BOOL,
	ACCEL_IR_I32,
	ACCEL_IR_F32,
	ACCEL_IR_INDEX_U32
};

enum accel_ir_opcode {
	ACCEL_IR_PARAMETER,
	ACCEL_IR_UNIFORM,
	ACCEL_IR_CONST_BOOL,
	ACCEL_IR_CONST_I32,
	ACCEL_IR_CONST_F32,
	ACCEL_IR_GLOBAL_INDEX,
	ACCEL_IR_BUFFER_LOAD,
	ACCEL_IR_BUFFER_STORE,
	ACCEL_IR_ADD,
	ACCEL_IR_SUB,
	ACCEL_IR_MUL,
	ACCEL_IR_DIV_I32,
	ACCEL_IR_MOD_I32,
	ACCEL_IR_BIT_AND,
	ACCEL_IR_BIT_OR,
	ACCEL_IR_BIT_XOR,
	ACCEL_IR_SHIFT_LEFT,
	ACCEL_IR_SHIFT_RIGHT_LOGICAL,
	ACCEL_IR_COMPARE_EQ,
	ACCEL_IR_COMPARE_NE,
	ACCEL_IR_COMPARE_LT,
	ACCEL_IR_COMPARE_LTE,
	ACCEL_IR_COMPARE_GT,
	ACCEL_IR_COMPARE_GTE,
	ACCEL_IR_SELECT,
	ACCEL_IR_ATOMIC_ADD_I32,
	ACCEL_IR_LOAD_RESULT_I32
};

struct accel_ir_instruction {
	int opcode;
	int result_type;
	uint32_t result;
	uint32_t operand[3];
	uint32_t reference;
	uint32_t literal_bits;
};

struct accel_ir_kernel {
	char *name;
	int source_line;
	int loop_block_id;
	uint32_t scalar_binding_count;
	uint32_t buffer_binding_count;
	int buffer_value_type[ACCEL_MAX_BUFFER_BINDINGS];
	uint32_t value_count;
	uint32_t instruction_count;
	uint32_t instruction_capacity;
	struct accel_ir_instruction *instruction;
};

struct accel_ir_builder {
	struct accel_ir_kernel *kernel;
	bool limit_exceeded;
	bool out_of_memory;
};

/*
 * Allocates an empty typed kernel.
 */
struct accel_ir_kernel *
accel_ir_kernel_create(
	const char *name,
	int source_line,
	int loop_block_id,
	uint32_t scalar_binding_count,
	uint32_t buffer_binding_count);

/*
 * Clones a typed kernel without retaining borrowed input storage.
 */
struct accel_ir_kernel *
accel_ir_kernel_clone(
	const struct accel_ir_kernel *kernel);

/*
 * Destroys a typed kernel and all storage it owns.
 */
void
accel_ir_kernel_destroy(
	struct accel_ir_kernel *kernel);

/*
 * Initializes an instruction builder for one kernel.
 */
void
accel_ir_builder_init(
	struct accel_ir_builder *builder,
	struct accel_ir_kernel *kernel);

/*
 * Records the scalar value type stored by one buffer binding.
 */
bool
accel_ir_kernel_set_buffer_type(
	struct accel_ir_kernel *kernel,
	uint32_t buffer_binding,
	int value_type);

/*
 * Appends one instruction and assigns its deterministic result value.
 */
bool
accel_ir_builder_append(
	struct accel_ir_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t *result_value);

/*
 * Validates one kernel's typed, use-before-definition-safe instruction stream.
 */
bool
accel_ir_kernel_validate(
	const struct accel_ir_kernel *kernel,
	char *error,
	size_t error_size);

#endif
