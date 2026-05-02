Suika3-Bauanleitung
========================

Suika3 nutzt das CMake-Build-System vollständig aus.

Hinweise:
* Erfordert CMake 3.22 oder höher
* Linux: GCC 4.4 oder höher (Clang wird auch unterstützt)
* Windows: Visual Studio 2022/2026 oder gcc/clang auf WSL
* macOS: Xcode 8.2.1 oder höher erforderlich

---

## Linux (Wayland/X11 Dual)

### Voraussetzungen

* Eine `Linux`-Maschine mit einem beliebigen Prozessor

Unter Debian, Ubuntu oder Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

Auf RedHat, Rocky Linux, Fedora usw.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build alsa-lib-devel libX11-devel libXpm-devel wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux
cmake --build --preset linux
```

Die Zieldatei `__TECH0__` wird erstellt.

Wenn Sie Suika3 mit GDB debuggen möchten, können Sie die Voreinstellung `linux-debug` anstelle von `linux` verwenden.

---

## Linux (nur X11)

### Voraussetzungen

* Eine `Linux`-Maschine mit einem beliebigen Prozessor

Unter Debian, Ubuntu oder Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libx11-dev libxpm-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

Auf RedHat, Rocky Linux, Fedora usw.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

Die Zieldatei `__TECH0__` wird erstellt.

Wenn Sie Suika3 mit GDB debuggen möchten, können Sie die Voreinstellung `linux-x11-debug` anstelle von `linux-x11` verwenden.


---

## Linux (Wayland)

Beachten Sie, dass die Wayland-Unterstützung noch experimentell ist. Es ist mit KDE kompatibel, hat jedoch Probleme mit der Anzeige von Fenstern unter GNOME.

### Voraussetzungen

* Eine `Linux`-Maschine mit einem beliebigen Prozessor

Unter Debian oder Ubuntu:
```
sudo apt-get install git cmake ninja-build build-essential libasound2-dev libwayland-dev wayland-protocols libegl1-mesa-dev libegl-dev libgles-dev libwayland-client0 libwayland-egl1 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x libdecor-0-dev
```

Auf RedHat, Rocky Linux, Fedora usw.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel libdecor-devel
```

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

Die Zieldatei `__TECH0__` wird erstellt.

Wenn Sie Suika3 mit GDB debuggen möchten, können Sie die Voreinstellung `linux-wayland-debug` anstelle von `linux-wayland` verwenden.

---

## Windows (Visual Studio 2026)

Visual Studio ist die empfohlene Build-Umgebung für Windows und wird für die offizielle Binärdatei verwendet.

### Voraussetzungen

* Ein `Windows 11`-PC mit einem Intel-, AMD- oder Arm64-Prozessor
* `Visual Studio 2026` installiert mit C/C++ und CMake konfiguriert

### Schritte

* Klonen Sie das Repository.
* Öffnen Sie das Stammverzeichnis des Quellcodeordners in Visual Studio.
* Warten Sie, bis die CMake-Konfiguration abgeschlossen ist.
* Wählen Sie das Ziel `VS2026 x64 Release`.
* Erstellen Sie das Projekt.

Die Zieldatei `__TECH0__` wird erstellt.

---

## Windows (WSL2)

This method uses MinGW on WSL2 to create a Windows exe file. Note that it is possible to build Suika3 using MinGW, but it is not recommended due to false positives in antivirus software.

### Voraussetzungen

* Ein `Windows 11`-PC mit einem Intel-, AMD- oder Arm64-Prozessor
* Die Funktion `WSL2` ist installiert
* `Ubuntu` or `Debian` installed on WSL2

```
sudo apt-get install cmake ninja-build mingw-w64
```

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## macOS (App-Bundle)

Diese Methode erstellt ein App-Bundle für macOS und wird für die offizielle Binärdatei verwendet.

### Voraussetzungen

* Ein Mac mit einem Apple Silicon- oder Intel-Prozessor
* `macOS 11` oder höher installiert
* `Xcode` installiert

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

Das Ziel `__TECH0__` wird erstellt.

### DMG-Verpackung

Wenn Sie Suika3 für macOS verteilen möchten, müssen Sie eine DMG-Datei mit Codesignatur und Benachrichtigung erstellen. Sie müssen über ein `Developer ID Application`-Zertifikat im Schlüsselbund verfügen und mit einem Apple Developer-Konto bei Xcode angemeldet sein. Bitte beachten Sie, dass auf ein über eine ZIP-Datei verteiltes App-Bundle kein Zugriff außerhalb des App-Bundles möglich ist. Daher verwenden wir hier DMG.

