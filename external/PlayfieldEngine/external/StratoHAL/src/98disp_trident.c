/* -*- tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Strato HAL
 * Trident TGUI96xx display driver for NEC PC-98 (DOS/4G).
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

/*
 * ===========================================================================
 * PC-98 built-in Trident graphics: analysis summary
 * ===========================================================================
 *
 * Target machines (desktops with the on-board Trident):
 *
 *   MATE R Ra43/Ra33/Ra266/Ra300  Trident TGUI9682XGi, VRAM 2MB
 *
 * The single most important discovery: XFree86 3.3.x shipped a
 * dedicated PC-98 glue driver for exactly these built-ins,
 *   xc/programs/Xserver/hw/xfree98/vga256/drivers/trident/pc98_tgui.c
 * ("NEC Trident TGUi96xx(PCI Bus Type)"), written by people with the
 * real V13/V16.  Everything below that is not from the generic
 * Trident literature comes from that file.
 *
 * ---------------------------------------------------------------------------
 * A. Bus attachment - this is a PCI chip, not a WAB one
 * ---------------------------------------------------------------------------
 *  - The on-board TGUI96xx is a PCI device: vendor 1023h, device
 *    9660h.  The chip generation is in the PCI revision ID, mirrored
 *    in SR09:  00h = TGUI9660, 01h = TGUI9680, 10h = ProVidia 9682,
 *    21h = ProVidia 9685.  (V16 -> 01h expected, Ra43 -> 10h.)
 *  - BAR0 is the 4MB(+) memory range; NEC's ITF assigns 20000000h
 *    or 21000000h (both candidates are hardcoded in pc98_tgui.c; we
 *    read the live value from configuration space instead).
 *    ** The linear framebuffer is at BAR0 + 0 ** (XF98 sets
 *    ChipLinearBase = vgaPCIInfo->MemBase), the memory-mapped
 *    graphics-engine block is at BAR0 + 400000h (unused here).
 *  - VGA registers decode at the NATIVE 3C0h/3C4h/3CEh/3D4h I/O
 *    block, NOT at the PC-98 WAB relocation 0CA0h/0DA4h.  This
 *    finally explains the observation recorded in the Cirrus driver:
 *    the V16 answers the WAB ID probe at 0FAAh with 5Bh (inside the
 *    supposed Cirrus range) and its relay registers work, but the
 *    VGA file at 0CA0h floats.  The Trident was at 3C0h all along.
 *  - Trident also decodes "extended" I/O aliases above the VGA
 *    block, all confirmed in use by pc98_tgui.c on the real
 *    machines:
 *        43C8h/43C9h  VCLK PLL     (n / m,k; see below)
 *        43C6h/43C7h  MCLK PLL
 *        83C8h/83C6h  "SYNCDAC" index/data (NEC sync/output glue,
 *                     register 04h holds the sync output enables)
 *
 * ---------------------------------------------------------------------------
 * B. Wakeup (PC-98 has no VGA BIOS; the chip has never been POSTed)
 * ---------------------------------------------------------------------------
 * Per pc98_tgui.c VideoEnable(), operating in the OLD register mode:
 *  - SR0E |= 20h selects "Configuration Port 1" at SR0C; SR0C bit4
 *    then tells which wakeup scheme the board straps:
 *      bit4 = 1:  outp(94h, 00h); outp(102h, 01h); outp(94h, 20h);
 *                 3C3h |= 01h            (the NEC/PC-98 wiring)
 *      bit4 = 0:  outp(46E8h, 10h); outp(102h, 01h); outp(46E8h, 08h)
 *                 (the AT-style setup ports)
 *    Note these are the un-relocated 94h/102h - the Trident world's
 *    "POS" ports, decoded by the chip itself, unrelated to the
 *    0904h/FF82h pair the Cirrus WAB machines use.
 *  - MISC is OR'd with C3h beforehand (RAM enable, color I/O).
 *
 * ---------------------------------------------------------------------------
 * C. Video output relay (per pc98_tgui.c crtswNEC96xx/crtswTGUiGen)
 * ---------------------------------------------------------------------------
 * Switching to the accelerator is an ordered dance:
 *   1. GDC side off:  outp(68h,0Eh); outp(6Ah,07h); outp(6Ah,8Fh);
 *      outp(6Ah,06h); and 9A8h=01h (force 31kHz) if it was 24kHz.
 *   2. Trident sync path on: CR23 &= ~20h; CR29 |= 04h;
 *      SYNCDAC[04h] |= 06h; wait 1ms; |= 08h; GR23 &= ~03h;
 *      SYNCDAC[04h] |= 01h; SR01 &= ~10h.
 *   3. The relay latch: outp(0FACh, 02h).
 * Back to the GDC runs the exact mirror (0FACh=00h first, then the
 * sync teardown, then the GDC side on with 6Ah=8Eh, 68h=0Fh).
 * The two-stage WAB interface at 0FAAh/0FABh is NOT used on the
 * 96xx machines (XF98 only uses it for the Cyber9320 one); on the
 * V16 it merely answers the ID 5Bh.  We read it for the log only.
 *
 * ---------------------------------------------------------------------------
 * D. Chip programming (generic TGUI96xx knowledge)
 * ---------------------------------------------------------------------------
 *  - Old/new register modes: READING SR0B selects the new mode and
 *    returns the chip ID (D3h = 9660 family); WRITING SR0B selects
 *    the old mode.  SR09 = revision (see A.).
 *  - SR0E in the new mode: bit7 = enable extension registers; low
 *    bits = 64KB bank.  ** Writes to new-mode SR0E invert bit1 on
 *    the way in ** (the classic Trident detection quirk: write 00h,
 *    read back 02h), so to store value V one must write V ^ 02h.
 *  - VCLK PLL at 43C8h/43C9h (TGUI9660/9680/9682 use the OLD clock
 *    layout; only 9685+ has the new one):
 *        f = 14.31818MHz * (N + 8) / ((M + 2) * 2^K)
 *        43C8h = N[6:0] | (M[0] << 7)
 *        43C9h = M[4:1] | (K << 4)
 *    (Verified in three independent code bases: Linux tridentfb,
 *    XFree86/Xorg trident, PCem vid_tgui9440.c.)  MCLK at 43C6h/
 *    43C7h uses the layout of pc98_tgui.c GetMCLK():
 *        43C6h = N[4:0] << 3 | M[2:0],  43C7h = K << 1 | N[5].
 *  - Depth selection: CR38 (Pixel Bus) 00h/05h/29h for 8/16/24bpp,
 *    plus the hidden DAC register (read 3C8h once, 3C6h four times,
 *    the next 3C6h access is the hidden register): 00h / 30h (565) /
 *    D0h.  No clock doubling/tripling on 96xx at 16/24bpp.
 *  - CR21 bit5 enables the linear aperture; on PCI parts the base
 *    comes from BAR0, no address bits needed in CR21.
 *  - CR1E = 80h (extended memory access), CR2A |= 40h (32-bit bus),
 *    GR0F = (old & F0h) | 12h, CR29 bits5:4 = pitch bits 9:8.
 *  - VRAM size in CR1F low nibble: 1=512KB 3=1MB 7=2MB Fh=4MB.
 *  - Board tuning pc98_tgui.c ChipInit() performs on these exact
 *    machines (DRAM/FIFO/latency values, MCLK=80MHz nominal,
 *    SYNCDAC[0]=01h, GR2Fh=20h, GR5Eh=88h, GR5Fh=48h):
 *    replayed here 1:1, but opt-in via M=1 for bring-up in case
 *    the ITF already programmed saner values.
 *    (The MCLK encodings in XF98 and Xorg disagree about the
 *    resulting frequency; the register VALUE 53h/00h is what XF98
 *    proved on the V13/V16, so the value is replayed verbatim
 *    rather than recomputed.)
 *
 * ---------------------------------------------------------------------------
 * E. Open items (to verify on real hardware - the driver logs all of it)
 * ---------------------------------------------------------------------------
 *  - Whether the Ra43 (TGUI9682XGi, 1999) keeps the V16's wakeup and
 *    relay wiring.  Same family and same NEC design lineage, but XF98
 *    predates it.  The 0FACh/SR0C/WAB-ID readbacks in the log will
 *    tell.
 *  - The 24bpp VRAM byte order is assumed B,G,R (little-endian
 *    convention, like the Cirrus).  If red/blue come out swapped on
 *    the real DAC, conv_row24() is the one place to touch.
 *  - [ANSWERED 2026-07] MCLK/DRAM tuning: the ITF defaults are
 *    fine for display; the XF98 tuning (M=1) does not fix the
 *    bulk-write dword drops either.  Paced+verified stores (V=1)
 *    are what makes the output pixel-perfect.
 *  - 800x600x24 would sit exactly at the 40MHz limit XF98 imposes on
 *    24bpp (Bpp_Clocks[3] = 40000); NEC never shipped that mode, so
 *    it is rejected here.
 *
 * ---------------------------------------------------------------------------
 * F. Ra43 real-hardware findings (log of 2026-07, first probe build)
 * ---------------------------------------------------------------------------
 * The first build assumed the V13/V16 model: legacy VGA I/O at the
 * native 3C0h block.  The Ra43 log disproved that:
 *   - PCI 0:8.0 = 1023:9660, BAR0 = 20000000h (4MB decode) - fine;
 *   - the PCI command register was 0002h as shipped: memory decode
 *     ON, ** I/O decode OFF **;
 *   - every 3C0h-block read returned FFh even after enabling I/O
 *     decode and running the wakeup.
 * Conclusion: on the Ra generation NEC drives the chip without
 * legacy VGA I/O at all.  The supported way in is the one Linux
 * tridentfb and Xorg trident use on such boards:
 *   ** BAR1 is a 64KB MMIO block in which the ENTIRE register file
 *   appears memory-mapped at its port offsets ** - SR at +3C4h/3C5h,
 *   CRTC at +3D4h/3D5h, the DAC at +3C6h..3C9h, and even the clock
 *   PLL at +43C8h/43C9h (tridentfb writes it through t_outb) - gated
 *   by CR39 bit0 (which such boards strap on at power-up; Xorg calls
 *   the configuration "MMIOonly").
 * This driver therefore probes in this order: PIO at 3C0h first
 * (keeps the V13/V16 path alive), then BAR1 MMIO, then - only if
 * both are dead - the blind wakeup sequences followed by a PIO
 * retest.  The AT-style 46E8h wakeup is tried before the 94h one:
 * on a machine whose Trident does not claim port 94h, writing it
 * hits the PC-98 FDC mode register, so 94h is the last resort.
 * Note the Ra43 also reports the odd PCI revision D3h (the SR0B
 * chip-ID value); the chip name is therefore decided from SR09
 * once register access works, not from the PCI revision.
 *
 * ---------------------------------------------------------------------------
 * G. NEC's own NT 4.0 miniport (trident.sys, linked 1996-10-15)
 * ---------------------------------------------------------------------------
 * Disassembling NEC's PC-98 NT4 trident.sys settled the remaining
 * questions with first-party answers:
 *  - It imports only VideoPort PORT-I/O helpers, yet after boot it
 *    touches VGA registers exclusively through `mov [base+port]`
 *    memory accesses: at init it reads SR0B, sets SR0E |= 80h and
 *    ** CR39 |= 01h via PIO, then VideoPortGetDeviceBase()-maps a
 *    64KB memory range and stores it as the register base ** -
 *    exactly the BAR1 MMIO model this driver fell back to for the
 *    Ra43.  NEC used MMIO for everything from day one; the offsets
 *    it adds are the raw port numbers (3C4h..3DAh, 43C6h..43C9h,
 *    83C6h/83C8h and even 3C3h/46E8h).
 *  - Every SR0E write in the binary pre-XORs bit1 (`or 80h, xor 02h`)
 *    - third independent confirmation of the invert quirk.
 *  - The NEC sync glue lives in Trident extension registers:
 *    GR20h-2Ah (board init), GR2Ch (relay sync path), GR30h/GR33h
 *    and GR24h (DAC/sync power sequencing, stepped one bit at a
 *    time with WHOLE-VSYNC-FRAME delays), GR40h-46h/50h-53h (sync
 *    mode per display mode), GR42h bit7 = monitor sense, GR5Ah/5Bh
 *    used as scratch, and a SHADOW CRTC bank at I/O 3A4h/3A5h
 *    (gated by GR30h bit6) that carries GDC-like 640x480 timings.
 *    XF98's "SYNCDAC" (83C8h) is also written (regs 00h/04h/08h/
 *    09h/37h/38h) but only during board init.
 *  - The relay proper: machines are typed; the 96xx desktops write
 *    ** 0FACh = 03h ** to switch to the accelerator (not the 02h
 *    XF98 used) and 00h to switch back; TGUI9440-generation WAB
 *    machines use the two-stage 0FAAh/0FABh (reg 03h = 03h/00h)
 *    instead; and unless the monitor code is 93h the driver also
 *    flips ** 0FAAh reg 84h |= / &= ~11h **.  The dance is
 *    bracketed by writes to an indexed 16-bit interface at
 *    ** 8F0h/8F2h ** (index 52h bit7, index 60h bit4) that neither
 *    XF98 nor any public source mentions.
 *  - NEC's own state save covers CRTC 00h-50h and GR 00h-5Fh plus
 *    the clocks - this driver's save ranges were widened to match.
 *  - Its access-range table holds the framebuffer at the FIXED
 *    physical address 73000000h (4MB) - it never reads BAR0.  The
 *    Ra43 ITF's CR21 value C7h decodes to exactly that address as
 *    (bits3:0 << 28) | (bits7:6 << 24), which identifies CR21 on
 *    this wiring as the linear-window placement register (bit5 =
 *    enable).  Field-confirmed: BAR0 reads junk and drops writes,
 *    while the CR21-decoded window is where VRAM answers.
 * The relay/glue tables and sequences extracted from the binary are
 * replayed below (tg_nt_* and the tg_glue_* tables).  The XF98
 * variant is documented above but not selectable in this build.
 *
 * ---------------------------------------------------------------------------
 * H. Relay experiment matrix (T=0..9 in the environment)
 * ---------------------------------------------------------------------------
 * With T set, trident_init_disp() replaces the normal relay with one
 * of the experiments below, then runs a common observation phase:
 *   1. register dump
 *   2. SR01 bit5 blink x3   -> changes on screen = Trident owns video
 *   3. 8bpp palette cycle   -> changes on screen = DAC path alive
 *   4. wait for a key; cleanup mirrors the exact experiment
 *
 *   T=0  minimal, FAC=02h                      (current default)
 *   T=1  minimal, FAC=03h                      (NT's value)
 *   T=2  minimal + XF98 SDAC[04] sync path     (06h -> 08h -> 01h)
 *   T=3  minimal + NT GR24/GR33 sync power-up  (one bit per frame)
 *   T=4  = T=3 + FAA[84h] |= 11h               (NT non-93h monitor)
 *   T=5  = T=4 + 8F0h bracket                  (full NT periphery)
 *   T=6  XF98 full dance
 *   T=7  NT full dance
 *   T=8  interactive FAC sweep 00h..03h x FAA84 (key after each)
 *   T=9  no relay at all                       (is white just the
 *                                               GDC teardown?)
 *
 * Run with -8 (8bpp) so the palette test is meaningful; 24bpp
 * bypasses the DAC lookup.
 *
 * ---------------------------------------------------------------------------
 * I. NEC's NT 4.0 GDI driver (trident.dll) - the accelerator protocol
 * ---------------------------------------------------------------------------
 * Field observation: direct CPU stores through the linear aperture
 * land only probabilistically while the CRTC is scanning out - the
 * classic sign that the DRAM interface does not arbitrate raw host
 * writes against refresh/scanout on this board.  NEC's own NT4 GDI
 * driver never bulk-writes the aperture as plain memory; every
 * paint goes through the graphics engine, whose FIFO is the
 * arbitrated path into VRAM.  Disassembling trident.dll (PE32,
 * imports only WIN32K Eng* helpers) yielded the exact protocol:
 *
 *  - WHERE THE GE BLOCK LIVES (settled by disassembling NEC's
 *    trident.sys miniport): the GE registers are an INDEPENDENT
 *    chip-level memory decode - NOT inside BAR1 at +2100h.  Its
 *    64KB page is placed by two CRTC registers:
 *        CR34 = GE physical address bits 23:16
 *        CR35 = GE physical address bits 31:24
 *    and the registers sit at offsets 00h..FFh inside that page
 *    (the classic map minus the 2100h bias).  The miniport, in
 *    its IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES handler, points
 *    CR34/35 at BAR1 and hands the mapped range to the GDI
 *    driver.  The placement is CHIP-TYPED (miniport types by
 *    SR0B/SR09): the TGUI9660 family (SR0B=D3h - the Ra43 class)
 *    gets BAR1 + 0, range 100h, i.e. the GE registers co-reside
 *    with the register file in the BOTTOM 256 BYTES of BAR1 (VGA
 *    relocations start at 3C0h; no overlap); only the TGUI9320
 *    (SR0B=A3h) gets BAR1 + 10000h with a 64KB range.  The GDI
 *    then addresses the registers as raw offsets off the range
 *    pointer:
 *        +20h  status      (word polled, mask 20h = engine busy;
 *                            mask 40h polled before pattern stores)
 *        +22h  operation mode (word; see below)
 *        +24h  command     (01h = BitBLT, 04h = Bresenham line,
 *                            05h = short vector)
 *        +27h  FMIX/ROP    (CCh = copy source, F0h = copy pattern)
 *        +28h  drawflag    (dword; 04h = source is display memory,
 *                            20h = mono pattern, 40h = mono source,
 *                            1000h = transparent, 4000h = solid
 *                            fill; ** neither 04h nor 40h set means
 *                            the source is the CPU data stream **)
 *        +2Ch  foreground colour   +30h  background colour
 *        +38h/+3Ah  dest X/Y       +3Ch/+3Eh  src X/Y
 *        +40h/+42h  dimension X/Y, PROGRAMMED AS W-1 / H-1
 *        +48h..+4Fh clip: src X/Y = top-left, dest X/Y =
 *                            bottom-right (96xx clips CPU-source
 *                            pixels - must be opened up, cf.
 *                            tridentfb tgui_init_accel())
 *        +80h..+FFh pattern data
 *    The miniport's other GE prerequisites, all replayed here:
 *    PCI cfg 14h |= 02h (the "hidden decode enable"), SR0E |= 80h,
 *    CR39 |= 01h.  And the enable proper, field-found on the Ra43
 *    (whose ITF ships it OFF) and confirmed in 86Box's
 *    tgui_recalcmapping():
 *        ** CR36 bits1:0 = GE memory-decode select: 00b = decode
 *        DISABLED, 01b = legacy window at B4000h, 10b = legacy
 *        window at BC000h, 11b = the CR34/35-placed block. **
 *    The miniport binary itself never writes CR36 - the value
 *    rides in its per-mode CRTC tables - which is why the first
 *    Ra43 probe of the CR34/35 block read like unclaimed memory
 *    (only a stray dword "latched": ordinary 25%-survival direct
 *    stores, not registers).
 *    (The 2100h-biased register block of the AT-world literature
 *    is the legacy/emulator view of the same registers; the Ra43
 *    field test proved it does not answer over BAR1 MMIO -
 *    everything read 00h until CR34/35 placed the real block.)
 *  - The CPU-source data port IS THE LINEAR APERTURE: the driver
 *    stores the framebuffer pointer from IOCTL_VIDEO_MAP_VIDEO_-
 *    MEMORY and, after issuing a CPU-source BLT, streams the pixel
 *    data with rep movsd to that address (restarting at the base
 *    for every scan of the operation).  While such a BLT is live
 *    the chip consumes ALL aperture stores into the engine FIFO in
 *    memory order (86Box models this 1:1 in tgui_accel_write_fb_*),
 *    which is what buys the scanout arbitration.
 *  - Per-operation sequence used by every paint path in the DLL:
 *        1. poll 2120h until bit5 clear
 *        2. 2127h = ROP, 2128h = drawflag, colours if needed
 *        3. 2138h/213Ah dest, 2140h/2142h = W-1/H-1
 *        4. 2124h = 01h
 *        5. poll 2120h bit5 again, then stream the data (CPU-source
 *           operations only; pattern/solid ops run on step 4)
 *  - 2122h operation mode, written once per mode from a
 *    pitch-x-depth table in DrvEnableSurface.  The power-of-two
 *    rows of NT's table match Linux tridentfb tgui_init_accel()
 *    exactly, which decodes the encoding as
 *        low byte = depth (8bpp: 0, 16bpp: 1, 24bpp: 3)
 *                 | engine pitch (512/8192: 00h, 1024: 04h,
 *                                 2048: 08h, 4096: 0Ch)
 *    (NT's high-byte values 40h..90h are the fractional-pitch
 *    variants for non-power-of-two pitches like 640/1920/2496;
 *    this driver sidesteps them by using power-of-two pitches.)
 *  - 24bpp: NT uses depth code 3, but the engine granularity at
 *    24bpp is the byte (86Box maps 24bpp to the 8bpp engine); this
 *    driver drives 24bpp with depth code 0 and byte coordinates
 *    (x*3, w*3), the same trick XFree86 used on chips of the era,
 *    so the semantics are identical on real silicon and emulators.
 *
 * The frame presentation below therefore runs as a CPU source FIFO
 * BLT: one full-rectangle BLT per frame, rows streamed through the
 * aperture.  The engine is self-tested at init (register readback,
 * solid fill, host blit, all verified through the aperture); if
 * anything fails the driver falls back to the old direct writes.
 *
 * FIELD OUTCOME (2026-07, verified-store build): with the paced,
 * read-verified direct stores (V=1, the default) the picture is
 * PIXEL-PERFECT at 8, 16, and 24bpp on the Ra43.  The dword-
 * granular write drops are a hardware-level defect of this
 * board's host-write path under scanout (whole PCI dword
 * transactions vanish from bulk bursts, ~75% of them, while
 * isolated writes and all reads are reliable); the XF98 board
 * tuning (M=1: MCLK 53h, GR2F/5E/5F, SYNCDAC) does NOT cure it -
 * M=1 with raw stores (V=0) still fails - so the mitigation is
 * the software layer: never offer the FIFO more than 4 dwords
 * between draining reads, then patch whatever still dropped.
 * M stays default-off.
 *
 * The graphics engine itself remains DARK on the Ra43: with the
 * full NT recipe applied (CR34/35 placement, CR36 bits1:0 = 11b,
 * cfg14h.1, SR0E.7, CR39.0), both placements read back 00h in
 * every register and execute nothing, under M=0 and M=1 alike.
 * (The lone "flags = 24h,50h" seen at BAR1+10000h is a stray
 * surviving direct store into unclaimed space, not a register -
 * it appears only at the placement that is outside BAR1's 64KB
 * claim.)  Presentation therefore runs on the verified stores;
 * the engine bring-up code and this protocol record stay for a
 * future attempt - the untapped lead is the miniport's per-mode
 * register tables (HwSetMode path), which may carry a further
 * gate this driver has not replayed.
 *
 * FIELD ADDENDUM - operating without a usable aperture: on the
 * target board the linear window cannot be trusted even for reads,
 * so nothing may depend on aperture verification.  Two facts make
 * the engine viable regardless:
 *  - the FIFO capture of CPU-source data is a PCI-target mechanism
 *    keyed on the chip's memory DECODE (armed while CR21 bit5 is
 *    set), independent of whether direct stores survive the DRAM
 *    path - so the window still serves as a WRITE-ONLY data port;
 *  - everything else the protocol needs is registers, which work.
 * Hence: the self-test accepts the engine on register proof alone
 * when the aperture is unverified (and even engages it blind if
 * the registers turn out write-only, provided the status neither
 * floats FFh nor sticks busy), the CR21-decoded window is adopted
 * as the write-only port, and the end-to-end judgement is the
 * fetch-experiment color bars, which are themselves drawn through
 * the CPU-source FIFO BLT.
 *
 * References:
 *  - XFree86 3.3.6 xfree98 pc98_tgui.c / pc98_tgui.h (primary:
 *    PCI base, native VGA I/O, wakeup, relay dance, SYNCDAC, board
 *    tuning, per-depth clock limits)
 *  - XFree86 3.3.6 tvga8900 t89_driver.c (PC98_TGUI ifdefs: linear
 *    base = PCI MemBase, PixelBusReg/CommandReg per depth)
 *  - Linux tridentfb.c (BIOS-less mode-set order, CRTC overflow
 *    layout CR27/CR2B, VCLK search, hidden DAC access)
 *  - Xorg xf86-video-trident trident_dac.c/trident_pll.c (restore
 *    ordering, SR0E XOR quirk, old/new clock layouts)
 *  - PCem vid_tgui9440.c via NP2kai wab/tgui9680.c (SR0E XOR
 *    emulation, 43C8h/43C9h decode, hidden DAC state machine)
 *  - VGADOC TRIDENT.TXT (register names)
 *  - NEC PC-9821V16/M7 official spec sheet (TGUI9680XGi, VRAM 2MB,
 *    640x480 16.77M / 800x600 65K / 1024x768 65K / 1280x1024 256)
 *  - NEC's NT4 trident.dll (GDI display driver; disassembled: GE
 *    register map, busy bits, CPU-source protocol, 2122h table)
 *  - NEC's NT4 trident.sys (miniport; disassembled: CR34/35 GE
 *    block placement, cfg14h.1 decode enable, access ranges)
 *  - 86Box vid_tgui9440.c (TGUI9660/9680 engine model: drawflag
 *    bit meanings, FIFO consumption of aperture stores, clipping)
 *
 * ===========================================================================
 * Driver design
 * ===========================================================================
 *  - trident_init_disp(mode, bpp) scans PCI for vendor 1023h.  The
 *    9660-family desktops are driven; Cyber laptops read out but are
 *    declined (PCI-BAR 98NOTE machines are out of scope for now).
 *    No PCI Trident -> false, so the main code can try other chips.
 *  - Register access is abstracted (tg_inb/tg_outb): legacy PIO at
 *    3C0h where it answers (V13/V16), BAR1 MMIO where it doesn't
 *    (Ra43) - see section F.
 *    The probe is non-destructive until the chip is positively
 *    fingerprinted (SR0B chip ID D3h + the SR0E write-0-reads-2
 *    signature); state is saved before anything is programmed and
 *    restored on cleanup.
 *  - Frame presentation: a CPU-source FIFO BLT through the graphics
 *    engine (the NT4 trident.dll protocol, section I) - the FIFO is
 *    the path that arbitrates against CRTC scanout.  The engine is
 *    self-tested at init; on any failure the driver falls back to
 *    direct stores through the linear aperture (the old behavior).
 *  - The pitch is the smallest power of two >= w * bytespp (1024/
 *    2048/4096) so the engine pitch code in GER 2122h is exact.
 *  - Modes: 640x480 (8/16/24bpp) and 800x600 (8/16bpp).  bpp == -1
 *    picks 24bpp when it fits the VRAM.
 *  - The relay sequence is the "min" one: flip only the 0FACh
 *    output mux and the GDC display element, leaving every
 *    ITF-configured NEC glue register alone (the Ra43 field test
 *    showed the full 1996-era NT sequence kills the sync there,
 *    while the ITF state works).
 *  - No environment variables: the field-proven configuration is
 *    unconditional (paced + verified LFB stores, ITF tuning kept,
 *    fetch combo 0).  Two compile-time knobs near the includes:
 *    TG_TRY_GE re-arms the graphics-engine bring-up for the next
 *    step, TG_FETCH_EXPERIMENT restores the interactive fetch
 *    walk.  (The former M/G/V env options are retired: M=1 XF98
 *    tuning was proven not to help, V=1 verified stores are now
 *    always on, G is TG_TRY_GE.)
 *  - Verbose by design, like the Cirrus driver: this driver only
 *    runs when the user passes the -24 option, and the register
 *    dumps have repeatedly been the only way to debug real hardware.
 */

