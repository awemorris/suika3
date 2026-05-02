Instrucciones de construcción de Suika3
========================

Suika3 utiliza completamente el sistema de compilación CMake.

Notas:
* Requiere CMake 3.22 o posterior
* Linux: GCC 4.4 o posterior (también se admite Clang)
* Windows: Visual Studio 2022/2026 o gcc/clang en WSL
* macOS: se requiere Xcode 8.2.1 o posterior

---

## Linux (Wayland/X11 doble)

### Requisitos previos

* Una máquina `Linux` con cualquier procesador

En el sistema operativo Debian, Ubuntu o Raspberry Pi:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

En RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build alsa-lib-devel libX11-devel libXpm-devel wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Pasos

Abra la terminal y escriba lo siguiente.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux
cmake --build --preset linux
```

Se creará el archivo de destino `build-linux/suika3`.

Si desea depurar Suika3 con GDB, puede usar el valor preestablecido `linux-debug` en lugar de `linux`.

---

## Linux (solo X11)

### Requisitos previos

* Una máquina `Linux` con cualquier procesador

En el sistema operativo Debian, Ubuntu o Raspberry Pi:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

En RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### Pasos

Abra la terminal y escriba lo siguiente.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

Se creará el archivo de destino `build-linux-x11/suika3`.

Si desea depurar Suika3 con GDB, puede usar el valor preestablecido `linux-x11-debug` en lugar de `linux-x11`.


---

## Linux (Wayland)

Tenga en cuenta que la compatibilidad con Wayland aún es experimental.
Es compatible con KDE, pero tiene problemas para mostrar ventanas en GNOME.

### Requisitos previos

* Una máquina `Linux` con cualquier procesador

En Debian o Ubuntu:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

En RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Pasos

Abra la terminal y escriba lo siguiente.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

Se creará el archivo de destino `build-linux-wayland/suika3`.

Si desea depurar Suika3 con GDB, puede usar el valor preestablecido `linux-wayland-debug` en lugar de `linux-wayland`.

---

## Windows (Visual Studio 2026)

Visual Studio es el entorno de compilación recomendado para Windows y se utiliza para el binario oficial.

### Requisitos previos

* Una PC `Windows 11` con procesador Intel, AMD o Arm64
* `Visual Studio 2026` instalado con C/C++ y CMake configurado

### Pasos

* Clonar el repositorio.
* Abra la raíz de la carpeta del código fuente en Visual Studio.
* Espere a que se complete la configuración de CMake.
* Elija el objetivo `VS2026 x64 Release`.
* Construir el proyecto.

Se creará el archivo de destino `out/build/windows-vs2026-x64-release/suika3.exe`.

---

## Windows (WSL2)

Este método utiliza MinGW en WSL2 para crear un archivo exe de Windows.
Tenga en cuenta que es posible construir Suika3 usando MinGW,
pero no se recomienda debido a los falsos positivos en el software antivirus.

### Requisitos previos

* Una PC `Windows 11` con procesador Intel, AMD o Arm64
* La característica `WSL2` instalada
* `Ubuntu` o `Debian` instalado en WSL2

```
sudo apt-get install cmake ninja-build mingw-w64
```

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

Se creará el archivo de destino `build-mingw-x86_64/suika3.exe`.

---

## macOS (paquete de aplicaciones)

Este método crea un paquete de aplicaciones para macOS y se utiliza para el binario oficial.

### Requisitos previos

* Una Mac con procesador Apple Silicon o Intel
* `macOS 11` o posterior instalado
* `Xcode` instalado

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

Se creará el objetivo `build-macos/Suika3.app`.

### Embalaje DMG

Si desea distribuir Suika3 para macOS, debe crear un archivo DMG con firma de código y notificación.
Debe tener un certificado `Developer ID Application` en el llavero e iniciar sesión con una cuenta de desarrollador de Apple en Xcode.
Tenga en cuenta que un paquete de aplicación distribuido a través de un archivo zip no puede acceder fuera del paquete de aplicación, por lo que usamos DMG aquí.

```
cd macos

# Sign the app.
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# Notarize the app. (takes some time)
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# Create a DMG file.
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# Sign the DMG file to allow access to files outside the bundle (avoid Gatekeeper issues).
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS (CLI)

Este método crea una versión de interfaz de línea de comandos (CLI) de Suika3 para macOS.
Es útil para depuración y desarrollo, pero no se recomienda para distribución.

### Requisitos previos

* Una Mac con procesador Apple Silicon o Intel
* `macOS 11` o posterior instalado
* `Xcode` instalado

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

Se creará el objetivo `build-macos/Suika3.app`.

Si desea depurar Suika3 con LLDB, puede usar el valor preestablecido `macos-cli-debug` en lugar de `macos`.

