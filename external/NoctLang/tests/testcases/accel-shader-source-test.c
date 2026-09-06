/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Focused target-neutral textual shader generator tests.
 */

#include "accel_shader_source.h"

#include <stdio.h>
#include <string.h>

static bool add_instruction(struct accel_ir_builder *builder, int opcode, int result_type, uint32_t reference, uint32_t literal_bits, uint32_t operand0, uint32_t operand1, uint32_t operand2);
static struct accel_ir_kernel *build_all_opcode_kernel(void);
static bool build_program(struct accel_program *program, struct accel_kernel_plan *plan, struct accel_ir_kernel **kernel);
static bool expect_text(const char *source, const char *fragment, const char *description);
static bool test_glsl(const struct accel_program *program);
static bool test_hlsl(const struct accel_program *program);
static bool test_msl(const struct accel_program *program);
static bool test_deterministic(const struct accel_program *program);
static bool test_invalid_input(struct accel_program *program);
static struct accel_program *build_scalar_result_program(void);
static bool test_scalar_result_sources(const struct accel_program *program);
static bool test_scalar_result_validation(const struct accel_program *program);
static bool test_scalar_results(void);
static bool print_msl(const struct accel_program *program);
static bool print_scalar_result_source(int dialect, uint32_t kernel_index);

/*
 * Runs the target-neutral textual shader generator tests.
 */
int
main(
	int argc,
	char *argv[])
{
	struct accel_program program;
	struct accel_kernel_plan plan;
	struct accel_ir_kernel *kernel;
	bool success;

	/* Build one valid program containing every current typed IR opcode. */
	success = build_program(&program, &plan, &kernel);
	if (!success) {
		fprintf(stderr, "failed to build shader test program\n");
		return 1;
	}

	/* Emit a native compiler fixture when explicitly requested. */
	if (argc == 2 && strcmp(argv[1], "--print-msl") == 0) {
		success = print_msl(&program);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-msl-result-producer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_MSL, 0);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-msl-result-consumer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_MSL, 1);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-glsl-result-producer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_GLSL_ES_310, 0);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-glsl-result-consumer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_GLSL_ES_310, 1);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-hlsl-result-producer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_HLSL, 0);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}
	if (argc == 2 &&
	    strcmp(argv[1], "--print-hlsl-result-consumer") == 0) {
		success = print_scalar_result_source(ACCEL_SHADER_SOURCE_HLSL, 1);
		accel_ir_kernel_destroy(kernel);
		return success ? 0 : 1;
	}

	/* Exercise both dialects, determinism, and invalid boundary handling. */
	success = test_glsl(&program);
	if (success)
		success = test_hlsl(&program);
	if (success)
		success = test_msl(&program);
	if (success)
		success = test_deterministic(&program);
	if (success)
		success = test_invalid_input(&program);
	if (success)
		success = test_scalar_results();

	/* Release the one kernel owned by the test fixture. */
	accel_ir_kernel_destroy(kernel);

	/* Report any focused test failure. */
	if (!success)
		return 1;

	puts("PASS");

	/* Report successful focused testing. */
	return 0;
}

/* Emit the generated Metal source for native compiler validation. */
static bool
print_msl(
	const struct accel_program *program)
{
	struct accel_shader_source source;
	char error[160];
	bool success;

	/* Generate exactly the same all-opcode fixture used by focused tests. */
	success = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_MSL,
		program,
		0,
		&source,
		error,
		sizeof(error));
	if (!success) {
		fprintf(stderr, "MSL generation failed: %s\n", error);
		return false;
	}

	/* Write the source without adding bytes to the compiler input. */
	success = fwrite(source.data, source.length, 1, stdout) == 1;
	accel_shader_source_cleanup(&source);

	return success;
}

/* Emit one generated scalar-result kernel for native validation. */
static bool
print_scalar_result_source(
	int dialect,
	uint32_t kernel_index)
{
	struct accel_program *program;
	struct accel_shader_source source;
	char error[160];
	bool success;

	/* Build the same producer-consumer fixture used by focused tests. */
	program = build_scalar_result_program();
	if (program == NULL)
		return false;

	/* Generate and write the requested native Metal source. */
	success = accel_shader_source_generate(
		dialect,
		program,
		kernel_index,
		&source,
		error,
		sizeof(error));
	if (!success) {
		fprintf(stderr, "scalar-result source generation failed: %s\n", error);
		accel_program_destroy(program);
		return false;
	}

	success = fwrite(source.data, source.length, 1, stdout) == 1;
	accel_shader_source_cleanup(&source);
	accel_program_destroy(program);

	return success;
}