```
cd macos

# Signieren Sie die App.
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# Beglaubigen Sie die App. (dauert einige Zeit)
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# Erstellen Sie eine DMG-Datei.
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# Signieren Sie die DMG-Datei, um den Zugriff auf Dateien außerhalb des Bundles zu ermöglichen (vermeiden Sie Gatekeeper-Probleme).
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS (CLI)

Diese Methode erstellt eine Befehlszeilenschnittstellenversion (CLI) von Suika3 für macOS. Es ist nützlich zum Debuggen und Entwickeln, wird jedoch nicht für die Verteilung empfohlen.

### Voraussetzungen

* Ein Mac mit einem Apple Silicon- oder Intel-Prozessor
* `macOS 11` oder höher installiert
* `Xcode` installiert

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

Das Ziel `__TECH0__` wird erstellt.

Wenn Sie Suika3 mit LLDB debuggen möchten, können Sie die Voreinstellung `macos-cli-debug` anstelle von `macos` verwenden.

---

## iOS

### Voraussetzungen

* Ein Mac mit einem Apple Silicon- oder Intel-Prozessor
* `macOS 11` oder höher installiert
* `Xcode` installiert

### Schritte

* Packen Sie Assets in die Datei `__TECH0__`.
* Laden Sie [die offizielle Binärdatei](https://github.com/suika3-community/suika/releases) herunter und extrahieren Sie sie.
* Kopieren Sie Ihre `__TECH0__`-Datei in den Ordner `__TECH1__`.
* Öffnen Sie den Ordner `__TECH0__` in Xcode.
* Erstellen und ausführen.

### Von Grund auf neu erstellen

Wenn Sie von Grund auf neu erstellen möchten, verwenden Sie `cmake --preset ios-device` oder `cmake --preset ios-simulator`, kopieren Sie dann die erstellte `__TECH0__`-Datei in den Ordner `__TECH1__` und öffnen Sie den Ordner `__TECH2__` in Xcode.

---

## Android

### Voraussetzungen

* `Android Studio`

### Schritte

* Laden Sie [die offizielle Binärdatei](https://github.com/suika3-community/suika/releases) herunter und extrahieren Sie sie.
* Kopieren Sie Ihre Asset-Dateien in den Ordner `__TECH0__`.
* Öffnen Sie den Ordner `__TECH0__` in Android Studio.

### Von Grund auf neu erstellen

Wenn Sie von Grund auf neu erstellen möchten, verwenden Sie `cmake --preset android-arm64`, kopieren Sie dann die erstellte `__TECH0__`-Datei in den Ordner `__TECH1__` und öffnen Sie den Ordner `__TECH2__` in Android Studio.

---

## HarmonyOS NEXT / OpenHarmony

### Voraussetzungen

* `OpenHarmony SDK` installiert

### Schritte

* Laden Sie [die offizielle Binärdatei](https://github.com/awemorris/suika/releases) herunter und extrahieren Sie sie.
* Kopieren Sie Ihre Asset-Dateien in den Ordner `__TECH0__`.
* Öffnen Sie den Ordner `__TECH0__` in DevEco.
* Erstellen und ausführen.

### Von Grund auf neu erstellen

Wenn Sie von Grund auf neu erstellen möchten, verwenden Sie `cmake --preset openharmony-arm64`, kopieren Sie dann die erstellte `__TECH0__`-Datei in den Ordner `__TECH1__` und öffnen Sie den Ordner `__TECH2__` in DevEco.

---

## WebAssembly

### Voraussetzungen

* `emsdk` installiert. (Jedes Betriebssystem reicht aus.)

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

Die Zieldatei `__TECH0__` wird erstellt.

### Testen

Um die App auszuführen, platzieren Sie Ihre `__TECH0__`-Datei im Ordner `wasm`.

Geben Sie anschließend `python -m __TECH0__` ein und öffnen Sie `http://localhost:8000` in einem Browserfenster.

Unter Windows können Sie `suika3-__TECH0__` anstelle von `python` verwenden. Es betreibt einen kleinen Webserver und öffnet den Browser automatisch.

---

## FreeBSD

### Voraussetzungen

* Eine `FreeBSD`-Maschine
* `cmake` und `ninja` installiert.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## NetBSD

### Voraussetzungen

* Eine `NetBSD`-Maschine
* `cmake` und `ninja` installiert.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## OpenBSD

### Voraussetzungen

* Eine `OpenBSD`-Maschine
* `cmake` und `ninja` installiert.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## Solaris 11

### Voraussetzungen

* Eine `Solaris 11`-Maschine
* `SunCC`, `cmake` und `gmake` installiert.
* `cmake` muss per Quellcode kompiliert werden.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris11
cmake --build --preset solaris11
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## Solaris 10

### Voraussetzungen

* Eine `Solaris 10`-Maschine
* `SunCC`, `cmake` und `gmake` installiert.
* `cmake` muss per Quellcode kompiliert werden.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset solaris10
cmake --build --preset solaris10
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## Haiku

### Voraussetzungen

* Eine `Haiku`-Maschine
* `gcc`, `cmake` und `ninja` installiert.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset haiku
cmake --build --preset haiku
```

Die Zieldatei `__TECH0__` wird erstellt.

---

## Unity-Plugin

### Voraussetzungen

* Vollständiger Build von `LLVM-22` installiert.

### Schritte

Öffnen Sie das Terminal und geben Sie Folgendes ein.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

Die Zieldatei `__TECH0__` wird erstellt.

Hinweis: Ersetzen Sie `win64` durch einen von `switch`, `ps5` und `xbox`.

Um ein offizielles SDK des Anbieters zu verwenden, öffnen Sie `__TECH0__*.cmake` und ersetzen Sie `CMAKE_C_COMPILER`, `CMAKE_CXX_COMPILER` und `CMAKE_AR`.
