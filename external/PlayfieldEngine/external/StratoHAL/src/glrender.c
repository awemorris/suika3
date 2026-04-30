/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Graphics HAL for OpenGL
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* HAL */
#include "stratohal/platform.h"
#include "glrender.h"

/* Standard C */
#include <assert.h>

/*
 * Android (OpenGL ES 3.0)
 */
#if defined(HAL_TARGET_ANDROID)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

/*
 * Emscripten (We use OpenGL ES 3.0)
 */
#if defined(HAL_TARGET_WASM)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

/*
 * Linux and POSIX variant (OpenGL 3.2)
 */
#if (defined(HAL_TARGET_LINUX) && !defined(HAL_USE_GLES)) || defined(HAL_TARGET_POSIX)
#include <GL/gl.h>
#include <GL/glext.h>
#include "glhelper.h"
#endif

/*
 * Linux GBM (OpenGL ES 2.0/3.0)
 */
#if (defined(HAL_TARGET_LINUX) || defined(HAL_TARGET_FREEBSD)) && defined(HAL_USE_GLES)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include "glhelper.h"
#endif

/*
 * OpenHarmony (OpenGL ES 3.0)
 */
#if defined(HAL_TARGET_OPENHARMONY)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

/*
 * Windows (OpenGL 3.2) (Unused)
 */
#if defined(HAL_TARGET_WINDOWS)
#include <windows.h>
#include <GL/gl.h>
#include "glhelper.h"
#endif

/*
 * macOS (OpenGL 3.0) (Unused)
 */
#if defined(HAL_TARGET_MACOS)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include "glhelper.h"
#endif

/*
 * Qt (We use a wrapper for QOpenGLFunctions class)
 */
#if defined(USE_QT)
#include "glhelper.h"
#endif

/*
 * Pipeline types.
 */

enum {
	PIPELINE_NORMAL,
	PIPELINE_ADD,
	PIPELINE_SUB,
	PIPELINE_DIM,
	PIPELINE_RULE,
	PIPELINE_MELT,
	PIPELINE_CROSS,
};

/*
 * This is our vertex format. (6 parameters, XYZUVA...)
 */
struct pseudo_vertex_info {
	/* 0 (V_POS) */		float x;
	/* 1 */			float y;
	/* 2 */			float z; /* 0 */
	/* 3 (V_TEX1) */	float u1;
	/* 4 */			float v1;
	/* 5 (V_TEX2) */	float u2;
	/* 6 */			float v2;
	/* 7 (V_ALPHA) */	float alpha;
};
enum {
	V_POS_X = 0,
	V_POS_Y = 1,
	V_POS_Z = 2,
	V_TEX1_U = 3,
	V_TEX1_V = 4,
	V_TEX2_U = 5,
	V_TEX2_V = 6,
	V_ALPHA = 7,
	V_SIZE = 8,
};

/*
 * The sole vertex shader that is shared between all fragment shaders.
 */
static GLuint vertex_shader = (GLuint)-1;

/*
 * Fragment shaders.
 */

/* The normal alpha blending. */
static GLuint fragment_shader_normal = (GLuint)-1;

/* The character dimming. (RGB 50%) */
static GLuint fragment_shader_dim = (GLuint)-1;

/* The rule shader. (1-bit universal transition) */
static GLuint fragment_shader_rule = (GLuint)-1;

/* The melt shader. (8-bit universal transition) */
static GLuint fragment_shader_melt = (GLuint)-1;

/* The cross shader.  */
static GLuint fragment_shader_cross = (GLuint)-1;

/*
 * Program per fragment shader.
 */

/* For the normal alpha blending. */
static GLuint program_normal;

/* For the character dimming. (RGB 50%) */
static GLuint program_dim;

/* For the rule shader. (1-bit universal transition) */
static GLuint program_rule;

/* For the melt shader. (8-bit universal transition) */
static GLuint program_melt;

/* For the cross shader. */
static GLuint program_cross;

/*
 * VAO per fragment shader.
 */

/* For the normal alpha blending. */
static GLuint vao_normal;

/* For the character dimming. (RGB 50%) */
static GLuint vao_dim;

/* For the rule shader. (1-bit universal transition) */
static GLuint vao_rule;

/* For the melt shader. (8-bit universal transition) */
static GLuint vao_melt;

/* For the cross shader. */
static GLuint vao_cross;

/*
 * VBO per fragment shader.
 */

/* For the normal alpha blending. */
static GLuint vbo_normal;

/* For the character dimming. (RGB 50%) */
static GLuint vbo_dim;

/* For the rule shader. (1-bit universal transition) */
static GLuint vbo_rule;

/* For the melt shader. (8-bit universal transition) */
static GLuint vbo_melt;

/* For the cross shader. */
static GLuint vbo_cross;

/*
 * IBO per fragment shader.
 */

/* For the normal alpha blending. */
static GLuint ibo_normal;

/* For the character dimming. (RGB 50%) */
static GLuint ibo_dim;

