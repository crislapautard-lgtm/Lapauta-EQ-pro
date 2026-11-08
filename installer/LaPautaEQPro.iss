#define MyAppName "LaPauta EQ Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "LaPauta"
#define MyAppURL "https://github.com"

[Setup]
AppId={{A1B2C3D4-1234-4A5B-8C9D-123456789ABC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
OutputBaseFilename=LaPautaEQPro-Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "..\build\LaPautaEQPro_artefacts\Release\VST3\LaPautaEQPro.vst3\*"; DestDir: "{commoncf64}\VST3\LaPautaEQPro.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}"
