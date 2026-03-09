Playfield Engine のビルド方法
=============================

## Windows (Visual Studio)

### 前提

* Intel, AMD, Arm64 のいずれかの `Windows 11` PC であること
* `Visual Studio 2022 Community` がインストールされていること (C++, CMake)
* `Git for Windows` がインストールされていること
* `Playfield Engine` ソースツリーが展開されていること

### 手順

- ソースコードフォルダをVisual Studioで開く
- MSVC ターゲットを選択する
- ビルドして実行する

## Windows (WSL2)

### 前提

* Intel, AMD, Arm64 のいずれかの `Windows 11` PC であること
* `WSL2` 環境がインストールされていること
* `Ubuntu` か `Debian` の WSL2 環境であること
* 次のコマンドを実行済みであること
```
sudo apt-get install mingw-w64
```

### 手順

WSL2 のターミナルを起動し、`PlayfieldEngine` ディレクトリに入り、次のコマンドをタイプする。

```
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

## Linux

### 前提

* Linux マシンであること
* `Ubuntu`, `Debian`, or `Raspberry Pi OS` がインストールされていること
* `GNU Make` がインストールされていること

Debian, Ubuntu, および Raspberry Pi OS:
```
sudo apt-get install build-essential libx11-dev libxpm-dev mesa-common-dev libasound2-dev
```

RedHat, Rocky Linux, Fedora, その他:
```
sudo dnf install patch libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel
```

### 手順

ターミナルを開き、`PlayfieldEngine` ディレクトリに入り、次のコマンドをタイプする。

```
cmake --preset linux
cmake --build --preset linux
```

## macOS

### 前提

* Intel および Apple Silicon の Mac
* `macOS 15` がインストールされていること
* `Xcode 16` がインストールされていること

### 手順

ターミナルを開き、`PlayfieldEngine` ディレクトリに入り、次のコマンドをタイプする。

```
cmake --preset macos
cmake --build --preset macos
```

## iOS

### Prerequisite

* Intel および Apple Silicon の Mac
* `macOS 15` がインストールされていること
* `Xcode 16` がインストールされていること

### 手順

- あらかじめアセットを`assets.arc` にまとめる
- 配布 SDK の `ios/resources` フォルダに `assets.arc` をコピーする
- 配布 SDK の `ios` フォルダを Xcode で開く

## Android

### 前提

* 以下のいずれかのマシン
    * `Windows 11`
    * `macOS 15`
    * `Linux`
* `Android Studio`

### 手順

- 配布 SDK の `android/app/src/main/assets` フォルダにアセットをコピーする
- 配布 SDK の `android` フォルダを Android Studio で開く

First, open a terminal and enter the `PlayfieldEngine` direcotry, then type the following.

```
cd projects/android-src
make src
```

Then, open the Android Studio project at `projects/android/android-src`.
Build the project on Android Studio.
Note that you need to copy game files into `assets` folder.

## FreeBSD

### 前提

* `FreeBSD` マシン

### 手順

ターミナルで `PlayfieldEngine` ディレクトリに入り、次のコマンドをタイプする。

```
cmake --preset freebsd
cmake --build --preset freebsd
```
