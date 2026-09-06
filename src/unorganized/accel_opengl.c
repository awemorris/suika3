/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Headless OpenGL ES compute backend using EGL. */

#include "../core/runtime.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_GL_PUSH_BINDING 31

struct accel_gl_resource {
	GLuint buffer;
	size_t size;
	uint64_t version;
	uint64_t host_version;
	struct accel_gl_resource *next;
};

struct accel_gl_runtime {
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	char renderer[256];
	bool unavailable;
	struct accel_gl_resource *resources;
};

struct accel_gl_pipeline {
	GLuint program;
	uint32_t push_size;
	uint32_t block_size;
	struct accel_gl_pipeline *next;
};

struct accel_gl_buffer {
	GLuint name;
	size_t size;
	bool owned;
};

enum accel_gl_submission_kind {
	ACCEL_GL_SUBMISSION_KERNEL,
	ACCEL_GL_SUBMISSION_RAW,
	ACCEL_GL_SUBMISSION_COPY_TO,
	ACCEL_GL_SUBMISSION_COPY_FROM
};

struct accel_gl_submission {
	int kind;
	GLsync fence;
	struct accel_gl_buffer buffers[NOCT_ARG_MAX];
	GLuint push_buffer;
	GLuint transfer_buffer;
	uint32_t arg_count;
	uint32_t output_index;
	uint32_t count;
	size_t transfer_size;
	size_t destination_offset;
	struct accel_gl_resource *output_resource;
	uint64_t output_version;
	bool output_host_was_current;
};

static void accel_gl_release_runtime(struct accel_gl_runtime *gl);
static struct accel_gl_runtime *accel_gl_get_runtime(struct rt_env *env);
static bool accel_gl_make_pipeline(struct rt_env *env,
				   struct accel_gl_runtime *gl,
				   struct accel_kernel *kernel,
				   const char *kernel_name,
				   uint32_t block_size,
				   struct accel_gl_pipeline **ret_pipeline);
static char *accel_gl_make_source(const char *source, size_t source_size,
				  size_t *output_size);
static char *accel_gl_replace(const char *source, const char *from,
			      const char *to, size_t *output_size);
static bool accel_gl_check_error(struct rt_env *env, const char *operation);
static bool accel_gl_read_buffer(struct rt_env *env, GLenum target,
				 GLintptr offset, GLsizeiptr size, void *destination,
				 const char *operation);
static int accel_gl_dispatch_internal(struct rt_env *env, struct rt_func *func,
				      uint32_t arg_count,
				      struct rt_value *arg,
				      struct accel_event *event);
static int accel_gl_dispatch_program(struct rt_env *env, struct rt_func *func,
				     uint32_t arg_count,
				     struct rt_value *arg);
static void accel_gl_free_submission(struct accel_gl_submission *submission);
static struct accel_gl_resource *accel_gl_get_resource(
	struct rt_env *env, struct rt_packed *packed);
static void accel_gl_advance_resource(struct accel_gl_resource *resource,
				      bool host_matches);

static void
accel_gl_release_runtime(
	struct accel_gl_runtime *gl)
{
	if (gl == NULL)
		return;
	if (gl->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(gl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			       EGL_NO_CONTEXT);
		if (gl->surface != EGL_NO_SURFACE)
			eglDestroySurface(gl->display, gl->surface);
		if (gl->context != EGL_NO_CONTEXT)
			eglDestroyContext(gl->display, gl->context);
		eglTerminate(gl->display);
	}
	gl->display = EGL_NO_DISPLAY;
	gl->context = EGL_NO_CONTEXT;
	gl->surface = EGL_NO_SURFACE;
}

static bool
accel_gl_is_software_renderer(
	const char *renderer)
{
	return renderer == NULL || strstr(renderer, "llvmpipe") != NULL ||
		strstr(renderer, "softpipe") != NULL ||
		strstr(renderer, "Software Rasterizer") != NULL;
}

bool
accel_opengl_list_devices(void)
{
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint config_count;
	EGLint major;
	EGLint minor;
	EGLint gl_major;
	EGLint gl_minor;
	EGLint ssbo_bindings;
	EGLint ubo_bindings;
	const GLubyte *renderer;
	bool found;
	static const EGLint config_attributes[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE,
	};
	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE,
	};
	static const EGLint surface_attributes[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE,
	};

	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	surface = EGL_NO_SURFACE;
	found = false;
#if defined(EGL_PLATFORM_SURFACELESS_MESA)
	display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
					EGL_DEFAULT_DISPLAY, NULL);
#endif
	if (display == EGL_NO_DISPLAY)
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY ||
	    !eglInitialize(display, &major, &minor) ||
	    !eglBindAPI(EGL_OPENGL_ES_API) ||
	    !eglChooseConfig(display, config_attributes, &config, 1,
			     &config_count) || config_count != 1)
		goto done;
	context = eglCreateContext(display, config, EGL_NO_CONTEXT,
				   context_attributes);
	if (context == EGL_NO_CONTEXT)
		goto done;
	surface = eglCreatePbufferSurface(display, config, surface_attributes);
	if (surface == EGL_NO_SURFACE ||
	    !eglMakeCurrent(display, surface, surface, context))
		goto done;
	glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
	glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &ssbo_bindings);
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &ubo_bindings);
	renderer = glGetString(GL_RENDERER);
	found = gl_major > 3 || (gl_major == 3 && gl_minor >= 1);
	found = found && ssbo_bindings >= NOCT_ARG_MAX;
	found = found && ubo_bindings > ACCEL_GL_PUSH_BINDING;
	found = found && !accel_gl_is_software_renderer((const char *)renderer);
	if (found) {
		printf("0  %s  OpenGL ES %d.%d  EGL %d.%d\n",
		       renderer != NULL ? (const char *)renderer : "unknown",
		       gl_major, gl_minor, major, minor);
	}
done:
	if (display != EGL_NO_DISPLAY) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			       EGL_NO_CONTEXT);
		if (surface != EGL_NO_SURFACE)
			eglDestroySurface(display, surface);
		if (context != EGL_NO_CONTEXT)
			eglDestroyContext(display, context);
		eglTerminate(display);
	}
	return found;
}

static struct accel_gl_runtime *
accel_gl_get_runtime(
	struct rt_env *env)
{
	struct accel_gl_runtime *gl;
	EGLConfig config;
	EGLint config_count;
	EGLint major;
	EGLint minor;
	EGLint gl_major;
	EGLint gl_minor;
	EGLint ssbo_bindings;
	EGLint ubo_bindings;
	const GLubyte *renderer;
	static const EGLint config_attributes[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE,
	};
	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE,
	};
	static const EGLint surface_attributes[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE,
	};

	if (env->vm->accel_runtime != NULL) {
		gl = env->vm->accel_runtime;
		return gl->unavailable ? NULL : gl;
	}
	gl = noct_calloc(1, sizeof(*gl));
	if (gl == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	gl->display = EGL_NO_DISPLAY;
	gl->context = EGL_NO_CONTEXT;
	gl->surface = EGL_NO_SURFACE;
	env->vm->accel_runtime = gl;
#if defined(EGL_PLATFORM_SURFACELESS_MESA)
	/* Prefer a display-server-independent context for SSH and services. */
	gl->display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
					    EGL_DEFAULT_DISPLAY, NULL);
