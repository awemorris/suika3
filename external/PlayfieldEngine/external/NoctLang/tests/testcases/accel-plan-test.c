/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Focused target-neutral accelerator plan tests.
 */

#include "accel_private.h"
#include "accel_program.h"
#include "ast.h"
#include "hir.h"
#include "hir_opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *directory, const char *name);
static struct hir_block *find_accel_function(void);
static bool run_applied_case(const char *directory);
static bool run_multi_region_case(const char *directory);
static bool run_dosum_case(const char *directory, const char *name, uint32_t kernel_count, uint32_t consumer_mask, uint32_t result_args_slot);
static bool run_transparent_dosum_case(const char *directory);
static bool run_multiple_dosum_case(const char *directory);
static bool run_dosum_prefix_split_case(const char *directory);
static bool run_local_host_case(const char *directory);
static bool run_device_only_case(const char *directory, const char *name);
static bool run_nondevice_case(const char *directory, const char *name);
static bool run_declined_case(const char *directory, const char *name);
static bool run_invalid_ir_case(void);
static bool build_case(const char *directory, const char *name, struct hir_block **func_block);
static void cleanup_case(void);

/*
 * Runs the focused accelerator plan tests.
 */
int
main(
	int argc,
	char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s CASE-DIRECTORY\n", argv[0]);
		return 2;
	}

	if (!run_applied_case(argv[1]))
		return 1;
	if (!run_multi_region_case(argv[1]))
		return 1;
	if (!run_dosum_case(
		argv[1],
		"dosum.noct",
		2,
		(uint32_t)1U << 1,
		4)) {
		return 1;
	}
	if (!run_dosum_case(argv[1], "dosum-uint.noct", 1, 0, 3))
		return 1;
	if (!run_transparent_dosum_case(argv[1]))
		return 1;
	if (!run_multiple_dosum_case(argv[1]))
		return 1;
	if (!run_dosum_prefix_split_case(argv[1]))
		return 1;
	if (!run_local_host_case(argv[1]))
		return 1;
	if (!run_device_only_case(argv[1], "device-only.noct"))
		return 1;
	if (!run_device_only_case(argv[1], "device-only-literal.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-cpu-use.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-unknown-call.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-reassigned.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-partial.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-dynamic-producer.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-folded-extent.noct"))
		return 1;
	if (!run_nondevice_case(argv[1], "device-return.noct"))
		return 1;
	if (!run_declined_case(argv[1], "declined.noct"))
		return 1;
	if (!run_declined_case(argv[1], "local-alias.noct"))
		return 1;
	if (!run_declined_case(argv[1], "f32-neg.noct"))
		return 1;
	if (!run_declined_case(argv[1], "dosum-float-declined.noct"))
		return 1;
	if (!run_declined_case(argv[1], "dosum-identity-declined.noct"))
		return 1;
	if (!run_declined_case(
		argv[1],
		"dosum-decl-not-last-declined.noct")) {
		return 1;
	}
	if (!run_declined_case(argv[1], "dosum-later-declined.noct"))
		return 1;
	if (!run_invalid_ir_case())
		return 1;

	puts("PASS");

	return 0;
}

/* Read one owned NUL-terminated fixture from the requested directory. */
static char *
read_source(
	const char *directory,
	const char *name)
{
	char path[1024];
	char *source;
	FILE *file;
	long length;
	size_t read_size;
	int path_length;

	path_length = snprintf(path, sizeof(path), "%s/%s", directory, name);
	if (path_length < 0 || (size_t)path_length >= sizeof(path))
		return NULL;

	file = fopen(path, "rb");
	if (file == NULL)
		return NULL;
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}

	length = ftell(file);
	if (length < 0) {
		fclose(file);
		return NULL;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}

	source = malloc((size_t)length + 1);
	if (source == NULL) {
		fclose(file);
		return NULL;
	}

	read_size = fread(source, 1, (size_t)length, file);
	if (read_size != (size_t)length) {
		free(source);
		fclose(file);
		return NULL;
	}
	source[length] = '\0';
	fclose(file);

	return source;
}

/* Find the single accelerator-hinted function in the current HIR table. */
static struct hir_block *
find_accel_function(
	void)
{
	struct hir_block *result;
	struct hir_block *func_block;
	uint32_t count;
	uint32_t i;

	result = NULL;
	count = hir_get_function_count();

	/* Find the one fixture function carrying the accelerator hint. */
	for (i = 0; i < count; i++) {
		func_block = hir_get_function(i);
		if (!func_block->val.func.is_accel)
			continue;
		if (result != NULL)
			return NULL;
		result = func_block;
	}

	return result;
}

