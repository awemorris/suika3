/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Run Mode
 */

#include <noct/noct.h>
#include "bytecode.h"
#include "bytecode_file.h"
#include "cli-main.h"

#if defined(NOCT_USE_ACCEL)
#include "accel_cli.h"
#endif

#if defined(NOCT_TARGET_WINDOWS)
#include "cli-win32.h"
#endif

enum cli_program_kind {
	CLI_PROGRAM_SOURCE,
	CLI_PROGRAM_BYTECODE
};

struct cli_program_input {
	uint8_t *storage;
	size_t storage_size;
	const uint8_t *payload;
	size_t payload_size;
	const char *file_name;
	enum bytecode_file_kind bytecode_kind;
	enum cli_program_kind kind;
	bool has_shebang;
};

static NoctVM *vm;
static NoctEnv *env;
static NoctConfig config;
static NoctValue arg;
static int file_arg;
static int prog_arg;
static size_t param_count;
static bool is_oneliner;
static bool gpu_requested;
static bool gpu_list_requested;
static char *gpu_name;
static struct cli_program_input program_input;

static bool parse_options(int argc, char *argv[]);
static bool parse_gpu_option(int argc, char *argv[], int index);
static int list_gpu_devices(void);
#if defined(NOCT_USE_ACCEL)
static bool print_gpu_device(const char *selector, void *userdata);
#endif
static bool prepare_program_input(int argc, char *argv[]);
static bool prepare_file_input(const char *path);
static bool prepare_oneliner_input(const char *command);
static bool classify_program_input(void);
static void cleanup_program_input(void);
static bool load_program(void);
static bool load_args(int argc, char *argv[]);
static bool check_params(const char *entry_name);

/*
 * Top level function for the run mode.
 */