#endif
	if (gl->display == EGL_NO_DISPLAY)
		gl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (gl->display == EGL_NO_DISPLAY ||
	    !eglInitialize(gl->display, &major, &minor) ||
	    !eglBindAPI(EGL_OPENGL_ES_API) ||
	    !eglChooseConfig(gl->display, config_attributes, &config, 1,
			     &config_count) || config_count != 1)
		goto unavailable;
	gl->context = eglCreateContext(gl->display, config, EGL_NO_CONTEXT,
				       context_attributes);
	if (gl->context == EGL_NO_CONTEXT)
		goto unavailable;
	gl->surface = eglCreatePbufferSurface(gl->display, config,
					      surface_attributes);
	if (gl->surface == EGL_NO_SURFACE ||
	    !eglMakeCurrent(gl->display, gl->surface, gl->surface, gl->context))
		goto unavailable;
	glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
	glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &ssbo_bindings);
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &ubo_bindings);
	renderer = glGetString(GL_RENDERER);
	if (renderer != NULL) {
		strncpy(gl->renderer, (const char *)renderer,
			sizeof(gl->renderer) - 1);
		gl->renderer[sizeof(gl->renderer) - 1] = '\0';
	}
	if ((gl_major < 3 || (gl_major == 3 && gl_minor < 1)) ||
	    ssbo_bindings < NOCT_ARG_MAX ||
	    ubo_bindings <= ACCEL_GL_PUSH_BINDING ||
	    accel_gl_is_software_renderer((const char *)renderer)) {
		if (env->vm->config.accel_info) {
			fprintf(stderr,
				"ACCEL: OpenGL device rejected: %s (OpenGL ES %d.%d)\n",
				gl->renderer[0] != '\0' ? gl->renderer : "unknown",
				gl_major, gl_minor);
			if (accel_gl_is_software_renderer((const char *)renderer))
				fprintf(stderr,
					"ACCEL: set GALLIUM_DRIVER=d3d12 under WSLg to enable the GPU\n");
		}
		goto unavailable;
	}
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: OpenGL device: %s (OpenGL ES %d.%d, EGL %d.%d)\n",
			gl->renderer, gl_major, gl_minor, major, minor);
	return gl;

unavailable:
	accel_gl_release_runtime(gl);
	gl->unavailable = true;
	return NULL;
}

static char *
accel_gl_replace(
	const char *source,
	const char *from,
	const char *to,
	size_t *output_size)
{
	const char *found;
	const char *cursor;
	char *output;
	char *dst;
	size_t source_size;
	size_t from_size;
	size_t to_size;
	size_t count;
	size_t size;

	source_size = strlen(source);
	from_size = strlen(from);
	to_size = strlen(to);
	if (from_size == 0)
		return NULL;
	count = 0;
	cursor = source;
	while ((found = strstr(cursor, from)) != NULL) {
		count++;
		cursor = found + from_size;
	}
	if (to_size == from_size) {
		size = source_size;
	} else if (to_size > from_size) {
		if (count > (SIZE_MAX - source_size - 1) / (to_size - from_size))
			return NULL;
		size = source_size + count * (to_size - from_size);
	} else {
		size = source_size - count * (from_size - to_size);
	}
	output = noct_malloc(size + 1);
	if (output == NULL)
		return NULL;
	dst = output;
	cursor = source;
	while ((found = strstr(cursor, from)) != NULL) {
		size_t prefix_size;
		prefix_size = (size_t)(found - cursor);
		memcpy(dst, cursor, prefix_size);
		dst += prefix_size;
		memcpy(dst, to, to_size);
		dst += to_size;
		cursor = found + from_size;
	}
	strcpy(dst, cursor);
	*output_size = size;
	return output;
}

static char *
accel_gl_make_source(
	const char *source,
	size_t source_size,
	size_t *output_size)
{
	char *stage1;
	char *stage2;
	char *stage3;
	char *stage4;
	size_t size1;
	size_t size2;
	size_t size3;
	size_t size4;

	UNUSED_PARAMETER(source_size);
	stage1 = accel_gl_replace(source, "#version 450",
				  "#version 310 es\nprecision highp float;\nprecision highp int;",
				  &size1);
	if (stage1 == NULL)
		return NULL;
	stage2 = accel_gl_replace(stage1, "set = 0, ", "", &size2);
	noct_free(stage1);
	if (stage2 == NULL)
		return NULL;
	stage3 = accel_gl_replace(stage2, "layout(push_constant)",
				  "layout(std140, binding = 31)", &size3);
	noct_free(stage2);
	if (stage3 == NULL)
		return NULL;
	stage4 = accel_gl_replace(stage3, "set=0,", "", &size4);
	noct_free(stage3);
	if (stage4 == NULL)
		return NULL;
	*output_size = size4;
	return stage4;
}

static bool
accel_gl_shader_status(
	struct rt_env *env,
	GLuint object,
	bool program,
	const char *kernel_name)
{
	GLint ok;
	GLint log_size;
	char *log;

	if (program)
		glGetProgramiv(object, GL_LINK_STATUS, &ok);
	else
		glGetShaderiv(object, GL_COMPILE_STATUS, &ok);
	if (ok == GL_TRUE)
		return true;
	if (program)
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_size);
	else
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_size);
	if (log_size < 1)
		log_size = 1;
	log = noct_malloc((size_t)log_size + 1);
	if (log == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	if (program)
		glGetProgramInfoLog(object, log_size, NULL, log);
	else
		glGetShaderInfoLog(object, log_size, NULL, log);
	log[log_size] = '\0';
	fprintf(stderr, "ACCEL: OpenGL kernel %s %s failed: %s\n",
		kernel_name, program ? "link" : "compile", log);
	noct_free(log);
	return false;
}

static bool
accel_gl_make_pipeline(
	struct rt_env *env,
	struct accel_gl_runtime *gl,
	struct accel_kernel *kernel,
	const char *kernel_name,
	uint32_t block_size,
	struct accel_gl_pipeline **ret_pipeline)
{
	struct accel_gl_pipeline *pipeline;
	char *source;
	char *materialized;
	size_t source_size;
	size_t materialized_size;
	char block_text[16];
	GLuint shader;
	GLuint block_index;
	uint32_t scalar_count;
	uint32_t i;

