/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Deterministic GLSL ES and HLSL compute-shader generation.
 */

#include "accel_shader_source.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_SHADER_LOCAL_SIZE	64U
#define ACCEL_SHADER_FRAGMENT_SIZE	512U

struct accel_shader_text {
	char *data;
	size_t length;
	size_t capacity;
	bool out_of_memory;
	bool format_error;
};

struct accel_shader_source_builder {
	int dialect;
	const struct accel_program *program;
	const struct accel_kernel_plan *kernel;
	struct accel_shader_text text;
	int value_type[ACCEL_MAX_IR_VALUES];
};

static bool accel_shader_source_error(char *error, size_t error_size, const char *message);
static bool accel_shader_source_validate(int dialect, const struct accel_program *program, uint32_t kernel_index, char *error, size_t error_size);
static bool accel_shader_text_reserve(struct accel_shader_text *text, size_t additional);
static bool accel_shader_text_append(struct accel_shader_text *text, const char *format, ...);
static void accel_shader_text_cleanup(struct accel_shader_text *text);
static bool accel_shader_source_build(struct accel_shader_source_builder *builder);
static bool accel_shader_source_build_preamble(struct accel_shader_source_builder *builder);
static bool accel_shader_source_build_bindings(struct accel_shader_source_builder *builder);
static bool accel_shader_source_build_entry(struct accel_shader_source_builder *builder);
static bool accel_shader_source_build_msl_entry(struct accel_shader_source_builder *builder, uint32_t range_word);
static bool accel_shader_source_build_instruction(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction, uint32_t instruction_index);
static bool accel_shader_source_build_input(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_constant(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_load(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_store(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_atomic_add(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction, uint32_t instruction_index);
static bool accel_shader_source_build_result_load(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_arithmetic(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_float_arithmetic(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction, const char *operation);
static bool accel_shader_source_build_comparison(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_source_build_select(struct accel_shader_source_builder *builder, const struct accel_ir_instruction *instruction);
static const char *accel_shader_source_operator(int opcode);

/*
 * Generates one deterministic textual compute shader.
 *
 * Numeric SSA values and storage-buffer elements remain raw 32-bit words.
 * The generated shader converts words only at typed operation boundaries.
 */
bool
accel_shader_source_generate(
	int dialect,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_shader_source *result,
	char *error,
	size_t error_size)
{
	struct accel_shader_source_builder builder;
	bool success;

	/* Reject a missing output before attempting to clear it. */
	if (result == NULL)
		return accel_shader_source_error(error, error_size, N_TR("missing shader output"));

	result->data = NULL;
	result->length = 0;

	/* Validate the complete source-generation boundary. */
	success = accel_shader_source_validate(
		dialect,
		program,
		kernel_index,
		error,
		error_size);
	if (!success)
		return false;

	/* Initialize the target-specific builder and SSA type table. */
	memset(&builder, 0, sizeof(builder));
	builder.dialect = dialect;
	builder.program = program;
	builder.kernel = &program->kernel[kernel_index];

	/* Generate every source section in deterministic order. */
	success = accel_shader_source_build(&builder);
	if (!success) {
		if (builder.text.out_of_memory) {
			accel_shader_source_error(
				error,
				error_size,
				N_TR("out of memory while generating shader source"));
		} else if (builder.text.format_error) {
			accel_shader_source_error(
				error,
				error_size,
				N_TR("shader source fragment exceeded its limit"));
		} else {
			accel_shader_source_error(
				error,
				error_size,
				N_TR("unsupported typed shader instruction"));
		}
		accel_shader_text_cleanup(&builder.text);
		return false;
	}

	/* Transfer the completed NUL-terminated source to the caller. */
	result->data = builder.text.data;
	result->length = builder.text.length;
	builder.text.data = NULL;
	builder.text.length = 0;
	builder.text.capacity = 0;

	/* Clear the caller's optional diagnostic on success. */
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	/* Report successful source generation. */
	return true;
}

/*
 * Releases one generated shader source and clears its fields.
 */
void
accel_shader_source_cleanup(
	struct accel_shader_source *source)
{
	/* Accept cleanup of an optional result. */
	if (source == NULL)
		return;

	/* Release the owned source and clear its public state. */
	noct_free(source->data);
	source->data = NULL;
	source->length = 0;
}

/* Copy one stable diagnostic into the caller's optional buffer. */
static bool
accel_shader_source_error(
	char *error,
	size_t error_size,
	const char *message)
{
	/* Publish a terminated diagnostic when storage is available. */
	if (error != NULL && error_size != 0) {
		strncpy(error, message, error_size - 1);
		error[error_size - 1] = '\0';
	}

	/* Report the failed operation. */
	return false;
}

/* Validate one program/kernel boundary without requiring backend objects. */
static bool
accel_shader_source_validate(
	int dialect,
	const struct accel_program *program,
	uint32_t kernel_index,
	char *error,
	size_t error_size)
{
	const struct accel_kernel_plan *kernel;
	const struct accel_ir_instruction *instruction;
	char ir_error[128];
	uint32_t i;

	/* Accept only the source dialects implemented by this module. */
	if (dialect != ACCEL_SHADER_SOURCE_GLSL_ES_310 &&
	    dialect != ACCEL_SHADER_SOURCE_HLSL &&
	    dialect != ACCEL_SHADER_SOURCE_MSL) {
		return accel_shader_source_error(
			error,
			error_size,
			N_TR("unsupported shader source dialect"));
	}

	/* Require a bounded program and a present kernel table. */
	if (program == NULL)
		return accel_shader_source_error(error, error_size, N_TR("missing accelerator program"));
	if (program->scalar_count > ACCEL_MAX_SCALAR_BINDINGS)
		return accel_shader_source_error(error, error_size, N_TR("scalar binding limit exceeded"));
	if (program->buffer_count > ACCEL_MAX_BUFFER_BINDINGS)
		return accel_shader_source_error(error, error_size, N_TR("buffer binding limit exceeded"));
	if (program->scalar_result_count > ACCEL_MAX_SCALAR_BINDINGS)
		return accel_shader_source_error(error, error_size, N_TR("scalar result limit exceeded"));
	if (program->scalar_result_count != 0 && program->scalar_result == NULL)
		return accel_shader_source_error(error, error_size, N_TR("missing scalar result table"));
	if (program->kernel_count > ACCEL_MAX_KERNELS)
		return accel_shader_source_error(error, error_size, N_TR("kernel limit exceeded"));
	if (kernel_index >= program->kernel_count)
		return accel_shader_source_error(error, error_size, N_TR("invalid shader kernel index"));
	if (program->kernel == NULL)
		return accel_shader_source_error(error, error_size, N_TR("missing accelerator kernel table"));

	/* Match the selected typed kernel to its program binding namespace. */
	kernel = &program->kernel[kernel_index];
	if (kernel->kernel_index != kernel_index)
		return accel_shader_source_error(error, error_size, N_TR("nondeterministic shader kernel index"));
	if (kernel->ir == NULL)
		return accel_shader_source_error(error, error_size, N_TR("missing typed shader kernel"));
	if (kernel->ir->scalar_binding_count != program->scalar_count)
		return accel_shader_source_error(error, error_size, N_TR("shader scalar table mismatch"));
	if (kernel->ir->buffer_binding_count != program->buffer_count)
		return accel_shader_source_error(error, error_size, N_TR("shader buffer table mismatch"));

	/* Validate the complete use-before-definition-safe typed stream. */
	if (!accel_ir_kernel_validate(kernel->ir, ir_error, sizeof(ir_error)))
		return accel_shader_source_error(error, error_size, ir_error);

	/* Bound every result instruction against the program result table. */
	for (i = 0; i < kernel->ir->instruction_count; i++) {
		instruction = &kernel->ir->instruction[i];
		if (instruction->opcode != ACCEL_IR_ATOMIC_ADD_I32 &&
		    instruction->opcode != ACCEL_IR_LOAD_RESULT_I32) {
			continue;
		}
		if (instruction->reference >= program->scalar_result_count) {
			return accel_shader_source_error(
				error,
				error_size,
				N_TR("scalar result reference out of range"));
		}
	}

	/* Report a valid source-generation boundary. */
	return true;
}

/* Grow one source buffer without losing its current allocation. */
static bool
accel_shader_text_reserve(
	struct accel_shader_text *text,
	size_t additional)
{
	char *data;
	size_t required;
	size_t capacity;

	/* Preserve a prior allocation failure. */
	if (text->out_of_memory)
		return false;

	/* Reject a wrapped source length. */
	if (additional > (size_t)-1 - text->length - 1) {
		text->out_of_memory = true;
		return false;
	}

	required = text->length + additional + 1;

	/* Reuse the current allocation when it already fits. */
	if (required <= text->capacity)
		return true;

	/* Grow geometrically while retaining an exact overflow guard. */
	capacity = text->capacity;
	if (capacity == 0)
		capacity = 1024;
	while (capacity < required) {
		if (capacity > (size_t)-1 / 2) {
			capacity = required;
			break;
		}
		capacity *= 2;
	}

	/* Replace the allocation only after successful growth. */
	data = noct_realloc(text->data, capacity);
	if (data == NULL) {
		text->out_of_memory = true;
		return false;
	}

	text->data = data;
	text->capacity = capacity;

	/* Report sufficient source storage. */
	return true;
}

/* Append one bounded formatted fragment to a source buffer. */
static bool
accel_shader_text_append(
	struct accel_shader_text *text,
	const char *format,
	...)
{
	char fragment[ACCEL_SHADER_FRAGMENT_SIZE];
	va_list arguments;
	int length;

	/* Format one deliberately small generated-language fragment. */
	va_start(arguments, format);
	length = vsnprintf(fragment, sizeof(fragment), format, arguments);
	va_end(arguments);
	if (length < 0 || (size_t)length >= sizeof(fragment)) {
		text->format_error = true;
		return false;
	}

	/* Reserve and append the complete fragment. */
	if (!accel_shader_text_reserve(text, (size_t)length))
		return false;
	memcpy(text->data + text->length, fragment, (size_t)length + 1);
	text->length += (size_t)length;

	/* Report a complete append. */
	return true;
}

/* Release one temporary source buffer. */
static void
accel_shader_text_cleanup(
	struct accel_shader_text *text)
{
	/* Accept cleanup of an optional buffer. */
	if (text == NULL)
		return;

	/* Release all owned text state. */
	noct_free(text->data);
	memset(text, 0, sizeof(*text));
}

/* Build all textual shader sections in stable order. */
static bool
accel_shader_source_build(
	struct accel_shader_source_builder *builder)
{
	const struct accel_ir_kernel *ir;
	const struct accel_ir_instruction *instruction;
	uint32_t i;

	ir = builder->kernel->ir;

	/* Cache each validated SSA result type by deterministic value number. */
	for (i = 0; i < ir->instruction_count; i++) {
		instruction = &ir->instruction[i];
		if (instruction->result_type != ACCEL_IR_VOID)
			builder->value_type[instruction->result] = instruction->result_type;
	}

	/* Emit the language preamble and resource binding declarations. */
	if (!accel_shader_source_build_preamble(builder))
		return false;
	if (!accel_shader_source_build_bindings(builder))
		return false;
	if (!accel_shader_source_build_entry(builder))
		return false;

	/* Report a complete deterministic source module. */
	return true;
}

/* Build one dialect's compute-language preamble. */
static bool
accel_shader_source_build_preamble(
	struct accel_shader_source_builder *builder)
{
	bool success;

	/* Select the exact language and workgroup declaration. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"#version 310 es\n"
			"precision highp float;\n"
			"precision highp int;\n"
			"layout(local_size_x = %u, local_size_y = 1, local_size_z = 1) in;\n\n",
			ACCEL_SHADER_LOCAL_SIZE);
	} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
		success = accel_shader_text_append(
			&builder->text,
			"// Noct target-neutral compute shader.\n\n");
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"#include <metal_stdlib>\n"
			"using namespace metal;\n\n");
	}

	/* Report the preamble append result. */
	return success;
}

/* Build raw-word buffer and scalar resource bindings. */
static bool
accel_shader_source_build_bindings(
	struct accel_shader_source_builder *builder)
{
	uint32_t i;
	bool success;

	/* Keep MSL resources in the compute kernel's argument list. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_MSL)
		return true;

	/* Declare each program buffer in deterministic binding order. */
	for (i = 0; i < builder->program->buffer_count; i++) {
		if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
			success = accel_shader_text_append(
				&builder->text,
				"layout(std430, binding = %u) buffer NoctBuffer%u { uint word[]; } noct_buffer_%u;\n",
				i,
				i,
				i);
		} else {
			success = accel_shader_text_append(
				&builder->text,
				"RWStructuredBuffer<uint> noct_buffer_%u : register(u%u);\n",
				i,
				i);
		}
		if (!success)
			return false;
	}

	/* Bind the immutable scalar words after every data buffer. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			builder->program->scalar_result_count == 0 ?
			"layout(std430, binding = %u) readonly buffer NoctScalarBlock { uint word[]; } noct_scalar;\n\n" :
			"layout(std430, binding = %u) readonly buffer NoctScalarBlock { uint word[]; } noct_scalar;\n",
			builder->program->buffer_count);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			builder->program->scalar_result_count == 0 ?
			"RWStructuredBuffer<uint> noct_scalar : register(u%u);\n\n" :
			"RWStructuredBuffer<uint> noct_scalar : register(u%u);\n",
			builder->program->buffer_count);
	}
	if (!success)
		return false;

	/* Bind one mutable result-word block only when the program owns results. */
	if (builder->program->scalar_result_count == 0)
		return true;
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"layout(std430, binding = %u) coherent buffer NoctResultBlock { uint word[]; } noct_result;\n\n",
			builder->program->buffer_count + 1);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"RWStructuredBuffer<uint> noct_result : register(u%u);\n\n",
			builder->program->buffer_count + 1);
	}

	/* Report the scalar binding result. */
	return success;
}

/* Build the guarded one-dimensional compute entry point. */
static bool
accel_shader_source_build_entry(
	struct accel_shader_source_builder *builder)
{
	const struct accel_ir_kernel *ir;
	uint32_t range_word;
	uint32_t i;
	bool success;

	ir = builder->kernel->ir;
	range_word = builder->program->scalar_count +
		builder->kernel->kernel_index * 2;

	/* Open the dialect-specific compute entry point. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"void main(void)\n"
			"{\n"
			"\tuint noct_lane = gl_GlobalInvocationID.x;\n"
			"\tuint noct_start = noct_scalar.word[%u];\n"
			"\tuint noct_trip = noct_scalar.word[%u];\n"
			"\tif (noct_lane >= noct_trip)\n"
			"\t\treturn;\n"
			"\t// noct_start is supplied for the shared start/trip ABI; typed IR uses the lane.\n",
			range_word,
			range_word + 1);
	} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
		success = accel_shader_text_append(
			&builder->text,
			"[numthreads(%u, 1, 1)]\n"
			"void main(uint3 noct_dispatch_id : SV_DispatchThreadID)\n"
			"{\n"
			"\tuint noct_lane = noct_dispatch_id.x;\n"
			"\tuint noct_start = noct_scalar[%u];\n"
			"\tuint noct_trip = noct_scalar[%u];\n"
			"\tif (noct_lane >= noct_trip)\n"
			"\t\treturn;\n"
			"\t// noct_start is supplied for the shared start/trip ABI; typed IR uses the lane.\n",
			ACCEL_SHADER_LOCAL_SIZE,
			range_word,
			range_word + 1);
	} else {
		success = accel_shader_source_build_msl_entry(
			builder,
			range_word);
	}
	if (!success)
		return false;

	/* Lower every typed instruction in SSA definition order. */
	for (i = 0; i < ir->instruction_count; i++) {
		if (!accel_shader_source_build_instruction(
			builder,
			&ir->instruction[i],
			i)) {
			return false;
		}
	}

	/* Close the single compute entry point. */
	success = accel_shader_text_append(&builder->text, "}\n");

	/* Report the completed entry point. */
	return success;
}

/* Open one MSL kernel with deterministic buffer argument bindings. */
static bool
accel_shader_source_build_msl_entry(
	struct accel_shader_source_builder *builder,
	uint32_t range_word)
{
	uint32_t i;
	bool success;

	/* Begin the fixed Metal compute entry point. */
	success = accel_shader_text_append(
		&builder->text,
		"[[max_total_threads_per_threadgroup(%u)]]\n"
		"kernel void noct_main(\n",
		ACCEL_SHADER_LOCAL_SIZE);
	if (!success)
		return false;

	/* Publish every raw-word program buffer in binding order. */
	for (i = 0; i < builder->program->buffer_count; i++) {
		success = accel_shader_text_append(
			&builder->text,
			"\tdevice uint *noct_buffer_%u [[buffer(%u)]],\n",
			i,
			i);
		if (!success)
			return false;
	}

	/* Bind immutable scalar words followed by the dispatch position. */
	if (builder->program->scalar_result_count == 0) {
		success = accel_shader_text_append(
			&builder->text,
			"\tdevice const uint *noct_scalar [[buffer(%u)]],\n"
			"\tuint3 noct_dispatch_id [[thread_position_in_grid]])\n"
			"{\n"
			"\tuint noct_lane = noct_dispatch_id.x;\n"
			"\tuint noct_start = noct_scalar[%u];\n"
			"\tuint noct_trip = noct_scalar[%u];\n"
			"\tif (noct_lane >= noct_trip)\n"
			"\t\treturn;\n"
			"\t// noct_start is supplied for the shared start/trip ABI; typed IR uses the lane.\n",
			builder->program->buffer_count,
			range_word,
			range_word + 1);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tdevice const uint *noct_scalar [[buffer(%u)]],\n"
			"\tdevice atomic_uint *noct_result [[buffer(%u)]],\n"
			"\tuint3 noct_dispatch_id [[thread_position_in_grid]])\n"
			"{\n"
			"\tuint noct_lane = noct_dispatch_id.x;\n"
			"\tuint noct_start = noct_scalar[%u];\n"
			"\tuint noct_trip = noct_scalar[%u];\n"
			"\tif (noct_lane >= noct_trip)\n"
			"\t\treturn;\n"
			"\t// noct_start is supplied for the shared start/trip ABI; typed IR uses the lane.\n",
			builder->program->buffer_count,
			builder->program->buffer_count + 1,
			range_word,
			range_word + 1);
	}

	/* Report the MSL entry declaration result. */
	return success;
}

/* Lower one typed target-neutral instruction to source text. */
static bool
accel_shader_source_build_instruction(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t instruction_index)
{
	bool success;

	UNUSED_PARAMETER(instruction_index);

	/* Select the exact lowering for this validated opcode. */
	switch (instruction->opcode) {
	case ACCEL_IR_PARAMETER:
	case ACCEL_IR_UNIFORM:
		success = accel_shader_source_build_input(builder, instruction);
		break;
	case ACCEL_IR_CONST_BOOL:
	case ACCEL_IR_CONST_I32:
	case ACCEL_IR_CONST_F32:
		success = accel_shader_source_build_constant(builder, instruction);
		break;
	case ACCEL_IR_GLOBAL_INDEX:
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_lane;\n",
			instruction->result);
		break;
	case ACCEL_IR_BUFFER_LOAD:
		success = accel_shader_source_build_load(builder, instruction);
		break;
	case ACCEL_IR_BUFFER_STORE:
		success = accel_shader_source_build_store(builder, instruction);
		break;
	case ACCEL_IR_ATOMIC_ADD_I32:
		success = accel_shader_source_build_atomic_add(
			builder,
			instruction,
			instruction_index);
		break;
	case ACCEL_IR_LOAD_RESULT_I32:
		success = accel_shader_source_build_result_load(builder, instruction);
		break;
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
		success = accel_shader_source_build_arithmetic(builder, instruction);
		break;
	case ACCEL_IR_COMPARE_EQ:
	case ACCEL_IR_COMPARE_NE:
	case ACCEL_IR_COMPARE_LT:
	case ACCEL_IR_COMPARE_LTE:
	case ACCEL_IR_COMPARE_GT:
	case ACCEL_IR_COMPARE_GTE:
		success = accel_shader_source_build_comparison(builder, instruction);
		break;
	case ACCEL_IR_SELECT:
		success = accel_shader_source_build_select(builder, instruction);
		break;
	default:
		success = false;
		break;
	}

	/* Report the selected instruction lowering. */
	return success;
}

/* Lower one parameter or uniform raw-word load. */
static bool
accel_shader_source_build_input(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool success;

	/* Load the scalar word through the dialect's resource syntax. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_scalar.word[%u];\n",
			instruction->result,
			instruction->reference);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_scalar[%u];\n",
			instruction->result,
			instruction->reference);
	}

	/* Report the scalar load result. */
	return success;
}

/* Lower one exact Boolean or raw numeric constant. */
static bool
accel_shader_source_build_constant(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool success;

	/* Preserve Boolean values as native predicates. */
	if (instruction->opcode == ACCEL_IR_CONST_BOOL) {
		success = accel_shader_text_append(
			&builder->text,
			"\tbool v%u = %s;\n",
			instruction->result,
			instruction->literal_bits != 0 ? "true" : "false");

		/* Report the Boolean constant append. */
		return success;
	}

	/* Preserve I32 and F32 constants as exact hexadecimal words. */
	success = accel_shader_text_append(
		&builder->text,
		"\tuint v%u = 0x%08Xu;\n",
		instruction->result,
		(unsigned int)instruction->literal_bits);

	/* Report the raw constant append. */
	return success;
}

/* Lower one typed raw-word storage-buffer load. */
static bool
accel_shader_source_build_load(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool success;

	/* Load one word through the dialect's storage-buffer syntax. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_buffer_%u.word[v%u];\n",
			instruction->result,
			instruction->reference,
			instruction->operand[0]);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_buffer_%u[v%u];\n",
			instruction->result,
			instruction->reference,
			instruction->operand[0]);
	}

	/* Report the storage load result. */
	return success;
}

/* Lower one typed raw-word storage-buffer store. */
static bool
accel_shader_source_build_store(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool success;

	/* Store one word through the dialect's storage-buffer syntax. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tnoct_buffer_%u.word[v%u] = v%u;\n",
			instruction->reference,
			instruction->operand[0],
			instruction->operand[1]);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tnoct_buffer_%u[v%u] = v%u;\n",
			instruction->reference,
			instruction->operand[0],
			instruction->operand[1]);
	}

	/* Report the storage store result. */
	return success;
}

/* Lower one wrapping I32 contribution into the shared result word. */
static bool
accel_shader_source_build_atomic_add(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t instruction_index)
{
	bool success;

	/* Select each dialect's raw-word atomic-add operation. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tatomicAdd(noct_result.word[%u], v%u);\n",
			instruction->reference,
			instruction->operand[0]);
	} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint noct_atomic_old_%u;\n"
			"\tInterlockedAdd(noct_result[%u], v%u, noct_atomic_old_%u);\n",
			instruction_index,
			instruction->reference,
			instruction->operand[0],
			instruction_index);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tatomic_fetch_add_explicit(&noct_result[%u], v%u, memory_order_relaxed);\n",
			instruction->reference,
			instruction->operand[0]);
	}

	/* Report the scalar-result contribution lowering. */
	return success;
}

/* Lower one raw scalar-result load for a later kernel. */
static bool
accel_shader_source_build_result_load(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool success;

	/* Load one result word through the dialect's resource syntax. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_result.word[%u];\n",
			instruction->result,
			instruction->reference);
	} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = noct_result[%u];\n",
			instruction->result,
			instruction->reference);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tuint v%u = atomic_load_explicit(&noct_result[%u], memory_order_relaxed);\n",
			instruction->result,
			instruction->reference);
	}

	/* Report the scalar-result load lowering. */
	return success;
}

/* Lower one arithmetic, bitwise, shift, or index operation. */
static bool
accel_shader_source_build_arithmetic(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	const char *operation;
	bool success;

	/* Keep floating arithmetic typed before converting it back to bits. */
	if (instruction->result_type == ACCEL_IR_F32) {
		operation = accel_shader_source_operator(instruction->opcode);
		if (operation == NULL)
			return false;

		success = accel_shader_source_build_float_arithmetic(
			builder,
			instruction,
			operation);

		/* Report the floating arithmetic lowering. */
		return success;
	}

	/* Apply signed division and remainder only at the operation boundary. */
	if (instruction->opcode == ACCEL_IR_DIV_I32 ||
	    instruction->opcode == ACCEL_IR_MOD_I32) {
		operation = accel_shader_source_operator(instruction->opcode);
		if (operation == NULL)
			return false;
		if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
			success = accel_shader_text_append(
				&builder->text,
				"\tuint v%u = uint(int(v%u) %s int(v%u));\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
			success = accel_shader_text_append(
				&builder->text,
				"\tuint v%u = asuint(asint(v%u) %s asint(v%u));\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else {
			success = accel_shader_text_append(
				&builder->text,
				"\tuint v%u = as_type<uint>(as_type<int>(v%u) %s as_type<int>(v%u));\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		}

		/* Report the signed arithmetic lowering. */
		return success;
	}

	/* Keep wrapping integer, index, bitwise, and shift operations unsigned. */
	operation = accel_shader_source_operator(instruction->opcode);
	if (operation == NULL)
		return false;
	success = accel_shader_text_append(
		&builder->text,
		"\tuint v%u = v%u %s v%u;\n",
		instruction->result,
		instruction->operand[0],
		operation,
		instruction->operand[1]);

	/* Report the unsigned word arithmetic lowering. */
	return success;
}

/* Lower one Float32 arithmetic operation through raw words. */
static bool
accel_shader_source_build_float_arithmetic(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction,
	const char *operation)
{
	bool success;

	/* Convert operands, evaluate at high precision, and restore the result word. */
	if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
		success = accel_shader_text_append(
			&builder->text,
			"\tfloat noct_f%u = uintBitsToFloat(v%u) %s uintBitsToFloat(v%u);\n"
			"\tuint v%u = floatBitsToUint(noct_f%u);\n",
			instruction->result,
			instruction->operand[0],
			operation,
			instruction->operand[1],
			instruction->result,
			instruction->result);
	} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
		success = accel_shader_text_append(
			&builder->text,
			"\tprecise float noct_f%u = asfloat(v%u) %s asfloat(v%u);\n"
			"\tuint v%u = asuint(noct_f%u);\n",
			instruction->result,
			instruction->operand[0],
			operation,
			instruction->operand[1],
			instruction->result,
			instruction->result);
	} else {
		success = accel_shader_text_append(
			&builder->text,
			"\tfloat noct_f%u = as_type<float>(v%u) %s as_type<float>(v%u);\n"
			"\tuint v%u = as_type<uint>(noct_f%u);\n",
			instruction->result,
			instruction->operand[0],
			operation,
			instruction->operand[1],
			instruction->result,
			instruction->result);
	}

	/* Report the Float32 lowering. */
	return success;
}

/* Lower one signed, ordered, unsigned, or Boolean comparison. */
static bool
accel_shader_source_build_comparison(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	const char *operation;
	int operand_type;
	bool success;

	operation = accel_shader_source_operator(instruction->opcode);
	if (operation == NULL)
		return false;
	operand_type = builder->value_type[instruction->operand[0]];

	/* Compare signed I32 words through explicit signed views. */
	if (operand_type == ACCEL_IR_I32) {
		if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = int(v%u) %s int(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = asint(v%u) %s asint(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = as_type<int>(v%u) %s as_type<int>(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		}

		/* Report the signed comparison lowering. */
		return success;
	}

	/* Compare Float32 words through explicit floating views. */
	if (operand_type == ACCEL_IR_F32) {
		if (builder->dialect == ACCEL_SHADER_SOURCE_GLSL_ES_310) {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = uintBitsToFloat(v%u) %s uintBitsToFloat(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else if (builder->dialect == ACCEL_SHADER_SOURCE_HLSL) {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = asfloat(v%u) %s asfloat(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		} else {
			success = accel_shader_text_append(
				&builder->text,
				"\tbool v%u = as_type<float>(v%u) %s as_type<float>(v%u);\n",
				instruction->result,
				instruction->operand[0],
				operation,
				instruction->operand[1]);
		}

		/* Report the ordered floating comparison lowering. */
		return success;
	}

	/* Compare native Boolean predicates without word conversion. */
	if (operand_type == ACCEL_IR_BOOL) {
		success = accel_shader_text_append(
			&builder->text,
			"\tbool v%u = v%u %s v%u;\n",
			instruction->result,
			instruction->operand[0],
			operation,
			instruction->operand[1]);

		/* Report the Boolean comparison lowering. */
		return success;
	}

	/* Reject a comparison type outside the validated scalar contract. */
	return false;
}

/* Lower one predicate-selected scalar value. */
static bool
accel_shader_source_build_select(
	struct accel_shader_source_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	const char *type;
	bool success;

	/* Preserve predicates as bool and all numeric values as raw words. */
	if (instruction->result_type == ACCEL_IR_BOOL)
		type = "bool";
	else
		type = "uint";
	success = accel_shader_text_append(
		&builder->text,
		"\t%s v%u = v%u ? v%u : v%u;\n",
		type,
		instruction->result,
		instruction->operand[0],
		instruction->operand[1],
		instruction->operand[2]);

	/* Report the select lowering. */
	return success;
}

/* Return one validated opcode's source-language operator. */
static const char *
accel_shader_source_operator(
	int opcode)
{
	/* Map every operator-bearing typed opcode. */
	switch (opcode) {
	case ACCEL_IR_ADD:
		return "+";
	case ACCEL_IR_SUB:
		return "-";
	case ACCEL_IR_MUL:
		return "*";
	case ACCEL_IR_DIV_I32:
		return "/";
	case ACCEL_IR_MOD_I32:
		return "%";
	case ACCEL_IR_BIT_AND:
		return "&";
	case ACCEL_IR_BIT_OR:
		return "|";
	case ACCEL_IR_BIT_XOR:
		return "^";
	case ACCEL_IR_SHIFT_LEFT:
		return "<<";
	case ACCEL_IR_SHIFT_RIGHT_LOGICAL:
		return ">>";
	case ACCEL_IR_COMPARE_EQ:
		return "==";
	case ACCEL_IR_COMPARE_NE:
		return "!=";
	case ACCEL_IR_COMPARE_LT:
		return "<";
	case ACCEL_IR_COMPARE_LTE:
		return "<=";
	case ACCEL_IR_COMPARE_GT:
		return ">";
	case ACCEL_IR_COMPARE_GTE:
		return ">=";
	default:
		return NULL;
	}
}