/* For the rule shader. (1-bit universal transition) */
static GLuint ibo_rule;

/* For the melt shader. (8-bit universal transition) */
static GLuint ibo_melt;

/* For the cross shader. */
static GLuint ibo_cross;

/*
 * The vertex shader source.
 */

/* The source string. */
static const char *vertex_shader_src =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                 \n"
#endif
#if defined(HAL_TARGET_OPENHARMONY)
	"precision mediump float;     \n"
#endif
	"attribute vec4 a_position;   \n"
	"attribute vec2 a_texCoord1;  \n"
	"attribute vec2 a_texCoord2;  \n"
	"attribute float a_alpha;     \n"
	"varying vec2 v_texCoord1;    \n"
	"varying vec2 v_texCoord2;    \n"
	"varying float v_alpha;       \n"
	"void main()                  \n"
	"{                            \n"
#if !defined(HAL_USE_ROT90)
	"  gl_Position = a_position;\n"
#else
	"  vec2 rotated = vec2(-a_position.y, a_position.x); \n"
	"  gl_Position = vec4(rotated, a_position.z, 1.0);   \n"
#endif
	"  v_texCoord1 = a_texCoord1;  \n"
	"  v_texCoord2 = a_texCoord2;  \n"
	"  v_alpha = a_alpha;         \n"
	"}                            \n";

/*
 * Fragment shader sources.
 */

/* The normal alpha blending shader. */
static const char *fragment_shader_src_normal =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                                        \n"
#endif
#if defined(USE_QT)
	"#undef mediump                                      \n"
#endif
#if !defined(HAL_TARGET_MACOS)
	"precision mediump float;                            \n"
#endif
	"varying vec2 v_texCoord1;                           \n"
	"varying vec2 v_texCoord2;                           \n"
	"varying float v_alpha;                              \n"
	"uniform sampler2D s_texture1;                       \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 tex = texture2D(s_texture1, v_texCoord1);    \n"
	"  tex.a = tex.a * v_alpha;                          \n"
	"  gl_FragColor = tex;                               \n"
	"}                                                   \n";

/* The character dimming shader. (RGB 50%) */
static const char *fragment_shader_src_dim =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                                        \n"
#endif
#if defined(USE_QT)
	"#undef mediump                                      \n"
#endif
#if !defined(HAL_TARGET_MACOS)
	"precision mediump float;                            \n"
#endif
	"varying vec2 v_texCoord1;                           \n"
	"varying vec2 v_texCoord2;                           \n"
	"uniform sampler2D s_texture1;                       \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 tex = texture2D(s_texture1, v_texCoord1);    \n"
	"  tex.r = tex.r * 0.5;                              \n"
	"  tex.g = tex.g * 0.5;                              \n"
	"  tex.b = tex.b * 0.5;                              \n"
	"  gl_FragColor = tex;                               \n"
	"}                                                   \n";

/* The rule shader. (1-bit universal transition) */
static const char *fragment_shader_src_rule =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                                        \n"
#endif
#if defined(USE_QT)
	"#undef mediump                                      \n"
#endif
#if !defined(HAL_TARGET_MACOS)
	"precision mediump float;                            \n"
#endif
	"varying vec2 v_texCoord1;                           \n"
	"varying vec2 v_texCoord2;                           \n"
	"varying float v_alpha;                              \n"
	"uniform sampler2D s_texture1;                       \n"
	"uniform sampler2D s_texture2;                       \n"
	"uniform sampler2D s_rule;                           \n"
	"void main()                                         \n"
	"{                                                   \n"
        "  vec4 tex = texture2D(s_texture1, v_texCoord1);    \n"
	"  vec4 rule = texture2D(s_texture2, v_texCoord2);   \n"
	"  tex.a = 1.0 - step(v_alpha, rule.b);              \n"
	"  gl_FragColor = tex;                               \n"
	"}                                                   \n";

/* The melt shader. (8-bit universal transition) */
static const char *fragment_shader_src_melt =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                                                       \n"
#endif
#if defined(USE_QT)
	"#undef mediump                                                     \n"
#endif
#if !defined(HAL_TARGET_MACOS)
	"precision mediump float;                                           \n"
#endif
	"varying vec2 v_texCoord1;                                          \n"
	"varying vec2 v_texCoord2;                                          \n"
	"varying float v_alpha;                                             \n"
	"uniform sampler2D s_texture;                                       \n"
	"uniform sampler2D s_rule;                                          \n"
	"void main()                                                        \n"
	"{                                                                  \n"
        "  vec4 tex = texture2D(s_texture, v_texCoord1);                    \n"
	"  vec4 rule = texture2D(s_rule, v_texCoord1);                      \n"
        "  tex.a = clamp((1.0 - rule.b) + (v_alpha * 2.0 - 1.0), 0.0, 1.0); \n"
	"  gl_FragColor = tex;                                              \n"
	"}                                                                  \n";

/* The cross shader. */
static const char *fragment_shader_src_cross =
#if !defined(HAL_TARGET_WASM) && !defined(HAL_TARGET_MACOS)
	"#version 100                                                     \n"