	UNUSED_PARAMETER(gl);
	pipeline = kernel->backend_data;
	while (pipeline != NULL) {
		if (pipeline->block_size == block_size) {
			*ret_pipeline = pipeline;
			return true;
		}
		pipeline = pipeline->next;
	}
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: kernel %s: compiling OpenGL pipeline\n",
			kernel_name);
	materialized = NULL;
	if (strstr(kernel->glsl, "__NOCT_LOCAL_SIZE_X__") != NULL) {
		snprintf(block_text, sizeof(block_text), "%u", block_size);
		materialized = accel_gl_replace(kernel->glsl,
			"__NOCT_LOCAL_SIZE_X__", block_text, &materialized_size);
		if (materialized == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}
	source = accel_gl_make_source(materialized != NULL ? materialized : kernel->glsl,
			      materialized != NULL ? materialized_size : kernel->glsl_size,
				      &source_size);
	noct_free(materialized);
	if (source == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	if (getenv("NOCT_ACCEL_DEBUG") != NULL)
		fprintf(stderr, "%s", source);
	shader = glCreateShader(GL_COMPUTE_SHADER);
	if (shader == 0) {
		noct_free(source);
		return false;
	}
	{
		const GLchar *shader_source;
		GLint shader_size;
		shader_source = source;
		shader_size = (GLint)source_size;
		glShaderSource(shader, 1, &shader_source, &shader_size);
	}
	glCompileShader(shader);
	noct_free(source);
	if (!accel_gl_shader_status(env, shader, false, kernel_name)) {
		glDeleteShader(shader);
		return false;
	}
	pipeline = noct_calloc(1, sizeof(*pipeline));
	if (pipeline == NULL) {
		glDeleteShader(shader);
		rt_out_of_memory(env);
		return false;
	}
	pipeline->program = glCreateProgram();
	pipeline->block_size = block_size;
	if (pipeline->program == 0) {
		glDeleteShader(shader);
		noct_free(pipeline);
		return false;
	}
	glAttachShader(pipeline->program, shader);
	glLinkProgram(pipeline->program);
	glDetachShader(pipeline->program, shader);
	glDeleteShader(shader);
	if (!accel_gl_shader_status(env, pipeline->program, true, kernel_name)) {
		glDeleteProgram(pipeline->program);
		noct_free(pipeline);
		return false;
	}
	block_index = glGetUniformBlockIndex(pipeline->program, "PushConstants");
	if (block_index == GL_INVALID_INDEX) {
		glDeleteProgram(pipeline->program);
		noct_free(pipeline);
		return false;
	}
	glUniformBlockBinding(pipeline->program, block_index,
			      ACCEL_GL_PUSH_BINDING);
	scalar_count = 0;
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			scalar_count++;
	}
	pipeline->push_size = (1 + scalar_count) * 4;
	if (!accel_gl_check_error(env, "pipeline creation")) {
		glDeleteProgram(pipeline->program);
		noct_free(pipeline);
		return false;
	}
	pipeline->next = kernel->backend_data;
	kernel->backend_data = pipeline;
	*ret_pipeline = pipeline;
	return true;
}

static bool
accel_gl_check_error(
	struct rt_env *env,
	const char *operation)
{
	GLenum error;

	error = glGetError();
	if (error == GL_NO_ERROR)
		return true;
	rt_error(env, "OpenGL accelerator %s failed (0x%x).", operation,
		 (unsigned int)error);
	return false;
}

/* GLES has no glGetBufferSubData(); map the bound buffer for readback. */
static bool
accel_gl_read_buffer(
	struct rt_env *env,
	GLenum target,
	GLintptr offset,
	GLsizeiptr size,
	void *destination,
	const char *operation)
{
	void *mapped;
	GLboolean unmapped;

	if (size == 0)
		return true;
	mapped = glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
	if (mapped == NULL) {
		if (accel_gl_check_error(env, operation))
			rt_error(env, "OpenGL ES accelerator %s returned no mapping.",
				 operation);
		return false;
	}
	memcpy(destination, mapped, (size_t)size);
	unmapped = glUnmapBuffer(target);
	if (unmapped != GL_TRUE) {
		if (accel_gl_check_error(env, operation))
			rt_error(env, "OpenGL ES accelerator %s lost mapped data.",
				 operation);
		return false;
	}
	return true;
}

static size_t
accel_gl_element_width(
	int type)
{
	switch (type) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
		return 1;
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
		return 2;
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
	case NOCT_PACKED_FLOAT32:
		return 4;
	case NOCT_PACKED_INT64:
	case NOCT_PACKED_UINT64:
	case NOCT_PACKED_FLOAT64:
		return 8;
	default:
		return 0;
	}
}

static struct accel_gl_resource *
accel_gl_get_resource(
	struct rt_env *env,
	struct rt_packed *packed)
{
	struct accel_gl_runtime *gl;
	struct accel_gl_resource *resource;
	size_t width;
	size_t size;

	if (!packed->is_accel_resource)
		return NULL;
	if (packed->accel_backend_data != NULL)
		return packed->accel_backend_data;
	gl = accel_gl_get_runtime(env);
	if (gl == NULL)
		return NULL;
	width = accel_gl_element_width(packed->type);
	if (width == 0 || packed->elem_size > SIZE_MAX / width)
		return NULL;
	size = packed->elem_size * width;
	resource = noct_calloc(1, sizeof(*resource));
	if (resource == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	resource->size = size;
	resource->version = 1;
	resource->host_version = 1;
	glGenBuffers(1, &resource->buffer);
	if (resource->buffer == 0) {
		noct_free(resource);
		return NULL;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)size,
		     packed->packed_buffer, GL_DYNAMIC_COPY);
	if (!accel_gl_check_error(env, "persistent resource allocation")) {
		glDeleteBuffers(1, &resource->buffer);
		noct_free(resource);
		return NULL;
	}
	resource->next = gl->resources;
	gl->resources = resource;
	packed->accel_backend_data = resource;
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: OpenGL persistent resource allocated (%lu bytes)\n",
			(unsigned long)size);
	return resource;
}

static void
accel_gl_advance_resource(
	struct accel_gl_resource *resource,
	bool host_matches)
{
	resource->version++;
	if (resource->version == 0) {
		resource->version = 1;
		resource->host_version = 0;
		host_matches = false;
	}
	if (host_matches)
		resource->host_version = resource->version;
}

