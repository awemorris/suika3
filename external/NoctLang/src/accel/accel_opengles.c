/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Headless OpenGL ES 3.1 accelerator backend using EGL.
 */

#include "accel_opengles.h"
#include "accel_context.h"
#include "accel_mutex.h"
#include "accel_runtime.h"
#include "accel_shader_source.h"
#include "hir.h"
#include "runtime.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_OPENGLES_WORKGROUP_SIZE	64U
#define ACCEL_OPENGLES_BACKEND_PRIORITY	300U
#define ACCEL_OPENGLES_DEVICE_SCORE	300U
#define ACCEL_OPENGLES_MINIMUM_BINDINGS	2

enum accel_opengles_open_status {
	ACCEL_OPENGLES_OPEN_FAILED,
	ACCEL_OPENGLES_OPEN_UNSUITABLE,
	ACCEL_OPENGLES_OPEN_SUITABLE
};

struct accel_opengles_backend {
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	struct accel_mutex mutex;
	char *renderer;
	GLint max_storage_bindings;
	GLint max_compute_storage_blocks;
	GLint max_group_count_x;
	GLint64 max_storage_block_size;
	bool display_initialized;
	bool allocation_failed;
};

struct accel_opengles_kernel {
	GLuint program;
};

struct accel_opengles_prepared {
	struct accel_program *program;
	struct accel_opengles_kernel *kernel;
	uint32_t kernel_count;
};

struct accel_opengles_buffer {
	GLuint name;
	int origin;
	uint32_t args_slot;
	int element_kind;
	uint32_t element_width;
	size_t element_count;
	size_t byte_count;
	bool active;
	bool upload;
	bool download;
};

struct accel_opengles_execution {
	struct accel_opengles_backend *backend;
	const struct accel_opengles_prepared *prepared;
	struct accel_opengles_buffer *buffer;
	uint32_t *scalar_word;
	uint32_t buffer_count;
	GLuint scalar_buffer;
	GLuint result_buffer;
	uint32_t scalar_word_count;
	uint32_t result_word_count;
	uint32_t expected_dispatch_count;
	uint32_t dispatch_count;
	uint32_t last_kernel;
	bool has_active_dispatch;
	bool dispatched;
	bool finished;
};

