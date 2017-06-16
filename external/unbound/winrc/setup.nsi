# The NSIS (http://nsis.sourceforge.net) install script.
# This script is BSD licensed.

# use the default compression to help anti-virus in scanning us
#SetCompressor /solid /final lzma

!include LogicLib.nsh
!include MUI2.nsh

!define VERSION "0.0.0"
!define QUADVERSION "0.0.0.0"

outFile "unbound_setup_${VERSION}.exe"
Name "Unbound"

# default install directory
installDir "$PROGRAMFILES\Unbound"
installDirRegKey HKLM "Software\Unbound" "InstallLocation"
RequestExecutionLevel admin
#give credits to Nullsoft: BrandingText ""
VIAddVersionKey "ProductName" "Unbound"
VIAddVersionKey "CompanyName" "NLnet Labs"
VIAddVersionKey "FileDescription" "(un)install the unbound DNS resolver"
VIAddVersionKey "LegalCopyright" "Copyright 2009, NLnet Labs"
VIAddVersionKey "FileVersion" "${QUADVERSION}"
VIAddVersionKey "ProductVersion" "${QUADVERSION}"
VIProductVersion "${QUADVERSION}"

# Global Variables
Var StartMenuFolder

# use ReserveFile for files required before actual installation
# makes the installer start faster
#ReserveFile "System.dll"
#ReserveFile "NsExec.dll"

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install-nsis.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall-nsis.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "setup_top.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "setup_left.bmp"
!define MUI_ABORTWARNING
#!define MUI_FINISHPAGE_NOAUTOCLOSE  # so we can inspect install log.

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Unbound"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Unbound"
!insertmacro MUI_PAGE_STARTMENU UnboundStartMenu $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the uninstallation of Unbound.$\r$\n$\r$\nClick Next to continue."
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English" 

# default section, one per component, we have one component.
section "Unbound" SectionUnbound
	SectionIn RO  # cannot unselect this one
	# real work in postinstall
sectionEnd

section "Root anchor - DNSSEC" SectionRootKey
	# add estimated size for key (Kb)
	AddSize 2
sectionEnd

section "-hidden.postinstall"
	# copy files
	setOutPath $INSTDIR
	File "..\LICENSE"
	File "README.txt"
	File "..\unbound.exe"
	File "..\unbound-checkconf.exe"
	File "..\unbound-control.exe"
	File "..\unbound-host.exe"
	File "..\unbound-anchor.exe"
	File "..\unbound-service-install.exe"
	File "..\unbound-service-remove.exe"
	File "..\anchor-update.exe"
	File "unbound-control-setup.cmd"
	File "unbound-website.url"
	File "..\doc\example.conf"
	File "..\doc\Changelog"

	# Does service.conf already exist?
	IfFileExists "$INSTDIR\service.conf" 0 service_conf_not_found
	# if so, leave it be and place the shipped file under another name
	File /oname=service.conf.shipped "service.conf"
	goto end_service_conf_not_found
	# or, it is not there, place it and fill it.
	service_conf_not_found:
	File "service.conf"

	# Store Root Key choice
	SectionGetFlags ${SectionRootKey} $R0
	IntOp $R0 $R0 & ${SF_SELECTED}
	${If} $R0 == ${SF_SELECTED}
		ClearErrors
		FileOpen $R1 "$INSTDIR\service.conf" a
		IfErrors done_rk
		FileSeek $R1 0 END
		FileWrite $R1 "$\nserver: auto-trust-anchor-file: $\"$INSTDIR\root.key$\"$\n"
		FileClose $R1
	  done_rk:
		WriteRegStr HKLM "Software\Unbound" "RootAnchor" "$\"$INSTDIR\unbound-anchor.exe$\" -a $\"$INSTDIR\root.key$\" -c $\"$INSTDIR\icannbundle.pem$\""
	${Else}
		WriteRegStr HKLM "Software\Unbound" "RootAnchor" ""
	${EndIf}
	end_service_conf_not_found:

	# store installation folder
	WriteRegStr HKLM "Software\Unbound" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Unbound" "ConfigFile" "$INSTDIR\service.conf"
	WriteRegStr HKLM "Software\Unbound" "CronAction" ""
	WriteRegDWORD HKLM "Software\Unbound" "CronTime" 86400

	# uninstaller
	WriteUninstaller "uninst.exe"

	# register uninstaller
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "DisplayName" "Unbound"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "UninstallString" "$\"$INSTDIR\uninst.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "QuietUninstallString" "$\"$INSTDIR\uninst.exe$\" /S"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "NoModify" "1"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "NoRepair" "1"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "URLInfoAbout" "http://unbound.net"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound" "Publisher" "NLnet Labs"

	# start menu items
	!insertmacro MUI_STARTMENU_WRITE_BEGIN UnboundStartMenu
	CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\unbound.net website.lnk" "$INSTDIR\unbound-website.url" "" "$INSTDIR\unbound.exe" "" "" "" "Visit the unbound website"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninst.exe" "" "" "" "" "" "Uninstall unbound"
	!insertmacro MUI_STARTMENU_WRITE_END

	# install service entry
	nsExec::ExecToLog '"$INSTDIR\unbound-service-install.exe"'
	Pop $0 # return value/error/timeout
	# start unbound service
	nsExec::ExecToLog '"$INSTDIR\unbound-service-install.exe" start'
	Pop $0 # return value/error/timeout