#endif
#if defined(USE_QT)
	"#undef mediump                                                   \n"
#endif
#if !defined(HAL_TARGET_MACOS)
	"precision mediump float;                                         \n"
#endif
	"varying vec2 v_texCoord1;                                        \n"
	"varying vec2 v_texCoord2;                                        \n"
	"varying float v_alpha;                                           \n"
	"uniform sampler2D s_texture1;                                    \n"
	"uniform sampler2D s_texture2;                                    \n"
	"void main()                                                      \n"
	"{                                                                \n"
	"    vec4 src1 = texture2D(s_texture1, v_texCoord1);              \n"
	"    vec4 src2 = texture2D(s_texture2, v_texCoord2);              \n"
        "    gl_FragColor = mix(src2, src1, v_alpha);                     \n"
        "}                                                                \n";

/* Window size. */
static int window_width;
static int window_height;

/* Indicates if the first rendering after re-init. */
static bool is_after_reinit;

/* Re-init count. */
static int reinit_count;

/*
 * The following functions are defined in this file if we don't use Qt.
 * In the case we use Qt, they are defined in openglwidget.cpp because
 * VAO/VBO/IBO are abstracted in a very different way on the framework.
 */
bool setup_vertex_shader(const char **vshader_src, GLuint *vshader);
bool setup_fragment_shader(const char **fshader_src, GLuint vshader,
			   bool use_second_texture, GLuint *fshader,
			   GLuint *prog, GLuint *vao, GLuint *vbo,
			   GLuint *ibo);
void cleanup_vertex_shader(GLuint vshader);
void cleanup_fragment_shader(GLuint fshader, GLuint prog, GLuint vao,
GLuint vbo, GLuint ibo);

/*
 * Forward declaration.
 */
static void draw_elements(int dst_left,
			  int dst_top,
			  int dst_width,
			  int dst_height,
			  struct hal_image *src1_image,
			  struct hal_image *src2_image,
			  int src1_left,
			  int src1_top,
			  int src1_width,
			  int src1_height,
			  int src2_left,
			  int src2_top,
			  int src2_width,
			  int src2_height,
			  int alpha,
			  int pipeline);
static void draw_elements_3d(float x1,
			     float y1,
			     float x2,
			     float y2,
			     float x3,
			     float y3,
			     float x4,
			     float y4,
			     struct hal_image *src1_image,
			     struct hal_image *src2_image,
			     float src1_x1,
			     float src1_y1,
			     float src1_x2,
			     float src1_y2,
			     float src1_x3,
			     float src1_y3,
			     float src1_x4,
			     float src1_y5,
			     float src2_x1,
			     float src2_y1,
			     float src2_x2,
			     float src2_y2,
			     float src2_x3,
			     float src2_y3,
			     float src2_x4,
			     float src2_y5,
			     int alpha,
			     int pipeline);
static void update_texture_if_needed(struct hal_image *img);

/*
 * Initialize OpenGL.
 */
bool
init_opengl(
	int width,
	int height)
{
#if defined(HAL_TARGET_ANDROID) || (HAL_TARGET_OPENHARMONY)
	cleanup_opengl();
#endif

	/* Set a viewport. */
	window_width = width;
	window_height = height;
#if !defined(HAL_USE_ROT90)
	glViewport(0, 0, window_width, window_height);
#else
	glViewport(0, 0, window_height, window_width);
#endif

	/* Setup a vertex shader. */
	if (!setup_vertex_shader(&vertex_shader_src, &vertex_shader))
		return false;

	/* Setup the fragment shader for normal alpha blending. */
	if (!setup_fragment_shader(&fragment_shader_src_normal,
				   vertex_shader,
				   false, /* no second texture */
				   &fragment_shader_normal,
				   &program_normal,
				   &vao_normal,
				   &vbo_normal,
				   &ibo_normal))
		return false;

	/* Setup the fragment shader for character dimming (RGB 50%). */
	if (!setup_fragment_shader(&fragment_shader_src_dim,
				   vertex_shader,
				   false, /* no second texture */
				   &fragment_shader_dim,
				   &program_dim,
				   &vao_dim,
				   &vbo_dim,
				   &ibo_dim))
		return false;

	/* Setup the fragment shader for rule (1-bit universal transition). */
	if (!setup_fragment_shader(&fragment_shader_src_rule,
				   vertex_shader,
				   true, /* use second texture */
				   &fragment_shader_rule,
				   &program_rule,
				   &vao_rule,
				   &vbo_rule,
				   &ibo_rule))
		return false;

	/* Setup the fragment shader for melt (8-bit universal transition). */
	if (!setup_fragment_shader(&fragment_shader_src_melt,
				   vertex_shader,
				   true, /* use second texture */
				   &fragment_shader_melt,
				   &program_melt,
				   &vao_melt,
				   &vbo_melt,
				   &ibo_melt))
		return false;

	/* Setup the fragment shader for cross. */
	if (!setup_fragment_shader(&fragment_shader_src_cross,
				   vertex_shader,
				   true, /* use second texture */
				   &fragment_shader_cross,
				   &program_cross,
				   &vao_cross,
				   &vbo_cross,
				   &ibo_cross))
		return false;

	is_after_reinit = true;
	reinit_count++;

	return true;
}

