/* -*- coding: utf-8; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * HAL for Unity C#
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

public class StratoScript : MonoBehaviour
{
	//
	// Screen Size (Please change here!)
	//

    private static int viewportWidth = 1280;
    private static int viewportHeight = 720;

    //
    // For Rendering
    //

    private Shader _normalShader;
    private Shader _addShader;
    private Shader _subShader;
    private Shader _dimShader;
    private Shader _ruleShader;
    private Shader _meltShader;
    private Shader _crossShader;
    private CommandBuffer _commandBuffer;
    private GameObject _audio1;
    private GameObject _audio2;
    private GameObject _audio3;
    private GameObject _audio4;
	private VideoPlayer _videoPlayer;
    private bool _isVideoPlaying;

	//
	// Constants
	//

	const int HAL_MOUSE_LEFT  = 0;
	const int HAL_MOUSE_RIGHT = 1;

	const int HAL_KEY_ESCAPE          = 0;
    const int HAL_KEY_RETURN          = 1;
    const int HAL_KEY_SPACE           = 2;
	const int HAL_KEY_TAB             = 3;
	const int HAL_KEY_BACKSPACE       = 4;
	const int HAL_KEY_DELETE          = 5;
	const int HAL_KEY_HOME            = 6;
	const int HAL_KEY_END             = 7;
	const int HAL_KEY_PAGEUP          = 8;
	const int HAL_KEY_PAGEDOWN        = 9;
	const int HAL_KEY_SHIFT           = 10;
	const int HAL_KEY_CONTROL         = 11;
	const int HAL_KEY_ALT             = 12;
	const int HAL_KEY_UP              = 13;
	const int HAL_KEY_DOWN            = 14;
	const int HAL_KEY_LEFT            = 15;
	const int HAL_KEY_RIGHT           = 16;
	const int HAL_KEY_A               = 17;
	const int HAL_KEY_B               = 18;
	const int HAL_KEY_C               = 19;
	const int HAL_KEY_D               = 20;
	const int HAL_KEY_E               = 21;
	const int HAL_KEY_F               = 22;
	const int HAL_KEY_G               = 23;
	const int HAL_KEY_H               = 24;
	const int HAL_KEY_I               = 25;
	const int HAL_KEY_J               = 26;
	const int HAL_KEY_K               = 27;
	const int HAL_KEY_L               = 28;
	const int HAL_KEY_M               = 29;
	const int HAL_KEY_N               = 30;
	const int HAL_KEY_O               = 31;
	const int HAL_KEY_P               = 32;
	const int HAL_KEY_Q               = 33;
	const int HAL_KEY_R               = 34;
	const int HAL_KEY_S               = 35;
	const int HAL_KEY_T               = 36;
	const int HAL_KEY_U               = 37;
	const int HAL_KEY_V               = 38;
	const int HAL_KEY_W               = 39;
	const int HAL_KEY_X               = 40;
	const int HAL_KEY_Y               = 41;
	const int HAL_KEY_Z               = 42;
	const int HAL_KEY_1               = 43;
	const int HAL_KEY_2               = 44;
	const int HAL_KEY_3               = 45;
	const int HAL_KEY_4               = 46;
	const int HAL_KEY_5               = 47;
	const int HAL_KEY_6               = 48;
	const int HAL_KEY_7               = 49;
	const int HAL_KEY_8               = 50;
	const int HAL_KEY_9               = 51;
	const int HAL_KEY_0               = 52;
	const int HAL_KEY_F1              = 53;
	const int HAL_KEY_F2              = 54;
	const int HAL_KEY_F3              = 55;
	const int HAL_KEY_F4              = 56;
	const int HAL_KEY_F5              = 57;
	const int HAL_KEY_F6              = 58;
	const int HAL_KEY_F7              = 59;
	const int HAL_KEY_F8              = 60;
	const int HAL_KEY_F9              = 61;
	const int HAL_KEY_F10             = 62;
	const int HAL_KEY_F11             = 63;
	const int HAL_KEY_F12             = 64;
	const int HAL_KEY_GAMEPAD_UP      = 65;
	const int HAL_KEY_GAMEPAD_DOWN    = 66;
	const int HAL_KEY_GAMEPAD_LEFT    = 67;
	const int HAL_KEY_GAMEPAD_RIGHT   = 68;
	const int HAL_KEY_GAMEPAD_A       = 69;
	const int HAL_KEY_GAMEPAD_B       = 70;
	const int HAL_KEY_GAMEPAD_X       = 71;
	const int HAL_KEY_GAMEPAD_Y       = 72;
	const int HAL_KEY_GAMEPAD_L       = 73;
	const int HAL_KEY_GAMEPAD_R       = 74;
	const int HAL_KEY_MAX             = 75;

	const int HAL_ANALOG_X1  = 0;
	const int HAL_ANALOG_Y1  = 1;
	const int HAL_ANALOG_X2  = 2;
	const int HAL_ANALOG_Y2  = 3;
	const int HAL_ANALOG_L   = 4;
	const int HAL_ANALOG_R   = 5;

	//
	// Constructor
	//
	public StratoScript()
	{
	}

    //
    // On awake
    //
    public void Awake()
    {
    }

    //
    // On start
    //
    public void Start()
    {
        // Save the instance.
        _instance = this;

        // Get the shaders.
        _normalShader = Resources.Load<Shader>("NormalShader");
        _addShader = Resources.Load<Shader>("AddShader");
        _subShader = Resources.Load<Shader>("SubShader");
        _dimShader = Resources.Load<Shader>("DimShader");
        _ruleShader = Resources.Load<Shader>("RuleShader");
        _meltShader = Resources.Load<Shader>("MeltShader");
        _crossShader = Resources.Load<Shader>("CrossShader");

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
    unsafe public void Update()
    {
        _commandBuffer.Clear();

		// Process inputs.
		ProcessMouseInput();
		ProcessKeyboardInput();
		//ProcessGamepadInput();

        // Do a frame update.
        if (on_event_update() == 0)
        {
            // Exit on finish or error.
			Destroy(this);
			return;
        }

		// Do a frame rendering.
		on_event_render();
    }

	private static void ProcessMouseInput()
	{
        int posX = (int)(Input.mousePosition.x * viewportWidth / Screen.width);
        int posY = (int)((Screen.height - Input.mousePosition.y) * viewportHeight / Screen.height);
		on_event_mouse_move(posX, posY);
		if (Input.GetMouseButtonDown(0))
            on_event_mouse_press(HAL_MOUSE_LEFT, posX, posY);
		if (Input.GetMouseButtonUp(0))
		   on_event_mouse_release(HAL_MOUSE_LEFT,posX, posY);
		if (Input.GetMouseButtonDown(1))
		   on_event_mouse_press(HAL_MOUSE_RIGHT, posX, posY);
		if (Input.GetMouseButtonUp(1))
		   on_event_mouse_release(HAL_MOUSE_RIGHT, posX, posY);
		if (Input.GetMouseButtonUp(2))
			on_event_mouse_wheel(-1, 1);
		if (Input.GetMouseButtonDown(2))
			on_event_mouse_wheel(1, 1);
	}

	private void ProcessKeyboardInput()
	{
        // Process key down.
        if (Input.GetKeyDown(KeyCode.LeftControl))
            on_event_key_press(HAL_KEY_CONTROL);
        if (Input.GetKeyDown(KeyCode.Space))
            on_event_key_press(HAL_KEY_SPACE);
        if (Input.GetKeyDown(KeyCode.Return))
            on_event_key_press(HAL_KEY_RETURN);
        if (Input.GetKeyDown(KeyCode.UpArrow))
            on_event_key_press(HAL_KEY_UP);
        if (Input.GetKeyDown(KeyCode.DownArrow))
            on_event_key_press(HAL_KEY_DOWN);
        if (Input.GetKeyDown(KeyCode.LeftArrow))
            on_event_key_press(HAL_KEY_LEFT);
        if (Input.GetKeyDown(KeyCode.RightArrow))
            on_event_key_press(HAL_KEY_RIGHT);
        if (Input.GetKeyDown(KeyCode.Escape))
            on_event_key_press(HAL_KEY_ESCAPE);

        // Process key up.
        if (Input.GetKeyUp(KeyCode.LeftControl))
            on_event_key_release(HAL_KEY_CONTROL);
        if (Input.GetKeyUp(KeyCode.Space))
            on_event_key_release(HAL_KEY_SPACE);
        if (Input.GetKeyUp(KeyCode.Return))
            on_event_key_release(HAL_KEY_RETURN);
        if (Input.GetKeyUp(KeyCode.UpArrow))
            on_event_key_release(HAL_KEY_UP);
        if (Input.GetKeyUp(KeyCode.UpArrow))
            on_event_key_release(HAL_KEY_UP);
        if (Input.GetKeyUp(KeyCode.LeftArrow))
            on_event_key_release(HAL_KEY_LEFT);
        if (Input.GetKeyUp(KeyCode.RightArrow))
            on_event_key_release(HAL_KEY_RIGHT);
        if (Input.GetKeyUp(KeyCode.Escape))
            on_event_key_release(HAL_KEY_ESCAPE);
	}

	private static void ProcessGamepadInput()
	{
		// Xbox / generic mapping in old Unity Input
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton0), HAL_KEY_GAMEPAD_A);
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton1), HAL_KEY_GAMEPAD_B);
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton2), HAL_KEY_GAMEPAD_X);
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton3), HAL_KEY_GAMEPAD_Y);
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton4), HAL_KEY_GAMEPAD_L);
		SendButtonEveryFrame(Input.GetKey(KeyCode.JoystickButton5), HAL_KEY_GAMEPAD_R);

		float x1 = GetAxisRawSafe("JoyX1");
		float y1 = GetAxisRawSafe("JoyY1");
		float x2 = GetAxisRawSafe("JoyX2");
		float y2 = GetAxisRawSafe("JoyY2");
		float lt = GetAxisRawSafe("JoyLTrigger");
		float rt = GetAxisRawSafe("JoyRTrigger");
		float dx = GetAxisRawSafe("JoyDPadX");
		float dy = GetAxisRawSafe("JoyDPadY");

		on_event_analog_input(HAL_ANALOG_X1, AxisToInt16(x1));
		on_event_analog_input(HAL_ANALOG_Y1, AxisToInt16(-y1));
		on_event_analog_input(HAL_ANALOG_X2, AxisToInt16(x2));
		on_event_analog_input(HAL_ANALOG_Y2, AxisToInt16(-y2));

		// Trigger axis may be 0..1 depending on Input Manager settings.
		on_event_analog_input(HAL_ANALOG_L, (int)(Mathf.Clamp01(lt) * 32767.0f));
		on_event_analog_input(HAL_ANALOG_R, (int)(Mathf.Clamp01(rt) * 32767.0f));

		SendButtonEveryFrame(dy >  0.5f, HAL_KEY_GAMEPAD_UP);
		SendButtonEveryFrame(dy < -0.5f, HAL_KEY_GAMEPAD_DOWN);
		SendButtonEveryFrame(dx < -0.5f, HAL_KEY_GAMEPAD_LEFT);
		SendButtonEveryFrame(dx >  0.5f, HAL_KEY_GAMEPAD_RIGHT);
	}

	private static float GetAxisRawSafe(string name)
	{
		try
		{
			return Input.GetAxisRaw(name);
		}
		catch
		{
			return 0.0f;
		}
	}

    private static int AxisToInt16(float v)
	{
		v = Mathf.Clamp(v, -1.0f, 1.0f);

		if (v >= 0)
			return (int)(v * 32767.0f);

		return (int)(v * 32768.0f);
	}

	private static void SendButtonEveryFrame(bool pressed, int key)
	{
		if (pressed)
			on_event_key_press(key);
		else
			on_event_key_release(key);
	}

    //
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //  [DO NOT TOUCH]
	//   C#-C bridging is for runtime experts only.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //

    //
    // The Sole Instance
    //
    private static StratoScript _instance;

    //
    // Delegate types for calling C# from C.
    //
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_log_info(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_log_warn(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_log_error(byte *s);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_log_out_of_memory();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_notify_image_update(int id, int width, int height, uint *pixels);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_notify_image_free(int id);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_normal(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_add(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_sub(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_dim(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_rule(int src_img, int rule_img, int threshold);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_melt(int src_img, int rule_img, int progress);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_cross(int src1_img, int src2_img, int src1_left, int src1_top, int src2_left, int src2_top, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_3d_normal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_3d_add(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_3d_sub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_3d_dim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_render_image_3d_cross(int src1_image, int src2_image, float src1_x1, float src1_y1, float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_reset_lap_timer(IntPtr origin);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate Int64 delegate_hal_get_lap_timer_millisec(IntPtr origin);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_play_sound(int stream, byte *wave);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_stop_sound(int stream);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_set_sound_volume(int stream, float vol);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_is_sound_finished(int stream);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_play_video(byte *fname, int is_skippable);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_stop_video();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_is_video_playing();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_is_full_screen_supported();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_is_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_enter_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_leave_full_screen_mode();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate IntPtr delegate_hal_get_system_language();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_hal_set_continuous_swipe_enabled(int is_enabled);
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
    unsafe delegate void delegate_init_hal_func_table(IntPtr hal_log_info,
                                                      IntPtr hal_log_warn,
                                                      IntPtr hal_log_error,
                                                      IntPtr hal_log_out_of_memory,
                                                      IntPtr hal_notify_image_update,
                                                      IntPtr hal_notify_image_free,
                                                      IntPtr hal_render_image_normal,
                                                      IntPtr hal_render_image_add,
                                                      IntPtr hal_render_image_sub,
                                                      IntPtr hal_render_image_dim,
                                                      IntPtr hal_render_image_rule,
                                                      IntPtr hal_render_image_melt,
                                                      IntPtr hal_render_image_cross,
                                                      IntPtr hal_render_image_3d_normal,
                                                      IntPtr hal_render_image_3d_add,
                                                      IntPtr hal_render_image_3d_sub,
                                                      IntPtr hal_render_image_3d_dim,
                                                      IntPtr hal_render_image_3d_cross,
                                                      IntPtr hal_reset_lap_timer,
                                                      IntPtr hal_get_lap_timer_millisec,
                                                      IntPtr hal_play_sound,
                                                      IntPtr hal_stop_sound,
                                                      IntPtr hal_set_sound_volume,
                                                      IntPtr hal_is_sound_finished,
                                                      IntPtr hal_play_video,
                                                      IntPtr hal_stop_video,
                                                      IntPtr hal_is_video_playing,
                                                      IntPtr hal_is_full_screen_supported,
                                                      IntPtr hal_is_full_screen_mode,
                                                      IntPtr hal_enter_full_screen_mode,
                                                      IntPtr hal_leave_full_screen_mode,
                                                      IntPtr hal_get_system_language,
                                                      IntPtr hal_set_continuous_swipe_enabled,
                                                      IntPtr check_file_exist,
                                                      IntPtr get_file_contents,
                                                      IntPtr open_save_file,
                                                      IntPtr write_save_file,
                                                      IntPtr close_save_file,
                                                      IntPtr free_shared);
													  
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_setup();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_start();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_update();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_on_event_render();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_key_press(int key);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_key_release(int key);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_press(int button, int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_release(int button, int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_move(int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_mouse_wheel(int v, int h);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_touch_cancel();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_swipe_down(float speed, float amount);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_swipe_up(float speed, float amount);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate void delegate_on_event_analog_input(int input, int val);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate int delegate_hal_get_wave_samples(byte* w, uint* buf, int samples);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] unsafe delegate bool delegate_hal_is_wave_eos(byte* w);

    //
    // Delegate objects for calling C# from C.
    //
    static delegate_hal_log_info d_hal_log_info;
    static delegate_hal_log_warn d_hal_log_warn;
    static delegate_hal_log_error d_hal_log_error;
    static delegate_hal_log_out_of_memory d_hal_log_out_of_memory;
    static delegate_hal_notify_image_update d_hal_notify_image_update;
    static delegate_hal_notify_image_free d_hal_notify_image_free;
    static delegate_hal_render_image_normal d_hal_render_image_normal;
    static delegate_hal_render_image_add d_hal_render_image_add;
    static delegate_hal_render_image_sub d_hal_render_image_sub;
    static delegate_hal_render_image_dim d_hal_render_image_dim;
    static delegate_hal_render_image_rule d_hal_render_image_rule;
    static delegate_hal_render_image_melt d_hal_render_image_melt;
    static delegate_hal_render_image_cross d_hal_render_image_cross;
    static delegate_hal_render_image_3d_normal d_hal_render_image_3d_normal;
    static delegate_hal_render_image_3d_add d_hal_render_image_3d_add;
    static delegate_hal_render_image_3d_sub d_hal_render_image_3d_sub;
	static delegate_hal_render_image_3d_dim d_hal_render_image_3d_dim;
    static delegate_hal_render_image_3d_cross d_hal_render_image_3d_cross;
    static delegate_hal_reset_lap_timer d_hal_reset_lap_timer;
    static delegate_hal_get_lap_timer_millisec d_hal_get_lap_timer_millisec;
    static delegate_hal_play_sound d_hal_play_sound;
    static delegate_hal_stop_sound d_hal_stop_sound;
    static delegate_hal_set_sound_volume d_hal_set_sound_volume;
    static delegate_hal_is_sound_finished d_hal_is_sound_finished;
    static delegate_hal_play_video d_hal_play_video;
    static delegate_hal_stop_video d_hal_stop_video;
    static delegate_hal_is_video_playing d_hal_is_video_playing;
    static delegate_hal_is_full_screen_supported d_hal_is_full_screen_supported;
    static delegate_hal_is_full_screen_mode d_hal_is_full_screen_mode;
    static delegate_hal_enter_full_screen_mode d_hal_enter_full_screen_mode;
    static delegate_hal_leave_full_screen_mode d_hal_leave_full_screen_mode;
    static delegate_hal_get_system_language d_hal_get_system_language;
    static delegate_hal_set_continuous_swipe_enabled d_hal_set_continuous_swipe_enabled;
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
    static delegate_on_event_setup d_on_event_setup;
    static delegate_on_event_start d_on_event_start;
    static delegate_on_event_update d_on_event_update;
    static delegate_on_event_render d_on_event_render;
    static delegate_on_event_key_press d_on_event_key_press;
    static delegate_on_event_key_release d_on_event_key_release;
    static delegate_on_event_mouse_press d_on_event_mouse_press;
    static delegate_on_event_mouse_release d_on_event_mouse_release;
    static delegate_on_event_mouse_move d_on_event_mouse_move;
    static delegate_on_event_mouse_wheel d_on_event_mouse_wheel;
    static delegate_on_event_touch_cancel d_on_event_touch_cancel;
    static delegate_on_event_swipe_down d_on_event_swipe_down;
    static delegate_on_event_swipe_up d_on_event_swipe_up;
    static delegate_on_event_analog_input d_on_event_analog_input;
    static delegate_hal_get_wave_samples d_hal_get_wave_samples;
    static delegate_hal_is_wave_eos d_hal_is_wave_eos;

    //
    // Delegate pointers for calling C# from C.
    //
    static IntPtr p_hal_log_info;
    static IntPtr p_hal_log_warn;
    static IntPtr p_hal_log_error;
    static IntPtr p_hal_log_out_of_memory;
    static IntPtr p_hal_make_save_directory;
    static IntPtr p_hal_make_real_path;
    static IntPtr p_hal_notify_image_update;
    static IntPtr p_hal_notify_image_free;
    static IntPtr p_hal_render_image_normal;
    static IntPtr p_hal_render_image_add;
    static IntPtr p_hal_render_image_sub;
    static IntPtr p_hal_render_image_dim;
    static IntPtr p_hal_render_image_rule;
    static IntPtr p_hal_render_image_melt;
    static IntPtr p_hal_render_image_cross;
    static IntPtr p_hal_render_image_3d_normal;
    static IntPtr p_hal_render_image_3d_add;
    static IntPtr p_hal_render_image_3d_sub;
    static IntPtr p_hal_render_image_3d_dim;
    static IntPtr p_hal_render_image_3d_cross;
    static IntPtr p_hal_reset_lap_timer;
    static IntPtr p_hal_get_lap_timer_millisec;
    static IntPtr p_hal_play_sound;
    static IntPtr p_hal_stop_sound;
    static IntPtr p_hal_set_sound_volume;
    static IntPtr p_hal_is_sound_finished;
    static IntPtr p_hal_play_video;
    static IntPtr p_hal_stop_video;
    static IntPtr p_hal_is_video_playing;
    static IntPtr p_hal_is_full_screen_supported;
    static IntPtr p_hal_is_full_screen_mode;
    static IntPtr p_hal_enter_full_screen_mode;
    static IntPtr p_hal_leave_full_screen_mode;
    static IntPtr p_hal_get_system_language;
    static IntPtr p_hal_set_continuous_swipe_enabled;
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
    static IntPtr p_on_event_setup;
    static IntPtr p_on_event_start;
    static IntPtr p_on_event_update;
    static IntPtr p_on_event_render;
    static IntPtr p_on_event_key_press;
    static IntPtr p_on_event_key_release;
    static IntPtr p_on_event_mouse_press;
    static IntPtr p_on_event_mouse_release;
    static IntPtr p_on_event_mouse_move;
    static IntPtr p_on_event_mouse_wheel;
    static IntPtr p_on_event_touch_cancel;
    static IntPtr p_on_event_swipe_down;
    static IntPtr p_on_event_swipe_up;
    static IntPtr p_on_event_analog_input;
    static IntPtr p_hal_get_wave_samples;
    static IntPtr p_hal_is_wave_eos;

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
        d_hal_log_info = new delegate_hal_log_info(hal_log_info);
        d_hal_log_warn = new delegate_hal_log_warn(hal_log_warn);
        d_hal_log_error = new delegate_hal_log_error(hal_log_error);
        d_hal_log_out_of_memory = new delegate_hal_log_out_of_memory(hal_log_out_of_memory);
        d_hal_notify_image_update = new delegate_hal_notify_image_update(hal_notify_image_update);
        d_hal_notify_image_free = new delegate_hal_notify_image_free(hal_notify_image_free);
        d_hal_render_image_normal = new delegate_hal_render_image_normal(hal_render_image_normal);
        d_hal_render_image_add = new delegate_hal_render_image_add(hal_render_image_add);
        d_hal_render_image_sub = new delegate_hal_render_image_sub(hal_render_image_sub);
        d_hal_render_image_dim = new delegate_hal_render_image_dim(hal_render_image_dim);
        d_hal_render_image_rule = new delegate_hal_render_image_rule(hal_render_image_rule);
        d_hal_render_image_melt = new delegate_hal_render_image_melt(hal_render_image_melt);
        d_hal_render_image_cross = new delegate_hal_render_image_cross(hal_render_image_cross);
        d_hal_render_image_3d_normal = new delegate_hal_render_image_3d_normal(hal_render_image_3d_normal);
        d_hal_render_image_3d_add = new delegate_hal_render_image_3d_add(hal_render_image_3d_add);
        d_hal_render_image_3d_sub = new delegate_hal_render_image_3d_sub(hal_render_image_3d_sub);
        d_hal_render_image_3d_dim = new delegate_hal_render_image_3d_dim(hal_render_image_3d_dim);
        d_hal_render_image_3d_cross = new delegate_hal_render_image_3d_cross(hal_render_image_3d_cross);
        d_hal_reset_lap_timer = new delegate_hal_reset_lap_timer(hal_reset_lap_timer);
        d_hal_get_lap_timer_millisec = new delegate_hal_get_lap_timer_millisec(hal_get_lap_timer_millisec);
        d_hal_play_sound = new delegate_hal_play_sound(hal_play_sound);
        d_hal_stop_sound = new delegate_hal_stop_sound(hal_stop_sound);
        d_hal_set_sound_volume = new delegate_hal_set_sound_volume(hal_set_sound_volume);
        d_hal_is_sound_finished = new delegate_hal_is_sound_finished(hal_is_sound_finished);
        d_hal_play_video = new delegate_hal_play_video(hal_play_video);
        d_hal_stop_video = new delegate_hal_stop_video(hal_stop_video);
        d_hal_is_video_playing = new delegate_hal_is_video_playing(hal_is_video_playing);
        d_hal_is_full_screen_supported = new delegate_hal_is_full_screen_supported(hal_is_full_screen_supported);
        d_hal_is_full_screen_mode = new delegate_hal_is_full_screen_mode(hal_is_full_screen_supported);
        d_hal_enter_full_screen_mode = new delegate_hal_enter_full_screen_mode(hal_enter_full_screen_mode);
        d_hal_leave_full_screen_mode = new delegate_hal_leave_full_screen_mode(hal_leave_full_screen_mode);
        d_hal_get_system_language = new delegate_hal_get_system_language(hal_get_system_language);
        d_hal_set_continuous_swipe_enabled = new delegate_hal_set_continuous_swipe_enabled(hal_set_continuous_swipe_enabled);
        d_free_shared = new delegate_free_shared(free_shared);
        d_get_file_contents = new delegate_get_file_contents(get_file_contents);
        d_open_save_file = new delegate_open_save_file(open_save_file);
        d_write_save_file = new delegate_write_save_file(write_save_file);
        d_close_save_file = new delegate_close_save_file(close_save_file);
        d_check_file_exist = new delegate_check_file_exist(check_file_exist);

		// Set delegate objects. (calling C from C#)
        d_init_hal_func_table = new delegate_init_hal_func_table(init_hal_func_table);
        d_on_event_setup = new delegate_on_event_setup(on_event_setup);
        d_on_event_start = new delegate_on_event_start(on_event_start);
        d_on_event_update = new delegate_on_event_update(on_event_update);
        d_on_event_render = new delegate_on_event_render(on_event_render);
        d_on_event_key_press = new delegate_on_event_key_press(on_event_key_press);
        d_on_event_key_release = new delegate_on_event_key_release(on_event_key_release);
        d_on_event_mouse_press = new delegate_on_event_mouse_press(on_event_mouse_press);
        d_on_event_mouse_release = new delegate_on_event_mouse_release(on_event_mouse_release);
        d_on_event_mouse_move = new delegate_on_event_mouse_move(on_event_mouse_move);
        d_on_event_mouse_wheel = new delegate_on_event_mouse_wheel(on_event_mouse_wheel);
        d_on_event_touch_cancel = new delegate_on_event_touch_cancel(on_event_touch_cancel);
        d_on_event_swipe_down = new delegate_on_event_swipe_down(on_event_swipe_down);
        d_on_event_swipe_up = new delegate_on_event_swipe_up(on_event_swipe_up);
        d_on_event_analog_input = new delegate_on_event_analog_input(on_event_analog_input);
        d_hal_get_wave_samples = new delegate_hal_get_wave_samples(hal_get_wave_samples);
        d_hal_is_wave_eos = new delegate_hal_is_wave_eos(hal_is_wave_eos);

        // Get function pointers. (calling back C# from C)
        p_hal_log_info = Marshal.GetFunctionPointerForDelegate(d_hal_log_info);
        p_hal_log_warn = Marshal.GetFunctionPointerForDelegate(d_hal_log_warn);
        p_hal_log_error = Marshal.GetFunctionPointerForDelegate(d_hal_log_error);
        p_hal_log_out_of_memory = Marshal.GetFunctionPointerForDelegate(d_hal_log_out_of_memory);
        p_hal_notify_image_update = Marshal.GetFunctionPointerForDelegate(d_hal_notify_image_update);
        p_hal_notify_image_free = Marshal.GetFunctionPointerForDelegate(d_hal_notify_image_free);
        p_hal_render_image_normal = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_normal);
        p_hal_render_image_add = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_add);
        p_hal_render_image_sub = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_sub);
        p_hal_render_image_dim = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_dim);
        p_hal_render_image_rule = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_rule);
        p_hal_render_image_melt = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_melt);
        p_hal_render_image_cross = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_cross);
        p_hal_render_image_3d_normal = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_3d_normal);
        p_hal_render_image_3d_add = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_3d_add);
        p_hal_render_image_3d_sub = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_3d_sub);
        p_hal_render_image_3d_dim = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_3d_dim);
        p_hal_render_image_3d_cross = Marshal.GetFunctionPointerForDelegate(d_hal_render_image_3d_cross);
        p_hal_reset_lap_timer = Marshal.GetFunctionPointerForDelegate(d_hal_reset_lap_timer);
        p_hal_get_lap_timer_millisec = Marshal.GetFunctionPointerForDelegate(d_hal_get_lap_timer_millisec);
        p_hal_play_sound = Marshal.GetFunctionPointerForDelegate(d_hal_play_sound);
        p_hal_stop_sound = Marshal.GetFunctionPointerForDelegate(d_hal_stop_sound);
        p_hal_set_sound_volume = Marshal.GetFunctionPointerForDelegate(d_hal_set_sound_volume);
        p_hal_is_sound_finished = Marshal.GetFunctionPointerForDelegate(d_hal_is_sound_finished);
        p_hal_play_video = Marshal.GetFunctionPointerForDelegate(d_hal_play_video);
        p_hal_stop_video = Marshal.GetFunctionPointerForDelegate(d_hal_stop_video);
        p_hal_is_video_playing = Marshal.GetFunctionPointerForDelegate(d_hal_is_video_playing);
        p_hal_is_full_screen_supported = Marshal.GetFunctionPointerForDelegate(d_hal_is_full_screen_supported);
        p_hal_is_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_hal_is_full_screen_mode);
        p_hal_enter_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_hal_enter_full_screen_mode);
        p_hal_leave_full_screen_mode = Marshal.GetFunctionPointerForDelegate(d_hal_leave_full_screen_mode);
        p_hal_get_system_language = Marshal.GetFunctionPointerForDelegate(d_hal_get_system_language);
        p_hal_set_continuous_swipe_enabled = Marshal.GetFunctionPointerForDelegate(d_hal_set_continuous_swipe_enabled);
        p_check_file_exist = Marshal.GetFunctionPointerForDelegate(d_check_file_exist);
        p_get_file_contents = Marshal.GetFunctionPointerForDelegate(d_get_file_contents);
        p_open_save_file = Marshal.GetFunctionPointerForDelegate(d_open_save_file);
        p_write_save_file = Marshal.GetFunctionPointerForDelegate(d_write_save_file);
        p_close_save_file = Marshal.GetFunctionPointerForDelegate(d_close_save_file);
        p_free_shared = Marshal.GetFunctionPointerForDelegate(d_free_shared);

        // Get function pointers. (calling C from C#)
        p_init_hal_func_table = Marshal.GetFunctionPointerForDelegate(d_init_hal_func_table);
        p_on_event_setup = Marshal.GetFunctionPointerForDelegate(d_on_event_setup);
        p_on_event_start = Marshal.GetFunctionPointerForDelegate(d_on_event_start);
        p_on_event_update = Marshal.GetFunctionPointerForDelegate(d_on_event_update);
        p_on_event_render = Marshal.GetFunctionPointerForDelegate(d_on_event_render);
        p_on_event_key_press = Marshal.GetFunctionPointerForDelegate(d_on_event_key_press);
        p_on_event_key_release = Marshal.GetFunctionPointerForDelegate(d_on_event_key_release);
        p_on_event_mouse_press = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_press);
        p_on_event_mouse_release = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_release);
        p_on_event_mouse_move = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_move);
        p_on_event_mouse_wheel = Marshal.GetFunctionPointerForDelegate(d_on_event_mouse_wheel);
        p_on_event_touch_cancel = Marshal.GetFunctionPointerForDelegate(d_on_event_touch_cancel);
        p_on_event_swipe_down = Marshal.GetFunctionPointerForDelegate(d_on_event_swipe_down);
        p_on_event_swipe_up = Marshal.GetFunctionPointerForDelegate(d_on_event_swipe_up);
        p_on_event_analog_input = Marshal.GetFunctionPointerForDelegate(d_on_event_analog_input);
        p_hal_get_wave_samples = Marshal.GetFunctionPointerForDelegate(d_hal_get_wave_samples);
        p_hal_is_wave_eos = Marshal.GetFunctionPointerForDelegate(d_hal_is_wave_eos);

        // Lock function pointers by Alloc(). (calling C# from C)
        GCHandle.Alloc(p_hal_log_info, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_log_warn, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_log_error, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_log_out_of_memory, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_make_save_directory, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_make_real_path, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_notify_image_update, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_notify_image_free, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_normal, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_add, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_sub, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_dim, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_rule, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_melt, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_cross, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_3d_normal, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_3d_add, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_3d_sub, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_3d_dim, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_render_image_3d_cross, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_reset_lap_timer, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_get_lap_timer_millisec, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_play_sound, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_stop_sound, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_set_sound_volume, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_is_sound_finished, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_play_video, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_stop_video, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_is_video_playing, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_is_full_screen_supported, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_is_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_enter_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_leave_full_screen_mode, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_get_system_language, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_set_continuous_swipe_enabled, GCHandleType.Pinned);
        GCHandle.Alloc(p_free_shared, GCHandleType.Pinned);
        GCHandle.Alloc(p_get_file_contents, GCHandleType.Pinned);
        GCHandle.Alloc(p_open_save_file, GCHandleType.Pinned);
        GCHandle.Alloc(p_write_save_file, GCHandleType.Pinned);
        GCHandle.Alloc(p_close_save_file, GCHandleType.Pinned);
        GCHandle.Alloc(p_check_file_exist, GCHandleType.Pinned);

		// Lock function pointers by Alloc(). (calling C from C#)
        GCHandle.Alloc(p_init_hal_func_table, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_setup, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_start, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_update, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_render, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_key_press, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_key_release, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_press, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_release, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_move, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_mouse_wheel, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_touch_cancel, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_swipe_down, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_swipe_up, GCHandleType.Pinned);
        GCHandle.Alloc(p_on_event_analog_input, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_get_wave_samples, GCHandleType.Pinned);
        GCHandle.Alloc(p_hal_is_wave_eos, GCHandleType.Pinned);

        // Lock function pointers by KeepAlive(). (calling C# from C)
        GC.KeepAlive(p_hal_log_info);
        GC.KeepAlive(p_hal_log_warn);
        GC.KeepAlive(p_hal_log_error);
        GC.KeepAlive(p_hal_log_out_of_memory);
        GC.KeepAlive(p_hal_make_save_directory);
        GC.KeepAlive(p_hal_make_real_path);
        GC.KeepAlive(p_hal_notify_image_update);
        GC.KeepAlive(p_hal_notify_image_free);
        GC.KeepAlive(p_hal_render_image_normal);
        GC.KeepAlive(p_hal_render_image_add);
        GC.KeepAlive(p_hal_render_image_sub);
        GC.KeepAlive(p_hal_render_image_dim);
        GC.KeepAlive(p_hal_render_image_rule);
        GC.KeepAlive(p_hal_render_image_melt);
        GC.KeepAlive(p_hal_render_image_cross);
        GC.KeepAlive(p_hal_render_image_3d_normal);
        GC.KeepAlive(p_hal_render_image_3d_add);
        GC.KeepAlive(p_hal_render_image_3d_sub);
        GC.KeepAlive(p_hal_render_image_3d_dim);
        GC.KeepAlive(p_hal_render_image_3d_cross);
        GC.KeepAlive(p_hal_reset_lap_timer);
        GC.KeepAlive(p_hal_get_lap_timer_millisec);
        GC.KeepAlive(p_hal_play_sound);
        GC.KeepAlive(p_hal_stop_sound);
        GC.KeepAlive(p_hal_set_sound_volume);
        GC.KeepAlive(p_hal_is_sound_finished);
        GC.KeepAlive(p_hal_play_video);
        GC.KeepAlive(p_hal_stop_video);
        GC.KeepAlive(p_hal_is_video_playing);
        GC.KeepAlive(p_hal_is_full_screen_supported);
        GC.KeepAlive(p_hal_is_full_screen_mode);
        GC.KeepAlive(p_hal_enter_full_screen_mode);
        GC.KeepAlive(p_hal_leave_full_screen_mode);
        GC.KeepAlive(p_hal_get_system_language);
        GC.KeepAlive(p_hal_set_continuous_swipe_enabled);
        GC.KeepAlive(p_check_file_exist);
        GC.KeepAlive(p_get_file_contents);
        GC.KeepAlive(p_open_save_file);
        GC.KeepAlive(p_write_save_file);
        GC.KeepAlive(p_close_save_file);
        GC.KeepAlive(p_free_shared);

        // Lock function pointers by KeepAlive(). (calling C from C#)
        GC.KeepAlive(p_init_hal_func_table);
        GC.KeepAlive(p_on_event_setup);
        GC.KeepAlive(p_on_event_start);
        GC.KeepAlive(p_on_event_update);
        GC.KeepAlive(p_on_event_render);
        GC.KeepAlive(p_on_event_key_press);
        GC.KeepAlive(p_on_event_key_release);
        GC.KeepAlive(p_on_event_mouse_press);
        GC.KeepAlive(p_on_event_mouse_release);
        GC.KeepAlive(p_on_event_mouse_move);
        GC.KeepAlive(p_on_event_mouse_wheel);
        GC.KeepAlive(p_on_event_touch_cancel);
        GC.KeepAlive(p_on_event_swipe_down);
        GC.KeepAlive(p_on_event_swipe_up);
        GC.KeepAlive(p_on_event_analog_input);
        GC.KeepAlive(p_hal_get_wave_samples);
        GC.KeepAlive(p_hal_is_wave_eos);

        // Call init_hal_func_table() to initialize the HAL function table.
        d_init_hal_func_table(
            p_hal_log_info,
            p_hal_log_warn,
            p_hal_log_error,
            p_hal_log_out_of_memory,
            p_hal_notify_image_update,
            p_hal_notify_image_free,
            p_hal_render_image_normal,
            p_hal_render_image_add,
            p_hal_render_image_sub,
            p_hal_render_image_dim,
            p_hal_render_image_rule,
            p_hal_render_image_melt,
            p_hal_render_image_cross,
            p_hal_render_image_3d_normal,
            p_hal_render_image_3d_add,
            p_hal_render_image_3d_sub,
            p_hal_render_image_3d_dim,
            p_hal_render_image_3d_cross,
            p_hal_reset_lap_timer,
            p_hal_get_lap_timer_millisec,
            p_hal_play_sound,
            p_hal_stop_sound,
            p_hal_set_sound_volume,
            p_hal_is_sound_finished,
            p_hal_play_video,
            p_hal_stop_video,
            p_hal_is_video_playing,
            p_hal_is_full_screen_supported,
            p_hal_is_full_screen_mode,
            p_hal_enter_full_screen_mode,
            p_hal_leave_full_screen_mode,
            p_hal_get_system_language,
            p_hal_set_continuous_swipe_enabled,
            p_check_file_exist,
            p_get_file_contents,
            p_open_save_file,
            p_write_save_file,
            p_close_save_file,
            p_free_shared);
        GC.KeepAlive(this);

        // Initialize the event subsystem.
        if (d_on_event_setup() == 0) {
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
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_init_hal_func_table))]
    static extern unsafe void init_hal_func_table(
        IntPtr hal_log_info,
        IntPtr hal_log_warn,
        IntPtr hal_log_error,
        IntPtr hal_log_out_of_memory,
        IntPtr hal_notify_image_update,
        IntPtr hal_notify_image_free,
        IntPtr hal_render_image_normal,
        IntPtr hal_render_image_add,
        IntPtr hal_render_image_sub,
        IntPtr hal_render_image_dim,
        IntPtr hal_render_image_rule,
        IntPtr hal_render_image_melt,
        IntPtr hal_render_image_cross,
        IntPtr hal_render_image_3d_normal,
        IntPtr hal_render_image_3d_add,
        IntPtr hal_render_image_3d_sub,
        IntPtr hal_render_image_3d_dim,
        IntPtr hal_render_image_3d_cross,
        IntPtr hal_reset_lap_timer,
        IntPtr hal_get_lap_timer_millisec,
        IntPtr hal_play_sound,
        IntPtr hal_stop_sound,
        IntPtr hal_set_sound_volume,
        IntPtr hal_is_sound_finished,
        IntPtr hal_play_video,
        IntPtr hal_stop_video,
        IntPtr hal_is_video_playing,
        IntPtr hal_is_full_screen_supported,
        IntPtr hal_is_full_screen_mode,
        IntPtr hal_enter_full_screen_mode,
        IntPtr hal_leave_full_screen_mode,
        IntPtr hal_get_system_language,
        IntPtr hal_set_continuous_swipe_enabled,
        IntPtr check_file_exist,
        IntPtr get_file_contents,
        IntPtr open_save_file,
        IntPtr write_save_file,
        IntPtr close_save_file,
        IntPtr free_shared);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_setup))]
    static extern unsafe int on_event_setup();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_start))]
    static extern unsafe int on_event_start();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_update))]
    static extern unsafe int on_event_update();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_render))]
    static extern unsafe int on_event_render();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_key_press))]
    static extern unsafe void on_event_key_press(int key);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_key_release))]
    static extern unsafe void on_event_key_release(int key);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_press))]
    static extern unsafe void on_event_mouse_press(int button, int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_release))]
    static extern unsafe void on_event_mouse_release(int button, int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_move))]
    static extern unsafe void on_event_mouse_move(int x, int y);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_mouse_wheel))]
    static extern unsafe void on_event_mouse_wheel(int v, int h);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_touch_cancel))]
    static extern unsafe void on_event_touch_cancel();

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_swipe_down))]
    static extern unsafe void on_event_swipe_down(float speed, float amount);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_swipe_up))]
    static extern unsafe void on_event_swipe_up(float speed, float amount);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_on_event_analog_input))]
    static extern unsafe void on_event_analog_input(int input, int val);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_hal_get_wave_samples))]
    public static extern unsafe int hal_get_wave_samples(byte *w, uint *buf, int samples);

#if HAL_UNITY_SWITCH || HAL_UNITY_PS5 || HAL_UNITY_GAMECORE_XBOXSERIES
    [DllImport("__Internal")]
#else
    [DllImport("libstrato")]
#endif
    [AOT.MonoPInvokeCallback(typeof(delegate_hal_is_wave_eos))]
    public static extern unsafe bool hal_is_wave_eos(byte *w);

    //
    // HAL functions
    //

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_log_info))]
    static unsafe void hal_log_info(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_log_warn))]
    static unsafe void hal_log_warn(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_log_error))]
    static unsafe void hal_log_error(byte *s)
    {
        string str = BytePtrToString(s);
        Debug.Log(str);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_log_out_of_memory))]
    static unsafe void hal_log_out_of_memory()
    {
        Debug.Log("Out of memory.");
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_notify_image_update))]
    static unsafe void hal_notify_image_update(int id, int width, int height, uint *pixels)
    {
        if (!imageDict.ContainsKey(id))
        {
            ManagedImage storeImage = new ManagedImage();
            storeImage.width = width;
            storeImage.height = height;
            storeImage.pixels = new Color[storeImage.width * storeImage.height];
            storeImage.texture = new Texture2D(storeImage.width, storeImage.height, TextureFormat.ARGB32, false);
            storeImage.texture.wrapMode = TextureWrapMode.Clamp;
			storeImage.texture.wrapModeU = TextureWrapMode.Clamp;
			storeImage.texture.wrapModeV = TextureWrapMode.Clamp;
			storeImage.texture.filterMode = FilterMode.Point;
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_notify_image_free))]
    static unsafe void hal_notify_image_free(int id)
    {
        if (imageDict.ContainsKey(id))
        {
            ManagedImage image = imageDict[id];
            MonoBehaviour.Destroy(image.texture);
            imageDict.Remove(id);
        }
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_normal))]
    static unsafe void hal_render_image_normal(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImageHelper(_instance._normalShader, dst_left, dst_top, dst_width, dst_height, src_img, -1, src_left, src_top, src_width, src_height, alpha);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_add))]
    static unsafe void hal_render_image_add(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImageHelper(_instance._addShader, dst_left, dst_top, dst_width, dst_height, src_img, -1, src_left, src_top, src_width, src_height, alpha);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_sub))]
    static unsafe void hal_render_image_sub(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImageHelper(_instance._subShader, dst_left, dst_top, dst_width, dst_height, src_img, -1, src_left, src_top, src_width, src_height, alpha);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_dim))]
    static unsafe void hal_render_image_dim(int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImageHelper(_instance._dimShader, dst_left, dst_top, dst_width, dst_height, src_img, -1, src_left, src_top, src_width, src_height, alpha);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_rule))]
    static unsafe void hal_render_image_rule(int src_img, int rule_img, int threshold)
    {
		DrawImageHelper(_instance._meltShader, 0, 0, viewportWidth, viewportHeight, src_img, rule_img, 0, 0, viewportWidth, viewportHeight, threshold);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_melt))]
    static unsafe void hal_render_image_melt(int src_img, int rule_img, int progress)
    {
		DrawImageHelper(_instance._meltShader, 0, 0, viewportWidth, viewportHeight, src_img, rule_img, 0, 0, viewportWidth, viewportHeight, progress);
    }

	static void DrawImageHelper(Shader shader, int dst_left, int dst_top, int dst_width, int dst_height, int src_img, int rule_img, int src_left, int src_top, int src_width, int src_height, int alpha)
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
            new Vector2((float)src_left / (float)srcImage.width,
			            (float)src_top / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width,
						(float)src_top / (float)srcImage.height),
            new Vector2((float)src_left / (float)srcImage.width,
						(float)(src_top + src_height) / (float)srcImage.height),
            new Vector2((float)(src_left + src_width) / (float)srcImage.width,
						(float)(src_top + src_height) / (float)srcImage.height)
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

        Material material = new Material(shader);
        material.mainTexture = srcImage.texture;
		if (rule_img != -1)
		{
			ManagedImage ruleImage;
			ruleImage = imageDict[rule_img];
			if (ruleImage.need_upload)
			{
				ruleImage.texture.SetPixels(ruleImage.pixels, 0);
				ruleImage.texture.Apply();
				ruleImage.need_upload = false;
				imageDict[rule_img] = ruleImage;
			}
			material.SetTexture("_RuleTex", ruleImage.texture);
		}

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv;
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_melt))]
    static unsafe void hal_render_image_cross(int src1_img, int src2_img, int src1_left, int src1_top, int src2_left, int src2_top, int alpha)
	{
        ManagedImage src1Image = imageDict[src1_img];
        if (src1Image.need_upload)
        {
            src1Image.texture.SetPixels(src1Image.pixels, 0);
            src1Image.texture.Apply();
            src1Image.need_upload = false;
            imageDict[src1_img] = src1Image;
        }

        ManagedImage src2Image = imageDict[src2_img];
        if (src2Image.need_upload)
        {
            src2Image.texture.SetPixels(src2Image.pixels, 0);
            src2Image.texture.Apply();
            src2Image.need_upload = false;
            imageDict[src2_img] = src2Image;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(-1.0f, -1.0f, 0),
            new Vector3( 1.0f, -1.0f, 0),
            new Vector3(-1.0f,  1.0f, 0),
            new Vector3( 1.0f,  1.0f, 0),
        };

        Vector2[] uv1 = new Vector2[] {
            new Vector2((float)-src1_left / (float)src1Image.width,
						(float)-src1_top / (float)src1Image.height),
            new Vector2((float)(viewportWidth - src1_left) / (float)src1Image.width,
						(float)-src1_top / (float)src1Image.height),
            new Vector2((float)-src1_left / (float)src1Image.width,
			            (float)(viewportHeight - src1_top) / (float)src1Image.height),
            new Vector2((float)(viewportWidth - src1_left) / (float)src1Image.width,
			            (float)(viewportHeight - src1_top) / (float)src1Image.height)
        };

        Vector2[] uv2 = new Vector2[] {
            new Vector2((float)-src2_left / (float)src2Image.width,
						(float)-src2_top / (float)src2Image.height),
            new Vector2((float)(viewportWidth - src2_left) / (float)src2Image.width,
						(float)-src2_top / (float)src2Image.height),
            new Vector2((float)-src2_left / (float)src2Image.width,
			            (float)(viewportHeight - src2_top) / (float)src2Image.height),
            new Vector2((float)(viewportWidth - src2_left) / (float)src2Image.width,
			            (float)(viewportHeight - src2_top) / (float)src2Image.height)
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

        Material material = new Material(_instance._meltShader);
        material.mainTexture = src1Image.texture;
        material.SetTexture("_Src2Tex", src2Image.texture);

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv1;
		mesh.SetUVs(1, uv2);
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
	}
	
    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_3d_normal))]
    static unsafe void hal_render_image_3d_normal(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImage3DHelper(_instance._normalShader, x1, y1, x2, y2, x3, y3, x4, y4, src_img, src_left, src_top, src_width, src_height, alpha);
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_3d_add))]
    static unsafe void hal_render_image_3d_add(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImage3DHelper(_instance._addShader, x1, y1, x2, y2, x3, y3, x4, y4, src_img, src_left, src_top, src_width, src_height, alpha);
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_3d_sub))]
    static unsafe void hal_render_image_3d_sub(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImage3DHelper(_instance._subShader, x1, y1, x2, y2, x3, y3, x4, y4, src_img, src_left, src_top, src_width, src_height, alpha);
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_3d_dim))]
    static unsafe void hal_render_image_3d_dim(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
    {
		DrawImage3DHelper(_instance._dimShader, x1, y1, x2, y2, x3, y3, x4, y4, src_img, src_left, src_top, src_width, src_height, alpha);
	}

    static unsafe void DrawImage3DHelper(Shader shader, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int src_img, int src_left, int src_top, int src_width, int src_height, int alpha)
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
			new Vector2((float)src_left / (float)srcImage.width,
			            (float)src_top / (float)srcImage.height),
			new Vector2((float)(src_left + src_width) / (float)srcImage.width,
						(float)src_top / (float)srcImage.height),
			new Vector2((float)src_left / (float)srcImage.width,
						(float)(src_top + src_height) / (float)srcImage.height),
			new Vector2((float)(src_left + src_width) / (float)srcImage.width,
						(float)(src_top + src_height) / (float)srcImage.height)
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

        Material material = new Material(shader);
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_render_image_3d_cross))]
    static unsafe void hal_render_image_3d_cross(int src1_image, int src2_image, float src1_x1, float src1_y1, float src1_x2, float src1_y2, float src1_x3, float src1_y3, float src1_x4, float src1_y4, float src2_x1, float src2_y1, float src2_x2, float src2_y2, float src2_x3, float src2_y3, float src2_x4, float src2_y4, int alpha)
	{
        ManagedImage src1Image = imageDict[src1_image];
        if (src1Image.need_upload)
        {
            src1Image.texture.SetPixels(src1Image.pixels, 0);
            src1Image.texture.Apply();
            src1Image.need_upload = false;
            imageDict[src1_image] = src1Image;
        }

        ManagedImage src2Image = imageDict[src2_image];
        if (src2Image.need_upload)
        {
            src2Image.texture.SetPixels(src2Image.pixels, 0);
            src2Image.texture.Apply();
            src2Image.need_upload = false;
            imageDict[src2_image] = src2Image;
        }

        Vector3[] vertices = new Vector3[] {
            new Vector3(-1.0f, -1.0f, 0),
            new Vector3((float)viewportWidth / (float)viewportWidth * 2.0f - 1.0f, -1.0f, 0),
            new Vector3(-1.0f, (float)viewportHeight / (float)viewportHeight * 2.0f - 1.0f, 0),
            new Vector3((float)viewportWidth / (float)viewportWidth * 2.0f - 1.0f, (float)viewportHeight / (float)viewportHeight * 2.0f - 1.0f, 0),
        };

		float[] s1_tx = new float[4];
        float[] s1_ty = new float[4];
		float[] s2_tx = new float[4];
        float[] s2_ty = new float[4];
		float[] screen_x = new float[] { 0.0f, (float)viewportWidth, 0.0f, (float)viewportWidth };
		float[] screen_y = new float[] { 0.0f, 0.0f, (float)viewportHeight, (float)viewportHeight };

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
					s1_tx[i] = a * (float)src1Image.width;
					s1_ty[i] = b * (float)src1Image.height;
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
					s2_tx[i] = a * (float)src2Image.width;
					s2_ty[i] = b * (float)src2Image.height;
				}
			} else {
				for (int i = 0; i < 4; i++) s2_tx[i] = s2_ty[i] = 0.0f;
			}
		}

        Vector2[] uv1 = new Vector2[] {
            new Vector2(s1_tx[0] / (float)src1Image.width,
						s1_ty[0] / (float)src1Image.height),
            new Vector2(s1_tx[1] / (float)src1Image.width,
						s1_ty[1] / (float)src1Image.height),
            new Vector2(s1_tx[2] / (float)src1Image.width,
						s1_ty[2] / (float)src1Image.height),
            new Vector2(s1_tx[3] / (float)src1Image.width,
						s1_ty[3] / (float)src1Image.height),
        };

        Vector2[] uv2 = new Vector2[] {
            new Vector2(s2_tx[0] / (float)src2Image.width,
						s2_ty[0] / (float)src2Image.height),
            new Vector2(s2_tx[1] / (float)src2Image.width,
						s2_ty[1] / (float)src2Image.height),
            new Vector2(s2_tx[2] / (float)src2Image.width,
						s2_ty[2] / (float)src2Image.height),
            new Vector2(s2_tx[3] / (float)src2Image.width,
						s2_ty[3] / (float)src2Image.height),
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

        Material material = new Material(_instance._meltShader);
        material.mainTexture = src1Image.texture;
        material.SetTexture("_Src2Tex", src2Image.texture);

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uv1;
		mesh.SetUVs(1, uv2);
        mesh.colors = colors;
        mesh.normals = normals;
        mesh.RecalculateBounds();

        _instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
	}

	static void UploadImageIfNeeded(int id)
	{
		ManagedImage img = imageDict[id];

		if (img.need_upload)
		{
			img.texture.SetPixels(img.pixels, 0);
			img.texture.Apply();
			img.need_upload = false;
			imageDict[id] = img;
		}
	}

	static float ToScreenX(float x)
	{
		return x / (float)viewportWidth * 2.0f - 1.0f;
	}

	static float ToScreenY(float y)
	{
		// HAL側は左上原点、Unity clip spaceは下が -1 / 上が +1
		return 1.0f - y / (float)viewportHeight * 2.0f;
	}

	static Vector3[] MakeRectVertices(float left, float top, float width, float height)
	{
		float x0 = ToScreenX(left);
		float x1 = ToScreenX(left + width);
		float y0 = ToScreenY(top);
		float y1 = ToScreenY(top + height);

		return new Vector3[] {
			new Vector3(x0, y0, 0), // top-left
			new Vector3(x1, y0, 0), // top-right
			new Vector3(x0, y1, 0), // bottom-left
			new Vector3(x1, y1, 0), // bottom-right
		};
	}

	static Vector3[] MakeQuadVertices(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
	{
		return new Vector3[] {
			new Vector3(ToScreenX(x1), ToScreenY(y1), 0),
			new Vector3(ToScreenX(x2), ToScreenY(y2), 0),
			new Vector3(ToScreenX(x3), ToScreenY(y3), 0),
			new Vector3(ToScreenX(x4), ToScreenY(y4), 0),
		};
	}

	static Vector2[] MakeRectUV(ManagedImage img, float left, float top, float width, float height)
	{
		float u0 = left / img.width;
		float u1 = (left + width) / img.width;

		// HAL側は左上原点、Unity UVは下原点
		float v0 = 1.0f - top / img.height;
		float v1 = 1.0f - (top + height) / img.height;

		return new Vector2[] {
			new Vector2(u0, v0), // top-left
			new Vector2(u1, v0), // top-right
			new Vector2(u0, v1), // bottom-left
			new Vector2(u1, v1), // bottom-right
		};
	}

	static readonly int[] RectTriangles = new int[] { 0, 1, 2, 1, 3, 2 };

	static readonly Vector3[] RectNormals = new Vector3[] {
		new Vector3(0, 0, -1),
		new Vector3(0, 0, -1),
		new Vector3(0, 0, -1),
		new Vector3(0, 0, -1)
	};

	static Color[] MakeAlphaColors(int alpha)
	{
		float a = alpha / 255.0f;

		return new Color[] {
			new Color(0, 0, 0, a),
			new Color(0, 0, 0, a),
			new Color(0, 0, 0, a),
			new Color(0, 0, 0, a)
		};
	}

	static void DrawTexturedQuad(
		Vector3[] vertices,
		Vector2[] uv,
		Color[] colors,
		Material material)
	{
		Mesh mesh = new Mesh();

		mesh.vertices = vertices;
		mesh.triangles = RectTriangles;
		mesh.uv = uv;
		mesh.colors = colors;
		mesh.normals = RectNormals;
		mesh.RecalculateBounds();

		_instance._commandBuffer.DrawMesh(mesh, Matrix4x4.identity, material);
	}

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_reset_lap_timer))]
    static unsafe void hal_reset_lap_timer(IntPtr origin)
    {
        Marshal.WriteInt64(origin, Environment.TickCount);
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_get_lap_timer_millisec))]
    static unsafe Int64 hal_get_lap_timer_millisec(IntPtr origin)
    {
        Int64 ret = Environment.TickCount - Marshal.ReadInt64(origin);
        return ret;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_play_sound))]
    static unsafe void hal_play_sound(int stream, byte *wave)
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_stop_sound))]
    static unsafe void hal_stop_sound(int stream)
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_set_sound_volume))]
    static unsafe void hal_set_sound_volume(int stream, float vol)
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_is_sound_finished))]
    static unsafe int hal_is_sound_finished(int stream)
    {
        if (stream == 0)
  		   return GameObject.Find("audio1").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else if (stream == 1)
            return GameObject.Find("audio2").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else if (stream == 2)
            return GameObject.Find("audio3").GetComponent<AudioSource>().isPlaying ? 1 : 0;
        else
            return GameObject.Find("audio4").GetComponent<AudioSource>().isPlaying ? 1 : 0;
		return 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_play_video))]
    static unsafe int hal_play_video(byte *fname, int is_skippable)
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_stop_video))]
    static unsafe void hal_stop_video()
    {
		_instance._videoPlayer.Stop();
		_instance._isVideoPlaying = false;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_is_video_playing))]
    static unsafe int hal_is_video_playing()
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

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_is_full_screen_supported))]
    static unsafe int hal_is_full_screen_supported()
    {
        // TODO
        return 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_is_full_screen_mode))]
    static unsafe int hal_is_full_screen_mode()
    {
        // TODO
        return 0;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_enter_full_screen_mode))]
    static unsafe void hal_enter_full_screen_mode()
    {
        // TODO
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_leave_full_screen_mode))]
    static unsafe void hal_leave_full_screen_mode()
    {
        // TODO
    }

    static unsafe IntPtr locale = IntPtr.Zero;

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_get_system_language))]
    static unsafe IntPtr hal_get_system_language()
    {
        // TODO
        if (locale == IntPtr.Zero)
        {
            locale = Marshal.StringToCoTaskMemUTF8("ja");
            GCHandle.Alloc(locale, GCHandleType.Pinned);
            GC.KeepAlive(locale);
        }
        return locale;
    }

    [AOT.MonoPInvokeCallback(typeof(delegate_hal_set_continuous_swipe_enabled))]
    static unsafe void hal_set_continuous_swipe_enabled(int is_enabled)
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
                StratoScript.hal_get_wave_samples(wave, (uint *)unsafePointer, samples);
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
            if (StratoScript.hal_is_wave_eos(wave))
                wave = null;
        }
    }
}
