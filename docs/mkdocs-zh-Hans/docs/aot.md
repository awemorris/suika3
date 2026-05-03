如何使用 AOT
==============

Suika3 支援指令码的 **Ahead-of-Time（AOT）编译**。
也就是说，应用程式可以完全以原生程式码执行，而不是以位元组码直译器运作。

`suika3-aotcomp` 指令会把 `.ray` 指令码转换成 **ANSI C 原始码**。
产生的 `library.c` 档案会与整个引擎一起编译。

---

## 1. 修改 `main.ray`

由于指令码会被编译成原生程式码，
因此不再需要载入执行期函式库。

开启 `main.ray`，并把 `loadLibrary()` 呼叫注解掉。

范例：
```
// Suika.loadPlugin("system")
```

请注意，`Suika.loadPlugin()` 不应该在 `main.ray` 以外的地方呼叫。

---

## 2. 产生 C 原始码

要把指令码编译成 C 原始码，请执行：

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

这个指令会产生以下档案：
```
library.c
```

产生的档案包含已编译的指令码函式库。

> [!TIPS]
> 请在命令列中指定所有指令码档，包括 `main.ray`。

范例：
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. 取代引擎函式库

将产生的 `library.c` 档案复制到引擎原始码树中：
```
external/PlayfieldEngine/src/library.c
```

覆盖既有档案。

---

## 4. 建置引擎

照常使用 CMake 建置 Suika3 专案。

编译后的指令码现在会被连结进引擎二进位档中。

### iOS

若要建置静态二进位档，请输入：
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

接著，将静态函式库复制到你的 iOS 专案：
* 将 `build-ios-device/libsuika3.a` 复制到 `Suika3.xcframework/ios-arm64/libsuika3.a`
* 将 `build-ios-simulator/libsuika3.a` 复制到 `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

覆盖既有档案。

### Android

若要建置共享二进位档，请输入：
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

接著，将共享函式库复制到你的 Android 专案：
* 将 `build-android-arm64/libsuika3.so` 复制到 `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* 将 `build-android-armv7/libsuika3.so` 复制到 `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* 将 `build-android-x86/libsuika3.so` 复制到 `app/src/main/jniLibs/x86/libplayfield.so`
* 将 `build-android-x86_64/libsuika3.so` 复制到 `app/src/main/jniLibs/x86_64/libplayfield.so`

覆盖既有档案。

### HarmonyOS NEXT

若要建置共享二进位档，请输入：
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

接著，将共享函式库复制到你的 HarmonyOS NEXT 专案：
* 将 `build-openharmony-arm64/libsuika3.a` 复制到 `entry/libs/arm64-v8a/libsuika3.a`
* 将 `build-openharmony-x86_64/libsuika3.a` 复制到 `entry/libs/x86_64/libsuika3.a`

覆盖既有档案。

### Unity Plugin

若要建置共享二进位档，请输入：
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

接著，将函式库复制到你的 Unity 专案：
* 将 `build-unity-win64/libsuika3.dll` 复制到 `Assets/Plugins/x86_64/libplayfield.dll`

覆盖既有档案。

---

## 结果

指令码会直接嵌入执行档，带来以下好处：

* 没有 JIT（可透过商店稽核）
* 不需要在执行期载入指令码
* 启动更快
