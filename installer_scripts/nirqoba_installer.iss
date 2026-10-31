; ============================================================================
; Nirqoba Compiler Windows Installer - Modern UI Edition
; Version 3.0 - Beautiful Aero-style Interface
; ============================================================================

[Setup]
; ==================== Basic Installer Information ====================
AppName=nirqoba Compiler
AppVersion=0.1.0
AppPublisher=Muhammad Tahmid Ahnaf
AppPublisherURL=https://freebuff.com
AppSupportURL=https://freebuff.com/support
AppComments=Modern Nirqoba Compiler Installation
DefaultDirName={pf}\nirqoba
DisableDirPage=yes
UninstallDeleteProgramPages=yes
UninstallDisplayIcon={app}\nirqoba.exe
UninstallDisplayName=nirqoba Compiler
UninstallLocationMode=select
UnrestartApp=yes
EnableAdminTouch=yes
RequestExecutionLevel admin

; ==================== Modern UI Options ====================
; Use modern, styled interface
MinVersionOnClient=0x06010000
; Enable visual styles (Windows 7+)
SolidInstall=no
UseCLIPageBitmap=no
UIFont=Tahoma
; Custom style - modern with search
StyleDir=C:\Program Files\Inno Setup 6\UI\Modern\

; ==================== Languages ====================
[Languages]
Name: "english"; Languages: english
Name: "german"; Languages: german
Name: "spanish"; Languages: spanish

; ==================== Directories ====================
[Dirs]
; Create these directories during installation
Dir: "{pf}\nirqoba"; Drv: auto; Permanentsubdir: nirqoba
Dir: "{pf}\nirqoba\bin"; Drv: auto
Dir: "{pf}\nirqoba\lib"; Drv: auto
Dir: "{pf}\nirqoba\examples"; Drv: auto

; ==================== Files ====================
; Main application binary
Source: "{src}\nirqoba.exe"; DestDir:{app}\bin; Flags: ignoreversion external
; Runtime library
Source: "{src}\*.dll"; DestDir:{app}\bin; Flags: ignoreversion external
; Documentation
Source: "{src}\nirqoba.chm"; DestDir:{app}\doc; Flags: ignoreversion external
; Examples
Source: "{src}\examples\*.nrqba"; DestDir:{app}\examples; Flags: recursesubdirs createallsubdirs

; ==================== File Associations ====================
[FileAssociations]
;nirqobaSource=nirqoba.exe "%1"
;Ext=nirqobaSource
;Assoc=yes

; ==================== Icons ====================
[Icons]
; Modern taskbar button overlay
Name: "{userdesktop}\nirqoba.lnk"; Filename: "{app}\bin\nirqoba.exe"; Tasks: desktopicon
Name: "{userstartmenu}\nirqoba.lnk"; Filename: "{app}\bin\nirqoba.exe"; Tasks: startmenuicon
Name: "{commonstartmenu}\nPrograms\nirqoba.lnk"; Filename: "{app}\bin\nirqoba.exe"; Tasks: startmenuicon
Name: "{computernirsicon}"; Filename: "{app}\bin\nirqoba.exe"

; ==================== Installation Tasks ====================
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut."; Flags: checked
Name: "startmenuicon"; Description: "Create a Start Menu shortcut."; Flags: checked
Name: "addtopath"; Description: "Add nirqoba to system PATH."; Flags: unchecked
Name: "installexamples"; Description: "Install example projects."; Flags: checked

; ==================== Registered Features ====================
[Registry]
; Register with auto-update support
Filename: "{app}\bin\nirqoba.exe"; Root: HKLM64; Subkey: "Software\Nirqoba\Registration"; ValueName: "Version"; ValueType: string; ValueData: "{#AppVersion}"; Flags: uninscopyifdoesntexist
; Add to PATH if task selected
Filename: "{app}\bin\nirqoba.exe"; Root: HKLM64; Subkey: "System\CurrentControlSet\Control\Session Manager\Environment"; ValueName: "PATH"; ValueType: expandable; ValueData: "{app}\bin"; Flags: uninscopyifdoesntexist

; ==================== UI Customization ====================
[UI]
; Hide the Welcome page for single-file installs
; Welcome page text and bitmap
WelcomeDescription={cm WelcomeDescription}
WelcomeIcon={src}\nirqoba.bmp
; Don't show the License page if no license file
; License page text and bitmap
LicenseText={cm LicenseText}
LicenseIcon={src}\nirqoba.bmp
; Show custom finished page
FinishText={cm FinishText}
; Modern finish options
FinishBitmap={src}\nirqoba.bmp
; Show the Uninstaller page
UninstPage="uninstall";