/* HAL */
#include <strato/strato.h>	/* Public Interface */
#include "98disp.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DOS */
#include <dos.h>
#include <conio.h>
#include <i86.h>
#include <stdint.h>

/*
 * Compile-time build knobs.  No environment variables: the driver
 * runs the field-proven configuration unconditionally.
 *  - TG_TRY_GE 1 re-arms the graphics-engine bring-up (the CPU
 *    source FIFO BLT of section I).  Parked at 0: the engine never
 *    responded on the Ra43; the protocol record and code stay for
 *    the next attempt.
 *  - TG_FETCH_EXPERIMENT 1 restores the interactive fetch-combo
 *    walk (color bars, key presses).  At 0 the driver silently
 *    applies combo 0, the state under which the Ra43 showed a
 *    pixel-perfect picture.
 */
#ifndef TG_TRY_GE
#define TG_TRY_GE 0
#endif
#ifndef TG_FETCH_EXPERIMENT
#define TG_FETCH_EXPERIMENT 0
#endif

/*
 * Requested geometry per DISP_* selector.
 */
static const struct disp_geo {
	int w, h;
} disp_geo[] = {
	{  640,  480 },		/* DISP_640X480 */
	{  800,  600 },		/* DISP_800X600 */
	{ 1024,  768 },		/* DISP_1024X768 */
	{ 1280, 1024 }		/* DISP_1280X1024 */
};

static struct trident_disp {
	bool active;
	const char *chip_name;

	/* Screen geometry. */
	int scr_w;		/* 640 or 800 */
	int scr_h;		/* 480 or 600 */
	int bpp;		/* 24, 16 or 8 */
	uint32_t pitch;		/* bytes per scanline */

	/*
	 * VGA register file.  io_3d4/io_3da follow MISC bit0 between
	 * the color and mono blocks.
	 */
	uint16_t io_3c0;
	uint16_t io_3d4;
	uint16_t io_3da;
	uint16_t io_3d4_col, io_3da_col;
	uint16_t io_3d4_mono, io_3da_mono;
	uint16_t io_vclk;	/* 43C8h: VCLK PLL (43C6h = MCLK) */
	uint16_t io_sdac;	/* 83C8h: SYNCDAC index (83C6h = data) */

	/*
	 * Register access path.  When use_mmio is set, every VGA
	 * register access goes through the BAR1 MMIO block (the port
	 * number doubles as the offset inside the block); the PC-98
	 * platform ports (0FACh, 68h, 6Ah, 9A8h, 5Fh) stay real I/O.
	 */
	bool use_mmio;
	int aper_width;		/* 4 = dwords OK, 1 = byte-only lane */
	volatile uint8_t *mmio;	/* mapped BAR1, 64KB */
	uint32_t mmio_phys;

	/*
	 * Graphics engine (CPU-source FIFO BLT, see section I).
	 * ge_xmul is the pixel -> engine-coordinate multiplier:
	 * 1 at 8/16bpp, 3 at 24bpp (byte-granular engine mode).
	 */
	bool use_ge;		/* engine self-tested OK and in use */
	bool aper_ok;		/* aperture verified readable/writable */
	int ge_xmul;
	uint8_t ge_opermode;	/* GER 2122h low byte */
	volatile uint8_t *ge;	/* CR34/35-decoded GE block (NULL =
				 * legacy 2100h fallback) */
	uint32_t ge_phys;

	/* VRAM aperture (always linear on this driver). */
	uint8_t *fb;
	uint32_t fb_phys;
	uint32_t vram_size;

	/* Chip information. */
	uint8_t wab_id;		/* raw 0FAAh register 00h readout (info) */
	uint8_t chip_id;	/* SR0B readback (D3h = 9660 family) */
	uint8_t chip_rev;	/* SR09 (00=9660 01=9680 10h=9682) */

	/* 9A8h horizontal sync rate as found (1 = 31kHz). */
	int hsync31;
} tdisp;

/* Blit placement (centering + clip against the screen). */
static int ofs_x, ofs_y;
static int draw_w, draw_h;

extern struct hal_image *back_image;
extern int game_width;
extern int game_height;

/* Frame presentation. */
static int tg_store_verified(volatile uint8_t *dst, const uint8_t *src,
			     int nbytes);
static void tg_clear_screen(void);
static void trident_flip_vram(void);
#if TG_TRY_GE
static void trident_flip_ge(void);
#endif
static void conv_row24(uint8_t *dst, const uint32_t *src, int n);
static void conv_row16(uint8_t *dst, const uint32_t *src, int n);
static void conv_row8(uint8_t *dst, const uint32_t *src, int n);

#if TG_TRY_GE
/* Graphics engine (CPU-source FIFO BLT, trident.dll protocol). */
static void tg_ge_out8(int reg, int val);
static void tg_ge_out16(int reg, int val);
static void tg_ge_out32(int reg, uint32_t val);
static int tg_ge_in8(int reg);
static bool tg_ge_wait(const char *who);
static uint32_t tg_ge_color(uint32_t c);
static void tg_ge_fill(int xu, int y, int wu, int h, uint32_t color);
static bool tg_ge_selftest(void);
static void tg_ge_init(void);
#endif /* TG_TRY_GE */


/* Low-level register access. */
static int tg_inb(unsigned port);
static void tg_outb(unsigned port, int val);
static void tg_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
			  uint16_t d4m, uint16_t dam);
static void tg_select_crtc(int misc);
static void tg_seq_write(int reg, int val);
static int tg_seq_read(int reg);
static void tg_gfx_write(int reg, int val);
static int tg_gfx_read(int reg);
static void tg_crtc_write(int reg, int val);
static int tg_crtc_read(int reg);
static void tg_attr_write(int reg, int val);
static int tg_attr_read(int reg);
static void tg_misc_write(int val);
static int tg_misc_read(void);
static void tg_hidden_dac_write(int val);
static int tg_hidden_dac_read(void);
static void tg_sdac_write(int reg, int val);
static int tg_sdac_read(int reg);
static void tg_sw_old(void);
static void tg_sw_new(void);
static void tg_wait_ms(int ms);

/* Detection / bring-up. */
static bool tg_pci_find(void);
static void tg_pci_dump_config(void);
static void tg_pci_dump_header(int bus, int dev, int fn);
static bool tg_regs_alive(void);
static bool tg_probe_access_path(void);
static void tg_wakeup_strapped(void);
static void tg_wakeup_blind_at(void);
static void tg_wakeup_blind_pc98(void);
static bool tg_fingerprint(void);
static uint32_t tg_vram_size(void);
static uint32_t tg_vram_probe(void);
static bool tg_aperture_test(const char *tag);
static bool tg_aperture_fix(void);
static void tg_dump_regs(const char *tag);

/* Mode set. */
static void tg_modeset(void);
static void tg_load_palette(void);
static int tg_resolve_bpp(int req, int w, int h, uint32_t vram);