int
command_run(
	int argc,
	char *argv[])
{
	NoctValue ret;
	const char *file;
	const char *message;
	int line;
	int result;

	vm = NULL;
	env = NULL;
	gpu_name = NULL;
	result = 1;
	memset(&program_input, 0, sizeof(program_input));
	noct_set_default_config(&config);
	cli_module_reset();
	config.require_resolver = cli_module_resolve;

	if (!parse_options(argc, argv))
		goto cleanup;

	/* List devices before creating source, module, or VM state. */
	if (gpu_list_requested) {
		if (gpu_requested || argc != 2 || file_arg != argc || is_oneliner) {
			wide_printf(N_TR("--gpu-list must be used by itself.\n"));
			cli_module_reset();
			return 1;
		}

		result = list_gpu_devices();
		cli_module_reset();
		return result;
	}

	if (file_arg == argc && !is_oneliner) {
		if (argc == 1) {
#if defined(NOCT_USE_REPL)
			result = command_repl();
			goto cleanup;
#else
			show_usage();
			goto cleanup;
#endif
		}
		goto cleanup;
	}

	if (!prepare_program_input(argc, argv))
		goto cleanup;
	if (!cli_module_build_input_graph(
		program_input.file_name,
		program_input.payload,
		program_input.payload_size,
		config.require_resolver)) {
		wide_printf(N_TR("%s\n"), cli_module_get_error());
		goto cleanup;
	}
	if (gpu_requested && program_input.kind != CLI_PROGRAM_SOURCE) {
		wide_printf(N_TR("GPU acceleration is available only when running Noct source.\n"));
		goto cleanup;
	}
#if !defined(NOCT_USE_ACCEL)
	if (gpu_requested) {
		wide_printf(N_TR("GPU acceleration is not available in this build.\n"));
		goto cleanup;
	}
#endif

	if (!noct_create_vm(&vm, &env, &config)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!noct_register_api_system(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!noct_register_api_console(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!noct_register_api_file(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (!noct_register_api_regex(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#if defined(NOCT_USE_MULTITHREAD)
	if (!noct_register_api_thread(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#endif
#if defined(NOCT_USE_HTTPSERVER)
	if (!noct_register_api_httpserver(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#endif
#if defined(NOCT_USE_TERM)
	if (!noct_register_api_term(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#endif
#if defined(NOCT_USE_BEUI)
	if (!noct_register_api_beui(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#endif
#if defined(NOCT_USE_PROCESS)
	if (!noct_register_api_process(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
#endif

	if (!register_cli_cfunc(env)) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}

#if defined(NOCT_USE_ACCEL)
	if (gpu_requested && !accel_initialize(vm, env, gpu_name))
		goto runtime_error;
#endif

	if (!load_program())
		goto runtime_error;
	if (!load_args(argc, argv))
		goto runtime_error;
	if (!check_params("main"))
		goto cleanup;

	if (!noct_enter_vm(
		env,
		"main",
		param_count == 0 ? 0 : 1,
		&arg,
		&ret)) {
		goto runtime_error;
	}

	result = 0;
	goto cleanup;

runtime_error:
	noct_get_error_file(env, &file);
	noct_get_error_line(env, &line);
	noct_get_error_message(env, &message);
	wide_printf(N_TR("%s:%d: Error: %s\n"), file, line, message);

cleanup:
	if (vm != NULL) {
#if defined(NOCT_USE_ACCEL)
		accel_finalize(vm);
#endif
		if (!noct_destroy_vm(vm))
			result = 1;
		vm = NULL;
		env = NULL;
	}
	free(gpu_name);
	gpu_name = NULL;
	cleanup_program_input();
	cli_module_reset();

	return result;
}

static bool
parse_options(int argc, char *argv[])
{
	int i;
	int optimize_level;
	bool lineinfo;
	enum cli_optimize_level_result optimize_result;

	file_arg = 1;
	is_oneliner = false;
	gpu_requested = false;
	gpu_list_requested = false;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (strcmp(argv[i], "-j0") == 0) {
			config.jit_enable = false;
			file_arg++;
			continue;
		}
		if (strcmp(argv[i], "-j") == 0) {
			config.jit_enable = true;
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--jit-code-size=", 16) == 0) {
			config.jit_code_size = (size_t)atoi(argv[i] + 16);
			file_arg++;
			continue;
		}
		optimize_result = parse_optimize_level_option(
		    argv[i], &optimize_level, &lineinfo);
		if (optimize_result == CLI_OPTIMIZE_LEVEL_VALID) {
			config.optimize_level = optimize_level;
			config.line_info = lineinfo;
			file_arg++;
			continue;
		}
		if (optimize_result == CLI_OPTIMIZE_LEVEL_INVALID) {
			wide_printf(N_TR("Invalid optimize-level option %s.\n"),
				    argv[i]);
			return false;
		}
		if (strcmp(argv[i], "--simd-info") == 0) {
			config.simd_info = true;
			file_arg++;
			continue;
		}
		if (strcmp(argv[i], "--gpu") == 0 ||
		    strncmp(argv[i], "--gpu=", 6) == 0) {
			if (!parse_gpu_option(argc, argv, i))
				return false;
			file_arg++;
			continue;
		}
		if (strcmp(argv[i], "--gpu-list") == 0) {
			if (gpu_list_requested) {
				wide_printf(N_TR("--gpu-list may be specified only once.\n"));
				return false;
			}
			gpu_list_requested = true;
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--path=", 7) == 0) {
			if (!cli_module_add_path(argv[i] + 7)) {
				wide_printf(N_TR("Invalid --path option.\n"));
				return false;
			}
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-nursery-size=", 18) == 0) {
			config.gc_nursery_size = (size_t)atoi(argv[i] + 18);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-graduate-size=", 21) == 0) {
			config.gc_graduate_size = (size_t)atoi(argv[i] + 21);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-tenure-size=", 17) == 0) {
			config.gc_tenure_size = (size_t)atoi(argv[i] + 17);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-lop-threshold=", 18) == 0) {
			config.gc_lop_threshold = (size_t)atoi(argv[i] + 18);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--gc-promotion-threshold=", 25) == 0) {
			config.gc_promotion_threshold =
			    (size_t)atoi(argv[i] + 25);
			file_arg++;
			continue;
		}
		if (strncmp(argv[i], "--one-line", 10) == 0 ||
		    strcmp(argv[i], "-e") == 0) {
			if (argc <= (int)i + 1) {
				wide_printf(N_TR("Specify a command.\n"));
				return 1;
			}
			is_oneliner = true;
			prog_arg = i + 1;
			i++;
			file_arg++;
			continue;
		}

		wide_printf(N_TR("Unknown option %s.\n"), argv[i]);
		return false;
	}

	return true;
}

/* List every suitable accelerator device in canonical selector form. */
static int
list_gpu_devices(
	void)
{
#if defined(NOCT_USE_ACCEL)
	char error[256];
	size_t device_count;

	error[0] = '\0';
	device_count = 0;

	/* Enumerate devices through the private CLI accelerator boundary. */
	if (!accel_list_devices(
		print_gpu_device,
		NULL,
		error,
		sizeof(error),
		&device_count)) {
		if (error[0] != '\0')
			wide_printf(N_TR("GPU enumeration failed: %s\n"), error);
		else
			wide_printf(N_TR("GPU enumeration failed.\n"));
		return 1;
	}

	/* Distinguish an empty suitable-device set from an API failure. */
	if (device_count == 0) {
		wide_printf(N_TR("No suitable GPU device is available.\n"));
		return 1;
	}

	return 0;
#else
	wide_printf(N_TR("GPU acceleration is not available in this build.\n"));

	return 1;
#endif
}

#if defined(NOCT_USE_ACCEL)
/* Print one canonical accelerator selector. */
static bool
print_gpu_device(
	const char *selector,
	void *userdata)
{
	UNUSED_PARAMETER(userdata);

	if (selector == NULL)
		return false;

	wide_printf("%s\n", selector);

	return true;
}
#endif

/* Parse one accelerator selection without making it build-dependent. */
static bool
parse_gpu_option(
	int argc,
	char *argv[],
	int index)
{
	const char *option;
	const char *name;
#if !defined(NOCT_TARGET_WINDOWS)
	size_t name_size;
#endif

#if !defined(NOCT_TARGET_WINDOWS)
	UNUSED_PARAMETER(argc);
#endif

	if (gpu_requested) {
		wide_printf(N_TR("GPU acceleration may be selected only once.\n"));
		return false;
	}

	option = argv[index];
	if (strcmp(option, "--gpu") == 0) {
		gpu_requested = true;
		return true;
	}

	name = option + 6;
	if (name[0] == '\0') {
		wide_printf(N_TR("GPU device name must not be empty.\n"));
		return false;
	}

#if defined(NOCT_TARGET_WINDOWS)
	if (!cli_windows_gpu_name_utf8(argc, index, &gpu_name)) {
		wide_printf(N_TR("Invalid GPU device name.\n"));
		return false;
	}
#else
	name_size = strlen(name);
	gpu_name = malloc(name_size + 1);
	if (gpu_name == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	memcpy(gpu_name, name, name_size + 1);
#endif

	gpu_requested = true;

	return true;
}

static bool
prepare_program_input(
	int argc,
	char *argv[])
{
	if (is_oneliner)
		return prepare_oneliner_input(argv[prog_arg]);
	if (file_arg >= argc)
		return false;

	return prepare_file_input(argv[file_arg]);
}

/* Read one program file once into an owned binary descriptor. */
static bool
prepare_file_input(
	const char *path)
{
	FILE *stream;
	long file_size;
	size_t read_size;
	bool succeeded;

	stream = NULL;
	succeeded = false;

	stream = fopen(path, "rb");
	if (stream == NULL)
		goto failure;
	if (fseek(stream, 0, SEEK_END) != 0)
		goto failure;
	file_size = ftell(stream);
	if (file_size < 0)
		goto failure;
	if (fseek(stream, 0, SEEK_SET) != 0)
		goto failure;

	read_size = (size_t)file_size;
	if ((long)read_size != file_size || read_size == SIZE_MAX)
		goto failure;
	program_input.storage = malloc(read_size + 1);
	if (program_input.storage == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		goto cleanup;
	}
	if (fread(program_input.storage, 1, read_size, stream) != read_size)
		goto failure;
	program_input.storage[read_size] = '\0';
	program_input.storage_size = read_size;
	program_input.file_name = path;
	if (fclose(stream) != 0) {
		stream = NULL;
		goto failure;
	}
	stream = NULL;

	succeeded = classify_program_input();
	goto cleanup;

failure:
	wide_printf(N_TR("Cannot read file %s.\n"), path);

cleanup:
	if (stream != NULL)
		fclose(stream);
	if (!succeeded)
		cleanup_program_input();

	return succeeded;
}

/* Wrap one command-line expression in an owned source descriptor. */
static bool
prepare_oneliner_input(
	const char *command)
{
	const char *prefix;
	const char *suffix;
	size_t prefix_size;
	size_t command_size;
	size_t suffix_size;
	size_t source_size;

	prefix = "func main() { ";
	suffix = "; }";
	prefix_size = strlen(prefix);
	command_size = strlen(command);
	suffix_size = strlen(suffix);
	if (prefix_size > SIZE_MAX - command_size)
		return false;
	source_size = prefix_size + command_size;
	if (source_size > SIZE_MAX - suffix_size)
		return false;
	source_size += suffix_size;
	if (source_size == SIZE_MAX)
		return false;

	program_input.storage = malloc(source_size + 1);
	if (program_input.storage == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}
	memcpy(program_input.storage, prefix, prefix_size);
	memcpy(
		program_input.storage + prefix_size,
		command,
		command_size);
	memcpy(
		program_input.storage + prefix_size + command_size,
		suffix,
		suffix_size + 1);
	program_input.storage_size = source_size;
	program_input.payload = program_input.storage;
	program_input.payload_size = source_size;
	program_input.file_name = "oneliner";
	program_input.bytecode_kind = BYTECODE_FILE_UNKNOWN;
	program_input.kind = CLI_PROGRAM_SOURCE;

	return true;
}

/* Classify one file payload by exact shebang and content magic. */
static bool
classify_program_input(
	void)
{
	size_t shebang_size;
	uint32_t registration_size;

	program_input.payload = program_input.storage;
	program_input.payload_size = program_input.storage_size;
	shebang_size = strlen(NOCT_APP_SHEBANG);
	if (program_input.payload_size >= shebang_size &&
	    memcmp(
		program_input.payload,
		NOCT_APP_SHEBANG,
		shebang_size) == 0) {
		program_input.payload += shebang_size;
		program_input.payload_size -= shebang_size;
		program_input.has_shebang = true;
	}

	program_input.bytecode_kind = bytecode_file_detect(
		program_input.payload,
		program_input.payload_size);
	if (program_input.bytecode_kind == BYTECODE_FILE_MODULE_UNKNOWN) {
		wide_printf(N_TR("Unsupported or malformed bytecode version.\n"));
		return false;
	}
	if (program_input.bytecode_kind == BYTECODE_FILE_APP_UNKNOWN) {
		wide_printf(N_TR("Unsupported or malformed application version.\n"));
		return false;
	}

	if (program_input.bytecode_kind == BYTECODE_FILE_UNKNOWN) {
		if (memchr(
			program_input.payload,
			'\0',
			program_input.payload_size) != NULL) {
			wide_printf(N_TR("NUL byte in source program.\n"));
			return false;
		}
		program_input.kind = CLI_PROGRAM_SOURCE;

		return true;
	}

	if (!bytecode_file_check_registration_size(
		program_input.payload_size,
		&registration_size)) {
		wide_printf(N_TR("Bytecode program is too large.\n"));
		return false;
	}
	program_input.kind = CLI_PROGRAM_BYTECODE;

	return true;
}

/* Release the current owned program descriptor exactly once. */
static void
cleanup_program_input(
	void)
{
	free(program_input.storage);
	memset(&program_input, 0, sizeof(program_input));
}

/* Register the already classified source, bytecode, or application payload. */
static bool
load_program(
	void)
{
	return cli_module_register_graph(env);
}

static bool
load_args(int argc, char *argv[])
{
	NoctValue val;
	int i;
	size_t index;

	/* Make the arguments for "main()". */
	if (!noct_make_empty_array(env, &arg))
		return false;

	index = 0;

	/* Preserve the CRT narrow argv byte contract on every platform. */
	for (i = file_arg + 1; i < argc; i++) {
		if (!noct_set_array_elem_make_string(
			env,
			&arg,
			index,
			&val,
			argv[i])) {
			return false;
		}
		index++;
	}

	return true;
}

static bool
check_params(const char *entry_name)
{
	NoctValue main_val;
	NoctFunc *main_func;

	/* Check for main(). */
	if (!noct_get_global(env, entry_name, &main_val)) {
		wide_printf(N_TR("%s() is not defined\n"), entry_name);
		return false;
	}

	if (!noct_get_func(env, &main_val, &main_func))
		return false;

	if (!noct_get_func_param_count(env, main_func, &param_count))
		return false;
	if (param_count > 1) {
		wide_printf(N_TR("%s() must have zero or one parameter\n"),
			    entry_name);
		return false;
	}

	return true;
}
