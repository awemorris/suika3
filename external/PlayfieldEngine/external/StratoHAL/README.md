StratoHAL
=========

`StratoHAL (libstrato)` is a highly portable hardware abstraction
layer (HAL) for game applications.

`StratoHAL` provides:

- Built-in platform initialization, game loop, and event callback
  mechanisms.
- API for 2D graphics, audio, and input.
- GPU acceleration as well as software rendering.

With 25+ years of its history as in-house software, it has been
published as free/libre software to serve as a core building block of
the Suika3 game scripting runtime. Today, it continues to evolve as an
independent library, bridging classic and modern platforms.

## Platforms

`StratoHAL` supports most major platforms that SDL3 covers. In
addition, it also supports software rendering for small embedded
devices and older computers before the GPU era.

- Windows
- macOS
- Linux
- *BSD (FreeBSD, NetBSD, and OpenBSD)
- iOS
- Android
- HarmonyOS NEXT (OpenHarmony)
- WebAssembly
- ChromeOS
- Solaris 10 / 11
- Haiku OS
- Unity (as a native plugin)
- Xbox (via Unity Plugin, DevKit required)
- PlayStation 4 / PlayStation 5 (via Unity Plugin, DevKit required)
- Switch / Switch 2 (via Unity Plugin, DevKit required)

## Backends

| Platform                    | Graphics                                                                | Audio                    | Input                                                                    |
|-----------------------------|-------------------------------------------------------------------------|--------------------------|--------------------------------------------------------------------------|
| Windows                     | DirectX 12/11/9 and GDI (fallback)                                      | DirectSound 5            | DirectInput 8 (for PS5 controllers) and GameInput (for Xbox controllers) |
| Microsoft GDK (PC and Xbox) | DirectX 12                                                              | XAudio2                  | XInput                                                                   |
| macOS                       | Metal                                                                   | AudioUnit                | GameController                                                           |
| Linux                       | (Wayland, X11, KMS, fbdev) x (OpenGL ES2, OpenGL 3, software rendering) | ALSA                     | Wayland, X11, and evdev                                                  |
| FreeBSD                     | (Wayland, X11) x (OpenGL ES2, OpenGL 3, software rendering)             | /dev/dsp                 | Wayland or X11                                                           |
| NetBSD                      | X11 x (OpenGL 3, software rendering)                                    | /dev/audio               | X11                                                                      |
| OpenBSD                     | X11 x (OpenGL 3, software rendering)                                    | /dev/audio               | X11                                                                      |
| Solaris 10                  | X11 x software rendering                                                | /dev/audio               | X11                                                                      |
| Solaris 11                  | X11 x software rendering                                                | /dev/dsp (OSS)           | X11                                                                      |
| Haiku OS                    | Haiku native + software rendering                                       | Haiku native             | Haiku native                                                             |
| MS-DOS PC-98                | GDC 640x400 16-colors                                                   | None                     | Keyboard                                                                 |
| MS-DOS PC/AT                | VGA 640x480 16-colors                                                   | None                     | Keyboard                                                                 |

## About the Name

"Strato" comes from a Latin word "stratum", i.e., a layer, for hardware abstraction.

## Usage

Graphic and audio APIs are self-described in
[include/strato/strato.h](include/strato/strato.h).

Examples are found in [Playfield Engine](https://github.com/awemorris/PlayfieldEngine).

```
#include <strato/strato.h>

static char window_title[] = "Hello";
static bool need_exit;

static bool on_start(void)
{
        return true;
}

static bool on_update(void)
{
        if (need_exit)
                return false;

	return true;
}

static void on_render(void)
{
}

static void on_mouse_press(int button, int x, int y)
{
        if (button == HAL_MOUSE_LEFT)
                need_exit = true;
}

bool hal_bootstrap(char **title, int *width, int *height, struct hal_callback *cb)
{

	*title = window_title;
	*width = 640;
	*height = 480;

	cb->on_start = on_start;
	cb->on_update = on_update;
	cb->on_render = on_render;
	cb->on_mouse_press = on_mouse_press;

	return true;
}

HAL_DEFINE_MAIN()
```
