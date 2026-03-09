/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Main loop
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
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

#include "mainloop.h"
#include "api.h"
#include "vm.h"
#include "i18n.h"

#include <stdio.h>

/* Show whether the game is running. */
static bool is_running;

/* Show whether the game should start in the full screen mode. */
static bool is_fulscreen_start;

/* Game start time. */
static uint64_t lap_origin;

/* Screen size. */
static int screen_width;
static int screen_height;

/* Input states. */
int pf_mouse_pos_x;
int pf_mouse_pos_y;
bool pf_is_mouse_left_pressed;
bool pf_is_mouse_right_pressed;
bool pf_is_mouse_left_clicked;
bool pf_is_mouse_right_clicked;
bool pf_is_mouse_dragging;
bool pf_is_touch_canceled;
bool pf_is_swiped;
bool pf_is_gamepad_left_pressed;
bool pf_is_gamepad_right_pressed;
bool pf_is_gamepad_up_pressed;
bool pf_is_gamepad_down_pressed;
bool pf_is_gamepad_a_pressed;
bool pf_is_gamepad_b_pressed;
bool pf_is_gamepad_x_pressed;
bool pf_is_gamepad_y_pressed;
bool pf_is_gamepad_l_pressed;
bool pf_is_gamepad_r_pressed;
int pf_gamepad_analog_x1;
int pf_gamepad_analog_y1;
int pf_gamepad_analog_x2;
int pf_gamepad_analog_y2;
int pf_gamepad_analog_l;
int pf_gamepad_analog_r;
bool pf_is_escape_key_pressed;
bool pf_is_return_key_pressed;
bool pf_is_space_key_pressed;
bool pf_is_tab_key_pressed;
bool pf_is_backspace_key_pressed;
bool pf_is_delete_key_pressed;
bool pf_is_home_key_pressed;
bool pf_is_end_key_pressed;
bool pf_is_pageup_key_pressed;
bool pf_is_pagedown_key_pressed;
bool pf_is_shift_key_pressed;
bool pf_is_control_key_pressed;
bool pf_is_alt_key_pressed;
bool pf_is_left_key_pressed;
bool pf_is_right_key_pressed;
bool pf_is_up_key_pressed;
bool pf_is_down_key_pressed;
bool pf_is_a_key_pressed;
bool pf_is_b_key_pressed;
bool pf_is_c_key_pressed;
bool pf_is_d_key_pressed;
bool pf_is_e_key_pressed;
bool pf_is_f_key_pressed;
bool pf_is_g_key_pressed;
bool pf_is_h_key_pressed;
bool pf_is_i_key_pressed;
bool pf_is_j_key_pressed;
bool pf_is_k_key_pressed;
bool pf_is_l_key_pressed;
bool pf_is_m_key_pressed;
bool pf_is_n_key_pressed;
bool pf_is_o_key_pressed;
bool pf_is_p_key_pressed;
bool pf_is_q_key_pressed;
bool pf_is_r_key_pressed;
bool pf_is_s_key_pressed;
bool pf_is_t_key_pressed;
bool pf_is_u_key_pressed;
bool pf_is_v_key_pressed;
bool pf_is_w_key_pressed;
bool pf_is_x_key_pressed;
bool pf_is_y_key_pressed;
bool pf_is_z_key_pressed;
bool pf_is_1_key_pressed;
bool pf_is_2_key_pressed;
bool pf_is_3_key_pressed;
bool pf_is_4_key_pressed;
bool pf_is_5_key_pressed;
bool pf_is_6_key_pressed;
bool pf_is_7_key_pressed;
bool pf_is_8_key_pressed;
bool pf_is_9_key_pressed;
bool pf_is_0_key_pressed;
bool pf_is_f1_key_pressed;
bool pf_is_f2_key_pressed;
bool pf_is_f3_key_pressed;
bool pf_is_f4_key_pressed;
bool pf_is_f5_key_pressed;
bool pf_is_f6_key_pressed;
bool pf_is_f7_key_pressed;
bool pf_is_f8_key_pressed;
bool pf_is_f9_key_pressed;
bool pf_is_f10_key_pressed;
bool pf_is_f11_key_pressed;
bool pf_is_f12_key_pressed;

/*
 * This function is called when the app is going to be initialized.
 */
bool
hal_callback_on_event_boot(
	char **title,
	int *width,
	int *height)
{
	char *title_ret;
	int width_ret;
	int height_ret;
	bool fullscreen_ret;
	
#ifdef USE_TRANSLATION
	/* Initialize the locale. */
	pf_init_locale();
#endif

	/* Initialize the API. */
	if (!pfi_init_api())
		return false;

	/* Create a VM, then call setup(). */
	if (!pfi_create_vm(&title_ret, &width_ret, &height_ret, &fullscreen_ret))
		return false;

	/* Save the window size. */
	if (!pfi_set_vm_int("screenWidth", width_ret))
		return false;
	if (!pfi_set_vm_int("screenHeight", height_ret))
		return false;

	/* Make the exit flag. */
	if (!pfi_set_vm_int("exitFlag", 0))
		return false;

	if (title != NULL)
		*title = title_ret;
	if (width != NULL)
		*width = width_ret;
	if (height != NULL)
		*height = height_ret;

	is_fulscreen_start = fullscreen_ret;
	screen_width = width_ret;
	screen_height = height_ret;

	return true;
}

/*
 * This function is called right before the game loop.
 */