/* Relay (fixed: minimal teardown + XF98 SDAC path + FAC=02h). */
static void tg_relay_to_accel(void);
static void tg_relay_to_gdc(void);

/* Fetch-path experiment (runs automatically at init). */
static void tg_fetch_apply_default(void);
#if TG_FETCH_EXPERIMENT
static void tg_fetch_experiment(void);
#endif
static void tg_fetch_capture_itf(void);
#if TG_FETCH_EXPERIMENT
static void tg_fetch_pattern(void);
#endif

/* State save/restore. */
static void tg_save_state(void);
static void tg_restore_state(void);

/* PCI access (defined in the PCI detection section). */
static uint32_t pci_read32(int bus, int dev, int fn, int reg);
static void pci_write32(int bus, int dev, int fn, int reg,
			uint32_t val);
static int pci_bus, pci_dev, pci_fn;
static uint32_t sv_cfg14 = 0xffffffffUL;	/* PCI cfg 14h as found */

/* Scratch row (conversion, verified stores, clears; 4096 >= pitch). */
static uint8_t tg_rowbuf[4096];


/* Misc. */
static void *tg_map_physical(uint32_t phys, uint32_t size);

/*****************************************************************************/
/* Public interface                                                          */
/*****************************************************************************/

bool
trident_init_disp(int mode, int bpp)
{
	int w, h;
	uint32_t need;

	if (mode < DISP_640X480 || mode > DISP_1280X1024) {
		hal_log_info("TRIDENT: invalid mode selector %d.", mode);
		return false;
	}
	if (bpp != -1 && bpp != 8 && bpp != 16 && bpp != 24) {
		hal_log_info("TRIDENT: invalid depth %d (8/16/24 or -1).",
			     bpp);
		return false;
	}

	memset(&tdisp, 0, sizeof(tdisp));

	w = disp_geo[mode].w;
	h = disp_geo[mode].h;
	hal_log_info("TRIDENT: probing; requested %dx%d, depth %d "
		     "(-1 = auto).", w, h, bpp);
	hal_log_info("TRIDENT-BUILD: LFB build (paced + verified "
		     "stores%s%s).",
		     TG_TRY_GE ? ", engine bring-up armed" : "",
		     TG_FETCH_EXPERIMENT
		     ? ", interactive fetch walk" : "");

	/*
	 * The WAB machine ID, for the log only (the 96xx machines do
	 * not use the two-stage interface).
	 */
	outp(0x0faa, 0x00);
	tdisp.wab_id = (uint8_t)inp(0x0fab);
	hal_log_info("TRIDENT: WAB ID (0FAAh reg 00h) reads %02Xh "
		     "(informational only).", tdisp.wab_id);

	/*
	 * The on-board TGUI96xx is a PCI device on every known
	 * machine (V13/V16, Ra series).  No PCI Trident, no driver.
	 */
	if (!tg_pci_find())
		return false;

	if (mode != DISP_640X480 && mode != DISP_800X600) {
		hal_log_info("TRIDENT: %dx%d not supported "
			     "(640x480 / 800x600 only).", w, h);
		return false;
	}

	/*
	 * Find a working register access path (PIO on the V13/V16
	 * generation, BAR1 MMIO on the Ra generation), then prove the
	 * chip's identity before programming anything.
	 */
	tg_set_iobase(0x03c0, 0x03d4, 0x03da, 0x03b4, 0x03ba);
	if (!tg_probe_access_path()) {
		tg_pci_dump_config();
		return false;
	}
	if (!tg_fingerprint())
		return false;

	/* Save the horizontal sync rate of the GDC side (9A8h). */
	tdisp.hsync31 = inp(0x09a8) & 0x01;
	hal_log_info("TRIDENT: GDC horizontal sync is %skHz (9A8h).",
		     tdisp.hsync31 ? "31.5" : "24.8");

	/* VRAM size from CR1F, then resolve the depth. */
	tdisp.vram_size = tg_vram_size();
	tdisp.bpp = tg_resolve_bpp(bpp, w, h, tdisp.vram_size);
	if (tdisp.bpp < 0)
		return false;
	if (mode == DISP_800X600 && tdisp.bpp == 24) {
		hal_log_info("TRIDENT: 800x600 at 24bpp would need the "
			     "full 40MHz dot clock at 24bpp - beyond "
			     "what NEC ever shipped; refusing.");
		return false;
	}

	tdisp.scr_w = w;
	tdisp.scr_h = h;
	tdisp.aper_width = 4;

	/*
	 * The pitch: smallest power of two holding a scanline, so
	 * the engine pitch code in GER 2122h is exact (1024: 04h,
	 * 2048: 08h, 4096: 0Ch - section I).  24bpp runs the engine
	 * byte-granular (depth code 0, coordinates x3).
	 */
	tdisp.pitch = 1024;
	while (tdisp.pitch < (uint32_t)w * (uint32_t)(tdisp.bpp / 8))
		tdisp.pitch <<= 1;
	switch (tdisp.bpp) {
	case 16:
		tdisp.ge_opermode = 0x01;
		tdisp.ge_xmul = 1;
		break;
	case 24:
		tdisp.ge_opermode = 0x00;	/* byte-granular */
		tdisp.ge_xmul = 3;
		break;
	default:
		tdisp.ge_opermode = 0x00;
		tdisp.ge_xmul = 1;
		break;
	}
	switch (tdisp.pitch) {
	case 1024:
		tdisp.ge_opermode |= 0x04;
		break;
	case 2048:
		tdisp.ge_opermode |= 0x08;
		break;
	default:			/* 4096 */
		tdisp.ge_opermode |= 0x0c;
		break;
	}

	need = (uint32_t)h * tdisp.pitch;
	if (need > tdisp.vram_size) {
		hal_log_info("TRIDENT: %dx%d %dbpp needs %luKB but only "
			     "%luKB VRAM.", w, h, tdisp.bpp,
			     (unsigned long)(need >> 10),
			     (unsigned long)(tdisp.vram_size >> 10));
		return false;
	}

	/* Map the linear framebuffer (BAR0 + 0). */
	tdisp.fb = (uint8_t *)tg_map_physical(tdisp.fb_phys,
					      tdisp.vram_size);
	if (tdisp.fb == NULL) {
		hal_log_info("TRIDENT: can't map the framebuffer at "
			     "%08lXh.", (unsigned long)tdisp.fb_phys);
		return false;
	}

	/* From here on we modify the chip: keep an exact undo image. */
	tg_save_state();
	tg_dump_regs("as found");

	/*
	 * The ITF board tuning is kept: the XF98 values (MCLK 53h,
	 * GR2F/5E/5F, SYNCDAC) were tried in the field and neither
	 * fix the bulk-write drops nor improve anything (2026-07).
	 */
	hal_log_info("TRIDENT: keeping the ITF board tuning.");

	/* Full mode set (leaves the screen blanked). */
	tg_modeset();
	tg_dump_regs("after mode set");

	/*
	 * Switch the video output relay to the accelerator with the
	 * screen still blanked (SR01 bit5 is set by the mode set).
	 */
	tg_relay_to_accel();

	/*
	 * Verify the linear aperture actually reaches VRAM, trying
	 * CR21 variants if the first attempt fails.  Continue either
	 * way (a garbage picture with working sync still tells us
	 * more than a dead screen).
	 */
	tdisp.aper_ok = tg_aperture_fix();
	if (tdisp.aper_ok) {
		uint32_t real;

		/* With the aperture live, measure the real VRAM. */
		real = tg_vram_probe();
		if (real != 0 && real != tdisp.vram_size) {
			hal_log_info("TRIDENT: real VRAM measures "
				     "%luKB (was sized %luKB from "
				     "CR1F).",
				     (unsigned long)(real >> 10),
				     (unsigned long)
				     (tdisp.vram_size >> 10));
			if (real > tdisp.vram_size) {
				uint8_t *bigger;

				bigger = (uint8_t *)
					tg_map_physical(tdisp.fb_phys,
							real);
				if (bigger != NULL) {
					tdisp.fb = bigger;
					tdisp.vram_size = real;
				}
			}
		}
	}


	/*
	 * ORDER MATTERS from here (Ra43 field lesson, 2026-07): the
	 * fetch-path registers (CR1E bit7 "extended memory access",
	 * GR0F, ...) govern the HOST addressing of VRAM through the
	 * aperture, not just the display fetch.  Under the ITF state
	 * (CR1E=00h) host accesses wrap at 256KB and interleave
	 * differently from the final scanout view - a clear executed
	 * there lands in the wrong places and self-verifies through
	 * the same wrong path.  So: apply combo 0 FIRST (screen
	 * still blanked), then run every bulk host access (engine
	 * bring-up, clear), then unblank.
	 */
#if TG_FETCH_EXPERIMENT
	tg_clear_screen();	/* best effort under the ITF state;
				 * the bars repaint everything */
	tg_fetch_experiment();	/* applies combos, unblanks itself */
#else
	tg_fetch_apply_default();	/* combo 0, still blanked */
#if TG_TRY_GE
	tg_ge_init();
#endif
	tg_clear_screen();
	tg_seq_write(0x01, tg_seq_read(0x01) & ~0x20);	/* unblank */
#endif

	/* Screen on (unblank: SR01 bit5 off). */
	tg_seq_write(0x01, 0x01);

	tdisp.active = true;

	/* Center the game image; clip if the screen is smaller. */
	ofs_x = (tdisp.scr_w - game_width) / 2;
	ofs_y = (tdisp.scr_h - game_height) / 2;
	if (ofs_x < 0)
		ofs_x = 0;
	if (ofs_y < 0)
		ofs_y = 0;
	draw_w = game_width < tdisp.scr_w ? game_width : tdisp.scr_w;
	draw_h = game_height < tdisp.scr_h ? game_height : tdisp.scr_h;
	draw_w &= ~3;	/* the row converters work 4 pixels at a time */

	hal_log_info("TRIDENT: === configuration summary ===");
	hal_log_info("TRIDENT: chip     : %s (SR0B=%02Xh SR09=%02Xh, "
		     "WAB ID=%02Xh).",
		     tdisp.chip_name, tdisp.chip_id, tdisp.chip_rev,
		     tdisp.wab_id);
	hal_log_info("TRIDENT: mode     : %dx%d, %d bpp, pitch %lu bytes.",
		     tdisp.scr_w, tdisp.scr_h, tdisp.bpp,
		     (unsigned long)tdisp.pitch);
	hal_log_info("TRIDENT: aperture : linear, %luKB at %08lXh%s, "
		     "%s.",
		     (unsigned long)(tdisp.vram_size >> 10),
		     (unsigned long)tdisp.fb_phys,
		     tdisp.fb_phys == 0x73000000UL
		     ? " (the NEC fixed window, not BAR0)"
		     : "",
		     tdisp.aper_ok
		     ? "verified"
		     : "UNVERIFIED (used as a write-only FIFO "
		       "port only)");
	if (tdisp.use_mmio)
		hal_log_info("TRIDENT: registers: BAR1 MMIO at %08lXh "
			     "(legacy VGA I/O is dead on this board).",
			     (unsigned long)tdisp.mmio_phys);
	else
		hal_log_info("TRIDENT: registers: legacy VGA I/O at "
			     "%03Xh.", tdisp.io_3c0);
#if TG_TRY_GE
	if (tdisp.use_ge)
		hal_log_info("TRIDENT: blitter  : CPU-source FIFO BLT "
			     "(GER22=%02Xh, %s coords).",
			     tdisp.ge_opermode,
			     tdisp.ge_xmul == 3 ? "byte" : "pixel");
	else
		hal_log_info("TRIDENT: blitter  : engine did not "
			     "pass its self-test; paced+verified "
			     "aperture stores in use.");
#else
	hal_log_info("TRIDENT: blitter  : parked (TG_TRY_GE=0); "
		     "frames go through paced+verified aperture "
		     "stores.");
#endif
	hal_log_info("TRIDENT: blit     : game %dx%d -> +%d,+%d "
		     "(draw %dx%d).",
		     game_width, game_height, ofs_x, ofs_y, draw_w, draw_h);

	return true;
}

void
trident_cleanup_disp(void)
{
	if (!tdisp.active)
		return;

#if TG_TRY_GE
	/* Never restore registers under a live engine operation. */
	if (tdisp.use_ge)
		(void)tg_ge_wait("cleanup");
#endif

	/* Blank while we unwind. */
	tg_seq_write(0x01, tg_seq_read(0x01) | 0x20);

	/* Output back to the 98 GDC (mirror of the fixed relay). */
	tg_relay_to_gdc();

	/* Put every register back the way we found it. */
	tg_restore_state();

	/* PCI cfg 14h back as found (the NT hidden decode enable). */
	if (sv_cfg14 != 0xffffffffUL) {
		pci_write32(pci_bus, pci_dev, pci_fn, 0x14, sv_cfg14);
		sv_cfg14 = 0xffffffffUL;
	}

	tdisp.active = false;
	hal_log_info("TRIDENT: cleanup done, output back on the 98 GDC.");
}

void
trident_flip(void)
{
	if (!tdisp.active)
		return;
	trident_flip_vram();
}

/*****************************************************************************/
/* Frame presentation - direct writes through the linear aperture            */
/*****************************************************************************/

/*
 * Blit the back image to VRAM.
 *
 * StratoHAL pixel layout (BGRA): low byte = B, then G, then R.
 * The Trident 24bpp framebuffer is assumed to be the same B,G,R
 * order.
 *
 * The default path is the CPU-source FIFO BLT (section I): one
 * BLT covering the whole destination rectangle, the converted
 * rows streamed through the aperture into the engine FIFO, which
 * is the VRAM path the chip arbitrates against scanout.  Direct
 * aperture stores remain as the fallback when the engine did not
 * pass its self-test (or died mid-session).
 */
/*
 * Verified, FIFO-paced store of one row.
 *
 * Field finding (Ra43, 8bpp): bulk back-to-back stores lose WHOLE
 * DWORD TRANSACTIONS (~75% of them under active scanout) while
 * isolated stores always land and reads are reliable - the chip's
 * host-write FIFO overruns without throttling.  Countermeasures,
 * layered:
 *   - pacing: a dummy read after every 4 dwords forces the posted
 *     writes to retire before more are queued (PCI ordering), so
 *     the FIFO is never offered more than it can hold;
 *   - verify passes: re-read and rewrite whatever still dropped,
 *     up to 8 passes.
 * Returns the number of patch passes needed (0 = clean on the
 * first verify), or -1 if bytes still differ after the cap.
 */
static int
tg_store_verified(volatile uint8_t *dst, const uint8_t *src, int nbytes)
{
	int pass, i, bad;

	if (tdisp.aper_width == 1) {
		for (pass = 0; pass < 8; pass++) {
			bad = 0;
			for (i = 0; i < nbytes; i++) {
				if (pass == 0 || dst[i] != src[i]) {
					dst[i] = src[i];
					bad++;
					if ((bad & 3) == 0)
						(void)dst[i];
				}
			}
			if (pass > 0 && bad == 0)
				return pass - 1;
		}
		return -1;
	}

	{
		volatile uint32_t *d = (volatile uint32_t *)dst;
		const uint32_t *s = (const uint32_t *)src;
		int n = nbytes / 4;

		for (i = 0; i < n; i++) {
			d[i] = s[i];
			if ((i & 3) == 3)
				(void)d[i];	/* drain the FIFO */
		}
		for (pass = 0; pass < 8; pass++) {
			bad = 0;
			for (i = 0; i < n; i++) {
				if (d[i] != s[i]) {
					d[i] = s[i];
					(void)d[i];
					bad++;
				}
			}
			if (bad == 0)
				return pass;
		}
	}
	return -1;
}

/*
 * Clear the visible screen (pitch * scr_h) with verified row
 * stores.  Must run under the final fetch-path register state
 * (combo 0) - see the ordering note in trident_init_disp().
 */
static void
tg_clear_screen(void)
{
	uint32_t y;

	if (!tdisp.aper_ok) {
		hal_log_info("TRIDENT: skipping the VRAM clear "
			     "(aperture unverified).");
		return;
	}
	memset(tg_rowbuf, 0, sizeof(tg_rowbuf));
	for (y = 0; y < (uint32_t)tdisp.scr_h; y++)
		(void)tg_store_verified(tdisp.fb + y * tdisp.pitch,
					tg_rowbuf, (int)tdisp.pitch);
}

static void
trident_flip_vram(void)
{
	static int drop_logged = 0;
	const uint32_t *pixels;
	int y, bytespp, rowlen, worst;

#if TG_TRY_GE
	if (tdisp.use_ge) {
		trident_flip_ge();
		return;
	}
#endif

	pixels = back_image->pixels;
	bytespp = tdisp.bpp / 8;
	rowlen = draw_w * bytespp;
	worst = 0;

	for (y = 0; y < draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;
		uint32_t off = (uint32_t)(y + ofs_y) * tdisp.pitch +
			       (uint32_t)ofs_x * (uint32_t)bytespp;
		int passes;

		switch (tdisp.bpp) {
		case 24:
			conv_row24(tg_rowbuf, src, draw_w);
			break;
		case 16:
			conv_row16(tg_rowbuf, src, draw_w);
			break;
		default:
			conv_row8(tg_rowbuf, src, draw_w);
			break;
		}

		passes = tg_store_verified(tdisp.fb + off,
					   tg_rowbuf, rowlen);
		if (passes < 0)
			passes = 8;
		if (passes > worst)
			worst = passes;
	}

	if (worst > 0 && drop_logged < 3) {
		drop_logged++;
		hal_log_info("TRIDENT: direct path needed up to %d "
			     "patch pass%s this frame (dword drops "
			     "under bulk stores).%s",
			     worst, worst == 1 ? "" : "es",
			     drop_logged == 3
			     ? "  (last such log)" : "");
	}
}

