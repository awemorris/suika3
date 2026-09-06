/* -*- c-basic-offset: 2; indent-tabs-mode: nil; -*- */

/*
 * Strato HAL
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

/* StratoHAL */
#include <strato/strato.h>
#include "gstplay.h"

/* Gstreamer */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#pragma GCC diagnostic pop


/* Standard C */
#include <stdio.h>

static GstElement * pipeline;
static GstElement *vsink;
static GstElement *vconv;
static _Bool is_eos;
static struct hal_image *image;

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
gstplay_play(const char *fname)
{
  GMainLoop *loop;
  GstElement *src, *dec, *capsfilter, *aconv, *asink;
  GstCaps *caps;
  GstBus *bus;

  loop = g_main_loop_new(NULL, FALSE);

  pipeline = gst_pipeline_new("appsink-player");

  src = gst_element_factory_make("filesrc", "fs");
  dec = gst_element_factory_make("decodebin", "dec");
  vconv = gst_element_factory_make("videoconvert", "vconv");
  capsfilter = gst_element_factory_make("capsfilter", "vcaps");
  vsink = gst_element_factory_make("appsink", "vsink");
  aconv = gst_element_factory_make("audioconvert", "aconv");
  asink = gst_element_factory_make("autoaudiosink", NULL);
  if (src == NULL || dec == NULL || vconv == NULL ||
      capsfilter == NULL || vsink == NULL || aconv == NULL)
    return;

  caps = gst_caps_from_string("video/x-raw,format=RGBA");
  g_object_set(capsfilter, "caps", caps, NULL);
  gst_caps_unref(caps);

  g_object_set(vsink, "sync", TRUE, "max-buffers", 1, "drop", TRUE, NULL);

  if (asink != NULL)
    gst_bin_add_many(GST_BIN(pipeline), src, dec, vconv, capsfilter, vsink, aconv, asink, NULL);
  else
    gst_bin_add_many(GST_BIN(pipeline), src, dec, vconv, capsfilter, vsink, NULL);

  gst_element_link(src, dec);

  g_signal_connect(dec, "pad-added", G_CALLBACK(cb_new_pad), vconv);
  if (asink != NULL)
    g_signal_connect(dec, "pad-added", G_CALLBACK(cb_new_pad), aconv);

  gst_element_link_many(vconv, capsfilter, vsink, NULL);
  if (asink != NULL)
    gst_element_link(aconv, asink);

  g_object_set(src, "location", fname, NULL);

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  is_eos = false;
}

static void
cb_new_pad(GstElement *element, GstPad *pad, gpointer data)
{
  GstElement *target;
  GstPad *sinkpad;
  GstCaps *caps;
  const gchar *name;

  (void)element;

  target = (GstElement *)data;

  caps = gst_pad_get_current_caps(pad);
  if (caps == NULL)
    caps = gst_pad_query_caps(pad, NULL);
  if (caps == NULL)
    return;

  name = gst_structure_get_name(gst_caps_get_structure(caps, 0));

  if (g_str_has_prefix(name, "video/")) {
    if (target != vconv)
      goto cleanup;
  } else if (g_str_has_prefix(name, "audio/")) {
    if (target == vconv)
      goto cleanup;
  } else {
    goto cleanup;
  }

  sinkpad = gst_element_get_static_pad(target, "sink");
  if (sinkpad != NULL) {
    if (!gst_pad_is_linked(sinkpad))
      gst_pad_link(pad, sinkpad);
    gst_object_unref(sinkpad);
  }

cleanup:
  gst_caps_unref(caps);
}

static gboolean
bus_call(G_GNUC_UNUSED GstBus *bus, GstMessage *msg, gpointer data)
{
  (void)data;

  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_EOS:
    fprintf(stderr, "GStreamer: EOS\n");
    is_eos = true;
    break;

  case GST_MESSAGE_ERROR:
  {
    GError *err = NULL;
    gchar *debug = NULL;

    gst_message_parse_error(msg, &err, &debug);

    fprintf(stderr, "GStreamer ERROR: %s\n", err ? err->message : "(unknown)");
    if (debug != NULL)
      fprintf(stderr, "GStreamer DEBUG: %s\n", debug);

    if (err != NULL)
      g_error_free(err);
    g_free(debug);

    is_eos = true;
    break;
  }

  case GST_MESSAGE_WARNING:
  {
    GError *err = NULL;
    gchar *debug = NULL;

    gst_message_parse_warning(msg, &err, &debug);

    fprintf(stderr, "GStreamer WARNING: %s\n", err ? err->message : "(unknown)");
    if (debug != NULL)
      fprintf(stderr, "GStreamer DEBUG: %s\n", debug);

    if (err != NULL)
      g_error_free(err);
    g_free(debug);
    break;
  }

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

struct hal_image *
gstplay_loop_iteration(void)
{
  GstSample *sample;
  GstBuffer *buffer;
  GstCaps *caps;
  GstVideoInfo info;
  GstMapInfo map;
  int width, height, stride, y;

  g_main_context_iteration(g_main_context_default(), FALSE);

  /* Non-blocking. */
  sample = gst_app_sink_try_pull_sample(GST_APP_SINK(vsink), 0);
  if (sample == NULL)
    return NULL;

  caps = gst_sample_get_caps(sample);
  if (caps == NULL) {
    gst_sample_unref(sample);
    return NULL;
  }
  if (!gst_video_info_from_caps(
          &info,
          caps)) {
    gst_sample_unref(sample);
    return NULL;
  }

  width  = GST_VIDEO_INFO_WIDTH(&info);
  height = GST_VIDEO_INFO_HEIGHT(&info);
  stride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);

  if (image != NULL && (image->width != width || image->height != height)) {
    hal_destroy_image(image);
    image = NULL;
  }
  if (image == NULL) {
    hal_create_image(width, height, &image);
    if (image == NULL)
      return NULL;
  }

  buffer = gst_sample_get_buffer(sample);
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    for (y = 0; y < height; y++)
      memcpy(image->pixels + y * width, (const unsigned char *)map.data + y * stride, (size_t)width * 4);
    hal_notify_image_update(image);
    gst_buffer_unmap(buffer, &map);
  }

  gst_sample_unref(sample);

  return image;
}

#else /* #ifndef NO_GST */

#include <strato/c89compat.h>

void
gstplay_init (int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
}

void
gstplay_play (const char *fname)
{
  UNUSED_PARAMETER(fname);
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

struct hal_image *
gstplay_loop_iteration (void)
{
  return NULL;
}

#endif /* defined(HAL_USE_GSTREAMER) */