/* Append one fully initialized typed instruction to the test kernel. */
static bool
add_instruction(
	struct accel_ir_builder *builder,
	int opcode,
	int result_type,
	uint32_t reference,
	uint32_t literal_bits,
	uint32_t operand0,
	uint32_t operand1,
	uint32_t operand2)
{
	struct accel_ir_instruction instruction;
	bool success;

	/* Initialize every field to its explicit unused sentinel. */
	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = opcode;
	instruction.result_type = result_type;
	instruction.result = ACCEL_IR_VALUE_NONE;
	instruction.operand[0] = operand0;
	instruction.operand[1] = operand1;
	instruction.operand[2] = operand2;
	instruction.reference = reference;
	instruction.literal_bits = literal_bits;

	/* Append the instruction with deterministic result numbering. */
	success = accel_ir_builder_append(builder, &instruction, NULL);

	/* Report the append result. */
	return success;
}

/* Build one valid typed kernel containing every current opcode. */
static struct accel_ir_kernel *
build_all_opcode_kernel(
	void)
{
	struct accel_ir_kernel *kernel;
	struct accel_ir_builder builder;
	uint32_t none;
	bool success;

	none = ACCEL_IR_VALUE_NONE;

	/* Allocate and type the two-buffer test kernel. */
	kernel = accel_ir_kernel_create("all_ops", 1, 1, 2, 2);
	if (kernel == NULL)
		return NULL;
	success = accel_ir_kernel_set_buffer_type(kernel, 0, ACCEL_IR_I32);
	if (!success) {
		accel_ir_kernel_destroy(kernel);
		return NULL;
	}
	success = accel_ir_kernel_set_buffer_type(kernel, 1, ACCEL_IR_F32);
	if (!success) {
		accel_ir_kernel_destroy(kernel);
		return NULL;
	}
	accel_ir_builder_init(&builder, kernel);

	/* Define scalar inputs, exact constants, and the dispatch lane. */
	success = add_instruction(&builder, ACCEL_IR_PARAMETER, ACCEL_IR_I32, 0, 0, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_UNIFORM, ACCEL_IR_F32, 1, 0, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_CONST_BOOL, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 1, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_CONST_BOOL, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_CONST_I32, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0xfffffff9U, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_CONST_I32, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 3, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_CONST_F32, ACCEL_IR_F32, ACCEL_IR_REFERENCE_NONE, 0x3fc00000U, none, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_GLOBAL_INDEX, ACCEL_IR_INDEX_U32, ACCEL_IR_REFERENCE_NONE, 0, none, none, none);

	/* Load one signed and one Float32 raw word. */
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BUFFER_LOAD, ACCEL_IR_I32, 0, 0, 7, none, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BUFFER_LOAD, ACCEL_IR_F32, 1, 0, 7, none, none);

	/* Exercise wrapping, signed, bitwise, and logical-shift operations. */
	if (success)
		success = add_instruction(&builder, ACCEL_IR_ADD, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SUB, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_MUL, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_DIV_I32, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_MOD_I32, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BIT_AND, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BIT_OR, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BIT_XOR, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SHIFT_LEFT, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SHIFT_RIGHT_LOGICAL, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 8, 5, none);

	/* Exercise Float32 and unsigned dispatch-index arithmetic. */
	if (success)
		success = add_instruction(&builder, ACCEL_IR_ADD, ACCEL_IR_F32, ACCEL_IR_REFERENCE_NONE, 0, 9, 6, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SUB, ACCEL_IR_F32, ACCEL_IR_REFERENCE_NONE, 0, 9, 6, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_MUL, ACCEL_IR_F32, ACCEL_IR_REFERENCE_NONE, 0, 9, 6, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_ADD, ACCEL_IR_INDEX_U32, ACCEL_IR_REFERENCE_NONE, 0, 7, 5, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SUB, ACCEL_IR_INDEX_U32, ACCEL_IR_REFERENCE_NONE, 0, 7, 5, none);

	/* Exercise every comparison opcode across signed, Float32, and bool views. */
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_EQ, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_NE, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_LT, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_LTE, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_GT, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_GTE, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 8, 4, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_EQ, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 9, 6, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_NE, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 9, 6, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_EQ, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 2, 3, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_COMPARE_NE, ACCEL_IR_BOOL, ACCEL_IR_REFERENCE_NONE, 0, 2, 3, none);

	/* Select and store one result through each typed data buffer. */
	if (success)
		success = add_instruction(&builder, ACCEL_IR_SELECT, ACCEL_IR_I32, ACCEL_IR_REFERENCE_NONE, 0, 25, 10, 11);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BUFFER_STORE, ACCEL_IR_VOID, 0, 0, 7, 35, none);
	if (success)
		success = add_instruction(&builder, ACCEL_IR_BUFFER_STORE, ACCEL_IR_VOID, 1, 0, 7, 20, none);

	/* Reject any incomplete instruction stream. */
	if (!success) {
		accel_ir_kernel_destroy(kernel);
		return NULL;
	}

	/* Return the complete owned test kernel. */
	return kernel;
}

