Инструкции по сборке Suika3
===========================

Suika3 полностью использует систему сборки CMake.

Примечания:
* Требуется CMake 3.22 или новее
* Linux: GCC 4.4 или новее (Clang также поддерживается)
* Windows: Visual Studio 2022/2026 или gcc/clang в WSL
* macOS: требуется Xcode 8.2.1 или новее

---

## Linux (Wayland/X11 Dual)

### Предварительные требования

* Машина с `Linux` на любом процессоре

В Debian, Ubuntu или Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

В RedHat, Rocky Linux, Fedora и т. п.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build alsa-lib-devel libX11-devel libXpm-devel wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Шаги

Откройте терминал и введите следующее.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux
cmake --build --preset linux
```

Будет создан целевой файл `build-linux/suika3`.

Если вы хотите отлаживать Suika3 с помощью GDB, можно использовать пресет `linux-debug` вместо `linux`.

---

## Linux (X11-Only)

### Предварительные требования

* Машина с `Linux` на любом процессоре

В Debian, Ubuntu или Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

В RedHat, Rocky Linux, Fedora и т. п.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### Шаги

Откройте терминал и введите следующее.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

Будет создан целевой файл `build-linux-x11/suika3`.

Если вы хотите отлаживать Suika3 с помощью GDB, можно использовать пресет `linux-x11-debug` вместо `linux-x11`.


---

## Linux (Wayland)

Обратите внимание, что поддержка Wayland все еще экспериментальная.
Она совместима с KDE, но имеет проблемы с отображением окон в GNOME.

### Предварительные требования

* Машина с `Linux` на любом процессоре

В Debian или Ubuntu:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

В RedHat, Rocky Linux, Fedora и т. п.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Шаги

Откройте терминал и введите следующее.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

Будет создан целевой файл `build-linux-wayland/suika3`.

Если вы хотите отлаживать Suika3 с помощью GDB, можно использовать пресет `linux-wayland-debug` вместо `linux-wayland`.

---

## Windows (Visual Studio 2026)

Visual Studio является рекомендуемой средой сборки для Windows и используется для официального бинарного файла.

### Предварительные требования

* ПК с `Windows 11` на процессоре Intel, AMD или Arm64
* Установленная `Visual Studio 2026` с настроенными C/C++ и CMake

### Шаги

* Клонируйте репозиторий.
* Откройте корень папки с исходным кодом в Visual Studio.
* Дождитесь завершения конфигурации CMake.
* Выберите цель `VS2026 x64 Release`.
* Соберите проект.

Будет создан целевой файл `out/build/windows-vs2026-x64-release/suika3.exe`.

---

## Windows (WSL2)

Этот способ использует MinGW в WSL2 для создания Windows exe-файла.
Обратите внимание, что Suika3 можно собрать с помощью MinGW,
но это не рекомендуется из-за ложных срабатываний антивирусного ПО.

### Предварительные требования

* ПК с `Windows 11` на процессоре Intel, AMD или Arm64
* Установленная возможность `WSL2`
* `Ubuntu` или `Debian`, установленная в WSL2

```
sudo apt-get install cmake ninja-build mingw-w64
```

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

Будет создан целевой файл `build-mingw-x86_64/suika3.exe`.

---

## macOS (App Bundle)

Этот способ создает пакет приложения для macOS и используется для официального бинарного файла.

### Предварительные требования

* Mac с процессором Apple Silicon или Intel
* Установленная `macOS 11` или новее
* Установленный `Xcode`

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

Будет создана цель `build-macos/Suika3.app`.

### Упаковка DMG

Если вы хотите распространять Suika3 для macOS, нужно создать DMG-файл с подписыванием кода и нотариальным заверением.
В связке ключей должен быть сертификат `Developer ID Application`, а в Xcode нужно войти в учетную запись Apple Developer.
Обратите внимание, что пакет приложения, распространяемый через zip-файл, не может получать доступ за пределы пакета приложения, поэтому здесь используется DMG.

```
cd macos

# Подписать приложение.
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# Нотариально заверить приложение. (занимает некоторое время)
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# Создать DMG-файл.
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# Подписать DMG-файл, чтобы разрешить доступ к файлам вне пакета (и избежать проблем Gatekeeper).
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS (CLI)

Этот способ создает версию Suika3 для интерфейса командной строки (CLI) на macOS.
Она полезна для отладки и разработки, но не рекомендуется для распространения.

### Предварительные требования

* Mac с процессором Apple Silicon или Intel
* Установленная `macOS 11` или новее
* Установленный `Xcode`

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

Будет создана цель `build-macos/Suika3.app`.

Если вы хотите отлаживать Suika3 с помощью LLDB, можно использовать пресет `macos-cli-debug` вместо `macos`.

