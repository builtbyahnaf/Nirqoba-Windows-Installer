#ifndef StageDir
  #define StageDir "..\dist\package"
#endif

#define ProductName "Nirqoba Compiler"
#define ProductVersion "0.1.0"

[Setup]
AppId={{C20973E4-7E23-4AA2-9A2B-51F0D17FA9C0}
AppName={#ProductName}
AppVersion={#ProductVersion}
AppPublisher=Muhammad Tahmid Ahnaf
DefaultDirName={autopf}\Nirqoba
DefaultGroupName=Nirqoba
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=nirqoba-setup
SetupLogging=yes
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayName={#ProductName}
UninstallDisplayIcon={app}\bin\nirqoba.exe
ChangesEnvironment=no
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Dirs]
Name: "{app}\bin"
Name: "{app}\lib"
Name: "{app}\toolchain"
Name: "{app}\examples"
Name: "{app}\docs"

[Files]
Source: "{#StageDir}\bin\nirqoba.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#StageDir}\lib\nrqoba_runtime.lib"; DestDir: "{app}\lib"; Flags: ignoreversion
Source: "{#StageDir}\toolchain\toolchain.json"; DestDir: "{app}\toolchain"; Flags: ignoreversion
Source: "{#StageDir}\toolchain\versions.lock.json"; DestDir: "{app}\toolchain"; Flags: ignoreversion
Source: "{#StageDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\examples\*"; DestDir: "{app}\examples"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Nirqoba Command Line"; Filename: "{app}\bin\nirqoba.exe"; Parameters: "version"
Name: "{userdesktop}\Nirqoba Command Line"; Filename: "{app}\bin\nirqoba.exe"; Parameters: "version"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\nirqoba.exe"; Parameters: "version"; Description: "Show installed compiler version"; Flags: postinstall nowait skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\examples"
Type: filesandordirs; Name: "{app}\docs"