/* Build one minimal program shell around the typed test kernel. */
static bool
build_program(
	struct accel_program *program,
	struct accel_kernel_plan *plan,
	struct accel_ir_kernel **kernel)
{
	/* Construct the typed kernel before publishing the program shell. */
	*kernel = build_all_opcode_kernel();
	if (*kernel == NULL)
		return false;

	/* Initialize the deterministic program and selected kernel metadata. */
	memset(program, 0, sizeof(*program));
	memset(plan, 0, sizeof(*plan));
	program->scalar_count = 2;
	program->buffer_count = 2;
	program->kernel_count = 1;
	program->kernel = plan;
	plan->kernel_index = 0;
	plan->ir = *kernel;

	/* Report a complete test program shell. */
	return true;
}

/* Require one semantic fragment in generated shader source. */
static bool
expect_text(
	const char *source,
	const char *fragment,
	const char *description)
{
	/* Accept only a present semantic fragment. */
	if (source == NULL || strstr(source, fragment) == NULL) {
		fprintf(stderr, "missing %s: %s\n", description, fragment);
		return false;
	}

	/* Report a present semantic fragment. */
	return true;
}

/* Verify GLSL ES declarations and every sensitive word conversion. */
static bool
test_glsl(
	const struct accel_program *program)
{
	struct accel_shader_source source;
	char error[160];
	bool success;

	/* Generate one GLSL ES 3.10 compute source. */
	success = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_GLSL_ES_310,
		program,
		0,
		&source,
		error,
		sizeof(error));
	if (!success) {
		fprintf(stderr, "GLSL generation failed: %s\n", error);
		return false;
	}

	/* Verify resource, dispatch, signed, Float32, and store semantics. */
	success = expect_text(source.data, "#version 310 es", "GLSL version");
	if (success)
		success = expect_text(source.data, "binding = 2", "scalar binding");
	if (success)
		success = expect_text(source.data, "uint noct_start = noct_scalar.word[2];", "start word");
	if (success)
		success = expect_text(source.data, "uint noct_trip = noct_scalar.word[3];", "trip word");
	if (success)
		success = expect_text(source.data, "uint v13 = uint(int(v8) / int(v5));", "signed division");
	if (success)
		success = expect_text(source.data, "uint v19 = v8 >> v5;", "logical right shift");
	if (success)
		success = expect_text(source.data, "float noct_f20 = uintBitsToFloat(v9) + uintBitsToFloat(v6);", "Float32 add");
	if (success)
		success = expect_text(source.data, "bool v27 = int(v8) < int(v4);", "signed comparison");
	if (success)
		success = expect_text(source.data, "bool v31 = uintBitsToFloat(v9) == uintBitsToFloat(v6);", "Float32 comparison");
	if (success)
		success = expect_text(source.data, "noct_buffer_0.word[v7] = v35;", "raw word store");

	accel_shader_source_cleanup(&source);

	/* Report the GLSL source result. */
	return success;
}