bool
hal_callback_on_event_start(void)
{
	if (is_fulscreen_start)
		hal_enter_full_screen_mode();

	/* Initialize the input states. */
	pf_mouse_pos_x = 0;
	pf_mouse_pos_y = 0;
	pf_is_mouse_left_pressed = false;
	pf_is_mouse_right_pressed = false;
	pf_is_mouse_left_clicked = false;
	pf_is_mouse_right_clicked = false;
	pf_is_mouse_dragging = false;
	pf_is_touch_canceled = false;
	pf_is_swiped = false;
	pf_is_gamepad_left_pressed = false;
	pf_is_gamepad_right_pressed = false;
	pf_is_gamepad_up_pressed = false;
	pf_is_gamepad_down_pressed = false;
	pf_is_gamepad_a_pressed = false;
	pf_is_gamepad_b_pressed = false;
	pf_is_gamepad_x_pressed = false;
	pf_is_gamepad_y_pressed = false;
	pf_is_gamepad_l_pressed = false;
	pf_is_gamepad_r_pressed = false;
	pf_gamepad_analog_x1 = 0;
	pf_gamepad_analog_y1 = 0;
	pf_gamepad_analog_x2 = 0;
	pf_gamepad_analog_y2 = 0;
	pf_gamepad_analog_l = 0;
	pf_gamepad_analog_r = 0;
	pf_is_escape_key_pressed = 0;
	pf_is_return_key_pressed = 0;
	pf_is_space_key_pressed = 0;
	pf_is_tab_key_pressed = 0;
	pf_is_backspace_key_pressed = 0;
	pf_is_delete_key_pressed = 0;
	pf_is_home_key_pressed = 0;
	pf_is_end_key_pressed = 0;
	pf_is_pageup_key_pressed = 0;
	pf_is_pagedown_key_pressed = 0;
	pf_is_shift_key_pressed = 0;
	pf_is_control_key_pressed = 0;
	pf_is_alt_key_pressed = 0;
	pf_is_left_key_pressed = 0;
	pf_is_right_key_pressed = 0;
	pf_is_up_key_pressed = 0;
	pf_is_down_key_pressed = 0;
	pf_is_a_key_pressed = 0;
	pf_is_b_key_pressed = 0;
	pf_is_c_key_pressed = 0;
	pf_is_d_key_pressed = 0;
	pf_is_e_key_pressed = 0;
	pf_is_f_key_pressed = 0;
	pf_is_g_key_pressed = 0;
	pf_is_h_key_pressed = 0;
	pf_is_i_key_pressed = 0;
	pf_is_j_key_pressed = 0;
	pf_is_k_key_pressed = 0;
	pf_is_l_key_pressed = 0;
	pf_is_m_key_pressed = 0;
	pf_is_n_key_pressed = 0;
	pf_is_o_key_pressed = 0;
	pf_is_p_key_pressed = 0;
	pf_is_q_key_pressed = 0;
	pf_is_r_key_pressed = 0;
	pf_is_s_key_pressed = 0;
	pf_is_t_key_pressed = 0;
	pf_is_u_key_pressed = 0;
	pf_is_v_key_pressed = 0;
	pf_is_w_key_pressed = 0;
	pf_is_z_key_pressed = 0;
	pf_is_1_key_pressed = 0;
	pf_is_2_key_pressed = 0;
	pf_is_3_key_pressed = 0;
	pf_is_4_key_pressed = 0;
	pf_is_5_key_pressed = 0;
	pf_is_6_key_pressed = 0;
	pf_is_7_key_pressed = 0;
	pf_is_8_key_pressed = 0;
	pf_is_9_key_pressed = 0;
	pf_is_0_key_pressed = 0;
	pf_is_f1_key_pressed = 0;
	pf_is_f2_key_pressed = 0;
	pf_is_f3_key_pressed = 0;
	pf_is_f4_key_pressed = 0;
	pf_is_f5_key_pressed = 0;
	pf_is_f6_key_pressed = 0;
	pf_is_f7_key_pressed = 0;
	pf_is_f8_key_pressed = 0;
	pf_is_f9_key_pressed = 0;
	pf_is_f10_key_pressed = 0;
	pf_is_f11_key_pressed = 0;
	pf_is_f12_key_pressed = 0;

	/* Initialize the API variables. */
	pfi_set_vm_int("mousePosX", 0);
	pfi_set_vm_int("mousePosY", 0);
	pfi_set_vm_int("millisec", 0);
	pfi_set_vm_int("isMouseLeftPressed", 0);
	pfi_set_vm_int("isMouseRightPressed", 0);
	pfi_set_vm_int("isMouseLeftClicked", 0);
	pfi_set_vm_int("isMouseRightClicked", 0);
	pfi_set_vm_int("isTouchCanceled", 0);
	pfi_set_vm_int("isSwiped", 0);
	pfi_set_vm_int("isEscapeKeyPressed", 0);
	pfi_set_vm_int("isReturnKeyPressed", 0);
	pfi_set_vm_int("isSpaceKeyPressed", 0);
	pfi_set_vm_int("isTabKeyPressed", 0);
	pfi_set_vm_int("isBackspaceKeyPressed", 0);
	pfi_set_vm_int("isDeleteKeyPressed", 0);
	pfi_set_vm_int("isHomeKeyPressed", 0);
	pfi_set_vm_int("isEndKeyPressed", 0);
	pfi_set_vm_int("isPageupKeyPressed", 0);
	pfi_set_vm_int("isPagedownKeyPressed", 0);
	pfi_set_vm_int("isShiftKeyPressed", 0);
	pfi_set_vm_int("isControlKeyPressed", 0);
	pfi_set_vm_int("isAltKeyPressed", 0);
	pfi_set_vm_int("isUpKeyPressed", 0);
	pfi_set_vm_int("isDownKeyPressed", 0);
	pfi_set_vm_int("isLeftKeyPressed", 0);
	pfi_set_vm_int("isRightKeyPressed", 0);
	pfi_set_vm_int("isAKeyPressed", 0);
	pfi_set_vm_int("isBKeyPressed", 0);
	pfi_set_vm_int("isCKeyPressed", 0);
	pfi_set_vm_int("isDKeyPressed", 0);
	pfi_set_vm_int("isEKeyPressed", 0);
	pfi_set_vm_int("isFKeyPressed", 0);
	pfi_set_vm_int("isGKeyPressed", 0);
	pfi_set_vm_int("isHKeyPressed", 0);
	pfi_set_vm_int("isIKeyPressed", 0);
	pfi_set_vm_int("isJKeyPressed", 0);
	pfi_set_vm_int("isKKeyPressed", 0);
	pfi_set_vm_int("isLKeyPressed", 0);
	pfi_set_vm_int("isMKeyPressed", 0);
	pfi_set_vm_int("isNKeyPressed", 0);
	pfi_set_vm_int("isOKeyPressed", 0);
	pfi_set_vm_int("isPKeyPressed", 0);
	pfi_set_vm_int("isQKeyPressed", 0);
	pfi_set_vm_int("isRKeyPressed", 0);
	pfi_set_vm_int("isSKeyPressed", 0);
	pfi_set_vm_int("isTKeyPressed", 0);
	pfi_set_vm_int("isUKeyPressed", 0);
	pfi_set_vm_int("isVKeyPressed", 0);
	pfi_set_vm_int("isWKeyPressed", 0);
	pfi_set_vm_int("isXKeyPressed", 0);
	pfi_set_vm_int("isYKeyPressed", 0);
	pfi_set_vm_int("isZKeyPressed", 0);
	pfi_set_vm_int("is1KeyPressed", 0);
	pfi_set_vm_int("is2KeyPressed", 0);
	pfi_set_vm_int("is3KeyPressed", 0);
	pfi_set_vm_int("is4KeyPressed", 0);
	pfi_set_vm_int("is5KeyPressed", 0);
	pfi_set_vm_int("is6KeyPressed", 0);
	pfi_set_vm_int("is7KeyPressed", 0);
	pfi_set_vm_int("is8KeyPressed", 0);
	pfi_set_vm_int("is9KeyPressed", 0);
	pfi_set_vm_int("is0KeyPressed", 0);
	pfi_set_vm_int("isF1KeyPressed", 0);
	pfi_set_vm_int("isF2KeyPressed", 0);
	pfi_set_vm_int("isF3KeyPressed", 0);
	pfi_set_vm_int("isF4KeyPressed", 0);
	pfi_set_vm_int("isF5KeyPressed", 0);
	pfi_set_vm_int("isF6KeyPressed", 0);
	pfi_set_vm_int("isF7KeyPressed", 0);
	pfi_set_vm_int("isF8KeyPressed", 0);
	pfi_set_vm_int("isF9KeyPressed", 0);
	pfi_set_vm_int("isF10KeyPressed", 0);
	pfi_set_vm_int("isF11KeyPressed", 0);
	pfi_set_vm_int("isF12KeyPressed", 0);
        pfi_set_vm_int("isGamepadLeftPressed", 0);
        pfi_set_vm_int("isGamepadRightPressed", 0);
        pfi_set_vm_int("isGamepadUpPressed", 0);
        pfi_set_vm_int("isGamepadDownPressed", 0);
        pfi_set_vm_int("isGamepadAPressed", 0);
        pfi_set_vm_int("isGamepadBPressed", 0);
        pfi_set_vm_int("isGamepadXPressed", 0);
        pfi_set_vm_int("isGamepadYPressed", 0);
        pfi_set_vm_int("isGamepadLPressed", 0);
        pfi_set_vm_int("isGamepadRPressed", 0);
	pfi_set_vm_int("gamepadAnalogX1", 0);
	pfi_set_vm_int("gamepadAnalogY1", 0);
	pfi_set_vm_int("gamepadAnalogX2", 0);
	pfi_set_vm_int("gamepadAnalogY2", 0);
	pfi_set_vm_int("gamepadAnalogL", 0);
	pfi_set_vm_int("gamepadAnalogR", 0);

	/* Initialize the lap timer. */
	hal_reset_lap_timer(&lap_origin);

	/* Initialize the upper layer. */
#ifdef PF_USE_INITHOOK
	pf_init_hook(screen_width, screen_height);
#endif

	/* Call start(). */
	if (!pfi_call_vm_function("start"))
		return false;

	is_running = true;

	return true;
}

