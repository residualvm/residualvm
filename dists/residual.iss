; -- residual.iss --
; Inno Setup 5 Script for Residual.

[Setup]
AppName=Residual
AppVerName=Residual GIT Snapshot
AllowNoIcons=true
Compression=lzma
DefaultDirName={pf}\Residual
DefaultGroupName=Residual
SolidCompression=yes
OutputDir=userdocs:Residual_ISS5_Output
OutputBaseFilename=residual-win32
UninstallDisplayIcon={app}\residual.exe

[Files]
Source: "AUTHORS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "COPYING.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "COPYING.LGPL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "COPYRIGHT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "NEWS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "README-SDL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "residual.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "modern.zip"; DestDir: "{app}"; Flags: ignoreversion
Source: "residual-grim-patch.lab"; DestDir: "{app}"; Flags: ignoreversion
Source: "SDL.dll"; DestDir: "{app}"

[Icons]
Name: {group}\{cm:UninstallProgram, Residual}; Filename: {uninstallexe}
Name: {group}\residual; Filename: {app}\residual.exe; WorkingDir: {app}; Comment: residual; Flags: createonlyiffileexists
Name: {group}\Authors; Filename: {app}\AUTHORS.txt; WorkingDir: {app}; Comment: AUTHORS; Flags: createonlyiffileexists
Name: {group}\Copying; Filename: {app}\COPYING.txt; WorkingDir: {app}; Comment: COPYING; Flags: createonlyiffileexists
Name: {group}\Copying.LGPL; Filename: {app}\COPYING.LGPL.txt; WorkingDir: {app}; Comment: COPYING.LGPL; Flags: createonlyiffileexists
Name: {group}\Copyright.txt; Filename: {app}\COPYRIGHT.txt; WorkingDir: {app}; Comment: COPYRIGHT; Flags: createonlyiffileexists
Name: {group}\Readme; Filename: {app}\README.txt; WorkingDir: {app}; Comment: README; Flags: createonlyiffileexists
Name: {group}\News; Filename: {app}\NEWS.txt; WorkingDir: {app}; Comment: NEWS; Flags: createonlyiffileexists

[Run]
Filename: {app}\residual.exe; Description: Launch Residual; Flags: nowait skipifdoesntexist postinstall skipifsilent
