!include "MUI2.nsh"

Name "Wheel"
OutFile "wheel_installer.exe"
InstallDir $LOCALAPPDATA\wheel
InstallDirRegKey HKCU "Software\Wheel" ""
RequestExecutionLevel user

;--------------------------------

  !define MUI_ABORTWARNING
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
  !insertmacro MUI_LANGUAGE "Russian"

;--------------------------------

Section ""
  SetOutPath $INSTDIR      
{files}
  WriteRegStr HKCU "Software\Wheel" "" $INSTDIR  
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\Wheel"
  CreateShortcut "$SMPROGRAMS\Wheel\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$SMPROGRAMS\Wheel\Wheel.lnk" "$INSTDIR\desktop.exe" "" "$INSTDIR\desktop.exe" 0  
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\Wheel\*.*"
{delete}
  DeleteRegKey /ifempty HKCU "Software\Wheel"
SectionEnd