static int
accel_gl_dispatch_program(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	struct accel_program *program;
	struct accel_gl_runtime *gl;
	struct accel_gl_buffer buffers[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_gl_resource *resources[ACCEL_PROGRAM_MAX_BUFFERS];
	int64_t scalar_arg[NOCT_ARG_MAX];
	int64_t buffer_length[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_program_step *step;
	struct accel_kernel *kernel;
	struct accel_gl_pipeline *pipeline;
	GLuint push_buffer;
	uint32_t push[NOCT_ARG_MAX + 1];
	uint32_t push_count;
	uint32_t i;
	uint32_t j;
	uint32_t trip_count;
	uint32_t group_count;
	uint32_t next_group_count;
	uint32_t current_buffer;
	uint32_t next_buffer;
	uint32_t bound_buffer;
	uint32_t zero_value;
	int64_t evaluated;
	size_t byte_size;
	size_t ubo_size;
	GLint max_group_count;
	char validation_error[128];
	int result;

	program = func->accel_program;
	if (program == NULL || arg_count != program->outer_param_count) {
		if (env->vm->config.accel_info)
			fprintf(stderr,
				"ACCEL: program %s unavailable (program=%s argc=%u)\n",
				func->name, program != NULL ? "yes" : "no",
				(unsigned int)arg_count);
		return ACCEL_DISPATCH_FALLBACK;
	}
	if (!accel_program_validate(program, validation_error,
				    sizeof(validation_error))) {
		if (env->vm->config.accel_info)
			fprintf(stderr, "ACCEL: program %s rejected: %s\n",
				func->name, validation_error);
		return ACCEL_DISPATCH_FALLBACK;
	}
	memset(scalar_arg, 0, sizeof(scalar_arg));
	memset(buffer_length, 0, sizeof(buffer_length));
	for (i = 0; i < arg_count; i++) {
		if (arg[i].type == NOCT_VALUE_INT)
			scalar_arg[i] = arg[i].val.i;
	}
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (desc->outer_param >= 0) {
			j = (uint32_t)desc->outer_param;
			if (j >= arg_count || arg[j].type != NOCT_VALUE_PACKED ||
			    arg[j].val.packed->type != desc->element_kind)
				return ACCEL_DISPATCH_FALLBACK;
			buffer_length[i] = (int64_t)arg[j].val.packed->elem_size;
		}
	}
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].outer_param < 0) {
			if (!accel_expr_evaluate(program,
						 program->buffer[i].length_expr,
						 arg_count, scalar_arg,
						 buffer_length, &evaluated))
				return ACCEL_DISPATCH_FALLBACK;
			buffer_length[i] = evaluated;
		}
		if (buffer_length[i] < 0 ||
		    (uint64_t)buffer_length[i] >
			SIZE_MAX / (size_t)program->buffer[i].element_width)
			return ACCEL_DISPATCH_FALLBACK;
	}
	for (i = 0; i < program->step_count; i++) {
		if (program->step[i].kind == ACCEL_STEP_DOSUM_REDUCTION &&
		    buffer_length[program->step[i].result_buffer] < 1) {
			rt_error(env,
				 "Accel.call(): DOSUM result buffer requires at least one element.");
			return ACCEL_DISPATCH_ERROR;
		}
	}
	gl = accel_gl_get_runtime(env);
	if (gl == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	memset(buffers, 0, sizeof(buffers));
	memset(resources, 0, sizeof(resources));
	push_buffer = 0;
	result = ACCEL_DISPATCH_FALLBACK;
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		byte_size = (size_t)buffer_length[i] *
			(size_t)desc->element_width;
		buffers[i].size = byte_size;
		if (desc->origin == ACCEL_BUFFER_DEVICE_PTR) {
			struct rt_packed *packed;
			packed = arg[desc->outer_param].val.packed;
			resources[i] = accel_gl_get_resource(env, packed);
			if (resources[i] == NULL ||
			    resources[i]->size != byte_size)
				goto cleanup;
			buffers[i].name = resources[i]->buffer;
			continue;
		}
		glGenBuffers(1, &buffers[i].name);
		if (buffers[i].name == 0)
			goto cleanup;
		buffers[i].owned = true;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[i].name);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)byte_size,
			     desc->upload ?
			     arg[desc->outer_param].val.packed->packed_buffer : NULL,
			     GL_DYNAMIC_COPY);
	}
	if (!accel_gl_check_error(env, "program buffer setup")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_group_count);
	for (i = 0; i < program->step_count; i++) {
		step = &program->step[i];
		if (step->kind != ACCEL_STEP_DOALL_DISPATCH &&
		    step->kind != ACCEL_STEP_DOSUM_REDUCTION) {
			result = ACCEL_DISPATCH_FALLBACK;
			goto cleanup;
		}
		kernel = program->kernel[step->kernel];
		if (!accel_expr_evaluate(program, step->trip_expr, arg_count,
					 scalar_arg, buffer_length, &evaluated) ||
		    evaluated < 0 || evaluated > UINT32_MAX) {
			result = ACCEL_DISPATCH_FALLBACK;
			goto cleanup;
		}
		trip_count = (uint32_t)evaluated;
		if (trip_count == 0) {
			if (step->kind == ACCEL_STEP_DOSUM_REDUCTION) {
				zero_value = 0;
				glBindBuffer(GL_SHADER_STORAGE_BUFFER,
					buffers[step->result_buffer].name);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
						4, &zero_value);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
						GL_BUFFER_UPDATE_BARRIER_BIT);
			}
			continue;
		}
		group_count = (trip_count + step->block_size - 1U) /
			step->block_size;
		if (group_count > (uint32_t)max_group_count) {
			if (step->kind != ACCEL_STEP_DOALL_DISPATCH)
				goto cleanup;
			group_count = (uint32_t)max_group_count;
		}
		if (!accel_gl_make_pipeline(env, gl, kernel,
					    kernel->name, step->block_size,
					    &pipeline))
			goto cleanup;
		for (j = 0; j < step->binding_count; j++) {
			if (step->binding[j].kind == ACCEL_BIND_BUFFER) {
				bound_buffer = (uint32_t)step->binding[j].value;
				if (step->kind == ACCEL_STEP_DOSUM_REDUCTION &&
				    step->binding[j].kernel_param ==
					(int)kernel->param_count - 1)
					bound_buffer = group_count == 1 ?
						(uint32_t)step->result_buffer :
						(uint32_t)step->scratch_buffer;
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
					(uint32_t)step->binding[j].kernel_param,
					buffers[bound_buffer].name);
			}
		}
		push_count = 0;
		push[push_count++] = trip_count;
		for (j = 0; j < kernel->param_count; j++) {
			uint32_t k;
			const struct accel_expr *binding_expr;
			if (kernel->param_transport[j] != ACCEL_TRANSPORT_SCALAR)
				continue;
			for (k = 0; k < step->binding_count; k++) {
				if (step->binding[k].kernel_param == (int)j)
					break;
			}
			if (k == step->binding_count ||
			    step->binding[k].kind != ACCEL_BIND_SCALAR_EXPR)
				goto cleanup;
			binding_expr = &program->expr[step->binding[k].value];
			if (binding_expr->op == ACCEL_EXPR_SCALAR_ARG &&
			    kernel->param_type[j] == NOCT_VALUE_FLOAT) {
				uint32_t outer;
				outer = (uint32_t)binding_expr->ref;
				memcpy(&push[push_count], &arg[outer].val.f, 4);
			} else {
				if (!accel_expr_evaluate(program,
							 step->binding[k].value,
							 arg_count, scalar_arg,
							 buffer_length, &evaluated) ||
				    evaluated < INT_MIN || evaluated > INT_MAX)
					goto cleanup;
				push[push_count] = (uint32_t)evaluated;
			}
			push_count++;
		}
		ubo_size = (pipeline->push_size + 15U) & ~(size_t)15U;
		glGenBuffers(1, &push_buffer);
		if (push_buffer == 0)
			goto cleanup;
		glBindBuffer(GL_UNIFORM_BUFFER, push_buffer);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)ubo_size, NULL,
			     GL_STREAM_DRAW);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, pipeline->push_size, push);
		glBindBufferBase(GL_UNIFORM_BUFFER, ACCEL_GL_PUSH_BINDING,
				 push_buffer);
		glUseProgram(pipeline->program);
		glDispatchCompute(group_count, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
				GL_BUFFER_UPDATE_BARRIER_BIT);
		glDeleteBuffers(1, &push_buffer);
		push_buffer = 0;

		if (step->kind != ACCEL_STEP_DOSUM_REDUCTION ||
		    group_count == 1)
			continue;
		current_buffer = (uint32_t)step->scratch_buffer;
		kernel = program->kernel[step->fold_kernel];
		if (!accel_gl_make_pipeline(env, gl, kernel,
					    kernel->name, step->block_size,
					    &pipeline))
			goto cleanup;
		while (group_count > 1) {
			next_group_count = (group_count + step->block_size - 1U) /
				step->block_size;
			if (next_group_count > (uint32_t)max_group_count)
				goto cleanup;
			if (next_group_count == 1)
				next_buffer = (uint32_t)step->result_buffer;
			else if (current_buffer == (uint32_t)step->scratch_buffer)
				next_buffer = (uint32_t)step->scratch_buffer2;
			else
				next_buffer = (uint32_t)step->scratch_buffer;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
				buffers[current_buffer].name);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
				buffers[next_buffer].name);
			push[0] = group_count;
			push[1] = group_count;
			ubo_size = (pipeline->push_size + 15U) & ~(size_t)15U;
			glGenBuffers(1, &push_buffer);
			if (push_buffer == 0)
				goto cleanup;
			glBindBuffer(GL_UNIFORM_BUFFER, push_buffer);
			glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)ubo_size,
				     NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_UNIFORM_BUFFER, 0,
					pipeline->push_size, push);
			glBindBufferBase(GL_UNIFORM_BUFFER,
					 ACCEL_GL_PUSH_BINDING, push_buffer);
			glUseProgram(pipeline->program);
			glDispatchCompute(next_group_count, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
					GL_BUFFER_UPDATE_BARRIER_BIT);
			glDeleteBuffers(1, &push_buffer);
			push_buffer = 0;
			current_buffer = next_buffer;
			group_count = next_group_count;
		}
	}
	glFinish();
	if (!accel_gl_check_error(env, "program dispatch")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (desc->download) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[i].name);
			if (!accel_gl_read_buffer(env, GL_SHADER_STORAGE_BUFFER, 0,
						  (GLsizeiptr)buffers[i].size,
						  arg[desc->outer_param].val.packed->packed_buffer,
						  "program output readback")) {
				result = ACCEL_DISPATCH_ERROR;
				goto cleanup;
			}
		}
		if (resources[i] != NULL &&
		    (program->outer_param_effect[desc->outer_param] &
		     ACCEL_EFFECT_WRITE) != 0)
			accel_gl_advance_resource(resources[i], false);
	}
	if (!accel_gl_check_error(env, "program output readback")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	result = ACCEL_DISPATCH_OK;

cleanup:
	glUseProgram(0);
	if (push_buffer != 0)
		glDeleteBuffers(1, &push_buffer);
	for (i = 0; i < program->buffer_count; i++) {
		if (buffers[i].owned && buffers[i].name != 0)
			glDeleteBuffers(1, &buffers[i].name);
	}
	return result;
}

