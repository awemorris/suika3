@echo off

set CURRENT_DIR=%~dp0

echo Downloading JDK...
curl -L -O https://aka.ms/download-jdk/microsoft-jdk-17.0.11-windows-x64.zip
call powershell -command "Expand-Archive microsoft-jdk-17.0.11-windows-x64.zip"
set JAVA_HOME=%CURRENT_DIR%microsoft-jdk-17.0.11-windows-x64\jdk-17.0.11+9

echo Downloading Android SDK...
curl -L -O https://dl.google.com/android/repository/commandlinetools-win-11076708_latest.zip
call powershell -command "Expand-Archive commandlinetools-win-11076708_latest.zip"
set ANDROID_SDK_ROOT=%CURRENT_DIR%commandlinetools-win-11076708_latest\cmdline-tools

echo Setting up Android SDK...
call commandlinetools-win-11076708_latest\cmdline-tools\bin\sdkmanager --install "cmdline-tools;latest" --sdk_root=%ANDROID_SDK_ROOT%
set ANDROID_SDK_ROOT=%CURRENT_DIR%commandlinetools-win-11076708_latest\cmdline-tools\latest
set ANDROID_HOME=%CURRENT_DIR%commandlinetools-win-11076708_latest\cmdline-tools\latest
call commandlinetools-win-11076708_latest\cmdline-tools\bin\sdkmanager --licenses --sdk_root=%ANDROID_SDK_ROOT%
call commandlinetools-win-11076708_latest\cmdline-tools\bin\sdkmanager "platforms;android-35"  --sdk_root=%ANDROID_SDK_ROOT%
echo "sdk.dir=%ANDROID_SDK_ROOT%" > local.properties

echo Building app...
call gradlew.bat --no-daemon --stacktrace build

copy app\build\outputs\apk\debug\app-debug.apk .

echo Please check the file app-debug.apk

exit
