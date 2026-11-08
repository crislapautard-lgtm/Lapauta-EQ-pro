#define MyAppName "La Pauta EQ Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "La Pauta"

[Setup]
AppId={{A1B2C3D4-1111-2222-3333-444455556666}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\installer-output
OutputBaseFilename=LaPautaEQPro-Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin

[Files]
Source: "..\build\LaPautaEQPro.vst3"; DestDir: "{commoncf}\VST3"; Flags: ignoreversion