static int
accel_gl_dispatch_internal(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	struct accel_kernel *kernel;
	struct accel_gl_runtime *gl;
	struct accel_gl_pipeline *pipeline;
	struct accel_gl_buffer buffers[NOCT_ARG_MAX];
	struct accel_gl_resource *resources[NOCT_ARG_MAX];
	struct accel_gl_submission *submission;
	struct accel_gl_resource *output_resource;
	GLuint push_buffer;
	GLsync fence;
	uint32_t push[NOCT_ARG_MAX + 1];
	uint32_t push_count;
	uint32_t count;
	uint32_t group_count;
	uint32_t local_size;
	uint32_t i;
	uint32_t j;
	size_t byte_size;
	size_t packed_size;
	size_t ubo_size;
	GLint max_group_count;
	bool submitted;
	bool output_host_was_current;
	uint64_t output_version;
	int result;

	kernel = func->accel_kernel;
	if (kernel == NULL || !kernel->eligible || arg_count != kernel->param_count)
		return ACCEL_DISPATCH_FALLBACK;
	count = 0;
	if (kernel->dispatch_param >= 0) {
		if (arg[kernel->dispatch_param].type != NOCT_VALUE_INT ||
		    arg[kernel->dispatch_param].val.i < 0)
			return ACCEL_DISPATCH_FALLBACK;
		count = (uint32_t)arg[kernel->dispatch_param].val.i;
	}
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			if (arg[i].type != kernel->param_type[i])
				return ACCEL_DISPATCH_FALLBACK;
			continue;
		}
		if (arg[i].type != NOCT_VALUE_PACKED ||
		    arg[i].val.packed->type != kernel->param_packed_type[i])
			return ACCEL_DISPATCH_FALLBACK;
		packed_size = arg[i].val.packed->elem_size;
		if ((size_t)count > packed_size)
			return ACCEL_DISPATCH_FALLBACK;
		for (j = 0; j < i; j++) {
			if (kernel->param_transport[j] != ACCEL_TRANSPORT_SCALAR &&
			    arg[j].val.packed == arg[i].val.packed)
				return ACCEL_DISPATCH_FALLBACK;
		}
	}
	if (count == 0)
		return ACCEL_DISPATCH_OK;
	gl = accel_gl_get_runtime(env);
	if (gl == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	local_size = kernel->parallel_mode == ACCEL_PARALLEL_SERIAL ? 1U : 64U;
	if (!accel_gl_make_pipeline(env, gl, kernel, func->name, local_size,
				    &pipeline))
		return ACCEL_DISPATCH_FALLBACK;
	memset(buffers, 0, sizeof(buffers));
	memset(resources, 0, sizeof(resources));
	submission = NULL;
	output_resource = NULL;
	output_host_was_current = false;
	output_version = 0;
	push_buffer = 0;
	fence = 0;
	submitted = false;
	result = ACCEL_DISPATCH_FALLBACK;
	if (event != NULL) {
		submission = noct_calloc(1, sizeof(*submission));
		if (submission == NULL) {
			rt_out_of_memory(env);
			return ACCEL_DISPATCH_ERROR;
		}
		if (kernel->output_param < 0) {
			noct_free(submission);
			return ACCEL_DISPATCH_ERROR;
		}
		event->output = arg[kernel->output_param];
		if (!rt_gc_pin_global(env, &event->output)) {
			noct_free(submission);
			return ACCEL_DISPATCH_ERROR;
		}
		event->output_pinned = true;
	}
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_group_count);
	group_count = kernel->parallel_mode == ACCEL_PARALLEL_SERIAL ?
		1U : (count + local_size - 1U) / local_size;
	if (group_count > (uint32_t)max_group_count)
		group_count = (uint32_t)max_group_count;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			continue;
		byte_size = arg[i].val.packed->elem_size * 4;
		buffers[i].size = byte_size;
		if (arg[i].val.packed->is_accel_resource) {
			struct accel_gl_resource *resource;
			resource = accel_gl_get_resource(env, arg[i].val.packed);
			if (resource == NULL || resource->size != byte_size)
				goto cleanup;
			buffers[i].name = resource->buffer;
			buffers[i].owned = false;
			resources[i] = resource;
		} else {
			glGenBuffers(1, &buffers[i].name);
			if (buffers[i].name == 0)
				goto cleanup;
			buffers[i].owned = true;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[i].name);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)byte_size,
				     kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_IN ?
				     arg[i].val.packed->packed_buffer : NULL,
				     GL_DYNAMIC_COPY);
		}
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, buffers[i].name);
	}
	push_count = 0;
	push[push_count++] = count;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR)
			continue;
		if (arg[i].type == NOCT_VALUE_FLOAT)
			memcpy(&push[push_count], &arg[i].val.f, 4);
		else
			memcpy(&push[push_count], &arg[i].val.i, 4);
		push_count++;
	}
	ubo_size = (pipeline->push_size + 15U) & ~(size_t)15U;
	glGenBuffers(1, &push_buffer);
	if (push_buffer == 0)
		goto cleanup;
	glBindBuffer(GL_UNIFORM_BUFFER, push_buffer);
	glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)ubo_size, NULL,
		     GL_STREAM_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, pipeline->push_size, push);
	glBindBufferBase(GL_UNIFORM_BUFFER, ACCEL_GL_PUSH_BINDING, push_buffer);
	if (!accel_gl_check_error(env, "buffer setup")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	glUseProgram(pipeline->program);
	glDispatchCompute(group_count, 1, 1);
	submitted = true;
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
			GL_BUFFER_UPDATE_BARRIER_BIT);
	output_resource = kernel->output_param >= 0 ?
		resources[(uint32_t)kernel->output_param] : NULL;
	if (output_resource != NULL) {
		output_host_was_current =
			output_resource->host_version == output_resource->version;
		accel_gl_advance_resource(output_resource, false);
		output_version = output_resource->version;
	}
	for (i = 0; i < arg_count; i++) {
		if (resources[i] == NULL || resources[i] == output_resource ||
		    (kernel->param_effect[i] & ACCEL_EFFECT_WRITE) == 0)
			continue;
		accel_gl_advance_resource(resources[i], false);
	}
	if (event != NULL) {
		uint32_t output_index;

		output_index = (uint32_t)kernel->output_param;
		/* Persistent resources can be reused by later queued commands.
		   Snapshot this event's output before those commands can modify it. */
		if (!buffers[output_index].owned) {
			GLuint snapshot;
			size_t snapshot_size;

			snapshot = 0;
			snapshot_size = (size_t)count * 4;
			glGenBuffers(1, &snapshot);
			if (snapshot == 0) {
				result = ACCEL_DISPATCH_ERROR;
				goto cleanup;
			}
			glBindBuffer(GL_COPY_WRITE_BUFFER, snapshot);
			glBufferData(GL_COPY_WRITE_BUFFER,
				     (GLsizeiptr)snapshot_size, NULL,
				     GL_STREAM_READ);
			glBindBuffer(GL_COPY_READ_BUFFER,
				     buffers[output_index].name);
			glCopyBufferSubData(GL_COPY_READ_BUFFER,
					    GL_COPY_WRITE_BUFFER, 0, 0,
					    (GLsizeiptr)snapshot_size);
			if (!accel_gl_check_error(env, "asynchronous output snapshot")) {
				glDeleteBuffers(1, &snapshot);
				result = ACCEL_DISPATCH_ERROR;
				goto cleanup;
			}
			buffers[output_index].name = snapshot;
			buffers[output_index].size = snapshot_size;
			buffers[output_index].owned = true;
		}
		fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush();
		if (fence == 0 || !accel_gl_check_error(env, "asynchronous submission")) {
			result = ACCEL_DISPATCH_ERROR;
			goto cleanup;
		}
		submission->fence = fence;
		submission->kind = ACCEL_GL_SUBMISSION_KERNEL;
		fence = 0;
		memcpy(submission->buffers, buffers, sizeof(buffers));
		submission->push_buffer = push_buffer;
		submission->arg_count = arg_count;
		submission->output_index = (uint32_t)kernel->output_param;
		submission->count = count;
		submission->output_resource = output_resource;
		submission->output_version = output_version;
		submission->output_host_was_current = output_host_was_current;
		memset(buffers, 0, sizeof(buffers));
		push_buffer = 0;
		event->backend_data = submission;
		submission = NULL;
		if (getenv("NOCT_ACCEL_DEBUG") != NULL)
			fprintf(stderr, "ACCEL: OpenGL asynchronous submission queued\n");
		result = ACCEL_DISPATCH_OK;
		goto cleanup;
	}
	glFinish();
	if (!accel_gl_check_error(env, "dispatch")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_COPY_OUT)
			continue;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[i].name);
		if (!accel_gl_read_buffer(env, GL_SHADER_STORAGE_BUFFER, 0,
					  (GLsizeiptr)((size_t)count * 4),
					  arg[i].val.packed->packed_buffer,
					  "output readback")) {
			result = ACCEL_DISPATCH_ERROR;
			goto cleanup;
		}
	}
	if (!accel_gl_check_error(env, "output readback")) {
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	if (output_resource != NULL &&
	    output_resource->version == output_version &&
	    (output_host_was_current ||
	     (size_t)count * 4 == output_resource->size))
		output_resource->host_version = output_resource->version;
	result = ACCEL_DISPATCH_OK;

cleanup:
	if (submitted && result == ACCEL_DISPATCH_FALLBACK)
		result = ACCEL_DISPATCH_ERROR;
	glUseProgram(0);
	if (fence != 0)
		glDeleteSync(fence);
	if (push_buffer != 0)
		glDeleteBuffers(1, &push_buffer);
	for (i = 0; i < arg_count; i++) {
		if (buffers[i].name != 0 && buffers[i].owned)
			glDeleteBuffers(1, &buffers[i].name);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	if (submission != NULL) {
		if (event != NULL && event->output_pinned) {
			rt_gc_unpin_global(env, &event->output);
			event->output_pinned = false;
		}
		noct_free(submission);
	}
	return result;
}

int
accel_opengl_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	if (func->accel_program != NULL)
		return accel_gl_dispatch_program(env, func, arg_count, arg);
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: kernel %s has no accel program\n", func->name);
	return accel_gl_dispatch_internal(env, func, arg_count, arg, NULL);
}