/*
 * Pixel format converters.  n is a multiple of four (enforced by
 * draw_w in trident_init_disp()).
 */

/* BGRA8888 -> packed BGR888.  VRAM layout is also B, G, R. */
static void
conv_row24(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 4) {
		uint32_t pix0 = src[x];
		uint32_t pix1 = src[x + 1];
		uint32_t pix2 = src[x + 2];
		uint32_t pix3 = src[x + 3];

		dst[0] = (pix0 & 0xffffff) | (pix1 << 24);
		dst[1] = ((pix1 & 0xffff00) >> 8) | ((pix2 & 0xffff) << 16);
		dst[2] = ((pix2 & 0xff0000) >> 16) | ((pix3 & 0xffffff) << 8);

		dst += 3;
	}
}

/* BGRA8888 -> RGB565 (little-endian words, R in bits 15:11). */
#define PACK565(p) \
	((((p) >> 8) & 0xf800) | (((p) >> 5) & 0x07e0) | (((p) >> 3) & 0x001f))

static void
conv_row16(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 2) {
		uint32_t p0 = src[x];
		uint32_t p1 = src[x + 1];

		*dst++ = PACK565(p0) | (PACK565(p1) << 16);
	}
}

/* BGRA8888 -> RGB332 (matches the palette set by tg_load_palette()). */
#define PACK332(p) \
	((((p) >> 16) & 0xe0) | (((p) >> 11) & 0x1c) | (((p) >> 6) & 0x03))

static void
conv_row8(uint8_t *dst8, const uint32_t *src, int n)
{
	uint32_t *dst = (uint32_t *)dst8;
	int x;

	for (x = 0; x < n; x += 4) {
		*dst++ = PACK332(src[x]) |
			 (PACK332(src[x + 1]) << 8) |
			 (PACK332(src[x + 2]) << 16) |
			 (PACK332(src[x + 3]) << 24);
	}
}

#if TG_TRY_GE
/*
 * Frame presentation through the engine, one CPU-source BitBLT PER
 * ROW.  Per-row operations are self-healing: a new command write
 * resets the engine's operation state (86Box: command latch clears
 * the internal x/y and recomputes the destination), so if a data
 * dword is ever lost on the bus the damage is confined to that one
 * row and the geometry re-syncs on the next, instead of shearing
 * the rest of the frame.  ROP and drawflag persist across rows.
 *
 * When the aperture is readable, the tail of the last row is read
 * back after the frame as a FIFO-loss diagnostic (first three
 * failures are logged).
 */
static void
trident_flip_ge(void)
{
	static int vfy_logged = 0;
	const uint32_t *pixels;
	int y, bytespp, rowlen;

	pixels = back_image->pixels;
	bytespp = tdisp.bpp / 8;
	rowlen = draw_w * bytespp;	/* dword multiple: draw_w &= ~3 */

	if (!tg_ge_wait("flip")) {
		/*
		 * The engine was just retired (no operation of ours
		 * can be pending); draw this very frame through the
		 * direct path instead of dropping it.
		 */
		trident_flip_vram();
		return;
	}

	tg_ge_out8(0x27, 0xcc);				/* ROP: copy */
	tg_ge_out32(0x28, 0x00000000UL);		/* CPU source */
	tg_ge_out16(0x38, ofs_x * tdisp.ge_xmul);	/* dest X */
	tg_ge_out16(0x40, draw_w * tdisp.ge_xmul - 1);	/* W - 1 */
	tg_ge_out16(0x42, 0);				/* H - 1 = 0 */

	for (y = 0; y < draw_h; y++) {
		const uint32_t *src = pixels + y * game_width;

		switch (tdisp.bpp) {
		case 24:
			conv_row24(tg_rowbuf, src, draw_w);
			break;
		case 16:
			conv_row16(tg_rowbuf, src, draw_w);
			break;
		default:
			conv_row8(tg_rowbuf, src, draw_w);
			break;
		}

		tg_ge_out16(0x3a, ofs_y + y);		/* dest Y */
		tg_ge_out8(0x24, 0x01);			/* BitBLT */
		if (!tg_ge_wait("flip data"))
			return;

		/*
		 * Stream the row into the FIFO through the aperture
		 * base, sequential like the NT driver's rep movsd.
		 */
		if (tdisp.aper_width == 1) {
			volatile uint8_t *out = tdisp.fb;
			int i;

			for (i = 0; i < rowlen; i++)
				out[i] = tg_rowbuf[i];
		} else {
			volatile uint32_t *out =
				(volatile uint32_t *)tdisp.fb;
			const uint32_t *in = (const uint32_t *)tg_rowbuf;
			int i, n = rowlen / 4;

			for (i = 0; i < n; i++)
				out[i] = in[i];
		}
	}

	if (!tg_ge_wait("flip end"))
		return;

	/* FIFO-loss diagnostic: the last row's head, read back. */
	if (tdisp.aper_ok && vfy_logged < 3) {
		volatile uint8_t *row = tdisp.fb +
			(uint32_t)(ofs_y + draw_h - 1) * tdisp.pitch +
			(uint32_t)ofs_x * (uint32_t)bytespp;
		int i, bad = 0;

		for (i = 0; i < 16; i++)
			if (row[i] != tg_rowbuf[i])
				bad++;
		if (bad != 0) {
			vfy_logged++;
			hal_log_info("TRIDENT-GE: frame verify: %d "
				     "of 16 tail bytes differ "
				     "(FIFO stream lost data this "
				     "frame; damage is per-row and "
				     "heals next frame).%s", bad,
				     vfy_logged == 3
				     ? "  (last such log)" : "");
		}
	}
}

#endif /* TG_TRY_GE */

#if TG_TRY_GE
/*****************************************************************************/
/* Graphics engine (CPU-source FIFO BLT - the trident.dll protocol)          */
/*****************************************************************************/

/*
 * GE register accessors.
 *
 * PRIMARY (the trident.sys recipe): the GE registers live in an
 * INDEPENDENT 64KB memory decode placed by CR34 (physical bits
 * 23:16) and CR35 (bits 31:24), at offsets 00h..FFh inside that
 * page - the miniport points it at BAR1 (+10000h on one machine
 * class) and NEC's GDI driver addresses the registers as raw
 * offsets 20h..FFh off the block, exactly the classic map minus
 * the 2100h bias.  tg_ge_init() places the block at BAR1+10000h
 * and maps it; tdisp.ge is that mapping.
 *
 * FALLBACK (tdisp.ge == NULL): the classic 2100h-biased access
 * through the register file / I/O, kept for boards without a
 * usable BAR1.  Register arguments are always the raw offsets
 * (20h..FFh).
 */
static void
tg_ge_out8(int reg, int val)
{
	if (tdisp.ge != NULL)
		tdisp.ge[reg] = (uint8_t)val;
	else
		tg_outb(0x2100 + reg, val);
}

static void
tg_ge_out16(int reg, int val)
{
	if (tdisp.ge != NULL)
		*(volatile uint16_t *)(tdisp.ge + reg) = (uint16_t)val;
	else if (tdisp.use_mmio)
		*(volatile uint16_t *)(tdisp.mmio + 0x2100 + reg) =
			(uint16_t)val;
	else
		outpw(0x2100 + reg, (unsigned short)val);
}

static void
tg_ge_out32(int reg, uint32_t val)
{
	if (tdisp.ge != NULL)
		*(volatile uint32_t *)(tdisp.ge + reg) = val;
	else if (tdisp.use_mmio)
		*(volatile uint32_t *)(tdisp.mmio + 0x2100 + reg) = val;
	else
		outpd(0x2100 + reg, val);
}

static int
tg_ge_in8(int reg)
{
	if (tdisp.ge != NULL)
		return tdisp.ge[reg];
	return tg_inb(0x2100 + reg);
}

/*
 * Engine-busy poll: status 2120h bit5, the bit the NT driver spins
 * on before programming and before streaming data.  Bounded: on a
 * timeout the engine is retired for the session and the caller's
 * path falls back to direct aperture stores.
 */
#define TG_GE_TIMEOUT	1000000L

static bool
tg_ge_wait(const char *who)
{
	long n;

	for (n = 0; n < TG_GE_TIMEOUT; n++) {
		if ((tg_ge_in8(0x20) & 0x20) == 0)
			return true;
	}
	hal_log_info("TRIDENT-GE: busy timeout (%s, status=%02Xh); "
		     "disabling the engine, falling back to direct "
		     "aperture stores.", who, tg_ge_in8(0x20));
	tdisp.use_ge = false;
	return false;
}

/* Replicate a pixel value across the 32-bit colour registers. */
static uint32_t
tg_ge_color(uint32_t c)
{
	if (tdisp.bpp == 16) {
		c &= 0xffffUL;
		return c | (c << 16);
	}
	/* 8bpp, and 24bpp in the byte-granular engine mode. */
	c &= 0xffUL;
	return c | (c << 8) | (c << 16) | (c << 24);
}

/*
 * Solid fill (ROP F0h + drawflag 4000h, per trident.dll).  xu/wu
 * are engine coordinates (pixels at 8/16bpp, bytes at 24bpp).
 * Runs on the command write; no data phase.
 */
static void
tg_ge_fill(int xu, int y, int wu, int h, uint32_t color)
{
	if (!tg_ge_wait("fill"))
		return;
	tg_ge_out8(0x27, 0xf0);
	tg_ge_out32(0x28, 0x00004000UL);
	tg_ge_out32(0x2c, tg_ge_color(color));
	tg_ge_out16(0x38, xu);
	tg_ge_out16(0x3a, y);
	tg_ge_out16(0x40, wu - 1);
	tg_ge_out16(0x42, h - 1);
	tg_ge_out8(0x24, 0x01);
	(void)tg_ge_wait("fill end");
}

/*
 * Prove the engine before trusting a frame to it.
 *
 * Field lesson (Ra43 log of 2026-07): GE registers other than the
 * 2120h status read back 00h on real silicon - they are write-only
 * (NEC's own NT4 GDI driver never reads any GE register except the
 * status, which corroborates this).  Register readback is therefore
 * DIAGNOSTIC ONLY and never fails the test; the decision is purely
 * functional:
 *
 *  Stage A (informational): readback of ROP/drawflag/dest/status,
 *  logged for the record.
 *
 *  Stage B (decisive, needs a readable aperture): a 16-unit solid
 *  fill and a 16-byte CPU-source blit, executed by the engine and
 *  verified through the aperture.  Reads through the aperture are
 *  reliable on the target board (only bulk WRITES drop), and the
 *  pre-zeroing is retried with verification to survive the ~25%
 *  direct-write success rate.
 *
 *  Without a readable aperture stage B is impossible; the engine
 *  is then accepted blind (see tg_ge_init) and judged on screen.
 */

/* Zero the first n bytes of a row, verified, direct-write-loss
 * tolerant (bounded retry; aperture reads are reliable). */
static bool
tg_ge_zero(volatile uint8_t *chk, int n)
{
	int pass, i;

	for (pass = 0; pass < 64; pass++) {
		bool ok = true;

		for (i = 0; i < n; i++)
			chk[i] = 0;
		for (i = 0; i < n; i++)
			if (chk[i] != 0) {
				ok = false;
				break;
			}
		if (ok)
			return true;
	}
	hal_log_info("TRIDENT-GE: cannot even zero %d bytes through "
		     "the aperture after 64 verified passes; "
		     "residue: %02Xh %02Xh %02Xh %02Xh %02Xh %02Xh "
		     "%02Xh %02Xh.", n,
		     chk[0], chk[1], chk[2], chk[3],
		     chk[4], chk[5], chk[6], chk[7]);
	return false;
}

static bool
tg_ge_selftest(void)
{
	static const uint8_t pat[16] = {
		0x5a, 0xa5, 0x3c, 0xc3, 0x0f, 0xf0, 0x69, 0x96,
		0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf1
	};
	volatile uint8_t *chk;
	int xu, i, bad, nfill, round;

	/* --- Stage A: readback, for the log only ------------------ */

	tg_ge_out8(0x27, 0xcc);
	tg_ge_out32(0x28, 0x00005024UL);
	tg_ge_out16(0x38, 0x0123);
	hal_log_info("TRIDENT-GE: readback (diagnostic): status="
		     "%02Xh ROP(cc)=%02Xh flags(24,50)=%02Xh,%02Xh "
		     "destX(23,01)=%02Xh,%02Xh GER22(%02Xh)=%02Xh.",
		     tg_ge_in8(0x20), tg_ge_in8(0x27),
		     tg_ge_in8(0x28), tg_ge_in8(0x29),
		     tg_ge_in8(0x38), tg_ge_in8(0x39),
		     tdisp.ge_opermode, tg_ge_in8(0x22));
	tg_ge_out32(0x28, 0x00000000UL);
	tg_ge_out16(0x38, 0);

	if (!tdisp.aper_ok) {
		hal_log_info("TRIDENT-GE: aperture unreadable: no "
			     "functional test possible here; the "
			     "engine is accepted and judged by the "
			     "fetch-experiment bars on screen.");
		return true;
	}

	/* --- Stage B: functional, verified through the aperture ---
	 * Test location: byte offset 64 of row 0 - inside the base
	 * region the aperture test already proved good (row scr_h
	 * turned out to be un-writable on the field board for
	 * reasons still unknown; do not test there).  The transient
	 * visible pixels are repainted by the engine clear on
	 * success and by the fetch bars on failure. */

	xu = 64 / ((tdisp.bpp == 16) ? 2 : 1);
	chk = tdisp.fb + 64;
	nfill = (tdisp.bpp == 16) ? 32 : 16;

	/* Solid fill of 16 engine units with A5h in every byte. */
	if (!tg_ge_zero(chk, 32))
		return false;
	tg_ge_fill(xu, 0, 16,  1,
		   (tdisp.bpp == 16) ? 0xa5a5UL : 0xa5UL);
	bad = 0;
	for (i = 0; i < nfill; i++)
		if (chk[i] != 0xa5)
			bad++;
	if (bad != 0) {
		/*
		 * The status may be write-only silicon (reads 00h)
		 * so the waits cannot pace a slow engine; give it
		 * a settle and look again before condemning it.
		 */
		tg_wait_ms(2);
		bad = 0;
		for (i = 0; i < nfill; i++)
			if (chk[i] != 0xa5)
				bad++;
	}
	if (bad != 0) {
		hal_log_info("TRIDENT-GE: solid-fill test failed "
			     "(%d of %d bytes wrong at offset 64: "
			     "%02Xh %02Xh %02Xh %02Xh ...); the "
			     "engine is not executing.",
			     bad, nfill,
			     chk[0], chk[1], chk[2], chk[3]);
		return false;
	}
	hal_log_info("TRIDENT-GE: solid-fill test PASSED (engine "
		     "executes; register readback state is "
		     "irrelevant).");

	/* CPU-source blit of the 16-byte pattern.  If the data
	 * stream itself gets dropped on the bus the operation sits
	 * waiting; retry the stream a few rounds and log how many
	 * it took - that number is the FIFO-loss diagnostic. */
	if (!tg_ge_wait("selftest"))
		return false;
	tg_ge_out8(0x27, 0xcc);
	tg_ge_out32(0x28, 0x00000000UL);
	tg_ge_out16(0x38, xu);
	tg_ge_out16(0x3a, 0);
	tg_ge_out16(0x40, ((tdisp.bpp == 16) ? 8 : 16) - 1);
	tg_ge_out16(0x42, 0);
	tg_ge_out8(0x24, 0x01);
	if (!tg_ge_wait("selftest data"))
		return false;
	for (round = 0; round < 4; round++) {
		if (tdisp.aper_width == 1) {
			volatile uint8_t *out = tdisp.fb;

			for (i = 0; i < 16; i++)
				out[i] = pat[i];
		} else {
			volatile uint32_t *out =
				(volatile uint32_t *)tdisp.fb;
			const uint32_t *in = (const uint32_t *)pat;

			for (i = 0; i < 4; i++)
				out[i] = in[i];
		}
		bad = 0;
		for (i = 0; i < 16; i++)
			if (chk[i] != pat[i])
				bad++;
		if (bad != 0) {
			tg_wait_ms(1);	/* settle, then re-check */
			bad = 0;
			for (i = 0; i < 16; i++)
				if (chk[i] != pat[i])
					bad++;
		}
		if (bad == 0)
			break;
	}
	if (bad != 0) {
		hal_log_info("TRIDENT-GE: CPU-source test failed "
			     "after %d stream rounds (%d of 16 "
			     "bytes wrong at offset 64: %02Xh %02Xh "
			     "%02Xh %02Xh ...).",
			     round, bad,
			     chk[0], chk[1], chk[2], chk[3]);
		return false;
	}
	hal_log_info("TRIDENT-GE: CPU-source test PASSED in %d "
		     "stream round%s.",
		     round + 1, round == 0 ? "" : "s");

	/* Scrub the test row (verified). */
	(void)tg_ge_zero(chk, 32);
	return true;
}

/*
 * Engine bring-up: operation mode + open clipping, then the
 * self-test, then an engine clear of the visible screen (which
 * also repairs any direct-store droppage from the memset above).
 */