/*
 * This function is called every frame periodically.
 */
bool
hal_callback_on_event_frame(void)
{
	int exit_flag;

	/* Get the lap timer. */
	pfi_set_vm_int("millisec", (int)hal_get_lap_timer_millisec(&lap_origin));

	/* Call update(). */
	if (!pfi_call_vm_function("update"))
		return false;

	/* Call render(). */
	if (!pfi_call_vm_function("render"))
		return false;

	/* Check the exit flag. */
	exit_flag = 0;
	pfi_get_vm_int("exitFlag", &exit_flag);
	if (exit_flag) {
		/* Exit the game loop. */
		return false;
	}

	/* Clear input states. */
	pf_is_mouse_left_clicked = false;
	pf_is_mouse_right_clicked = false;
	pf_is_touch_canceled = false;
	pf_is_swiped = false;
	pfi_set_vm_int("isMouseLeftClicked", 0);
	pfi_set_vm_int("isMouseRightClicked", 0);
	pfi_set_vm_int("isTouchCanceled", 0);
	pfi_set_vm_int("isSwiped", 0);

	/* Continue the game loop. */
	return true;
}

void
hal_callback_on_event_stop(void)
{
	/* Cleanup the API */
	pfi_cleanup_api();

	/* Destroy the VM. */
	pfi_destroy_vm();

	is_running = false;
}