int
accel_opengl_dispatch_async(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	if (func->accel_program != NULL)
		return ACCEL_DISPATCH_FALLBACK;
	return accel_gl_dispatch_internal(env, func, arg_count, arg, event);
}

int
accel_opengl_dispatch_raw_async(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t grid_size,
	uint32_t block_size,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	struct accel_kernel *kernel;
	struct accel_gl_runtime *gl;
	struct accel_gl_pipeline *pipeline;
	struct accel_gl_submission *submission;
	struct accel_gl_resource *resource;
	GLuint push_buffer;
	uint32_t push[NOCT_ARG_MAX + 1];
	uint32_t push_count;
	uint32_t i;
	size_t ubo_size;
	GLint max_grid;
	GLint max_block;
	GLint max_invocations;

	kernel = func->accel_kernel;
	if (func->func_kind != NOCT_FUNC_GPU || kernel == NULL ||
	    !kernel->eligible || arg_count != kernel->param_count)
		return ACCEL_DISPATCH_ERROR;
	gl = accel_gl_get_runtime(env);
	if (gl == NULL) {
		rt_error(env, "OpenGL accelerator backend is unavailable for __gpu func.");
		return ACCEL_DISPATCH_ERROR;
	}
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_grid);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_block);
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
	if (grid_size == 0 || block_size == 0 ||
	    grid_size > (uint32_t)max_grid || block_size > (uint32_t)max_block ||
	    block_size > (uint32_t)max_invocations) {
		rt_error(env, "Accel.dispatchAsync(): launch dimensions exceed OpenGL limits.");
		return ACCEL_DISPATCH_ERROR;
	}
	if (!accel_gl_make_pipeline(env, gl, kernel, func->name, block_size,
				    &pipeline)) {
		rt_error(env, "Accel.dispatchAsync(): raw GPU pipeline creation failed.");
		return ACCEL_DISPATCH_ERROR;
	}
	submission = noct_calloc(1, sizeof(*submission));
	if (submission == NULL) { rt_out_of_memory(env); return ACCEL_DISPATCH_ERROR; }
	push_buffer = 0;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			continue;
		resource = accel_gl_get_resource(env, arg[i].val.packed);
		if (resource == NULL) goto fail;
		submission->buffers[i].name = resource->buffer;
		submission->buffers[i].size = resource->size;
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, resource->buffer);
	}
	push_count = 0;
	push[push_count++] = grid_size;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR) continue;
		if (arg[i].type == NOCT_VALUE_FLOAT)
			memcpy(&push[push_count], &arg[i].val.f, 4);
		else
			memcpy(&push[push_count], &arg[i].val.i, 4);
		push_count++;
	}
	ubo_size = (pipeline->push_size + 15U) & ~(size_t)15U;
	glGenBuffers(1, &push_buffer);
	if (push_buffer == 0) goto fail;
	glBindBuffer(GL_UNIFORM_BUFFER, push_buffer);
	glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)ubo_size, NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, pipeline->push_size, push);
	glBindBufferBase(GL_UNIFORM_BUFFER, ACCEL_GL_PUSH_BINDING, push_buffer);
	glUseProgram(pipeline->program);
	glDispatchCompute(grid_size, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_DEVICE_PTR ||
		    (kernel->param_effect[i] & ACCEL_EFFECT_WRITE) == 0)
			continue;
		resource = accel_gl_get_resource(env, arg[i].val.packed);
		if (resource != NULL) accel_gl_advance_resource(resource, false);
	}
	submission->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();
	if (submission->fence == 0 || !accel_gl_check_error(env, "raw GPU submission"))
		goto fail;
	submission->kind = ACCEL_GL_SUBMISSION_RAW;
	submission->push_buffer = push_buffer;
	submission->arg_count = arg_count;
	push_buffer = 0;
	event->backend_data = submission;
	glUseProgram(0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ACCEL_DISPATCH_OK;

fail:
	if (push_buffer != 0) glDeleteBuffers(1, &push_buffer);
	accel_gl_free_submission(submission);
	glUseProgram(0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ACCEL_DISPATCH_ERROR;
}

static void
accel_gl_free_submission(
	struct accel_gl_submission *submission)
{
	uint32_t i;

	if (submission == NULL)
		return;
	if (submission->fence != 0)
		glDeleteSync(submission->fence);
	if (submission->push_buffer != 0)
		glDeleteBuffers(1, &submission->push_buffer);
	if (submission->transfer_buffer != 0)
		glDeleteBuffers(1, &submission->transfer_buffer);
	for (i = 0; i < submission->arg_count; i++) {
		if (submission->buffers[i].name != 0 &&
		    submission->buffers[i].owned)
			glDeleteBuffers(1, &submission->buffers[i].name);
	}
	noct_free(submission);
}

int
accel_opengl_copy_to(
	struct rt_env *env,
	struct rt_packed *packed,
	size_t offset,
	size_t size)
{
	struct accel_gl_resource *resource;
	bool host_matches;

	resource = accel_gl_get_resource(env, packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > resource->size || size > resource->size - offset) {
		rt_error(env, "OpenGL accelerator upload range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	if (size != 0) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, (GLintptr)offset,
				(GLsizeiptr)size,
				(char *)packed->packed_buffer + offset);
		if (!accel_gl_check_error(env, "persistent resource upload"))
			return ACCEL_DISPATCH_ERROR;
		host_matches = resource->host_version == resource->version ||
			(offset == 0 && size == resource->size);
		accel_gl_advance_resource(resource, host_matches);
	}
	return ACCEL_DISPATCH_OK;
}

int
accel_opengl_copy_from(
	struct rt_env *env,
	struct rt_packed *packed,
	size_t offset,
	size_t size)
{
	struct accel_gl_resource *resource;

	resource = accel_gl_get_resource(env, packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > resource->size || size > resource->size - offset) {
		rt_error(env, "OpenGL accelerator download range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	if (size != 0) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
		if (!accel_gl_read_buffer(env, GL_SHADER_STORAGE_BUFFER,
					  (GLintptr)offset, (GLsizeiptr)size,
					  (char *)packed->packed_buffer + offset,
					  "persistent resource download"))
			return ACCEL_DISPATCH_ERROR;
		if (!accel_gl_check_error(env, "persistent resource download"))
			return ACCEL_DISPATCH_ERROR;
		if (offset == 0 && size == resource->size)
			resource->host_version = resource->version;
	}
	return ACCEL_DISPATCH_OK;
}

int
accel_opengl_copy_async(
	struct rt_env *env,
	bool to_accel,
	struct rt_packed *source,
	size_t source_offset,
	struct rt_packed *destination,
	size_t destination_offset,
	size_t size,
	struct accel_event *event)
{
	struct rt_packed *packed;
	struct accel_gl_resource *resource;
	struct accel_gl_submission *submission;
	GLsync fence;
	bool pinned;
	bool host_matches;

	if (size == 0)
		return ACCEL_DISPATCH_OK;
	packed = to_accel ? destination : source;
	resource = accel_gl_get_resource(env, packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	submission = noct_calloc(1, sizeof(*submission));
	if (submission == NULL) {
		rt_out_of_memory(env);
		return ACCEL_DISPATCH_ERROR;
	}
	pinned = false;
	fence = 0;
	if (to_accel) {
		memmove((char *)destination->packed_buffer + destination_offset,
			(char *)source->packed_buffer + source_offset, size);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER,
				(GLintptr)destination_offset, (GLsizeiptr)size,
				(char *)destination->packed_buffer +
				destination_offset);
		submission->kind = ACCEL_GL_SUBMISSION_COPY_TO;
	} else {
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGenBuffers(1, &submission->transfer_buffer);
		if (submission->transfer_buffer == 0)
			goto error;
		glBindBuffer(GL_COPY_WRITE_BUFFER, submission->transfer_buffer);
		glBufferData(GL_COPY_WRITE_BUFFER, (GLsizeiptr)size, NULL,
			     GL_STREAM_READ);
		glBindBuffer(GL_COPY_READ_BUFFER, resource->buffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
				    (GLintptr)source_offset, 0,
				    (GLsizeiptr)size);
		event->output.type = NOCT_VALUE_PACKED;
		event->output.val.packed = destination;
		if (!rt_gc_pin_global(env, &event->output))
			goto error;
		event->output_pinned = true;
		pinned = true;
		submission->kind = ACCEL_GL_SUBMISSION_COPY_FROM;
		submission->transfer_size = size;
		submission->destination_offset = destination_offset;
	}
	if (!accel_gl_check_error(env, to_accel ?
				 "asynchronous upload" : "asynchronous download"))
		goto error;
	if (to_accel) {
		host_matches = resource->host_version == resource->version ||
			(destination_offset == 0 && size == resource->size);
		accel_gl_advance_resource(resource, host_matches);
	}
	fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();
	if (fence == 0 ||
	    !accel_gl_check_error(env, "asynchronous copy submission"))
		goto error;
	submission->fence = fence;
	event->backend_data = submission;
	if (getenv("NOCT_ACCEL_DEBUG") != NULL)
		fprintf(stderr, "ACCEL: OpenGL asynchronous %s queued\n",
			to_accel ? "upload" : "download");
	return ACCEL_DISPATCH_OK;

error:
	if (fence != 0)
		glDeleteSync(fence);
	if (pinned) {
		rt_gc_unpin_global(env, &event->output);
		event->output_pinned = false;
	}
	event->output.type = NOCT_VALUE_INT;
	event->output.val.i = 0;
	accel_gl_free_submission(submission);
	return ACCEL_DISPATCH_ERROR;
}

bool
accel_opengl_sync_cpu(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	bool before_call)
{
	struct accel_gl_runtime *gl;
	struct accel_gl_resource *resource;
	uint32_t i;

	gl = accel_gl_get_runtime(env);
	if (gl == NULL)
		return true;
	for (i = 0; i < arg_count && i < func->param_count; i++) {
		if (func->param_accel_transport[i] == ACCEL_TRANSPORT_SCALAR ||
		    arg[i].type != NOCT_VALUE_PACKED ||
		    arg[i].val.packed == NULL ||
		    !arg[i].val.packed->is_accel_resource)
			continue;
		resource = accel_gl_get_resource(env, arg[i].val.packed);
		if (resource == NULL)
			return false;
		if (before_call) {
			if (resource->host_version == resource->version)
				continue;
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
			if (!accel_gl_read_buffer(env, GL_SHADER_STORAGE_BUFFER, 0,
						  (GLsizeiptr)resource->size,
						  arg[i].val.packed->packed_buffer,
						  "CPU fallback resource download"))
				return false;
			if (!accel_gl_check_error(env,
						  "CPU fallback resource download"))
				return false;
			resource->host_version = resource->version;
			if (getenv("NOCT_ACCEL_DEBUG") != NULL)
				fprintf(stderr,
					"ACCEL: OpenGL resource synchronized to CPU\n");
		} else if ((func->param_accel_effect[i] & ACCEL_EFFECT_WRITE) != 0) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, resource->buffer);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
					(GLsizeiptr)resource->size,
					arg[i].val.packed->packed_buffer);
			if (!accel_gl_check_error(env,
						  "CPU fallback resource upload"))
				return false;
			accel_gl_advance_resource(resource, true);
			if (getenv("NOCT_ACCEL_DEBUG") != NULL)
				fprintf(stderr,
					"ACCEL: OpenGL CPU result synchronized to GPU\n");
		}
	}
	return true;
}