#ifndef HAL_USE_QT

/*
 * Setup a vertex shader.
 */
bool
setup_vertex_shader(
	const char **vshader_src,	/* IN: A vertex shader source string. */
	GLuint *vshader)		/* OUT: A vertex shader ID. */
{
	char buf[1024];
	GLint compiled;
	int len;

	*vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(*vshader, 1, vshader_src, NULL);
	glCompileShader(*vshader);

	/* Check errors. */
	glGetShaderiv(*vshader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		hal_log_info("Vertex shader compile error");
		glGetShaderInfoLog(*vshader, sizeof(buf), &len, &buf[0]);
		hal_log_info("%s", buf);
		return false;
	}

	return true;
}

/*
 * Cleanup a vertex shader.
 */
void
cleanup_vertex_shader(
	GLuint vshader)
{
	glDeleteShader(vshader);
}

/*
 * Setup a fragment shader, a program, a VAO, a VBO and an IBO.
 */
bool
setup_fragment_shader(
	const char **fshader_src,	/* IN: A fragment shader source string. */
	GLuint vshader,			/* IN: A vertex shader ID. */
	bool use_second_texture,	/* IN: Whether to use a second texture. */
	GLuint *fshader,		/* OUT: A fragment shader ID. */
	GLuint *prog,			/* OUT: A program ID. */
	GLuint *vao,			/* OUT: A VAO ID. */
	GLuint *vbo,			/* OUT: A VBO ID. */
	GLuint *ibo)			/* OUT: An IBO ID. */
{
	char err_msg[1024];
	GLint pos_loc, tex1_loc, tex2_loc, alpha_loc, sampler1_loc, sampler2_loc;
	GLint is_succeeded;
	int err_len;

	const GLushort indices[] = {0, 1, 2, 3};

	/* Create a fragment shader. */
	*fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(*fshader, 1, fshader_src, NULL);
	glCompileShader(*fshader);

	/* Check errors. */
	glGetShaderiv(*fshader, GL_COMPILE_STATUS, &is_succeeded);
	if (!is_succeeded) {
		hal_log_info("Fragment shader compile error");
		glGetShaderInfoLog(*fshader, sizeof(err_msg), &err_len, &err_msg[0]);
		hal_log_info("%s", err_msg);
		return false;
	}

	/* Create a program. */
	*prog = glCreateProgram();
	glAttachShader(*prog, vshader);
	glAttachShader(*prog, *fshader);
	glLinkProgram(*prog);
	glGetProgramiv(*prog, GL_LINK_STATUS, &is_succeeded);
	if (!is_succeeded) {
		hal_log_info("Program link error\n");
		glGetProgramInfoLog(*prog, sizeof(err_msg), &err_len, &err_msg[0]);
		hal_log_info("%s", err_msg);
		return false;
	}
	glUseProgram(*prog);

	/* Create a VAO. */
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	/* Create a VBO. */
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);

	/* Set the vertex attibute for "a_position" (V_POS) in the vertex shader. */
	pos_loc = glGetAttribLocation(*prog, "a_position");
	glVertexAttribPointer((GLuint)pos_loc,
			      3,	/* (x, y, z) */
			      GL_FLOAT,
			      GL_FALSE,
			      V_SIZE * sizeof(GLfloat),
			      (const GLvoid *)V_POS_X);
	glEnableVertexAttribArray((GLuint)pos_loc);

	/* Set the vertex attibute for "a_texCoord1" (V_TEX1) in the vertex shader. */
	tex1_loc = glGetAttribLocation(*prog, "a_texCoord1");
	glVertexAttribPointer((GLuint)tex1_loc,
			      2,	/* (u1, v1) */
			      GL_FLOAT,
			      GL_FALSE,
			      V_SIZE * sizeof(GLfloat),
			      (const GLvoid *)(V_TEX1_U * sizeof(GLfloat)));
	glEnableVertexAttribArray((GLuint)tex1_loc);

	/* Set the vertex attibute for "a_texCoord2" (V_TEX2) in the vertex shader. */
	tex2_loc = glGetAttribLocation(*prog, "a_texCoord2");
	glVertexAttribPointer((GLuint)tex2_loc,
			      2,	/* (u2, v2) */
			      GL_FLOAT,
			      GL_FALSE,
			      V_SIZE * sizeof(GLfloat),
			      (const GLvoid *)(V_TEX2_U * sizeof(GLfloat)));
	glEnableVertexAttribArray((GLuint)tex2_loc);

	/* Set the vertex attibute for "a_alpha" (V_ALPHA) in the vertex shader. */
	alpha_loc = glGetAttribLocation(*prog, "a_alpha");
	glVertexAttribPointer((GLuint)alpha_loc,
			      1,	/* (alpha) */
			      GL_FLOAT,
			      GL_FALSE,
			      V_SIZE * sizeof(GLfloat),
			      (const GLvoid *)(V_ALPHA * sizeof(GLfloat)));
	glEnableVertexAttribArray((GLuint)alpha_loc);

	/* Setup "s_texture" in a fragment shader. */
	sampler1_loc = glGetUniformLocation(*prog, "s_texture1");
	glUniform1i(sampler2_loc, 0);

	/* Setup "s_rule" in a fragment shader if we use a second texture. */
	if (use_second_texture) {
		sampler2_loc = glGetUniformLocation(*prog, "s_texture2");
		glUniform1i(sampler2_loc, 1);
	}

	/* Create an IBO. */
	glGenBuffers(1, ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		     GL_STATIC_DRAW);

	return true;
}