/* Verify HLSL declarations and every sensitive word conversion. */
static bool
test_hlsl(
	const struct accel_program *program)
{
	struct accel_shader_source source;
	char error[160];
	bool success;

	/* Generate one HLSL compute source. */
	success = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_HLSL,
		program,
		0,
		&source,
		error,
		sizeof(error));
	if (!success) {
		fprintf(stderr, "HLSL generation failed: %s\n", error);
		return false;
	}

	/* Verify resource, dispatch, signed, Float32, and store semantics. */
	success = expect_text(source.data, "RWStructuredBuffer<uint> noct_scalar : register(u2);", "HLSL scalar binding");
	if (success)
		success = expect_text(source.data, "[numthreads(64, 1, 1)]", "HLSL workgroup");
	if (success)
		success = expect_text(source.data, "uint noct_start = noct_scalar[2];", "start word");
	if (success)
		success = expect_text(source.data, "uint v13 = asuint(asint(v8) / asint(v5));", "signed division");
	if (success)
		success = expect_text(source.data, "uint v19 = v8 >> v5;", "logical right shift");
	if (success)
		success = expect_text(source.data, "precise float noct_f20 = asfloat(v9) + asfloat(v6);", "precise Float32 add");
	if (success)
		success = expect_text(source.data, "bool v27 = asint(v8) < asint(v4);", "signed comparison");
	if (success)
		success = expect_text(source.data, "bool v31 = asfloat(v9) == asfloat(v6);", "Float32 comparison");
	if (success)
		success = expect_text(source.data, "noct_buffer_0[v7] = v35;", "raw word store");

	accel_shader_source_cleanup(&source);

	/* Report the HLSL source result. */
	return success;
}

/* Verify MSL bindings and every sensitive raw-word conversion. */
static bool
test_msl(
	const struct accel_program *program)
{
	struct accel_shader_source source;
	char error[160];
	bool success;

	/* Generate one Metal Shading Language compute source. */
	success = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_MSL,
		program,
		0,
		&source,
		error,
		sizeof(error));
	if (!success) {
		fprintf(stderr, "MSL generation failed: %s\n", error);
		return false;
	}

	/* Verify preamble, buffer ABI, dispatch guard, and typed word views. */
	success = expect_text(source.data, "#include <metal_stdlib>", "MSL preamble");
	if (success)
		success = expect_text(source.data, "[[max_total_threads_per_threadgroup(64)]]", "MSL threadgroup ABI");
	if (success)
		success = expect_text(source.data, "kernel void noct_main(", "MSL entry point");
	if (success)
		success = expect_text(source.data, "device uint *noct_buffer_0 [[buffer(0)]],", "MSL first buffer binding");
	if (success)
		success = expect_text(source.data, "device uint *noct_buffer_1 [[buffer(1)]],", "MSL second buffer binding");
	if (success)
		success = expect_text(source.data, "device const uint *noct_scalar [[buffer(2)]],", "MSL scalar binding");
	if (success)
		success = expect_text(source.data, "[[thread_position_in_grid]]", "MSL global position");
	if (success)
		success = expect_text(source.data, "uint noct_start = noct_scalar[2];", "MSL start word");
	if (success)
		success = expect_text(source.data, "uint noct_trip = noct_scalar[3];", "MSL trip word");
	if (success)
		success = expect_text(source.data, "uint v13 = as_type<uint>(as_type<int>(v8) / as_type<int>(v5));", "MSL signed division");
	if (success)
		success = expect_text(source.data, "uint v19 = v8 >> v5;", "MSL logical right shift");
	if (success)
		success = expect_text(source.data, "float noct_f20 = as_type<float>(v9) + as_type<float>(v6);", "MSL Float32 add");
	if (success)
		success = expect_text(source.data, "uint v20 = as_type<uint>(noct_f20);", "MSL Float32 result bits");
	if (success)
		success = expect_text(source.data, "bool v27 = as_type<int>(v8) < as_type<int>(v4);", "MSL signed comparison");
	if (success)
		success = expect_text(source.data, "bool v31 = as_type<float>(v9) == as_type<float>(v6);", "MSL Float32 comparison");
	if (success)
		success = expect_text(source.data, "noct_buffer_0[v7] = v35;", "MSL raw word store");

	accel_shader_source_cleanup(&source);

	/* Report the MSL source result. */
	return success;
}

