Name 'dSS4win'
!define dSSVersion "1.2.0"
!include "MUI2.nsh"
!include InstallOptions.nsh
!include StrRep.nsh
!include ReplaceInFile.nsh

;modern interface configuration
!define MUI_HEADER_IMAGE
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
!define MUI_HEADERIMAGE_BITMAP "Company_Logos.bmp"

!define MUI_ABORTWARNING
!define MUI_PAGE_HEADER_TEXT "dSS4win ${dssVersion} Setup"
!define MUI_WELCOMEFINISHPAGE_BITMAP Company_Logos.bmp 

!define MUI_WELCOMEPAGE
!define MUI_WELCOMEPAGE_TITLE "dSS4win ${dssVersion} Setup"
!define MUI_WELCOMEPAGE_TEXT "This Setup will install and configure dSS4win on your computer"
!define MUI_STARTMENUPAGE
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_TEXT "The dSS server is now installed.$\nMake sure you generate an SSL certificate before the first use. $\n$\nThank you for installing dSS4win"

;pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_STARTMENU page_id $R0
!insertmacro MUI_PAGE_INSTFILES
Page custom CustomPageFunction ValidateCustom
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

outFile "Setup dSS4win - ${dssVersion}.exe"
Icon "digistrom.ico"
VIProductVersion "${dssVersion}.0"
VIAddVersionKey CompanyName "bbv Software Services AG"
RequestExecutionLevel admin ;necessary for proper removal of start menu items on Vista/Win7

installDir  "$PROGRAMFILES\dSS4win"

Function .onInit
  InitPluginsDir
  File /oname=$PLUGINSDIR\comport.ini "comport.ini"
FunctionEnd

Function CustomPageFunction
  !insertmacro INSTALLOPTIONS_DISPLAY "comport.ini"
FunctionEnd

Function ValidateCustom
  SetShellVarContext all
	
  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 1" "State"
  StrCmp $0 0 next1
    !insertmacro _ReplaceInFile "$INSTDIR\ds485d.bat" "@SERIAL_PORT@" "/dev/ttyS0"
next1:
  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 3" "State"
  StrCmp $0 0 next2
    !insertmacro _ReplaceInFile "$INSTDIR\ds485d.bat" "@SERIAL_PORT@" "/dev/ttyS1"
next2:
  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 4" "State"
  StrCmp $0 0 next3
    !insertmacro _ReplaceInFile "$INSTDIR\ds485d.bat" "@SERIAL_PORT@" "/dev/ttyS2"
next3:
  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 5" "State"
  StrCmp $0 0 next4
    !insertmacro _ReplaceInFile "$INSTDIR\ds485d.bat" "@SERIAL_PORT@" "/dev/ttyS3"
next4:
  Delete "$INSTDIR\ds485d.bat.old"
  SetShellVarContext all
FunctionEnd

!insertmacro MUI_LANGUAGE "English" ;NSIS needs this

section
  ;copy programm file and dlls
  setOutPath $INSTDIR
  File "*.dll"
  File "dss.exe"
  File "dss.bat"
  File "ds485d.exe"
  File "ds485d.bat"
  File "digistrom.ico"

  File "C:\cygwin\bin\cygwin1.dll"
  File "C:\cygwin\bin\cyggcc_s-1.dll"
  File "C:\cygwin\bin\cygstdc++-6.dll"
  File "C:\cygwin\bin\cygz.dll"
  File "C:\cygwin\bin\cygcrypto-0.9.8.dll"
  File "C:\cygwin\bin\cygssl-0.9.8.dll"
  File "C:\cygwin\usr\local\bin\cygboost_program_options-mt.dll"
  File "C:\cygwin\usr\local\bin\cygboost_system-mt.dll"
  File "C:\cygwin\usr\local\bin\cygboost_thread-mt.dll"
  File "C:\cygwin\usr\local\bin\cygboost_filesystem-mt.dll"

  WriteUninstaller $INSTDIR\uninstall.exe

  File /r "..\..\examples"
  setOutPath $INSTDIR\jslib
  File "..\..\jslib\*"
  
  ;test
  SetShellVarContext all
;  setOutPath "$LOCALAPPDATA"
  setOutPath "$APPDATA\digitalSTROM"
  File /r "DssData"

  ;write registry
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dSS4win" "DisplayName" "dSS4win"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dSS4win" "UninstallString" '"$INSTDIR\uninstall.exe"'
  
  ;create start menu items
  !insertmacro MUI_STARTMENU_WRITE_BEGIN page_id
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\digitalSTROM"
  CreateShortCut "$SMPROGRAMS\digitalSTROM\dSS4win.lnk" "$INSTDIR\dss.bat" "" "$INSTDIR\digistrom.ico"
  CreateShortCut "$SMPROGRAMS\digitalSTROM\DS485 Daemon.lnk" "$INSTDIR\ds485d.bat"
  CreateShortCut "$SMPROGRAMS\digitalSTROM\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
sectionEnd

Section "Uninstall"
  SetShellVarContext all
  ;delete registry entry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dSS4win"
  ;delete files and directories
  Delete $INSTDIR\digistrom.ico
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\ds485d.bat
  Delete $INSTDIR\ds485d.exe
  Delete $INSTDIR\dss.bat
  Delete $INSTDIR\dss.exe
  Delete $INSTDIR\*.dll
  RMDir  /r $INSTDIR\jslib
  RMDir  /r $INSTDIR\examples
  RMDir  $INSTDIR
  RMDir  /r "$SMPROGRAMS\digitalSTROM"
  ;RMDir  /r $LOCALAPPDATA\DssData
  SetShellVarContext all
  RMDir  /r $APPDATA\digitalSTROM
SectionEnd