static bool accel_opengles_set_error(char *error, size_t error_size, const char *message);
static char *accel_opengles_duplicate(const char *text);
static bool accel_opengles_ascii_contains(const char *text, const char *needle);
static bool accel_opengles_is_software_renderer(const char *renderer);
static void accel_opengles_reset_backend(struct accel_opengles_backend *backend);
static enum accel_opengles_open_status accel_opengles_open(struct accel_opengles_backend *backend);
static enum accel_opengles_open_status accel_opengles_try_display(struct accel_opengles_backend *backend, EGLDisplay display);
static bool accel_opengles_query_device(struct accel_opengles_backend *backend);
static void accel_opengles_close(struct accel_opengles_backend *backend);
static bool accel_opengles_acquire(struct accel_opengles_backend *backend, char *error, size_t error_size);
static bool accel_opengles_release(struct accel_opengles_backend *backend, char *error, size_t error_size);
static void accel_opengles_clear_gl_errors(void);
static bool accel_opengles_check_gl(char *error, size_t error_size, const char *message);
static const struct accel_backend_ops *accel_opengles_backend_ops(void);
static const struct accel_executor_ops *accel_opengles_executor_ops(void);
static enum accel_compile_status accel_opengles_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static bool accel_opengles_program_uses_f32(const struct accel_program *program);
static bool accel_opengles_prepare_kernel_current(const struct accel_program *program, uint32_t kernel_index, struct accel_opengles_kernel *result);
static void accel_opengles_report_shader_error(int source_line, const char *message);
static void accel_opengles_destroy_prepared_current(struct accel_opengles_prepared *prepared);
static void accel_opengles_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool accel_opengles_register_runtime(struct accel_context *context, struct rt_env *env);
static void accel_opengles_destroy_backend_state(void *backend_state);
static const struct accel_program *accel_opengles_get_program(const struct accel_prepared_program *prepared);
static bool accel_opengles_validate_dispatch_limit(void *backend_state, const struct accel_prepared_program *prepared, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static uint32_t accel_opengles_count_active_dispatches(const struct accel_program *program, const uint32_t scalar_word[]);
static bool accel_opengles_create_execution(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool accel_opengles_validate_execution_inputs(struct accel_opengles_backend *backend, const struct accel_opengles_prepared *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_opengles_create_buffer_current(struct accel_opengles_backend *backend, size_t byte_count, const void *data, GLuint *result, char *error, size_t error_size);
static void accel_opengles_destroy_execution_current(struct accel_opengles_execution *execution);
static void accel_opengles_free_execution(struct accel_opengles_execution *execution);
static bool accel_opengles_dispatch_execution(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_opengles_bind_execution_current(struct accel_opengles_execution *execution, char *error, size_t error_size);
static bool accel_opengles_finish_execution(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_opengles_validate_finish(const struct accel_opengles_execution *execution, uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_opengles_read_buffer_current(const struct accel_opengles_buffer *source, struct accel_runtime_buffer *destination, char *error, size_t error_size);
static bool accel_opengles_read_result_current(const struct accel_opengles_execution *execution, uint32_t result_word_count, uint32_t result_word[], char *error, size_t error_size);
static void accel_opengles_destroy_execution(void *execution);

/*
 * Enumerates the suitable OpenGL ES compute device exposed by EGL.
 */
bool
accel_opengles_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	struct accel_opengles_backend backend;
	enum accel_opengles_open_status status;
	bool appended;

	/* Clear the optional diagnostic before opening EGL. */
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	/* Reject an absent shared device registry. */
	if (list == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES device list."));

	/* Open a temporary headless context and inspect its compute limits. */
	accel_opengles_reset_backend(&backend);
	status = accel_opengles_open(&backend);
	if (status == ACCEL_OPENGLES_OPEN_FAILED) {
		accel_opengles_close(&backend);
		return accel_opengles_set_error(error, error_size, N_TR("Failed to initialize a headless OpenGL ES 3.1 context."));
	}

	/* Treat a working but unsuitable implementation as an empty backend. */
	if (status == ACCEL_OPENGLES_OPEN_UNSUITABLE) {
		accel_opengles_close(&backend);
		return true;
	}

	/* Append the one renderer exposed by the selected EGL display. */
	appended = accel_device_list_append(
		list,
		ACCEL_BACKEND_OPENGLES,
		backend.renderer,
		ACCEL_OPENGLES_BACKEND_PRIORITY,
		ACCEL_OPENGLES_DEVICE_SCORE,
		(uintptr_t)0);
	accel_opengles_close(&backend);
	if (!appended)
		return accel_opengles_set_error(error, error_size, N_TR("Out of memory while enumerating OpenGL ES devices."));

	/* Report a complete enumeration transaction. */
	return true;
}

/*
 * Creates the OpenGL ES backend for one selected device record.
 */
bool
accel_opengles_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	struct accel_opengles_backend *backend;
	enum accel_opengles_open_status status;

	/* Clear ownership outputs before validating the request. */
	if (ops != NULL)
		*ops = NULL;
	if (backend_state != NULL)
		*backend_state = NULL;

	/* Reject incomplete ownership and runtime arguments. */
	if (env == NULL ||
	    ops == NULL ||
	    backend_state == NULL) {
		return false;
	}

	/* Require a record produced by the OpenGL ES enumerator. */
	if (device == NULL ||
	    device->backend != ACCEL_BACKEND_OPENGLES ||
	    device->name == NULL ||
	    device->name[0] == '\0') {
		rt_error(env, N_TR("Invalid selected OpenGL ES device."));
		return false;
	}

	/* Allocate the private EGL execution state. */
	backend = noct_calloc(1, sizeof(*backend));
	if (backend == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	accel_opengles_reset_backend(backend);

	/* Reopen and revalidate the selected renderer in a fresh context. */
	status = accel_opengles_open(backend);
	if (status != ACCEL_OPENGLES_OPEN_SUITABLE) {
		accel_opengles_close(backend);
		noct_free(backend);
		rt_error(env, N_TR("No suitable OpenGL ES 3.1 compute device is available."));
		return false;
	}

	/* Reject a renderer that changed after enumeration. */
	if (strcmp(device->name, backend->renderer) != 0) {
		accel_opengles_close(backend);
		noct_free(backend);
		rt_error(env, N_TR("The selected OpenGL ES device is no longer available."));
		return false;
	}

	/* Initialize exclusive context ownership before publication. */
	if (!accel_mutex_init(&backend->mutex)) {
		accel_opengles_close(backend);
		noct_free(backend);
		rt_error(env, N_TR("Failed to initialize the OpenGL ES context mutex."));
		return false;
	}

	/* Transfer the complete backend to the accelerator context. */
	*ops = accel_opengles_backend_ops();
	*backend_state = backend;

	/* Report successful backend publication. */
	return true;
}

/* Return the immutable OpenGL ES backend operation table. */
static const struct accel_backend_ops *
accel_opengles_backend_ops(void)
{
	static const struct accel_backend_ops ops = {
		accel_opengles_prepare_program,
		accel_opengles_destroy_prepared_program,
		accel_opengles_register_runtime,
		accel_opengles_destroy_backend_state
	};

	/* Return the process-lifetime backend operations. */
	return &ops;
}

/* Return the immutable OpenGL ES executor operation table. */
static const struct accel_executor_ops *
accel_opengles_executor_ops(void)
{
	static const struct accel_executor_ops ops = {
		"OpenGL ES",
		accel_opengles_get_program,
		accel_opengles_validate_dispatch_limit,
		accel_opengles_create_execution,
		accel_opengles_dispatch_execution,
		accel_opengles_finish_execution,
		accel_opengles_destroy_execution
	};

	/* Return the process-lifetime executor operations. */
	return &ops;
}

/* Copy one stable backend diagnostic into optional caller storage. */
static bool
accel_opengles_set_error(
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

/* Duplicate one renderer name through the configured allocator. */
static char *
accel_opengles_duplicate(
	const char *text)
{
	char *copy;
	size_t length;

	length = strlen(text);

	/* Reject the one impossible terminated-string size. */
	if (length == (size_t)-1)
		return NULL;

	/* Allocate and copy the complete renderer name. */
	copy = noct_malloc(length + 1);
	if (copy == NULL)
		return NULL;
	memcpy(copy, text, length + 1);

	/* Return the deep-owned name. */
	return copy;
}

/* Search one renderer string using locale-independent ASCII folding. */
static bool
accel_opengles_ascii_contains(
	const char *text,
	const char *needle)
{
	const char *candidate;
	const char *left;
	const char *right;
	int left_character;
	int right_character;

	/* Reject absent strings and accept the conventional empty needle. */
	if (text == NULL || needle == NULL)
		return false;
	if (needle[0] == '\0')
		return true;

	/* Compare each possible suffix after folding ASCII letters. */
	for (candidate = text; candidate[0] != '\0'; candidate++) {
		left = candidate;
		right = needle;
		while (left[0] != '\0' && right[0] != '\0') {
			left_character = tolower((unsigned char)left[0]);
			right_character = tolower((unsigned char)right[0]);
			if (left_character != right_character)
				break;
			left++;
			right++;
		}
		if (right[0] == '\0')
			return true;
	}

	/* Report that no complete folded substring matched. */
	return false;
}

/* Reject known CPU rasterizers from accelerator device selection. */
static bool
accel_opengles_is_software_renderer(
	const char *renderer)
{
	/* Reject a renderer that the driver did not identify. */
	if (renderer == NULL || renderer[0] == '\0')
		return true;

	/* Match the common Mesa, platform, and translation-layer CPU drivers. */
	if (accel_opengles_ascii_contains(renderer, "llvmpipe"))
		return true;
	if (accel_opengles_ascii_contains(renderer, "softpipe"))
		return true;
	if (accel_opengles_ascii_contains(renderer, "software rasterizer"))
		return true;
	if (accel_opengles_ascii_contains(renderer, "swiftshader"))
		return true;

	/* Accept every other renderer for capability validation. */
	return false;
}

/* Initialize all EGL handles and queried limits to closed values. */
static void
accel_opengles_reset_backend(
	struct accel_opengles_backend *backend)
{
	memset(backend, 0, sizeof(*backend));
	backend->display = EGL_NO_DISPLAY;
	backend->context = EGL_NO_CONTEXT;
	backend->surface = EGL_NO_SURFACE;
	backend->config = (EGLConfig)0;
}

/* Open the strongest suitable headless EGL display available. */
static enum accel_opengles_open_status
accel_opengles_open(
	struct accel_opengles_backend *backend)
{
#if defined(EGL_PLATFORM_SURFACELESS_MESA)
	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display;
#endif
	EGLDisplay default_display;
	EGLDisplay surfaceless_display;
	enum accel_opengles_open_status default_status;
	enum accel_opengles_open_status surfaceless_status;
	bool saw_unsuitable;

	surfaceless_display = EGL_NO_DISPLAY;
	default_display = EGL_NO_DISPLAY;
	saw_unsuitable = false;

	/* Prefer the display-server-independent Mesa EGL platform. */
#if defined(EGL_PLATFORM_SURFACELESS_MESA)
	get_platform_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
		eglGetProcAddress("eglGetPlatformDisplayEXT");
	if (get_platform_display != NULL) {
		surfaceless_display = get_platform_display(
			EGL_PLATFORM_SURFACELESS_MESA,
			EGL_DEFAULT_DISPLAY,
			NULL);
	}
#endif

	/* Accept the surfaceless platform only after full capability checks. */
	if (surfaceless_display != EGL_NO_DISPLAY) {
		surfaceless_status = accel_opengles_try_display(
			backend,
			surfaceless_display);
		if (surfaceless_status == ACCEL_OPENGLES_OPEN_SUITABLE)
			return surfaceless_status;
		if (surfaceless_status == ACCEL_OPENGLES_OPEN_UNSUITABLE)
			saw_unsuitable = true;
		if (backend->allocation_failed) {
			accel_opengles_close(backend);
			return ACCEL_OPENGLES_OPEN_FAILED;
		}
		accel_opengles_close(backend);
	}

	/* Fall back to the platform's default EGL display. */
	default_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (default_display != EGL_NO_DISPLAY &&
	    default_display != surfaceless_display) {
		default_status = accel_opengles_try_display(
			backend,
			default_display);
		if (default_status == ACCEL_OPENGLES_OPEN_SUITABLE)
			return default_status;
		if (default_status == ACCEL_OPENGLES_OPEN_UNSUITABLE)
			saw_unsuitable = true;
		if (backend->allocation_failed) {
			accel_opengles_close(backend);
			return ACCEL_OPENGLES_OPEN_FAILED;
		}
		accel_opengles_close(backend);
	}

	/* Distinguish a valid CPU implementation from total EGL failure. */
	if (saw_unsuitable)
		return ACCEL_OPENGLES_OPEN_UNSUITABLE;

	/* Report that no EGL display could satisfy initialization. */
	return ACCEL_OPENGLES_OPEN_FAILED;
}

/* Create and inspect one EGL display without publishing partial state. */
static enum accel_opengles_open_status
accel_opengles_try_display(
	struct accel_opengles_backend *backend,
	EGLDisplay display)
{
	EGLint config_count;
	EGLint major;
	EGLint minor;
	bool suitable;
	static const EGLint config_attribute[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_NONE
	};
	static const EGLint context_attribute[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	static const EGLint surface_attribute[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE
	};

	backend->display = display;
	backend->context = EGL_NO_CONTEXT;
	backend->surface = EGL_NO_SURFACE;
	backend->config = (EGLConfig)0;
	backend->display_initialized = false;
	major = 0;
	minor = 0;
	config_count = 0;

	/* Initialize the display before selecting the OpenGL ES client API. */
	if (!eglInitialize(display, &major, &minor))
		return ACCEL_OPENGLES_OPEN_FAILED;
	backend->display_initialized = true;
	if (!eglBindAPI(EGL_OPENGL_ES_API))
		return ACCEL_OPENGLES_OPEN_FAILED;

	/* Select one ES 3 pbuffer configuration for headless execution. */
	if (!eglChooseConfig(
		display,
		config_attribute,
		&backend->config,
		1,
		&config_count)) {
		return ACCEL_OPENGLES_OPEN_FAILED;
	}
	if (config_count != 1)
		return ACCEL_OPENGLES_OPEN_FAILED;

	/* Create a version-3 context and a minimal offscreen surface. */
	backend->context = eglCreateContext(
		display,
		backend->config,
		EGL_NO_CONTEXT,
		context_attribute);
	if (backend->context == EGL_NO_CONTEXT)
		return ACCEL_OPENGLES_OPEN_FAILED;
	backend->surface = eglCreatePbufferSurface(
		display,
		backend->config,
		surface_attribute);
	if (backend->surface == EGL_NO_SURFACE)
		return ACCEL_OPENGLES_OPEN_FAILED;

	/* Make the context current only while querying immutable capabilities. */
	if (!eglMakeCurrent(
		display,
		backend->surface,
		backend->surface,
		backend->context)) {
		return ACCEL_OPENGLES_OPEN_FAILED;
	}
	suitable = accel_opengles_query_device(backend);
	if (!eglMakeCurrent(
		display,
		EGL_NO_SURFACE,
		EGL_NO_SURFACE,
		EGL_NO_CONTEXT)) {
		return ACCEL_OPENGLES_OPEN_FAILED;
	}

	/* Report either a hardware compute device or a valid unsuitable driver. */
	if (suitable)
		return ACCEL_OPENGLES_OPEN_SUITABLE;
	return ACCEL_OPENGLES_OPEN_UNSUITABLE;
}

/* Query renderer identity and the complete execution limit set. */
static bool
accel_opengles_query_device(
	struct accel_opengles_backend *backend)
{
	const GLubyte *renderer;
	GLint major;
	GLint minor;
	GLint max_group_size_x;
	GLint max_invocations;
	GLenum gl_error;

	major = 0;
	minor = 0;
	max_group_size_x = 0;
	max_invocations = 0;
	accel_opengles_clear_gl_errors();

	/* Query ES version, SSBO capacity, and one-dimensional compute limits. */
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	glGetIntegerv(
		GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
		&backend->max_storage_bindings);
	glGetIntegerv(
		GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,
		&backend->max_compute_storage_blocks);
	glGetIntegeri_v(
		GL_MAX_COMPUTE_WORK_GROUP_COUNT,
		0,
		&backend->max_group_count_x);
	glGetIntegeri_v(
		GL_MAX_COMPUTE_WORK_GROUP_SIZE,
		0,
		&max_group_size_x);
	glGetIntegerv(
		GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
		&max_invocations);
	glGetInteger64v(
		GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
		&backend->max_storage_block_size);
	renderer = glGetString(GL_RENDERER);
	gl_error = glGetError();

	/* Reject incomplete or erroneous capability queries. */
	if (gl_error != GL_NO_ERROR)
		return false;
	if (major < 3 ||
	    (major == 3 &&
	     minor < 1)) {
		return false;
	}
	if (backend->max_storage_bindings < ACCEL_OPENGLES_MINIMUM_BINDINGS)
		return false;
	if (backend->max_compute_storage_blocks < ACCEL_OPENGLES_MINIMUM_BINDINGS)
		return false;
	if (backend->max_group_count_x <= 0)
		return false;
	if (max_group_size_x < (GLint)ACCEL_OPENGLES_WORKGROUP_SIZE)
		return false;
	if (max_invocations < (GLint)ACCEL_OPENGLES_WORKGROUP_SIZE)
		return false;
	if (backend->max_storage_block_size < (GLint64)sizeof(uint32_t))
		return false;
	if (accel_opengles_is_software_renderer((const char *)renderer))
		return false;

	/* Deep-copy the renderer identity before releasing the context. */
	backend->renderer = accel_opengles_duplicate((const char *)renderer);
	if (backend->renderer == NULL) {
		backend->allocation_failed = true;
		return false;
	}

	/* Report a complete suitable device record. */
	return true;
}

/* Close one partial or complete EGL backend in reverse order. */
static void
accel_opengles_close(
	struct accel_opengles_backend *backend)
{
	/* Ignore an absent optional backend. */
	if (backend == NULL)
		return;

	/* Release any context ownership held by the calling thread. */
	if (backend->display != EGL_NO_DISPLAY &&
	    backend->display_initialized) {
		(void)eglMakeCurrent(
			backend->display,
			EGL_NO_SURFACE,
			EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
	}

	/* Destroy the surface and context before terminating their display. */
	if (backend->display != EGL_NO_DISPLAY &&
	    backend->surface != EGL_NO_SURFACE) {
		(void)eglDestroySurface(backend->display, backend->surface);
	}
	if (backend->display != EGL_NO_DISPLAY &&
	    backend->context != EGL_NO_CONTEXT) {
		(void)eglDestroyContext(backend->display, backend->context);
	}
	if (backend->display != EGL_NO_DISPLAY &&
	    backend->display_initialized) {
		(void)eglTerminate(backend->display);
	}

	/* Release the renderer name and return every handle to a closed value. */
	noct_free(backend->renderer);
	backend->renderer = NULL;
	backend->display = EGL_NO_DISPLAY;
	backend->context = EGL_NO_CONTEXT;
	backend->surface = EGL_NO_SURFACE;
	backend->config = (EGLConfig)0;
	backend->display_initialized = false;
}

/* Acquire exclusive ownership and make the EGL context current. */
static bool
accel_opengles_acquire(
	struct accel_opengles_backend *backend,
	char *error,
	size_t error_size)
{
	accel_mutex_lock(&backend->mutex);

	/* Bind the serialized context to the current worker thread. */
	if (!eglMakeCurrent(
		backend->display,
		backend->surface,
		backend->surface,
		backend->context)) {
		accel_mutex_unlock(&backend->mutex);
		return accel_opengles_set_error(error, error_size, N_TR("Failed to make the OpenGL ES context current."));
	}

	/* Remove stale driver errors before executing one backend operation. */
	accel_opengles_clear_gl_errors();

	/* Report exclusive current-context ownership. */
	return true;
}

/* Release current-context ownership and the backend mutex. */
static bool
accel_opengles_release(
	struct accel_opengles_backend *backend,
	char *error,
	size_t error_size)
{
	bool released;

	/* Detach the context so a later callback may run on another thread. */
	released = eglMakeCurrent(
		backend->display,
		EGL_NO_SURFACE,
		EGL_NO_SURFACE,
		EGL_NO_CONTEXT) == EGL_TRUE;

	/* Drain work while the failed detach still leaves this context current. */
	if (!released)
		glFinish();
	accel_mutex_unlock(&backend->mutex);

	/* Diagnose a failed ownership transfer after unlocking the backend. */
	if (!released)
		return accel_opengles_set_error(error, error_size, N_TR("Failed to release the OpenGL ES context."));

	/* Report successful context release. */
	return true;
}

/* Drain every stale OpenGL ES error from the private context. */
static void
accel_opengles_clear_gl_errors(void)
{
	/* Consume the bounded sticky error flags before a checked operation. */
	while (glGetError() != GL_NO_ERROR)
		;
}

/* Check one OpenGL ES operation and publish a stable diagnostic. */
static bool
accel_opengles_check_gl(
	char *error,
	size_t error_size,
	const char *message)
{
	GLenum gl_error;

	gl_error = glGetError();

	/* Preserve the first error as a backend callback failure. */
	if (gl_error != GL_NO_ERROR)
		return accel_opengles_set_error(error, error_size, message);

	/* Report an error-free OpenGL ES operation. */
	return true;
}

/* Prepare every immutable OpenGL ES program for one accelerator region. */
static enum accel_compile_status
accel_opengles_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	struct accel_opengles_backend *backend;
	struct accel_opengles_prepared *prepared;
	char validation_error[128];
	char context_error[128];
	uint32_t binding_count;
	uint32_t i;
	bool prepared_all;
	bool released;
	struct accel_prepared_program cleanup_program;

	/* Clear the opaque publication slot before validation. */
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;
	result->payload = NULL;
	backend = backend_state;

	/* Reject an incomplete compiler-to-backend request. */
	if (backend == NULL || program == NULL) {
		hir_error(0, N_TR("Invalid OpenGL ES program preparation request."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Validate the complete target-neutral plan before device inspection. */
	if (!accel_program_validate(
		program,
		validation_error,
		sizeof(validation_error))) {
		hir_error(program->source_line, N_TR("Invalid accelerator program reached the OpenGL ES backend."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Decline Float32 until strict Noct controls are guaranteed by GLES. */
	if (accel_opengles_program_uses_f32(program))
		return ACCEL_COMPILE_DECLINED;

	/* Apply per-stage resource limits before allocating backend objects. */
	binding_count = program->buffer_count + 1;
	if (program->scalar_result_count != 0)
		binding_count++;
	if (binding_count > (uint32_t)backend->max_storage_bindings)
		return ACCEL_COMPILE_DECLINED;
	if (binding_count > (uint32_t)backend->max_compute_storage_blocks)
		return ACCEL_COMPILE_DECLINED;

	/* Allocate the deep-owned prepared-program wrapper. */
	prepared = noct_calloc(1, sizeof(*prepared));
	if (prepared == NULL) {
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}
	prepared->kernel_count = program->kernel_count;

	/* Allocate one immutable GL program slot per typed kernel. */
	if (prepared->kernel_count != 0) {
		prepared->kernel = noct_calloc(
			prepared->kernel_count,
			sizeof(*prepared->kernel));
		if (prepared->kernel == NULL) {
			noct_free(prepared);
			hir_out_of_memory();
			return ACCEL_COMPILE_ERROR;
		}
	}

	/* Clone all backend-neutral metadata before opening GL ownership. */
	prepared->program = accel_program_clone(program);
	if (prepared->program == NULL) {
		noct_free(prepared->kernel);
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Acquire the private context for the complete preparation transaction. */
	context_error[0] = '\0';
	if (!accel_opengles_acquire(
		backend,
		context_error,
		sizeof(context_error))) {
		accel_program_destroy(prepared->program);
		noct_free(prepared->kernel);
		noct_free(prepared);
		hir_error(program->source_line, N_TR("Failed to acquire the OpenGL ES compiler context."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Compile and link every kernel before publishing any payload. */
	prepared_all = true;
	for (i = 0; i < prepared->kernel_count; i++) {
		if (!accel_opengles_prepare_kernel_current(
			program,
			i,
			&prepared->kernel[i])) {
			prepared_all = false;
			break;
		}
	}

	/* Roll back every GL object while the transaction still owns context. */
	if (!prepared_all)
		accel_opengles_destroy_prepared_current(prepared);

	/* Release thread ownership after all GL creation or rollback is complete. */
	released = accel_opengles_release(
		backend,
		context_error,
		sizeof(context_error));
	if (!prepared_all || !released) {
		if (prepared_all) {
			cleanup_program.payload = prepared;
			accel_opengles_destroy_prepared_program(
				backend,
				&cleanup_program);
		} else {
			accel_program_destroy(prepared->program);
			noct_free(prepared->kernel);
			noct_free(prepared);
		}
		if (!released)
			hir_error(program->source_line, N_TR("Failed to release the OpenGL ES compiler context."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Publish the complete immutable backend payload. */
	result->payload = prepared;

	/* Report transactional preparation success. */
	return ACCEL_COMPILE_APPLIED;
}

/* Detect every Float32 use that requires unavailable strict controls. */
static bool
accel_opengles_program_uses_f32(
	const struct accel_program *program)
{
	const struct accel_ir_kernel *kernel;
	uint32_t i;
	uint32_t j;

	/* Inspect all uniform scalar declarations. */
	for (i = 0; i < program->scalar_count; i++) {
		if (program->scalar[i].value_type == ACCEL_IR_F32)
			return true;
	}

	/* Inspect every buffer and instruction type in every kernel. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = program->kernel[i].ir;
		for (j = 0; j < kernel->buffer_binding_count; j++) {
			if (kernel->buffer_value_type[j] == ACCEL_IR_F32)
				return true;
		}
		for (j = 0; j < kernel->instruction_count; j++) {
			if (kernel->instruction[j].result_type == ACCEL_IR_F32)
				return true;
		}
	}

	/* Report an integer-only program. */
	return false;
}

/* Generate, compile, and link one immutable compute entry point. */
static bool
accel_opengles_prepare_kernel_current(
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_opengles_kernel *result)
{
	struct accel_shader_source source;
	const GLchar *source_pointer;
	char source_error[128];
	GLuint shader;
	GLuint linked_program;
	GLint compiled;
	GLint linked;
	GLint source_length;
	bool generated;

	memset(&source, 0, sizeof(source));
	result->program = 0;
	shader = 0;
	linked_program = 0;
	compiled = GL_FALSE;
	linked = GL_FALSE;

	/* Generate deterministic GLSL ES 3.10 from the typed kernel. */
	generated = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_GLSL_ES_310,
		program,
		kernel_index,
		&source,
		source_error,
		sizeof(source_error));
	if (!generated) {
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to generate an OpenGL ES compute shader."));
		return false;
	}

	/* Reject source that cannot be represented by the GLES length API. */
	source_length = (GLint)source.length;
	if (source_length < 0 || (size_t)source_length != source.length) {
		accel_shader_source_cleanup(&source);
		hir_error(program->kernel[kernel_index].source_line, N_TR("OpenGL ES compute shader source is too large."));
		return false;
	}

	/* Compile the one generated compute shader. */
	shader = glCreateShader(GL_COMPUTE_SHADER);
	if (shader == 0) {
		accel_shader_source_cleanup(&source);
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to create an OpenGL ES compute shader."));
		return false;
	}
	source_pointer = (const GLchar *)source.data;
	glShaderSource(shader, 1, &source_pointer, &source_length);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	accel_shader_source_cleanup(&source);
	if (compiled != GL_TRUE) {
		accel_opengles_report_shader_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to compile an OpenGL ES compute shader."));
		glDeleteShader(shader);
		return false;
	}

	/* Link the shader into one immutable compute program. */
	linked_program = glCreateProgram();
	if (linked_program == 0) {
		glDeleteShader(shader);
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to create an OpenGL ES compute program."));
		return false;
	}
	glAttachShader(linked_program, shader);
	glLinkProgram(linked_program);
	glGetProgramiv(linked_program, GL_LINK_STATUS, &linked);
	glDetachShader(linked_program, shader);
	glDeleteShader(shader);
	if (linked != GL_TRUE) {
		accel_opengles_report_shader_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to link an OpenGL ES compute program."));
		glDeleteProgram(linked_program);
		return false;
	}

	/* Reject any driver error not represented by compile/link status. */
	if (glGetError() != GL_NO_ERROR) {
		glDeleteProgram(linked_program);
		hir_error(program->kernel[kernel_index].source_line, N_TR("OpenGL ES reported an error while preparing a compute program."));
		return false;
	}

	/* Publish the complete linked program. */
	result->program = linked_program;

	/* Report successful kernel preparation. */
	return true;
}

/* Publish one bounded shader compiler or linker diagnostic. */
static void
accel_opengles_report_shader_error(
	int source_line,
	const char *message)
{
	/* Publish the stable backend diagnostic through the HIR error channel. */
	hir_error(source_line, message);
}

/* Delete every linked program while its EGL context is current. */
static void
accel_opengles_destroy_prepared_current(
	struct accel_opengles_prepared *prepared)
{
	uint32_t i;

	/* Accept cleanup of an optional or partially built payload. */
	if (prepared == NULL)
		return;

	/* Delete all linked programs in reverse ownership order. */
	if (prepared->kernel != NULL) {
		for (i = 0; i < prepared->kernel_count; i++) {
			if (prepared->kernel[i].program != 0) {
				glDeleteProgram(prepared->kernel[i].program);
				prepared->kernel[i].program = 0;
			}
		}
	}
}

/* Destroy one backend-prepared program and clear its opaque slot. */
static void
accel_opengles_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct accel_opengles_backend *backend;
	struct accel_opengles_prepared *prepared;
	char error[128];
	bool acquired;

	/* Accept cleanup of an absent or already-cleared publication slot. */
	if (program == NULL || program->payload == NULL)
		return;

	backend = backend_state;
	prepared = program->payload;
	error[0] = '\0';
	acquired = false;

	/* Delete GL programs only while the serialized context is current. */
	if (backend != NULL && backend->mutex.initialized) {
		acquired = accel_opengles_acquire(
			backend,
			error,
			sizeof(error));
		if (acquired) {
			accel_opengles_destroy_prepared_current(prepared);
			(void)accel_opengles_release(
				backend,
				error,
				sizeof(error));
		}
	}

	/* Release all CPU metadata regardless of a lost GL context. */
	accel_program_destroy(prepared->program);
	noct_free(prepared->kernel);
	noct_free(prepared);
	program->payload = NULL;
}

/* Register the shared private accelerator runtime with GLES callbacks. */
static bool
accel_opengles_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	bool success;

	/* Copy the target-specific executor table into VM-owned metadata. */
	success = accel_runtime_register(
		context,
		env,
		accel_opengles_executor_ops());

	/* Report the shared package publication result. */
	return success;
}

/* Destroy the serialized EGL context after all programs and sessions. */
static void
accel_opengles_destroy_backend_state(
	void *backend_state)
{
	struct accel_opengles_backend *backend;

	backend = backend_state;

	/* Accept cleanup of an absent backend. */
	if (backend == NULL)
		return;

	/* Exclude context callbacks while closing EGL objects. */
	if (backend->mutex.initialized)
		accel_mutex_lock(&backend->mutex);
	accel_opengles_close(backend);
	if (backend->mutex.initialized)
		accel_mutex_unlock(&backend->mutex);

	/* Release serialization state and the backend wrapper. */
	accel_mutex_destroy(&backend->mutex);
	noct_free(backend);
}

/* Borrow target-neutral metadata from one prepared GLES payload. */
static const struct accel_program *
accel_opengles_get_program(
	const struct accel_prepared_program *prepared)
{
	const struct accel_opengles_prepared *payload;

	/* Reject an absent opaque publication slot. */
	if (prepared == NULL || prepared->payload == NULL)
		return NULL;

	payload = prepared->payload;

	/* Return the immutable deep-owned program plan. */
	return payload->program;
}

/* Validate one checked dispatch against immutable GLES device limits. */
static bool
accel_opengles_validate_dispatch_limit(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	const struct accel_opengles_prepared *payload;
	struct accel_opengles_backend *backend;
	uint32_t group_count;

	UNUSED_PARAMETER(start);

	backend = backend_state;

	/* Validate backend and program ownership before applying limits. */
	if (backend == NULL ||
	    prepared == NULL ||
	    prepared->payload == NULL) {
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES dispatch limit request."));
	}
	payload = prepared->payload;
	if (kernel_index >= payload->kernel_count)
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES kernel index."));

	/* Empty source loops require no device dispatch. */
	if (trip == 0)
		return true;

	/* Round the source iteration count to fixed 64-lane workgroups. */
	group_count = trip / ACCEL_OPENGLES_WORKGROUP_SIZE;
	if (trip % ACCEL_OPENGLES_WORKGROUP_SIZE != 0)
		group_count++;

	/* Decline a dispatch larger than the selected device can represent. */
	if (group_count > (uint32_t)backend->max_group_count_x)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES dispatch exceeds the device workgroup-count limit."));

	/* Report a representable one-dimensional dispatch. */
	return true;
}

/* Count nonempty dispatch ranges in one validated scalar block. */
static uint32_t
accel_opengles_count_active_dispatches(
	const struct accel_program *program,
	const uint32_t scalar_word[])
{
	uint32_t active_count;
	uint32_t range_word;
	uint32_t i;

	active_count = 0;

	/* Counts each nonzero trip word in immutable source order. */
	for (i = 0; i < program->kernel_count; i++) {
		range_word = program->scalar_count + i * 2;

		/* Records only ranges that require a device dispatch. */
		if (scalar_word[range_word + 1] != 0)
			active_count++;
	}

	/* Return the exact number of dispatch callbacks expected at finish. */
	return active_count;
}

/* Create all SSBO resources from plain runtime-owned snapshots. */
static bool
accel_opengles_create_execution(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	void **execution,
	char *error,
	size_t error_size)
{
	struct accel_opengles_backend *backend;
	const struct accel_opengles_prepared *payload;
	struct accel_opengles_execution *created;
	const void *upload_data;
	size_t allocation_byte_count;
	size_t scalar_byte_count;
	uint32_t i;
	bool created_all;
	bool released;

	/* Clear the opaque output before validating runtime snapshots. */
	if (execution != NULL)
		*execution = NULL;
	if (execution == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES execution output."));

	backend = backend_state;

	/* Recover immutable program metadata from the published payload. */
	if (prepared == NULL || prepared->payload == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES prepared program."));
	payload = prepared->payload;

	/* Validate all plain input arrays before allocating device objects. */
	if (!accel_opengles_validate_execution_inputs(
		backend,
		payload,
		scalar_word_count,
		scalar_word,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Allocate one execution wrapper and its GL buffer metadata. */
	created = noct_calloc(1, sizeof(*created));
	if (created == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Out of memory while creating an OpenGL ES execution."));
	created->backend = backend;
	created->prepared = payload;
	created->buffer_count = buffer_count;
	created->scalar_word_count = scalar_word_count;
	created->result_word_count = result_word_count;
	created->expected_dispatch_count =
		accel_opengles_count_active_dispatches(
			payload->program,
			scalar_word);
	created->has_active_dispatch =
		created->expected_dispatch_count != 0;

	/* Deep-copy scalar metadata used to verify active dispatch callbacks. */
	if (created->has_active_dispatch && scalar_word_count != 0) {
		created->scalar_word = noct_malloc(
			(size_t)scalar_word_count *
			sizeof(*created->scalar_word));
		if (created->scalar_word == NULL) {
			noct_free(created);
			return accel_opengles_set_error(error, error_size, N_TR("Out of memory while copying OpenGL ES scalar metadata."));
		}
		memcpy(
			created->scalar_word,
			scalar_word,
			(size_t)scalar_word_count *
			sizeof(*created->scalar_word));
	}

	/* Allocate immutable metadata storage for every data binding. */
	if (buffer_count != 0) {
		created->buffer = noct_calloc(
			buffer_count,
			sizeof(*created->buffer));
		if (created->buffer == NULL) {
			accel_opengles_free_execution(created);
			return accel_opengles_set_error(error, error_size, N_TR("Out of memory while creating OpenGL ES buffers."));
		}
	}

	/* Snapshot the immutable metadata for every declared data binding. */
	for (i = 0; i < buffer_count; i++) {
		created->buffer[i].origin = buffer[i].origin;
		created->buffer[i].args_slot = buffer[i].args_slot;
		created->buffer[i].element_kind = buffer[i].element_kind;
		created->buffer[i].element_width = buffer[i].element_width;
		created->buffer[i].element_count = buffer[i].element_count;
		created->buffer[i].byte_count = buffer[i].byte_count;
		created->buffer[i].active = buffer[i].active;
		created->buffer[i].upload = buffer[i].upload;
		created->buffer[i].download = buffer[i].download;
	}

	/* Publish an all-empty execution without creating any GPU resource. */
	if (!created->has_active_dispatch) {
		*execution = created;
		return true;
	}

	/* Acquire the context for the complete resource-creation transaction. */
	if (!accel_opengles_acquire(backend, error, error_size)) {
		accel_opengles_free_execution(created);
		return false;
	}

	/* Create each data SSBO or one legal dummy for an inactive declaration. */
	created_all = true;
	for (i = 0; i < buffer_count; i++) {
		allocation_byte_count = buffer[i].active ?
			buffer[i].byte_count : 0;
		upload_data = buffer[i].active && buffer[i].upload ?
			buffer[i].snapshot : NULL;
		if (!accel_opengles_create_buffer_current(
			backend,
			allocation_byte_count,
			upload_data,
			&created->buffer[i].name,
			error,
			error_size)) {
			created_all = false;
			break;
		}
	}

	/* Create the immutable scalar and dispatch-range SSBO last. */
	scalar_byte_count = (size_t)scalar_word_count * sizeof(*scalar_word);
	if (created_all) {
		created_all = accel_opengles_create_buffer_current(
			backend,
			scalar_byte_count,
			scalar_word,
			&created->scalar_buffer,
			error,
			error_size);
	}

	/* Create the mutable scalar-result SSBO only when the ABI declares it. */
	if (created_all && result_word_count != 0) {
		created_all = accel_opengles_create_buffer_current(
			backend,
			(size_t)result_word_count * sizeof(*result_word),
			result_word,
			&created->result_buffer,
			error,
			error_size);
	}

	/* Roll back partial GL ownership before releasing the context. */
	if (!created_all)
		accel_opengles_destroy_execution_current(created);

	/* Release the context after all resources are complete or destroyed. */
	released = accel_opengles_release(backend, error, error_size);
	if (!created_all || !released) {
		if (created_all)
			accel_opengles_destroy_execution(created);
		else
			accel_opengles_free_execution(created);
		return false;
	}

	/* Publish the plain backend execution to the shared runtime. */
	*execution = created;

	/* Report successful resource creation. */
	return true;
}

/* Validate execution arrays, exact ABI word counts, and SSBO sizes. */
static bool
accel_opengles_validate_execution_inputs(
	struct accel_opengles_backend *backend,
	const struct accel_opengles_prepared *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *binding;
	size_t expected_scalar_count;
	size_t result_byte_count;
	uint32_t i;
	bool has_active_dispatch;

	/* Require the selected backend and immutable target-neutral plan. */
	if (backend == NULL ||
	    prepared == NULL ||
	    prepared->program == NULL) {
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES execution request."));
	}

	/* Match every data buffer to the compiled binding namespace. */
	if (buffer_count != prepared->program->buffer_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES buffer table does not match the prepared program."));
	if (buffer_count != 0 && buffer == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES buffer descriptors."));

	/* Match scalar values followed by two range words per kernel. */
	expected_scalar_count = prepared->program->scalar_count;
	expected_scalar_count += (size_t)prepared->program->kernel_count * 2;
	if ((size_t)scalar_word_count != expected_scalar_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES scalar block does not match the prepared program."));
	if (scalar_word_count != 0 && scalar_word == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES scalar words."));

	/* Match the optional result block and preserve the zero-count ABI. */
	if (result_word_count != prepared->program->scalar_result_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES result block does not match the prepared program."));
	if (result_word_count == 0 && result_word != NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Unexpected OpenGL ES result words."));
	if (result_word_count != 0 && result_word == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES result words."));
	has_active_dispatch =
		accel_opengles_count_active_dispatches(
			prepared->program,
			scalar_word) != 0;

	/* Validate all raw-word snapshots and only active device allocations. */
	for (i = 0; i < buffer_count; i++) {
		binding = &prepared->program->buffer[i];

		/* Match immutable binding identity to the prepared program. */
		if (buffer[i].origin != binding->origin ||
		    buffer[i].args_slot != binding->args_slot ||
		    buffer[i].element_kind != binding->element_kind ||
		    buffer[i].element_width != binding->element_width) {
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES buffer metadata does not match the prepared program."));
		}

		/* Validate the runtime extent and dynamic transfer plan. */
		if (buffer[i].element_width != sizeof(uint32_t))
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES supports only 32-bit accelerator buffers."));
		if (buffer[i].element_count >
		    (size_t)-1 / buffer[i].element_width ||
		    buffer[i].element_count * buffer[i].element_width !=
		    buffer[i].byte_count) {
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES buffer extent metadata is inconsistent."));
		}
		if (!buffer[i].active &&
		    (buffer[i].upload || buffer[i].download)) {
			return accel_opengles_set_error(error, error_size, N_TR("Inactive OpenGL ES buffers cannot request host transfers."));
		}
		if (buffer[i].active &&
		    buffer[i].byte_count >
		    (size_t)backend->max_storage_block_size) {
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES buffer exceeds the device storage-block limit."));
		}
		if (buffer[i].upload &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES upload snapshot."));
		}
		if (buffer[i].download &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES download snapshot."));
		}
	}

	/* Validate the scalar SSBO against the same device block-size limit. */
	if (expected_scalar_count > (size_t)-1 / sizeof(uint32_t))
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES scalar block size overflowed."));
	if (has_active_dispatch &&
	    expected_scalar_count * sizeof(uint32_t) >
	    (size_t)backend->max_storage_block_size) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES scalar block exceeds the device storage-block limit."));
	}

	/* Validate the optional result SSBO against the same block-size limit. */
	result_byte_count = (size_t)result_word_count * sizeof(uint32_t);
	if (result_word_count != 0 &&
	    result_byte_count / sizeof(uint32_t) != result_word_count) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES result block size overflowed."));
	}
	if (has_active_dispatch &&
	    result_byte_count >
	    (size_t)backend->max_storage_block_size) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES result block exceeds the device storage-block limit."));
	}

	/* Report a complete and representable execution snapshot. */
	return true;
}

/* Allocate one raw-word SSBO while the private context is current. */
static bool
accel_opengles_create_buffer_current(
	struct accel_opengles_backend *backend,
	size_t byte_count,
	const void *data,
	GLuint *result,
	char *error,
	size_t error_size)
{
	uint32_t zero;
	size_t allocation_size;
	GLsizeiptr gl_size;

	UNUSED_PARAMETER(backend);

	*result = 0;
	zero = 0;
	allocation_size = byte_count;

	/* Satisfy globally declared but unused shader bindings with one raw word. */
	if (allocation_size == 0) {
		allocation_size = sizeof(zero);
		data = &zero;
	}

	/* Reject a size that the platform GLsizeiptr cannot represent. */
	gl_size = (GLsizeiptr)allocation_size;
	if (gl_size < 0 || (size_t)gl_size != allocation_size)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES buffer size is not representable."));

	/* Allocate and initialize one complete storage buffer. */
	glGenBuffers(1, result);
	if (*result == 0)
		return accel_opengles_set_error(error, error_size, N_TR("Failed to allocate an OpenGL ES buffer name."));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, *result);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		gl_size,
		data,
		GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	/* Roll back an allocation rejected by the driver. */
	if (!accel_opengles_check_gl(error, error_size, "Failed to allocate OpenGL ES storage-buffer memory.")) {
		glDeleteBuffers(1, result);
		*result = 0;
		return false;
	}

	/* Report a complete buffer allocation. */
	return true;
}

/* Delete every GL buffer owned by one current execution. */
static void
accel_opengles_destroy_execution_current(
	struct accel_opengles_execution *execution)
{
	uint32_t i;

	/* Accept cleanup of an optional or partially built execution. */
	if (execution == NULL)
		return;

	/* Delete scalar resources followed by every data SSBO. */
	if (execution->result_buffer != 0) {
		glDeleteBuffers(1, &execution->result_buffer);
		execution->result_buffer = 0;
	}
	if (execution->scalar_buffer != 0) {
		glDeleteBuffers(1, &execution->scalar_buffer);
		execution->scalar_buffer = 0;
	}
	if (execution->buffer != NULL) {
		for (i = 0; i < execution->buffer_count; i++) {
			if (execution->buffer[i].name != 0) {
				glDeleteBuffers(1, &execution->buffer[i].name);
				execution->buffer[i].name = 0;
			}
		}
	}
}

/* Free the plain metadata owned by one detached GLES execution. */
static void
accel_opengles_free_execution(
	struct accel_opengles_execution *execution)
{
	/* Accept cleanup of an optional or partially built execution. */
	if (execution == NULL)
		return;

	/* Release only backend-owned host allocations. */
	noct_free(execution->scalar_word);
	noct_free(execution->buffer);
	noct_free(execution);
}

/* Dispatch one active kernel in source order with an SSBO barrier. */
static bool
accel_opengles_dispatch_execution(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct accel_opengles_execution *active;
	const struct accel_program *program;
	uint32_t group_count;
	uint32_t range_word;
	bool commands_issued;
	bool dispatched;
	bool released;

	active = execution;

	/* Validate the shared runtime's published execution and kernel index. */
	if (active == NULL || active->prepared == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES execution."));
	if (active->finished)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES execution was already finished."));
	if (!active->has_active_dispatch)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES execution has no active dispatch."));
	if (kernel_index >= active->prepared->kernel_count)
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES kernel index."));
	if (trip == 0)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES received an empty active dispatch."));
	if (active->dispatched && kernel_index <= active->last_kernel)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES kernels were dispatched out of order."));
	if (active->dispatch_count >= active->expected_dispatch_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES received too many active dispatches."));

	program = active->prepared->program;
	range_word = program->scalar_count + kernel_index * 2;

	/* Validate the duplicated callback range against the immutable SSBO. */
	if (active->scalar_word == NULL ||
	    range_word + 1 >= active->scalar_word_count) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES dispatch range is missing from the scalar block."));
	}
	if (active->scalar_word[range_word] != start ||
	    active->scalar_word[range_word + 1] != trip) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES dispatch range changed after execution creation."));
	}

	/* Round the checked active trip count to 64-lane workgroups. */
	group_count = trip / ACCEL_OPENGLES_WORKGROUP_SIZE;
	if (trip % ACCEL_OPENGLES_WORKGROUP_SIZE != 0)
		group_count++;

	/* Acquire context ownership and bind this execution's complete resources. */
	if (!accel_opengles_acquire(active->backend, error, error_size))
		return false;
	commands_issued = false;
	dispatched = accel_opengles_bind_execution_current(
		active,
		error,
		error_size);

	/* Submit the selected compute program and order subsequent SSBO access. */
	if (dispatched) {
		glUseProgram(active->prepared->kernel[kernel_index].program);
		glDispatchCompute(group_count, 1, 1);
		commands_issued = true;
		glMemoryBarrier(
			GL_SHADER_STORAGE_BARRIER_BIT |
			GL_BUFFER_UPDATE_BARRIER_BIT);
		glFlush();
		glUseProgram(0);
		dispatched = accel_opengles_check_gl(
			error,
			error_size,
			"OpenGL ES compute dispatch failed.");
	}

	/* Drain any submitted commands before reporting a failed dispatch. */
	if (!dispatched && commands_issued)
		glFinish();

	/* Release context ownership after commands and barriers are recorded. */
	released = accel_opengles_release(
		active->backend,
		error,
		error_size);
	if (!dispatched || !released)
		return false;

	/* Remember the increasing backend dispatch order. */
	active->last_kernel = kernel_index;
	active->dispatch_count++;
	active->dispatched = true;

	/* Report a complete ordered dispatch. */
	return true;
}

/* Bind all data and scalar SSBOs for one current execution. */
static bool
accel_opengles_bind_execution_current(
	struct accel_opengles_execution *execution,
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Bind every declared data block in deterministic program order. */
	for (i = 0; i < execution->buffer_count; i++) {
		glBindBufferBase(
			GL_SHADER_STORAGE_BUFFER,
			i,
			execution->buffer[i].name);
	}

	/* Bind immutable scalar words immediately after all data buffers. */
	glBindBufferBase(
		GL_SHADER_STORAGE_BUFFER,
		execution->buffer_count,
		execution->scalar_buffer);

	/* Bind mutable scalar results only when the program declared them. */
	if (execution->result_word_count != 0) {
		glBindBufferBase(
			GL_SHADER_STORAGE_BUFFER,
			execution->buffer_count + 1,
			execution->result_buffer);
	}

	/* Report any binding rejected by the selected context. */
	return accel_opengles_check_gl(
		error,
		error_size,
		"Failed to bind OpenGL ES storage buffers.");
}

/* Synchronize all commands and copy downloads into plain snapshots. */
static bool
accel_opengles_finish_execution(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_opengles_execution *active;
	uint32_t i;
	bool copied;
	bool released;

	active = execution;

	/* Validate all mutable output descriptors before synchronizing GL. */
	if (!accel_opengles_validate_finish(
		active,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Complete an all-empty execution without touching the GL context. */
	if (!active->has_active_dispatch) {
		active->finished = true;
		return true;
	}

	/* Acquire the private context and complete every queued dispatch. */
	if (!accel_opengles_acquire(active->backend, error, error_size))
		return false;
	glMemoryBarrier(
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_BUFFER_UPDATE_BARRIER_BIT);
	glFinish();
	copied = accel_opengles_check_gl(
		error,
		error_size,
		"Failed to finish OpenGL ES compute commands.");

	/* Map each requested output only after synchronous device completion. */
	for (i = 0; copied && i < buffer_count; i++) {
		if (!buffer[i].download)
			continue;
		copied = accel_opengles_read_buffer_current(
			&active->buffer[i],
			&buffer[i],
			error,
			error_size);
	}

	/* Reads the optional result block after the same completion boundary. */
	if (copied && result_word_count != 0) {
		copied = accel_opengles_read_result_current(
			active,
			result_word_count,
			result_word,
			error,
			error_size);
	}

	/* Leave no generic target binding behind before releasing the context. */
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	if (copied) {
		copied = accel_opengles_check_gl(
			error,
			error_size,
			"Failed to complete OpenGL ES output copies.");
	}

	/* Release ownership only after every output snapshot is complete. */
	released = accel_opengles_release(
		active->backend,
		error,
		error_size);
	if (!copied || !released)
		return false;
	active->finished = true;

	/* Report synchronous completion and readback. */
	return true;
}

/* Validate output snapshots against immutable execution metadata. */
static bool
accel_opengles_validate_finish(
	const struct accel_opengles_execution *execution,
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Require the complete execution and exact buffer table. */
	if (execution == NULL || execution->backend == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Invalid OpenGL ES finish request."));
	if (execution->finished)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES execution was already finished."));
	if (buffer_count != execution->buffer_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES finish buffer table changed."));
	if (buffer_count != 0 && buffer == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES finish buffers."));
	if (result_word_count != execution->result_word_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES finish result table changed."));
	if (result_word_count == 0 && result_word != NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Unexpected OpenGL ES finish result words."));
	if (result_word_count != 0 && result_word == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES finish result words."));
	if (execution->has_active_dispatch &&
	    execution->dispatch_count != execution->expected_dispatch_count) {
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES active dispatch sequence is incomplete."));
	}
	if (execution->has_active_dispatch &&
	    result_word_count != 0 &&
	    execution->result_buffer == 0) {
		return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES result buffer."));
	}

	/* Match every output extent and snapshot to its created SSBO. */
	for (i = 0; i < buffer_count; i++) {
		if (buffer[i].origin != execution->buffer[i].origin)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer origin changed."));
		if (buffer[i].args_slot != execution->buffer[i].args_slot)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer slot changed."));
		if (buffer[i].element_kind != execution->buffer[i].element_kind)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer element type changed."));
		if (buffer[i].element_width != execution->buffer[i].element_width)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer element width changed."));
		if (buffer[i].element_count != execution->buffer[i].element_count)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer extent changed."));
		if (buffer[i].byte_count != execution->buffer[i].byte_count)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer size changed."));
		if (buffer[i].active != execution->buffer[i].active)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer activity changed."));
		if (buffer[i].upload != execution->buffer[i].upload)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output upload plan changed."));
		if (buffer[i].download != execution->buffer[i].download)
			return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output transfer plan changed."));
		if (buffer[i].download &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_opengles_set_error(error, error_size, N_TR("Missing OpenGL ES output snapshot."));
		}
	}

	/* Report a stable output publication boundary. */
	return true;
}

/* Map one completed SSBO and fill its runtime-owned plain snapshot. */
static bool
accel_opengles_read_buffer_current(
	const struct accel_opengles_buffer *source,
	struct accel_runtime_buffer *destination,
	char *error,
	size_t error_size)
{
	void *mapped;
	GLsizeiptr gl_size;
	GLboolean unmapped;

	/* Empty logical buffers have no host bytes to publish. */
	if (destination->byte_count == 0)
		return true;

	/* Convert the already-validated allocation extent for the map API. */
	gl_size = (GLsizeiptr)destination->byte_count;
	if (gl_size < 0 || (size_t)gl_size != destination->byte_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output size is not representable."));

	/* Map, copy, and unmap the complete raw-word output synchronously. */
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, source->name);
	mapped = glMapBufferRange(
		GL_SHADER_STORAGE_BUFFER,
		0,
		gl_size,
		GL_MAP_READ_BIT);
	if (mapped == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Failed to map an OpenGL ES output buffer."));
	memcpy(destination->snapshot, mapped, destination->byte_count);
	unmapped = glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	if (unmapped != GL_TRUE)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES output buffer became invalid while unmapping."));

	/* Report a complete plain readback snapshot. */
	return accel_opengles_check_gl(
		error,
		error_size,
		"Failed to read an OpenGL ES output buffer.");
}

/* Map the completed result SSBO into runtime-owned raw words. */
static bool
accel_opengles_read_result_current(
	const struct accel_opengles_execution *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	char *error,
	size_t error_size)
{
	void *mapped;
	size_t byte_count;
	GLsizeiptr gl_size;
	GLboolean unmapped;

	/* Converts the already-validated logical result extent for GL. */
	byte_count = (size_t)result_word_count * sizeof(*result_word);
	gl_size = (GLsizeiptr)byte_count;
	if (gl_size <= 0 || (size_t)gl_size != byte_count)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES result size is not representable."));

	/* Maps, copies, and unmaps every completed result word synchronously. */
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, execution->result_buffer);
	mapped = glMapBufferRange(
		GL_SHADER_STORAGE_BUFFER,
		0,
		gl_size,
		GL_MAP_READ_BIT);
	if (mapped == NULL)
		return accel_opengles_set_error(error, error_size, N_TR("Failed to map the OpenGL ES result buffer."));
	memcpy(result_word, mapped, byte_count);
	unmapped = glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	if (unmapped != GL_TRUE)
		return accel_opengles_set_error(error, error_size, N_TR("OpenGL ES result buffer became invalid while unmapping."));

	return accel_opengles_check_gl(
		error,
		error_size,
		"Failed to read the OpenGL ES result buffer.");
}

/* Destroy one execution without retaining runtime snapshot pointers. */
static void
accel_opengles_destroy_execution(
	void *execution)
{
	struct accel_opengles_execution *active;
	char error[128];
	bool acquired;

	active = execution;

	/* Accept cleanup of an absent execution. */
	if (active == NULL)
		return;

	/* Delete created GL resources while the backend context remains alive. */
	error[0] = '\0';
	acquired = false;
	if (active->has_active_dispatch) {
		acquired = accel_opengles_acquire(
			active->backend,
			error,
			sizeof(error));
	}
	if (acquired) {
		accel_opengles_destroy_execution_current(active);
		(void)accel_opengles_release(
			active->backend,
			error,
			sizeof(error));
	}

	/* Release only backend-owned plain metadata. */
	accel_opengles_free_execution(active);
}