void
cleanup_fragment_shader(
	GLuint fshader,
	GLuint prog,
	GLuint vao,
	GLuint vbo,
	GLuint ibo)
{
	GLuint a[1];

	/* Delete a fragment shader. */
	glDeleteShader(fshader);

	/* Delete a program. */
	glDeleteProgram(prog);

	/* Delete a VAO. */
	a[0] = vao;
	glDeleteVertexArrays(1, (const GLuint *)&a);

	/* Delete a VBO. */
	a[0] = vbo;
	glDeleteBuffers(1, (const GLuint *)&a);

	/* Delete an IBO. */
	a[0] = ibo;
	glDeleteBuffers(1, (const GLuint *)&a);
}

#endif	/* ifndef HAL_USE_QT */

/*
 * Cleanup OpenGL.
 *  - Note: On Emscripten, this will never be called
 */
void
cleanup_opengl(void)
{
	if (fragment_shader_normal != (GLuint)-1) {
		cleanup_fragment_shader(fragment_shader_normal,
					program_normal,
					vao_normal,
					vbo_normal,
					ibo_normal);
		fragment_shader_normal = (GLuint)-1;
	}
	if (fragment_shader_dim != (GLuint)-1) {
		cleanup_fragment_shader(fragment_shader_dim,
					program_dim,
					vao_dim,
					vbo_dim,
					ibo_dim);
		fragment_shader_dim = (GLuint)-1;
	}
	if (fragment_shader_rule != (GLuint)-1) {
		cleanup_fragment_shader(fragment_shader_rule,
					program_rule,
					vao_rule,
					vbo_rule,
					ibo_rule);
		fragment_shader_rule = (GLuint)-1;
	}
	if (fragment_shader_melt != (GLuint)-1) {
		cleanup_fragment_shader(fragment_shader_melt,
					program_melt,
					vao_melt,
					vbo_melt,
					ibo_melt);
		fragment_shader_melt = (GLuint)-1;
	}
	if (vertex_shader != (GLuint)-1) {
		cleanup_vertex_shader(vertex_shader);
		vertex_shader = (GLuint)-1;
	}
}

/*
 * Start a frame rendering.
 */
void
opengl_start_rendering(void)
{
#if defined(HAL_USE_QT) || defined(HAL_USE_WAYLAND)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
#else
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
#endif
}

/*
 * End a frame rendering.
 */
void
opengl_end_rendering(void)
{
	glFlush();
	is_after_reinit = false;
}

/*
 * Texture manipulation:
 *  - "Texture" here is a GPU backend of an image.
 *  - The app abstracts modifications of textures by "notify" operations.
 *  - Updated textures will be uploaded to GPU using glTexImage2D() when they are rendered.
 */

/*
 * Lock a texture.
 *  - This will just allocate memory for a texture management struct
 *  - We just use pixels of a frontend image for modification
 */
void
opengl_notify_image_update(
	struct hal_image *img)
{
	img->need_upload = true;
}

/*
 * Destroy a texture.
 */
void
opengl_notify_image_free(
	struct hal_image *img)
{
	GLuint id;

	id = (GLuint)(uintptr_t)img->texture - 1;

	/* FIXME: is_after_reinit */
	if (id != 0) {
		glDeleteTextures(1, &id);
		img->texture = NULL;
	}

	img->need_upload = false;
}

/*
 * Render an image with the normal pipeline.
 */
void
opengl_render_image_normal(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	/* Check for the case we don't need to render. */
	if (alpha == 0 || dst_width == 0 || dst_height == 0)
		return;

	draw_elements(dst_left,
		      dst_top,
		      dst_width,
		      dst_height,
		      src_image,
		      NULL,
		      src_left,
		      src_top,
		      src_width,
		      src_height,
		      0, 0, 0, 0,
		      alpha,
		      PIPELINE_NORMAL);
}

/*
 * Render an image with the add pipeline.
 */
void
opengl_render_image_add(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	/* Check for the case we don't need to render. */
	if (alpha == 0 || dst_width == 0 || dst_height == 0)
		return;

	draw_elements(dst_left,
		      dst_top,
		      dst_width,
		      dst_height,
		      src_image,
		      NULL,
		      src_left,
		      src_top,
		      src_width,
		      src_height,
		      0, 0, 0, 0,
		      alpha,
		      PIPELINE_ADD);
}

