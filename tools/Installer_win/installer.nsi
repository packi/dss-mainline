Name 'dSS4win'
!include "MUI2.nsh"
!include InstallOptions.nsh
!include "XML.nsh"

;modern interface configuration
!define MUI_HEADER_IMAGE
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
!define MUI_HEADERIMAGE_BITMAP "BBV.bmp"

!define MUI_ABORTWARNING
!define MUI_PAGE_HEADER_TEXT "dSS4win Setup"
!define MUI_WELCOMEFINISHPAGE_BITMAP BBV.bmp

!define MUI_WELCOMEPAGE
!define MUI_WELCOMEPAGE_TITLE "dSS4win Setup"
!define MUI_WELCOMEPAGE_TEXT "This Setup will install and configure dSS4win on your computer"
!define MUI_STARTMENUPAGE
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_FINISHPAGE
!define MUI_FINISHPAGE_TEXT "Thank you for installing dSS4win"

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

outFile "Setup dSS4win.exe"
Icon "digistrom.ico"
VIProductVersion "0.8.0.0"
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
  ${xml::LoadFile} "$APPDATA\digitalSTROM\DssData\config.xml" $0
  ${xml::GotoPath} "/properties/property/property[1]/value" $0
	
  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 1" "State"
  StrCmp $0 0 +2
    ${xml::SetText} "/dev/ttyS0" $0

  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 3" "State"
  StrCmp $0 0 +2
    ${xml::SetText} "/dev/ttyS1" $0

  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 4" "State"
  StrCmp $0 0 +2
    ${xml::SetText} "/dev/ttyS2" $0

  ReadINIStr $0 "$PLUGINSDIR\comport.ini" "Field 5" "State"
  StrCmp $0 0 +2
    ${xml::SetText} "/dev/ttyS3" $0
  
  SetShellVarContext all
  ${xml::SaveFile} "$APPDATA\digitalSTROM\DssData\config.xml" $0
FunctionEnd

!insertmacro MUI_LANGUAGE "English" ;NSIS needs this

section
  ;copy programm file and dlls
  setOutPath $INSTDIR
  File "*.dll"
  File "dss.exe"
  WriteUninstaller $INSTDIR\uninstall.exe
  
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
  CreateShortCut "$SMPROGRAMS\digitalSTROM\dSS4win.lnk" "$INSTDIR\dss.exe"
  CreateShortCut "$SMPROGRAMS\digitalSTROM\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
sectionEnd

Section "Uninstall"
  SetShellVarContext all
  ;delete registry entry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dSS4win"
  ;delete files and directories
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\dss.exe
  Delete $INSTDIR\*.dll
  RMDir  $INSTDIR
  RMDir  /r "$SMPROGRAMS\digitalSTROM"
  ;RMDir  /r $LOCALAPPDATA\DssData
  SetShellVarContext all
  RMDir  /r $APPDATA\digitalSTROM
SectionEnd
