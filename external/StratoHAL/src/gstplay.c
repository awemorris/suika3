/* -*- c-basic-offset: 2; indent-tabs-mode: nil; -*- */

/*
 * Playfield Engine
 * Video playback HAL for GStreamer on X11
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

#if defined(HAL_USE_GSTREAMER)

#include "gstplay.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#pragma GCC diagnostic pop

#include <stdio.h>

static GstElement * pipeline;
static guintptr video_window_handle;
static _Bool is_eos;

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data);

static void
cb_new_pad (GstElement *element, GstPad *pad, gpointer data);

static
gboolean bus_call (G_GNUC_UNUSED GstBus *bus, GstMessage *msg, gpointer data);

void
gstplay_init (int argc, char *argv[])
{
  gst_init (&argc, &argv);
}

void
gstplay_play(const char *fname, Window window)
{
  (void)fname;

  GMainLoop * loop;
  GstElement * src, * dec, * vconv, * aconv, * vsink, * asink;
  GstBus * bus;

  video_window_handle = window;

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("xvoverlay");
  src = gst_element_factory_make ("filesrc", "fs");
  dec = gst_element_factory_make ("decodebin", "dec");
  vconv = gst_element_factory_make ("videoconvert", "vconv");
  aconv = gst_element_factory_make ("audioconvert", "aconv");
  vsink = gst_element_factory_make ("xvimagesink", NULL);
  asink = gst_element_factory_make ("alsasink", NULL);
  if (src == NULL || dec == NULL || vconv == NULL || aconv == NULL ||
      vsink == NULL || asink == NULL)
    return;

  gst_bin_add_many (GST_BIN (pipeline), src, dec, vconv, aconv, vsink, asink,
                    NULL);
  gst_element_link (src, dec);
  g_signal_connect (dec, "pad-added", G_CALLBACK (cb_new_pad), vconv);
  g_signal_connect (dec, "pad-added", G_CALLBACK (cb_new_pad), aconv);
  gst_element_link (vconv, vsink);
  gst_element_link (aconv, asink);

  g_object_set(src, "location", fname, NULL);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, NULL,
                            NULL);
  gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  is_eos = False;
}

static void
cb_new_pad (GstElement *element, GstPad *pad, gpointer data)
{
  (void) pad;
  gst_element_link(element, (GstElement *) data);
}

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
  (void)bus;
  (void)user_data;

  if (!gst_is_video_overlay_prepare_window_handle_message (message))
    return GST_BUS_PASS;

  if (video_window_handle != 0) {
    gst_video_overlay_set_window_handle (
      GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message)),
      video_window_handle);
  }

  gst_message_unref (message);
  return GST_BUS_DROP;
}

static
gboolean bus_call (G_GNUC_UNUSED GstBus *bus, GstMessage *msg, gpointer data)
{
  (void) data;

  switch (GST_MESSAGE_TYPE (msg)) {
  case GST_MESSAGE_EOS:
    is_eos = True;
    break;
  default:
    break;
  }

  return TRUE;
}

void
gstplay_stop (void)
{
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  pipeline = NULL;
}

int
gstplay_is_playing (void)
{
  return !is_eos;
}

void
gstplay_loop_iteration (void)
{
  g_main_context_iteration (g_main_context_default(), False);
}

#else /* #ifndef NO_GST */

#include "stratohal/c89compat.h"
#include <X11/Xlib.h>

void
gstplay_init (int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
}

void
gstplay_play (const char *fname, Window window)
{
  UNUSED_PARAMETER(fname);
  UNUSED_PARAMETER(window);
}

void
gstplay_stop (void)
{
}

int
gstplay_is_playing (void)
{
  return 0;
}

void
gstplay_loop_iteration (void)
{
}

#endif /* defined(HAL_USE_GSTREAMER) */
