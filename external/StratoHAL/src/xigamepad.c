/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Gamepad HAL for XInput
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

#include "stratohal/platform.h"
#include "xigamepad.h"

#include <windows.h>
#include <xinput.h>

VOID XInputUpdate(VOID)
{
    XINPUT_STATE state;
    DWORD result;

    result = XInputGetState(0, &state);
    if (result != ERROR_SUCCESS)
        return;

    hal_callback_on_event_analog_input(HAL_ANALOG_X1, state.Gamepad.sThumbLX);
    hal_callback_on_event_analog_input(HAL_ANALOG_Y1, state.Gamepad.sThumbLY);
    hal_callback_on_event_analog_input(HAL_ANALOG_X2, state.Gamepad.sThumbRX);
    hal_callback_on_event_analog_input(HAL_ANALOG_Y2, state.Gamepad.sThumbRY);
    hal_callback_on_event_analog_input(HAL_ANALOG_L, state.Gamepad.bLeftTrigger);
    hal_callback_on_event_analog_input(HAL_ANALOG_R, state.Gamepad.bRightTrigger);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_UP);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_UP);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_DOWN);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_DOWN);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_LEFT);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_LEFT);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_RIGHT);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_RIGHT);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_A);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_A);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_B);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_B);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_X);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_X);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_Y);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_Y);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_L);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_L);

    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        hal_callback_on_event_key_press(HAL_KEY_GAMEPAD_R);
    else
        hal_callback_on_event_key_release(HAL_KEY_GAMEPAD_R);
}
