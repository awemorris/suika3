/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Deterministic SPIR-V assembly generation and validation.
 */

#include "accel_vulkan_shader.h"
#include "hir.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_SPIRV_MAGIC		0x07230203U
#define ACCEL_SPIRV_VERSION_1_5		0x00010500U
#define ACCEL_SPIRV_MAX_BOUND		65536U

#define ACCEL_SPIRV_OP_ENTRY_POINT	15U
#define ACCEL_SPIRV_OP_EXECUTION_MODE	16U
#define ACCEL_SPIRV_OP_CAPABILITY	17U
#define ACCEL_SPIRV_OP_DECORATE		71U
#define ACCEL_SPIRV_OP_FADD		129U
#define ACCEL_SPIRV_OP_FSUB		131U
#define ACCEL_SPIRV_OP_FMUL		133U

#define ACCEL_SPIRV_EXEC_GL_COMPUTE	5U
#define ACCEL_SPIRV_MODE_LOCAL_SIZE	17U
#define ACCEL_SPIRV_MODE_DENORM_PRESERVE	4459U
#define ACCEL_SPIRV_MODE_SIGNED_ZERO	4461U
#define ACCEL_SPIRV_MODE_ROUND_RTE	4462U

#define ACCEL_SPIRV_CAP_DENORM_PRESERVE	4464U
#define ACCEL_SPIRV_CAP_SIGNED_ZERO	4466U
#define ACCEL_SPIRV_CAP_ROUND_RTE	4467U

#define ACCEL_SPIRV_DEC_RELAXED_PRECISION 0U
#define ACCEL_SPIRV_DEC_NO_CONTRACTION	42U

enum accel_shader_build_status {
	ACCEL_SHADER_BUILD_SUCCESS,
	ACCEL_SHADER_BUILD_OUT_OF_MEMORY,
	ACCEL_SHADER_BUILD_INVALID_IR
};

struct accel_text {
	char *data;
	size_t length;
	size_t capacity;
	bool out_of_memory;
};

struct accel_shader_builder {
	struct accel_text entry;
	struct accel_text annotation;
	struct accel_text declaration;
	struct accel_text body;
	const struct accel_program *program;
	const struct accel_kernel_plan *kernel;
	bool uses_f32;
};