---

##iOS

### Requisitos previos

* Una Mac con procesador Apple Silicon o Intel
* `macOS 11` o posterior instalado
* `Xcode` instalado

### Pasos

* Empaquetar activos en el archivo `assets.arc`.
* Descargue [el binario ](https://github.com/suika3-community/suika/releases) oficial y extráigalo.
* Copie su archivo `assets.arc` en la carpeta `SDK/ios/resources`.
* Abra la carpeta `SDK/ios` en Xcode.
* Construir y ejecutar.

### Construir desde cero

Si desea construir desde cero, use `cmake --preset ios-device` o `cmake --preset ios-simulator`,
luego copie el archivo `libsuika3.a` creado en la carpeta `SDK/ios/lib` y abra la carpeta `SDK/ios` en Xcode.

---

## androide

### Requisitos previos

* `Android Studio`

### Pasos

* Descargue [el binario ](https://github.com/suika3-community/suika/releases) oficial y extráigalo.
* Copie sus archivos de activos en la carpeta `SDK/android/app/src/main/assets`.
* Abra la carpeta `SDK/android` en Android Studio.

### Construir desde cero

Si desea compilar desde cero, use `cmake --preset android-arm64`, luego copie el archivo `libsuika3.so` compilado en la carpeta `SDK/android/app/src/main/jniLibs/arm64-v8a` y abra la carpeta `SDK/android` en Android Studio.

---

## HarmonyOS SIGUIENTE / OpenHarmony

### Requisitos previos

* `OpenHarmony SDK` instalado

### Pasos

* Descargue [el binario ](https://github.com/awemorris/suika/releases) oficial y extráigalo.
* Copie sus archivos de activos en la carpeta `SDK/openharmony/entry/src/main/resources/rawfile`.
* Abra la carpeta `SDK/openharmony` en DevEco.
* Construir y ejecutar.

### Construir desde cero

Si desea construir desde cero, use `cmake --preset openharmony-arm64`,
luego copie el archivo `libsuika3.a` creado en la carpeta `SDK/openharmony/entry/libs/arm64-v8a`,
y abra la carpeta `SDK/openharmony` en DevEco.

---

## Asamblea web

### Requisitos previos

* `emsdk` instalado. (Cualquier sistema operativo servirá).

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

Se creará el archivo de destino `wasm/index.html`.

### Pruebas

Para ejecutar la aplicación, coloque su archivo `assets.arc` en la carpeta `wasm`.

Después de eso, escriba `python -m http.server` y abra `http://localhost:8000` en una ventana del navegador.

En Windows, puedes usar `suika3-web.exe` en lugar de `python`. Ejecuta un pequeño servidor web y abre el navegador automáticamente.

---

## FreeBSD

### Requisitos previos

* Una máquina `FreeBSD`
* `cmake` y `ninja` instalados.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

Se creará el archivo de destino `build-freebsd/suika3`.

---

##NetBSD

### Requisitos previos

* Una máquina `NetBSD`
* `cmake` y `ninja` instalados.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

Se creará el archivo de destino `build-netbsd/suika3`.

---

##OpenBSD

### Requisitos previos

* Una máquina `OpenBSD`
* `cmake` y `ninja` instalados.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

Se creará el archivo de destino `build-openbsd/suika3`.

---

## Solaris 11

### Requisitos previos

* Una máquina `Solaris 11`
* `SunCC`, `cmake` y `gmake` instalados.
* `cmake` debe compilarse mediante código fuente.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris11
cmake --build --preset solaris11
```

Se creará el archivo de destino `build-solaris11/suika3`.

---

## Solaris 10

### Requisitos previos

* Una máquina `Solaris 10`
* `SunCC`, `cmake` y `gmake` instalados.
* `cmake` debe compilarse mediante código fuente.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris10
cmake --build --preset solaris10
```

Se creará el archivo de destino `build-solaris10/suika3`.

---

## haikus

### Requisitos previos

* Una máquina `Haiku`
* `gcc`, `cmake` y `ninja` instalados.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset haiku
cmake --build --preset haiku
```

Se creará el archivo de destino `build-solaris10/suika3`.

---

## Complemento de Unity

### Requisitos previos

* Versión completa de `LLVM-22` instalada.

### Pasos

Abra la terminal y escriba lo siguiente.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

Se creará el archivo de destino `build-unity-win64/libsuika3.dll`.

Nota: Reemplace `win64` por uno de `switch`, `ps5` y `xbox`.

Para utilizar un SDK oficial del proveedor, abra `cmake/toolchains/unity-*.cmake` y reemplace `CMAKE_C_COMPILER`, `CMAKE_CXX_COMPILER` y `CMAKE_AR`.
