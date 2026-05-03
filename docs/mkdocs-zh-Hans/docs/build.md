Suika3 建置说明
========================

Suika3 完全采用 CMake 建置系统。

注意事项：
* 需要 CMake 3.22 以上
* Linux：GCC 4.4 以上（也支援 Clang）
* Windows：Visual Studio 2022/2026，或在 WSL 上使用 gcc/clang
* macOS：需要 Xcode 8.2.1 以上

---

## Linux（Wayland/X11 双模式）

### 必要条件

* 一台任何处理器的 `Linux` 电脑

在 Debian、Ubuntu 或 Raspberry Pi OS 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系统上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build alsa-lib-devel libX11-devel libXpm-devel wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### 步骤

开启终端机并输入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux
cmake --build --preset linux
```

会产生目标档 `build-linux/suika3`。

如果你想用 GDB 除错 Suika3，可以改用 `linux-debug` preset，而不是 `linux`。

---

## Linux（仅 X11）

### 必要条件

* 一台任何处理器的 `Linux` 电脑

在 Debian、Ubuntu 或 Raspberry Pi OS 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系统上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### 步骤

开启终端机并输入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

会产生目标档 `build-linux-x11/suika3`。

如果你想用 GDB 除错 Suika3，可以改用 `linux-x11-debug` preset，而不是 `linux-x11`。


---

## Linux（Wayland）

请注意，Wayland 支援仍属实验性质。
它和 KDE 相容，但在 GNOME 上显示视窗时有问题。

### 必要条件

* 一台任何处理器的 `Linux` 电脑

在 Debian 或 Ubuntu 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系统上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### 步骤

开启终端机并输入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

会产生目标档 `build-linux-wayland/suika3`。

如果你想用 GDB 除错 Suika3，可以改用 `linux-wayland-debug` preset，而不是 `linux-wayland`。

---

## Windows（Visual Studio 2026）

Visual Studio 是 Windows 的建议建置环境，也是官方二进位档所使用的方式。

### 必要条件

* 一台配备 Intel、AMD 或 Arm64 处理器的 `Windows 11` PC
* 已安装并设定好 C/C++ 与 CMake 的 `Visual Studio 2026`

### 步骤

* 复制此储存库。
* 在 Visual Studio 中开启原始码资料夹的根目录。
* 等待 CMake 组态完成。
* 选择 `VS2026 x64 Release` 目标。
* 建置专案。

会产生目标档 `out/build/windows-vs2026-x64-release/suika3.exe`。

---

## Windows（WSL2）

这种方式会在 WSL2 上使用 MinGW 建立 Windows exe 档。
请注意，虽然可以用 MinGW 建置 Suika3，但不建议这样做，因为防毒软体可能会出现误判。

### 必要条件

* 一台配备 Intel、AMD 或 Arm64 处理器的 `Windows 11` PC
* 已安装 `WSL2` 功能
* WSL2 上已安装 `Ubuntu` 或 `Debian`

```
sudo apt-get install cmake ninja-build mingw-w64
```

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

会产生目标档 `build-mingw-x86_64/suika3.exe`。

---

## macOS（App Bundle）

这种方式会建立 macOS 的 app bundle，也是官方二进位档所使用的方式。

### 必要条件

* 一台配备 Apple Silicon 或 Intel 处理器的 Mac
* 已安装 `macOS 11` 或更新版本
* 已安装 `Xcode`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

会产生目标档 `build-macos/Suika3.app`。

### DMG 打包

如果你想释出 macOS 版 Suika3，就需要建立一个具备程式码签章与公证的 DMG 档。
你需要在钥匙圈中拥有 `Developer ID Application` 凭证，并且已在 Xcode 中登入 Apple Developer 帐号。
请注意，透过 zip 档释出的 app bundle 无法存取 bundle 外部的内容，因此这里改用 DMG。

```
cd macos

# 签署 app。
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# 公证 app。（需要一些时间）
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# 建立 DMG 档。
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# 签署 DMG 档，以允许存取 bundle 外部的档案（避免 Gatekeeper 问题）。
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS（CLI）

这种方式会建立 macOS 版 Suika3 的命令列介面（CLI）版本。
它适合除错与开发，但不建议用于释出。

### 必要条件

* 一台配备 Apple Silicon 或 Intel 处理器的 Mac
* 已安装 `macOS 11` 或更新版本
* 已安装 `Xcode`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

会产生目标档 `build-macos/Suika3.app`。

如果你想用 LLDB 除错 Suika3，可以改用 `macos-cli-debug` preset，而不是 `macos`。

---

## iOS

### 必要条件

