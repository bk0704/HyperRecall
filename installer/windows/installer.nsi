; HyperRecall NSIS Installer Script
; This script creates a Windows installer for HyperRecall

!ifndef VERSION
  !define VERSION "1.0.0"
!endif

!define APP_NAME "HyperRecall"
!define COMP_NAME "HyperRecall Project"
!define WEB_SITE "https://github.com/bk0704/HyperRecall"
!define COPYRIGHT "MIT License"
!define DESCRIPTION "Spaced Repetition Study Application"
!define INSTALLER_NAME "HyperRecall-Setup-${VERSION}.exe"
!define MAIN_APP_EXE "hyperrecall.exe"
!define INSTALL_TYPE "SetShellVarContext all"
!define REG_ROOT "HKLM"
!define REG_APP_PATH "Software\Microsoft\Windows\CurrentVersion\App Paths\${MAIN_APP_EXE}"
!define UNINSTALL_PATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

######################################################################

; Ensure version has 4 components for VIProductVersion (e.g., 1.0.0.0)
!searchparse /noerrors "${VERSION}" "" _V1 "." _V2 "." _V3 "." _V4
!ifndef _V4
  !define VI_VERSION "${VERSION}.0"
!else
  !define VI_VERSION "${VERSION}"
!endif

VIProductVersion "${VI_VERSION}"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "${COMP_NAME}"
VIAddVersionKey "LegalCopyright" "${COPYRIGHT}"
VIAddVersionKey "FileDescription" "${DESCRIPTION}"
VIAddVersionKey "FileVersion" "${VERSION}"

######################################################################

SetCompressor ZLIB
Name "${APP_NAME}"
Caption "${APP_NAME} Installer"
OutFile "installer\windows\${INSTALLER_NAME}"
BrandingText "${APP_NAME}"
XPStyle on
InstallDirRegKey "${REG_ROOT}" "${REG_APP_PATH}" ""
InstallDir "$PROGRAMFILES64\HyperRecall"

######################################################################

!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

; Icon files (optional - will use default if not found)
!ifdef APP_ICON
!define MUI_ICON "${APP_ICON}"
!define MUI_UNICON "${APP_ICON}"
!endif

!insertmacro MUI_PAGE_WELCOME

!ifdef LICENSE_TXT
!insertmacro MUI_PAGE_LICENSE "${LICENSE_TXT}"
!endif

!insertmacro MUI_PAGE_DIRECTORY

!ifdef REG_START_MENU
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${APP_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${REG_ROOT}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${UNINSTALL_PATH}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${REG_START_MENU}"
!insertmacro MUI_PAGE_STARTMENU Application $SM_Folder
!endif

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\${MAIN_APP_EXE}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

######################################################################

; Determine build output directory (supports both multi-config and single-config builds)
!ifexist "build\bin\${MAIN_APP_EXE}"
  !define BUILD_OUTPUT_DIR "build\bin"
!elseifexist "build\bin\Release\${MAIN_APP_EXE}"
  !define BUILD_OUTPUT_DIR "build\bin\Release"
!else
  !error "Could not find ${MAIN_APP_EXE}. Please build the project before creating the installer."
!endif

Section -MainProgram
${INSTALL_TYPE}
SetOverwrite ifnewer
SetOutPath "$INSTDIR"

; Copy main executable
File "${BUILD_OUTPUT_DIR}\${MAIN_APP_EXE}"

; Copy assets directory
SetOutPath "$INSTDIR\assets"
File /r "${BUILD_OUTPUT_DIR}\assets\*.*"

; Copy runtime dependencies (DLLs from vcpkg)
SetOutPath "$INSTDIR"
File /nonfatal "vcpkg\installed\x64-windows\bin\*.dll"

; Copy documentation
File "README.md"
File "LICENSE"

SectionEnd

######################################################################

Section -Icons_Reg
SetOutPath "$INSTDIR"
WriteUninstaller "$INSTDIR\uninstall.exe"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
CreateDirectory "$SMPROGRAMS\$SM_Folder"
CreateShortCut "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$SMPROGRAMS\$SM_Folder\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
!insertmacro MUI_STARTMENU_WRITE_END
!endif

!ifndef REG_START_MENU
CreateDirectory "$SMPROGRAMS\${APP_NAME}"
CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${MAIN_APP_EXE}"
CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
!endif

WriteRegStr ${REG_ROOT} "${REG_APP_PATH}" "" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayName" "${APP_NAME}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "UninstallString" "$INSTDIR\uninstall.exe"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayIcon" "$INSTDIR\${MAIN_APP_EXE}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "DisplayVersion" "${VERSION}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "Publisher" "${COMP_NAME}"
WriteRegStr ${REG_ROOT} "${UNINSTALL_PATH}" "URLInfoAbout" "${WEB_SITE}"

SectionEnd

######################################################################

Section Uninstall
${INSTALL_TYPE}

; Remove shortcuts
Delete "$DESKTOP\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\*.*"
RMDir "$SMPROGRAMS\${APP_NAME}"

!ifdef REG_START_MENU
!insertmacro MUI_STARTMENU_GETFOLDER "Application" $SM_Folder
Delete "$SMPROGRAMS\$SM_Folder\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\$SM_Folder\Uninstall ${APP_NAME}.lnk"
RMDir "$SMPROGRAMS\$SM_Folder"
!endif

; Remove application files
Delete "$INSTDIR\${MAIN_APP_EXE}"
Delete "$INSTDIR\*.dll"
Delete "$INSTDIR\README.md"
Delete "$INSTDIR\LICENSE"
Delete "$INSTDIR\uninstall.exe"

; Remove assets
RMDir /r "$INSTDIR\assets"

; Remove installation directory if empty
RMDir "$INSTDIR"

; Remove registry keys
DeleteRegKey ${REG_ROOT} "${REG_APP_PATH}"
DeleteRegKey ${REG_ROOT} "${UNINSTALL_PATH}"

SectionEnd

######################################################################
