; Inno Setup script for Nirqoba compiler
; Version 2.0 - Complete Windows installer

[Setup]
; ==================== Basic Installer Information ====================
AppName=nirqoba Compiler
AppVersion=0.1.0
AppPublisher=Muhammad Tahmid Ahnaf
AppPublisherURL=https://freebuff.com
DefaultDirName={pf}\nirqoba
DisableDirPage=yes
UninstallDeleteProgramPages=yes
UninstallDisplayIcon={app}\nirqoba.exe
UninstallDisplayName=nirqoba Compiler
UninstallLocationMode=select
UnrestartApp=yes
EnableAdminTouch=yes
; Request administrative privileges for system-wide install
RequestExecutionLevel admin

; ==================== Installation Tasks ====================
TasksDescription=Tasks
Task descriptions=desktopicon, startmenuicon, addtopath
Task=desktopicon:Create a desktop shortcut.|"@DesktopIcon"
Task=startmenuicon:Create a Start Menu shortcut.|"@StartMenuIcon"
Task=addtopath:Add nirqoba to system PATH.|"@AddToPATH"

; ==================== Files ====================
; Source directory - set by /src switch or default
; Source: "{src}\nirqoba.exe"; DestDir:{app}; Flags: ignoreversion
; Source: "{src}\*.dll"; DestDir:{app}; Flags: ignoreversion

; ==================== Directories ====================
SubDir: {app}; Flags: createfolder

; ==================== File Associations ====================
; Associate .nrqba files with nirqoba.exe
FileType=nirqobaSource
Desc=nirqoba Source File
Ext=.nrqba
Assoc=yes
%FileType%: NSIS_Help

; ==================== Icons ====================
Name: "{userdesktop}\nirqoba.lnk"; Filename: "{src}\nirqoba.exe"
Name: "{userstartmenu}\nirqoba.lnk"; Filename: "{src}\nirqoba.exe"
Name: "{commonstartmenu}\nirqoba.lnk"; Filename: "{src}\nirqoba.exe"

; ==================== Post-Installation ====================
[Run]
Filename: "{app}\nirqoba.exe"; Parameters: "--version"; StatusMsg: "Displaying version information..."; Flags: nowait
Filename: "{app}\nirqoba.exe"; Parameters: "help"; StatusMsg: "Showing help..."; Flags: nowait

; ==================== Uninstallation ====================
[Uninstall]
FilesFile={app}\nirqoba.exe
RegistryKey=HKLM64\Software\Microsoft\Windows\CurrentVersion\Uninstall\nirqoba
RegistryValue=DisplayIcon
; Clean up PATH entry
Filename: "{sys}\reg.exe"; Parameters: "delete \"HKLM64\Software\Microsoft\Windows\CurrentVersion\Path\nirqoba\""; Flags: waitrunning
; Remove directory
UninstallDelete={app}\*

; ==================== Customization ==================
[Registry]
; Add to PATH if task selected
Filename: "{app}\nirqoba.exe"; Root: HKLM64; Subkey: "System\CurrentControlSet\Control\Session Manager\Environment"; ValueName: "Path"; ValueType: string; ValueData: "{app}"; Flags: uninscopyifdoesntexist

; ==================== Messages ==================
[Strings]
AppName=nirqoba Compiler
AppVersion=0.1.0
AppPublisher=Muhammad Tahmid Ahnaf
DesktopIcon="nirqoba Compiler"
StartMenuIcon="nirqoba Compiler"
AddToPATH="Add nirqoba to system PATH"
RemoveFromPATH="Remove nirqoba from system PATH"
Installation="Installation Complete"
Uninstallation="Uninstallation Complete"
Error="Error"
Warning="Warning"
ConfirmUninstall="Are you sure you want to uninstall the nirqoba compiler?"
SetupFailed="Setup failed."
;
; Help content
NSIS_Help="Usage: nirqoba <command> [options]\n\nCommands:\n  build <files>    Compile Nirqoba source code\n  run   <files>    Compile and run Nirqoba program\n  check <files>    Check source code for errors\n  new   <dir>      Create a new project from template\n  version          Print compiler version\n  help [command]   Show help for a command\n"