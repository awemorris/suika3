Suika3 建置說明
========================

Suika3 完全採用 CMake 建置系統。

注意事項：
* 需要 CMake 3.22 以上
* Linux：GCC 4.4 以上（也支援 Clang）
* Windows：Visual Studio 2022/2026，或在 WSL 上使用 gcc/clang
* macOS：需要 Xcode 8.2.1 以上

---

## Linux（Wayland/X11 雙模式）

### 必要條件

* 一臺任何處理器的 `Linux` 電腦

在 Debian、Ubuntu 或 Raspberry Pi OS 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系統上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build alsa-lib-devel libX11-devel libXpm-devel wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### 步驟

開啟終端機並輸入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux
cmake --build --preset linux
```

會產生目標檔 `build-linux/suika3`。

如果你想用 GDB 除錯 Suika3，可以改用 `linux-debug` preset，而不是 `linux`。

---

## Linux（僅 X11）

### 必要條件

* 一臺任何處理器的 `Linux` 電腦

在 Debian、Ubuntu 或 Raspberry Pi OS 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系統上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### 步驟

開啟終端機並輸入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

會產生目標檔 `build-linux-x11/suika3`。

如果你想用 GDB 除錯 Suika3，可以改用 `linux-x11-debug` preset，而不是 `linux-x11`。


---

## Linux（Wayland）

請注意，Wayland 支援仍屬實驗性質。
它和 KDE 相容，但在 GNOME 上顯示視窗時有問題。

### 必要條件

* 一臺任何處理器的 `Linux` 電腦

在 Debian 或 Ubuntu 上：
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

在 RedHat、Rocky Linux、Fedora 等系統上：
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### 步驟

開啟終端機並輸入以下指令。
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

會產生目標檔 `build-linux-wayland/suika3`。

如果你想用 GDB 除錯 Suika3，可以改用 `linux-wayland-debug` preset，而不是 `linux-wayland`。

---

## Windows（Visual Studio 2026）

Visual Studio 是 Windows 的建議建置環境，也是官方二進位檔所使用的方式。

### 必要條件

* 一臺配備 Intel、AMD 或 Arm64 處理器的 `Windows 11` PC
* 已安裝並設定好 C/C++ 與 CMake 的 `Visual Studio 2026`

### 步驟

* 複製此儲存庫。
* 在 Visual Studio 中開啟原始碼資料夾的根目錄。
* 等待 CMake 組態完成。
* 選擇 `VS2026 x64 Release` 目標。
* 建置專案。

會產生目標檔 `out/build/windows-vs2026-x64-release/suika3.exe`。

---

## Windows（WSL2）

這種方式會在 WSL2 上使用 MinGW 建立 Windows exe 檔。
請注意，雖然可以用 MinGW 建置 Suika3，但不建議這樣做，因為防毒軟體可能會出現誤判。

### 必要條件

* 一臺配備 Intel、AMD 或 Arm64 處理器的 `Windows 11` PC
* 已安裝 `WSL2` 功能
* WSL2 上已安裝 `Ubuntu` 或 `Debian`

```
sudo apt-get install cmake ninja-build mingw-w64
```

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

會產生目標檔 `build-mingw-x86_64/suika3.exe`。

---

## macOS（App Bundle）

這種方式會建立 macOS 的 app bundle，也是官方二進位檔所使用的方式。

### 必要條件

* 一臺配備 Apple Silicon 或 Intel 處理器的 Mac
* 已安裝 `macOS 11` 或更新版本
* 已安裝 `Xcode`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

會產生目標檔 `build-macos/Suika3.app`。

### DMG 打包

如果你想釋出 macOS 版 Suika3，就需要建立一個具備程式碼簽章與公證的 DMG 檔。
你需要在鑰匙圈中擁有 `Developer ID Application` 憑證，並且已在 Xcode 中登入 Apple Developer 帳號。
請注意，透過 zip 檔釋出的 app bundle 無法存取 bundle 外部的內容，因此這裡改用 DMG。

```
cd macos

# 簽署 app。
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# 公證 app。（需要一些時間）
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# 建立 DMG 檔。
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# 簽署 DMG 檔，以允許存取 bundle 外部的檔案（避免 Gatekeeper 問題）。
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS（CLI）

這種方式會建立 macOS 版 Suika3 的命令列介面（CLI）版本。
它適合除錯與開發，但不建議用於釋出。

### 必要條件

* 一臺配備 Apple Silicon 或 Intel 處理器的 Mac
* 已安裝 `macOS 11` 或更新版本
* 已安裝 `Xcode`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

會產生目標檔 `build-macos/Suika3.app`。

如果你想用 LLDB 除錯 Suika3，可以改用 `macos-cli-debug` preset，而不是 `macos`。

---

## iOS

### 必要條件

