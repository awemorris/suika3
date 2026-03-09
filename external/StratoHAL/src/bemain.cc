/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Main code for BeOS and Haiku
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
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* HAL */
extern "C" {
#include "stratohal/platform.h"		/* Public Interface */
#include "stdfile.h"			/* Standard C File Implementation */
};

/* BeOS */
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Bitmap.h>
#include <SoundPlayer.h>

/* POSIX */
#include <sys/types.h>
#include <sys/stat.h>	/* stat(), mkdir() */
#include <sys/time.h>	/* gettimeofday() */
#include <unistd.h>	/* usleep(), access() */

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

const uint32 kMsgUpdate = 'updt';

BBitmap* bitmap;
struct image* image;
char *window_title;
int window_width;
int window_height;

BSoundPlayer *sound_player[HAL_SOUND_TRACKS];
struct hal_wave *wave[HAL_SOUND_TRACKS];
bool is_finished[HAL_SOUND_TRACKS];

extern "C" void fill_buffer(void *cookie, void *buffer, size_t size, const media_raw_audio_format& format);

class NoctView : public BView
{
	public:
	NoctView(BRect frame) :
		BView(frame, "noctview", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		SetViewColor(255, 255, 255);
	}

	void Draw(BRect updateRect) override
	{
		clear_image(image, 0);

		on_event_frame();

		DrawBitmap(bitmap, BPoint(0, 0));
	}

	void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override
	{
		on_event_mouse_move((int)where.x, (int)where.y);
	}

	void MouseDown(BPoint where) override
	{
		on_event_mouse_move((int)where.x, (int)where.y);

		uint32 buttons = 0;
		if (Window()->CurrentMessage() &&
		    Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons) == B_OK) {
			if (buttons & B_PRIMARY_MOUSE_BUTTON)
				on_event_mouse_press(HAL_MOUSE_LEFT, (int)where.x, (int)where.y);
			if (buttons & B_SECONDARY_MOUSE_BUTTON)
				on_event_mouse_press(HAL_MOUSE_RIGHT, (int)where.x, (int)where.y);
		}
	}

	void MouseUp(BPoint where) override
	{
		on_event_mouse_move((int)where.x, (int)where.y);

		uint32 buttons = 0;
		if (Window()->CurrentMessage() &&
		    Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons) == B_OK) {
			if (buttons & B_PRIMARY_MOUSE_BUTTON)
				on_event_mouse_release(HAL_MOUSE_LEFT, (int)where.x, (int)where.y);
			if (buttons & B_SECONDARY_MOUSE_BUTTON)
				on_event_mouse_release(HAL_MOUSE_RIGHT, (int)where.x, (int)where.y);
		}
	}

	void KeyDown(const char* bytes, int32 numBytes) override
	{
		// TODO
	}

	void KeyUp(const char* bytes, int32 numBytes) override
	{
		// TODO
	}

	void MessageReceived(BMessage* message) override
	{
		if (message->what == kMsgUpdate) {
			Invalidate();
		} else {
			BView::MessageReceived(message);
		}
	}
};
		
class NoctWindow : public BWindow
{
	BMessageRunner *runner;

public:
	NoctWindow(const char *title, int width, int height) :
		BWindow(BRect(100, 100, width + 100, height + 100),
				title,
				B_TITLED_WINDOW,
				0)
	{
		bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
		create_image_with_pixels(width, height, (pixel_t*)bitmap->Bits(), &image);

		NoctView* view = new NoctView(Bounds());
		AddChild(view);

		runner = new BMessageRunner(view, new BMessage(kMsgUpdate), 33000);
	}

	~NoctWindow()
	{
		delete runner;
	}

	bool QuitRequested() override
	{
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
	}
};

class NoctApplication : public BApplication
{
public:
	NoctApplication() : BApplication("application/x-vnd.example.noctvm")
	{
	}

	void ReadyToRun() override {
		if (!init_file())
			exit(1);
		if (!on_event_boot(&window_title, &window_width, &window_height))
			exit(1);

		NoctWindow *window = new NoctWindow(window_title, window_width, window_height);
		window->Show();

#if 0
		media_raw_audio_format format = {
			44100.0, 2, media_raw_audio_format::B_AUDIO_SHORT, B_MEDIA_LITTLE_ENDIAN, 4
		};
		for (int i = 0; i < HAL_SOUND_TRACKS; i++) {
			sound_player[i] = new BSoundPlayer(&format, "SoundPlayer", fill_buffer, NULL, (void*)(intptr_t)i);
			sound_player[i]->Start();
		}
#endif

		if (!on_event_start())
			exit(1);
	}
};

