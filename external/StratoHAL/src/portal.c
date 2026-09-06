/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * XDG Portal
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
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <gio/gio.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

static GMainLoop *g_loop;
static char *g_selected_path;

static void
on_response(
	GDBusConnection *conn,
	const char	*sender_name,
	const char	*object_path,
	const char	*interface_name,
	const char	*signal_name,
	GVariant	*parameters,
	gpointer	 user_data);

int
open_portal_and_set_work_dir(int argc, char *argv[])
{
	GDBusConnection *conn;
	GVariantBuilder options;
	GVariant *ret;
	GError *error = NULL;
	const char *handle;
	guint sub_id;
	gboolean ok = FALSE;

	(void)argc;
	(void)argv;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (conn == NULL) {
		if (error) g_error_free(error);
		return FALSE;
	}

	g_loop = g_main_loop_new(NULL, FALSE);
	g_selected_path = NULL;

	g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(FALSE));
	g_variant_builder_add(&options, "{sv}", "directory", g_variant_new_boolean(TRUE));
	g_variant_builder_add(&options, "{sv}", "modal", g_variant_new_boolean(TRUE));
	g_variant_builder_add(&options, "{sv}", "accept_label", g_variant_new_string("Open"));

#if 0
	if (argc >= 3) {
		char *dir = strdup(argv[2]);

		/* If the path is a .ray or .novel file, convert it to parent directory */
		char *ext = strrchr(dir, '.');
		if (ext != NULL &&
		    (strcmp(ext, ".ray") == 0 || strcmp(ext, ".novel") == 0)) {
			char *slash = strrchr(dir, '/');
			if (slash != NULL)
				*slash = '\0';
		}

		g_variant_builder_add(&options, "{sv}", "current_folder", g_variant_new_bytestring(dir));
	}
#endif

	ret = g_dbus_connection_call_sync(
		conn,
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.FileChooser",
		"OpenFile",
		g_variant_new(
			"(ssa{sv})",
			"",		    /* parent_window */
			"Open Suika3 Project",
			&options),
		G_VARIANT_TYPE("(o)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		&error);

	if (ret == NULL) {
		if (error) g_error_free(error);
		goto cleanup;
	}

	g_variant_get(ret, "(&o)", &handle);

	sub_id = g_dbus_connection_signal_subscribe(
		conn,
		"org.freedesktop.portal.Desktop",
		"org.freedesktop.portal.Request",
		"Response",
		handle,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		on_response,
		NULL,
		NULL);

	g_main_loop_run(g_loop);

	g_dbus_connection_signal_unsubscribe(conn, sub_id);
	g_variant_unref(ret);

	if (g_selected_path == NULL)
		goto cleanup;

	if (g_file_test(g_selected_path, G_FILE_TEST_IS_DIR)) {
		ok = chdir(g_selected_path) == 0;
	} else {
		char *tmp = g_strdup(g_selected_path);
		char *dir = dirname(tmp);
		ok = chdir(dir) == 0;
		g_free(tmp);
	}

cleanup:
	g_clear_pointer(&g_selected_path, g_free);
	if (g_loop)
		g_main_loop_unref(g_loop);
	g_object_unref(conn);
	return ok;
}

static void
on_response(
	GDBusConnection *conn,
	const char	*sender_name,
	const char	*object_path,
	const char	*interface_name,
	const char	*signal_name,
	GVariant	*parameters,
	gpointer	 user_data)
{
	guint32 response;
	GVariant *results;
	GVariant *uris;
	const char *uri;

	(void)conn;
	(void)sender_name;
	(void)object_path;
	(void)interface_name;
	(void)signal_name;
	(void)parameters;
	(void)user_data;

	g_variant_get(parameters, "(u@a{sv})", &response, &results);

	if (response != 0) {
		g_variant_unref(results);
		g_main_loop_quit(g_loop);
		return;
	}

	uris = g_variant_lookup_value(results, "uris", G_VARIANT_TYPE("as"));
	if (uris != NULL && g_variant_n_children(uris) > 0) {
		g_variant_get_child(uris, 0, "&s", &uri);

		GError *error = NULL;
		g_selected_path = g_filename_from_uri(uri, NULL, &error);
		if (error != NULL) {
			g_error_free(error);
			g_selected_path = NULL;
		}

		g_variant_unref(uris);
	}

	g_variant_unref(results);
	g_main_loop_quit(g_loop);
}
