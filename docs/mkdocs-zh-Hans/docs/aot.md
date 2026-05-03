如何使用 AOT
==============

Suika3 支持脚本的**提前编译**。
也就是说，应用程序可以完全运行原生代码，而不是作为字节码解释器运行。

`suika3-aotcomp` 命令将 `.ray` 脚本转换为 **ANSI C 源代码**。
生成的 `library.c` 文件将与整个引擎一起编译。

---

## 1. 修改 `main.ray`

由于脚本将被编译为原生代码，因此不再需要加载运行时库。

打开 `main.ray` 并注释掉 `loadLibrary()` 调用。

示例：
```
// Suika.loadPlugin("system")
```

请注意，你不应该在 `main.ray` 文件之外调用 `Suika.loadPlugin()`。

---

## 2. 生成 C 源代码

要将脚本编译为 C 源代码，请运行：

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

此命令会生成以下文件：
```
library.c
```

生成的文件包含编译后的脚本库。

> [!提示]
> 在命令行中指定所有脚本文件，包括 `main.ray`。

示例：
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. 替换引擎库

将生成的 `library.c` 文件复制到引擎源代码树中：
```
external/PlayfieldEngine/src/library.c
```

覆盖现有文件。

---

## 4. 构建引擎

像往常一样使用 CMake 构建 Suika3 项目。

编译后的脚本现在将链接到引擎二进制文件中。

### iOS

要构建静态库，请输入：
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

之后，将静态库复制到你的 iOS 项目中：
* Copy `build-ios-device/libsuika3.a` to `Suika3.xcframework/ios-arm64/libsuika3.a`
* Copy `build-ios-simulator/libsuika3.a` to `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

覆盖现有文件。

### Android

要构建共享库，请输入：
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

之后，将共享库复制到你的 Android 项目中：
* 将 `build-android-arm64/libsuika3.so` 复制到 `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* 将 `build-android-armv7/libsuika3.so` 复制到 `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* 将 `build-android-x86/libsuika3.so` 复制到 `app/src/main/jniLibs/x86/libplayfield.so`
* 将 `build-android-x86_64/libsuika3.so` 复制到 `app/src/main/jniLibs/x86_64/libplayfield.so`

覆盖现有文件。

### HarmonyOS NEXT

要构建共享库，请输入：
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

之后，将共享库复制到你的 HarmonyOS NEXT 项目中：
* 将 `build-openharmony-arm64/libsuika3.a` 复制到 `entry/libs/arm64-v8a/libsuika3.a`
* 将 `build-openharmony-x86_64/libsuika3.a` 复制到 `entry/libs/x86_64/libsuika3.a`

覆盖现有文件。

### Unity Plugin

要构建共享库，请输入：
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

之后，将库复制到你的 Unity 项目中：
* Copy `build-unity-win64/libsuika3.dll` to `Assets/Plugins/x86_64/libplayfield.dll`

覆盖现有文件。

---

## 结果

脚本直接嵌入到可执行文件中，从而提供：

* 无即时编译（用于应用商店审核）
* 无运行时脚本加载
* 更快的启动速度