static void
tg_ge_init(void)
{
	tdisp.use_ge = false;

	/*
	 * With the aperture unverified, the CPU-source data still
	 * has to be addressed AT the chip's memory decode - the
	 * FIFO capture is a PCI-target mechanism, independent of
	 * whether DRAM commits from direct stores survive.  Point
	 * the port at the CR21-decoded window (the decode that is
	 * field-proven on this wiring); it is used write-only.
	 */
	if (!tdisp.aper_ok) {
		int cr21 = tg_crtc_read(0x21);
		uint32_t w = ((uint32_t)(cr21 & 0x0f) << 28) |
			     ((uint32_t)((cr21 >> 6) & 0x03) << 24);

		if (w != 0 && w != tdisp.fb_phys) {
			uint8_t *port;

			port = (uint8_t *)tg_map_physical(w,
							  tdisp.pitch);
			if (port != NULL) {
				tdisp.fb = port;
				tdisp.fb_phys = w;
			}
		}
		hal_log_info("TRIDENT-GE: aperture unverified; the "
			     "FIFO data port is the CR21(%02Xh)-"
			     "decoded window at %08lXh, write-only.",
			     cr21, (unsigned long)tdisp.fb_phys);
	}

	/*
	 * Place the GE block (the trident.sys recipe, chip-typed):
	 * the miniport types the chip by SR0B/SR09 and hands the GDI
	 * a GE range of BAR1 + 0, length 100h for the TGUI9660
	 * family (SR0B=D3h; the Ra43 class) - the GE registers
	 * co-reside with the register file in the BOTTOM 256 BYTES
	 * of BAR1 (VGA relocations start at 3C0h, no overlap).  Only
	 * the TGUI9320 (SR0B=A3h, "type 2") uses BAR1 + 10000h with
	 * a 64KB range.  Both placements are tried here, NEC's pick
	 * for this family first, with the functional self-test as
	 * the oracle.  For each: CR34 = physical bits 23:16, CR35 =
	 * bits 31:24, CR36 bits1:0 = 11b (decode select: 00b =
	 * DISABLED - the Ra43 ITF state, 01b/10b = legacy B4000h/
	 * BC000h windows, 11b = the CR34/35 block; cf. 86Box
	 * tgui_recalcmapping).  PCI cfg 14h bit1 is set the way the
	 * miniport does at init; everything is restored at cleanup.
	 */
	tdisp.ge = NULL;
	tdisp.ge_phys = 0;
	{
		uint32_t bar1 = tdisp.mmio_phys;
		int cand;

		if (bar1 == 0)
			bar1 = pci_read32(pci_bus, pci_dev, pci_fn,
					  0x14) & ~0xfUL;
		if (bar1 == 0) {
			hal_log_info("TRIDENT-GE: no BAR1; cannot "
				     "place the engine block.");
		} else {
			uint32_t v;

			v = pci_read32(pci_bus, pci_dev, pci_fn, 0x14);
			if ((v & 0x02UL) == 0) {
				if (sv_cfg14 == 0xffffffffUL)
					sv_cfg14 = v;
				pci_write32(pci_bus, pci_dev, pci_fn,
					    0x14, v | 0x02UL);
			}
		}

		for (cand = 0; bar1 != 0 && cand < 2; cand++) {
			uint32_t phys = bar1 +
				(cand == 0 ? 0UL : 0x10000UL);
			volatile uint8_t *map;
			int cr36;

			if (cand == 0 && tdisp.use_mmio &&
			    tdisp.mmio != NULL)
				map = tdisp.mmio;
			else
				map = (volatile uint8_t *)
					tg_map_physical(phys, 0x10000);
			if (map == NULL)
				continue;

			tdisp.ge = map;
			tdisp.ge_phys = phys;
			tg_crtc_write(0x34,
				      (int)((phys >> 16) & 0xff));
			tg_crtc_write(0x35,
				      (int)((phys >> 24) & 0xff));
			cr36 = tg_crtc_read(0x36);
			tg_crtc_write(0x36, (cr36 & ~0x03) | 0x03);
			hal_log_info("TRIDENT-GE: placement %d: block "
				     "at %08lXh (BAR1+%lXh), CR34/35="
				     "%02Xh/%02Xh, CR36 %02Xh->%02Xh.",
				     cand, (unsigned long)phys,
				     (unsigned long)(phys - bar1),
				     tg_crtc_read(0x34),
				     tg_crtc_read(0x35),
				     cr36, tg_crtc_read(0x36));

			/* Program the neutral engine state. */
			tg_ge_out16(0x22, tdisp.ge_opermode);
			tg_ge_out32(0x48, 0x00000000UL);
			tg_ge_out32(0x4c, (2047UL << 16) | 4095UL);
			tg_ge_out32(0x28, 0x00000000UL);
			tg_ge_out16(0x3c, 0);
			tg_ge_out16(0x3e, 0);
			tg_ge_out16(0x34, 0);

			if (tg_ge_selftest())
				goto placed;
		}
		tdisp.ge = NULL;
		tdisp.ge_phys = 0;
	}

	if (tdisp.ge == NULL) {
		hal_log_info("TRIDENT-GE: no placement passed the "
			     "functional test; frames will go through "
			     "direct aperture stores (the old path).");
		tdisp.use_ge = false;
		return;
	}
placed:

	tdisp.use_ge = true;
	hal_log_info("TRIDENT-GE: self-test passed; frames will go "
		     "through the CPU-source FIFO BLT "
		     "(GER22=%02Xh, clip open, %s coordinates).",
		     tdisp.ge_opermode,
		     tdisp.ge_xmul == 3 ? "byte" : "pixel");

	/* Engine clear of the visible screen. */
	tg_ge_fill(0, 0, tdisp.scr_w * tdisp.ge_xmul, tdisp.scr_h, 0);
}

#endif /* TG_TRY_GE */

/*****************************************************************************/
/* Register file access                                                      */
/*****************************************************************************/

/*
 * The access-path switch.  MMIO uses the port number as the offset
 * into the BAR1 block, exactly the layout Linux tridentfb and Xorg
 * use ("MMIOonly" boards); volatile keeps Watcom from caching.
 */
static int
tg_inb(unsigned port)
{
	if (tdisp.use_mmio)
		return tdisp.mmio[port];
	return inp(port);
}

static void
tg_outb(unsigned port, int val)
{
	if (tdisp.use_mmio)
		tdisp.mmio[port] = (uint8_t)val;
	else
		outp(port, val);
}

static void
tg_set_iobase(uint16_t b3c0, uint16_t d4c, uint16_t dac,
	      uint16_t d4m, uint16_t dam)
{
	tdisp.io_3c0 = b3c0;
	tdisp.io_3d4_col = d4c;
	tdisp.io_3da_col = dac;
	tdisp.io_3d4_mono = d4m;
	tdisp.io_3da_mono = dam;
	tdisp.io_3d4 = d4c;
	tdisp.io_3da = dac;
	/* 3C8h + 4000h and 3C8h + 8000h, in base-relative terms. */
	tdisp.io_vclk = (uint16_t)((b3c0 + 0x08) | 0x4000);
	tdisp.io_sdac = (uint16_t)((b3c0 + 0x08) | 0x8000);
}

/* Point the CRTC/ST1 accessors at the block MISC bit0 selects. */
static void
tg_select_crtc(int misc)
{
	if (misc & 0x01) {
		tdisp.io_3d4 = tdisp.io_3d4_col;
		tdisp.io_3da = tdisp.io_3da_col;
	} else {
		tdisp.io_3d4 = tdisp.io_3d4_mono;
		tdisp.io_3da = tdisp.io_3da_mono;
	}
}

static void
tg_seq_write(int reg, int val)
{
	tg_outb(tdisp.io_3c0 + 0x04, reg);
	tg_outb(tdisp.io_3c0 + 0x05, val);
}

static int
tg_seq_read(int reg)
{
	tg_outb(tdisp.io_3c0 + 0x04, reg);
	return tg_inb(tdisp.io_3c0 + 0x05);
}

static void
tg_gfx_write(int reg, int val)
{
	tg_outb(tdisp.io_3c0 + 0x0e, reg);
	tg_outb(tdisp.io_3c0 + 0x0f, val);
}

static int
tg_gfx_read(int reg)
{
	tg_outb(tdisp.io_3c0 + 0x0e, reg);
	return tg_inb(tdisp.io_3c0 + 0x0f);
}

static void
tg_crtc_write(int reg, int val)
{
	tg_outb(tdisp.io_3d4, reg);
	tg_outb(tdisp.io_3d4 + 1, val);
}

static int
tg_crtc_read(int reg)
{
	tg_outb(tdisp.io_3d4, reg);
	return tg_inb(tdisp.io_3d4 + 1);
}

static void
tg_attr_write(int reg, int val)
{
	(void)tg_inb(tdisp.io_3da);	/* reset the index/data flip-flop */
	tg_outb(tdisp.io_3c0 + 0x00, reg);
	tg_outb(tdisp.io_3c0 + 0x00, val);
}

static int
tg_attr_read(int reg)
{
	int val;

	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0 + 0x00, reg);
	val = tg_inb(tdisp.io_3c0 + 0x01);
	(void)tg_inb(tdisp.io_3da);	/* leave the flip-flop reset */
	return val;
}

static void
tg_misc_write(int val)
{
	tg_outb(tdisp.io_3c0 + 0x02, val);	/* 3C2h: write */
	tg_select_crtc(val);			/* keep CRTC base coherent */
}

static int
tg_misc_read(void)
{
	return tg_inb(tdisp.io_3c0 + 0x0c);	/* 3CCh: read */
}

/*
 * The Trident hidden DAC register: read the DAC Write Index (3C8h)
 * once to reset the state machine, read the Pixel Mask (3C6h) four
 * times, and the next 3C6h access hits the hidden register.  A final
 * 3C8h read resets the state again.
 */
static void
tg_hidden_dac_write(int val)
{
	(void)tg_inb(tdisp.io_3c0 + 0x08);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	tg_outb(tdisp.io_3c0 + 0x06, val);
	(void)tg_inb(tdisp.io_3c0 + 0x08);
}

static int
tg_hidden_dac_read(void)
{
	int val;

	(void)tg_inb(tdisp.io_3c0 + 0x08);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x06);
	val = tg_inb(tdisp.io_3c0 + 0x06);
	(void)tg_inb(tdisp.io_3c0 + 0x08);
	return val;
}

/* The NEC SYNCDAC glue: 83C8h = index, 83C6h = data (pc98_tgui.c). */
static void
tg_sdac_write(int reg, int val)
{
	tg_outb(tdisp.io_sdac, reg);
	tg_outb((uint16_t)(tdisp.io_sdac - 2), val);
}

static int
tg_sdac_read(int reg)
{
	tg_outb(tdisp.io_sdac, reg);
	return tg_inb((uint16_t)(tdisp.io_sdac - 2));
}

/*
 * Old/new register modes.  WRITING SR0B selects the old mode;
 * READING it selects the new mode and returns the chip ID.
 */
static void
tg_sw_old(void)
{
	int v;

	v = tg_seq_read(0x0b);		/* (also: new mode) */
	tg_seq_write(0x0b, v);		/* write -> old mode */
}

static void
tg_sw_new(void)
{
	(void)tg_seq_read(0x0b);	/* read -> new mode */
}

/*
 * Millisecond-ish delay via the PC-98 wait port (5Fh, ~0.6us per
 * access) - no timers touched, works at any CPU speed.
 */
static void
tg_wait_ms(int ms)
{
	long i;

	for (i = 0; i < (long)ms * 1700L; i++)
		(void)inp(0x5f);
}

/*****************************************************************************/
/* PCI detection                                                             */
/*****************************************************************************/

#define PCI_CONFIG_ADDR		0x0cf8
#define PCI_CONFIG_DATA		0x0cfc
#define PCI_VENDOR_TRIDENT	0x1023

static uint32_t
pci_read32(int bus, int dev, int fn, int reg)
{
	outpd(PCI_CONFIG_ADDR,
	      0x80000000UL |
	      ((uint32_t)bus << 16) |
	      ((uint32_t)dev << 11) |
	      ((uint32_t)fn << 8) |
	      ((uint32_t)reg & 0xfc));

	return (uint32_t)inpd(PCI_CONFIG_DATA);
}

static void
pci_write32(int bus, int dev, int fn, int reg, uint32_t val)
{
	outpd(PCI_CONFIG_ADDR,
	      0x80000000UL |
	      ((uint32_t)bus << 16) |
	      ((uint32_t)dev << 11) |
	      ((uint32_t)fn << 8) |
	      ((uint32_t)reg & 0xfc));
	outpd(PCI_CONFIG_DATA, val);
}

/*
 * Scan the first buses for a Trident, logging everything we pass by.
 */
static bool
tg_pci_find(void)
{
	int bus, dev, fn, nfn, ndev;
	uint32_t id, classrev, bar0, cmd, mask, orig;
	uint16_t device;
	uint8_t rev;

	ndev = 0;
	for (bus = 0; bus < 4; bus++) {
		for (dev = 0; dev < 32; dev++) {
			id = pci_read32(bus, dev, 0, 0x00);
			if ((id & 0xffff) == 0xffff || (id & 0xffff) == 0)
				continue;
			nfn = (pci_read32(bus, dev, 0, 0x0c) &
			       0x00800000UL) ? 8 : 1;
			for (fn = 0; fn < nfn; fn++) {
				id = pci_read32(bus, dev, fn, 0x00);
				if ((id & 0xffff) == 0xffff ||
				    (id & 0xffff) == 0)
					continue;
				classrev = pci_read32(bus, dev, fn, 0x08);
				hal_log_info("TRIDENT: PCI %d:%d.%d = "
					     "%04lX:%04lX class %02lXh "
					     "rev %02lXh.",
					     bus, dev, fn,
					     (unsigned long)(id & 0xffff),
					     (unsigned long)(id >> 16),
					     (unsigned long)(classrev >> 24),
					     (unsigned long)(classrev & 0xff));
				ndev++;
				if ((id & 0xffff) != PCI_VENDOR_TRIDENT)
					continue;

				device = (uint16_t)(id >> 16);
				rev = (uint8_t)(classrev & 0xff);
				if (device != 0x9660) {
					hal_log_info("TRIDENT: device "
						     "%04Xh is not the "
						     "9660 desktop family "
						     "(Cyber laptop or "
						     "unknown); leaving "
						     "it alone.", device);
					continue;
				}

				pci_bus = bus;
				pci_dev = dev;
				pci_fn = fn;

				tdisp.chip_name = "TGUI96xx family";

				/* BAR0 + decode size + enable. */
				cmd = pci_read32(bus, dev, fn, 0x04);
				bar0 = pci_read32(bus, dev, fn, 0x10);
				pci_write32(bus, dev, fn, 0x04,
					    cmd & ~0x2UL);
				orig = bar0;
				pci_write32(bus, dev, fn, 0x10,
					    0xffffffffUL);
				mask = pci_read32(bus, dev, fn, 0x10) &
				       ~0xfUL;
				pci_write32(bus, dev, fn, 0x10, orig);
				pci_write32(bus, dev, fn, 0x04,
					    cmd | 0x03);

				bar0 &= ~0xfUL;
				hal_log_info("TRIDENT: %s (PCI rev "
					     "%02Xh) at %d:%d.%d, "
					     "BAR0=%08lXh (decode %luMB), "
					     "cmd=%04lXh -> readback "
					     "%04lXh.",
					     tdisp.chip_name, rev,
					     bus, dev, fn,
					     (unsigned long)bar0,
					     (unsigned long)
					     ((mask ? (~mask + 1) : 0)
					      >> 20),
					     (unsigned long)(cmd & 0xffff),
					     (unsigned long)
					     (pci_read32(bus, dev, fn,
							 0x04) & 0xffff));
				if (bar0 == 0) {
					hal_log_info("TRIDENT: BAR0 is "
						     "unassigned, giving "
						     "up.");
					return false;
				}

				tdisp.fb_phys = bar0;

				/*
				 * BAR1 is the 64KB register MMIO
				 * block (tridentfb/Xorg).  If the ITF
				 * left it unassigned, park it just
				 * past BAR0's 4MB decode.
				 */
				{
					uint32_t bar1;

					bar1 = pci_read32(bus, dev, fn,
							  0x14);
					bar1 &= ~0xfUL;
					if (bar1 == 0) {
						bar1 = bar0 + 0x400000UL;
						pci_write32(bus, dev, fn,
							    0x14, bar1);
						bar1 = pci_read32(bus, dev,
								  fn, 0x14)
						       & ~0xfUL;
						hal_log_info("TRIDENT: "
							     "BAR1 was "
							     "unassigned; "
							     "parked at "
							     "%08lXh.",
							     (unsigned long)
							     bar1);
					} else {
						hal_log_info("TRIDENT: "
							     "BAR1 (register "
							     "MMIO) at "
							     "%08lXh.",
							     (unsigned long)
							     bar1);
					}
					tdisp.mmio_phys = bar1;
				}
				return true;
			}
		}
	}

	if (ndev == 0)
		hal_log_info("TRIDENT: PCI config space is silent.");
	else
		hal_log_info("TRIDENT: no Trident on PCI (%d devices "
			     "seen); the 96xx built-ins are PCI, so "
			     "yielding to other drivers.", ndev);
	return false;
}

/*
 * Dump a PCI configuration header to the log.
 */
