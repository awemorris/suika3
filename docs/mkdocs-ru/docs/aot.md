Как использовать AOT
====================

Suika3 поддерживает **Ahead-of-Time (AOT) compilation** для скриптов.
Это означает, что приложение может выполнять полностью нативный код вместо интерпретатора байткода.

Команда `suika3-aotcomp` преобразует скрипты `.ray` в **исходный код ANSI C**.
Сгенерированный файл `library.c` будет скомпилирован вместе со всем движком.

---

## 1. Измените `main.ray`

Поскольку скрипты будут скомпилированы в нативный код, загрузка библиотеки
во время выполнения больше не нужна.

Откройте `main.ray` и закомментируйте вызовы loadLibrary().

Пример:
```
// Suika.loadPlugin("system")
```

Обратите внимание: не следует вызывать `Suika.loadPlugin()` вне файла
`main.ray`.

---

## 2. Сгенерируйте исходный код C

Чтобы скомпилировать скрипты в исходный код C, выполните:

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

Эта команда создает следующий файл:
```
library.c
```

Сгенерированный файл содержит скомпилированную библиотеку скриптов.

> [!TIPS]
> Укажите в командной строке все файлы скриптов, включая `main.ray`.

Пример:
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. Замените библиотеку движка

Скопируйте сгенерированный файл `library.c` в дерево исходного кода движка:
```
external/PlayfieldEngine/src/library.c
```

Перезапишите существующий файл.

---

## 4. Соберите движок

Соберите проект Suika3 с помощью CMake как обычно.

Скомпилированные скрипты теперь будут связаны с бинарным файлом движка.

### iOS

Чтобы собрать статические бинарные файлы, выполните:
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

После этого скопируйте статические библиотеки в свой iOS-проект:
* Скопируйте `build-ios-device/libsuika3.a` в `Suika3.xcframework/ios-arm64/libsuika3.a`
* Скопируйте `build-ios-simulator/libsuika3.a` в `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

Перезапишите существующий файл.

### Android

Чтобы собрать динамические бинарные файлы, выполните:
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

После этого скопируйте динамические библиотеки в свой Android-проект:
* Скопируйте `build-android-arm64/libsuika3.so` в `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* Скопируйте `build-android-armv7/libsuika3.so` в `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* Скопируйте `build-android-x86/libsuika3.so` в `app/src/main/jniLibs/x86/libplayfield.so`
* Скопируйте `build-android-x86_64/libsuika3.so` в `app/src/main/jniLibs/x86_64/libplayfield.so`

Перезапишите существующий файл.

### HarmonyOS NEXT

Чтобы собрать динамические бинарные файлы, выполните:
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

После этого скопируйте динамические библиотеки в свой проект HarmonyOS NEXT:
* Скопируйте `build-openharmony-arm64/libsuika3.a` в `entry/libs/arm64-v8a/libsuika3.a`
* Скопируйте `build-openharmony-x86_64/libsuika3.a` в `entry/libs/x86_64/libsuika3.a`

Перезапишите существующий файл.

### Unity Plugin

Чтобы собрать динамические бинарные файлы, выполните:
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

После этого скопируйте библиотеки в свой Unity-проект:
* Скопируйте `build-unity-win64/libsuika3.dll` в `Assets/Plugins/x86_64/libplayfield.dll`

Перезапишите существующий файл.

---

## Результаты

Скрипты встраиваются непосредственно в исполняемый файл, что дает:

* Отсутствие JIT (для проверки в магазинах приложений)
* Отсутствие загрузки скриптов во время выполнения
* Более быстрый запуск
