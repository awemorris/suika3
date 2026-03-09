Playfield Engine Design Overview
================================

## Porting Layer

Did you know that `Playfield Engine` runs seamlessly on Windows,
macOS, Linux, iPhone, iPad, Android, Web browsers, — and even game
consoles?

The `extern/StratoHAL` directory contains its platform abstraction layer,
which cleanly handles all platform-specific differences.

### Common Components

|File           |Description                         |
|---------------|------------------------------------|
|image.c        |Image manipulation                  |
|stdfile.c      |File access via C stdio library     |
|glyph.c        |Font drawing via FreeType library   |
|wave.c         |OggVorbis decoder via libvorbis     |

### Windows Layer

|File           |Description                         |
|---------------|------------------------------------|
|winmain.c      |wWinMain()                          |
|d3drender.c    |Rendering dispatch                  |
|d3d12render.cc |Direct3D 12 rendering               |
|d3d11render.cc |Direct3D 11 rendering               |
|d3d9render.cc  |Direct3D 9 rendering                |
|gdirender.c    |GDI rendering                       |
|dsound.c       |DirectSound audio                   |
|dsvideo.cc     |DirectShow video                    |

### macOS Layer

|File           |Description                         |
|---------------|------------------------------------|
|nsmain.m       |main() using AppKit                 |
|GameRenderer.m |Metal rendering                     |
|aunit.c        |Audio Unit audio                    |

### Linux and BSD Layer

|File           |Description                         |
|---------------|------------------------------------|
|x11main.c      |X11 main                            |
|glrender.c     |OpenGL rendering                    |
|asound.c       |ALSA audio                          |
|bsdsound.c     |/dev/dsp and /dev/audio audio       |

### iOS Layer

|File           |Description                         |
|---------------|------------------------------------|
|uimain.m       |main() using UIKit                  |
|GameRenderer.m |Metal rendering                     |
|aunit.c        |Audio Unit audio                    |

### Android Layer

|File              |Description                           |
|------------------|--------------------------------------|
|MainActivity.java |Android activity                      |
|ndkmain.c         |C code using Android NDK              |
|ndkfile.c         |Asset accedd C code using Android NDK |

### SIMD Acceleration

Only enabled on x86 and x86_64 Windows target.

|File           |Description                         |
|---------------|------------------------------------|
|drawimage.h    |Definitions of drawing functions    |
|avx2.c         |AVX2                                |
|avx.c          |AVX                                 |
|sse42.c        |SSE4.2                              |
|sse4.c         |SSE4.1                              |
|sse3.c         |SSE3                                |
|sse2.c         |SSE2                                |
|sse.c          |SSE                                 |

## Main Layer

The main layer consists of `src/*.c` files, which defines API for
`Playfield Engine`. There are C APIs defined in `src/api.c`, and there
are Noct wrappers defined in `src/vm.c`.

### Tag File

Tagged file support is provided by the engine and is optionally available.
Developers may define an engine to run tag file scripts.