bool
accel_opengl_join(
	struct rt_env *env,
	struct accel_event *event)
{
	struct accel_gl_submission *submission;
	GLenum wait_result;
	bool ok;
	int submission_kind;

	submission = event->backend_data;
	if (submission == NULL || submission->fence == 0) {
		rt_error(env, "OpenGL accelerator event has no submission.");
		return false;
	}
	submission_kind = submission->kind;
	wait_result = glClientWaitSync(submission->fence,
				       GL_SYNC_FLUSH_COMMANDS_BIT,
				       GL_TIMEOUT_IGNORED);
	ok = wait_result == GL_ALREADY_SIGNALED ||
		wait_result == GL_CONDITION_SATISFIED;
	if (ok && submission->kind == ACCEL_GL_SUBMISSION_KERNEL &&
	    (submission->output_resource == NULL ||
	     submission->output_resource->version == submission->output_version)) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER,
			     submission->buffers[submission->output_index].name);
		ok = accel_gl_read_buffer(env, GL_SHADER_STORAGE_BUFFER, 0,
					  (GLsizeiptr)((size_t)submission->count * 4),
					  event->output.val.packed->packed_buffer,
					  "asynchronous output readback");
		if (ok && submission->output_resource != NULL &&
		    (submission->output_host_was_current ||
		     (size_t)submission->count * 4 ==
		     submission->output_resource->size))
			submission->output_resource->host_version =
				submission->output_resource->version;
	} else if (ok && submission->kind == ACCEL_GL_SUBMISSION_COPY_FROM) {
		glBindBuffer(GL_COPY_READ_BUFFER, submission->transfer_buffer);
		ok = accel_gl_read_buffer(env, GL_COPY_READ_BUFFER, 0,
					  (GLsizeiptr)submission->transfer_size,
					  (char *)event->output.val.packed->packed_buffer +
					  submission->destination_offset,
					  "asynchronous download commit");
	} else if (!ok) {
		rt_error(env, "OpenGL accelerator event wait failed (0x%x).",
			 (unsigned int)wait_result);
	}
	accel_gl_free_submission(submission);
	event->backend_data = NULL;
	if (event->output_pinned) {
		rt_gc_unpin_global(env, &event->output);
		event->output_pinned = false;
	}
	event->output.type = NOCT_VALUE_INT;
	event->output.val.i = 0;
	if (getenv("NOCT_ACCEL_DEBUG") != NULL) {
		const char *kind;
		kind = submission_kind == ACCEL_GL_SUBMISSION_COPY_TO ? "upload" :
			submission_kind == ACCEL_GL_SUBMISSION_COPY_FROM ?
			"download" : "submission";
		fprintf(stderr, "ACCEL: OpenGL asynchronous %s completed\n", kind);
	}
	return ok;
}