void
hal_callback_on_event_key_press(
	int key)
{
	if (!is_running)
		return;

	switch (key) {
	case HAL_KEY_ESCAPE:
		pf_is_escape_key_pressed = true;
		pfi_set_vm_int("isEscapeKeyPressed", 1);
		break;
	case HAL_KEY_RETURN:
		pf_is_return_key_pressed = true;
		pfi_set_vm_int("isReturnKeyPressed", 1);
		break;
	case HAL_KEY_SPACE:
		pf_is_space_key_pressed = true;
		pfi_set_vm_int("isSpaceKeyPressed", 1);
		break;
	case HAL_KEY_TAB:
		pf_is_tab_key_pressed = true;
		pfi_set_vm_int("isTabKeyPressed", 1);
		break;
	case HAL_KEY_BACKSPACE:
		pf_is_backspace_key_pressed = true;
		pfi_set_vm_int("isBackspaceKeyPressed", 1);
		break;
	case HAL_KEY_DELETE:
		pf_is_delete_key_pressed = true;
		pfi_set_vm_int("isDeleteKeyPressed", 1);
		break;
	case HAL_KEY_HOME:
		pf_is_home_key_pressed = true;
		pfi_set_vm_int("isHomeKeyPressed", 1);
		break;
	case HAL_KEY_END:
		pf_is_end_key_pressed = true;
		pfi_set_vm_int("isEndKeyPressed", 1);
		break;
	case HAL_KEY_PAGEUP:
		pf_is_pageup_key_pressed = true;
		pfi_set_vm_int("isPageupKeyPressed", 1);
		break;
	case HAL_KEY_PAGEDOWN:
		pf_is_pagedown_key_pressed = true;
		pfi_set_vm_int("isPagedownKeyPressed", 1);
		break;
	case HAL_KEY_SHIFT:
		pf_is_shift_key_pressed = true;
		pfi_set_vm_int("isShiftKeyPressed", 1);
		break;
	case HAL_KEY_CONTROL:
		pf_is_control_key_pressed = true;
		pfi_set_vm_int("isControlKeyPressed", 1);
		break;
	case HAL_KEY_ALT:
		pf_is_alt_key_pressed = true;
		pfi_set_vm_int("isAltKeyPressed", 1);
		break;
	case HAL_KEY_LEFT:
		pf_is_left_key_pressed = true;
		pfi_set_vm_int("isLeftKeyPressed", 1);
		break;
	case HAL_KEY_RIGHT:
		pf_is_right_key_pressed = true;
		pfi_set_vm_int("isRightKeyPressed", 1);
		break;
	case HAL_KEY_UP:
		pf_is_up_key_pressed = true;
		pfi_set_vm_int("isUpKeyPressed", 1);
		break;
	case HAL_KEY_DOWN:
		pf_is_down_key_pressed = true;
		pfi_set_vm_int("isDownKeyPressed", 1);
		break;
	case HAL_KEY_A:
		pf_is_a_key_pressed = true;
		pfi_set_vm_int("isAKeyPressed", 1);
		break;
	case HAL_KEY_B:
		pf_is_b_key_pressed = true;
		pfi_set_vm_int("isBKeyPressed", 1);
		break;
	case HAL_KEY_C:
		pf_is_c_key_pressed = true;
		pfi_set_vm_int("isCKeyPressed", 1);
		break;
	case HAL_KEY_D:
		pf_is_d_key_pressed = true;
		pfi_set_vm_int("isDKeyPressed", 1);
		break;
	case HAL_KEY_E:
		pf_is_e_key_pressed = true;
		pfi_set_vm_int("isEKeyPressed", 1);
		break;
	case HAL_KEY_F:
		pf_is_f_key_pressed = true;
		pfi_set_vm_int("isFKeyPressed", 1);
		break;
	case HAL_KEY_G:
		pf_is_g_key_pressed = true;
		pfi_set_vm_int("isGKeyPressed", 1);
		break;
	case HAL_KEY_H:
		pf_is_h_key_pressed = true;
		pfi_set_vm_int("isHKeyPressed", 1);
		break;
	case HAL_KEY_I:
		pf_is_i_key_pressed = true;
		pfi_set_vm_int("isIKeyPressed", 1);
		break;
	case HAL_KEY_J:
		pf_is_j_key_pressed = true;
		pfi_set_vm_int("isJKeyPressed", 1);
		break;
	case HAL_KEY_K:
		pf_is_k_key_pressed = true;
		pfi_set_vm_int("isKKeyPressed", 1);
		break;
	case HAL_KEY_L:
		pf_is_l_key_pressed = true;
		pfi_set_vm_int("isLKeyPressed", 1);
		break;
	case HAL_KEY_M:
		pf_is_m_key_pressed = true;
		pfi_set_vm_int("isMKeyPressed", 1);
		break;
	case HAL_KEY_N:
		pf_is_n_key_pressed = true;
		pfi_set_vm_int("isNKeyPressed", 1);
		break;
	case HAL_KEY_O:
		pf_is_o_key_pressed = true;
		pfi_set_vm_int("isOKeyPressed", 1);
		break;
	case HAL_KEY_P:
		pf_is_p_key_pressed = true;
		pfi_set_vm_int("isPKeyPressed", 1);
		break;
	case HAL_KEY_Q:
		pf_is_q_key_pressed = true;
		pfi_set_vm_int("isQKeyPressed", 1);
		break;
	case HAL_KEY_R:
		pf_is_r_key_pressed = true;
		pfi_set_vm_int("isRKeyPressed", 1);
		break;
	case HAL_KEY_S:
		pf_is_s_key_pressed = true;
		pfi_set_vm_int("isSKeyPressed", 1);
		break;
	case HAL_KEY_T:
		pf_is_t_key_pressed = true;
		pfi_set_vm_int("isTKeyPressed", 1);
		break;
	case HAL_KEY_U:
		pf_is_u_key_pressed = true;
		pfi_set_vm_int("isUKeyPressed", 1);
		break;
	case HAL_KEY_V:
		pf_is_v_key_pressed = true;
		pfi_set_vm_int("isVKeyPressed", 1);
		break;
	case HAL_KEY_W:
		pf_is_w_key_pressed = true;
		pfi_set_vm_int("isWKeyPressed", 1);
		break;
	case HAL_KEY_X:
		pf_is_x_key_pressed = true;
		pfi_set_vm_int("isXKeyPressed", 1);
		break;
	case HAL_KEY_Y:
		pf_is_y_key_pressed = true;
		pfi_set_vm_int("isYKeyPressed", 1);
		break;
	case HAL_KEY_Z:
		pf_is_z_key_pressed = true;
		pfi_set_vm_int("isZKeyPressed", 1);
		break;
	case HAL_KEY_1:
		pf_is_1_key_pressed = true;
		pfi_set_vm_int("is1KeyPressed", 1);
		break;
	case HAL_KEY_2:
		pf_is_2_key_pressed = true;
		pfi_set_vm_int("is2KeyPressed", 1);
		break;
	case HAL_KEY_3:
		pf_is_3_key_pressed = true;
		pfi_set_vm_int("is3KeyPressed", 1);
		break;
	case HAL_KEY_4:
		pf_is_4_key_pressed = true;
		pfi_set_vm_int("is4KeyPressed", 1);
		break;
	case HAL_KEY_5:
		pf_is_5_key_pressed = true;
		pfi_set_vm_int("is5KeyPressed", 1);
		break;
	case HAL_KEY_6:
		pf_is_6_key_pressed = true;
		pfi_set_vm_int("is6KeyPressed", 1);
		break;
	case HAL_KEY_7:
		pf_is_7_key_pressed = true;
		pfi_set_vm_int("is7KeyPressed", 1);
		break;
	case HAL_KEY_8:
		pf_is_8_key_pressed = true;
		pfi_set_vm_int("is8KeyPressed", 1);
		break;
	case HAL_KEY_9:
		pf_is_9_key_pressed = true;
		pfi_set_vm_int("is9KeyPressed", 1);
		break;
	case HAL_KEY_0:
		pf_is_0_key_pressed = true;
		pfi_set_vm_int("is0KeyPressed", 1);
		break;
	case HAL_KEY_F1:
		pf_is_f1_key_pressed = true;
		pfi_set_vm_int("isF1KeyPressed", 1);
		break;
	case HAL_KEY_F2:
		pf_is_f2_key_pressed = true;
		pfi_set_vm_int("isF2KeyPressed", 1);
		break;
	case HAL_KEY_F3:
		pf_is_f3_key_pressed = true;
		pfi_set_vm_int("isF3KeyPressed", 1);
		break;
	case HAL_KEY_F4:
		pf_is_f4_key_pressed = true;
		pfi_set_vm_int("isF4KeyPressed", 1);
		break;
	case HAL_KEY_F5:
		pf_is_f5_key_pressed = true;
		pfi_set_vm_int("isF5KeyPressed", 1);
		break;
	case HAL_KEY_F6:
		pf_is_f6_key_pressed = true;
		pfi_set_vm_int("isF6KeyPressed", 1);
		break;
	case HAL_KEY_F7:
		pf_is_f7_key_pressed = true;
		pfi_set_vm_int("isF7KeyPressed", 1);
		break;
	case HAL_KEY_F8:
		pf_is_f8_key_pressed = true;
		pfi_set_vm_int("isF8KeyPressed", 1);
		break;
	case HAL_KEY_F9:
		pf_is_f9_key_pressed = true;
		pfi_set_vm_int("isF9KeyPressed", 1);
		break;
	case HAL_KEY_F10:
		pf_is_f10_key_pressed = true;
		pfi_set_vm_int("isF10KeyPressed", 1);
		break;
	case HAL_KEY_F11:
		pf_is_f11_key_pressed = true;
		pfi_set_vm_int("isF11KeyPressed", 1);
		break;
	case HAL_KEY_F12:
		pf_is_f12_key_pressed = true;
		pfi_set_vm_int("isF12KeyPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_LEFT:
		pf_is_gamepad_left_pressed = true;
		pfi_set_vm_int("isGamepadLeftPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_RIGHT:
		pf_is_gamepad_right_pressed = true;
		pfi_set_vm_int("isGamepadRightPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_UP:
		pf_is_gamepad_up_pressed = true;
		pfi_set_vm_int("isGamepadUpPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_DOWN:
		pf_is_gamepad_down_pressed = true;
		pfi_set_vm_int("isGamepadDownPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_A:
		pf_is_gamepad_a_pressed = true;
		pfi_set_vm_int("isGamepadAPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_B:
		pf_is_gamepad_b_pressed = true;
		pfi_set_vm_int("isGamepadBPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_X:
		pf_is_gamepad_x_pressed = true;
		pfi_set_vm_int("isGamepadXPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_Y:
		pf_is_gamepad_y_pressed = true;
		pfi_set_vm_int("isGamepadYPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_L:
		pf_is_gamepad_l_pressed = true;
		pfi_set_vm_int("isGamepadLPressed", 1);
		break;
	case HAL_KEY_GAMEPAD_R:
		pf_is_gamepad_r_pressed = true;
		pfi_set_vm_int("isGamepadRPressed", 1);
		break;
	default:
		break;
	}
}

void
hal_callback_on_event_key_release(
	int key)
{
	if (!is_running)
		return;

	switch (key) {
	case HAL_KEY_ESCAPE:
		pf_is_escape_key_pressed = false;
		pfi_set_vm_int("isEscapeKeyPressed", 0);
		break;
	case HAL_KEY_RETURN:
		pfi_set_vm_int("isReturnKeyPressed", 0);
		break;
	case HAL_KEY_SPACE:
		pf_is_space_key_pressed = false;
		pfi_set_vm_int("isSpaceKeyPressed", 0);
		break;
	case HAL_KEY_TAB:
		pf_is_tab_key_pressed = false;
		pfi_set_vm_int("isTabKeyPressed", 0);
		break;
	case HAL_KEY_BACKSPACE:
		pf_is_backspace_key_pressed = false;
		pfi_set_vm_int("isBackspaceKeyPressed", 0);
		break;
	case HAL_KEY_DELETE:
		pf_is_delete_key_pressed = false;
		pfi_set_vm_int("isDeleteKeyPressed", 0);
		break;
	case HAL_KEY_HOME:
		pf_is_home_key_pressed = false;
		pfi_set_vm_int("isHomeKeyPressed", 0);
		break;
	case HAL_KEY_END:
		pf_is_end_key_pressed = false;
		pfi_set_vm_int("isEndKeyPressed", 0);
		break;
	case HAL_KEY_PAGEUP:
		pf_is_pageup_key_pressed = false;
		pfi_set_vm_int("isPageupKeyPressed", 0);
		break;
	case HAL_KEY_PAGEDOWN:
		pf_is_pagedown_key_pressed = false;
		pfi_set_vm_int("isPagedownKeyPressed", 0);
		break;
	case HAL_KEY_SHIFT:
		pf_is_shift_key_pressed = false;
		pfi_set_vm_int("isShiftKeyPressed", 0);
		break;
	case HAL_KEY_CONTROL:
		pf_is_control_key_pressed = false;
		pfi_set_vm_int("isControlKeyPressed", 0);
		break;
	case HAL_KEY_ALT:
		pf_is_alt_key_pressed = false;
		pfi_set_vm_int("isAltKeyPressed", 0);
		break;
	case HAL_KEY_LEFT:
		pf_is_left_key_pressed = false;
		pfi_set_vm_int("isLeftKeyPressed", 0);
		break;
	case HAL_KEY_RIGHT:
		pf_is_right_key_pressed = false;
		pfi_set_vm_int("isRightKeyPressed", 0);
		break;
	case HAL_KEY_UP:
		pf_is_up_key_pressed = false;
		pfi_set_vm_int("isUpKeyPressed", 0);
		break;
	case HAL_KEY_DOWN:
		pf_is_down_key_pressed = false;
		pfi_set_vm_int("isDownKeyPressed", 0);
		break;
	case HAL_KEY_A:
		pf_is_a_key_pressed = false;
		pfi_set_vm_int("isAKeyPressed", 0);
		break;
	case HAL_KEY_B:
		pf_is_b_key_pressed = false;
		pfi_set_vm_int("isBKeyPressed", 0);
		break;
	case HAL_KEY_C:
		pf_is_c_key_pressed = false;
		pfi_set_vm_int("isCKeyPressed", 0);
		break;
	case HAL_KEY_D:
		pf_is_d_key_pressed = false;
		pfi_set_vm_int("isDKeyPressed", 0);
		break;
	case HAL_KEY_E:
		pf_is_e_key_pressed = false;
		pfi_set_vm_int("isEKeyPressed", 0);
		break;
	case HAL_KEY_F:
		pf_is_f_key_pressed = false;
		pfi_set_vm_int("isFKeyPressed", 0);
		break;
	case HAL_KEY_G:
		pf_is_g_key_pressed = false;
		pfi_set_vm_int("isGKeyPressed", 0);
		break;
	case HAL_KEY_H:
		pf_is_h_key_pressed = false;
		pfi_set_vm_int("isHKeyPressed", 0);
		break;
	case HAL_KEY_I:
		pf_is_i_key_pressed = false;
		pfi_set_vm_int("isIKeyPressed", 0);
		break;
	case HAL_KEY_J:
		pf_is_j_key_pressed = false;
		pfi_set_vm_int("isJKeyPressed", 0);
		break;
	case HAL_KEY_K:
		pf_is_k_key_pressed = false;
		pfi_set_vm_int("isKKeyPressed", 0);
		break;
	case HAL_KEY_L:
		pf_is_l_key_pressed = false;
		pfi_set_vm_int("isLKeyPressed", 0);
		break;
	case HAL_KEY_M:
		pf_is_m_key_pressed = false;
		pfi_set_vm_int("isMKeyPressed", 0);
		break;
	case HAL_KEY_N:
		pf_is_n_key_pressed = false;
		pfi_set_vm_int("isNKeyPressed", 0);
		break;
	case HAL_KEY_O:
		pf_is_o_key_pressed = false;
		pfi_set_vm_int("isOKeyPressed", 0);
		break;
	case HAL_KEY_P:
		pf_is_p_key_pressed = false;
		pfi_set_vm_int("isPKeyPressed", 0);
		break;
	case HAL_KEY_Q:
		pf_is_q_key_pressed = false;
		pfi_set_vm_int("isQKeyPressed", 0);
		break;
	case HAL_KEY_R:
		pf_is_r_key_pressed = false;
		pfi_set_vm_int("isRKeyPressed", 0);
		break;
	case HAL_KEY_S:
		pf_is_s_key_pressed = false;
		pfi_set_vm_int("isSKeyPressed", 0);
		break;
	case HAL_KEY_T:
		pf_is_t_key_pressed = false;
		pfi_set_vm_int("isTKeyPressed", 0);
		break;
	case HAL_KEY_U:
		pf_is_u_key_pressed = false;
		pfi_set_vm_int("isUKeyPressed", 0);
		break;
	case HAL_KEY_V:
		pf_is_v_key_pressed = false;
		pfi_set_vm_int("isVKeyPressed", 0);
		break;
	case HAL_KEY_W:
		pf_is_w_key_pressed = false;
		pfi_set_vm_int("isWKeyPressed", 0);
		break;
	case HAL_KEY_X:
		pf_is_x_key_pressed = false;
		pfi_set_vm_int("isXKeyPressed", 0);
		break;
	case HAL_KEY_Y:
		pf_is_y_key_pressed = false;
		pfi_set_vm_int("isYKeyPressed", 0);
		break;
	case HAL_KEY_Z:
		pf_is_z_key_pressed = false;
		pfi_set_vm_int("isZKeyPressed", 0);
		break;
	case HAL_KEY_1:
		pf_is_1_key_pressed = false;
		pfi_set_vm_int("is1KeyPressed", 0);
		break;
	case HAL_KEY_2:
		pf_is_2_key_pressed = false;
		pfi_set_vm_int("is2KeyPressed", 0);
		break;
	case HAL_KEY_3:
		pf_is_3_key_pressed = false;
		pfi_set_vm_int("is3KeyPressed", 0);
		break;
	case HAL_KEY_4:
		pf_is_4_key_pressed = false;
		pfi_set_vm_int("is4KeyPressed", 0);
		break;
	case HAL_KEY_5:
		pf_is_5_key_pressed = false;
		pfi_set_vm_int("is5KeyPressed", 0);
		break;
	case HAL_KEY_6:
		pf_is_6_key_pressed = false;
		pfi_set_vm_int("is6KeyPressed", 0);
		break;
	case HAL_KEY_7:
		pf_is_7_key_pressed = false;
		pfi_set_vm_int("is7KeyPressed", 0);
		break;
	case HAL_KEY_8:
		pf_is_8_key_pressed = false;
		pfi_set_vm_int("is8KeyPressed", 0);
		break;
	case HAL_KEY_9:
		pf_is_9_key_pressed = false;
		pfi_set_vm_int("is9KeyPressed", 0);
		break;
	case HAL_KEY_0:
		pf_is_0_key_pressed = false;
		pfi_set_vm_int("is0KeyPressed", 0);
		break;
	case HAL_KEY_F1:
		pf_is_f1_key_pressed = false;
		pfi_set_vm_int("isF1KeyPressed", 0);
		break;
	case HAL_KEY_F2:
		pf_is_f2_key_pressed = false;
		pfi_set_vm_int("isF2KeyPressed", 0);
		break;
	case HAL_KEY_F3:
		pf_is_f3_key_pressed = false;
		pfi_set_vm_int("isF3KeyPressed", 0);
		break;
	case HAL_KEY_F4:
		pf_is_f4_key_pressed = false;
		pfi_set_vm_int("isF4KeyPressed", 0);
		break;
	case HAL_KEY_F5:
		pf_is_f5_key_pressed = false;
		pfi_set_vm_int("isF5KeyPressed", 0);
		break;
	case HAL_KEY_F6:
		pf_is_f6_key_pressed = false;
		pfi_set_vm_int("isF6KeyPressed", 0);
		break;
	case HAL_KEY_F7:
		pf_is_f7_key_pressed = false;
		pfi_set_vm_int("isF7KeyPressed", 0);
		break;
	case HAL_KEY_F8:
		pf_is_f8_key_pressed = false;
		pfi_set_vm_int("isF8KeyPressed", 0);
		break;
	case HAL_KEY_F9:
		pf_is_f9_key_pressed = false;
		pfi_set_vm_int("isF9KeyPressed", 0);
		break;
	case HAL_KEY_F10:
		pf_is_f10_key_pressed = false;
		pfi_set_vm_int("isF10KeyPressed", 0);
		break;
	case HAL_KEY_F11:
		pf_is_f11_key_pressed = false;
		pfi_set_vm_int("isF11KeyPressed", 0);
		break;
	case HAL_KEY_F12:
		pf_is_f12_key_pressed = false;
		pfi_set_vm_int("isF12KeyPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_LEFT:
		pf_is_gamepad_left_pressed = false;
		pfi_set_vm_int("isGamepadLeftPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_RIGHT:
		pf_is_gamepad_right_pressed = false;
		pfi_set_vm_int("isGamepadRightPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_UP:
		pf_is_gamepad_up_pressed = false;
		pfi_set_vm_int("isGamepadUpPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_DOWN:
		pf_is_gamepad_down_pressed = false;
		pfi_set_vm_int("isGamepadDownPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_A:
		pf_is_gamepad_a_pressed = false;
		pfi_set_vm_int("isGamepadAPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_B:
		pf_is_gamepad_b_pressed = false;
		pfi_set_vm_int("isGamepadBPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_X:
		pf_is_gamepad_x_pressed = false;
		pfi_set_vm_int("isGamepadXPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_Y:
		pf_is_gamepad_y_pressed = false;
		pfi_set_vm_int("isGamepadYPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_L:
		pf_is_gamepad_l_pressed = false;
		pfi_set_vm_int("isGamepadLPressed", 0);
		break;
	case HAL_KEY_GAMEPAD_R:
		pf_is_gamepad_r_pressed = false;
		pfi_set_vm_int("isGamepadRPressed", 0);
		break;
	default:
		break;
	}
}

void
hal_callback_on_event_mouse_press(
	int button,
	int x,
	int y)
{
	if (is_running) {
		if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
			return;

		pf_mouse_pos_x = x;
		pf_mouse_pos_y = y;
		pfi_set_vm_int("mousePosX", x);
		pfi_set_vm_int("mousePosY", y);

		if (button == HAL_MOUSE_LEFT) {
			pf_is_mouse_left_pressed = true;
			pfi_set_vm_int("isMouseLeftPressed", 1);

			pf_is_mouse_dragging = true;
			pfi_set_vm_int("isMouseDragging", 1);
		} else {
			pf_is_mouse_right_pressed = true;
			pfi_set_vm_int("isMouseRightPressed", 1);
		}
	}
}

void
hal_callback_on_event_mouse_release(
	int button,
	int x,
	int y)
{
	if (is_running) {
		pf_mouse_pos_x = x;
		pf_mouse_pos_y = y;
		pfi_set_vm_int("mousePosX", x);
		pfi_set_vm_int("mousePosY", y);

		pf_is_mouse_dragging = false;
		pfi_set_vm_int("isMouseDragging", 1);

		if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
			return;

		if (button == HAL_MOUSE_LEFT) {
			pf_is_mouse_left_pressed = false;
			pf_is_mouse_left_clicked = true;
			pfi_set_vm_int("isMouseLeftPressed", 0);
			pfi_set_vm_int("isMouseLeftClicked", 1);
		} else {
			pf_is_mouse_right_pressed = false;
			pf_is_mouse_right_clicked = true;
			pfi_set_vm_int("isMouseRightPressed", 0);
			pfi_set_vm_int("isMouseRightClicked", 1);
		}
	}
}

void
hal_callback_on_event_mouse_move(
	int x,
	int y)
{
	if (is_running) {
		pf_mouse_pos_x = x;
		pf_mouse_pos_y = y;
		pfi_set_vm_int("mousePosX", x);
		pfi_set_vm_int("mousePosY", y);
	}
}

void
hal_callback_on_event_analog_input(
	int input,
	int val)
{
	if (is_running) {
		switch (input) {
		case HAL_ANALOG_X1:
			pf_gamepad_analog_x1 = val;
			pfi_set_vm_int("gamepadAnalogX1", val);
			break;
		case HAL_ANALOG_Y1:
			pf_gamepad_analog_y1 = val;
			pfi_set_vm_int("gamepadAnalogY1", val);
			break;
		case HAL_ANALOG_X2:
			pf_gamepad_analog_x2 = val;
			pfi_set_vm_int("gamepadAnalogX2", val);
			break;
		case HAL_ANALOG_Y2:
			pf_gamepad_analog_y2 = val;
			pfi_set_vm_int("gamepadAnalogY2", val);
			break;
		case HAL_ANALOG_L:
			pf_gamepad_analog_l = val;
			pfi_set_vm_int("gamepadAnalogL", val);
			break;
		case HAL_ANALOG_R:
			pf_gamepad_analog_r = val;
			pfi_set_vm_int("gamepadAnalogR", val);
			break;
		default:
			break;
		}
	}
}

void
hal_callback_on_event_touch_cancel(void)
{
	if (is_running) {
		pf_is_mouse_left_pressed = false;
		pf_is_touch_canceled = true;
		pfi_set_vm_int("isMouseLeftPressed", 0);
		pfi_set_vm_int("isTouchCanceled", 1);
	}
}

void
hal_callback_on_event_swipe_down(void)
{
	if (is_running) {
		pf_is_mouse_left_pressed = false;
		pf_is_swiped = true;
		pfi_set_vm_int("isMouseLeftPressed", 0);
		pfi_set_vm_int("isSwiped", 1);
	}
}

void
hal_callback_on_event_swipe_up(void)
{
	if (is_running) {
		pf_is_mouse_left_pressed = false;
		pf_is_swiped = true;
		pfi_set_vm_int("isMouseLeftPressed", 0);
		pfi_set_vm_int("isSwiped", 1);
	}
}