/* Verify every dialect produces byte-identical source repeatedly. */
static bool
test_deterministic(
	const struct accel_program *program)
{
	struct accel_shader_source first;
	struct accel_shader_source second;
	int dialect[3];
	const char *name[3];
	char error[160];
	uint32_t i;
	bool success;

	/* Define the complete deterministic source dialect table. */
	dialect[0] = ACCEL_SHADER_SOURCE_GLSL_ES_310;
	dialect[1] = ACCEL_SHADER_SOURCE_HLSL;
	dialect[2] = ACCEL_SHADER_SOURCE_MSL;
	name[0] = "GLSL ES";
	name[1] = "HLSL";
	name[2] = "MSL";
	success = true;

	/* Generate and compare each source twice without shared mutable state. */
	for (i = 0; i < 3; i++) {
		success = accel_shader_source_generate(
			dialect[i],
			program,
			0,
			&first,
			error,
			sizeof(error));
		if (!success)
			return false;
		success = accel_shader_source_generate(
			dialect[i],
			program,
			0,
			&second,
			error,
			sizeof(error));
		if (!success) {
			accel_shader_source_cleanup(&first);
			return false;
		}

		/* Compare both length and complete NUL-terminated contents. */
		success = first.length == second.length;
		if (success)
			success = strcmp(first.data, second.data) == 0;
		if (!success)
			fprintf(stderr, "%s shader generation was not deterministic\n", name[i]);

		accel_shader_source_cleanup(&second);
		accel_shader_source_cleanup(&first);
		if (!success)
			return false;
	}

	/* Report the deterministic generation result. */
	return true;
}

/* Build one owned two-kernel scalar-result reduction program. */
static struct accel_program *
build_scalar_result_program(
	void)
{
	struct accel_program *program;
	struct accel_ir_kernel *producer;
	struct accel_ir_kernel *consumer;
	struct accel_ir_builder builder;
	struct accel_size_expression expression;
	struct accel_scalar_result scalar_result;
	struct accel_kernel_plan kernel;
	uint32_t expression_index;
	uint32_t ignored;
	uint32_t none;
	bool success;

	/* Allocate the owned program and its one constant range expression. */
	program = accel_program_create("result.noct", "sum", 1, 0, 0, 0, 1, 2);
	if (program == NULL)
		return NULL;

	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_CONSTANT;
	expression.value = 1;
	success = accel_program_add_size_expression(
		program,
		&expression,
		&expression_index);
	if (!success) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Publish one zero-identity I32 result produced by the first kernel. */
	memset(&scalar_result, 0, sizeof(scalar_result));
	scalar_result.name = "sum";
	scalar_result.args_slot = 0;
	scalar_result.value_type = ACCEL_IR_I32;
	scalar_result.identity_bits = 0;
	scalar_result.producer_kernel = 0;
	scalar_result.gpu_consumer_mask = (uint32_t)1U << 1;
	scalar_result.cpu_publication = true;
	success = accel_program_add_scalar_result(
		program,
		&scalar_result,
		&ignored);
	if (!success) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Build one producer with a single static atomic contribution. */
	producer = accel_ir_kernel_create("sum_producer", 1, 1, 0, 0);
	if (producer == NULL) {
		accel_program_destroy(program);
		return NULL;
	}
	accel_ir_builder_init(&builder, producer);
	none = ACCEL_IR_VALUE_NONE;
	success = add_instruction(
		&builder,
		ACCEL_IR_CONST_I32,
		ACCEL_IR_I32,
		ACCEL_IR_REFERENCE_NONE,
		7,
		none,
		none,
		none);
	if (success) {
		success = add_instruction(
			&builder,
			ACCEL_IR_ATOMIC_ADD_I32,
			ACCEL_IR_VOID,
			0,
			0,
			0,
			none,
			none);
	}
	if (!success) {
		accel_ir_kernel_destroy(producer);
		accel_program_destroy(program);
		return NULL;
	}

	/* Transfer the producer with one deterministic dispatch range. */
	memset(&kernel, 0, sizeof(kernel));
	kernel.source_line = 1;
	kernel.loop_block_id = 1;
	kernel.start_expression = expression_index;
	kernel.stop_expression = expression_index;
	kernel.trip_expression = expression_index;
	kernel.ir = producer;
	success = accel_program_add_kernel(program, &kernel, &ignored);
	if (!success) {
		accel_ir_kernel_destroy(producer);
		accel_program_destroy(program);
		return NULL;
	}

	/* Build one later consumer which loads the reduced result word. */
	consumer = accel_ir_kernel_create("sum_consumer", 2, 2, 0, 0);
	if (consumer == NULL) {
		accel_program_destroy(program);
		return NULL;
	}
	accel_ir_builder_init(&builder, consumer);
	success = add_instruction(
		&builder,
		ACCEL_IR_LOAD_RESULT_I32,
		ACCEL_IR_I32,
		0,
		0,
		none,
		none,
		none);
	if (!success) {
		accel_ir_kernel_destroy(consumer);
		accel_program_destroy(program);
		return NULL;
	}

	/* Transfer the consumer as the second deterministic kernel. */
	memset(&kernel, 0, sizeof(kernel));
	kernel.source_line = 2;
	kernel.loop_block_id = 2;
	kernel.start_expression = expression_index;
	kernel.stop_expression = expression_index;
	kernel.trip_expression = expression_index;
	kernel.ir = consumer;
	success = accel_program_add_kernel(program, &kernel, &ignored);
	if (!success) {
		accel_ir_kernel_destroy(consumer);
		accel_program_destroy(program);
		return NULL;
	}

	/* Return the complete owned scalar-result fixture. */
	return program;
}