/*
 * Render an image with the sub pipeline.
 */
void
opengl_render_image_sub(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	/* Check for the case we don't need to render. */
	if (alpha == 0 || dst_width == 0 || dst_height == 0)
		return;

	draw_elements(dst_left,
		      dst_top,
		      dst_width,
		      dst_height,
		      src_image,
		      NULL,
		      src_left,
		      src_top,
		      src_width,
		      src_height,
		      0, 0, 0, 0,
		      alpha,
		      PIPELINE_SUB);
}

/*
 * Render an image with the dim pipeline.
 */
void
opengl_render_image_dim(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	/* Check for the case we don't need to render. */
	if (alpha == 0 || dst_width == 0 || dst_height == 0)
		return;

	draw_elements(dst_left,
		      dst_top,
		      dst_width,
		      dst_height,
		      src_image,
		      NULL,
		      src_left,
		      src_top,
		      src_width,
		      src_height,
		      0, 0, 0, 0,
		      255,
		      PIPELINE_DIM);
}

/*
 * Render an image with the rule pipeline.
 */
void
opengl_render_image_rule(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int threshold)
{
	draw_elements(0,
		      0,
		      window_width,
		      window_height,
		      src_image,
		      rule_image,
		      0,
		      0,
		      window_width,
		      window_height,
		      0,
		      0,
		      rule_image->width,
		      rule_image->height,
		      threshold,
		      PIPELINE_RULE);
}

/*
 * Render an image with the melt pipeline.
 */
void
opengl_render_image_melt(
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int progress)
{
	draw_elements(0,
		      0,
		      window_width,
		      window_height,
		      src_image,
		      rule_image,
		      0,
		      0,
		      window_width,
		      window_height,
		      0,
		      0,
		      rule_image->width,
		      rule_image->height,
		      progress,
		      PIPELINE_MELT);
}

/*
 * Render an image with the cross pipeline.
 */
void
opengl_render_image_cross(
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	int src1_left,
	int src1_top,
	int src2_left,
	int src2_top,
	int alpha)
{
	draw_elements_3d(0,
			 0,
			 window_width,
			 0,
			 0,
			 window_height,
			 window_width,
			 window_height,
			 src1_image,
			 src2_image,
			 -src1_left,			-src1_top,
			 window_width - src1_left, 	-src1_top,
			 -src1_left,			window_height - src1_top,
			 window_width - src1_left,	window_height - src1_top,
			 -src2_left,                	-src2_top,
			 window_width - src2_left,	-src2_top,
			 -src2_left,			window_height - src2_top,
			 window_width - src2_left,	window_height - src2_top,
			 alpha,
			 PIPELINE_CROSS);
}

/* Render two triangle primitives. */
static void
draw_elements(
	int dst_left,
	int dst_top,
	int dst_width,
	int dst_height,
	struct hal_image *src_image,
	struct hal_image *rule_image,
	int src1_left,
	int src1_top,
	int src1_width,
	int src1_height,
	int src2_left,
	int src2_top,
	int src2_width,
	int src2_height,
	int alpha,
	int pipeline)
{
	draw_elements_3d((float)dst_left,
			 (float)dst_top,
			 (float)(dst_left + dst_width),
			 (float)dst_top,
			 (float)dst_left,
			 (float)(dst_top + dst_height),
			 (float)(dst_left + dst_width),
			 (float)(dst_top + dst_height),
			 src_image,
			 rule_image,
			 src1_left,
			 src1_top,
			 src1_left + src1_width,
			 src1_top,
			 src1_left,
			 src1_top + src1_height,
			 src1_left + src1_width,
			 src1_top + src1_height,
			 src2_left,
			 src2_top,
			 src2_left + src2_width,
			 src2_top,
			 src2_left,
			 src2_top + src2_height,
			 src2_left + src2_width,
			 src2_top + src2_height,
			 alpha,
			 pipeline);
}

/*
 * Render an image with the normal pipeline.
 */
void
opengl_render_image_3d_normal(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	draw_elements_3d(x1,
			 y1,
			 x2,
			 y2,
			 x3,
			 y3,
			 x4,
			 y4,
			 src_image,
			 NULL,
			 src_left,
			 src_top,
			 src_left + src_width,
			 src_top,
			 src_left,
			 src_top + src_height,
			 src_left + src_width,
			 src_top + src_height,
			 0, 0, 0, 0, 0, 0, 0, 0,
			 alpha,
			 PIPELINE_NORMAL);
}

/*
 * Render an image with the add pipeline.
 */
void
opengl_render_image_3d_add(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	draw_elements_3d(x1,
			 y1,
			 x2,
			 y2,
			 x3,
			 y3,
			 x4,
			 y4,
			 src_image,
			 NULL,
			 src_left,
			 src_top,
			 src_left + src_width,
			 src_top,
			 src_left,
			 src_top + src_height,
			 src_left + src_width,
			 src_top + src_height,
			 0, 0, 0, 0, 0, 0, 0, 0,
			 alpha,
			 PIPELINE_ADD);
}

