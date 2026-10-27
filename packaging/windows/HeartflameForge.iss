#ifndef AppVersion
  #define AppVersion "1.3.0"
#endif
#ifndef StageDir
  #define StageDir "dist\HeartflameForge"
#endif
#ifndef RedistPath
  #define RedistPath "dist\VC_redist.x64.exe"
#endif
#ifndef OutputDir
  #define OutputDir "dist"
#endif
#define AppName "Heartflame Forge"

[Setup]
AppId={{4B2C2AE8-6975-4F39-9E3A-0A6D9042E4D1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Heartflame Forge Contributors
DefaultDirName={autopf}\Heartflame Forge
DefaultGroupName=Heartflame Forge
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir={#OutputDir}
OutputBaseFilename=HeartflameForge-Setup-x64-v{#AppVersion}
SetupIconFile={#SourcePath}\..\..\assets\heartflame-forge.ico
UninstallDisplayIcon={app}\HeartflameForge.exe
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}
VersionInfoDescription=Modular local asset inspection and modding workbench

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "VC_redist.x64.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#RedistPath}"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{autoprograms}\Heartflame Forge"; Filename: "{app}\HeartflameForge.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\Heartflame Forge"; Filename: "{app}\HeartflameForge.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{tmp}\VC_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime..."; Flags: waituntilterminated; Check: not IsVCRedistInstalled
Filename: "{app}\HeartflameForge.exe"; Description: "Launch Heartflame Forge"; Flags: postinstall nowait skipifsilent runasoriginaluser

[Code]
function IsVCRedistInstalled: Boolean;
var
  Installed: Cardinal;
begin
  Result := RegQueryDWordValue(HKLM64,
    'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
    'Installed', Installed) and (Installed = 1);
end;