/* Compile and validate a two-kernel int/float accelerator candidate. */
static bool
run_applied_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct accel_program *clone;
	struct hir_block *func_block;
	const struct accel_program *program;
	int64_t scalar_value[ACCEL_MAX_SCALAR_BINDINGS];
	int64_t trip;
	int64_t required_end;
	char error[160];
	enum accel_compile_status status;
	uint32_t i;
	bool valid;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, "doall.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "doall.noct was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	if (!func_block->val.func.is_accel) {
		fprintf(stderr, "analysis mutated the accelerator hint\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	if (accel_function_plan_get_region_count(plan) != 1 ||
	    accel_function_plan_get_generated_local_count(plan) != 2) {
		fprintf(stderr, "incorrect function plan shape\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	program = accel_function_plan_get_region(plan, 0);
	if (program == NULL || program->kernel_count != 3) {
		fprintf(stderr, "incorrect region kernel count\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	if (!accel_program_validate(program, error, sizeof(error))) {
		fprintf(stderr, "program validation failed: %s\n", error);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Keep the initial transfer policy conservative for dynamic ranges. */
	for (i = 0; i < program->buffer_count; i++) {
		if (!program->buffer[i].upload_required) {
			fprintf(stderr, "unsafe upload elision in initial plan\n");
			cleanup_case();
			accel_function_plan_destroy(plan);
			return false;
		}
	}

	memset(scalar_value, 0, sizeof(scalar_value));

	/* Supply dynamic n and scale values by program scalar-binding order. */
	for (i = 0; i < program->scalar_count; i++) {
		if (strcmp(program->scalar[i].name, "n") == 0)
			scalar_value[i] = 7;
	}
	if (!accel_program_evaluate_size(
		program,
		program->kernel[0].trip_expression,
		program->scalar_count,
		scalar_value,
		&trip)) {
		fprintf(stderr, "trip evaluation failed\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	if (trip != 7) {
		fprintf(stderr, "incorrect dynamic trip result\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	required_end = -1;

	/* Verify the affine source[i + 1] requirement is preserved in the DAG. */
	for (i = 0; i < program->buffer_count; i++) {
		if (strcmp(program->buffer[i].name, "source") != 0)
			continue;
		if (!accel_program_evaluate_size(
			program,
			program->buffer[i].required_end_expression,
			program->scalar_count,
			scalar_value,
			&required_end)) {
			fprintf(stderr, "required range evaluation failed\n");
			cleanup_case();
			accel_function_plan_destroy(plan);
			return false;
		}
	}
	if (required_end != 8) {
		fprintf(stderr, "affine required range was not preserved\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	clone = accel_program_clone(program);
	if (clone == NULL) {
		fprintf(stderr, "program clone failed\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Destroy all HIR/AST arenas before checking deep-owned plan strings. */
	cleanup_case();
	valid = accel_program_validate(program, error, sizeof(error));
	if (!valid || strcmp(program->function_name, "transform") != 0) {
		fprintf(stderr, "program retained source-arena storage\n");
		accel_program_destroy(clone);
		accel_function_plan_destroy(plan);
		return false;
	}
	if (!accel_program_validate(clone, error, sizeof(error))) {
		fprintf(stderr, "cloned program validation failed: %s\n", error);
		accel_program_destroy(clone);
		accel_function_plan_destroy(plan);
		return false;
	}

	accel_program_destroy(clone);
	accel_function_plan_destroy(plan);

	return true;
}

/* Compile and validate two maximal regions separated by CPU code. */
static bool
run_multi_region_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *first;
	const struct accel_program *second;
	enum accel_compile_status status;
	uint32_t i;
	bool destination_found;
	bool scratch_found;
	bool source_found;
	bool valid;

	plan = NULL;
	func_block = NULL;
	destination_found = false;
	scratch_found = false;
	source_found = false;
	if (!build_case(directory, "multi-region.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "multi-region.noct was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	valid = true;
	if (accel_function_plan_get_region_count(plan) != 2 ||
	    accel_function_plan_get_generated_local_count(plan) != 4) {
		fprintf(stderr, "incorrect multi-region function plan shape\n");
		valid = false;
	}

	first = accel_function_plan_get_region(plan, 0);
	second = accel_function_plan_get_region(plan, 1);
	if (first == NULL || second == NULL) {
		fprintf(stderr, "missing multi-region program\n");
		valid = false;
	} else if (first->region_index != 0 || second->region_index != 1 ||
		   first->kernel_count != 2 || second->kernel_count != 1) {
		fprintf(stderr, "incorrect region-local kernel layout\n");
		valid = false;
	} else if (first->last_block_id >= second->first_block_id) {
		fprintf(stderr, "multi-region programs are not source ordered\n");
		valid = false;
	} else if (first->kernel[0].kernel_index != 0 ||
		   first->kernel[1].kernel_index != 1 ||
		   second->kernel[0].kernel_index != 0) {
		fprintf(stderr, "kernel indices are not region local\n");
		valid = false;
	} else if (first->buffer_count != 3 || second->buffer_count != 2) {
		fprintf(stderr, "buffer bindings leaked across regions\n");
		valid = false;
	}

	/* Verify conservative host transfers on the second region only. */
	if (second != NULL) {
		for (i = 0; i < second->buffer_count; i++) {
			if (strcmp(second->buffer[i].name, "source") == 0)
				source_found = true;
			if (strcmp(second->buffer[i].name, "scratch") == 0) {
				scratch_found = true;
				if (!second->buffer[i].upload_required ||
				    second->buffer[i].download_required) {
					valid = false;
				}
			}
			if (strcmp(second->buffer[i].name, "destination") == 0) {
				destination_found = true;
				if (!second->buffer[i].upload_required ||
				    !second->buffer[i].download_required) {
					valid = false;
				}
			}
		}
		if (source_found || !scratch_found || !destination_found) {
			fprintf(stderr, "incorrect second-region transfer plan\n");
			valid = false;
		}
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	return valid;
}

/* Compile one integer reduction and verify its scalar-result IR contract. */
static bool
run_dosum_case(
	const char *directory,
	const char *name,
	uint32_t kernel_count,
	uint32_t consumer_mask,
	uint32_t result_args_slot)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *program;
	const struct accel_ir_instruction *instruction;
	enum accel_compile_status status;
	char error[160];
	uint32_t atomic_count;
	uint32_t load_count;
	uint32_t i;
	uint32_t j;
	bool valid;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, name, &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "%s reduction was not applied\n", name);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	valid = true;
	program = accel_function_plan_get_region(plan, 0);
	if (accel_function_plan_get_region_count(plan) != 1 ||
	    program == NULL || program->kernel_count != kernel_count) {
		fprintf(stderr, "%s has an incorrect reduction region\n", name);
		valid = false;
	}

	/* Match the dense result descriptor before inspecting its IR uses. */
	if (program != NULL) {
		if (program->scalar_result_count != 1 ||
		    strcmp(program->scalar_result[0].name, "sum") != 0 ||
		    program->scalar_result[0].result_entry_id != 0 ||
		    program->scalar_result[0].args_slot != result_args_slot ||
		    program->scalar_result[0].value_type != ACCEL_IR_I32 ||
		    program->scalar_result[0].identity_bits != 0 ||
		    program->scalar_result[0].producer_kernel != 0 ||
		    program->scalar_result[0].gpu_consumer_mask != consumer_mask ||
		    !program->scalar_result[0].cpu_publication) {
			fprintf(stderr, "%s has invalid scalar-result metadata\n", name);
			valid = false;
		}
	}

	atomic_count = 0;
	load_count = 0;

	/* Count exact producer and consumer opcodes across the region. */
	if (program != NULL) {
		for (i = 0; i < program->kernel_count; i++) {
			for (j = 0; j < program->kernel[i].ir->instruction_count; j++) {
				instruction =
					&program->kernel[i].ir->instruction[j];
				if (instruction->opcode ==
				    ACCEL_IR_ATOMIC_ADD_I32) {
					atomic_count++;
					if (instruction->reference != 0 || i != 0)
						valid = false;
				} else if (instruction->opcode ==
					   ACCEL_IR_LOAD_RESULT_I32) {
					load_count++;
					if (instruction->reference != 0 || i == 0)
						valid = false;
				}
			}
		}
		if (!accel_program_validate(program, error, sizeof(error))) {
			fprintf(stderr, "%s validation failed: %s\n", name, error);
			valid = false;
		}
	}
	if (atomic_count != 1) {
		fprintf(stderr, "%s has no unique reduction producer\n", name);
		valid = false;
	}
	if ((consumer_mask == 0 && load_count != 0) ||
	    (consumer_mask != 0 && load_count == 0)) {
		fprintf(stderr, "%s has incorrect reduction consumers\n", name);
		valid = false;
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	return valid;
}

/* Keep one sole zero initializer inside a three-kernel region. */
static bool
run_transparent_dosum_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *program;
	const struct accel_ir_instruction *instruction;
	int64_t scalar_value[ACCEL_MAX_SCALAR_BINDINGS];
	int64_t trip;
	enum accel_compile_status status;
	char error[160];
	uint32_t atomic_count;
	uint32_t load_count;
	uint32_t i;
	uint32_t j;
	bool valid;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, "dosum-transparent.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "transparent DOSUM fixture was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	valid = true;
	program = accel_function_plan_get_region(plan, 0);
	if (accel_function_plan_get_region_count(plan) != 1 ||
	    program == NULL || program->kernel_count != 3) {
		fprintf(stderr, "transparent initializer split the GPU region\n");
		valid = false;
	}

	/* Require the result to be produced by the middle kernel. */
	if (program != NULL) {
		if (program->first_block_id != program->kernel[0].loop_block_id ||
		    program->last_block_id != program->kernel[2].loop_block_id ||
		    program->scalar_result_count != 1 ||
		    strcmp(program->scalar_result[0].name, "sum") != 0 ||
		    program->scalar_result[0].args_slot != 3 ||
		    program->scalar_result[0].producer_kernel != 1 ||
		    program->scalar_result[0].gpu_consumer_mask !=
			((uint32_t)1U << 2) ||
		    !program->scalar_result[0].cpu_publication) {
			fprintf(stderr, "transparent DOSUM metadata is invalid\n");
			valid = false;
		}
	}

	memset(scalar_value, 0, sizeof(scalar_value));
	trip = -1;

	/* Require a zero runtime extent to retain the additive identity. */
	if (program != NULL &&
	    (!accel_program_evaluate_size(
		program,
		program->kernel[1].trip_expression,
		program->scalar_count,
		scalar_value,
		&trip) ||
	     trip != 0 ||
	     program->scalar_result[0].identity_bits != 0)) {
		fprintf(stderr, "transparent DOSUM zero-trip identity is invalid\n");
		valid = false;
	}

	atomic_count = 0;
	load_count = 0;

	/* Verify the producer and later consumer remain region-local. */
	if (program != NULL) {
		for (i = 0; i < program->kernel_count; i++) {
			for (j = 0; j < program->kernel[i].ir->instruction_count; j++) {
				instruction =
					&program->kernel[i].ir->instruction[j];
				if (instruction->opcode ==
				    ACCEL_IR_ATOMIC_ADD_I32) {
					atomic_count++;
					if (i != 1 || instruction->reference != 0)
						valid = false;
				} else if (instruction->opcode ==
					   ACCEL_IR_LOAD_RESULT_I32) {
					load_count++;
					if (i != 2 || instruction->reference != 0)
						valid = false;
				}
			}
		}

		if (!accel_program_validate(program, error, sizeof(error))) {
			fprintf(stderr, "transparent DOSUM validation failed: %s\n", error);
			valid = false;
		}
	}
	if (atomic_count != 1 || load_count == 0) {
		fprintf(stderr, "transparent DOSUM IR is incomplete\n");
		valid = false;
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	return valid;
}

/* Allocate dense entries for two reductions in one transparent region. */
static bool
run_multiple_dosum_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *program;
	const struct accel_ir_instruction *instruction;
	enum accel_compile_status status;
	char error[160];
	uint32_t atomic_mask;
	uint32_t load_mask;
	uint32_t i;
	uint32_t j;
	bool valid;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, "dosum-multiple.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "multiple DOSUM fixture was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	valid = true;
	program = accel_function_plan_get_region(plan, 0);
	if (accel_function_plan_get_region_count(plan) != 1 ||
	    program == NULL || program->kernel_count != 3 ||
	    program->scalar_result_count != 2) {
		fprintf(stderr, "multiple DOSUM region shape is invalid\n");
		valid = false;
	}

	/* Match both dense result entries and their shared consumer kernel. */
	if (program != NULL && program->scalar_result_count == 2) {
		if (program->scalar_result[0].result_entry_id != 0 ||
		    strcmp(program->scalar_result[0].name, "first_sum") != 0 ||
		    program->scalar_result[0].args_slot != 3 ||
		    program->scalar_result[0].producer_kernel != 0 ||
		    program->scalar_result[0].gpu_consumer_mask !=
			((uint32_t)1U << 2) ||
		    program->scalar_result[1].result_entry_id != 1 ||
		    strcmp(program->scalar_result[1].name, "second_sum") != 0 ||
		    program->scalar_result[1].args_slot != 4 ||
		    program->scalar_result[1].producer_kernel != 1 ||
		    program->scalar_result[1].gpu_consumer_mask !=
			((uint32_t)1U << 2)) {
			fprintf(stderr, "multiple DOSUM entries are not dense\n");
			valid = false;
		}
	}

	atomic_mask = 0;
	load_mask = 0;

	/* Collect exact result-entry references across all three kernels. */
	if (program != NULL) {
		for (i = 0; i < program->kernel_count; i++) {
			for (j = 0; j < program->kernel[i].ir->instruction_count; j++) {
				instruction =
					&program->kernel[i].ir->instruction[j];
				if (instruction->opcode ==
				    ACCEL_IR_ATOMIC_ADD_I32) {
					if (instruction->reference >= 2 ||
					    instruction->reference != i) {
						valid = false;
					} else {
						atomic_mask |=
							(uint32_t)1U <<
							instruction->reference;
					}
				} else if (instruction->opcode ==
					   ACCEL_IR_LOAD_RESULT_I32) {
					if (i != 2 || instruction->reference >= 2) {
						valid = false;
					} else {
						load_mask |=
							(uint32_t)1U <<
							instruction->reference;
					}
				}
			}
		}

		if (!accel_program_validate(program, error, sizeof(error))) {
			fprintf(stderr, "multiple DOSUM validation failed: %s\n", error);
			valid = false;
		}
	}
	if (atomic_mask != 3 || load_mask != 3) {
		fprintf(stderr, "multiple DOSUM IR references are incomplete\n");
		valid = false;
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	return valid;
}

/* Split at a prefixed declaration block while retaining the DOSUM region. */
static bool
run_dosum_prefix_split_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *first;
	const struct accel_program *second;
	enum accel_compile_status status;
	char error[160];
	bool valid;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, "dosum-prefix-split.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "prefixed DOSUM fixture was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	valid = true;
	first = accel_function_plan_get_region(plan, 0);
	second = accel_function_plan_get_region(plan, 1);
	if (accel_function_plan_get_region_count(plan) != 2 ||
	    accel_function_plan_get_generated_local_count(plan) != 4 ||
	    first == NULL || second == NULL) {
		fprintf(stderr, "prefixed initializer did not split the regions\n");
		valid = false;
	} else if (first->kernel_count != 1 ||
		   first->scalar_result_count != 0 ||
		   second->kernel_count != 2 ||
		   second->scalar_result_count != 1 ||
		   second->scalar_result[0].producer_kernel != 0 ||
		   second->scalar_result[0].gpu_consumer_mask !=
			((uint32_t)1U << 1) ||
		   second->scalar_result[0].args_slot != 3) {
		fprintf(stderr, "prefixed DOSUM regions have invalid metadata\n");
		valid = false;
	}

	if (first != NULL &&
	    !accel_program_validate(first, error, sizeof(error))) {
		fprintf(stderr, "first prefixed region is invalid: %s\n", error);
		valid = false;
	}
	if (second != NULL &&
	    !accel_program_validate(second, error, sizeof(error))) {
		fprintf(stderr, "second prefixed region is invalid: %s\n", error);
		valid = false;
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	return valid;
}

/* Compile one ordinary local Packed as a CPU-backed session argument. */
static bool
run_local_host_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *program;
	enum accel_compile_status status;
	uint32_t i;
	bool found;

	plan = NULL;
	func_block = NULL;
	found = false;
	if (!build_case(directory, "local.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "local.noct was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	program = accel_function_plan_get_region(plan, 0);
	if (program == NULL) {
		fprintf(stderr, "local.noct has no region program\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Locate the local by stable source symbol and verify its host contract. */
	for (i = 0; i < program->buffer_count; i++) {
		if (strcmp(program->buffer[i].name, "temporary") != 0)
			continue;
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_HOST ||
		    program->buffer[i].args_slot !=
		    func_block->val.func.param_count ||
		    !program->buffer[i].upload_required) {
			fprintf(stderr, "incorrect local host residency plan\n");
			cleanup_case();
			accel_function_plan_destroy(plan);
			return false;
		}
		found = true;
	}

	cleanup_case();
	accel_function_plan_destroy(plan);
	if (!found) {
		fprintf(stderr, "local host buffer was not planned\n");
		return false;
	}

	return true;
}

/* Compile one proven local without a host Packed argument or transfer. */
static bool
run_device_only_case(
	const char *directory,
	const char *name)
{
	struct accel_function_plan *plan;
	struct accel_program *clone;
	struct hir_block *func_block;
	const struct accel_program *program;
	const struct accel_buffer_binding *device;
	enum accel_compile_status status;
	char error[160];
	uint32_t saved_kernel_count;
	uint32_t saved_trip;
	uint32_t device_index;
	uint32_t i;
	uint32_t j;
	bool consumer_mutated;
	bool found;
	bool valid;

	plan = NULL;
	clone = NULL;
	func_block = NULL;
	device = NULL;
	device_index = ACCEL_PROGRAM_INDEX_NONE;
	found = false;
	valid = true;
	if (!build_case(directory, name, &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "device-only.noct was not applied\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	program = accel_function_plan_get_region(plan, 0);
	if (program == NULL || program->kernel_count != 2) {
		fprintf(stderr, "device-only.noct has an invalid region\n");
		valid = false;
	}

	/* Find the promoted local and verify its host-free descriptor. */
	if (program != NULL) {
		for (i = 0; i < program->buffer_count; i++) {
			if (strcmp(program->buffer[i].name, "temporary") != 0)
				continue;
			device = &program->buffer[i];
			device_index = i;
			found = true;
		}
	}
	if (!found ||
	    device->origin != ACCEL_BUFFER_LOCAL_DEVICE ||
	    device->args_slot != ACCEL_ARGS_SLOT_NONE ||
	    device->host_visible ||
	    device->upload_required ||
	    device->download_required ||
	    device->materialization_required ||
	    device->extent_expression >= program->size_expression_count ||
	    program->parameter_count != func_block->val.func.param_count ||
	    program->first_block_id == program->kernel[0].loop_block_id) {
		fprintf(stderr, "device-only.noct has an invalid residency plan\n");
		valid = false;
	}
	if (program != NULL &&
	    !accel_program_validate(program, error, sizeof(error))) {
		fprintf(stderr, "device-only program validation failed: %s\n", error);
		valid = false;
	}

	/* Reject a malformed trip that could skip the required producer. */
	if (program != NULL) {
		clone = accel_program_clone(program);
		if (clone == NULL) {
			valid = false;
		} else {
			saved_trip = clone->kernel[0].trip_expression;
			clone->kernel[0].trip_expression =
				clone->kernel[0].start_expression;
			if (accel_program_validate(clone, error, sizeof(error)))
				valid = false;
			clone->kernel[0].trip_expression = saved_trip;

			saved_kernel_count = clone->kernel_count;
			clone->kernel_count = 0;
			if (accel_program_validate(clone, error, sizeof(error)))
				valid = false;
			clone->kernel_count = saved_kernel_count;

			/* Reject a device descriptor without an actual later IR load. */
			consumer_mutated = false;
			for (i = 1; i < clone->kernel_count; i++) {
				for (j = 0;
				     j < clone->kernel[i].ir->instruction_count;
				     j++) {
					if (clone->kernel[i].ir->instruction[j].opcode !=
					    ACCEL_IR_BUFFER_LOAD) {
						continue;
					}
					if (clone->kernel[i].ir->instruction[j].reference !=
					    device_index) {
						continue;
					}
					clone->kernel[i].ir->instruction[j].reference =
						device_index == 0 ? 1 : 0;
					consumer_mutated = true;
				}
			}
			if (!consumer_mutated ||
			    accel_program_validate(clone, error, sizeof(error))) {
				valid = false;
			}
		}
	}

	accel_program_destroy(clone);
	cleanup_case();
	accel_function_plan_destroy(plan);

	/* Reports the complete focused device-local contract. */
	return valid;
}

/* Keep every unproven local CPU-backed or decline the optional transform. */
static bool
run_nondevice_case(
	const char *directory,
	const char *name)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	const struct accel_program *program;
	enum accel_compile_status status;
	uint32_t region_count;
	uint32_t i;
	uint32_t j;
	bool valid;

	plan = NULL;
	func_block = NULL;
	valid = true;
	if (!build_case(directory, name, &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status == ACCEL_COMPILE_ERROR) {
		fprintf(stderr, "%s failed instead of falling back\n", name);
		valid = false;
	} else if (status == ACCEL_COMPILE_APPLIED) {
		region_count = accel_function_plan_get_region_count(plan);

		/* Reject device residency in every retained region program. */
		for (i = 0; i < region_count; i++) {
			program = accel_function_plan_get_region(plan, i);
			if (program == NULL) {
				valid = false;
				continue;
			}
			for (j = 0; j < program->buffer_count; j++) {
				if (strcmp(program->buffer[j].name, "temporary") == 0 &&
				    program->buffer[j].origin ==
					ACCEL_BUFFER_LOCAL_DEVICE) {
					valid = false;
				}
			}
		}
	}

	cleanup_case();
	accel_function_plan_destroy(plan);

	/* Reports a safe CPU-backed or all-CPU fallback decision. */
	return valid;
}

/* Compile one valid CPU function that the initial GPU subset must decline. */
static bool
run_declined_case(
	const char *directory,
	const char *name)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	enum accel_compile_status status;

	plan = NULL;
	func_block = NULL;
	if (!build_case(directory, name, &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	cleanup_case();
	if (status != ACCEL_COMPILE_DECLINED || plan != NULL) {
		fprintf(stderr, "%s did not decline cleanly\n", name);
		accel_function_plan_destroy(plan);
		return false;
	}

	return true;
}

/* Ensure the validator rejects a use-before-definition buffer index. */
static bool
run_invalid_ir_case(
	void)
{
	struct accel_ir_instruction instruction;
	struct accel_ir_builder builder;
	struct accel_ir_kernel *kernel;
	uint32_t result;
	char error[160];
	bool valid;

	kernel = accel_ir_kernel_create("invalid", 1, 1, 0, 1);
	if (kernel == NULL)
		return false;
	if (!accel_ir_kernel_set_buffer_type(kernel, 0, ACCEL_IR_I32)) {
		accel_ir_kernel_destroy(kernel);
		return false;
	}

	accel_ir_builder_init(&builder, kernel);
	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_BUFFER_LOAD;
	instruction.result_type = ACCEL_IR_I32;
	instruction.result = ACCEL_IR_VALUE_NONE;
	instruction.operand[0] = 7;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = 0;
	if (!accel_ir_builder_append(&builder, &instruction, &result)) {
		accel_ir_kernel_destroy(kernel);
		return false;
	}

	valid = accel_ir_kernel_validate(kernel, error, sizeof(error));
	accel_ir_kernel_destroy(kernel);
	if (valid) {
		fprintf(stderr, "invalid IR was accepted\n");
		return false;
	}

	return true;
}

/* Parse, build, type, and return the fixture's accelerator HIR function. */
static bool
build_case(
	const char *directory,
	const char *name,
	struct hir_block **func_block)
{
	char *source;

	*func_block = NULL;
	source = read_source(directory, name);
	if (source == NULL) {
		fprintf(stderr, "failed to read %s\n", name);
		return false;
	}

	if (!ast_build(name, source)) {
		fprintf(stderr, "%s:%d: %s\n", name, ast_get_error_line(), ast_get_error_message());
		free(source);
		ast_cleanup();
		return false;
	}
	free(source);

	if (!hir_build()) {
		fprintf(stderr, "%s:%d: %s\n", name, hir_get_error_line(), hir_get_error_message());
		hir_cleanup();
		ast_cleanup();
		return false;
	}

	*func_block = find_accel_function();
	if (*func_block == NULL) {
		fprintf(stderr, "%s has no unique accelerator function\n", name);
		cleanup_case();
		return false;
	}
	if (!hir_opt_typed_func(*func_block)) {
		fprintf(stderr, "%s typed pass failed\n", name);
		cleanup_case();
		return false;
	}

	return true;
}

/* Release the current HIR before its source AST arena. */
static void
cleanup_case(
	void)
{
	hir_cleanup();
	ast_cleanup();
}
