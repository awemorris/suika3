/* -*- coding: utf-8; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Unity Plugin's C# part.
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

using System;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Video;

public class PlayfieldScript : MonoBehaviour
{
    //
    // For Rendering
    //
    private static int viewportWidth = 1280;
    private static int viewportHeight = 720;
    private Shader _normalShader;
    private Shader _addShader;
    private Shader _dimShader;
    private Shader _ruleShader;
    private Shader _meltShader;
    private CommandBuffer _commandBuffer;
    private GameObject _audio1;
    private GameObject _audio2;
    private GameObject _audio3;
    private GameObject _audio4;
	private VideoPlayer _videoPlayer;
    private bool _isVideoPlaying;

    //
    // On awake
    //
    void Awake()
    {
    }

    //
    // On start
    //
    void Start()
    {
        // Save the instance.
        _instance = this;

        // Get the shaders.
        _normalShader = Resources.Load<Shader>("NormalShader");
        _addShader = Resources.Load<Shader>("AddShader");
        _dimShader = Resources.Load<Shader>("DimShader");
        _ruleShader = Resources.Load<Shader>("RuleShader");
        _meltShader = Resources.Load<Shader>("MeltShader");

        // Make a command buffer.
        _commandBuffer = new CommandBuffer();
        _commandBuffer.name = "FrameCommand";
        _commandBuffer.SetRenderTarget(BuiltinRenderTextureType.CameraTarget);
        _commandBuffer.SetViewport(new Rect(0, 0, viewportWidth, viewportHeight));
        _commandBuffer.SetViewMatrix(Matrix4x4.TRS(new Vector3(-1f, -1f, 0), Quaternion.identity, new Vector3(2f / viewportWidth, 2f / viewportHeight, 1f)));
        _commandBuffer.ClearRenderTarget(true, true, Color.black);

        // Make a camera.
        Camera camera = Camera.main;
        camera.clearFlags = CameraClearFlags.SolidColor;
        camera.backgroundColor = new Color(0, 0, 0, 0);
        camera.AddCommandBuffer(CameraEvent.BeforeForwardAlpha, _commandBuffer);

		// Make sound streams.
		_audio1 = new GameObject("audio1", typeof(PlayfieldAudio));
		_audio2 = new GameObject("audio2", typeof(PlayfieldAudio));
		_audio3 = new GameObject("audio3", typeof(PlayfieldAudio));
		_audio4 = new GameObject("audio4", typeof(PlayfieldAudio));

        InitNativeCode();
    }

    //
    // On frame update
    //
    unsafe void Update()
    {
        int KEY_CONTROL = 0;
        int KEY_SPACE = 1;
        int KEY_RETURN = 2;
        int KEY_UP = 3;
        int KEY_DOWN = 4;
        int KEY_LEFT = 5;
        int KEY_RIGHT = 6;
        int KEY_ESCAPE = 7;
		int BUTTON_LEFT = 0;
		int BUTTON_RIGHT = 1;

        // Process mouse input.
		int posX = (int)Input.mousePosition.x;
		int posY = viewportHeight - (int)Input.mousePosition.y;
		on_event_mouse_move(posX, posY);
		if (Input.GetMouseButtonDown(0))
		   on_event_mouse_press(BUTTON_LEFT, posX, posY);
		if (Input.GetMouseButtonUp(0))
		   on_event_mouse_release(BUTTON_LEFT,posX, posY);
		if (Input.GetMouseButtonDown(1))
		   on_event_mouse_press(BUTTON_RIGHT, posX, posY);
		if (Input.GetMouseButtonUp(1))
		   on_event_mouse_release(BUTTON_RIGHT, posX, posY);
		if (Input.GetMouseButtonUp(2))
		{
		   on_event_key_press(KEY_UP);
		   on_event_key_release(KEY_UP);
		}
		if (Input.GetMouseButtonDown(2))
		{
		   on_event_key_press(KEY_DOWN);
		   on_event_key_release(KEY_DOWN);
		}
		   
        // Process key down.
        if (Input.GetKeyDown(KeyCode.LeftControl))
            on_event_key_press(KEY_CONTROL);
        if (Input.GetKeyDown(KeyCode.Space))
            on_event_key_press(KEY_SPACE);
        if (Input.GetKeyDown(KeyCode.Return))
            on_event_key_press(KEY_RETURN);
        if (Input.GetKeyDown(KeyCode.UpArrow))
            on_event_key_press(KEY_UP);
        if (Input.GetKeyDown(KeyCode.DownArrow))
            on_event_key_press(KEY_DOWN);
        if (Input.GetKeyDown(KeyCode.LeftArrow))
            on_event_key_press(KEY_LEFT);
        if (Input.GetKeyDown(KeyCode.RightArrow))
            on_event_key_press(KEY_RIGHT);
        if (Input.GetKeyDown(KeyCode.Escape))
            on_event_key_press(KEY_ESCAPE);

        // Process key up.
        if (Input.GetKeyUp(KeyCode.LeftControl))
            on_event_key_release(KEY_CONTROL);
        if (Input.GetKeyUp(KeyCode.Space))
            on_event_key_release(KEY_SPACE);
        if (Input.GetKeyUp(KeyCode.Return))
            on_event_key_release(KEY_RETURN);
        if (Input.GetKeyUp(KeyCode.UpArrow))
            on_event_key_release(KEY_UP);
        if (Input.GetKeyUp(KeyCode.UpArrow))
            on_event_key_release(KEY_UP);
        if (Input.GetKeyUp(KeyCode.LeftArrow))
            on_event_key_release(KEY_LEFT);
        if (Input.GetKeyUp(KeyCode.RightArrow))
            on_event_key_release(KEY_RIGHT);
        if (Input.GetKeyUp(KeyCode.Escape))
            on_event_key_release(KEY_ESCAPE);

        // Do a frame rendering.
        _commandBuffer.Clear();
        if (on_event_frame() == 0)
        {
            // Exit on finish or error.
			Destroy(this);
        }
    }

    //
    // ===================================================================
    // Following code is for bridging C# and C.
    // For experts only.
    // ===================================================================
    //

    //
    // The Sole Instance
    //
    private static PlayfieldScript _instance;

    //
    // Delegate types for calling C# from C.
    //
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_log_info(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_log_warn(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_log_error(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_log_out_of_memory();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_make_save_directory();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_make_real_path(byte* fname, byte *dst, int len);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_notify_image_update(int id, int width, int height, uint *pixels);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_notify_image_free(int id);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_normal(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_add(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_sub(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_dim(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_rule(int src_img, int rule_img, int threshold);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_melt(int src_img, int rule_img, int progress);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_3d_normal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_3d_add(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_3d_sub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_render_image_3d_dim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_reset_lap_timer(IntPtr origin);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate Int64 delegate_get_lap_timer_millisec(IntPtr origin);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_play_sound(int stream, byte *wave);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_stop_sound(int stream);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_set_sound_volume(int stream, float vol);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_is_sound_finished(int stream);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_play_video(byte *fname, int is_skippable);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_stop_video();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_is_video_playing();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_is_full_screen_supported();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_is_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_enter_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_leave_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate IntPtr delegate_get_system_language();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_set_continuous_swipe_enabled(int is_enabled);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_free_shared(IntPtr p);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_check_file_exist(byte *pFileName);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate IntPtr delegate_get_file_contents(byte *pFileName, int *len);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_open_save_file(byte *pFileName);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_write_save_file(int b);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_close_save_file();

    //
    // Delegate types for calling C from C#.
    //
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    unsafe delegate void delegate_init_hal_func_table(IntPtr log_info,
                                                      IntPtr log_warn,
                                                      IntPtr log_error,
                                                      IntPtr log_out_of_memory,
                                                      IntPtr make_save_directory,
                                                      IntPtr make_real_path,
                                                      IntPtr notify_image_update,
                                                      IntPtr notify_image_free,
                                                      IntPtr render_image_normal,
                                                      IntPtr render_image_add,
                                                      IntPtr render_image_sub,
                                                      IntPtr render_image_dim,
                                                      IntPtr render_image_rule,
                                                      IntPtr render_image_melt,
                                                      IntPtr render_image_3d_normal,
                                                      IntPtr render_image_3d_add,
                                                      IntPtr render_image_3d_sub,
                                                      IntPtr render_image_3d_dim,
                                                      IntPtr reset_lap_timer,
                                                      IntPtr get_lap_timer_millisec,
                                                      IntPtr play_sound,
                                                      IntPtr stop_sound,
                                                      IntPtr set_sound_volume,
                                                      IntPtr is_sound_finished,
                                                      IntPtr play_video,
                                                      IntPtr stop_video,
                                                      IntPtr is_video_playing,
                                                      IntPtr is_full_screen_supported,
                                                      IntPtr is_full_screen_mode,
                                                      IntPtr enter_full_screen_mode,
                                                      IntPtr leave_full_screen_mode,
                                                      IntPtr get_system_language,
                                                      IntPtr set_continuous_swipe_enabled,
                                                      IntPtr free_shared,
                                                      IntPtr check_file_exist,
                                                      IntPtr get_file_contents,
                                                      IntPtr open_save_file,
                                                      IntPtr write_save_file,
                                                      IntPtr close_save_file);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_boot(byte *dummy1, byte *dummy2, byte *dummy3);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_start();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_frame();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_key_press(int key);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_key_release(int key);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_press(int button, int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_release(int button, int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_move(int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_touch_cancel();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_swipe_down();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_swipe_up();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_get_wave_samples(byte* w, uint* buf, int samples);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate bool delegate_is_wave_eos(byte* w);

    //
    // Delegate objects for calling C# from C.
    //
    static delegate_log_info d_log_info;
    static delegate_log_warn d_log_warn;
    static delegate_log_error d_log_error;
    static delegate_log_out_of_memory d_log_out_of_memory;
    static delegate_make_save_directory d_make_save_directory;
    static delegate_make_real_path d_make_real_path;
    static delegate_notify_image_update d_notify_image_update;
    static delegate_notify_image_free d_notify_image_free;
    static delegate_render_image_normal d_render_image_normal;
    static delegate_render_image_add d_render_image_add;
    static delegate_render_image_add d_render_image_sub;
    static delegate_render_image_dim d_render_image_dim;
    static delegate_render_image_rule d_render_image_rule;
    static delegate_render_image_melt d_render_image_melt;
    static delegate_render_image_3d_normal d_render_image_3d_normal;
    static delegate_render_image_3d_add d_render_image_3d_add;
    static delegate_render_image_3d_sub d_render_image_3d_sub;
    static delegate_render_image_3d_dim d_render_image_3d_dim
    static delegate_reset_lap_timer d_reset_lap_timer;
    static delegate_get_lap_timer_millisec d_get_lap_timer_millisec;
    static delegate_play_sound d_play_sound;
    static delegate_stop_sound d_stop_sound;
    static delegate_set_sound_volume d_set_sound_volume;
    static delegate_is_sound_finished d_is_sound_finished;
    static delegate_play_video d_play_video;
    static delegate_stop_video d_stop_video;
    static delegate_is_video_playing d_is_video_playing;
    static delegate_is_full_screen_supported d_is_full_screen_supported;
    static delegate_is_full_screen_mode d_is_full_screen_mode;
    static delegate_enter_full_screen_mode d_enter_full_screen_mode;
    static delegate_leave_full_screen_mode d_leave_full_screen_mode;
    static delegate_get_system_language d_get_system_language;
    static delegate_set_continuous_swipe_enabled d_set_continuous_swipe_enabled;
    static delegate_free_shared d_free_shared;
    static delegate_check_file_exist d_check_file_exist;
    static delegate_get_file_contents d_get_file_contents;
    static delegate_open_save_file d_open_save_file;
    static delegate_write_save_file d_write_save_file;
    static delegate_close_save_file d_close_save_file;

	//
    // Delegate objects for calling C from C#.
	//
    static delegate_init_hal_func_table d_init_hal_func_table;
    static delegate_on_event_boot d_on_event_boot;
    static delegate_on_event_start d_on_event_start;
    static delegate_on_event_frame d_on_event_frame;
    static delegate_on_event_key_press d_on_event_key_press;
    static delegate_on_event_key_release d_on_event_key_release;
    static delegate_on_event_mouse_press d_on_event_mouse_press;
    static delegate_on_event_mouse_release d_on_event_mouse_release;
    static delegate_on_event_mouse_move d_on_event_mouse_move;
    static delegate_on_event_touch_cancel d_on_event_touch_cancel;
    static delegate_on_event_swipe_down d_on_event_swipe_down;
    static delegate_on_event_swipe_up d_on_event_swipe_up;
    static delegate_get_wave_samples d_get_wave_samples;
    static delegate_is_wave_eos d_is_wave_eos;

    //
    // Delegate pointers for calling C# from C.
    //
    static IntPtr p_log_info;
    static IntPtr p_log_warn;
    static IntPtr p_log_error;
    static IntPtr p_log_out_of_memory;
    static IntPtr p_make_save_directory;
    static IntPtr p_make_real_path;
    static IntPtr p_notify_image_update;
    static IntPtr p_notify_image_free;
    static IntPtr p_render_image_normal;
    static IntPtr p_render_image_add;
    static IntPtr p_render_image_sub;
    static IntPtr p_render_image_dim;
    static IntPtr p_render_image_rule;
    static IntPtr p_render_image_melt;
    static IntPtr p_render_image_3d_normal;
    static IntPtr p_render_image_3d_add;
    static IntPtr p_render_image_3d_sub;
    static IntPtr p_render_image_3d_dim;
    static IntPtr p_reset_lap_timer;
    static IntPtr p_get_lap_timer_millisec;
    static IntPtr p_play_sound;
    static IntPtr p_stop_sound;
    static IntPtr p_set_sound_volume;
    static IntPtr p_is_sound_finished;
    static IntPtr p_play_video;
    static IntPtr p_stop_video;
    static IntPtr p_is_video_playing;
    static IntPtr p_is_full_screen_supported;
    static IntPtr p_is_full_screen_mode;
    static IntPtr p_enter_full_screen_mode;
    static IntPtr p_leave_full_screen_mode;
    static IntPtr p_get_system_language;
    static IntPtr p_set_continuous_swipe_enabled;
    static IntPtr p_free_shared;
    static IntPtr p_check_file_exist;
    static IntPtr p_get_file_contents;
    static IntPtr p_open_save_file;
    static IntPtr p_write_save_file;
    static IntPtr p_close_save_file;

    //
    // Delegate pointers for calling C from C#.
    //
    static IntPtr p_init_hal_func_table;
    static IntPtr p_on_event_boot;
    static IntPtr p_on_event_start;
    static IntPtr p_on_event_frame;
    static IntPtr p_on_event_key_press;
    static IntPtr p_on_event_key_release;
    static IntPtr p_on_event_mouse_press;
    static IntPtr p_on_event_mouse_release;
    static IntPtr p_on_event_mouse_move;
    static IntPtr p_on_event_touch_cancel;
    static IntPtr p_on_event_swipe_down;
    static IntPtr p_on_event_swipe_up;
    static IntPtr p_get_wave_samples;
    static IntPtr p_is_wave_eos;

    //
    // C# Image structure.
    //
    public struct ManagedImage {
        public int width;
        public int height;
        public Color[] pixels;
        public Texture2D texture;
        public bool need_upload;
    };

    //
    // Image lists.
    //
    private static Dictionary<int, ManagedImage> imageDict = new Dictionary<int, ManagedImage>();

    //
    // Initialize the calling bridges on loading.
    //
    unsafe void InitNativeCode()
    {
        GC.KeepAlive(this);

        // Set delegate objects. (calling C# from C)
        d_log_info = new delegate_log_info(log_info);
        d_log_warn = new delegate_log_warn(log_warn);
        d_log_error = new delegate_log_error(log_error);
        d_log_out_of_memory = new delegate_log_out_of_memory(log_out_of_memory);
        d_make_save_directory = new delegate_make_save_directory(make_save_directory);
        d_make_real_path = new delegate_make_real_path(make_real_path);
        d_notify_image_update = new delegate_notify_image_update(notify_image_update);
        d_notify_image_free = new delegate_notify_image_free(notify_image_free);
        d_render_image_normal = new delegate_render_image_normal(render_image_normal);
        d_render_image_add = new delegate_render_image_add(render_image_add);
        d_render_image_sub = new delegate_render_image_sub(render_image_sub);
        d_render_image_dim = new delegate_render_image_dim(render_image_dim);
        d_render_image_rule = new delegate_render_image_rule(render_image_rule);
        d_render_image_melt = new delegate_render_image_melt(render_image_melt);
        d_render_image_3d_normal = new delegate_render_image_3d_normal(render_image_3d_normal);
        d_render_image_3d_add = new delegate_render_image_3d_add(render_image_3d_add);
        d_render_image_3d_sub = new delegate_render_image_3d_sub(render_image_3d_sub);
        d_render_image_3d_dim = new delegate_render_image_3d_dim(render_image_3d_dim);
        d_reset_lap_timer = new delegate_reset_lap_timer(reset_lap_timer);
        d_get_lap_timer_millisec = new delegate_get_lap_timer_millisec(get_lap_timer_millisec);
        d_play_sound = new delegate_play_sound(play_sound);
        d_stop_sound = new delegate_stop_sound(stop_sound);
        d_set_sound_volume = new delegate_set_sound_volume(set_sound_volume);
        d_is_sound_finished = new delegate_is_sound_finished(is_sound_finished);
        d_play_video = new delegate_play_video(play_video);
        d_stop_video = new delegate_stop_video(stop_video);
        d_is_video_playing = new delegate_is_video_playing(is_video_playing);
        d_is_full_screen_supported = new delegate_is_full_screen_supported(is_full_screen_supported);
        d_is_full_screen_mode = new delegate_is_full_screen_mode(is_full_screen_supported);
        d_enter_full_screen_mode = new delegate_enter_full_screen_mode(enter_full_screen_mode);
        d_leave_full_screen_mode = new delegate_leave_full_screen_mode(leave_full_screen_mode);
        d_get_system_language = new delegate_get_system_language(get_system_language);
        d_set_continuous_swipe_enabled = new delegate_set_continuous_swipe_enabled(set_continuous_swipe_enabled);
        d_free_shared = new delegate_free_shared(free_shared);
        d_check_file_exist = new delegate_check_file_exist(check_file_exist);
        d_get_file_contents = new delegate_get_file_contents(get_file_contents);
        d_open_save_file = new delegate_open_save_file(open_save_file);
        d_write_save_file = new delegate_write_save_file(write_save_file);
        d_close_save_file = new delegate_close_save_file(close_save_file);

		// Set delegate objects. (calling C from C#)
        d_init_hal_func_table = new delegate_init_hal_func_table(init_hal_func_table);
        d_on_event_boot = new delegate_on_event_boot(on_event_boot);
        d_on_event_start = new delegate_on_event_start(on_event_start);
        d_on_event_frame = new delegate_on_event_frame(on_event_frame);
        d_on_event_key_press = new delegate_on_event_key_press(on_event_key_press);
        d_on_event_key_release = new delegate_on_event_key_release(on_event_key_release);
        d_on_event_mouse_press = new delegate_on_event_mouse_press(on_event_mouse_press);
        d_on_event_mouse_release = new delegate_on_event_mouse_release(on_event_mouse_release);
        d_on_event_mouse_move = new delegate_on_event_mouse_move(on_event_mouse_move);
        d_on_event_touch_cancel = new delegate_on_event_touch_cancel(on_event_touch_cancel);
        d_on_event_swipe_down = new delegate_on_event_swipe_down(on_event_swipe_down);
        d_on_event_swipe_up = new delegate_on_event_swipe_up(on_event_swipe_up);
        d_get_wave_samples = new delegate_get_wave_samples(get_wave_samples);
        d_is_wave_eos = new delegate_is_wave_eos(is_wave_eos);

        // Get function pointers. (calling C# from C)
        p_log_info = Marshal.GetFunctionPointerForDelegate(d_log_info);
        p_log_warn = Marshal.GetFunctionPointerForDelegate(d_log_warn);
        p_log_error = Marshal.GetFunctionPointerForDelegate(d_log_error);
        p_log_out_of_memory = Marshal.GetFunctionPointerForDelegate(d_log_out_of_memory);
        p_make_save_directory = Marshal.GetFunctionPointerForDelegate(d_make_save_directory);
        p_make_real_path = Marshal.GetFunctionPointerForDelegate(d_make_real_path);
        p_notify_image_update = Marshal.GetFunctionPointerForDelegate(d_notify_image_update);
        p_notify_image_free = Marshal.GetFunctionPointerForDelegate(d_notify_image_free);
        p_render_image_normal = Marshal.GetFunctionPointerForDelegate(d_render_image_normal);
        p_render_image_add = Marshal.GetFunctionPointerForDelegate(d_render_image_add);
        p_render_image_sub = Marshal.GetFunctionPointerForDelegate(d_render_image_sub);
        p_render_image_dim = Marshal.GetFunctionPointerForDelegate(d_render_image_dim);
        p_render_image_rule = Marshal.GetFunctionPointerForDelegate(d_render_image_rule);
        p_render_image_melt = Marshal.GetFunctionPointerForDelegate(d_render_image_melt);
        p_render_image_3d_normal = Marshal.GetFunctionPointerForDelegate(d_render_image_3d_normal);
        p_render_image_3d_add = Marshal.GetFunctionPointerForDelegate(d_render_image_3d_add);
        p_render_image_3d_sub = Marshal.GetFunctionPointerForDelegate(d_render_image_3d_sub);
        p_render_image_3d_dim = Marshal.GetFunctionPointerForDelegate(d_render_image_3d_dim);
        p_reset_lap_timer = Marshal.GetFunctionPointerForDelegate(d_reset_lap_timer);
        p_get_lap_timer_millisec = Marshal.GetFunctionPointerForDelegate(d_get_lap_timer_millisec);
        p_play_sound = Marshal.GetFunctionPointerForDelegate(d_play_sound);
        p_stop_sound = Marshal.GetFunctionPointerForDelegate(d_stop_sound);
        p_set_sound_volume = Marshal.GetFunctionPointerForDelegate(d_set_sound_volume);
        p_is_sound_finished = Marshal.GetFunctionPointerForDelegate(d_is_sound_finished);
        p_play_video = Marshal.GetFunctionPointerForDelegate(d_play_video);
        p_stop_video = Marshal.GetFunctionPointerForDelegate(d_stop_video);
        p_is_video_playing = Marshal.GetFunctionPointerForDelegate(d_is_video_playing);
        p_is_full_screen_supported = Marshal.GetFunctionPointerForDelegate(d_is_full_screen_supported);
        p_is_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_is_full_screen_mode);
        p_enter_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_enter_full_screen_mode);
        p_leave_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_leave_full_screen_mode);
        p_get_system_language = Marshal.GetFunctionPointerForDelegate(d_get_system_language);
        p_set_continuous_swipe_enabled = Marshal.GetFunctionPointerForDelegate(d_set_continuous_swipe_enabled);
        p_free_shared = Marshal.GetFunctionPointerForDelegate(d_free_shared);
        p_check_file_exist = Marshal.GetFunctionPointerForDelegate(d_check_file_exist);
        p_get_file_contents = Marshal.GetFunctionPointerForDelegate(d_get_file_contents);
        p_open_save_file = Marshal.GetFunctionPointerForDelegate(d_open_save_file);
        p_write_save_file = Marshal.GetFunctionPointerForDelegate(d_write_save_file);
        p_close_save_file = Marshal.GetFunctionPointerForDelegate(d_close_save_file);

        // Get function pointers. (calling C from C#)
        p_init_hal_func_table = Marshal.GetFunctionPointerForDelegate(d_init_hal_func_table);
        p_on_event_boot = Marshal.GetFunctionPointerForDelegate(d_on_event_boot);
        p_on_event_start = Marshal.GetFunctionPointerForDelegate(d_on_event_start);
        p_on_event_frame = Marshal.GetFunctionPointerForDelegate(d_on_event_frame);
        p_on_event_key_press = Marshal.GetFunctionPointerForDelegate(d_on_event_key_press);
        p_on_event_key_release = Marshal.GetFunctionPointerForDelegate(d_on_event_key_release);
        p_on_event_mouse_press = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_press);
        p_on_event_mouse_release = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_release);
        p_on_event_mouse_move = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_move);
        p_on_event_touch_cancel = Marshal.GetFunctionPointerForDelegate(d_on_event_touch_cancel);
        p_on_event_swipe_down = Marshal.GetFunctionPointerForDelegate(d_on_event_swipe_down);
        p_on_event_swipe_up = Marshal.GetFunctionPointerForDelegate(d_on_event_swipe_up);
        p_get_wave_samples = Marshal.GetFunctionPointerForDelegate(d_get_wave_samples);
        p_is_wave_eos = Marshal.GetFunctionPointerForDelegate(d_is_wave_eos);

        // Lock function pointers by Alloc(). (calling C# from C)
        GCHandle.Alloc(p_log_info, GCHandleType.Pinned);
        GCHandle.Alloc(p_log_warn, GCHandleType.Pinned);
        GCHandle.Alloc(p_log_error, GCHandleType.Pinned);
        GCHandle.Alloc(p_log_out_of_memory, GCHandleType.Pinned);
        GCHandle.Alloc(p_make_save_directory, GCHandleType.Pinned);
        GCHandle.Alloc(p_make_real_path, GCHandleType.Pinned);
        GCHandle.Alloc(p_notify_image_update, GCHandleType.Pinned);
        GCHandle.Alloc(p_notify_image_free, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_normal, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_add, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_sub, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_dim, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_rule, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_melt, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_3d_normal, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_3d_add, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_3d_sub, GCHandleType.Pinned);
        GCHandle.Alloc(p_render_image_3d_dim, GCHandleType.Pinned);
        GCHandle.Alloc(p_reset_lap_timer, GCHandleType.Pinned);
        GCHandle.Alloc(p_get_lap_timer_millisec, GCHandleType.Pinned);
        GCHandle.Alloc(p_play_sound, GCHandleType.Pinned);
        GCHandle.Alloc(p_stop_sound, GCHandleType.Pinned);
        GCHandle.Alloc(p_set_sound_volume, GCHandleType.Pinned);
        GCHandle.Alloc(p_is_sound_finished, GCHandleType.Pinned);
        GCHandle.Alloc(p_play_video, GCHandleType.Pinned);
        GCHandle.Alloc(p_stop_video, GCHandleType.Pinned);
        GCHandle.Alloc(p_is_video_playing, GCHandleType.Pinned);
        GCHandle.Alloc(p_is_full_screen_supported, GCHandleType.Pinned);
        GCHandle.Alloc(p_is_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_enter_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_leave_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_get_system_language, GCHandleType.Pinned);
        GCHandle.Alloc(p_set_continuous_swipe_enabled, GCHandleType.Pinned);
        GCHandle.Alloc(p_free_shared, GCHandleType.Pinned);
        GCHandle.Alloc(p_check_file_exist, GCHandleType.Pinned);
        GCHandle.Alloc(p_get_file_contents, GCHandleType.Pinned);
        GCHandle.Alloc(p_open_save_file, GCHandleType.Pinned);
        GCHandle.Alloc(p_write_save_file, GCHandleType.Pinned);
        GCHandle.Alloc(p_close_save_file, GCHandleType.Pinned);

		// Lock function pointers by Alloc(). (calling C from C#)
        GCHandle.Alloc(p_init_hal_func_table, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_boot, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_start, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_frame, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_key_press, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_key_release, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_press, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_release, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_move, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_touch_cancel, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_swipe_down, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_swipe_up, GCHandleType.Pinned);
        GCHandle.Alloc(p_get_wave_samples, GCHandleType.Pinned);
        GCHandle.Alloc(p_is_wave_eos, GCHandleType.Pinned);

        // Lock function pointers by KeepAlive(). (calling C# from C)
        GC.KeepAlive(p_log_info);
        GC.KeepAlive(p_log_warn);
        GC.KeepAlive(p_log_error);
        GC.KeepAlive(p_log_out_of_memory);
        GC.KeepAlive(p_make_save_directory);
        GC.KeepAlive(p_make_real_path);
        GC.KeepAlive(p_notify_image_update);
        GC.KeepAlive(p_notify_image_free);
        GC.KeepAlive(p_render_image_normal);
        GC.KeepAlive(p_render_image_add);
        GC.KeepAlive(p_render_image_sub);
        GC.KeepAlive(p_render_image_dim);
        GC.KeepAlive(p_render_image_rule);
        GC.KeepAlive(p_render_image_melt);
        GC.KeepAlive(p_render_image_3d_normal);
        GC.KeepAlive(p_render_image_3d_add);
        GC.KeepAlive(p_render_image_3d_sub);
        GC.KeepAlive(p_render_image_3d_dim);
        GC.KeepAlive(p_reset_lap_timer);
        GC.KeepAlive(p_get_lap_timer_millisec);
        GC.KeepAlive(p_play_sound);
        GC.KeepAlive(p_stop_sound);
        GC.KeepAlive(p_set_sound_volume);
        GC.KeepAlive(p_is_sound_finished);
        GC.KeepAlive(p_play_video);
        GC.KeepAlive(p_stop_video);
        GC.KeepAlive(p_is_video_playing);
        GC.KeepAlive(p_is_full_screen_supported);
        GC.KeepAlive(p_is_full_screen_mode);
        GC.KeepAlive(p_enter_full_screen_mode);
        GC.KeepAlive(p_leave_full_screen_mode);
        GC.KeepAlive(p_get_system_language);
        GC.KeepAlive(p_set_continuous_swipe_enabled);
        GC.KeepAlive(p_free_shared);
        GC.KeepAlive(p_check_file_exist);
        GC.KeepAlive(p_get_file_contents);
        GC.KeepAlive(p_open_save_file);
        GC.KeepAlive(p_write_save_file);
        GC.KeepAlive(p_close_save_file);

        // Lock function pointers by KeepAlive(). (calling C from C#)
        GC.KeepAlive(p_init_hal_func_table);
        GC.KeepAlive(p_on_event_boot);
        GC.KeepAlive(p_on_event_start);
        GC.KeepAlive(p_on_event_frame);
        GC.KeepAlive(p_on_event_key_press);
        GC.KeepAlive(p_on_event_key_release);
        GC.KeepAlive(p_on_event_mouse_press);
        GC.KeepAlive(p_on_event_mouse_release);
        GC.KeepAlive(p_on_event_mouse_move);
        GC.KeepAlive(p_on_event_touch_cancel);
        GC.KeepAlive(p_on_event_swipe_down);
        GC.KeepAlive(p_on_event_swipe_up);
        GC.KeepAlive(p_get_wave_samples);
        GC.KeepAlive(p_is_wave_eos);

        // Call init_hal_func_table() to initialize the HAL function table.
        d_init_hal_func_table(
            p_log_info,
            p_log_warn,
            p_log_error,
            p_log_out_of_memory,
            p_make_save_directory,
            p_make_real_path,
            p_notify_image_update,
            p_notify_image_free,
            p_render_image_normal,
            p_render_image_add,
            p_render_image_sub,
            p_render_image_dim,
            p_render_image_rule,
            p_render_image_melt,
            p_render_image_3d_normal,
            p_render_image_3d_add,
            p_render_image_3d_sub,
            p_render_image_3d_dim,
            p_reset_lap_timer,
            p_get_lap_timer_millisec,
            p_play_sound,
            p_stop_sound,
            p_set_sound_volume,
            p_is_sound_finished,
            p_play_video,
            p_stop_video,
            p_is_video_playing,
            p_is_full_screen_supported,
            p_is_full_screen_mode,
            p_enter_full_screen_mode,
            p_leave_full_screen_mode,
            p_get_system_language,
            p_set_continuous_swipe_enabled,
            p_free_shared,
            p_check_file_exist,
            p_get_file_contents,
            p_open_save_file,
            p_write_save_file,
            p_close_save_file);
        GC.KeepAlive(this);

        // Initialize the event subsystem.
        if (d_on_event_boot(null, null, null) == 0) {
            Debug.Log("Failed to initialize.");
            return;
        }
        GC.KeepAlive(this);

        // Initialize the event subsystem.
        if (d_on_event_start() == 0) {
            Debug.Log("Failed to initialize.");
            return;
        }
        GC.KeepAlive(this);
    }

    //
    // Native Code
    //

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_init_hal_func_table))]
    static extern unsafe void init_hal_func_table(
        IntPtr log_info,
        IntPtr log_warn,
        IntPtr log_error,
        IntPtr log_out_of_memory,
        IntPtr make_save_directory,
        IntPtr make_real_path,
        IntPtr notify_image_update,
        IntPtr notify_image_free,
        IntPtr render_image_normal,
        IntPtr render_image_add,
        IntPtr render_image_sub,
        IntPtr render_image_dim,
        IntPtr render_image_rule,
        IntPtr render_image_melt,
        IntPtr render_image_3d_normal,
        IntPtr render_image_3d_add,
        IntPtr render_image_3d_sub,
        IntPtr render_image_3d_dim,
        IntPtr reset_lap_timer,
        IntPtr get_lap_timer_millisec,
        IntPtr play_sound,
        IntPtr stop_sound,
        IntPtr set_sound_volume,
        IntPtr is_sound_finished,
        IntPtr play_video,
        IntPtr stop_video,
        IntPtr is_video_playing,
        IntPtr is_full_screen_supported,
        IntPtr is_full_screen_mode,
        IntPtr enter_full_screen_mode,
        IntPtr leave_full_screen_mode,
        IntPtr get_system_language,
        IntPtr set_continuous_swipe_enabled,
        IntPtr free_shared,
        IntPtr check_file_exist,
        IntPtr get_file_contents,
        IntPtr open_save_file,
        IntPtr write_save_file,
        IntPtr close_save_file);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_boot))]
    static extern unsafe int on_event_boot(byte *dummy1, byte *dummy2, byte *dummy3);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_start))]
    static extern unsafe int on_event_start();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_frame))]
    static extern unsafe int on_event_frame();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_key_press))]
    static extern unsafe void on_event_key_press(int key);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_key_release))]
    static extern unsafe void on_event_key_release(int key);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_press))]
    static extern unsafe void on_event_mouse_press(int button, int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_release))]
    static extern unsafe void on_event_mouse_release(int button, int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_move))]
    static extern unsafe void on_event_mouse_move(int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_touch_cancel))]
    static extern unsafe void on_event_touch_cancel();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_swipe_down))]
    static extern unsafe void on_event_swipe_down();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_swipe_up))]
    static extern unsafe void on_event_swipe_up();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_get_wave_samples))]
    public static extern unsafe int get_wave_samples(byte *w, uint *buf, int samples);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libplayfield")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_is_wave_eos))]
    public static extern unsafe bool is_wave_eos(byte *w);

    //
    // HAL functions
    //

    [AOT.MonoPInvokeCallback(typeof(delegate_log_info))]
    static unsafe void log_info(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_log_warn))]
    static unsafe void log_warn(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_log_error))]
    static unsafe void log_error(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_log_out_of_memory))]
    static unsafe void log_out_of_memory()
    {
        Debug.Log("Out of memory.");
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_make_save_directory))]
    static unsafe void make_save_directory()
    {
        // Do nothing.
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_make_real_path))]
    static unsafe void make_real_path(byte *fname, byte *dst, int len)
    {
        string Path = BytePtrToString(fname);

        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(Path);
        for (int i = 0; i < bytes.Length && i < len; i++)
        {
            dst[i] = bytes[i];
        }
        dst[bytes.Length] = 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_notify_image_update))]
    static unsafe void notify_image_update(int id, int width, int height, uint *pixels)
    {
        if (!imageDict.ContainsKey(id))
        {
            ManagedImage storeImage = new ManagedImage();
            storeImage.width = width;
            storeImage.height = height;
            storeImage.pixels = new Color[storeImage.width * storeImage.height];
            storeImage.texture = new Texture2D(storeImage.width, storeImage.height, TextureFormat.ARGB32, false);
            storeImage.need_upload = false;
            imageDict.Add(id, storeImage);
        }

        ManagedImage dstImage = imageDict[id];
        for (int y = 0; y < dstImage.height; y++)
        {
            for (int x = 0; x < dstImage.width; x++)
            {
                uint p = pixels[y * dstImage.width + x];
                Color cl = new Color(((p >> 16) & 0xff) / 255.0f,
                                              ((p >> 8) & 0xff) / 255.0f,
                                      (p & 0xff) / 255.0f,
                                      ((p >> 24) & 0xff) / 255.0f);
                dstImage.pixels[y * dstImage.width + x] = cl;
            }
        }
        dstImage.need_upload = true;
        imageDict[id] = dstImage;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_notify_image_free))]
    static unsafe void notify_image_free(int id)
    {
        if (imageDict.ContainsKey(id))
        {
            ManagedImage image = imageDict[id];
            MonoBehaviour.Destroy(image.texture);
            imageDict.Remove(id);
        }
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_normal))]
    static unsafe void render_image_normal(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)src_left / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)src_left / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] {0, 1, 2, 1, 3, 2};

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._normalShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_add))]
    static unsafe void render_image_add(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)src_left / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)src_left / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._addShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_sub))]
    static unsafe void render_image_sub(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)src_left / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)src_left / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._addShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_dim))]
    static unsafe void render_image_dim(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, dst_top / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_left / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((dst_left + dst_width) / (float)viewportWidth * 2.0f - 1.0f, (dst_top + dst_height) / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)src_left / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)src_top / (float)srcImage.height),
            new Vector2((float)src_left / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width, (float)(src_top + src_height) / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] {0, 1, 2, 1, 3, 2};

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._dimShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_rule))]
    static unsafe void render_image_rule(int src_img, int rule_img, int threshold)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        ManagedImage ruleImage = imageDict[rule_img];
        if (ruleImage.need_upload)
        {
            ruleImage.texture.SetPixels(ruleImage.pixels, 0);
            ruleImage.texture.Apply();
            ruleImage.need_upload = false;
            imageDict[rule_img] = ruleImage;
        }

        int dst_width = srcImage.width;
        int dst_height = srcImage.height;
        Vector3[] vertices = new Vector3[] {
            new Vector3(-1.0f, -1.0f, 0),
            new Vector3(dst_width / (float)viewportWidth * 2.0f - 1.0f, -1.0f, 0),
            new Vector3(-1.0f, dst_height / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_width / (float)viewportWidth * 2.0f - 1.0f, dst_height / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2(0, 0),
            new Vector2(1.0f, 0),
            new Vector2(0, 1.0f),
            new Vector2(1.0f, 1.0f)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, threshold / 255.0f),
            new Color(0, 0, 0, threshold / 255.0f),
            new Color(0, 0, 0, threshold / 255.0f),
            new Color(0, 0, 0, threshold / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._ruleShader);
        material.mainTexture = srcImage.texture;
        material.SetTexture("_RuleTex", ruleImage.texture);

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_melt))]
    static unsafe void render_image_melt(int src_img, int rule_img, int progress)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        ManagedImage ruleImage = imageDict[rule_img];
        if (ruleImage.need_upload)
        {
            ruleImage.texture.SetPixels(ruleImage.pixels, 0);
            ruleImage.texture.Apply();
            ruleImage.need_upload = false;
            imageDict[rule_img] = ruleImage;
        }

        int dst_width = srcImage.width;
        int dst_height = srcImage.height;
        Vector3[] vertices = new Vector3[] {
            new Vector3(-1.0f, -1.0f, 0),
            new Vector3(dst_width / (float)viewportWidth * 2.0f - 1.0f, -1.0f, 0),
            new Vector3(-1.0f, dst_height / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(dst_width / (float)viewportWidth * 2.0f - 1.0f, dst_height / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

        Vector2[] uv = new Vector2[] {
            new Vector2(0, 0),
            new Vector2(1.0f, 0),
            new Vector2(0, 1.0f),
            new Vector2(1.0f, 1.0f)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, progress / 255.0f),
            new Color(0, 0, 0, progress / 255.0f),
            new Color(0, 0, 0, progress / 255.0f),
            new Color(0, 0, 0, progress / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._meltShader);
        material.mainTexture = srcImage.texture;
        material.SetTexture("_RuleTex", ruleImage.texture);

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_3d_normal))]
    static unsafe void render_image_3d_normal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(x1 / (float)viewportWidth * 2.0f - 1.0f, y1 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x2 / (float)viewportWidth * 2.0f - 1.0f, y2 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x3 / (float)viewportWidth * 2.0f - 1.0f, y3 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x4 / (float)viewportWidth * 2.0f - 1.0f, y4 / (float)viewportHeight * 2.0f - 1.0f, 0)
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)x1 / (float)srcImage.width, (float)y1 / (float)srcImage.height),
            new Vector2((float)x2 / (float)srcImage.width, (float)y2 / (float)srcImage.height),
            new Vector2((float)x3 / (float)srcImage.width, (float)y3 / (float)srcImage.height),
            new Vector2((float)x4 / (float)srcImage.width, (float)y4 / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] {0, 1, 2, 1, 3, 2};

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._normalShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_3d_add))]
    static unsafe void render_image_3d_add(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(x1 / (float)viewportWidth * 2.0f - 1.0f, y1 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x2 / (float)viewportWidth * 2.0f - 1.0f, y2 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x3 / (float)viewportWidth * 2.0f - 1.0f, y3 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x4 / (float)viewportWidth * 2.0f - 1.0f, y4 / (float)viewportHeight * 2.0f - 1.0f, 0)
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)x1 / (float)srcImage.width, (float)y1 / (float)srcImage.height),
            new Vector2((float)x2 / (float)srcImage.width, (float)y2 / (float)srcImage.height),
            new Vector2((float)x3 / (float)srcImage.width, (float)y3 / (float)srcImage.height),
            new Vector2((float)x4 / (float)srcImage.width, (float)y4 / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._addShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_3d_sub))]
    static unsafe void render_image_3d_sub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(x1 / (float)viewportWidth * 2.0f - 1.0f, y1 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x2 / (float)viewportWidth * 2.0f - 1.0f, y2 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x3 / (float)viewportWidth * 2.0f - 1.0f, y3 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x4 / (float)viewportWidth * 2.0f - 1.0f, y4 / (float)viewportHeight * 2.0f - 1.0f, 0)
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)x1 / (float)srcImage.width, (float)y1 / (float)srcImage.height),
            new Vector2((float)x2 / (float)srcImage.width, (float)y2 / (float)srcImage.height),
            new Vector2((float)x3 / (float)srcImage.width, (float)y3 / (float)srcImage.height),
            new Vector2((float)x4 / (float)srcImage.width, (float)y4 / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._addShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_render_image_3d_dim))]
    static unsafe void render_image_3d_dim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
        ManagedImage srcImage = imageDict[src_img];
        if (srcImage.need_upload)
        {
            srcImage.texture.SetPixels(srcImage.pixels, 0);
            srcImage.texture.Apply();
            srcImage.need_upload = false;
            imageDict[src_img] = srcImage;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(x1 / (float)viewportWidth * 2.0f - 1.0f, y1 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x2 / (float)viewportWidth * 2.0f - 1.0f, y2 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x3 / (float)viewportWidth * 2.0f - 1.0f, y3 / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3(x4 / (float)viewportWidth * 2.0f - 1.0f, y4 / (float)viewportHeight * 2.0f - 1.0f, 0)
        };

        Vector2[] uv = new Vector2[] {
            new Vector2((float)x1 / (float)srcImage.width, (float)y1 / (float)srcImage.height),
            new Vector2((float)x2 / (float)srcImage.width, (float)y2 / (float)srcImage.height),
            new Vector2((float)x3 / (float)srcImage.width, (float)y3 / (float)srcImage.height),
            new Vector2((float)x4 / (float)srcImage.width, (float)y4 / (float)srcImage.height)
        };

        Color[] colors = new Color[] {
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f),
            new Color(0, 0, 0, alpha / 255.0f)
        };

        int[] triangles = new int[] { 0, 1, 2, 1, 3, 2 };

        Vector3[] normals = new Vector3[] {
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1),
            new Vector3(0, 0, -1)
        };

        Material material = new Material(_instance._addShader);
        material.mainTexture = srcImage.texture;

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_reset_lap_timer))]
    static unsafe void reset_lap_timer(IntPtr origin)
    {
        Marshal.WriteInt64(origin, Environment.TickCount);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_get_lap_timer_millisec))]
    static unsafe Int64 get_lap_timer_millisec(IntPtr origin)
    {
        Int64 ret = Environment.TickCount - Marshal.ReadInt64(origin);
        return ret;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_play_sound))]
    static unsafe void play_sound(int stream, byte *wave)
    {
        if (stream == 0)
            GameObject.Find("audio1").GetComponent<PlayfieldAudio>().SetSource(wave);
        else if (stream == 1)
            GameObject.Find("audio2").GetComponent<PlayfieldAudio>().SetSource(wave);
        else if (stream == 2)
            GameObject.Find("audio3").GetComponent<PlayfieldAudio>().SetSource(wave);
        else
            GameObject.Find("audio4").GetComponent<PlayfieldAudio>().SetSource(wave);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_stop_sound))]
    static unsafe void stop_sound(int stream)
    {
        if (stream == 0)
            GameObject.Find("audio1").GetComponent<PlayfieldAudio>().SetSource(null);
        else if (stream == 1)
            GameObject.Find("audio2").GetComponent<PlayfieldAudio>().SetSource(null);
        else if (stream == 2)
            GameObject.Find("audio3").GetComponent<PlayfieldAudio>().SetSource(null);
        else
            GameObject.Find("audio4").GetComponent<PlayfieldAudio>().SetSource(null);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_set_sound_volume))]
    static unsafe void set_sound_volume(int stream, float vol)
    {
        if (stream == 0)
            GameObject.Find("audio1").GetComponent<AudioSource>().volume = vol;
        else if (stream == 1)
            GameObject.Find("audio2").GetComponent<AudioSource>().volume = vol;
        else if (stream == 2)
            GameObject.Find("audio3").GetComponent<AudioSource>().volume = vol;
        else
            GameObject.Find("audio4").GetComponent<AudioSource>().volume = vol;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_is_sound_finished))]
    static unsafe int is_sound_finished(int stream)
    {
        if (stream == 0)
  		   return GameObject.Find("audio1").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else if (stream == 1)
            return GameObject.Find("audio2").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else if (stream == 2)
            return GameObject.Find("audio3").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else
            return GameObject.Find("audio4").GetComponent<AudioSource>().isPlaying ? 1 : 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_play_video))]
    static unsafe int play_video(byte *fname, int is_skippable)
    {
        string Path = BytePtrToString(fname);

        GameObject camera = GameObject.Find("Main Camera");
		_instance._videoPlayer = camera.AddComponent<VideoPlayer>();
		_instance._videoPlayer.playOnAwake = false;
		_instance._videoPlayer.renderMode = UnityEngine.Video.VideoRenderMode.CameraNearPlane;
		_instance._videoPlayer.targetCameraAlpha = 1.0f;
		_instance._videoPlayer.url = "file://" + Application.streamingAssetsPath + "/video/" + Path;
		_instance._videoPlayer.isLooping = false;
		_instance._videoPlayer.Prepare();
		_instance._videoPlayer.Play();
		_instance._videoPlayer.loopPointReached += EndOfVideoReached;
		_instance._isVideoPlaying = true;

        return 1;
    }

	static void EndOfVideoReached(VideoPlayer vp) {
		Destroy(_instance._videoPlayer);
		_instance._videoPlayer = null;
		_instance._isVideoPlaying = false;
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_stop_video))]
    static unsafe void stop_video()
    {
		_instance._videoPlayer.Stop();
		_instance._isVideoPlaying = false;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_is_video_playing))]
    static unsafe int is_video_playing()
    {
        if (_instance._videoPlayer == null)
		    return 0;
		if (!_instance._isVideoPlaying)
            return 0;

		if ((ulong)_instance._videoPlayer.frame == _instance._videoPlayer.frameCount)
        {
            _instance._isVideoPlaying = false;
		    return 0;
        }

        return 1;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_is_full_screen_supported))]
    static unsafe int is_full_screen_supported()
    {
        // TODO
        return 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_is_full_screen_mode))]
    static unsafe int is_full_screen_mode()
    {
        // TODO
        return 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_enter_full_screen_mode))]
    static unsafe void enter_full_screen_mode()
    {
        // TODO
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_leave_full_screen_mode))]
    static unsafe void leave_full_screen_mode()
    {
        // TODO
    }

    static unsafe IntPtr locale = IntPtr.Zero;

    [AOT.MonoPInvokeCallback(typeof(delegate_get_system_language))]
    static unsafe IntPtr get_system_language()
    {
        // TODO
        if (locale == null)
        {
            locale = Marshal.StringToCoTaskMemUTF8("ja");
            GCHandle.Alloc(locale, GCHandleType.Pinned);
            GC.KeepAlive(locale);
        }
        return locale;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_set_continuous_swipe_enabled))]
    static unsafe void set_continuous_swipe_enabled(int is_enabled)
    {
        // TODO
    }

    //
    // HAL helpers
    //

    private static unsafe string _saveFile;
    private static unsafe byte[] _saveData;
    private static int _saveSize;
    private static int SAVE_DATA_MAX = 10 * 1024 * 1024;

    [AOT.MonoPInvokeCallback(typeof(delegate_free_shared))]
    static unsafe void free_shared(IntPtr p)
    {
        Marshal.FreeCoTaskMem(p);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_check_file_exist))]
    static unsafe int check_file_exist(byte *pFileName)
    {
        string FileName = BytePtrToString(pFileName);
        if (FileName.StartsWith("save/"))
        {
            string s = PlayerPrefs.GetString(FileName.Split("/")[1], "");
            if (s == "")
                return 0;
            return 1;
        }
        else
        {
            if (!System.IO.File.Exists(Application.streamingAssetsPath + "/" + FileName))
                return 0;
            return 1;
        }
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_get_file_contents))]
    static unsafe IntPtr get_file_contents(byte *pFileName, int *len)
    {
        IntPtr ret = IntPtr.Zero;
        string FileName = BytePtrToString(pFileName);
        if (FileName.StartsWith("save/"))
        {
            string s = PlayerPrefs.GetString(FileName.Split("/")[1], "");
            if (s == "")
                return IntPtr.Zero;
            byte[] decoded = Convert.FromBase64String(s);
            if (decoded == null)
                return IntPtr.Zero;
            ret = Marshal.AllocCoTaskMem(decoded.Length);
            Marshal.Copy(decoded, 0, ret, decoded.Length);
            *len = decoded.Length;
        }
        else
        {
            try
            {
                byte[] fileBody = System.IO.File.ReadAllBytes(Application.streamingAssetsPath + "/" + FileName);
                if (fileBody == null)
                    return IntPtr.Zero;
                ret = Marshal.AllocCoTaskMem(fileBody.Length);
                Marshal.Copy(fileBody, 0, ret, fileBody.Length);
                *len = fileBody.Length;
            }
            catch(Exception)
            {
                Debug.Log(Application.streamingAssetsPath + "/" + FileName + " not found.");
            }
        }

        GC.KeepAlive(ret);

        return ret;
    }

    private static unsafe string BytePtrToString(byte *s)
    {
        byte *b = s;
        int len = 0;
        while (*b != 0)
        {
            len++;
            b++;
        }
        byte[] managed = new byte[len];
        for (int i = 0; i < len; i++)
        {
            managed[i] = *s++;
        }
        string ret = Encoding.UTF8.GetString(managed);
         return ret;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_open_save_file))]
    static unsafe void open_save_file(byte *pFileName) {
        _saveFile = BytePtrToString(pFileName);
        _saveData = new byte[SAVE_DATA_MAX];
		_saveSize = 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_write_save_file))]
    static unsafe void write_save_file(int b) {
        _saveData[_saveSize++] = (byte)b;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_close_save_file))]
    static unsafe void close_save_file() {
        string val = Convert.ToBase64String(_saveData, 0, _saveSize);
        string old = PlayerPrefs.GetString(_saveFile, val);
    }

	//
	// Audio
	//
	[RequireComponent(typeof(AudioSource))]
    public class PlayfieldAudio : MonoBehaviour
	{
	    unsafe byte *wave;

		unsafe public void SetSource(byte *w)
		{
			wave = w;
		}

        void Start()
        {
        }

        unsafe void OnAudioFilterRead(float[] data, int channels)
        {
            if (wave == null)
                return;

            // Assume (channels==2)
            int samples = data.Length / channels;

            // Get PCM samples.
            short[] intData = new short[samples * 2];
            fixed(short *unsafePointer = intData)
            {
                PlayfieldScript.get_wave_samples(wave, (uint *)unsafePointer, samples);
                if (channels == 2)
                {
                    for (int i = 0; i < samples * 2; i++)
                        data[i] = intData[i] / 32767.0f;
                }
                else
                {
                    for (int i = 0; i < samples; i++)
                        data[i] = intData[i] / 32767.0f;
                }
            }

            // Stop if we reached to an end-of-stream.
            if (PlayfieldScript.is_wave_eos(wave))
                wave = null;
        }
    }
}
