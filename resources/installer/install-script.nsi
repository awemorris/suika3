!include "MUI2.nsh"
!define MUI_ICON "icon.ico"
!define MUI_UNICON "icon.ico"

Name "Playfield"
OutFile "playfield-installer.exe"
InstallDir "$APPDATA\Local\playfield"

RequestExecutionLevel user
SetCompressor /SOLID /FINAL lzma
SilentInstall normal

!insertmacro MUI_PAGE_WELCOME
Page directory
Page instfiles

Section "Install"
  SetOutPath "$INSTDIR"
  RMDir /r "$INSTDIR"
  File "playfield.exe"
  File "playfield-editor.exe"
  File "icon.ico"
  File "NOTICE"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\playfield"
  CreateShortcut "$SMPROGRAMS\playfield\Playfield.lnk" "$INSTDIR\playfiedl-editor.exe" ""
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "InstDir" '"$INSTDIR"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "DisplayName" "playfield"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "DisplayIcon" '"$INSTDIR\icon.ico"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "DisplayVersion" "1"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "Publisher" "The Playfield Project"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "Software\Classes\.pf" "" "playfield.script"
  WriteRegStr HKCU "Software\Classes\playfield.script" "" "Playfield Script"
  WriteRegStr HKCU "Software\Classes\playfield.script\DefaultIcon" "" "$INSTDIR\playfield-editor.exe"
  WriteRegStr HKCU "Software\Classes\playfield.script\Shell\open\command" "" '"$INSTDIR\playfield-editor.exe" "%1"'
  SetShellVarContext current
  CreateShortCut "$DESKTOP\Playfield.lnk" "$INSTDIR\playfield-editor.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\playfield.exe"
  Delete "$INSTDIR\playfield-editor.exe"
  Delete "$SMPROGRAMS\playfield\Playfield.lnk"
  RMDir "$SMPROGRAMS\playfield"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\playfield"
  Delete "$DESKTOP\Playfield.lnk"
  DeleteRegKey HKCU "Software\Classes\.pf"
  DeleteRegKey HKCU "Software\Classes\playfield.script"
  DeleteRegKey HKCU "Software\Classes\playfield.script\DefaultIcon"
  DeleteRegKey HKCU "Software\Classes\playfield.script\Shell\open\command"
SectionEnd

Function .OnInstSuccess
  Exec "$INSTDIR\playfield-editor.exe"
FunctionEnd

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Japanese"
