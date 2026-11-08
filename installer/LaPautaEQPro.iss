; ==========================================
; LaPautaEQPro - Instalador Completo
; Para PC de cartón - No pide Visual C++
; ==========================================

[Setup]
AppName=LaPauta EQ Pro
AppVersion=1.0.0
AppPublisher=LaPauta
AppPublisherURL=https://github.com/tu-usuario/LaPautaEQPro
DefaultDirName={autopf}\LaPautaEQPro
DefaultGroupName=LaPautaEQPro
DisableProgramGroupPage=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
OutputBaseFilename=LaPautaEQPro_Setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\LaPautaEQPro.vst3
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "Crear acceso directo en escritorio"; GroupDescription: "Accesos directos:"

[Files]
; --- ESTA ES LA RUTA QUE USA GITHUB ACTIONS ---
; Si tu build sale en otra carpeta, cambia solo esta línea
Source: "build\LaPautaEQPro_artefacts\Release\VST3\LaPautaEQPro.vst3"; DestDir: "{commoncf64}\VST3"; Flags: ignoreversion
Source: "build\LaPautaEQPro_artefacts\Release\VST3\LaPautaEQPro.vst3"; DestDir: "{commoncf32}\VST3"; Flags: ignoreversion

; --- Si tu .vst3 está en la misma carpeta que el .iss, usa estas 2 en vez de las de arriba ---
; Source: "LaPautaEQPro.vst3"; DestDir: "{commoncf64}\VST3"; Flags: ignoreversion
; Source: "LaPautaEQPro.vst3"; DestDir: "{commoncf32}\VST3"; Flags: ignoreversion

[Dirs]
Name: "{commoncf64}\VST3"
Name: "{commoncf32}\VST3"

[Icons]
Name: "{group}\Desinstalar LaPauta EQ Pro"; Filename: "{uninstallexe}"
Name: "{autodesktop}\LaPauta EQ Pro"; Filename: "{commoncf64}\VST3"; Tasks: desktopicon

[Run]
Filename: "{cmd}"; Parameters: "/c del /q ""{commoncf64}\VST3\LaPautaEQPro.vst3"""; Flags: runhidden; StatusMsg: "Limpiando versión vieja..."

[UninstallDelete]
Type: files; Name: "{commoncf64}\VST3\LaPautaEQPro.vst3"
Type: files; Name: "{commoncf32}\VST3\LaPautaEQPro.vst3"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