sectionEnd

# set section descriptions
LangString DESC_unbound ${LANG_ENGLISH} "The base unbound DNS(SEC) validating caching resolver. $\r$\n$\r$\nStarted at boot from the Services control panel, logs to the Application Log, and the config file is its Program Files folder."
LangString DESC_rootkey ${LANG_ENGLISH} "Set up to use the DNSSEC root trust anchor. It is automatically updated. $\r$\n$\r$\nThis provides the main key that is used for security verification."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SectionUnbound} $(DESC_unbound)
  !insertmacro MUI_DESCRIPTION_TEXT ${SectionRootKey} $(DESC_rootkey)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# setup macros for uninstall functions.
!ifdef UN
!undef UN
!endif
!define UN "un."

# uninstaller section
section "un.Unbound"
	# stop unbound service
	nsExec::ExecToLog '"$INSTDIR\unbound-service-remove.exe" stop'
	Pop $0 # return value/error/timeout
	# uninstall service entry
	nsExec::ExecToLog '"$INSTDIR\unbound-service-remove.exe"'
	Pop $0 # return value/error/timeout
	# deregister uninstall
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Unbound"
	Delete "$INSTDIR\uninst.exe"   # delete self
	Delete "$INSTDIR\LICENSE"
	Delete "$INSTDIR\README.txt"
	Delete "$INSTDIR\unbound.exe"
	Delete "$INSTDIR\unbound-checkconf.exe"
	Delete "$INSTDIR\unbound-control.exe"
	Delete "$INSTDIR\unbound-host.exe"
	Delete "$INSTDIR\unbound-anchor.exe"
	Delete "$INSTDIR\unbound-service-install.exe"
	Delete "$INSTDIR\unbound-service-remove.exe"
	Delete "$INSTDIR\anchor-update.exe"
	Delete "$INSTDIR\unbound-control-setup.cmd"
	Delete "$INSTDIR\unbound-website.url"
	# keep the service.conf with potential local modifications
	#Delete "$INSTDIR\service.conf"
	Delete "$INSTDIR\service.conf.shipped"
	Delete "$INSTDIR\example.conf"
	Delete "$INSTDIR\Changelog"
	Delete "$INSTDIR\root.key"
	RMDir "$INSTDIR"

	# start menu items
	!insertmacro MUI_STARTMENU_GETFOLDER UnboundStartMenu $StartMenuFolder
	Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
	Delete "$SMPROGRAMS\$StartMenuFolder\unbound.net website.lnk"
	RMDir "$SMPROGRAMS\$StartMenuFolder"

	DeleteRegKey HKLM "Software\Unbound"
sectionEnd
