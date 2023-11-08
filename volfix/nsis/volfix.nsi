!define OWNER "dfw"
!define PRODUCT "VolFix"
!define FULLID "${PRODUCT}"
!define FULLIDTEXT "${FULLID}"
!define MainEXE "volfix.exe"

XPStyle on

Name "${FULLID}"
Caption "${FULLIDTEXT} Setup"
OutFile "${FULLID}Setup.exe"

LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Hungarian.nlf"

Function .onInit

	;Language selection dialog

	Push ""
	Push ${LANG_ENGLISH}
	Push English
	Push ${LANG_HUNGARIAN}
	Push Magyar
	Push A ; A means auto count languages
	       ; for the auto count to work the first empty push (Push "") must remain
	LangDLL::LangDialog "Installer Language" "Please select the language of the installer"

	Pop $LANGUAGE
	StrCmp $LANGUAGE "cancel" 0 +2
		Abort
FunctionEnd

Function un.onInit

	;Language selection dialog

	Push ""
	Push ${LANG_ENGLISH}
	Push English
	Push ${LANG_HUNGARIAN}
	Push Magyar
	Push A ; A means auto count languages
	       ; for the auto count to work the first empty push (Push "") must remain
	LangDLL::LangDialog "Installer Language" "Please select the language of the installer"

	Pop $LANGUAGE
	StrCmp $LANGUAGE "cancel" 0 +2
		Abort
FunctionEnd

; Some default compiler settings (uncomment and change at will):
; SetCompress auto ; (can be off or force)
; SetDatablockOptimize on ; (can be off)
; CRCCheck on ; (can be off)
; AutoCloseWindow false ; (can be true for the window go away automatically at end)
; ShowInstDetails hide ; (can be show to have them shown, or nevershow to disable)
; SetDateSave off ; (can be on to have files restored to their orginal date)

InstallDir "$PROGRAMFILES\${FULLID}"
InstallDirRegKey HKEY_CURRENT_USER "SOFTWARE\${OWNER}\${FULLID}" ""
DirText "Select the directory to install ${FULLIDTEXT} in:"
AutoCloseWindow true

BrandingText "${FULLIDTEXT}"
ComponentText "This will install the ${FULLIDTEXT} on your computer. Select which optional things you want installed."

Section "${FULLIDTEXT}" ; (default section)
	SetOutPath "$INSTDIR"
	; add files / whatever that need to be installed here.
	WriteRegStr HKEY_CURRENT_USER "SOFTWARE\${OWNER}\${FULLID}\Installdir" "" "$INSTDIR"
	WriteRegStr HKEY_CURRENT_USER "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLIDTEXT}" "DisplayName" "${FULLIDTEXT} (remove only)"
	WriteRegStr HKEY_CURRENT_USER "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLIDTEXT}" "UninstallString" '"$INSTDIR\uninst.exe"'
	; write out uninstaller
	WriteUninstaller "$INSTDIR\uninst.exe"
	File "${MainEXE}"
	CreateDirectory "$SMPROGRAMS\${FULLIDTEXT}"
	CreateShortCut "$SMPROGRAMS\${FULLIDTEXT}\${FULLIDTEXT}.lnk" "$INSTDIR\${MainEXE}"
	CreateShortCut "$SMPROGRAMS\${FULLIDTEXT}\$(^UninstallCaption).lnk" "$INSTDIR\uninst.exe"
SectionEnd ; end of default section

; optional section
LangString CSCDesktop ${LANG_ENGLISH} "Create shortcut on desktop?"
LangString CSCDesktop ${LANG_HUNGARIAN} "Parancsikon létrehozása az asztalon?"
Section $(CSCDesktop)
	CreateShortCut "$DESKTOP\${FULLIDTEXT}.lnk" "$INSTDIR\${MainEXE}"
SectionEnd

; optional section
LangString CSCQuickLaunch ${LANG_ENGLISH} "Create shortcut on quicklaunch?"
LangString CSCQuickLaunch ${LANG_HUNGARIAN} "Parancsikon létrehozása a gyorsindítón?"
Section $(CSCQuickLaunch)
;"Create icon on quicklaunch?"
	CreateShortCut "$QUICKLAUNCH\${FULLIDTEXT}.lnk" "$INSTDIR\${MainEXE}"
SectionEnd

; optional section
LangString RunProgram ${LANG_ENGLISH} "Run the installed application?"
LangString RunProgram ${LANG_HUNGARIAN} "Futtassam a telepített programot?"
Section /o $(RunProgram)
    Exec "$INSTDIR\${MainEXE}"
SectionEnd

; begin uninstall settings/section
UninstallText "This will uninstall ${FULLIDTEXT} from your system"
AutoCloseWindow true

Section Uninstall
	; add delete commands to delete whatever files/registry keys/etc you installed here.
	Delete "$INSTDIR\${MainEXE}"
	Delete "$INSTDIR\uninst.exe"
	RMDir "$INSTDIR"
	Delete "$SMPROGRAMS\${FULLIDTEXT}\*.*"
	RMDir "$SMPROGRAMS\${FULLIDTEXT}"
	Delete "$DESKTOP\${FULLIDTEXT}.lnk"
	Delete "$QUICKLAUNCH\${FULLIDTEXT}.lnk"
	DeleteRegKey HKEY_CURRENT_USER "SOFTWARE\${OWNER}\${FULLID}"
	DeleteRegKey HKEY_CURRENT_USER "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${FULLIDTEXT}"
SectionEnd ; end of uninstall section

; eof