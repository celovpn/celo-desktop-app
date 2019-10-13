!include "MUI2.nsh"
!define MUI_ICON "celo.ico"
Name "Celo VPN"
OutFile CeloVPNInstaller.exe ; NsiDecompiler: generated value!
InstallColors 00FF00 000000
InstallDir 'C:\Program Files\CeloVPN'

!define celovpn_version 2.0.0
!define celovpn_name "Celo VPN"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
LangString MUI_TEXT_INSTALLING_TITLE  ${LANG_ENGLISH} "${celovpn_name}"
LangString MUI_TEXT_INSTALLING_SUBTITLE ${LANG_ENGLISH} "Installing..."
LangString MUI_TEXT_FINISH_SUBTITLE ${LANG_ENGLISH} "Finished"

;--------------------------------
;Version Information

VIProductVersion "${celovpn_version}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${celovpn_name}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${celovpn_name}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright Celo VPN"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${celovpn_name}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" ${celovpn_version}
  
Section main

SectionIn RO
       CreateDirectory $INSTDIR
       SetOutPath $INSTDIR
       # Stop and uninstall service in case it's running
       nsExec::Exec '$INSTDIR\celovpnservice.exe -t'
       Pop $0
       nsExec::Exec '$INSTDIR\celovpnservice.exe -u'
       Pop $0
       
       CreateDirectory $INSTDIR
       SetOutPath $INSTDIR
       File  Qt5Core.dll
       File  Qt5Gui.dll
       File  Qt5Network.dll
       File  Qt5Svg.dll
       File  Qt5Widgets.dll
       File  Qt5Xml.dll
       File  libeay32.dll
       File  vcredist_x86.exe
       File  ssleay32.dll
       File  celo.ico
       SetOutPath $INSTDIR\platforms
       File  platforms\qwindows.dll
       SetOutPath $INSTDIR
       File  "celovpn.exe"
       File  "celovpnservice.exe"
       SetOutPath $TEMP
       File  openvpn-celo-VPN.exe
       Push $0
       ExecWait '$OUTDIR\openvpn-celo-VPN.exe /S /SELECT_PATH=0 /SELECT_OPENVPNGUI=0 /SELECT_SHORTCUTS=0 /D=$INSTDIR\OpenVPN' $0
       IfErrors Label_0x19 Label_0x1A

  Label_0x19:
       MessageBox  MB_OK 'OpenVPN network card driver installation fails. Reinstall, please.' /SD IDOK

  Label_0x1A:
       Delete  $OUTDIR\openvpn-celo-VPN.exe
       ExecWait '$INSTDIR\vcredist_x86.exe /install /quiet /norestart'

       ; Stop and unnistall in case a previous build is installed
       nsExec::Exec '$INSTDIR\celovpnservice.exe -t'
       Pop $0
       nsExec::Exec '$INSTDIR\celovpnservice.exe -u'
       Pop $0
       nsExec::Exec '$INSTDIR\celovpnservice.exe -i'
       Pop $0
       nsExec::Exec '$INSTDIR\celovpnservice.exe -s'
       Pop $0

    # create the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    # create a shortcut named "new shortcut" in the start menu programs directory
    # point the new shortcut at the program uninstaller

    CreateShortCut  "$DESKTOP\celo VPN.lnk" "$INSTDIR\celovpn.exe"
    CreateDirectory "$SMPROGRAMS\celo VPN"
    CreateShortCut  "$SMPROGRAMS\celo VPN\celo VPN.lnk" "$INSTDIR\celovpn.exe"
    CreateShortCut  "$SMPROGRAMS\celo VPN\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    # Add uninstaller to registry for easy uninstallation
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "DisplayName" "${celovpn_name}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "DisplayIcon" "$INSTDIR\celo.ico"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "Publisher" "celo.net"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "DisplayVersion" "${celovpn_version} build 4"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN" \
            "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
	WriteRegStr HKLM "SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters" \
			"DisabledComponents" 0
SectionEnd

# uninstaller section start
Section "uninstall"

    Delete  "$DESKTOP\Celo VPN.lnk"
    Delete  "$SMPROGRAMS\Celo VPN\Celo VPN.lnk"
    Delete  "$SMPROGRAMS\Celo VPN\Uninstall.lnk"
    ExecWait '$INSTDIR\OpenVPN\Uninstall.exe /S'  $0
    RMDir /r $INSTDIR\*.*
    RMDir $INSTDIR
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CeloVPN"

# uninstaller section end
SectionEnd