* 一台配备 Apple Silicon 或 Intel 处理器的 Mac
* 已安装 `macOS 11` 或更新版本
* 已安装 `Xcode`

### 步骤

* 将素材打包成 `assets.arc` 档。
* 下载 [官方二进位档](https://github.com/suika3-community/suika/releases) 并解压缩。
* 将你的 `assets.arc` 档复制到 `SDK/ios/resources` 资料夹。
* 在 Xcode 中开启 `SDK/ios` 资料夹。
* 建置并执行。

### 从头建置

如果你想从头建置，请使用 `cmake --preset ios-device` 或 `cmake --preset ios-simulator`，
接著把建置好的 `libsuika3.a` 档复制到 `SDK/ios/lib` 资料夹，然后在 Xcode 中开启 `SDK/ios` 资料夹。

---

## Android

### 必要条件

* `Android Studio`

### 步骤

* 下载 [官方二进位档](https://github.com/suika3-community/suika/releases) 并解压缩。
* 将你的素材档复制到 `SDK/android/app/src/main/assets` 资料夹。
* 在 Android Studio 中开启 `SDK/android` 资料夹。

### 从头建置

如果你想从头建置，请使用 `cmake --preset android-arm64`，然后把建置好的 `libsuika3.so` 档复制到 `SDK/android/app/src/main/jniLibs/arm64-v8a` 资料夹，最后在 Android Studio 中开启 `SDK/android` 资料夹。

---

## HarmonyOS NEXT / OpenHarmony

### 必要条件

* 已安装 `OpenHarmony SDK`

### 步骤

* 下载 [官方二进位档](https://github.com/awemorris/suika/releases) 并解压缩。
* 将你的素材档复制到 `SDK/openharmony/entry/src/main/resources/rawfile` 资料夹。
* 在 DevEco 中开启 `SDK/openharmony` 资料夹。
* 建置并执行。

### 从头建置

如果你想从头建置，请使用 `cmake --preset openharmony-arm64`，
接著把建置好的 `libsuika3.a` 档复制到 `SDK/openharmony/entry/libs/arm64-v8a` 资料夹，
然后在 DevEco 中开启 `SDK/openharmony` 资料夹。

---

## WebAssembly

### 必要条件

* 已安装 `emsdk`。（任何作业系统都可以。）

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

会产生目标档 `wasm/index.html`。

### 测试

要执行应用程式，请把你的 `assets.arc` 档放到 `wasm` 资料夹中。

接著输入 `python -m http.server`，并在浏览器视窗中开启 `http://localhost:8000`。

在 Windows 上，你可以改用 `suika3-web.exe` 取代 `python`。它会启动一个小型网页伺服器，并自动开启浏览器。

---

## FreeBSD

### 必要条件

* 一台 `FreeBSD` 电脑
* 已安装 `cmake` 与 `ninja`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

会产生目标档 `build-freebsd/suika3`。

---

## NetBSD

### 必要条件

* 一台 `NetBSD` 电脑
* 已安装 `cmake` 与 `ninja`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

会产生目标档 `build-netbsd/suika3`。

---

## OpenBSD

### 必要条件

* 一台 `OpenBSD` 电脑
* 已安装 `cmake` 与 `ninja`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

会产生目标档 `build-openbsd/suika3`。

---

## Solaris 11

### 必要条件

* 一台 `Solaris 11` 电脑
* 已安装 `SunCC`、`cmake` 与 `gmake`
* `cmake` 必须以原始码编译

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris11
cmake --build --preset solaris11
```

会产生目标档 `build-solaris11/suika3`。

---

## Solaris 10

### 必要条件

* 一台 `Solaris 10` 电脑
* 已安装 `SunCC`、`cmake` 与 `gmake`
* `cmake` 必须以原始码编译

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris10
cmake --build --preset solaris10
```

会产生目标档 `build-solaris10/suika3`。

---

## Haiku

### 必要条件

* 一台 `Haiku` 电脑
* 已安装 `gcc`、`cmake` 与 `ninja`

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset haiku
cmake --build --preset haiku
```

会产生目标档 `build-solaris10/suika3`。

---

## Unity Plugin

### 必要条件

* 已安装完整建置的 `LLVM-22`。

### 步骤

开启终端机并输入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

会产生目标档 `build-unity-win64/libsuika3.dll`。

注意：请把 `win64` 改成 `switch`、`ps5` 或 `xbox` 其中之一。

若要使用厂商官方 SDK，请开启 `cmake/toolchains/unity-*.cmake`，并修改 `CMAKE_C_COMPILER`、`CMAKE_CXX_COMPILER` 与 `CMAKE_AR`。