int main(int argc, char *argv[])
{
	NoctApplication app;
	app.Run();
	return 0;
}

extern "C" {

void
hal_notify_image_update(
    struct hal_image *img)
{
}

void
hal_notify_image_free(
    struct hal_image *img)
{
}

void
hal_render_image_normal(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
	if (dst_width == -1)
		dst_width = src_image->width;
	if (dst_height == -1)
		dst_height = src_image->height;
	if (src_width == -1)
		src_width = src_image->width;
	if (src_height == -1)
		src_height = src_image->height;

	draw_image_alpha(image,
			 dst_left,
			 dst_top,
			 src_image,
			 src_width,
			 src_height,
			 src_left,
			 src_top,
			 alpha);
}

void
hal_render_image_add(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
}

void
hal_render_image_dim(
	int dst_left,			/* The X coordinate of the screen */
	int dst_top,			/* The Y coordinate of the screen */
	int dst_width,			/* The width of the destination rectangle */
	int dst_height,			/* The height of the destination rectangle */
	struct image *src_image,	/* [IN] The image to be rendered */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
}

void
hal_render_image_rule(
	struct image *src_img,		/* [IN] The source image */
	struct image *rule_img,		/* [IN] The rule image */
	int threshold)			/* The threshold (0 to 255) */
{
}

void
hal_render_image_melt(
	struct image *src_img,		/* [IN] The source image */
	struct image *rule_img,		/* [IN] The rule image */
	int progress)			/* The progress (0 to 255) */
{
}

void
hal_render_image_3d_normal(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
}

void
hal_render_image_3d_add(
	float x1,			/* x1 */
	float y1,			/* y1 */
	float x2,			/* x2 */
	float y2,			/* y2 */
	float x3,			/* x3 */
	float y3,			/* y3 */
	float x4,			/* x4 */
	float y4,			/* y4 */
	struct image *src_image,	/* [IN] The source image */
	int src_left,			/* The X coordinate of a source image */
	int src_top,			/* The Y coordinate of a source image */
	int src_width,			/* The width of the source rectangle */
	int src_height,			/* The height of the source rectangle */
	int alpha)			/* The alpha value (0 to 255) */
{
}

void
hal_reset_lap_timer(
    uint64_t *origin)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	*origin = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

uint64_t
hal_get_lap_timer_millisec(
    uint64_t *origin)
{
	struct timeval tv;
	uint64_t end;
	
	gettimeofday(&tv, NULL);

	end = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);

	return (uint64_t)(end - *origin);
}

bool
hal_play_video(
    const char *fname,	/* file name */
    bool is_skippable)	/* allow skip for a unseen video */
{
	return true;
}

void
hal_stop_video(void)
{
}

bool
hal_is_video_playing(void)
{
	return false;
}

bool
hal_is_full_screen_supported(void){
	return false;
}

bool
hal_is_full_screen_mode(void)
{
	return false;
}

void
hal_enter_full_screen_mode(void)
{
}

void
hal_leave_full_screen_mode(void)
{
}

bool
hal_make_save_directory(void)
{
	return true;
}

char *
hal_make_real_path(const char *fname)
{
	return strdup(fname);
}

bool
hal_log_info(const char *s, ...)
{
	return true;
}

bool
hal_log_warn(const char *s, ...)
{
	return true;
}

bool
hal_log_error(const char *s, ...)
{
	return true;
}

bool
hal_log_out_of_memory(void)
{
	return true;
}

const char *
hal_get_system_language(void)
{
	return "en";
}

void
hal_set_continuous_swipe_enabled(
    bool is_enabled)
{
}

/*
 * Sound
 */

void
fill_buffer(
    void *cookie,
    void *buffer,
    size_t size,
    const media_raw_audio_format& format)
{
	int n = (intptr_t)cookie;

	if (wave[n] == NULL)
		return;

	uint32_t *buf = (uint32_t*)buffer;
	memset(buffer, 0, size);
	int rsize = hal_get_wave_samples(wave[n], buf, size / 4);
	if (hal_is_wave_eos(wave[n])) {
		wave[n] = NULL;
		is_finished[n] = true;
	}
}

bool
hal_play_sound(
    int n,
    struct hal_wave *w)
{
	wave[n] = w;
	return true;
}

bool
hal_stop_sound(
    int n)
{
	wave[n] = NULL;
	return true;
}

bool
hal_set_sound_volume(
    int n,
    float vol)
{
	return true;
}

bool
hal_is_sound_finished(
    int n)
{
	return is_finished[n];
}

}; /* extern "C" */
