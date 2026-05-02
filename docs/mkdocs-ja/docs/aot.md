AOT の使い方
============

Suika3 の Ray 言語処理系は、スクリプトの **事前 (Ahead-of-Time: AOT) コンパイル** に対応しています。
つまり、Ray で書かれたプログラムは、バイトコードインタプリタを介してではなく、完全にネイティブコードに変換してから実行できます。

`suika3-aotcomp` コマンドは、`.ray` スクリプトを **ANSI C ソースコード**に変換します。
生成された `library.c` ファイルは、エンジン全体と一緒にコンパイルされます。

バイナリに直接変換せずに C 言語を経由する理由は、未知の CPU アーキテクチャに対応することと、コンソールにおいてベンダのコンパイラが生成したコード以外が許可されないケースがあるからです。

---

## 1. `main.ray` を変更する

スクリプトはネイティブコードにコンパイルされるため、
実行時ライブラリを読み込む必要はなくなります。

`main.ray` を開き、loadLibrary() の呼び出しをコメントアウトしてください。

例:
```
// Suika.loadPlugin("system")
```

`Suika.loadPlugin()` は `main.ray` ファイル以外から呼び出さないでください。

---

## 2. C ソースを生成する

スクリプトを C ソースコードにコンパイルするには、次を実行します。

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

このコマンドは、次のファイルを生成します。
```
library.c
```

生成されたファイルには、コンパイル済みのスクリプトライブラリが含まれます。

> [!TIPS]
> `main.ray` を含むすべてのスクリプトファイルをコマンドラインで指定してください。

例:
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. エンジンライブラリを置き換える

生成された `library.c` ファイルをエンジンのソースツリーにコピーします。
```
external/PlayfieldEngine/src/library.c
```

既存のファイルを上書きしてください。

---

## 4. エンジンをビルドする

通常どおり CMake を使って Suika3 プロジェクトをビルドします。

コンパイル済みスクリプトが、エンジンバイナリにリンクされるようになります。

### iOS

静的バイナリをビルドするには、次を入力します。
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

その後、静的ライブラリを iOS プロジェクトにコピーします。
* `build-ios-device/libsuika3.a` を `Suika3.xcframework/ios-arm64/libsuika3.a` にコピーする
* `build-ios-simulator/libsuika3.a` を `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a` にコピーする

既存のファイルを上書きしてください。

### Android

共有バイナリをビルドするには、次を入力します。
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

その後、共有ライブラリを Android プロジェクトにコピーします。
* `build-android-arm64/libsuika3.so` を `app/src/main/jniLibs/arm64-v8a/libplayfield.so` にコピーする
* `build-android-armv7/libsuika3.so` を `app/src/main/jniLibs/armeabi-v7a/libplayfield.so` にコピーする
* `build-android-x86/libsuika3.so` を `app/src/main/jniLibs/x86/libplayfield.so` にコピーする
* `build-android-x86_64/libsuika3.so` を `app/src/main/jniLibs/x86_64/libplayfield.so` にコピーする

既存のファイルを上書きしてください。

### HarmonyOS NEXT

共有バイナリをビルドするには、次を入力します。
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

その後、共有ライブラリを HarmonyOS NEXT プロジェクトにコピーします。
* `build-openharmony-arm64/libsuika3.a` を `entry/libs/arm64-v8a/libsuika3.a` にコピーする
* `build-openharmony-x86_64/libsuika3.a` を `entry/libs/x86_64/libsuika3.a` にコピーする

既存のファイルを上書きしてください。

### Unity Plugin

共有バイナリをビルドするには、次を入力します。
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

その後、ライブラリを Unity プロジェクトにコピーします。
* `build-unity-win64/libsuika3.dll` を `Assets/Plugins/x86_64/libplayfield.dll` にコピーする

既存のファイルを上書きしてください。

---

## 結果

スクリプトは実行ファイルに直接埋め込まれ、次の利点があります。

* JIT が不要になる (ストア審査向け)
* 実行時のスクリプト読み込みが不要になる
* 起動が速くなる