* 一臺配備 Apple Silicon 或 Intel 處理器的 Mac
* 已安裝 `macOS 11` 或更新版本
* 已安裝 `Xcode`

### 步驟

* 將素材打包成 `assets.arc` 檔。
* 下載 [官方二進位檔](https://github.com/suika3-community/suika/releases) 並解壓縮。
* 將你的 `assets.arc` 檔複製到 `SDK/ios/resources` 資料夾。
* 在 Xcode 中開啟 `SDK/ios` 資料夾。
* 建置並執行。

### 從頭建置

如果你想從頭建置，請使用 `cmake --preset ios-device` 或 `cmake --preset ios-simulator`，
接著把建置好的 `libsuika3.a` 檔複製到 `SDK/ios/lib` 資料夾，然後在 Xcode 中開啟 `SDK/ios` 資料夾。

---

## Android

### 必要條件

* `Android Studio`

### 步驟

* 下載 [官方二進位檔](https://github.com/suika3-community/suika/releases) 並解壓縮。
* 將你的素材檔複製到 `SDK/android/app/src/main/assets` 資料夾。
* 在 Android Studio 中開啟 `SDK/android` 資料夾。

### 從頭建置

如果你想從頭建置，請使用 `cmake --preset android-arm64`，然後把建置好的 `libsuika3.so` 檔複製到 `SDK/android/app/src/main/jniLibs/arm64-v8a` 資料夾，最後在 Android Studio 中開啟 `SDK/android` 資料夾。

---

## HarmonyOS NEXT / OpenHarmony

### 必要條件

* 已安裝 `OpenHarmony SDK`

### 步驟

* 下載 [官方二進位檔](https://github.com/awemorris/suika/releases) 並解壓縮。
* 將你的素材檔複製到 `SDK/openharmony/entry/src/main/resources/rawfile` 資料夾。
* 在 DevEco 中開啟 `SDK/openharmony` 資料夾。
* 建置並執行。

### 從頭建置

如果你想從頭建置，請使用 `cmake --preset openharmony-arm64`，
接著把建置好的 `libsuika3.a` 檔複製到 `SDK/openharmony/entry/libs/arm64-v8a` 資料夾，
然後在 DevEco 中開啟 `SDK/openharmony` 資料夾。

---

## WebAssembly

### 必要條件

* 已安裝 `emsdk`。（任何作業系統都可以。）

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

會產生目標檔 `wasm/index.html`。

### 測試

要執行應用程式，請把你的 `assets.arc` 檔放到 `wasm` 資料夾中。

接著輸入 `python -m http.server`，並在瀏覽器視窗中開啟 `http://localhost:8000`。

在 Windows 上，你可以改用 `suika3-web.exe` 取代 `python`。它會啟動一個小型網頁伺服器，並自動開啟瀏覽器。

---

## FreeBSD

### 必要條件

* 一臺 `FreeBSD` 電腦
* 已安裝 `cmake` 與 `ninja`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

會產生目標檔 `build-freebsd/suika3`。

---

## NetBSD

### 必要條件

* 一臺 `NetBSD` 電腦
* 已安裝 `cmake` 與 `ninja`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

會產生目標檔 `build-netbsd/suika3`。

---

## OpenBSD

### 必要條件

* 一臺 `OpenBSD` 電腦
* 已安裝 `cmake` 與 `ninja`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

會產生目標檔 `build-openbsd/suika3`。

---

## Solaris 11

### 必要條件

* 一臺 `Solaris 11` 電腦
* 已安裝 `SunCC`、`cmake` 與 `gmake`
* `cmake` 必須以原始碼編譯

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris11
cmake --build --preset solaris11
```

會產生目標檔 `build-solaris11/suika3`。

---

## Solaris 10

### 必要條件

* 一臺 `Solaris 10` 電腦
* 已安裝 `SunCC`、`cmake` 與 `gmake`
* `cmake` 必須以原始碼編譯

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris10
cmake --build --preset solaris10
```

會產生目標檔 `build-solaris10/suika3`。

---

## Haiku

### 必要條件

* 一臺 `Haiku` 電腦
* 已安裝 `gcc`、`cmake` 與 `ninja`

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset haiku
cmake --build --preset haiku
```

會產生目標檔 `build-solaris10/suika3`。

---

## Unity Plugin

### 必要條件

* 已安裝完整建置的 `LLVM-22`。

### 步驟

開啟終端機並輸入以下指令。

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

會產生目標檔 `build-unity-win64/libsuika3.dll`。

注意：請把 `win64` 改成 `switch`、`ps5` 或 `xbox` 其中之一。

若要使用廠商官方 SDK，請開啟 `cmake/toolchains/unity-*.cmake`，並修改 `CMAKE_C_COMPILER`、`CMAKE_CXX_COMPILER` 與 `CMAKE_AR`。