/*
 * Render an image with the sub pipeline.
 */
void
opengl_render_image_3d_sub(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	draw_elements_3d(x1,
			 y1,
			 x2,
			 y2,
			 x3,
			 y3,
			 x4,
			 y4,
			 src_image,
			 NULL,
			 src_left,
			 src_top,
			 src_left + src_width,
			 src_top,
			 src_left,
			 src_top + src_height,
			 src_left + src_width,
			 src_top + src_height,
			 0, 0, 0, 0, 0, 0, 0, 0,
			 alpha,
			 PIPELINE_SUB);
}

/*
 * Render an image with the dim pipeline.
 */
void
opengl_render_image_3d_dim(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src_image,
	int src_left,
	int src_top,
	int src_width,
	int src_height,
	int alpha)
{
	draw_elements_3d(x1,
			 y1,
			 x2,
			 y2,
			 x3,
			 y3,
			 x4,
			 y4,
			 src_image,
			 NULL,
			 src_left,
			 src_top,
			 src_left + src_width,
			 src_top,
			 src_left,
			 src_top + src_height,
			 src_left + src_width,
			 src_top + src_height,
			 0, 0, 0, 0, 0, 0, 0, 0,
			 alpha,
			 PIPELINE_DIM);
}

void
opengl_render_image_cross_3d(
        struct hal_image *src1_image,
        struct hal_image *src2_image,
	float src1_x1,
	float src1_y1,
	float src1_x2,
	float src1_y2,
	float src1_x3,
	float src1_y3,
	float src1_x4,
	float src1_y4,
	float src2_x1,
	float src2_y1,
	float src2_x2,
	float src2_y2,
	float src2_x3,
	float src2_y3,
	float src2_x4,
	float src2_y4,
	int alpha)
{
    float s1_tx[4], s1_ty[4];
    float s2_tx[4], s2_ty[4];
    float screen_x[] = { 0.0f, (float)window_width, 0.0f, (float)window_width };
    float screen_y[] = { 0.0f, 0.0f, (float)window_height, (float)window_height };

    {
        float dx1 = src1_x2 - src1_x1;
        float dy1 = src1_y2 - src1_y1;
        float dx2 = src1_x3 - src1_x1;
        float dy2 = src1_y3 - src1_y1;
        float det = dx1 * dy2 - dy1 * dx2;

        if (det != 0.0f) {
            for (int i = 0; i < 4; i++) {
                float rx = screen_x[i] - src1_x1;
                float ry = screen_y[i] - src1_y1;
                float a = ( dy2 * rx - dx2 * ry) / det;
                float b = (-dy1 * rx + dx1 * ry) / det;
                s1_tx[i] = a * (float)src1_image->width;
                s1_ty[i] = b * (float)src1_image->height;
            }
        } else {
            for (int i = 0; i < 4; i++) s1_tx[i] = s1_ty[i] = 0.0f;
        }
    }

    {
        float dx1 = src2_x2 - src2_x1;
        float dy1 = src2_y2 - src2_y1;
        float dx2 = src2_x3 - src2_x1;
        float dy2 = src2_y3 - src2_y1;
        float det = dx1 * dy2 - dy1 * dx2;

        if (det != 0.0f) {
            for (int i = 0; i < 4; i++) {
                float rx = screen_x[i] - src2_x1;
                float ry = screen_y[i] - src2_y1;
                float a = ( dy2 * rx - dx2 * ry) / det;
                float b = (-dy1 * rx + dx1 * ry) / det;
                s2_tx[i] = a * (float)src2_image->width;
                s2_ty[i] = b * (float)src2_image->height;
            }
        } else {
            for (int i = 0; i < 4; i++) s2_tx[i] = s2_ty[i] = 0.0f;
        }
    }

    draw_elements_3d(0.0f, 0.0f,
		     (float)window_width, 0.0f,
		     0.0f, (float)window_height,
		     (float)window_width, (float)window_height,
		     src1_image, src2_image,
		     s1_tx[0], s1_ty[0], s1_tx[1], s1_ty[1],
		     s1_tx[2], s1_ty[2], s1_tx[3], s1_ty[3],
		     s2_tx[0], s2_ty[0], s2_tx[1], s2_ty[1],
		     s2_tx[2], s2_ty[2], s2_tx[3], s2_ty[3],
		     alpha, PIPELINE_CROSS);
}

