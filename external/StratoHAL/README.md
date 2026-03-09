StratoHAL
=========

`libstrato` is a portable hardware abstraction layer for the
[NoctVM](https://github.com/awemorris/NoctVM) runtime.

`libstrato` provides a game loop with a unified interface for audio,
graphics, and input across multiple platforms.

---

## Features

- Cross-platform abstraction for:
    - Audio output
    - 2D/3D graphics
    - Gamepad and keyboard input

- Supports:
    - Windows, macOS, Linux, iOS, Android, WebAssembly
    - Also available as a native Unity Plugin for game consoles (PS, Switch, etc)

- Backends:
    - DirectX 9/11/12, Metal, OpenGL, Unity, and more

- Easy to extend with custom backends

---

## Design Philosophy

`libstrato` simplifies the chaotic hardware jungle into a simple
horizon.  HALs should be elegant, lightweight, and invisible.

---

## About the Name

"Strato" comes from a Latin word "stratum", i.e., a layer.

`NoctVM` rises into clarity, supported by `libstrato`.

---

## Author's Note

`libstrato` is one of the core building blocks of `NoctVM`, alongside
`NoctLang`, though the two remain independent. It is designed to
provide a unified foundation across gaming platforms, with a clear and
consistent game loop at its heart.
