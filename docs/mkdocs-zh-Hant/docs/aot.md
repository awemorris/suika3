如何使用 AOT
==============

Suika3 支援指令碼的 **Ahead-of-Time（AOT）編譯**。
也就是說，應用程式可以完全以原生程式碼執行，而不是以位元組碼直譯器運作。

`suika3-aotcomp` 指令會把 `.ray` 指令碼轉換成 **ANSI C 原始碼**。
產生的 `library.c` 檔案會與整個引擎一起編譯。

---

## 1. 修改 `main.ray`

由於指令碼會被編譯成原生程式碼，
因此不再需要載入執行期函式庫。

開啟 `main.ray`，並把 `loadLibrary()` 呼叫註解掉。

範例：
```
// Suika.loadPlugin("system")
```

請注意，`Suika.loadPlugin()` 不應該在 `main.ray` 以外的地方呼叫。

---

## 2. 產生 C 原始碼

要把指令碼編譯成 C 原始碼，請執行：

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

這個指令會產生以下檔案：
```
library.c
```

產生的檔案包含已編譯的指令碼函式庫。

> [!TIPS]
> 請在命令列中指定所有指令碼檔，包括 `main.ray`。

範例：
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. 取代引擎函式庫

將產生的 `library.c` 檔案複製到引擎原始碼樹中：
```
external/PlayfieldEngine/src/library.c
```

覆蓋既有檔案。

---

## 4. 建置引擎

照常使用 CMake 建置 Suika3 專案。

編譯後的指令碼現在會被連結進引擎二進位檔中。

### iOS

若要建置靜態二進位檔，請輸入：
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

接著，將靜態函式庫複製到你的 iOS 專案：
* 將 `build-ios-device/libsuika3.a` 複製到 `Suika3.xcframework/ios-arm64/libsuika3.a`
* 將 `build-ios-simulator/libsuika3.a` 複製到 `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

覆蓋既有檔案。

### Android

若要建置共享二進位檔，請輸入：
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

接著，將共享函式庫複製到你的 Android 專案：
* 將 `build-android-arm64/libsuika3.so` 複製到 `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* 將 `build-android-armv7/libsuika3.so` 複製到 `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* 將 `build-android-x86/libsuika3.so` 複製到 `app/src/main/jniLibs/x86/libplayfield.so`
* 將 `build-android-x86_64/libsuika3.so` 複製到 `app/src/main/jniLibs/x86_64/libplayfield.so`

覆蓋既有檔案。

### HarmonyOS NEXT

若要建置共享二進位檔，請輸入：
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

接著，將共享函式庫複製到你的 HarmonyOS NEXT 專案：
* 將 `build-openharmony-arm64/libsuika3.a` 複製到 `entry/libs/arm64-v8a/libsuika3.a`
* 將 `build-openharmony-x86_64/libsuika3.a` 複製到 `entry/libs/x86_64/libsuika3.a`

覆蓋既有檔案。

### Unity Plugin

若要建置共享二進位檔，請輸入：
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

接著，將函式庫複製到你的 Unity 專案：
* 將 `build-unity-win64/libsuika3.dll` 複製到 `Assets/Plugins/x86_64/libplayfield.dll`

覆蓋既有檔案。

---

## 結果

指令碼會直接嵌入執行檔，帶來以下好處：

* 沒有 JIT（可透過商店稽核）
* 不需要在執行期載入指令碼
* 啟動更快
