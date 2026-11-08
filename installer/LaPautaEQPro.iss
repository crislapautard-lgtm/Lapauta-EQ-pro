
#define MyAppName "La Pauta EQ Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "La Pauta"

[Setup]
AppId={{A73D8D12-5C19-4D89-9B25-6D5F8B6A1001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Common Files\VST3
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=LaPauta_EQ_Pro_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Uninstallable=yes

[Files]
Source: "PLUGIN\*"; DestDir: "{app}\La Pauta EQ Pro.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Desinstalar La Pauta EQ Pro"; Filename: "{uninstallexe}"

[Run]
Filename: "{cmd}"; Parameters: "/c echo La Pauta EQ Pro instalado. Reinicia REAPER para detectar el plugin."; Flags: postinstall skipifsilent
