/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * NEC PC-9821 Graphics
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

#ifndef STRATO_98DISP_H
#define STRATO_98DISP_H

#include <strato/c89compat.h>	/* int types */

/*
 * PC-9821 Graphics Architecture: WAB vs. PCI/BAR Access
 *
 * The following tables show whether the "WAB" (Window Accelerator
 * Bus/Board) legacy I/O ports (e.g., 00A8h family) can be used to
 * map and access the video hardware, or if modern "PCI BAR" (Base
 * Address Register) configuration is strictly required.
 *
 * Desktop models aggressively maintained hardware-level WAB emulation
 * (I/O port mapping) until the final generation to ensure backward
 * compatibility with legacy MS-DOS CAD software and early Windows 3.1
 * local-bus drivers.
 *
 * Since laptops lacked C-Bus/MATE expansion slots, NEC had no reason
 * to maintain heavy legacy emulation circuits. As the Pentium era
 * arrived, they aggressively dropped WAB port mapping in favor of
 * pure PCI (BAR-based) architectures.
 *
 * | Series             | Models                 | Video Chipset        | WAB Port Access |
 * |--------------------|------------------------|----------------------|-----------------|
 * | MATE A             | Ap, As, Ae, Ap2, As2   | S3 86C928            | Yes (Native)    |
 * |                    |                        | S3 86C805            |                 |
 * | MATE B             | Bp, Bs, Be             | S3 86C805            | Yes (Native)    |
 * | MATE X (Early)     | Xa, Xt, Xf, Xp         | S3 86C928            | Yes             |
 * |                    |                        | S3 Vision864         |                 |
 * |                    |                        |                      |                 |
 * | MATE X (Late)      | Xa7, Xa9, Xa10, Xt13   | S3 Trio64            | Yes             |
 * |                    |                        | S3 Millennium        |                 |
 * |                    |                        | S3 Other             |                 |
 * | MATE X (High)      | Xa12, Xa13, Xa16, Xt16 | Matrox MGA-2064W     | Yes             |
 * | MATE R (Final)     | Ra43, Ra33, Ra266      | Trident TGUI9682XGi  | Yes             |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | ValueStar          | V10, V16, V200         | Cirrus Logic GD-5440 | Yes             |
 * |                    |                        | Cirrus Logic GD-5446 |                 |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | CanBe              | Cb, Cx, Cf             | S3 Trio64            | Yes             |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | 98NOTE Early       | Ne, Np, Ns, Nd         | Cirrus Logic GD-5428 | Yes             |
 * |                    |                        |                      |                 |
 * | 98NOTE (Early-Mid) | Na7, Na9               | S3 86C868            | Partially Yes   |
 * |                    |                        | S3 Trio64            |                 |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | 98NOTE (Mid Gen)   | Nb10, Nr12             | Cirrus Logic GD-7548 | No (PCI BAR)    |
 * |                    |                        | Cirrus Logic GD-7555 |                 |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | 98NOTE (Late Gen)  | Nr13, Nr150, Nr166     | Trident Cyber9385    | No (PCI BAR)    |
 * |                    |                        |                      |                 |
 * |                    |                        |                      |                 |
 * | 98NOTE (Final Gen) | Nr233, Nr266, Nr300    | Trident Cyber9397    | No (PCI BAR)    |
 * |                    |                        |                      |                 |
 *
 */

/*
 * Screen mode selectors for cirrus_init_disp().
 * (Guarded in case the main code already defines them.)
 */
#ifndef DISP_640X480
#define DISP_640X480	0
#define DISP_800X600	1
#define DISP_1024X768	2
#define DISP_1280X1024	3
#endif

/*
 * GDC Driver
 */

bool gdc_init_disp(void);
bool gdc_cleanup_disp(void);
bool gdc_flip(void);

/*
 * Cirrus Driver
 */

bool cirrus_init_disp(int mode, int bpp);
void cirrus_cleanup_disp(void);
void cirrus_flip(void);

/*
 * S3 Driver
 */

bool s3_init_disp(int mode, int bpp);
void s3_cleanup_disp(void);
void s3_flip(void);

/*
 * Trident Driver
 */

bool trident_init_disp(int mode, int bpp);
void trident_cleanup_disp(void);
void trident_flip(void);

/*
 * Matrox Driver
 */

bool matrox_init_disp(int mode, int bpp);
void matrox_cleanup_disp(void);
void matrox_flip(void);

#endif