---

## iOS

### Предварительные требования

* Mac с процессором Apple Silicon или Intel
* Установленная `macOS 11` или новее
* Установленный `Xcode`

### Шаги

* Упакуйте ресурсы в файл `assets.arc`.
* Скачайте [официальный бинарный файл](https://github.com/suika3-community/suika/releases) и распакуйте его.
* Скопируйте файл `assets.arc` в папку `SDK/ios/resources`.
* Откройте папку `SDK/ios` в Xcode.
* Соберите и запустите.

### Сборка с нуля

Если вы хотите собрать с нуля, используйте `cmake --preset ios-device` или `cmake --preset ios-simulator`,
затем скопируйте собранный файл `libsuika3.a` в папку `SDK/ios/lib` и откройте папку `SDK/ios` в Xcode.

---

## Android

### Предварительные требования

* `Android Studio`

### Шаги

* Скачайте [официальный бинарный файл](https://github.com/suika3-community/suika/releases) и распакуйте его.
* Скопируйте файлы ресурсов в папку `SDK/android/app/src/main/assets`.
* Откройте папку `SDK/android` в Android Studio.

### Сборка с нуля

Если вы хотите собрать с нуля, используйте `cmake --preset android-arm64`, затем скопируйте собранный файл `libsuika3.so` в папку `SDK/android/app/src/main/jniLibs/arm64-v8a` и откройте папку `SDK/android` в Android Studio.

---

## HarmonyOS NEXT / OpenHarmony

### Предварительные требования

* Установленный `OpenHarmony SDK`

### Шаги

* Скачайте [официальный бинарный файл](https://github.com/awemorris/suika/releases) и распакуйте его.
* Скопируйте файлы ресурсов в папку `SDK/openharmony/entry/src/main/resources/rawfile`.
* Откройте папку `SDK/openharmony` в DevEco.
* Соберите и запустите.

### Сборка с нуля

Если вы хотите собрать с нуля, используйте `cmake --preset openharmony-arm64`,
затем скопируйте собранный файл `libsuika3.a` в папку `SDK/openharmony/entry/libs/arm64-v8a`,
и откройте папку `SDK/openharmony` в DevEco.

---

## WebAssembly

### Предварительные требования

* Установленный `emsdk`. (Подойдет любая ОС.)

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

Будет создан целевой файл `wasm/index.html`.

### Тестирование

Чтобы запустить приложение, поместите файл `assets.arc` в папку `wasm`.

После этого введите `python -m http.server` и откройте `http://localhost:8000` в окне браузера.

В Windows можно использовать `suika3-web.exe` вместо `python`. Он запускает небольшой веб-сервер и автоматически открывает браузер.

---

## FreeBSD

### Предварительные требования

* Машина с `FreeBSD`
* Установленные `cmake` и `ninja`.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

Будет создан целевой файл `build-freebsd/suika3`.

---

## NetBSD

### Предварительные требования

* Машина с `NetBSD`
* Установленные `cmake` и `ninja`.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

Будет создан целевой файл `build-netbsd/suika3`.

---

## OpenBSD

### Предварительные требования

* Машина с `OpenBSD`
* Установленные `cmake` и `ninja`.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

Будет создан целевой файл `build-openbsd/suika3`.

---

## Solaris 11

### Предварительные требования

* Машина с `Solaris 11`
* Установленные `SunCC`, `cmake` и `gmake`.
* `cmake` должен быть собран из исходного кода.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris11
cmake --build --preset solaris11
```

Будет создан целевой файл `build-solaris11/suika3`.

---

## Solaris 10

### Предварительные требования

* Машина с `Solaris 10`
* Установленные `SunCC`, `cmake` и `gmake`.
* `cmake` должен быть собран из исходного кода.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris10
cmake --build --preset solaris10
```

Будет создан целевой файл `build-solaris10/suika3`.

---

## Haiku

### Предварительные требования

* Машина с `Haiku`
* Установленные `gcc`, `cmake` и `ninja`.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset haiku
cmake --build --preset haiku
```

Будет создан целевой файл `build-solaris10/suika3`.

---

## Unity Plugin

### Предварительные требования

* Установленная полная сборка `LLVM-22`.

### Шаги

Откройте терминал и введите следующее.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

Будет создан целевой файл `build-unity-win64/libsuika3.dll`.

Примечание: замените `win64` на одно из значений `switch`, `ps5` или `xbox`.

Чтобы использовать официальный SDK поставщика, откройте `cmake/toolchains/unity-*.cmake` и замените `CMAKE_C_COMPILER`, `CMAKE_CXX_COMPILER` и `CMAKE_AR`.
