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
 * | Y/M     | Series         | Models        | Video Chipset       | Connection        | Driver          |
 * |---------|----------------|---------------|---------------------|-------------------|-----------------|
 * | 1993/02 | MATE A         | Ae/M2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | Ae/M7         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | Ap/U2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/02 | MATE A         | As/U2         | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993/08 | MATE A         | Af/U9W        | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993    | MATE A         | Ap2           | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1993    | MATE A         | As2           | S3 86C928           | WAB Local-Bus     | WAB             |
 * | 1994/05 | MATE A         | An/U2         | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE A         | Ap3           | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE A         | As3           | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xn            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xs            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xp            | S3 Vision864        | WAB Local-Bus     | WAB             |
 * | 1994    | MATE X         | Xf            | Matrox MGA-II       | WAB Local-Bus     | WAB             |
 * | 1993    | MATE B         | Be            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1993    | MATE B         | Bp            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1993    | MATE B         | Bs            | Cirrus Logic GD5428 | WAB Emulation     | WAB             |
 * | 1994    | CanBe          | Cb            | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1994    | CanBe          | Cx            | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | CanBe          | Cx2           | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | CanBe          | Cb2           | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1995    | MATE X         | Xe10          | Cirrus Logic GD5430 | WAB Emulation     | WAB             |
 * | 1994    | 98NOTE         | Ne            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nf            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Np            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Ns            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nx            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Nd            | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1994    | 98NOTE         | Ne2           | Cirrus Logic GD5428 | WAB Emulation     | WAB LCD         |
 * | 1995/11 | CanBe          | Cb3           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995/11 | CanBe          | Cx3           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | ValueStar      | V7            | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V13           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V20           | Cirrus Logic GD5440 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | CanBe          | Cu10          | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | CanBe          | Ct16          | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | ValueStar      | V10           | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1996    | ValueStar      | V12           | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997    | ValueStar      | V16/S5P       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997/02 | MATE X         | Xc13/S5       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1997/02 | MATE X         | Xc16/M7       | Cirrus Logic GD5446 | Core-Graph Bridge | CGB Cirrus      |
 * | 1995    | 98NOTE         | Nb7           | Cirrus Logic GD7543 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Nb10          | Cirrus Logic GD7548 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Na13          | Cirrus Logic GD7548 | PCI               | PCI Cirrus LCD  |
 * | 1996    | 98NOTE         | Ls12          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Nr12          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Nr13          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Ls13          | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1997    | 98NOTE         | Ls150         | Cirrus Logic GD7555 | PCI               | PCI Cirrus LCD  |
 * | 1995    | MATE X         | Xt13          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xv13          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xt16          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1996    | MATE X         | Xv20          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1997    | MATE R         | Rv20          | Matrox MGA-2064W    | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V166          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V200          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1997    | ValueStar      | V233          | Matrox MGA-1064SG   | PCI               | PCI Matrox      |
 * | 1995    | 98NOTE         | Nx            | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Nd2           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Lt2           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Ne3           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Na7           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1995    | 98NOTE         | Na9           | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1996    | 98NOTE         | Na12          | Trident Cyber9320   | PCI               | PCI Trident LCD |
 * | 1996    | 98NOTE         | Na15          | Trident Cyber9382   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr15          | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr150         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nr166         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nw133         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1997    | 98NOTE         | Nw150         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1998    | 98NOTE         | Nr233         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1998    | 98NOTE         | Nr266         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1999    | 98NOTE         | Nr300         | Trident Cyber9385   | PCI               | PCI Trident LCD |
 * | 1995    | CanBe          | Cu13          | Trident TGUI9685    | PCI               | PCI Trident LCD |
 * | 1995    | CanBe          | Ct20          | Trident TGUI9685    | PCI               | PCI Trident CRT |
 * | 1995    | MATE X         | Xa7           | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1995    | MATE X         | Xa10          | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1996    | MATE X         | Xa12          | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1996    | MATE X         | Xa13          | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1996    | MATE X         | Xa16          | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1997/07 | MATE X         | Xc13/M7       | Trident TGUI9680XGi | PCI               | PCI Trident CRT |
 * | 1996    | ValueStar      | V16/M7        | Trident TGUI9682XGi | PCI               | PCI Trident CRT |
 * | 1997    | MATE R         | Ra266         | Trident TGUI9682XGi | PCI               | PCI Trident CRT |
 * | 1998    | MATE R         | Ra300         | Trident TGUI9682XGi | PCI               | PCI Trident CRT |
 * | 1998    | MATE R         | Ra333         | Trident TGUI9682XGi | PCI               | PCI Trident CRT |
 * | 2000    | MATE R         | Ra43          | Trident TGUI9682XGi | PCI               | PCI Trident CRT |
 */

#include <strato/c89compat.h>

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

bool cirrus_init_disp(int mode, int requested_bpp);
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