static void
tg_pci_dump_header(int bus, int dev, int fn)
{
	int reg;

	hal_log_info("TRIDENT: PCI config header of %d:%d.%d:",
		     bus, dev, fn);
	for (reg = 0; reg < 0x40; reg += 0x10) {
		hal_log_info("TRIDENT:   %02Xh: %08lXh %08lXh "
			     "%08lXh %08lXh.", reg,
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 4),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 8),
			     (unsigned long)pci_read32(bus, dev,
						       fn, reg + 12));
	}
}

static void
tg_pci_dump_config(void)
{
	tg_pci_dump_header(pci_bus, pci_dev, pci_fn);
}

/*****************************************************************************/
/* Wakeup and fingerprint                                                    */
/*****************************************************************************/

/*
 * A cheap liveness test for the current register access path: the
 * pattern of an existing chip is SR0B == D3h; a dead path reads FFh
 * everywhere (also probe SR00/SR01 so a floating D3h can't fool us).
 */
static bool
tg_regs_alive(void)
{
	int id, s0, s1;

	tg_sw_old();
	id = tg_seq_read(0x0b);		/* also selects the new mode */
	s0 = tg_seq_read(0x00);
	s1 = tg_seq_read(0x01);

	if (id == 0xff && s0 == 0xff && s1 == 0xff)
		return false;
	return id == 0xd3;
}

/*
 * The strap-guided wakeup (pc98_tgui.c VideoEnable(), verbatim).
 * Runs in the OLD register mode: SR0E bit5 there selects the
 * Configuration Port at SR0C, whose bit4 straps the wakeup flavor.
 * Only meaningful when the register path already answers.
 */
static void
tg_wakeup_strapped(void)
{
	int tmp, cfg;

	/* MISC: RAM enable, color I/O (ChipInit does this first). */
	tg_misc_write(tg_misc_read() | 0xc3);

	tg_sw_old();

	tmp = tg_seq_read(0x0e);
	tg_seq_write(0x0e, tmp | 0x20);	/* select Configuration Port 1 */
	cfg = tg_seq_read(0x0c);
	tg_seq_write(0x0e, tmp);

	hal_log_info("TRIDENT: wakeup: old-mode SR0E=%02Xh, SR0C=%02Xh "
		     "-> %s scheme.", tmp, cfg,
		     (cfg & 0x10) ? "PC-98 (94h/102h/3C3h)"
				  : "AT (46E8h/102h)");

	if ((cfg & 0x10) == 0x10)
		tg_wakeup_blind_pc98();
	else
		tg_wakeup_blind_at();
}

/* The AT-style setup-port wakeup.  Safe to fire blind. */
static void
tg_wakeup_blind_at(void)
{
	hal_log_info("TRIDENT: wakeup: firing the AT scheme "
		     "(46E8h/102h).");
	outp(0x46e8, 0x10);
	outp(0x102, 0x01);
	outp(0x46e8, 0x08);
}

/*
 * The PC-98-style setup-port wakeup.  NOT safe to fire blind on an
 * arbitrary machine: if the Trident does not claim port 94h, the
 * write lands on the PC-98 FDC mode register.  Fired last, and only
 * when everything else has already failed.
 */
static void
tg_wakeup_blind_pc98(void)
{
	int v;

	hal_log_info("TRIDENT: wakeup: firing the PC-98 scheme "
		     "(94h/102h/3C3h) - last resort.");
	outp(0x94, 0x00);
	outp(0x102, 0x01);
	outp(0x94, 0x20);
	v = inp(tdisp.io_3c0 + 0x03);
	outp(tdisp.io_3c0 + 0x03, v == 0xff ? 0x01 : (v | 0x01));
}

/*
 * Establish a working register access path.
 *
 *  1. Legacy PIO at the native 3C0h block (the V13/V16 wiring).
 *  2. BAR1 MMIO (the Ra wiring; CR39 bit0 strapped on).
 *  3. Blind wakeups (AT scheme first, the FDC-hazardous 94h scheme
 *     last), then a PIO retest.
 */
static bool
tg_probe_access_path(void)
{
	tdisp.use_mmio = false;

	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: legacy VGA I/O answers at "
			     "%03Xh; using PIO.", tdisp.io_3c0);
		tg_wakeup_strapped();
		return true;
	}
	hal_log_info("TRIDENT: legacy VGA I/O at %03Xh is dead "
		     "(reads FFh); trying BAR1 MMIO.", tdisp.io_3c0);

	if (tdisp.mmio_phys != 0) {
		tdisp.mmio = (volatile uint8_t *)
			tg_map_physical(tdisp.mmio_phys, 0x10000);
		if (tdisp.mmio != NULL) {
			tdisp.use_mmio = true;
			if (tg_regs_alive()) {
				hal_log_info("TRIDENT: BAR1 MMIO at "
					     "%08lXh answers (SR0B via "
					     "memory); using MMIO for "
					     "all registers.",
					     (unsigned long)
					     tdisp.mmio_phys);
				return true;
			}
			tdisp.use_mmio = false;
			hal_log_info("TRIDENT: BAR1 MMIO is mapped but "
				     "the register file does not "
				     "answer there.");
		} else {
			hal_log_info("TRIDENT: can't map BAR1 at "
				     "%08lXh.",
				     (unsigned long)tdisp.mmio_phys);
		}
	} else {
		hal_log_info("TRIDENT: no BAR1 to try.");
	}

	/* Both blind wakeups, least dangerous first, then retest PIO. */
	tg_wakeup_blind_at();
	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: PIO came alive after the AT "
			     "wakeup.");
		return true;
	}
	tg_wakeup_blind_pc98();
	if (tg_regs_alive()) {
		hal_log_info("TRIDENT: PIO came alive after the PC-98 "
			     "wakeup.");
		return true;
	}

	hal_log_info("TRIDENT: no register access path works (PIO "
		     "dead, MMIO dead, wakeups didn't help).  Config "
		     "dump follows for diagnosis.");
	return false;
}

/*
 * Prove there is a Trident TGUI96xx behind the VGA block before a
 * single register is programmed:
 *  - reading SR0B must return the 9660-family ID D3h;
 *  - the classic signature: writing 00h to new-mode SR0E must read
 *    back 02h (the hardware inverts bit1 on the way in).
 */
static bool
tg_fingerprint(void)
{
	int id, rev, old0e, sig;

	tg_sw_old();
	id = tg_seq_read(0x0b);		/* read: chip ID, now new mode */
	rev = tg_seq_read(0x09);

	old0e = tg_seq_read(0x0e);
	tg_seq_write(0x0e, 0x00);
	sig = tg_seq_read(0x0e);
	tg_seq_write(0x0e, old0e ^ 0x02);	/* re-store the old value */

	hal_log_info("TRIDENT: fingerprint: SR0B=%02Xh SR09=%02Xh, "
		     "SR0E write-00h reads %02Xh (expect xxx2h).",
		     id, rev, sig);

	if (id != 0xd3) {
		hal_log_info("TRIDENT: SR0B is not the TGUI9660-family "
			     "ID (D3h); refusing to program the chip.");
		return false;
	}
	if ((sig & 0x0f) != 0x02) {
		hal_log_info("TRIDENT: the SR0E bit1-invert signature "
			     "failed; refusing to program the chip.");
		return false;
	}

	tdisp.chip_id = (uint8_t)id;
	tdisp.chip_rev = (uint8_t)rev;

	/* SR09 is the authoritative revision. */
	switch (rev) {
	case 0x00:
		tdisp.chip_name = "TGUI9660";
		break;
	case 0x01:
		tdisp.chip_name = "TGUI9680";
		break;
	case 0x10:
		tdisp.chip_name = "ProVidia TGUI9682";
		break;
	case 0x21:
		tdisp.chip_name = "ProVidia TGUI9685";
		break;
	default:
		tdisp.chip_name = "TGUI96xx (unrecognized SR09)";
		break;
	}
	hal_log_info("TRIDENT: chip fingerprint OK: %s via %s.",
		     tdisp.chip_name,
		     tdisp.use_mmio ? "BAR1 MMIO" : "legacy PIO");
	return true;
}

/*
 * One write/read cycle against the mapped framebuffer, fully
 * logged.  Retried up to three times: on this board direct stores
 * are known to be dropped probabilistically against scanout (the
 * very defect the FIFO BLT path exists for), and a single dropped
 * dword must not condemn a live aperture.
 */
static bool
tg_aperture_test(const char *tag)
{
	volatile uint32_t *p = (volatile uint32_t *)tdisp.fb;
	volatile uint8_t *b = (volatile uint8_t *)tdisp.fb;
	uint32_t r0, r1, w0, w1;
	int b0, b1, b2, b3;
	int attempt;
	bool dw_ok, by_ok;

	dw_ok = by_ok = false;
	r0 = r1 = w0 = w1 = 0;
	b0 = b1 = b2 = b3 = 0;

	for (attempt = 1; attempt <= 3; attempt++) {
		/* Dword cycles at offsets 0/4. */
		r0 = p[0];
		r1 = p[1];
		p[0] = 0x55aa1234UL;
		p[1] = 0xc3a5960fUL;
		w0 = p[0];
		w1 = p[1];
		dw_ok = (w0 == 0x55aa1234UL && w1 == 0xc3a5960fUL);

		/* Byte cycles at offsets 8..0Bh. */
		b[8] = 0xa5;
		b[9] = 0x5a;
		b[10] = 0xc3;
		b[11] = 0x3c;
		b0 = b[8];
		b1 = b[9];
		b2 = b[10];
		b3 = b[11];
		by_ok = (b0 == 0xa5 && b1 == 0x5a &&
			 b2 == 0xc3 && b3 == 0x3c);

		if (dw_ok || by_ok)
			break;
	}

	hal_log_info("TRIDENT: aperture test (%s, attempt %d): "
		     "dwords found %08lXh %08lXh, after write "
		     "%08lXh %08lXh -> "
		     "%s; bytes read %02Xh %02Xh %02Xh %02Xh -> %s.",
		     tag, attempt > 3 ? 3 : attempt,
		     (unsigned long)r0, (unsigned long)r1,
		     (unsigned long)w0, (unsigned long)w1,
		     dw_ok ? "OK" : "dead",
		     b0, b1, b2, b3,
		     by_ok ? "OK" : "dead");

	if (dw_ok) {
		tdisp.aper_width = 4;
		return true;
	}
	if (by_ok) {
		tdisp.aper_width = 1;
		hal_log_info("TRIDENT: aperture is byte-lane only; "
			     "blits will fall back to byte copies.");
		return true;
	}
	return false;
}

/*
 * Get the linear aperture reaching VRAM (BAR0 first, then the NT
 * hidden decode enable, then BAR2 / the CR21-decoded window / the
 * NT4 fixed 73000000h window).
 */
static bool
tg_aperture_fix(void)
{
	uint32_t cand[3];
	const char *cname[3];
	uint8_t *newfb;
	int cr21, i;

	if (tg_aperture_test("BAR0"))
		return true;

	/*
	 * The NT4 driver's enable path sets bit1 of PCI config reg
	 * 14h before touching the framebuffer.  Set it and retest.
	 */
	{
		uint32_t v;

		v = pci_read32(pci_bus, pci_dev, pci_fn, 0x14);
		sv_cfg14 = v;
		pci_write32(pci_bus, pci_dev, pci_fn, 0x14,
			    v | 0x02UL);
		hal_log_info("TRIDENT: PCI cfg14h (NT hidden decode "
			     "enable): %08lXh -> %08lXh.",
			     (unsigned long)v,
			     (unsigned long)pci_read32(pci_bus,
						       pci_dev,
						       pci_fn,
						       0x14));
	}
	if (tg_aperture_test("BAR0 + cfg14h.1"))
		return true;

	cr21 = tg_crtc_read(0x21);

	cand[0] = pci_read32(pci_bus, pci_dev, pci_fn, 0x18) & ~0xfUL;
	cname[0] = "BAR2";
	cand[1] = ((uint32_t)(cr21 & 0x0f) << 28) |
		  ((uint32_t)((cr21 >> 6) & 0x03) << 24);
	cname[1] = "CR21 window";
	cand[2] = 0x73000000UL;	/* NEC NT4 fixed range, backstop */
	cname[2] = "NT4 fixed window";

	hal_log_info("TRIDENT: BAR0 aperture is dead; candidates: "
		     "BAR2=%08lXh, CR21(%02Xh)-decoded=%08lXh, "
		     "NT4 fixed=73000000h.",
		     (unsigned long)cand[0], cr21,
		     (unsigned long)cand[1]);

	for (i = 0; i < 3; i++) {
		int j, dup;

		if (cand[i] == 0 || cand[i] == tdisp.fb_phys)
			continue;
		dup = 0;
		for (j = 0; j < i; j++)
			if (cand[j] == cand[i])
				dup = 1;
		if (dup)
			continue;
		newfb = (uint8_t *)tg_map_physical(cand[i],
						   tdisp.vram_size);
		if (newfb == NULL) {
			hal_log_info("TRIDENT: can't map %08lXh.",
				     (unsigned long)cand[i]);
			continue;
		}
		tdisp.fb = newfb;
		tdisp.fb_phys = cand[i];
		if (tg_aperture_test(cname[i])) {
			hal_log_info("TRIDENT: framebuffer adopted "
				     "at %08lXh (%s).",
				     (unsigned long)cand[i],
				     cname[i]);
			return true;
		}
	}

	hal_log_info("TRIDENT: no candidate window reaches VRAM; "
		     "the picture will show stale VRAM.  NEC "
		     "companion device headers for diagnosis:");
	tg_pci_dump_header(0, 6, 0);	/* 1033:002C bridge */
	tg_pci_dump_header(0, 7, 0);	/* 1033:0009 display */
	tg_pci_dump_header(pci_bus, pci_dev, pci_fn);
	return false;
}

/*
 * Measure the VRAM through the linear aperture: plant distinct tags
 * just under each candidate size, largest first; the largest intact
 * tag is the true size.
 */
static uint32_t
tg_vram_probe(void)
{
	volatile uint32_t *p;
	uint32_t sizes[3];
	uint32_t s;
	int i, cr21;

	sizes[0] = 4096UL * 1024UL;
	sizes[1] = 2048UL * 1024UL;
	sizes[2] = 1024UL * 1024UL;

	p = (volatile uint32_t *)tg_map_physical(tdisp.fb_phys,
						 sizes[0]);
	if (p == NULL)
		return 0;

	cr21 = tg_crtc_read(0x21);
	tg_crtc_write(0x21, cr21 | 0x20);	/* aperture on */

	p[0] = 0x55aa1234UL;
	for (i = 0; i < 3; i++)
		p[(sizes[i] - 16) / 4] = sizes[i] ^ 0x0badf00dUL;

	hal_log_info("TRIDENT: VRAM probe: base dword reads %08lXh "
		     "(wrote 55AA1234h).", (unsigned long)p[0]);
	s = 0;
	if (p[0] == 0x55aa1234UL) {
		for (i = 0; i < 3; i++) {
			if (p[(sizes[i] - 16) / 4] ==
			    (sizes[i] ^ 0x0badf00dUL)) {
				s = sizes[i];
				break;
			}
		}
		if (s == 0)
			s = 512UL * 1024UL;
	}

	tg_crtc_write(0x21, cr21);		/* as found */
	return s;
}

/* VRAM size from CR1F (SPR) low nibble. */
static uint32_t
tg_vram_size(void)
{
	int spr;
	uint32_t k;

	tg_sw_new();
	tg_select_crtc(tg_misc_read());
	spr = tg_crtc_read(0x1f);

	switch (spr & 0x0f) {
	case 0x01:
		k = 512UL * 1024UL;
		break;
	case 0x03:
		k = 1024UL * 1024UL;
		break;
	case 0x07:
		k = 2048UL * 1024UL;
		break;
	case 0x0f:
		k = 4096UL * 1024UL;
		break;
	default:
		hal_log_info("TRIDENT: CR1F=%02Xh is not a known VRAM "
			     "code; probing through the aperture.",
			     spr);
		k = tg_vram_probe();
		if (k != 0) {
			hal_log_info("TRIDENT: aperture probe -> "
				     "%luKB VRAM.",
				     (unsigned long)(k >> 10));
			return k;
		}
		k = 1024UL * 1024UL;
		hal_log_info("TRIDENT: aperture probe failed too; "
			     "assuming 1MB.");
		break;
	}
	hal_log_info("TRIDENT: CR1F=%02Xh -> %luKB VRAM.",
		     spr, (unsigned long)(k >> 10));
	return k;
}

