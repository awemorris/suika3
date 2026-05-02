Wie AOT verwendet wird
======================

Suika3 unterstützt die **Ahead-of-Time-(AOT)-Kompilierung** von Skripten.
Das heißt, eine App kann vollständig nativen Code ausführen, statt als Bytecode-Interpreter zu laufen.

Der Befehl `suika3-aotcomp` konvertiert `.ray`-Skripte in **ANSI-C-Quellcode**.
Die erzeugte Datei `library.c` wird mit der gesamten Engine kompiliert.

---

## 1. `main.ray` anpassen

Da die Skripte in nativen Code kompiliert werden,
ist das Laden der Runtime-Bibliothek nicht mehr erforderlich.

Öffne `main.ray` und kommentiere die `loadLibrary()`-Aufrufe aus.

Beispiel:
```
// Suika.loadPlugin("system")
```

Bitte beachte, dass du `Suika.loadPlugin()` nicht außerhalb der Datei
`main.ray` aufrufen solltest.

---

## 2. C-Quellcode erzeugen

Um Skripte in C-Quellcode zu kompilieren, führe aus:

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

Dieser Befehl erzeugt die folgende Datei:
```
library.c
```

Die erzeugte Datei enthält die kompilierte Skriptbibliothek.

> [!TIPS]
> Gib in der Befehlszeile alle Skriptdateien an, einschließlich `main.ray`.

Beispiel:
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. Die Engine-Bibliothek ersetzen

Kopiere die erzeugte Datei `library.c` in den Quellbaum der Engine:
```
external/PlayfieldEngine/src/library.c
```

Überschreibe die vorhandene Datei.

---

## 4. Die Engine bauen

Baue das Suika3-Projekt wie gewohnt mit CMake.

Die kompilierten Skripte werden nun in das Engine-Binary eingebunden.

### iOS

Um statische Binärdateien zu erstellen, gib ein:
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

Danach kopiere die statischen Bibliotheken in dein iOS-Projekt:
* Kopiere `build-ios-device/libsuika3.a` nach `Suika3.xcframework/ios-arm64/libsuika3.a`
* Kopiere `build-ios-simulator/libsuika3.a` nach `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

Überschreibe die vorhandene Datei.

### Android

Um geteilte Binärdateien zu erstellen, gib ein:
```
cmake --preset android-arm64
cmake --preset android-arvm7
cmake --preset android-x86
cmake --preset android-x86_64
cmake --build --preset android-arm64
cmake --build --preset android-arvm7
cmake --build --preset android-x86
cmake --build --preset android-x86_64
```

Danach kopiere die geteilten Bibliotheken in dein Android-Projekt:
* Kopiere `build-android-arm64/libsuika3.so` nach `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* Kopiere `build-android-armv7/libsuika3.so` nach `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* Kopiere `build-android-x86/libsuika3.so` nach `app/src/main/jniLibs/x86/libplayfield.so`
* Kopiere `build-android-x86_64/libsuika3.so` nach `app/src/main/jniLibs/x86_64/libplayfield.so`

Überschreibe die vorhandene Datei.

### HarmonyOS NEXT

Um geteilte Binärdateien zu erstellen, gib ein:
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

Danach kopiere die geteilten Bibliotheken in dein HarmonyOS-NEXT-Projekt:
* Kopiere `build-openharmony-arm64/libsuika3.a` nach `entry/libs/arm64-v8a/libsuika3.a`
* Kopiere `build-openharmony-x86_64/libsuika3.a` nach `entry/libs/x86_64/libsuika3.a`

Überschreibe die vorhandene Datei.

### Unity Plugin

Um geteilte Binärdateien zu erstellen, gib ein:
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

Danach kopiere die Bibliotheken in dein Unity-Projekt:
* Kopiere `build-unity-win64/libsuika3.dll` nach `Assets/Plugins/x86_64/libplayfield.dll`

Überschreibe die vorhandene Datei.

---

## Ergebnisse

Die Skripte werden direkt in die ausführbare Datei eingebettet, wodurch Folgendes erreicht wird:

* Kein JIT (für Store-Reviews)
* Kein Laden von Skripten zur Laufzeit
* Schnellere Startzeit