static void
accel_gl_delete_kernel_pipelines(
	struct accel_kernel *kernel)
{
	struct accel_gl_pipeline *pipeline;
	struct accel_gl_pipeline *next_pipeline;

	if (kernel == NULL)
		return;
	pipeline = kernel->backend_data;
	while (pipeline != NULL) {
		next_pipeline = pipeline->next;
		glDeleteProgram(pipeline->program);
		noct_free(pipeline);
		pipeline = next_pipeline;
	}
	kernel->backend_data = NULL;
}

void
accel_opengl_cleanup(
	struct rt_vm *vm)
{
	struct accel_gl_runtime *gl;
	struct rt_func *func;
	struct accel_gl_resource *resource;
	struct accel_gl_resource *next_resource;
	uint32_t i;
	uint32_t k;

	gl = vm->accel_runtime;
	if (gl == NULL)
		return;
	if (!gl->unavailable && gl->display != EGL_NO_DISPLAY &&
	    gl->context != EGL_NO_CONTEXT) {
		eglMakeCurrent(gl->display, gl->surface, gl->surface, gl->context);
		for (i = 0; i < ACCEL_EVENT_MAX; i++) {
			if (vm->accel_event[i].state == ACCEL_EVENT_SUBMITTED) {
				accel_opengl_join(vm->env_list, &vm->accel_event[i]);
				vm->accel_event[i].state = ACCEL_EVENT_JOINED;
			}
		}
		glFinish();
		func = vm->func_list;
		while (func != NULL) {
			accel_gl_delete_kernel_pipelines(func->accel_kernel);
			if (func->accel_program != NULL) {
				for (k = 0; k < func->accel_program->kernel_count; k++)
					accel_gl_delete_kernel_pipelines(
						func->accel_program->kernel[k]);
			}
			func = func->next;
		}
		resource = gl->resources;
		while (resource != NULL) {
			next_resource = resource->next;
			if (resource->buffer != 0)
				glDeleteBuffers(1, &resource->buffer);
			noct_free(resource);
			resource = next_resource;
		}
		gl->resources = NULL;
	}
	accel_gl_release_runtime(gl);
	noct_free(gl);
	vm->accel_runtime = NULL;
}
