#!/bin/sh

CUR_DIR=`dirname $0`
cd $CUR_DIR

ARCH=`uname -m`
OS=`uname -s`

echo "Downloading JDK..."
if [ "$OS" == "Linux" ]; then
    if [ "$ARCH" == "arm64" ]; then
	curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_linux-aarch64_bin.tar.gz;
	tar xzf openjdk-17.0.1_macos-aarch64_bin.tar.gz;
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    else
	curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_linux-x64_bin.tar.gz;
	tar xzf openjdk-17.0.1_macos-x64_bin.tar.gz;
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    fi
fi
if [ "$OS" == "Darwin" ]; then
    if [ "$ARCH" == "arm64" ]; then
	curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_macos-aarch64_bin.tar.gz;
	tar xzf openjdk-17.0.1_macos-aarch64_bin.tar.gz;
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    else
	curl -O https://download.java.net/java/GA/jdk17.0.1/2a2082e5a09d4267845be086888add4f/12/GPL/openjdk-17.0.1_macos-x64_bin.tar.gz;
	tar xzf openjdk-17.0.1_macos-x64_bin.tar.gz;
	export JAVA_HOME=$CUR_DIR/jdk-17.0.1.jdk/Contents/Home;
    fi
fi

echo "Downloading Android SDK..."
if [ "$OS" == "Linux" ]; then
    curl -L -O https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip;
    unzip commandlinetools-linux-11076708_latest.zip;
fi
if [ "$OS" == "Darwin" ]; then
    curl -L -O https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip;
    unzip commandlinetools-mac-11076708_latest.zip;
fi

echo "Setting up Android SDK..."
export ANDROID_SDK_ROOT=$CUR_DIR/cmdline-tools
yes | ./cmdline-tools/bin/sdkmanager --install "cmdline-tools;latest" --sdk_root=$ANDROID_SDK_ROOT
export ANDROID_SDK_ROOT=$CUR_DIR/cmdline-tools/latest
export ANDROID_HOME=$CUR_DIR/cmdline-tools/latest
yes | ./cmdline-tools/bin/sdkmanager --install --licenses --sdk_root=$ANDROID_SDK_ROOT
yes | ./cmdline-tools/bin/sdkmanager "platforms;android-35" --sdk_root=$ANDROID_SDK_ROOT
echo "sdk.dir=$ANDROID_SDK_ROOT" > local.properties

echo "Building app..."
sh ./gradlew --no-daemon --stacktrace build

cp app/build/outputs/apk/debug/app-debug.apk .

echo "\n\nPlease check the file app-debug.apk\n\n"
if [ "$OS" == "Darwin" ]; then
    open .;
fi

exit