/* Verify scalar-result bindings and operations in every source dialect. */
static bool
test_scalar_result_sources(
	const struct accel_program *program)
{
	struct accel_shader_source producer;
	struct accel_shader_source consumer;
	char error[160];
	int dialect[3];
	const char *binding[3];
	const char *atomic_add[3];
	const char *load[3];
	uint32_t i;
	bool success;

	/* Define each dialect's exact result binding and operation spelling. */
	dialect[0] = ACCEL_SHADER_SOURCE_GLSL_ES_310;
	dialect[1] = ACCEL_SHADER_SOURCE_HLSL;
	dialect[2] = ACCEL_SHADER_SOURCE_MSL;
	binding[0] = "coherent buffer NoctResultBlock";
	binding[1] = "RWStructuredBuffer<uint> noct_result : register(u1);";
	binding[2] = "device atomic_uint *noct_result [[buffer(1)]],";
	atomic_add[0] = "atomicAdd(noct_result.word[0], v0);";
	atomic_add[1] = "InterlockedAdd(noct_result[0], v0, noct_atomic_old_1);";
	atomic_add[2] = "atomic_fetch_add_explicit(&noct_result[0], v0, memory_order_relaxed);";
	load[0] = "uint v0 = noct_result.word[0];";
	load[1] = "uint v0 = noct_result[0];";
	load[2] = "uint v0 = atomic_load_explicit(&noct_result[0], memory_order_relaxed);";

	/* Generate both sides of the reduction dependency in every dialect. */
	for (i = 0; i < 3; i++) {
		success = accel_shader_source_generate(
			dialect[i],
			program,
			0,
			&producer,
			error,
			sizeof(error));
		if (!success) {
			fprintf(stderr, "scalar-result producer generation failed: %s\n", error);
			return false;
		}
		success = accel_shader_source_generate(
			dialect[i],
			program,
			1,
			&consumer,
			error,
			sizeof(error));
		if (!success) {
			accel_shader_source_cleanup(&producer);
			fprintf(stderr, "scalar-result consumer generation failed: %s\n", error);
			return false;
		}

		/* Require the shared binding and each side's exact operation. */
		success = expect_text(producer.data, binding[i], "scalar-result binding");
		if (success)
			success = expect_text(producer.data, atomic_add[i], "scalar-result atomic add");
		if (success)
			success = expect_text(consumer.data, load[i], "scalar-result load");

		accel_shader_source_cleanup(&consumer);
		accel_shader_source_cleanup(&producer);
		if (!success)
			return false;
	}

	/* Report complete source lowering coverage. */
	return true;
}