static bool accel_text_reserve(struct accel_text *text, size_t additional);
static bool accel_text_append(struct accel_text *text, const char *format, ...);
static void accel_text_cleanup(struct accel_text *text);
static enum accel_shader_build_status accel_shader_build_assembly(struct accel_shader_builder *builder, struct accel_text *assembly);
static void accel_shader_work_cleanup(struct accel_shader_builder *builder, struct accel_text *assembly);
static bool accel_shader_uses_f32(const struct accel_ir_kernel *kernel);
static bool accel_shader_build_entry(struct accel_shader_builder *builder);
static bool accel_shader_build_annotations(struct accel_shader_builder *builder);
static bool accel_shader_build_declarations(struct accel_shader_builder *builder);
static bool accel_shader_append_f32_constant(struct accel_shader_builder *builder, uint32_t result, uint32_t bits);
static bool accel_shader_build_body(struct accel_shader_builder *builder);
static bool accel_shader_build_instruction(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction, uint32_t instruction_index);
static bool accel_shader_build_constant(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_build_uniform(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_build_load(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_build_store(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction, uint32_t instruction_index);
static bool accel_shader_build_atomic_add(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction, uint32_t instruction_index);
static bool accel_shader_build_result_load(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_build_arithmetic(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static bool accel_shader_build_comparison(struct accel_shader_builder *builder, const struct accel_ir_instruction *instruction);
static const char *accel_shader_type_name(int type);
static int accel_shader_value_type(const struct accel_ir_kernel *kernel, uint32_t value);
static const char *accel_shader_arithmetic_name(const struct accel_ir_instruction *instruction);
static const char *accel_shader_comparison_name(const struct accel_ir_instruction *instruction, int operand_type);
static bool accel_shader_join(struct accel_shader_builder *builder, struct accel_text *assembly);
static void accel_shader_builder_cleanup(struct accel_shader_builder *builder);
static bool accel_spirv_validate(const uint32_t *word, size_t word_count, bool strict_f32, char *error, size_t error_size);
static bool accel_spirv_error(char *error, size_t error_size, const char *message);
static bool accel_spirv_entry_is_main(const uint32_t *instruction, uint32_t instruction_word_count);
static void accel_shader_hir_error(int line, const char *prefix, const char *detail);

/*
 * Compiles one target-neutral kernel into owned Vulkan SPIR-V words.
 */
enum accel_compile_status
accel_vulkan_shader_compile(
	shaderc_compiler_t compiler,
	shaderc_compile_options_t options,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_vulkan_spirv *result)
{
	struct accel_shader_builder builder;
	struct accel_text assembly;
	shaderc_compilation_result_t compiled;
	shaderc_compilation_status status;
	const char *byte;
	const char *message;
	size_t byte_count;
	char validation_error[128];
	enum accel_shader_build_status build_status;
	bool valid;

	/* Reject a missing result destination. */
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;

	/* Clear the caller's result before starting compilation. */
	result->word = NULL;
	result->word_count = 0;

	/* Reject an incomplete compiler request. */
	if (compiler == NULL ||
	    options == NULL ||
	    program == NULL) {
		hir_error(0, N_TR("Invalid Vulkan shader compiler input."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Reject a kernel index outside the immutable program. */
	if (kernel_index >= program->kernel_count) {
		hir_error(0, N_TR("Invalid Vulkan kernel index."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Initialize the temporary assembly builder. */
	memset(&builder, 0, sizeof(builder));
	memset(&assembly, 0, sizeof(assembly));
	builder.program = program;
	builder.kernel = &program->kernel[kernel_index];
	builder.uses_f32 = accel_shader_uses_f32(builder.kernel->ir);

	/* Build the deterministic SPIR-V assembly text. */
	build_status = accel_shader_build_assembly(&builder, &assembly);

	/* Report an exhausted text allocation. */
	if (build_status == ACCEL_SHADER_BUILD_OUT_OF_MEMORY) {
		hir_out_of_memory();
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Report an instruction outside the validated Vulkan subset. */
	if (build_status == ACCEL_SHADER_BUILD_INVALID_IR) {
		hir_error(
			builder.kernel->source_line,
			N_TR("Unsupported instruction reached the Vulkan shader generator."));
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Assemble the completed text with shaderc. */
	compiled = shaderc_assemble_into_spv(
		compiler,
		assembly.data,
		assembly.length,
		options);

	/* Reject a missing shaderc result. */
	if (compiled == NULL) {
		hir_error(
			builder.kernel->source_line,
			N_TR("Vulkan shader assembler returned no result."));
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Inspect the shaderc compilation status. */
	status = shaderc_result_get_compilation_status(compiled);

	/* Report a shaderc assembly failure. */
	if (status != shaderc_compilation_status_success) {
		message = shaderc_result_get_error_message(compiled);

		/* Supply a stable message when shaderc provides none. */
		if (message == NULL)
			message = N_TR("unknown assembler failure");

		accel_shader_hir_error(
			builder.kernel->source_line,
			N_TR("Vulkan shader assembly failed: "),
			message);
		shaderc_result_release(compiled);
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Read the assembled SPIR-V byte range. */
	byte_count = shaderc_result_get_length(compiled);
	byte = shaderc_result_get_bytes(compiled);

	/* Reject a missing, short, or misaligned binary module. */
	if (byte == NULL ||
	    byte_count < 5 * sizeof(uint32_t) ||
	    byte_count % sizeof(uint32_t) != 0) {
		hir_error(
			builder.kernel->source_line,
			N_TR("Vulkan shader assembler returned malformed SPIR-V."));
		shaderc_result_release(compiled);
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Allocate the owned result module. */
	result->word = noct_malloc(byte_count);
	if (result->word == NULL) {
		shaderc_result_release(compiled);
		hir_out_of_memory();
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/*
	 * Copy and normalize the assembled module header.  Shaderc emits a
	 * SPIR-V 1.0 header even when its target option requests SPIR-V 1.5.
	 */
	memcpy(result->word, byte, byte_count);
	result->word_count = byte_count / sizeof(uint32_t);
	shaderc_result_release(compiled);
	result->word[1] = ACCEL_SPIRV_VERSION_1_5;

	/* Validate the generated module before publishing it. */
	valid = accel_spirv_validate(
		result->word,
		result->word_count,
		builder.uses_f32,
		validation_error,
		sizeof(validation_error));

	/* Reject a generated module outside the backend contract. */
	if (!valid) {
		accel_shader_hir_error(
			builder.kernel->source_line,
			N_TR("Generated Vulkan SPIR-V is invalid: "),
			validation_error);
		accel_vulkan_shader_cleanup(result);
		accel_shader_work_cleanup(&builder, &assembly);
		return ACCEL_COMPILE_ERROR;
	}

	/* Release temporary text after publishing the owned module. */
	accel_shader_work_cleanup(&builder, &assembly);

	/* Report successful shader compilation. */
	return ACCEL_COMPILE_APPLIED;
}

/*
 * Releases an owned SPIR-V result and clears its fields.
 */
void
accel_vulkan_shader_cleanup(
	struct accel_vulkan_spirv *spirv)
{
	/* Ignore an absent result container. */
	if (spirv == NULL)
		return;

	/* Release the module and clear the public result. */
	noct_free(spirv->word);
	spirv->word = NULL;
	spirv->word_count = 0;
}

/* Build every ordered assembly section and classify one failure. */
static enum accel_shader_build_status
accel_shader_build_assembly(
	struct accel_shader_builder *builder,
	struct accel_text *assembly)
{
	bool built;

	/* Build the module entry and execution modes. */
	built = accel_shader_build_entry(builder);
	if (!built)
		return ACCEL_SHADER_BUILD_OUT_OF_MEMORY;

	/* Build the module annotations. */
	built = accel_shader_build_annotations(builder);
	if (!built)
		return ACCEL_SHADER_BUILD_OUT_OF_MEMORY;

	/* Build the module declarations. */
	built = accel_shader_build_declarations(builder);
	if (!built)
		return ACCEL_SHADER_BUILD_OUT_OF_MEMORY;

	/* Build the validated instruction body. */
	built = accel_shader_build_body(builder);
	if (!built)
		return ACCEL_SHADER_BUILD_INVALID_IR;

	/* Join the sections into one shaderc input. */
	built = accel_shader_join(builder, assembly);
	if (!built)
		return ACCEL_SHADER_BUILD_OUT_OF_MEMORY;

	/* Report a complete assembly string. */
	return ACCEL_SHADER_BUILD_SUCCESS;
}

/* Release all temporary shader-generation text. */
static void
accel_shader_work_cleanup(
	struct accel_shader_builder *builder,
	struct accel_text *assembly)
{
	/* Release joined text before its source sections. */
	accel_text_cleanup(assembly);
	accel_shader_builder_cleanup(builder);
}

/* Grow one assembly section without losing its current allocation. */
static bool
accel_text_reserve(
	struct accel_text *text,
	size_t additional)
{
	char *data;
	size_t required;
	size_t capacity;

	if (text->out_of_memory)
		return false;
	if (additional > (size_t)-1 - text->length - 1) {
		text->out_of_memory = true;
		return false;
	}

	required = text->length + additional + 1;
	if (required <= text->capacity)
		return true;

	capacity = text->capacity;
	if (capacity == 0)
		capacity = 1024;

	/* Double the text allocation until the requested append fits. */
	while (capacity < required) {
		if (capacity > (size_t)-1 / 2) {
			capacity = required;
			break;
		}
		capacity *= 2;
	}

	data = noct_realloc(text->data, capacity);
	if (data == NULL) {
		text->out_of_memory = true;
		return false;
	}

	text->data = data;
	text->capacity = capacity;

	return true;
}

/* Append one formatted fragment to an assembly section. */
static bool
accel_text_append(
	struct accel_text *text,
	const char *format,
	...)
{
	va_list arguments;
	va_list measured_arguments;
	int length;

	va_start(arguments, format);
	va_copy(measured_arguments, arguments);
	length = vsnprintf(NULL, 0, format, measured_arguments);
	va_end(measured_arguments);
	if (length < 0) {
		va_end(arguments);
		return false;
	}

	if (!accel_text_reserve(text, (size_t)length)) {
		va_end(arguments);
		return false;
	}

	(void)vsnprintf(
		text->data + text->length,
		text->capacity - text->length,
		format,
		arguments);
	va_end(arguments);
	text->length += (size_t)length;

	return true;
}

/* Release one assembly section. */
static void
accel_text_cleanup(
	struct accel_text *text)
{
	if (text == NULL)
		return;

	noct_free(text->data);
	memset(text, 0, sizeof(*text));
}

/* Detect whether a kernel requires the strict Float32 contract. */
static bool
accel_shader_uses_f32(
	const struct accel_ir_kernel *kernel)
{
	uint32_t i;

	if (kernel == NULL)
		return false;

	/* Inspect each buffer element type. */
	for (i = 0; i < kernel->buffer_binding_count; i++) {
		if (kernel->buffer_value_type[i] == ACCEL_IR_F32)
			return true;
	}

	/* Inspect each SSA result type. */
	for (i = 0; i < kernel->instruction_count; i++) {
		if (kernel->instruction[i].result_type == ACCEL_IR_F32)
			return true;
	}

	return false;
}

/* Build capabilities, entry point, and execution modes. */
static bool
accel_shader_build_entry(
	struct accel_shader_builder *builder)
{
	uint32_t i;

	if (!accel_text_append(&builder->entry, "OpCapability Shader\n"))
		return false;
	if (builder->uses_f32) {
		if (!accel_text_append(&builder->entry, "OpCapability DenormPreserve\n"))
			return false;
		if (!accel_text_append(&builder->entry, "OpCapability SignedZeroInfNanPreserve\n"))
			return false;
		if (!accel_text_append(&builder->entry, "OpCapability RoundingModeRTE\n"))
			return false;
	}

	if (!accel_text_append(&builder->entry, "OpMemoryModel Logical GLSL450\n"))
		return false;
	if (!accel_text_append(&builder->entry, "OpEntryPoint GLCompute %%main \"main\" %%global_id"))
		return false;

	/* Publish every deterministic data-buffer interface variable. */
	for (i = 0; i < builder->program->buffer_count; i++) {
		if (!accel_text_append(&builder->entry, " %%buffer_%u", i))
			return false;
	}

	/* Append the required scalar-block interface variable. */
	if (!accel_text_append(&builder->entry, " %%scalar"))
		return false;

	/* Append the optional scalar-result interface variable. */
	if (builder->program->scalar_result_count != 0) {
		/* Append the result only when the program publishes one. */
		if (!accel_text_append(&builder->entry, " %%result"))
			return false;
	}

	/* Complete the entry-point declaration. */
	if (!accel_text_append(&builder->entry, "\n"))
		return false;
	if (!accel_text_append(&builder->entry, "OpExecutionMode %%main LocalSize 64 1 1\n"))
		return false;
	if (builder->uses_f32) {
		if (!accel_text_append(&builder->entry, "OpExecutionMode %%main SignedZeroInfNanPreserve 32\n"))
			return false;
		if (!accel_text_append(&builder->entry, "OpExecutionMode %%main DenormPreserve 32\n"))
			return false;
		if (!accel_text_append(&builder->entry, "OpExecutionMode %%main RoundingModeRTE 32\n"))
			return false;
	}

	return true;
}

/* Build descriptor, layout, builtin, and arithmetic decorations. */
static bool
accel_shader_build_annotations(
	struct accel_shader_builder *builder)
{
	const struct accel_ir_instruction *instruction;
	uint32_t i;

	if (!accel_text_append(&builder->annotation, "OpDecorate %%global_id BuiltIn GlobalInvocationId\n"))
		return false;
	if (!accel_text_append(&builder->annotation, "OpDecorate %%word_array ArrayStride 4\n"))
		return false;
	if (!accel_text_append(&builder->annotation, "OpMemberDecorate %%buffer_block 0 Offset 0\n"))
		return false;
	if (!accel_text_append(&builder->annotation, "OpDecorate %%buffer_block Block\n"))
		return false;

	/* Bind each program buffer in deterministic device order. */
	for (i = 0; i < builder->program->buffer_count; i++) {
		if (!accel_text_append(&builder->annotation, "OpDecorate %%buffer_%u DescriptorSet 0\n", i))
			return false;
		if (!accel_text_append(&builder->annotation, "OpDecorate %%buffer_%u Binding %u\n", i, i))
			return false;
	}

	if (!accel_text_append(&builder->annotation, "OpDecorate %%scalar DescriptorSet 0\n"))
		return false;
	if (!accel_text_append(&builder->annotation, "OpDecorate %%scalar Binding %u\n", builder->program->buffer_count))
		return false;

	/* Decorate the optional scalar-result storage buffer. */
	if (builder->program->scalar_result_count != 0) {
		/* Assign the scalar-result descriptor set. */
		if (!accel_text_append(&builder->annotation, "OpDecorate %%result DescriptorSet 0\n"))
			return false;

		/* Assign the scalar-result binding after the scalar block. */
		if (!accel_text_append(&builder->annotation, "OpDecorate %%result Binding %u\n", builder->program->buffer_count + 1))
			return false;
	}

	/* Prohibit contraction for every generated floating arithmetic result. */
	for (i = 0; i < builder->kernel->ir->instruction_count; i++) {
		instruction = &builder->kernel->ir->instruction[i];
		if (instruction->result_type != ACCEL_IR_F32)
			continue;
		if (instruction->opcode != ACCEL_IR_ADD &&
		    instruction->opcode != ACCEL_IR_SUB &&
		    instruction->opcode != ACCEL_IR_MUL) {
			continue;
		}
		if (!accel_text_append(&builder->annotation, "OpDecorate %%v%u NoContraction\n", instruction->result))
			return false;
	}

	return true;
}

/* Build reusable scalar, pointer, buffer, and literal declarations. */
static bool
accel_shader_build_declarations(
	struct accel_shader_builder *builder)
{
	const struct accel_ir_instruction *instruction;
	uint32_t scalar_word_count;
	uint32_t constant_word_count;
	uint32_t i;

	if (!accel_text_append(&builder->declaration, "%%void = OpTypeVoid\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%function = OpTypeFunction %%void\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%bool = OpTypeBool\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%u32 = OpTypeInt 32 0\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%i32 = OpTypeInt 32 1\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%f32 = OpTypeFloat 32\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%v3u32 = OpTypeVector %%u32 3\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%ptr_input_v3u32 = OpTypePointer Input %%v3u32\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%word_array = OpTypeRuntimeArray %%u32\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%buffer_block = OpTypeStruct %%word_array\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%ptr_storage_block = OpTypePointer StorageBuffer %%buffer_block\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%ptr_storage_u32 = OpTypePointer StorageBuffer %%u32\n"))
		return false;
	if (!accel_text_append(&builder->declaration, "%%global_id = OpVariable %%ptr_input_v3u32 Input\n"))
		return false;

	/* Declare every storage-buffer interface variable. */
	for (i = 0; i < builder->program->buffer_count; i++) {
		if (!accel_text_append(&builder->declaration, "%%buffer_%u = OpVariable %%ptr_storage_block StorageBuffer\n", i))
			return false;
	}
	if (!accel_text_append(&builder->declaration, "%%scalar = OpVariable %%ptr_storage_block StorageBuffer\n"))
		return false;

	/* Declare the optional scalar-result storage buffer. */
	if (builder->program->scalar_result_count != 0) {
		/* Emit the interface variable only when it is referenced. */
		if (!accel_text_append(&builder->declaration, "%%result = OpVariable %%ptr_storage_block StorageBuffer\n"))
			return false;
	}

	/* Compute the complete range of reusable unsigned constants. */
	scalar_word_count = builder->program->scalar_count +
		builder->program->kernel_count * 2;
	constant_word_count = scalar_word_count;

	/* Include every scalar-result index in the constant range. */
	if (constant_word_count < builder->program->scalar_result_count)
		constant_word_count = builder->program->scalar_result_count;

	/* Declare the structure index and every referenced word index. */
	for (i = 0; i < constant_word_count; i++) {
		if (!accel_text_append(&builder->declaration, "%%u32_%u = OpConstant %%u32 %u\n", i, i))
			return false;
	}

	/* Guarantee the structure-member index for an otherwise empty range. */
	if (constant_word_count == 0) {
		/* Declare the mandatory zero index. */
		if (!accel_text_append(&builder->declaration, "%%u32_0 = OpConstant %%u32 0\n"))
			return false;
	}

	/* Declare constants required by scalar-result atomics. */
	if (builder->program->scalar_result_count != 0) {
		/* Declare the device scope used by every reduction. */
		if (!accel_text_append(&builder->declaration, "%%scope_device = OpConstant %%u32 1\n"))
			return false;

		/* Declare the unordered atomic-memory semantics. */
		if (!accel_text_append(&builder->declaration, "%%semantics_none = OpConstant %%u32 0\n"))
			return false;
	}

	/* Declare each source constant outside the function body. */
	for (i = 0; i < builder->kernel->ir->instruction_count; i++) {
		instruction = &builder->kernel->ir->instruction[i];
		if (instruction->opcode == ACCEL_IR_CONST_BOOL) {
			if (instruction->literal_bits != 0) {
				if (!accel_text_append(&builder->declaration, "%%v%u = OpConstantTrue %%bool\n", instruction->result))
					return false;
			} else {
				if (!accel_text_append(&builder->declaration, "%%v%u = OpConstantFalse %%bool\n", instruction->result))
					return false;
			}
		}
		if (instruction->opcode == ACCEL_IR_CONST_I32) {
			if (!accel_text_append(&builder->declaration, "%%v%u = OpConstant %%i32 %d\n", instruction->result, (int32_t)instruction->literal_bits))
				return false;
		}
		if (instruction->opcode == ACCEL_IR_CONST_F32) {
			if (!accel_shader_append_f32_constant(
				builder,
				instruction->result,
				instruction->literal_bits)) {
				return false;
			}
		}
	}

	return true;
}

/* Append one exact finite IEEE-754 binary32 constant without locale use. */
static bool
accel_shader_append_f32_constant(
	struct accel_shader_builder *builder,
	uint32_t result,
	uint32_t bits)
{
	const char *sign;
	uint32_t exponent;
	uint32_t fraction;
	int power;

	sign = (bits & 0x80000000U) != 0 ? "-" : "";
	exponent = (bits >> 23) & 0xffU;
	fraction = bits & 0x007fffffU;
	if (exponent == 0xffU)
		return false;
	if (exponent == 0) {
		if (fraction == 0) {
			return accel_text_append(
				&builder->declaration,
				"%%v%u = OpConstant %%f32 %s0x0p+0\n",
				result,
				sign);
		}

		return accel_text_append(
			&builder->declaration,
			"%%v%u = OpConstant %%f32 %s0x0.%06xp-126\n",
			result,
			sign,
			fraction << 1);
	}

	power = (int)exponent - 127;

	return accel_text_append(
		&builder->declaration,
		"%%v%u = OpConstant %%f32 %s0x1.%06xp%+d\n",
		result,
		sign,
		fraction << 1,
		power);
}

/* Build the guarded one-dimensional compute entry point. */
static bool
accel_shader_build_body(
	struct accel_shader_builder *builder)
{
	uint32_t trip_word;
	uint32_t i;

	trip_word = builder->program->scalar_count +
		builder->kernel->kernel_index * 2 + 1;

	if (!accel_text_append(&builder->body, "%%main = OpFunction %%void None %%function\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%entry = OpLabel\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%global_vector = OpLoad %%v3u32 %%global_id\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%lane = OpCompositeExtract %%u32 %%global_vector 0\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%trip_pointer = OpAccessChain %%ptr_storage_u32 %%scalar %%u32_0 %%u32_%u\n", trip_word))
		return false;
	if (!accel_text_append(&builder->body, "%%trip = OpLoad %%u32 %%trip_pointer\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%outside = OpUGreaterThanEqual %%bool %%lane %%trip\n"))
		return false;
	if (!accel_text_append(&builder->body, "OpSelectionMerge %%exit None\n"))
		return false;
	if (!accel_text_append(&builder->body, "OpBranchConditional %%outside %%exit %%work\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%work = OpLabel\n"))
		return false;

	/* Emit target instructions in validated SSA definition order. */
	for (i = 0; i < builder->kernel->ir->instruction_count; i++) {
		if (!accel_shader_build_instruction(
			builder,
			&builder->kernel->ir->instruction[i],
			i)) {
			return false;
		}
	}

	if (!accel_text_append(&builder->body, "OpBranch %%exit\n"))
		return false;
	if (!accel_text_append(&builder->body, "%%exit = OpLabel\n"))
		return false;
	if (!accel_text_append(&builder->body, "OpReturn\n"))
		return false;
	if (!accel_text_append(&builder->body, "OpFunctionEnd\n"))
		return false;

	return true;
}

/* Lower one typed target-neutral instruction to SPIR-V assembly. */
static bool
accel_shader_build_instruction(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t instruction_index)
{
	bool built;

	/* Select the exact lowering for this typed operation. */
	switch (instruction->opcode) {
	case ACCEL_IR_PARAMETER:
	case ACCEL_IR_UNIFORM:
		built = accel_shader_build_uniform(builder, instruction);
		break;
	case ACCEL_IR_CONST_BOOL:
	case ACCEL_IR_CONST_I32:
	case ACCEL_IR_CONST_F32:
		built = accel_shader_build_constant(builder, instruction);
		break;
	case ACCEL_IR_GLOBAL_INDEX:
		built = accel_text_append(
			&builder->body,
			"%%v%u = OpCopyObject %%u32 %%lane\n",
			instruction->result);
		break;
	case ACCEL_IR_BUFFER_LOAD:
		built = accel_shader_build_load(builder, instruction);
		break;
	case ACCEL_IR_BUFFER_STORE:
		built = accel_shader_build_store(
			builder,
			instruction,
			instruction_index);
		break;
	case ACCEL_IR_ATOMIC_ADD_I32:
		built = accel_shader_build_atomic_add(
			builder,
			instruction,
			instruction_index);
		break;
	case ACCEL_IR_LOAD_RESULT_I32:
		built = accel_shader_build_result_load(builder, instruction);
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
		built = accel_shader_build_arithmetic(builder, instruction);
		break;
	case ACCEL_IR_COMPARE_EQ:
	case ACCEL_IR_COMPARE_NE:
	case ACCEL_IR_COMPARE_LT:
	case ACCEL_IR_COMPARE_LTE:
	case ACCEL_IR_COMPARE_GT:
	case ACCEL_IR_COMPARE_GTE:
		built = accel_shader_build_comparison(builder, instruction);
		break;
	case ACCEL_IR_SELECT:
		built = accel_text_append(
			&builder->body,
			"%%v%u = OpSelect %s %%v%u %%v%u %%v%u\n",
			instruction->result,
			accel_shader_type_name(instruction->result_type),
			instruction->operand[0],
			instruction->operand[1],
			instruction->operand[2]);
		break;
	default:
		built = false;
		break;
	}

	/* Report whether the selected instruction was lowered. */
	return built;
}

/* Lower one exact scalar constant. */
static bool
accel_shader_build_constant(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	UNUSED_PARAMETER(builder);

	if (instruction->opcode == ACCEL_IR_CONST_BOOL)
		return true;
	if (instruction->opcode == ACCEL_IR_CONST_I32)
		return true;
	if (instruction->opcode == ACCEL_IR_CONST_F32)
		return true;

	return false;
}

/* Lower one immutable scalar-block load. */
static bool
accel_shader_build_uniform(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	if (!accel_text_append(
		&builder->body,
		"%%p%u = OpAccessChain %%ptr_storage_u32 %%scalar %%u32_0 %%u32_%u\n",
		instruction->result,
		instruction->reference)) {
		return false;
	}
	if (!accel_text_append(
		&builder->body,
		"%%r%u = OpLoad %%u32 %%p%u\n",
		instruction->result,
		instruction->result)) {
		return false;
	}

	return accel_text_append(
		&builder->body,
		"%%v%u = OpBitcast %s %%r%u\n",
		instruction->result,
		accel_shader_type_name(instruction->result_type),
		instruction->result);
}

/* Lower one typed storage-buffer load. */
static bool
accel_shader_build_load(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	if (!accel_text_append(
		&builder->body,
		"%%p%u = OpAccessChain %%ptr_storage_u32 %%buffer_%u %%u32_0 %%v%u\n",
		instruction->result,
		instruction->reference,
		instruction->operand[0])) {
		return false;
	}
	if (!accel_text_append(
		&builder->body,
		"%%r%u = OpLoad %%u32 %%p%u\n",
		instruction->result,
		instruction->result)) {
		return false;
	}

	return accel_text_append(
		&builder->body,
		"%%v%u = OpBitcast %s %%r%u\n",
		instruction->result,
		accel_shader_type_name(instruction->result_type),
		instruction->result);
}

/* Lower one typed storage-buffer store. */
static bool
accel_shader_build_store(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t instruction_index)
{
	if (!accel_text_append(
		&builder->body,
		"%%store_value_%u = OpBitcast %%u32 %%v%u\n",
		instruction_index,
		instruction->operand[1])) {
		return false;
	}
	if (!accel_text_append(
		&builder->body,
		"%%store_pointer_%u = OpAccessChain %%ptr_storage_u32 %%buffer_%u %%u32_0 %%v%u\n",
		instruction_index,
		instruction->reference,
		instruction->operand[0])) {
		return false;
	}

	return accel_text_append(
		&builder->body,
		"OpStore %%store_pointer_%u %%store_value_%u\n",
		instruction_index,
		instruction_index);
}

/* Lower one wrapping I32 contribution into a scalar-result word. */
static bool
accel_shader_build_atomic_add(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction,
	uint32_t instruction_index)
{
	bool appended;

	/* Convert the signed contribution to its wrapping word form. */
	appended = accel_text_append(
		&builder->body,
		"%%result_add_value_%u = OpBitcast %%u32 %%v%u\n",
		instruction_index,
		instruction->operand[0]);
	if (!appended)
		return false;

	/* Address the selected scalar-result word. */
	appended = accel_text_append(
		&builder->body,
		"%%result_add_pointer_%u = OpAccessChain %%ptr_storage_u32 %%result %%u32_0 %%u32_%u\n",
		instruction_index,
		instruction->reference);
	if (!appended)
		return false;

	/* Add the contribution with device-wide unordered atomicity. */
	appended = accel_text_append(
		&builder->body,
		"%%result_add_old_%u = OpAtomicIAdd %%u32 %%result_add_pointer_%u %%scope_device %%semantics_none %%result_add_value_%u\n",
		instruction_index,
		instruction_index,
		instruction_index);
	if (!appended)
		return false;

	/* Report a complete atomic reduction lowering. */
	return true;
}

/* Lower one scalar-result load into a signed I32 SSA value. */
static bool
accel_shader_build_result_load(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	bool appended;

	/* Address the selected scalar-result word. */
	appended = accel_text_append(
		&builder->body,
		"%%result_pointer_%u = OpAccessChain %%ptr_storage_u32 %%result %%u32_0 %%u32_%u\n",
		instruction->result,
		instruction->reference);
	if (!appended)
		return false;

	/* Load the wrapping result word. */
	appended = accel_text_append(
		&builder->body,
		"%%result_word_%u = OpLoad %%u32 %%result_pointer_%u\n",
		instruction->result,
		instruction->result);
	if (!appended)
		return false;

	/* Restore the signed I32 SSA representation. */
	appended = accel_text_append(
		&builder->body,
		"%%v%u = OpBitcast %%i32 %%result_word_%u\n",
		instruction->result,
		instruction->result);
	if (!appended)
		return false;

	/* Report a complete scalar-result load. */
	return true;
}

/* Lower one arithmetic, bitwise, shift, or index operation. */
static bool
accel_shader_build_arithmetic(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	const char *operation;
	const char *type;

	operation = accel_shader_arithmetic_name(instruction);
	if (operation == NULL)
		return false;
	type = accel_shader_type_name(instruction->result_type);
	if (type == NULL)
		return false;

	if (instruction->result_type == ACCEL_IR_INDEX_U32) {
		if (!accel_text_append(
			&builder->body,
			"%%index_adjust_%u = OpBitcast %%u32 %%v%u\n",
			instruction->result,
			instruction->operand[1])) {
			return false;
		}

		return accel_text_append(
			&builder->body,
			"%%v%u = %s %%u32 %%v%u %%index_adjust_%u\n",
			instruction->result,
			operation,
			instruction->operand[0],
			instruction->result);
	}

	return accel_text_append(
		&builder->body,
		"%%v%u = %s %s %%v%u %%v%u\n",
		instruction->result,
		operation,
		type,
		instruction->operand[0],
		instruction->operand[1]);
}

/* Lower one signed, ordered, or Boolean comparison. */
static bool
accel_shader_build_comparison(
	struct accel_shader_builder *builder,
	const struct accel_ir_instruction *instruction)
{
	const char *operation;
	int operand_type;

	operand_type = accel_shader_value_type(
		builder->kernel->ir,
		instruction->operand[0]);
	operation = accel_shader_comparison_name(instruction, operand_type);
	if (operation == NULL)
		return false;

	return accel_text_append(
		&builder->body,
		"%%v%u = %s %%bool %%v%u %%v%u\n",
		instruction->result,
		operation,
		instruction->operand[0],
		instruction->operand[1]);
}

/* Find one prior SSA value's exact private IR type. */
static int
accel_shader_value_type(
	const struct accel_ir_kernel *kernel,
	uint32_t value)
{
	uint32_t i;

	/* Locate the defining instruction in deterministic result order. */
	for (i = 0; i < kernel->instruction_count; i++) {
		if (kernel->instruction[i].result == value)
			return kernel->instruction[i].result_type;
	}

	return ACCEL_IR_VOID;
}

/* Return the textual SPIR-V type name for one private IR type. */
static const char *
accel_shader_type_name(
	int type)
{
	/* Map the private value type to one exact 32-bit shader type. */
	switch (type) {
	case ACCEL_IR_BOOL:
		return "%bool";
	case ACCEL_IR_I32:
		return "%i32";
	case ACCEL_IR_F32:
		return "%f32";
	case ACCEL_IR_INDEX_U32:
		return "%u32";
	default:
		return NULL;
	}
}

/* Return the exact SPIR-V arithmetic opcode for one typed operation. */
static const char *
accel_shader_arithmetic_name(
	const struct accel_ir_instruction *instruction)
{
	if (instruction->opcode == ACCEL_IR_ADD) {
		if (instruction->result_type == ACCEL_IR_F32)
			return "OpFAdd";
		return "OpIAdd";
	}
	if (instruction->opcode == ACCEL_IR_SUB) {
		if (instruction->result_type == ACCEL_IR_F32)
			return "OpFSub";
		return "OpISub";
	}
	if (instruction->opcode == ACCEL_IR_MUL) {
		if (instruction->result_type == ACCEL_IR_F32)
			return "OpFMul";
		return "OpIMul";
	}
	if (instruction->opcode == ACCEL_IR_DIV_I32)
		return "OpSDiv";
	if (instruction->opcode == ACCEL_IR_MOD_I32)
		return "OpSRem";
	if (instruction->opcode == ACCEL_IR_BIT_AND)
		return "OpBitwiseAnd";
	if (instruction->opcode == ACCEL_IR_BIT_OR)
		return "OpBitwiseOr";
	if (instruction->opcode == ACCEL_IR_BIT_XOR)
		return "OpBitwiseXor";
	if (instruction->opcode == ACCEL_IR_SHIFT_LEFT)
		return "OpShiftLeftLogical";
	if (instruction->opcode == ACCEL_IR_SHIFT_RIGHT_LOGICAL)
		return "OpShiftRightLogical";

	return NULL;
}

/* Return the exact SPIR-V comparison opcode for one typed operation. */
static const char *
accel_shader_comparison_name(
	const struct accel_ir_instruction *instruction,
	int operand_type)
{
	if (operand_type == ACCEL_IR_BOOL) {
		if (instruction->opcode == ACCEL_IR_COMPARE_EQ)
			return "OpLogicalEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_NE)
			return "OpLogicalNotEqual";

		return NULL;
	}
	if (operand_type == ACCEL_IR_I32) {
		if (instruction->opcode == ACCEL_IR_COMPARE_EQ)
			return "OpIEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_NE)
			return "OpINotEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_LT)
			return "OpSLessThan";
		if (instruction->opcode == ACCEL_IR_COMPARE_LTE)
			return "OpSLessThanEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_GT)
			return "OpSGreaterThan";
		if (instruction->opcode == ACCEL_IR_COMPARE_GTE)
			return "OpSGreaterThanEqual";

		return NULL;
	}
	if (operand_type == ACCEL_IR_F32) {
		if (instruction->opcode == ACCEL_IR_COMPARE_EQ)
			return "OpFOrdEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_NE)
			return "OpFUnordNotEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_LT)
			return "OpFOrdLessThan";
		if (instruction->opcode == ACCEL_IR_COMPARE_LTE)
			return "OpFOrdLessThanEqual";
		if (instruction->opcode == ACCEL_IR_COMPARE_GT)
			return "OpFOrdGreaterThan";
		if (instruction->opcode == ACCEL_IR_COMPARE_GTE)
			return "OpFOrdGreaterThanEqual";
	}

	return NULL;
}

/* Join ordered assembly sections into one NUL-terminated source string. */
static bool
accel_shader_join(
	struct accel_shader_builder *builder,
	struct accel_text *assembly)
{
	if (!accel_text_append(assembly, "%s", builder->entry.data))
		return false;
	if (!accel_text_append(assembly, "%s", builder->annotation.data))
		return false;
	if (!accel_text_append(assembly, "%s", builder->declaration.data))
		return false;
	if (!accel_text_append(assembly, "%s", builder->body.data))
		return false;

	return true;
}

/* Release every temporary assembly section. */
static void
accel_shader_builder_cleanup(
	struct accel_shader_builder *builder)
{
	accel_text_cleanup(&builder->body);
	accel_text_cleanup(&builder->declaration);
	accel_text_cleanup(&builder->annotation);
	accel_text_cleanup(&builder->entry);
}

/* Validate the generated SPIR-V header and semantic contract. */
static bool
accel_spirv_validate(
	const uint32_t *word,
	size_t word_count,
	bool strict_f32,
	char *error,
	size_t error_size)
{
	bool *no_contraction;
	bool *float_result;
	uint32_t bound;
	uint32_t instruction_word_count;
	uint32_t opcode;
	uint32_t target;
	uint32_t value;
	size_t offset;
	uint32_t i;
	bool has_main;
	bool has_local_size;
	bool has_denorm_mode;
	bool has_signed_zero_mode;
	bool has_round_mode;
	bool has_denorm_capability;
	bool has_signed_zero_capability;
	bool has_round_capability;

	if (word == NULL || word_count < 5)
		return accel_spirv_error(error, error_size, N_TR("short module"));
	if (word[0] != ACCEL_SPIRV_MAGIC)
		return accel_spirv_error(error, error_size, N_TR("wrong magic"));
	if (word[1] != ACCEL_SPIRV_VERSION_1_5)
		return accel_spirv_error(error, error_size, N_TR("wrong SPIR-V version"));

	bound = word[3];
	if (bound == 0 || bound > ACCEL_SPIRV_MAX_BOUND)
		return accel_spirv_error(error, error_size, N_TR("invalid ID bound"));

	no_contraction = noct_calloc(bound, sizeof(*no_contraction));
	if (no_contraction == NULL)
		return accel_spirv_error(error, error_size, N_TR("validator allocation failed"));

	float_result = noct_calloc(bound, sizeof(*float_result));
	if (float_result == NULL) {
		noct_free(no_contraction);
		return accel_spirv_error(error, error_size, N_TR("validator allocation failed"));
	}

	has_main = false;
	has_local_size = false;
	has_denorm_mode = false;
	has_signed_zero_mode = false;
	has_round_mode = false;
	has_denorm_capability = false;
	has_signed_zero_capability = false;
	has_round_capability = false;
	offset = 5;

	/* Inspect each binary instruction without trusting its encoded length. */
	while (offset < word_count) {
		instruction_word_count = word[offset] >> 16;
		opcode = word[offset] & 0xffffU;
		if (instruction_word_count == 0 ||
		    instruction_word_count > word_count - offset) {
			noct_free(float_result);
			noct_free(no_contraction);
			return accel_spirv_error(error, error_size, N_TR("invalid instruction length"));
		}

		if (opcode == ACCEL_SPIRV_OP_ENTRY_POINT &&
		    instruction_word_count >= 5 &&
		    word[offset + 1] == ACCEL_SPIRV_EXEC_GL_COMPUTE &&
		    accel_spirv_entry_is_main(
			&word[offset],
			instruction_word_count)) {
			has_main = true;
		}
		if (opcode == ACCEL_SPIRV_OP_EXECUTION_MODE &&
		    instruction_word_count >= 3) {
			value = word[offset + 2];
			if (value == ACCEL_SPIRV_MODE_LOCAL_SIZE &&
			    instruction_word_count >= 6 &&
			    word[offset + 3] == 64 &&
			    word[offset + 4] == 1 &&
			    word[offset + 5] == 1) {
				has_local_size = true;
			}
			if (value == ACCEL_SPIRV_MODE_DENORM_PRESERVE &&
			    instruction_word_count >= 4 &&
			    word[offset + 3] == 32) {
				has_denorm_mode = true;
			}
			if (value == ACCEL_SPIRV_MODE_SIGNED_ZERO &&
			    instruction_word_count >= 4 &&
			    word[offset + 3] == 32) {
				has_signed_zero_mode = true;
			}
			if (value == ACCEL_SPIRV_MODE_ROUND_RTE &&
			    instruction_word_count >= 4 &&
			    word[offset + 3] == 32) {
				has_round_mode = true;
			}
		}
		if (opcode == ACCEL_SPIRV_OP_CAPABILITY &&
		    instruction_word_count >= 2) {
			value = word[offset + 1];
			if (value == ACCEL_SPIRV_CAP_DENORM_PRESERVE)
				has_denorm_capability = true;
			if (value == ACCEL_SPIRV_CAP_SIGNED_ZERO)
				has_signed_zero_capability = true;
			if (value == ACCEL_SPIRV_CAP_ROUND_RTE)
				has_round_capability = true;
		}
		if (opcode == ACCEL_SPIRV_OP_DECORATE &&
		    instruction_word_count >= 3) {
			target = word[offset + 1];
			value = word[offset + 2];
			if (value == ACCEL_SPIRV_DEC_RELAXED_PRECISION) {
				noct_free(float_result);
				noct_free(no_contraction);
				return accel_spirv_error(error, error_size, N_TR("RelaxedPrecision present"));
			}
			if (value == ACCEL_SPIRV_DEC_NO_CONTRACTION && target < bound)
				no_contraction[target] = true;
		}
		if ((opcode == ACCEL_SPIRV_OP_FADD ||
		     opcode == ACCEL_SPIRV_OP_FSUB ||
		     opcode == ACCEL_SPIRV_OP_FMUL) &&
		    instruction_word_count >= 3) {
			target = word[offset + 2];
			if (target >= bound) {
				noct_free(float_result);
				noct_free(no_contraction);
				return accel_spirv_error(error, error_size, N_TR("float result exceeds bound"));
			}
			float_result[target] = true;
		}

		offset += instruction_word_count;
	}

	if (!has_main || !has_local_size) {
		noct_free(float_result);
		noct_free(no_contraction);
		return accel_spirv_error(error, error_size, N_TR("missing compute entry contract"));
	}
	if (strict_f32 &&
	    (!has_denorm_mode ||
	     !has_signed_zero_mode ||
	     !has_round_mode ||
	     !has_denorm_capability ||
	     !has_signed_zero_capability ||
	     !has_round_capability)) {
		noct_free(float_result);
		noct_free(no_contraction);
		return accel_spirv_error(error, error_size, N_TR("missing strict Float32 contract"));
	}

	/* Require NoContraction on every floating arithmetic result. */
	for (i = 0; i < bound; i++) {
		if (float_result[i] && !no_contraction[i]) {
			noct_free(float_result);
			noct_free(no_contraction);
			return accel_spirv_error(error, error_size, N_TR("float contraction is not prohibited"));
		}
	}

	noct_free(float_result);
	noct_free(no_contraction);
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	return true;
}

/* Copy one deterministic binary validation error to caller storage. */
static bool
accel_spirv_error(
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

/* Verify the exact GLCompute entry-point name encoded in one instruction. */
static bool
accel_spirv_entry_is_main(
	const uint32_t *instruction,
	uint32_t instruction_word_count)
{
	const unsigned char *name;
	size_t byte_count;

	if (instruction_word_count < 5)
		return false;

	name = (const unsigned char *)&instruction[3];
	byte_count = (instruction_word_count - 3) * sizeof(uint32_t);
	if (byte_count < 5)
		return false;
	if (name[0] != 'm' ||
	    name[1] != 'a' ||
	    name[2] != 'i' ||
	    name[3] != 'n' ||
	    name[4] != '\0') {
		return false;
	}

	return true;
}

/* Report one generated-shader failure without trusting backend text as format. */
static void
accel_shader_hir_error(
	int line,
	const char *prefix,
	const char *detail)
{
	char message[1024];

	(void)snprintf(message, sizeof(message), "%s%s", prefix, detail);
	message[sizeof(message) - 1] = '\0';
	hir_error(line, message);
}