/* Dump the registers that matter for remote debugging. */
static void
tg_dump_regs(const char *tag)
{
	tg_sw_new();
	tg_select_crtc(tg_misc_read());

	hal_log_info("TRIDENT: regs (%s):", tag);
	hal_log_info("TRIDENT:   MISC=%02Xh HDR=%02Xh SR01=%02Xh "
		     "SR0D=%02Xh SR0E=%02Xh SR0F=%02Xh.",
		     tg_misc_read(), tg_hidden_dac_read(),
		     tg_seq_read(0x01), tg_seq_read(0x0d),
		     tg_seq_read(0x0e), tg_seq_read(0x0f));
	hal_log_info("TRIDENT:   CR1E=%02Xh CR1F=%02Xh CR21=%02Xh "
		     "CR27=%02Xh CR29=%02Xh CR2A=%02Xh CR2B=%02Xh.",
		     tg_crtc_read(0x1e), tg_crtc_read(0x1f),
		     tg_crtc_read(0x21), tg_crtc_read(0x27),
		     tg_crtc_read(0x29), tg_crtc_read(0x2a),
		     tg_crtc_read(0x2b));
	hal_log_info("TRIDENT:   CR20=%02Xh CR23=%02Xh CR25=%02Xh "
		     "CR2F=%02Xh CR30=%02Xh CR34=%02Xh CR35=%02Xh "
		     "CR36=%02Xh CR38=%02Xh CR39=%02Xh.",
		     tg_crtc_read(0x20), tg_crtc_read(0x23),
		     tg_crtc_read(0x25), tg_crtc_read(0x2f),
		     tg_crtc_read(0x30), tg_crtc_read(0x34),
		     tg_crtc_read(0x35), tg_crtc_read(0x36),
		     tg_crtc_read(0x38), tg_crtc_read(0x39));
	hal_log_info("TRIDENT:   GR0F=%02Xh GR23=%02Xh GR2F=%02Xh; "
		     "VCLK=%02Xh/%02Xh MCLK=%02Xh/%02Xh; "
		     "SDAC[00]=%02Xh SDAC[04]=%02Xh; 0FACh=%02Xh.",
		     tg_gfx_read(0x0f), tg_gfx_read(0x23),
		     tg_gfx_read(0x2f),
		     tg_inb(tdisp.io_vclk), tg_inb(tdisp.io_vclk + 1),
		     tg_inb(tdisp.io_vclk - 2), tg_inb(tdisp.io_vclk - 1),
		     tg_sdac_read(0x00), tg_sdac_read(0x04),
		     inp(0x0fac));
}

/*****************************************************************************/
/* Mode set                                                                  */
/*****************************************************************************/

/*
 * CRTC values built with the tridentfb rules (standard VGA layout;
 * Trident keeps the 6-bit Horizontal Blanking End compare).
 *
 * 640x480@60: 25.175MHz, H 640/16/96/48, V 480/10/2/33, -h -v sync.
 */
static const uint8_t tg_crtc_640x480[] = {
	0x5f,	/* 00: Horizontal Total (800/8 - 5) */
	0x4f,	/* 01: Horizontal Display End (640/8 - 1) */
	0x50,	/* 02: Horizontal Blanking Start (640/8) */
	0x02,	/* 03: Horizontal Blanking End ((95+3) & 1Fh) */
	0x52,	/* 04: Horizontal Sync Start (656/8) */
	0x9e,	/* 05: Hsync End (752/8 & 1Fh), bit7 = HBE bit5 */
	0x0b,	/* 06: Vertical Total (523, low byte) */
	0x3e,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x40,	/* 09: Max Scan Line */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0xea,	/* 10: Vertical Sync Start (490, low byte) */
	0x0c,	/* 11: Vsync End (492 % 16), unprotected */
	0xdf,	/* 12: Vertical Display End (479, low byte) */
	0x00,	/* 13: Offset (patched from tdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0xe0,	/* 15: Vertical Blanking Start (480, low byte) */
	0x0b,	/* 16: Vertical Blanking End (523, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * 800x600@60 (VESA): 40.000MHz, H 800/40/128/88, V 600/1/4/23,
 * +h +v sync.
 */
static const uint8_t tg_crtc_800x600[] = {
	0x7f,	/* 00: Horizontal Total (1056/8 - 5) */
	0x63,	/* 01: Horizontal Display End (800/8 - 1) */
	0x64,	/* 02: Horizontal Blanking Start (800/8) */
	0x02,	/* 03: Horizontal Blanking End ((127+3) & 1Fh) */
	0x69,	/* 04: Horizontal Sync Start (840/8) */
	0x19,	/* 05: Hsync End (968/8 & 1Fh), bit7 = HBE bit5 = 0 */
	0x72,	/* 06: Vertical Total (626, low byte) */
	0xf0,	/* 07: Overflow */
	0x00,	/* 08: Preset Row Scan */
	0x60,	/* 09: Max Scan Line (bit5 = VBS bit9) */
	0x20,	/* 0A: Cursor Start (off) */
	0x00,	/* 0B: Cursor End */
	0x00,	/* 0C: Start Address High */
	0x00,	/* 0D: Start Address Low */
	0x00,	/* 0E: Cursor Location High */
	0x00,	/* 0F: Cursor Location Low */
	0x59,	/* 10: Vertical Sync Start (601, low byte) */
	0x0d,	/* 11: Vsync End (605 % 16), unprotected */
	0x57,	/* 12: Vertical Display End (599, low byte) */
	0x00,	/* 13: Offset (patched from tdisp.pitch at runtime) */
	0x00,	/* 14: Underline */
	0x58,	/* 15: Vertical Blanking Start (600, low byte) */
	0x72,	/* 16: Vertical Blanking End (626, low byte) */
	0xc3,	/* 17: Mode Control (byte mode, wrap) */
	0xff	/* 18: Line Compare */
};

/*
 * VCLK PLL pairs (old layout, ports 43C8h/43C9h):
 *   f = 14.31818MHz * (N+8) / ((M+2) * 2^K)
 *   43C8h = N | (M bit0 << 7),  43C9h = (M >> 1) | (K << 4)
 */
#define TG_VCLK_25175_LO	0xd7	/* N=87 M=25 K=1 -> 25.188MHz */
#define TG_VCLK_25175_HI	0x1c
#define TG_VCLK_40000_LO	0xbe	/* N=62 M=23 K=0 -> 40.091MHz */
#define TG_VCLK_40000_HI	0x0b

/* Hidden DAC (Command Register) per depth. */
static int
tg_hdr_value(void)
{
	switch (tdisp.bpp) {
	case 24:
		return 0xd0;	/* 8-8-8 packed truecolor */
	case 16:
		return 0x30;	/* 5-6-5 */
	default:
		return 0x00;	/* palette mode */
	}
}

/* CR38 (Pixel Bus) per depth (TGUI96xx values). */
static int
tg_pixelbus_value(void)
{
	switch (tdisp.bpp) {
	case 24:
		return 0x29;
	case 16:
		return 0x05;	/* 16bpp + 16-bit bus */
	default:
		return 0x00;
	}
}

/*
 * Load the DAC.  In the direct-color modes the DAC is bypassed, so a
 * grayscale ramp is loaded just in case.  In 8bpp the palette
 * implements RGB332, matching conv_row8().
 */
static void
tg_load_palette(void)
{
	int i;

	tg_outb(tdisp.io_3c0 + 0x06, 0xff);	/* no pixel mask */
	tg_outb(tdisp.io_3c0 + 0x08, 0x00);	/* write index 0 */

	if (tdisp.bpp == 8) {
		for (i = 0; i < 256; i++) {
			int r = (i >> 5) & 7;
			int g = (i >> 2) & 7;
			int b = i & 3;

			/* 6-bit DAC entries. */
			tg_outb(tdisp.io_3c0 + 0x09, r * 63 / 7);
			tg_outb(tdisp.io_3c0 + 0x09, g * 63 / 7);
			tg_outb(tdisp.io_3c0 + 0x09, b * 63 / 3);
		}
	} else {
		for (i = 0; i < 256; i++) {
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
			tg_outb(tdisp.io_3c0 + 0x09, i >> 2);
		}
	}
}

/*
 * The full mode set, assembled from tridentfb's BIOS-less order and
 * XF98's SetRegisters().  Leaves the screen blanked (SR01 bit5); the
 * caller unblanks after clearing VRAM.
 *
 * The five fetch-path registers (CR1E, CR2A, CR2F, GR0F, GR2F) are
 * NOT touched here: neither the XF98 values (stripes) nor the ITF
 * values (blue-only) are right on the Ra43-class board, so they are
 * owned by tg_fetch_experiment(), which walks the combinations.
 */
static void
tg_modeset(void)
{
	const uint8_t *tab;
	uint32_t offset;
	int i, misc;

	hal_log_info("TRIDENT: setting %dx%d %d bpp (pitch %lu), "
		     "VCLK %s.",
		     tdisp.scr_w, tdisp.scr_h, tdisp.bpp,
		     (unsigned long)tdisp.pitch,
		     tdisp.scr_w == 800 ? "40.0MHz" : "25.175MHz");

	if (tdisp.scr_w == 800) {
		tab = tg_crtc_800x600;
		misc = 0x2b;	/* +h +v sync, VCLK PLL, color, RAM */
	} else {
		tab = tg_crtc_640x480;
		misc = 0xeb;	/* -h -v sync, VCLK PLL, color, RAM */
	}
	offset = tdisp.pitch / 8;

	tg_sw_new();

	/* Sequencer: run, blank, extensions unlocked (bank 0). */
	tg_seq_write(0x00, 0x03);
	tg_seq_write(0x01, 0x21);	/* 8-dot clock, screen off */
	tg_seq_write(0x0e, 0x82);	/* -> stored 80h: ext on, bank 0 */
	tg_seq_write(0x02, 0x0f);	/* plane write mask */
	tg_seq_write(0x03, 0x00);	/* character map */
	tg_seq_write(0x04, 0x0e);	/* memory mode: ext, chain4 */

	/* XF98 SetRegisters: old-mode SR0D = 20h, new-mode SR0D = 0. */
	tg_sw_old();
	tg_seq_write(0x0d, 0x20);
	tg_sw_new();
	tg_seq_write(0x0d, 0x00);

	/* VCLK before MISC selects it. */
	if (tdisp.scr_w == 800) {
		tg_outb(tdisp.io_vclk, TG_VCLK_40000_LO);
		tg_outb(tdisp.io_vclk + 1, TG_VCLK_40000_HI);
	} else {
		tg_outb(tdisp.io_vclk, TG_VCLK_25175_LO);
		tg_outb(tdisp.io_vclk + 1, TG_VCLK_25175_HI);
	}

	/* MISC: clock select 10b = the programmable VCLK. */
	tg_misc_write(misc);

	/* CRTC: unlock, then the timing table with the pitch patched. */
	tg_crtc_write(0x11, tg_crtc_read(0x11) & 0x7f);
	for (i = 0; i < 0x19; i++) {
		if (i == 0x13)
			tg_crtc_write(i, (int)(offset & 0xff));
		else
			tg_crtc_write(i, tab[i]);
	}

	/* CR27: vertical overflow bits 10 (all zero here) + LC bit10. */
	tg_crtc_write(0x27, (tg_crtc_read(0x27) & 0x07) | 0x08);
	/* CR2B: horizontal overflow bits 8 (all zero for our modes). */
	tg_crtc_write(0x2b, 0x00);
	/*
	 * CR21: linear aperture on.  The Ra43 ITF leaves C7h here,
	 * so set bit5 and preserve the rest instead of overwriting.
	 */
	{
		int cr21 = tg_crtc_read(0x21);

		tg_crtc_write(0x21, cr21 | 0x20);
		hal_log_info("TRIDENT: CR21 %02Xh -> %02Xh "
			     "(linear aperture enabled).",
			     cr21, tg_crtc_read(0x21));
	}
	/* CR29: pitch bits 9:8 (keep bit2 - the relay uses it). */
	tg_crtc_write(0x29, (tg_crtc_read(0x29) & 0xcf) |
			    (int)((offset & 0x300) >> 4));

	/* CR38: pixel bus = the depth. */
	tg_crtc_write(0x38, tg_pixelbus_value());
	/*
	 * CR39: no PCI bursts.  Bit0 gates the BAR1 register MMIO -
	 * clearing it while running over MMIO would saw off the
	 * branch we sit on, so it is preserved (set) in MMIO mode.
	 */
	if (tdisp.use_mmio)
		tg_crtc_write(0x39, (tg_crtc_read(0x39) & ~0x06) | 0x01);
	else
		tg_crtc_write(0x39, tg_crtc_read(0x39) & ~0x07);
	/* CR50: hardware cursor off. */
	tg_crtc_write(0x50, 0x00);

	/* Graphics controller: packed-pixel graphics at A0000. */
	tg_gfx_write(0x00, 0x00);
	tg_gfx_write(0x01, 0x00);
	tg_gfx_write(0x02, 0x00);
	tg_gfx_write(0x03, 0x00);
	tg_gfx_write(0x04, 0x00);
	tg_gfx_write(0x05, 0x40);	/* mode: 256-color shift */
	tg_gfx_write(0x06, 0x05);	/* misc: graphics, A0000 64KB */
	tg_gfx_write(0x07, 0x0f);
	tg_gfx_write(0x08, 0xff);
	/* (GR0F / GR2F belong to the fetch experiment.) */

	/* Attribute controller: identity palette + graphics mode. */
	(void)tg_inb(tdisp.io_3da);		/* reset flip-flop */
	for (i = 0; i < 16; i++) {
		tg_outb(tdisp.io_3c0, (uint8_t)i);
		tg_outb(tdisp.io_3c0, (uint8_t)i);
	}
	tg_outb(tdisp.io_3c0, 0x10);
	tg_outb(tdisp.io_3c0, 0x41);	/* graphics, 8-bit color */
	tg_outb(tdisp.io_3c0, 0x11);
	tg_outb(tdisp.io_3c0, 0x00);	/* overscan */
	tg_outb(tdisp.io_3c0, 0x12);
	tg_outb(tdisp.io_3c0, 0x0f);	/* plane enable */
	tg_outb(tdisp.io_3c0, 0x13);
	tg_outb(tdisp.io_3c0, 0x00);	/* pixel panning */
	tg_outb(tdisp.io_3c0, 0x14);
	tg_outb(tdisp.io_3c0, 0x00);	/* color select */
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);	/* re-enable video output */

	/* Hidden DAC: the depth format. */
	tg_hidden_dac_write(tg_hdr_value());

	/* DAC: the RGB332 palette for 8bpp / a ramp otherwise. */
	tg_load_palette();
}

/*
 * Resolve the depth for a request.  req == -1: the highest depth
 * that fits VRAM.
 */
static int
tg_resolve_bpp(int req, int w, int h, uint32_t vram)
{
	int b;

	if (req == -1) {
		b = 24;
		while (b > 8 &&
		       (uint32_t)w * (uint32_t)h * (uint32_t)(b / 8) > vram)
			b = (b == 24) ? 16 : 8;
		if (w == 800 && b == 24)
			b = 16;		/* see the 40MHz note */
		hal_log_info("TRIDENT: auto depth -> %d bpp "
			     "(VRAM %luKB).",
			     b, (unsigned long)(vram >> 10));
		return b;
	}

	if ((uint32_t)w * (uint32_t)h * (uint32_t)(req / 8) > vram) {
		hal_log_info("TRIDENT: %dx%d at %d bpp does not fit "
			     "%luKB VRAM.",
			     w, h, req, (unsigned long)(vram >> 10));
		return -1;
	}
	return req;
}

/*****************************************************************************/
/* Video output relay (fixed: the field-proven sequence)                     */
/*****************************************************************************/

/*
 * The relay, fixed to the sequence that the T=0..9 experiment
 * matrix proved on the Ra43-class board (tests 2 and 6 produced a
 * picture; everything else was white or lost sync):
 *   minimal GDC teardown + the XF98 SDAC[04] sync path + FAC=02h.
 * FAC=03h (the NT value) kills sync on this board and must not be
 * used.
 */
static void
tg_relay_to_accel(void)
{
	hal_log_info("TRIDENT: relay: 0FACh reads %02Xh, "
		     "SDAC[04]=%02Xh; switching (SDAC path).",
		     inp(0x0fac), tg_sdac_read(0x04));

	outp(0x68, 0x0e);	/* GDC display element off */
	(void)inp(0x5f);
	(void)inp(0x5f);
	tg_gfx_write(0x21, tg_gfx_read(0x21) & ~0x20);

	/* The XF98 SDAC[04] sync path (the load-bearing part). */
	tg_crtc_write(0x23, tg_crtc_read(0x23) & ~0x20);
	tg_crtc_write(0x29, tg_crtc_read(0x29) | 0x04);
	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x06);
	tg_wait_ms(1);
	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x08);
	tg_gfx_write(0x23, tg_gfx_read(0x23) & ~0x03);
	tg_sdac_write(0x04, tg_sdac_read(0x04) | 0x01);
	tg_seq_write(0x01, tg_seq_read(0x01) & ~0x10);

	outp(0x0fac, 0x02);

	hal_log_info("TRIDENT: relay: 0FACh now reads %02Xh, "
		     "SDAC[04]=%02Xh.",
		     inp(0x0fac), tg_sdac_read(0x04));
}

static void
tg_relay_to_gdc(void)
{
	outp(0x0fac, 0x00);

	/* SDAC sync path off (mirror). */
	tg_seq_write(0x01, tg_seq_read(0x01) | 0x10);
	tg_sdac_write(0x04, tg_sdac_read(0x04) & ~0x0f);
	tg_gfx_write(0x23, 0x01 | (tg_gfx_read(0x23) & ~0x03));
	tg_crtc_write(0x29, tg_crtc_read(0x29) & ~0x04);
	tg_crtc_write(0x23, tg_crtc_read(0x23) | 0x20);

	tg_gfx_write(0x21, tg_gfx_read(0x21) | 0x20);
	outp(0x68, 0x0f);	/* GDC display element on */
}

/*****************************************************************************/
/* Fetch-path experiment (runs automatically; key press to advance)          */
/*****************************************************************************/

/*
 * Five registers differ between the XF98 recipe and the ITF state,
 * and neither endpoint works (see the header).  This walks the
 * combinations from the most likely fix downward.
 */