; ==================== Modern Welcome Page ====================
[WizardPage]
; Welcome page - shown by default
; Custom welcome text with rich formatting
WelcomeDescription=Welcome to the Nirqoba Compiler {#AppVersion} installation wizard.\n\nThis wizard will install the Nirqoba compiler and related files on your system.\n\nClick Next to continue.

; Show advanced options button
WelcomeShowPoweredBy=yes
WelcomePoweredByLabel=Powered by Inno Setup 6

; ==================== Modern License Page ====================
[WizardPage]
; License page
LicencePageShow=yes
LicenceText={cm LicenceText}
LicenceRichText=yes
; Modern license formatting - MIT license
LicenceHeader=MIT License
LicenceHeaderColor=000000
LicenceAllowAcceptPage=yes
LicenceAcceptPageShow=yes
LicenceAcceptPageRichText=yes
LicenceAcceptPageText=By clicking "I Agree", you agree to the terms of the MIT license.\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
LicenceAcceptPageAcceptBtnLabel=I Agree
LicenceAcceptPageDeclineBtnLabel=Decline

; ==================== Modern Welcome/Finish Customization ====================
[Messages]
WelcomeDescription="Welcome to the Nirqoba Compiler installation wizard.\n\nThis wizard will install the Nirqoba compiler and related files on your system.\n\nClick Next to continue."
LicenceText="The following software is released under the MIT license:\n\n{#LicenceHeader}\n\n{#LicenceTextFull}"
FinishText="The Nirqoba Compiler has been successfully installed.\n\nVersion: {#AppVersion}\n\nPublisher: {#AppPublisher}\n\nClick Finish to complete the installation."
ErrorInfoPageText="An error occurred during installation."
Abort="Abort"
Retry="Retry"
Continue="Continue"
Cancel="Cancel"
Uninstall="Uninstall"
Yes="Yes"
No="No"
OK="OK"
CancelPrompt="Are you sure you want to cancel the installation?"
RetryCancel="Retry or Cancel?"
MsgBoxCaption="nirqoba Installer"

; ==================== Progress Page ====================
[ProgressPage]
; Show progress during file copy
ProgressInfoTitle="Installing nirqoba Compiler"
ProgressInfoDetails="Copying files..."
ProgressText="Writing {#App}..."
; Custom progress bitmap
ProgressBitmap={src}\nirqoba.bmp
; Show status window
ProgressCancel=yes

; ==================== Uninstaller ====================
[UninstallApps]
; Uninstall nirqoba
nirqoba|nirqoba|{uninstallexe}|{app}|0

[Uninstall]
; Remove the Nirqoba directory
Type: dir; RmtDir: {app}; Recursive; 
; Remove desktop shortcut
Type: file; F: {userdesktop}\nirqoba.lnk; Rft; 
; Remove start menu shortcut  
Type: file; F: {userstartmenu}\nirqoba.lnk; Rft;
; Remove from PATH
Type: reg; Root: HKLM64; Subkey: "System\CurrentControlSet\Control\Session Manager\Environment"; ValueName: "PATH"; Flags: skipifdoesnotexist

; ==================== Customization - Modern Checkboxes ====================
[Tasks]
Name: "desktopicon"; Description: "&Create a desktop shortcut."; Flags: checked
Name: "startmenuicon"; Description: "Create a &Start Menu shortcut."; Flags: checked
Name: "addtopath"; Description: "&Add nirqoba to system PATH."; Flags: unchecked
Name: "installexamples"; Description: "Install &example projects."; Flags: checked

; ==================== Preprocessor Definitions ====================
[CustomMessages]
; These can be overridden at install time
WelcomeDescription="Welcome to the Nirqoba Compiler installation wizard.\n\nThis wizard will install the Nirqoba compiler and related files on your system.\n\nClick Next to continue."
LicenceText="The MIT License (MIT)\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
FinishText="The Nirqoba Compiler has been successfully installed.\n\nVersion: {#AppVersion}\n\nPublisher: {#AppPublisher}\n\nClick Finish to complete the installation."
ErrorInfoPageText="An error occurred during installation. Click Retry to try again, or Cancel to exit the installation program."
Abort="Abort"
Retry="Retry"
Continue="Continue"
Cancel="Cancel"
Uninstall="Uninstall"
Yes="Yes"
No="No"
OK="OK"
CancelPrompt="Are you sure you want to cancel the installation?"
RetryCancel="Retry or Cancel?"
MsgBoxCaption="nirqoba Installer"
InstallFailed="Installation failed."
SetupFailed="Setup failed."
;

; ============================================================================
; End of Inno Setup Script
; ============================================================================