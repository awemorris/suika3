#!/bin/sh

CUR_DIR=`dirname $0`
cd $CUR_DIR

ARCH=`uname -m`
OS=`uname -s`

echo "Copying assets..."
TARGET_ASSETS_DIR="$(cd "$(dirname "$0")"; pwd)/app/src/main/assets"
SRC_DIR="../../"
rm -rf "$TARGET_ASSETS_DIR"
mkdir -p "$TARGET_ASSETS_DIR"
find "$SRC_DIR" \
    \( -name ".vscode" -o -name "SDK" \) -prune -o \
    \( -name "suika3.exe" -o -name "assets.arc" \) -o \
    -type f -print | while read -r file; do
        rel_path="${file#$SRC_DIR}"
        dest_file="$TARGET_ASSETS_DIR/$rel_path"
        mkdir -p "$(dirname "$dest_file")"
        cp "$file" "$dest_file"
        echo "$rel_path"
done
if [ $? -ne 0 ]; then
    echo "[ERROR] Copy failed"
    exit 1
fi

echo "Downloading JDK..."
export CUR_DIR=$(pwd)
if [ "$OS" == "Linux" ]; then
    if [ "$ARCH" == "arm64" ]; then
        if [ ! -d jdk-17.0.1.jdk ]; then
            if [ ! -f openjdk-17.0.1_linux-aarch64_bin.tar.gz ]; then
                curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_linux-aarch64_bin.tar.gz;
            fi
            tar xzf openjdk-17.0.1_macos-aarch64_bin.tar.gz;
            export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
        fi
    else
        if [ ! -d jdk-17.0.1.jdk ]; then
            if [ ! -f openjdk-17.0.1_linux-x64_bin.tar.gz ]; then
                curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_linux-x64_bin.tar.gz;
            fi
            tar xzf openjdk-17.0.1_macos-x64_bin.tar.gz;
        fi
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    fi
fi
if [ "$OS" == "Darwin" ]; then
    if [ "$ARCH" == "arm64" ]; then
        if [ ! -d jdk-17.0.1.jdk ]; then
            if [ ! -f openjdk-17.0.1_linux-aarch64_bin.tar.gz ]; then
                curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_macos-aarch64_bin.tar.gz;
            fi
            tar xzf openjdk-17.0.1_macos-aarch64_bin.tar.gz;
        fi
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    else
        if [ ! -d jdk-17.0.1.jdk ]; then
            if [ ! -f openjdk-17.0.1_linux-aarch64_bin.tar.gz ]; then
                curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_macos-x64_bin.tar.gz;
            fi
            tar xzf openjdk-17.0.1_macos-x64_bin.tar.gz;
        fi
        export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    fi
fi

echo "Downloading Android SDK..."
if [ "$OS" == "Linux" ]; then
    if [ ! -d cmdline-tools ]; then
        if [ ! -f commandlinetools-linux-11076708_latest.zip ]; then
            curl -L -O https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip;
        fi
        unzip commandlinetools-linux-11076708_latest.zip;
    fi
fi
if [ "$OS" == "Darwin" ]; then
    if [ ! -d cmdline-tools ]; then
        if [ ! -f commandlinetools-mac-11076708_latest.zip ]; then
            curl -L -O https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip;
        fi
        unzip commandlinetools-mac-11076708_latest.zip;
    fi
fi

echo "Setting up Android SDK..."
export SDK_TEMP_ROOT=$CUR_DIR/android-sdk
yes | ./cmdline-tools/bin/sdkmanager --install "cmdline-tools;latest" --sdk_root=$SDK_TEMP_ROOT
export ANDROID_SDK_ROOT=$SDK_TEMP_ROOT
export ANDROID_HOME=$SDK_TEMP_ROOT
export NEW_SDKMANAGER=$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager
yes | $NEW_SDKMANAGER --install --sdk_root=$ANDROID_SDK_ROOT
yes | $NEW_SDKMANAGER --licenses --sdk_root=$ANDROID_SDK_ROOT
yes | $NEW_SDKMANAGER "platforms;android-35" "platform-tools" "build-tools;35.0.0" --sdk_root=$ANDROID_SDK_ROOT
echo "sdk.dir=$ANDROID_SDK_ROOT" > local.properties

echo "Building app..."
sh ./gradlew --no-daemon --stacktrace build

echo
echo "Please check the APK files:"
echo "  Debug:   $CUR_DIR/app/build/outputs/apk/debug/app-debug.apk"
echo "  Release: $CUR_DIR/app/build/outputs/apk/release/app-release-unsigned.apk"

echo
export PACKAGE_NAME=`$ANDROID_HOME/build-tools/35.0.0/aapt2 dump packagename $CUR_DIR/app/build/outputs/apk/debug/app-debug.apk`
echo "Package Name: $PACKAGE_NAME"
echo
echo "If you want to change the package name, edit applicationId in $CUR_DIR/app/build.gradle"
echo

echo -n "Do you want to install the APK to your Android device via ADB? (y/N): "
read s
if [ "$s" == "y" ]; then
    echo
    echo Uninstalling existing APK...
    $ANDROID_HOME/platform-tools/adb uninstall $PACKAGE_NAME

    echo Installing new APK...
    $ANDROID_SDK_ROOT/platform-tools/adb install -r $CUR_DIR/app/build/outputs/apk/debug/app-debug.apk

    echo Starting app...
    $ANDROID_HOME/platform-tools/adb shell am start -n $PACKAGE_NAME/io.noctvm.playfield.engineandroid.MainActivity
fi

echo

exit 0