/* Verify scalar-result ownership and producer-consumer validation. */
static bool
test_scalar_result_validation(
	const struct accel_program *program)
{
	struct accel_program *clone;
	struct accel_ir_builder builder;
	struct accel_shader_source source;
	uint32_t none;
	uint32_t saved;
	uint32_t saved_instruction_count;
	uint32_t saved_value_count;
	char error[160];
	bool success;

	/* Validate and deep-clone the complete result table. */
	if (!accel_program_validate(program, error, sizeof(error))) {
		fprintf(stderr, "scalar-result program rejected: %s\n", error);
		return false;
	}
	clone = accel_program_clone(program);
	if (clone == NULL)
		return false;
	if (clone->scalar_result == program->scalar_result ||
	    clone->scalar_result[0].name == program->scalar_result[0].name) {
		accel_program_destroy(clone);
		return false;
	}
	if (!accel_program_validate(clone, error, sizeof(error))) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject a nondense result ID. */
	clone->scalar_result[0].result_entry_id = 1;
	success = accel_program_validate(clone, error, sizeof(error));
	clone->scalar_result[0].result_entry_id = 0;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject an out-of-range result reference before source generation. */
	saved = clone->kernel[0].ir->instruction[1].reference;
	clone->kernel[0].ir->instruction[1].reference = 1;
	success = accel_program_validate(clone, error, sizeof(error));
	if (!success) {
		success = accel_shader_source_generate(
			ACCEL_SHADER_SOURCE_GLSL_ES_310,
			clone,
			0,
			&source,
			error,
			sizeof(error));
		if (success)
			accel_shader_source_cleanup(&source);
	}
	clone->kernel[0].ir->instruction[1].reference = saved;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject a producer kernel which tries to consume its own result. */
	accel_ir_builder_init(&builder, clone->kernel[0].ir);
	saved_instruction_count = clone->kernel[0].ir->instruction_count;
	saved_value_count = clone->kernel[0].ir->value_count;
	none = ACCEL_IR_VALUE_NONE;
	success = add_instruction(
		&builder,
		ACCEL_IR_LOAD_RESULT_I32,
		ACCEL_IR_I32,
		0,
		0,
		none,
		none,
		none);
	if (!success) {
		accel_program_destroy(clone);
		return false;
	}
	success = accel_program_validate(clone, error, sizeof(error));
	clone->kernel[0].ir->instruction_count = saved_instruction_count;
	clone->kernel[0].ir->value_count = saved_value_count;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject consumer metadata which omits a real GPU load. */
	clone->scalar_result[0].gpu_consumer_mask = 0;
	success = accel_program_validate(clone, error, sizeof(error));
	clone->scalar_result[0].gpu_consumer_mask = (uint32_t)1U << 1;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject a producer assignment that contradicts both kernels. */
	clone->scalar_result[0].producer_kernel = 1;
	success = accel_program_validate(clone, error, sizeof(error));
	clone->scalar_result[0].producer_kernel = 0;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject a scalar-result entry whose static producer is missing. */
	saved_instruction_count = clone->kernel[0].ir->instruction_count;
	clone->kernel[0].ir->instruction_count--;
	success = accel_program_validate(clone, error, sizeof(error));
	clone->kernel[0].ir->instruction_count = saved_instruction_count;
	if (success) {
		accel_program_destroy(clone);
		return false;
	}

	/* Reject a second static atomic producer for the same result entry. */
	accel_ir_builder_init(&builder, clone->kernel[0].ir);
	success = add_instruction(
		&builder,
		ACCEL_IR_ATOMIC_ADD_I32,
		ACCEL_IR_VOID,
		0,
		0,
		0,
		none,
		none);
	if (!success) {
		accel_program_destroy(clone);
		return false;
	}
	success = accel_program_validate(clone, error, sizeof(error));
	accel_program_destroy(clone);
	if (success)
		return false;

	/* Report complete scalar-result validation coverage. */
	return true;
}

/* Run target-neutral scalar-result ownership and lowering tests. */
static bool
test_scalar_results(
	void)
{
	struct accel_program *program;
	bool success;

	/* Build one shared producer-consumer reduction program. */
	program = build_scalar_result_program();
	if (program == NULL)
		return false;

	/* Exercise both source generation and the deep-owned program model. */
	success = test_scalar_result_sources(program);
	if (success)
		success = test_scalar_result_validation(program);

	accel_program_destroy(program);

	return success;
}

/* Verify invalid dialects and malformed typed IR are rejected. */
static bool
test_invalid_input(
	struct accel_program *program)
{
	struct accel_shader_source source;
	struct accel_ir_instruction *instruction;
	char error[160];
	int saved_opcode;
	bool success;

	/* Reject a dialect that has no defined source contract. */
	success = accel_shader_source_generate(
		99,
		program,
		0,
		&source,
		error,
		sizeof(error));
	if (success) {
		accel_shader_source_cleanup(&source);
		fprintf(stderr, "invalid shader dialect was accepted\n");
		return false;
	}

	/* Corrupt one opcode and require typed validation before generation. */
	instruction = &program->kernel[0].ir->instruction[0];
	saved_opcode = instruction->opcode;
	instruction->opcode = 999;
	success = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_GLSL_ES_310,
		program,
		0,
		&source,
		error,
		sizeof(error));
	instruction->opcode = saved_opcode;
	if (success) {
		accel_shader_source_cleanup(&source);
		fprintf(stderr, "invalid typed IR was accepted\n");
		return false;
	}

	/* Report correct invalid-input rejection. */
	return true;
}
