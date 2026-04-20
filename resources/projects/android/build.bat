@echo off

REM Check directory path.
echo "%~dp0" | findstr /R "[^ -~]" >nul
if %errorlevel% equ 0 (
    echo.
    echo  [ERROR] Path contains non-ASCII characters.
    echo   --^> Please move or rename the project.
    echo.
    echo  Current Path: %~dp0
    echo ===============================================================
    pause
    exit /b
)

REM Check directory path length.
for /f %%i in ('powershell -command "('%~dp0').Length"') do set PATH_LEN=%%i
if %PATH_LEN% geq 56 (
    echo.
    echo  [ERROR] Path is too long! (%PATH_LEN% characters^)
    echo  --^> Please move the project to a shorter path (e.g., C:\S3^).
    echo.
    echo  Current Path: %~dp0
    pause
    exit /b
)

echo Copying assets...
set TARGET_ASSETS_DIR=%~dp0app\src\main\assets
robocopy ..\.. "%TARGET_ASSETS_DIR%" /MIR /XD .vscode /XD misc /XF suika3.exe /XF assets.arc /NJH /NJS /NS /NC
if %errorlevel% GEQ 8 (
    echo [ERROR] Copy failed with errorlevel %errorlevel%
)

echo Setting up JDK...
if exist "microsoft-jdk-17.0.11-windows-x64" (
    echo JDK folder already exists. Skipping download and extract.
) else (
    if not exist "microsoft-jdk-17.0.11-windows-x64.zip" (
        echo Downloading JDK...
        curl -L -O https://aka.ms/download-jdk/microsoft-jdk-17.0.11-windows-x64.zip
    ) else (
        echo JDK zip already exists. Skipping download.
    )
    echo Extracting JDK...
    call powershell -command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive microsoft-jdk-17.0.11-windows-x64.zip"
)
set CURRENT_DIR=%~dp0
set JAVA_HOME=%CURRENT_DIR%microsoft-jdk-17.0.11-windows-x64\jdk-17.0.11+9

echo Setting up Android SDK...
if exist "commandlinetools-win-11076708_latest" (
    echo Android SDK folder already exists. Skipping download and extract.
) else (
    if not exist "commandlinetools-win-11076708_latest.zip" (
        echo Downloading Android SDK...
        curl -L -O https://dl.google.com/android/repository/commandlinetools-win-11076708_latest.zip
    ) else (
        echo Android SDK zip already exists. Skipping download.
    )
    echo Extracting Android SDK...
    call powershell -command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive commandlinetools-win-11076708_latest.zip"
)
set ANDROID_SDK_ROOT=%CURRENT_DIR%commandlinetools-win-11076708_latest\cmdline-tools

echo Configuring Android SDK...
set SDK_TEMP_ROOT=%CURRENT_DIR%android-sdk
call commandlinetools-win-11076708_latest\cmdline-tools\bin\sdkmanager --install "cmdline-tools;latest" --sdk_root="%SDK_TEMP_ROOT%"
set ANDROID_SDK_ROOT=%SDK_TEMP_ROOT%
set ANDROID_HOME=%SDK_TEMP_ROOT%
set NEW_SDKMANAGER=%ANDROID_SDK_ROOT%\cmdline-tools\latest\bin\sdkmanager.bat
call "%NEW_SDKMANAGER%" --licenses --sdk_root="%ANDROID_SDK_ROOT%"
call "%NEW_SDKMANAGER%" "platforms;android-35" "platform-tools" "build-tools;35.0.0" --sdk_root="%ANDROID_SDK_ROOT%"
echo "sdk.dir=%ANDROID_SDK_ROOT%" > local.properties

echo Building app...
call gradlew.bat --no-daemon --stacktrace build

set DISPLAY_DIR=%CURRENT_DIR:\=/%
echo.
echo Please check the APK files:
echo   Debug:   %CURRENT_DIR%app\build\outputs\apk\debug\app-debug.apk
echo   Release: %CURRENT_DIR%app\build\outputs\apk\release\app-release-unsigned.apk
echo.

echo.
set AAPT2_PATH=%ANDROID_HOME%\build-tools\35.0.0\aapt2.exe
for /f "usebackq delims=" %%A in (`%AAPT2_PATH% dump packagename %CURRENT_DIR%app\build\outputs\apk\debug\app-debug.apk`) do set PACKAGE_NAME=%%A
echo Package Name: %PACKAGE_NAME%
echo.
echo If you want to change the package name, edit applicationId in %CURRENT_DIR%app\build.gradle

set /p ANSWER="Do you want to install the APK to your Android device via ADB? (y/N): "
if /i "%ANSWER%"=="y" (
    echo.
    echo Uninstalling existing APK...
    call "%ANDROID_HOME%\platform-tools\adb.exe" uninstall %PACKAGE_NAME%

    echo Installing new APK...
    call %ANDROID_SDK_ROOT%\platform-tools\adb.exe install -r %CURRENT_DIR%app\build\outputs\apk\debug\app-debug.apk
    if %errorlevel% equ 0 (
        echo Starting app...
        call "%ANDROID_HOME%\platform-tools\adb.exe" shell am start -n %PACKAGE_NAME%/io.noctvm.playfield.engineandroid.MainActivity
    )
)

echo.

exit
