Cómo utilizar AOT
==============

Suika3 admite la **compilación anticipada (AOT)** de scripts.
Es decir, una aplicación puede ejecutar código completamente nativo en lugar de hacerlo como un intérprete de código de bytes.

El comando `suika3-aotcomp` convierte los scripts `.ray` en **código fuente ANSI C**.
El archivo `library.c` generado se compilará con todo el motor.

---

## 1. Modificar `main.ray`

Debido a que los scripts se compilarán en código nativo,
Ya no es necesario cargar la biblioteca en tiempo de ejecución.

Abra `main.ray` y comente las llamadas loadLibrary().

Ejemplo:
```
// Suika.loadPlugin("system")
```

Tenga en cuenta que no debe llamar a `Suika.loadPlugin()` fuera del
Archivo `main.ray`.

---

## 2. Generar fuente C

Para compilar scripts en código fuente C, ejecute:

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

Este comando genera el siguiente archivo:
```
library.c
```

El archivo generado contiene la biblioteca de scripts compilados.

> [!CONSEJOS]
> Especifique todos los archivos de script en la línea de comando, incluido `main.ray`.

Ejemplo:
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. Reemplace la biblioteca del motor

Copie el archivo `library.c` generado en el árbol de fuentes del motor:
```
external/PlayfieldEngine/src/library.c
```

Sobrescriba el archivo existente.

---

## 4. Construya el motor

Construya el proyecto Suika3 usando CMake como de costumbre.

Los scripts compilados ahora estarán vinculados al binario del motor.

### iOS

Para construir binarios estáticos, escriba:
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

Después de eso, copie las bibliotecas estáticas a su proyecto de iOS:
* Copiar `build-ios-device/libsuika3.a` a `Suika3.xcframework/ios-arm64/libsuika3.a`
* Copiar `build-ios-simulator/libsuika3.a` a `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

Sobrescriba el archivo existente.

### androide

Para crear archivos binarios compartidos, escriba:
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

Después de eso, copie las bibliotecas compartidas a su proyecto de Android:
* Copiar `build-android-arm64/libsuika3.so` a `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* Copiar `build-android-armv7/libsuika3.so` a `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* Copiar `build-android-x86/libsuika3.so` a `app/src/main/jniLibs/x86/libplayfield.so`
* Copiar `build-android-x86_64/libsuika3.so` a `app/src/main/jniLibs/x86_64/libplayfield.so`

Sobrescriba el archivo existente.

### HarmonyOS SIGUIENTE

Para crear archivos binarios compartidos, escriba:
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

Después de eso, copie las bibliotecas compartidas a su proyecto HarmonyOS NEXT:
* Copiar `build-openharmony-arm64/libsuika3.a` a `entry/libs/arm64-v8a/libsuika3.a`
* Copiar `build-openharmony-x86_64/libsuika3.a` a `entry/libs/x86_64/libsuika3.a`

Sobrescriba el archivo existente.

### Complemento de Unity

Para crear archivos binarios compartidos, escriba:
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

Después de eso, copie las bibliotecas a su proyecto de Unity:
* Copiar `build-unity-win64/libsuika3.dll` a `Assets/Plugins/x86_64/libplayfield.dll`

Sobrescriba el archivo existente.

---

## Resultados

Los scripts se incrustan directamente en el ejecutable y proporcionan:

* Sin JIT (para revisión de la tienda)
* No se carga el script en tiempo de ejecución
* Inicio más rápido
