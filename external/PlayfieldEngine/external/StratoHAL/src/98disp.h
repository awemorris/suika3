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
 * | Year   | Series          | Models        | Video Chipset               | Graphics              | Description                    |
 * |--------|-----------------|---------------|-----------------------------|-----------------------|--------------------------------|
 * | 1993   | MATE A          | Ae            | S3 86C928                   | WAB                   | Original PC-9821 WAB.          |
 * | 1993   | MATE A          | Ap            | S3 86C928                   | WAB                   |                                |
 * | 1993   | MATE A          | Ap2           | S3 86C928                   | WAB                   |                                |
 * | 1993   | MATE A          | As            | S3 86C928                   | WAB                   |                                |
 * | 1993   | MATE A          | As2           | S3 86C928                   | WAB                   |                                |
 * | 1993   | MATE B          | Be            | S3 86C805                   | WAB                   |                                |
 * | 1993   | MATE B          | Bp            | S3 86C805                   | WAB                   |                                |
 * | 1993   | MATE B          | Bs            | S3 86C805                   | WAB                   |                                |
 * | 1994   | MATE A          | An            | S3 Vision864                | WAB                   |                                |
 * | 1994   | MATE A          | Ap3           | S3 Vision864                | WAB                   |                                |
 * | 1994   | MATE A          | As3           | S3 Vision864                | WAB                   |                                |
 * | 1995   | 98NOTE          | Na7           | S3 86C868                   | WAB Reduced           | Simply ported GAB.             |
 * | 1995   | 98NOTE          | Na9           | S3 86C868                   | WAB Reduced           |                                |
 * | 1994   | MATE X          | Xa            | S3 Vision864                | WAB Emulation         | Moved to PCI translation.      |
 * | 1994   | MATE X          | Xf            | S3 Vision864                | WAB Emulation         |                                |
 * | 1994   | MATE X          | Xn            | S3 Vision864                | WAB Emulation         |                                |
 * | 1994   | MATE X          | Xp            | S3 Vision964                | WAB Emulation         |                                |
 * | 1994   | MATE X          | Xt            | S3 Vision964                | WAB Emulation         |                                |
 * | 1995   | MATE X          | Xa10          | S3 Trio64                   | WAB Emulation         |                                |
 * | 1995   | MATE X          | Xa7           | S3 Trio64                   | WAB Emulation         |                                |
 * | 1995   | MATE X          | Xa9           | S3 Trio64                   | WAB Emulation         |                                |
 * | 1995   | MATE X          | Xt13          | S3 Trio64                   | WAB Emulation         |                                |
 * | 1996   | MATE X          | Xa12/C        | S3 Trio64V+                 | WAB Emulation         |                                |
 * | 1996   | MATE X          | Xa13/W        | S3 Trio64V+                 | WAB Emulation         |                                |
 * | 1996   | MATE X          | Xv13          | S3 Trio64V+                 | WAB Emulation         |                                |
 * | 1996   | MATE X          | Xv20          | S3 Trio64V+                 | WAB Emulation         |                                |
 * | 1994   | 98NOTE          | Nd            | Cirrus Logic GD-5428        | Core-Graph Bridge     | Integrated GDC + Cirrus.       |
 * | 1994   | 98NOTE          | Ne            | Cirrus Logic GD-5428        | Core-Graph Bridge     |                                |
 * | 1994   | 98NOTE          | Nf            | Cirrus Logic GD-5428        | Core-Graph Bridge     |                                |
 * | 1994   | 98NOTE          | Np            | Cirrus Logic GD-5428        | Core-Graph Bridge     |                                |
 * | 1994   | 98NOTE          | Ns            | Cirrus Logic GD-5428        | Core-Graph Bridge     |                                |
 * | 1994   | 98NOTE          | Nx            | Cirrus Logic GD-5428        | Core-Graph Bridge     |                                |
 * | 1994   | CanBe           | Cb            | S3 Trio64                   | Core-Graph Bridge     | Used faster S3.                |
 * | 1994   | CanBe           | Cf            | S3 Trio64                   | Core-Graph Bridge     |                                |
 * | 1994   | CanBe           | Cx            | S3 Trio64                   | Core-Graph Bridge     |                                |
 * | 1995   | CanBe           | Ct            | S3 Trio64V+                 | Core-Graph Bridge     |                                |
 * | 1995   | CanBe           | Cu            | S3 Trio64V+                 | Core-Graph Bridge     |                                |
 * | 1996   | 98NOTE          | Na12          | S3 Trio64V+                 | Core-Graph Bridge     |                                |
 * | 1996   | 98NOTE          | Na13          | S3 Trio64                   | Core-Graph Bridge     |                                |
 * | 1996   | 98NOTE          | Na15          | S3 Trio64V+                 | Core-Graph Bridge     |                                |
 * | 1995   | ValueStar       | V10           | Cirrus Logic GD-5440        | Core-Graph Bridge     | Used cheap Cirrus for desktop. |
 * | 1995   | ValueStar       | V13           | Cirrus Logic GD-5440        | Core-Graph Bridge     |                                |
 * | 1995   | ValueStar       | V7            | Cirrus Logic GD-5430        | Core-Graph Bridge     |                                |
 * | 1995   | ValueStar       | V9            | Cirrus Logic GD-5430        | Core-Graph Bridge     |                                |
 * | 1996   | MATE X          | Xa12          | Matrox MGA-2064W            | Core-Graph Bridge     | Highend Matrox model.          |
 * | 1996   | MATE X          | Xa13          | Matrox MGA-2064W            | Core-Graph Bridge     |                                |
 * | 1996   | MATE X          | Xa16          | Matrox MGA-2064W            | Core-Graph Bridge     |                                |
 * | 1996   | 98NOTE          | Ls12          | NeoMagic MagicGraph128      | Core-Graph Bridge     |                                |
 * | 1996   | ValueStar       | V12           | Cirrus Logic GD-5446        | Core-Graph Bridge     |                                |
 * | 1996   | ValueStar       | V13           | Cirrus Logic GD-5446        | Core-Graph Bridge     |                                |
 * | 1996   | ValueStar       | V16           | Cirrus Logic GD-5446        | Core-Graph Bridge     |                                |
 * | 1995   | 98NOTE          | Nb10          | Cirrus Logic GD-7548        | PCI                   | Internal CG->Cirrus path.      |
 * | 1996   | 98NOTE          | Nr12          | Cirrus Logic GD-7555        | PCI                   |                                |
 * | 1996   | ValueStar       | V150          | Cirrus Logic GD-5446        | PCI                   | Core-Graph is independent.     |
 * | 1996   | ValueStar       | V166          | Cirrus Logic GD-5446        | PCI                   |                                |
 * | 1996   | ValueStar       | V200          | Cirrus Logic GD-5446        | PCI                   |                                |
 * | 1997   | ValueStar       | V233          | Cirrus Logic GD-5446        | PCI                   |                                |
 * | 1997   | MATE R          | Rv20          | Matrox MGA-1064SG           | PCI                   |                                |
 * | 1996   | MATE X          | Xt16          | Matrox MGA-2064W            | PCI                   |                                |
 * | 1997   | 98NOTE          | Ls150         | NeoMagic MagicGraph128      | PCI                   | Used PC/AT laptop de-facto.    |
 * | 1998   | 98NOTE          | Lw23          | NeoMagic MagicMedia256      | PCI                   |                                |
 * | 1998   | 98NOTE          | Lw26          | NeoMagic MagicMedia256      | PCI                   |                                |
 * | 1999   | 98NOTE          | Lw33          | NeoMagic MagicMedia256      | PCI                   |                                |
 * | 1997   | MATE R          | Ra266         | Trident TGUI9682XGi         | PCI                   | Used Trident overlay mixing.   |
 * | 1998   | MATE R          | Ra33          | Trident TGUI9682XGi         | PCI                   |                                |
 * | 1999   | MATE R          | Ra43          | Trident TGUI9682XGi         | PCI                   |                                |
 * | 1997   | 98NOTE          | Nr13          | Trident Cyber9385           | PCI                   |                                |
 * | 1997   | 98NOTE          | Nr150         | Trident Cyber9385           | PCI                   |                                |
 * | 1997   | 98NOTE          | Nr166         | Trident Cyber9385           | PCI                   |                                |
 * | 1998   | 98NOTE          | Nr233         | Trident Cyber9397           | PCI                   |                                |
 * | 1998   | 98NOTE          | Nr266         | Trident Cyber9397           | PCI                   |                                |
 * | 1999   | 98NOTE          | Nr300         | Trident Cyber9397           | PCI                   |                                |
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