struct tg_fetch_combo {
	const char *name;
	uint8_t cr1e;		/* absolute value; 0 = keep the ITF value */
	uint8_t cr2a_or;	/* OR onto the ITF value */
	uint8_t cr2f_or;	/* OR onto the ITF value */
	int gr0f_ext;		/* 1: (itf & F0h) | 12h, 0: the ITF value */
	int gr2f_set;		/* 1: write 24h, 0: the ITF value */
};

static const struct tg_fetch_combo tg_fetch_combos[] = {
	/* 0: the prime suspect fix: XF98 minus CR2A bit6. */
	{ "XF98 minus CR2A.6",		0x80, 0x00, 0x10, 1, 1 },
	/* 1: minimum extension: ITF + CR1E only. */
	{ "ITF + CR1E=80h",		0x80, 0x00, 0x00, 0, 0 },
	/* 2: ITF + GR0F ext only. */
	{ "ITF + GR0F ext",		0x00, 0x00, 0x00, 1, 0 },
	/* 3: CR1E + GR0F, no CR2A.6, no GR2F. */
	{ "CR1E + GR0F ext",		0x80, 0x00, 0x00, 1, 0 },
	/* 4: 3 + GR2F=24h. */
	{ "CR1E + GR0F + GR2F",		0x80, 0x00, 0x00, 1, 1 },
	/* 5: full XF98 (the striped baseline, for comparison). */
	{ "full XF98 (stripes?)",	0x80, 0x40, 0x10, 1, 1 },
	/* 6: full ITF (the blue-only baseline, for comparison). */
	{ "full ITF (blue?)",		0x00, 0x00, 0x00, 0, 0 }
};
#define TG_NFETCH \
	(int)(sizeof(tg_fetch_combos) / sizeof(tg_fetch_combos[0]))

/* The ITF values, captured before the first combo is applied. */
static uint8_t tg_itf_cr1e, tg_itf_cr2a, tg_itf_cr2f;
static uint8_t tg_itf_gr0f, tg_itf_gr2f;

static void
tg_fetch_capture_itf(void)
{
	tg_itf_cr1e = (uint8_t)tg_crtc_read(0x1e);
	tg_itf_cr2a = (uint8_t)tg_crtc_read(0x2a);
	tg_itf_cr2f = (uint8_t)tg_crtc_read(0x2f);
	tg_itf_gr0f = (uint8_t)tg_gfx_read(0x0f);
	tg_itf_gr2f = (uint8_t)tg_gfx_read(0x2f);
	hal_log_info("TRIDENT-F: ITF fetch state: CR1E=%02Xh "
		     "CR2A=%02Xh CR2F=%02Xh GR0F=%02Xh GR2F=%02Xh.",
		     tg_itf_cr1e, tg_itf_cr2a, tg_itf_cr2f,
		     tg_itf_gr0f, tg_itf_gr2f);
}

static void
tg_fetch_apply(const struct tg_fetch_combo *c)
{
	tg_crtc_write(0x1e, c->cr1e ? c->cr1e : tg_itf_cr1e);
	tg_crtc_write(0x2a, tg_itf_cr2a | c->cr2a_or);
	tg_crtc_write(0x2f, tg_itf_cr2f | c->cr2f_or);
	if (c->gr0f_ext)
		tg_gfx_write(0x0f, (tg_itf_gr0f & 0xf0) | 0x12);
	else
		tg_gfx_write(0x0f, tg_itf_gr0f);
	if (c->gr2f_set)
		tg_gfx_write(0x2f, 0x24);
	else
		tg_gfx_write(0x2f, tg_itf_gr2f);

	hal_log_info("TRIDENT-F: applied: CR1E=%02Xh CR2A=%02Xh "
		     "CR2F=%02Xh GR0F=%02Xh GR2F=%02Xh.",
		     tg_crtc_read(0x1e), tg_crtc_read(0x2a),
		     tg_crtc_read(0x2f), tg_gfx_read(0x0f),
		     tg_gfx_read(0x2f));
}

/*
 * Draw eight color bars + a grayscale gradient strip at the bottom,
 * byte-lane writes only (the aperture is byte-only on this board).
 * Correct rendering: white, yellow, cyan, green, magenta, red,
 * blue, black, darker every 16 lines.
 */
#if TG_FETCH_EXPERIMENT
/* Generate one row of the test pattern in the mode's pixel format. */
static void
tg_fetch_row(int y, uint8_t *dst)
{
	static const uint8_t rgb[8][3] = {
		{255,255,255}, {255,255,0}, {0,255,255}, {0,255,0},
		{255,0,255},   {255,0,0},   {0,0,255},   {0,0,0}
	};
	int x, bar;

	for (x = 0; x < tdisp.scr_w; x++) {
		uint8_t r, g, b;

		if (y >= tdisp.scr_h - 64) {
			/* bottom strip: gradient */
			r = g = b = (uint8_t)
				((x * 255) / (tdisp.scr_w - 1));
		} else {
			bar = (x * 8) / tdisp.scr_w;
			if (bar > 7)
				bar = 7;
			r = rgb[bar][0];
			g = rgb[bar][1];
			b = rgb[bar][2];
			if (y & 0x10) {
				r >>= 1;
				g >>= 1;
				b >>= 1;
			}
		}

		if (tdisp.bpp == 24) {
			dst[x * 3 + 0] = b;
			dst[x * 3 + 1] = g;
			dst[x * 3 + 2] = r;
		} else if (tdisp.bpp == 16) {
			unsigned p = ((r & 0xf8) << 8) |
				     ((g & 0xfc) << 3) |
				     (b >> 3);
			dst[x * 2 + 0] = (uint8_t)p;
			dst[x * 2 + 1] = (uint8_t)(p >> 8);
		} else {
			dst[x] = (uint8_t)((r & 0xe0) |
				((g >> 3) & 0x1c) | (b >> 6));
		}
	}
}

/*
 * Draw the test pattern.  With the engine up this goes through the
 * CPU-source FIFO BLT and doubles as the end-to-end proof of that
 * path (this is the definitive test when the aperture cannot be
 * read back); otherwise it falls back to direct aperture stores.
 */
static void
tg_fetch_pattern(void)
{
	int y, rowlen;

	rowlen = tdisp.scr_w * (tdisp.bpp / 8);

#if TG_TRY_GE
	if (tdisp.use_ge && tg_ge_wait("fetch pattern")) {
		int i;

		tg_ge_out8(0x27, 0xcc);
		tg_ge_out32(0x28, 0x00000000UL);
		tg_ge_out16(0x38, 0);
		tg_ge_out16(0x40, tdisp.scr_w * tdisp.ge_xmul - 1);
		tg_ge_out16(0x42, 0);		/* one row per op */
		for (y = 0; y < tdisp.scr_h; y++) {
			tg_fetch_row(y, tg_rowbuf);
			tg_ge_out16(0x3a, y);
			tg_ge_out8(0x24, 0x01);
			if (!tg_ge_wait("fetch pattern data"))
				return;
			if (tdisp.aper_width == 1) {
				volatile uint8_t *out = tdisp.fb;

				for (i = 0; i < rowlen; i++)
					out[i] = tg_rowbuf[i];
			} else {
				volatile uint32_t *out =
					(volatile uint32_t *)tdisp.fb;
				const uint32_t *in =
					(const uint32_t *)tg_rowbuf;
				int n = rowlen / 4;

				for (i = 0; i < n; i++)
					out[i] = in[i];
			}
		}
		(void)tg_ge_wait("fetch pattern end");
		return;
	}
#endif /* TG_TRY_GE */

	/* Fallback: verified direct row stores through the aperture. */
	for (y = 0; y < tdisp.scr_h; y++) {
		tg_fetch_row(y, tg_rowbuf);
		(void)tg_store_verified(tdisp.fb +
					(uint32_t)y * tdisp.pitch,
					tg_rowbuf, rowlen);
	}
}

/*
 * The experiment driver: called once after the relay and the VRAM
 * clear.  Unblanks, steps through the combos (key press to
 * advance), leaves combo 0 in force at the end.
 */
static void
tg_fetch_experiment(void)
{
	int i;

	tg_fetch_capture_itf();
	tg_seq_write(0x01, tg_seq_read(0x01) & ~0x20);	/* unblank */

	for (i = 0; i < TG_NFETCH; i++) {
		hal_log_info("TRIDENT-F: === combo %d: %s ===",
			     i, tg_fetch_combos[i].name);
		tg_fetch_apply(&tg_fetch_combos[i]);
		tg_fetch_pattern();
		hal_log_info("TRIDENT-F: combo %d on screen -- press "
			     "a key for the next.", i);
		(void)getch();
	}

	/* Leave the most likely candidate (combo 0) in force. */
	hal_log_info("TRIDENT-F: sequence done; re-applying combo 0.");
	tg_fetch_apply(&tg_fetch_combos[0]);
	tg_fetch_pattern();
}
#endif /* TG_FETCH_EXPERIMENT */

/*
 * The production path: apply combo 0 (the fetch state under which
 * the Ra43 field test showed a pixel-perfect picture) and unblank.
 * No bars, no key presses.
 */
static void
tg_fetch_apply_default(void)
{
	tg_fetch_capture_itf();
	tg_fetch_apply(&tg_fetch_combos[0]);
	hal_log_info("TRIDENT: fetch path = combo 0 (%s), the "
		     "Ra43-proven state.", tg_fetch_combos[0].name);
	/* NOTE: does not unblank - the caller clears first. */
}

/*****************************************************************************/
/* State save/restore                                                        */
/*****************************************************************************/

/*
 * Save ranges follow NEC's own NT driver, which snapshots CRTC
 * 00h-50h and GR 00h-5Fh (we take 00h-6Fh) plus the clocks.  The
 * 3A4h shadow CRTC bank is saved too.
 */
static uint8_t sv_crtc[0x51];
static uint8_t sv_sr[0x10];		/* new mode; 0Bh skipped */
static uint8_t sv_sr0d_old, sv_sr0d_new, sv_sr0e_new;
static uint8_t sv_gr[0x70];
static uint8_t sv_crb[0x19];		/* 3A4h shadow CRTC */
static uint8_t sv_gr30;
static uint8_t sv_attr[0x15];
static uint8_t sv_misc, sv_hdr, sv_dac_mask;
static uint8_t sv_vclk_lo, sv_vclk_hi, sv_mclk_lo, sv_mclk_hi;
static uint8_t sv_sdac[5];
static const uint8_t tg_sdac_idx[5] = { 0x00, 0x04, 0x08, 0x09, 0x37 };
static uint8_t sv_sdac38;
static uint8_t sv_dac[256 * 3];

static void
tg_save_state(void)
{
	int i;

	sv_misc = (uint8_t)tg_misc_read();
	tg_select_crtc(sv_misc);

	/* Both flavors of the mode-dependent sequencer registers. */
	tg_sw_new();
	sv_sr0d_new = (uint8_t)tg_seq_read(0x0d);
	sv_sr0e_new = (uint8_t)tg_seq_read(0x0e);
	tg_sw_old();
	sv_sr0d_old = (uint8_t)tg_seq_read(0x0d);
	tg_sw_new();

	for (i = 0; i < 0x10; i++) {
		if (i == 0x0b || i == 0x0d || i == 0x0e) {
			sv_sr[i] = 0;
			continue;	/* handled above / mode switch */
		}
		sv_sr[i] = (uint8_t)tg_seq_read(i);
	}

	for (i = 0; i <= 0x50; i++)
		sv_crtc[i] = (uint8_t)tg_crtc_read(i);

	for (i = 0; i < 0x70; i++)
		sv_gr[i] = (uint8_t)tg_gfx_read(i);

	/* The 3A4h shadow CRTC bank, gated by GR30 bit6. */
	sv_gr30 = sv_gr[0x30];
	if ((sv_gr30 & 0x40) == 0)
		tg_gfx_write(0x30, sv_gr30 | 0x40);
	for (i = 0; i <= 0x18; i++) {
		tg_outb(0x03a4, i);
		sv_crb[i] = (uint8_t)tg_inb(0x03a5);
	}
	if ((sv_gr30 & 0x40) == 0)
		tg_gfx_write(0x30, sv_gr30);

	for (i = 0; i < 0x15; i++)
		sv_attr[i] = (uint8_t)tg_attr_read(i);

	sv_hdr = (uint8_t)tg_hidden_dac_read();
	sv_dac_mask = (uint8_t)tg_inb(tdisp.io_3c0 + 0x06);

	tg_outb(tdisp.io_3c0 + 0x07, 0x00);
	for (i = 0; i < 256 * 3; i++)
		sv_dac[i] = (uint8_t)tg_inb(tdisp.io_3c0 + 0x09);

	sv_vclk_lo = (uint8_t)tg_inb(tdisp.io_vclk);
	sv_vclk_hi = (uint8_t)tg_inb(tdisp.io_vclk + 1);
	sv_mclk_lo = (uint8_t)tg_inb(tdisp.io_vclk - 2);
	sv_mclk_hi = (uint8_t)tg_inb(tdisp.io_vclk - 1);

	for (i = 0; i < 5; i++)
		sv_sdac[i] = (uint8_t)tg_sdac_read(tg_sdac_idx[i]);
	sv_sdac38 = (uint8_t)tg_sdac_read(0x38);
}

static void
tg_restore_state(void)
{
	int i;

	tg_sw_new();

	/* Sequencer basics (skip the mode-dependent ones for now). */
	for (i = 0; i < 0x10; i++) {
		if (i == 0x0b || i == 0x0d || i == 0x0e)
			continue;
		if (i == 0x01)
			continue;	/* unblank last */
		tg_seq_write(i, sv_sr[i]);
	}
	tg_sw_old();
	tg_seq_write(0x0d, sv_sr0d_old);
	tg_sw_new();
	tg_seq_write(0x0d, sv_sr0d_new);

	/*
	 * GR file: restore GR30 last within the block so the shadow
	 * CRTC gate finishes in its saved position, and put the
	 * shadow bank back while the gate is forced open.
	 */
	for (i = 0; i < 0x70; i++) {
		if (i == 0x30)
			continue;
		tg_gfx_write(i, sv_gr[i]);
	}
	tg_gfx_write(0x30, sv_gr30 | 0x40);
	for (i = 0; i <= 0x18; i++) {
		if (i == 0x11)
			continue;	/* handled below */
		tg_outb(0x03a4, i);
		tg_outb(0x03a5, sv_crb[i]);
	}
	tg_outb(0x03a4, 0x11);
	tg_outb(0x03a5, sv_crb[0x11]);
	tg_gfx_write(0x30, sv_gr30);

	/* MISC first: bit0 re-selects the CRTC base for the writes. */
	tg_misc_write(sv_misc);

	/* CRTC 00h-50h: unlock CR0-7 first, CR11 written back last. */
	tg_crtc_write(0x11, sv_crtc[0x11] & 0x7f);
	for (i = 0; i <= 0x50; i++) {
		if (i == 0x11)
			continue;
		tg_crtc_write(i, sv_crtc[i]);
	}
	tg_crtc_write(0x11, sv_crtc[0x11]);

	for (i = 0; i < 0x15; i++)
		tg_attr_write(i, sv_attr[i]);
	(void)tg_inb(tdisp.io_3da);
	tg_outb(tdisp.io_3c0, 0x20);

	/* DAC + hidden DAC + pixel mask. */
	tg_outb(tdisp.io_3c0 + 0x08, 0x00);
	for (i = 0; i < 256 * 3; i++)
		tg_outb(tdisp.io_3c0 + 0x09, sv_dac[i]);
	tg_hidden_dac_write(sv_hdr);
	tg_outb(tdisp.io_3c0 + 0x06, sv_dac_mask);

	/* Clocks. */
	tg_outb(tdisp.io_vclk, sv_vclk_lo);
	tg_outb(tdisp.io_vclk + 1, sv_vclk_hi);
	tg_outb(tdisp.io_vclk - 2, sv_mclk_lo);
	tg_outb(tdisp.io_vclk - 1, sv_mclk_hi);

	/* The NEC glue. */
	tg_sdac_write(0x38, sv_sdac38);
	for (i = 4; i >= 0; i--)
		tg_sdac_write(tg_sdac_idx[i], sv_sdac[i]);

	/*
	 * SR0E last: writes in the new mode invert bit1, so pre-XOR
	 * to land the stored value exactly where it was.  Unblank via
	 * the saved SR01 as the final act.
	 */
	tg_seq_write(0x0e, sv_sr0e_new ^ 0x02);
	tg_seq_write(0x01, sv_sr[0x01]);
}

/*****************************************************************************/
/* Misc                                                                      */
/*****************************************************************************/

/*
 * DPMI 0x0800: Map a physical address into linear address space.
 * (DOS/4GW uses a zero-based flat address space, so the returned
 * linear address is directly usable as a pointer.)
 */
static void *
tg_map_physical(uint32_t phys, uint32_t size)
{
	union REGS r;

	if (phys < 0x100000UL)
		return (void *)phys;	/* first MB is identity-mapped */

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0800;
	r.w.bx = (uint16_t)(phys >> 16);
	r.w.cx = (uint16_t)(phys & 0xffff);
	r.w.si = (uint16_t)(size >> 16);
	r.w.di = (uint16_t)(size & 0xffff);
	int386(0x31, &r, &r);
	if (r.w.cflag)
		return NULL;

	return (void *)(((uint32_t)r.w.bx << 16) | r.w.cx);
}