/* Render two triangle primitives. */
static void
draw_elements_3d(
	float x1,
	float y1,
	float x2,
	float y2,
	float x3,
	float y3,
	float x4,
	float y4,
	struct hal_image *src1_image,
	struct hal_image *src2_image,
	float src1_tx1,
	float src1_ty1,
	float src1_tx2,
	float src1_ty2,
	float src1_tx3,
	float src1_ty3,
	float src1_tx4,
	float src1_ty4,
	float src2_tx1,
	float src2_ty1,
	float src2_tx2,
	float src2_ty2,
	float src2_tx3,
	float src2_ty3,
	float src2_tx4,
	float src2_ty4,
	int alpha,
	int pipeline)
{
	GLfloat pos[32];
	float hw, hh, tw1, th1, tw2, th2;
	GLuint tex1, tex2;

	update_texture_if_needed(src1_image);
	update_texture_if_needed(src2_image);

	/* Get textures. */
	tex1 = (GLuint)(intptr_t)src1_image->texture - 1;
	assert(tex1 != 0);
	if (src2_image != NULL) {
		tex2 = (GLuint)(intptr_t)src2_image->texture - 1;
		assert(tex1 != 0);
	} else {
		tex2 = 0;
	}

	/* Get the half of the window size. */
	hw = (float)window_width / 2.0f;
	hh = (float)window_height / 2.0f;

	/* Get the texture size. */
	tw1 = (float)src1_image->width;
	th1 = (float)src1_image->height;
	if (src2_image != NULL) {
		tw2 = (float)src2_image->width;
		th2 = (float)src2_image->height;
	} else {
		tw2 = 1;
		th2 = 1;
	}

	/* Left-Top */
	pos[0] = (x1 - hw) / hw;
	pos[1] = -(y1 - hh) / hh;
	pos[2] = 0.0f;
	pos[3] = src1_tx1 / tw1;
	pos[4] = src1_ty1 / th1;
	pos[5] = src2_tx1 / tw2;
	pos[6] = src2_ty1 / th2;
	pos[7] = (float)alpha / 255.0f;

	/* Right-Top */
	pos[8] = (x2 - hw) / hw;
	pos[9] = -(y2 - hh) / hh;
	pos[10] = 0.0f;
	pos[11] = src1_tx2 / tw1;
	pos[12] = src1_ty2 / th1;
	pos[13] = src2_tx2 / tw2;
	pos[14] = src2_ty2 / th2;
	pos[15] = (float)alpha / 255.0f;

	/* Left-Bottom */
	pos[16] = (x3 - hw) / hw;
	pos[17] = -(y3 - hh) / hh;
	pos[18] = 0.0f;
	pos[19] = src1_tx3 / tw1;
	pos[20] = src1_ty3 / th1;
	pos[21] = src2_tx3 / tw2;
	pos[22] = src2_ty3 / th2;
	pos[23] = (float)alpha / 255.0f;

	/* Right-Bottom */
	pos[24] = (x4 - hw) / hw;
	pos[25] = -(y4 - hh) / hh;
	pos[26] = 0.0f;
	pos[27] = src1_tx4 / tw1;
	pos[28] = src1_ty4 / th1;
	pos[29] = src2_tx4 / tw2;
	pos[30] = src2_ty4 / th2;
	pos[31] = (float)alpha / 255.0f;

	/* Setup the shader. */
	switch (pipeline) {
	case PIPELINE_NORMAL:
		glUseProgram(program_normal);
		glBindVertexArray(vao_normal);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_normal);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case PIPELINE_ADD:
		glUseProgram(program_normal);
		glBindVertexArray(vao_normal);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_normal);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case PIPELINE_SUB:
		glUseProgram(program_normal);
		glBindVertexArray(vao_normal);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_normal);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_SUBTRACT);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case PIPELINE_DIM:
		glUseProgram(program_dim);
		glBindVertexArray(vao_dim);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_dim);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_dim);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case PIPELINE_RULE:
		glUseProgram(program_rule);
		glBindVertexArray(vao_rule);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_rule);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_rule);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case PIPELINE_MELT:
		glUseProgram(program_melt);
		glBindVertexArray(vao_melt);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_melt);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_melt);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case PIPELINE_CROSS:
		glUseProgram(program_cross);
		glBindVertexArray(vao_cross);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cross);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cross);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	default:
		assert(0);
		break;
	}

	/* Transfer the vertices */
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);

	/* Select textures. */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex1);
	if (src2_image != NULL) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex2);
	}

	/* Render primitives. */
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
}

/* Upload a texture. */
static void
update_texture_if_needed(
	struct hal_image *img)
{
	GLuint id;

	if (img == NULL)
		return;
	if (img->context == reinit_count && !is_after_reinit && !img->need_upload)
		return;

	if (img->context == reinit_count || is_after_reinit || img->texture == NULL) {
		glGenTextures(1, &id);
		img->texture = (void *)(intptr_t)(id + 1);
	} else {
		id = (GLuint)(intptr_t)img->texture - 1;
	}

	/* Create or update an OpenGL texture. */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, id);
#ifdef HAL_TARGET_WASM
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
	glActiveTexture(GL_TEXTURE0);

	img->need_upload = false;
	img->context = reinit_count;
}

/*
 * Set viewport.
 */
void opengl_set_screen(int x, int y, int w, int h)
{
#if !defined(HAL_USE_ROT90)
	glViewport(x, y, w, h);
#else
	glViewport(y, x, h, w);
#endif
}
