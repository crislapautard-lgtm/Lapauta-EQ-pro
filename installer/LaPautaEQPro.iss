
name: La Pauta EQ Pro - Windows Installer

on:
  workflow_dispatch:
  push:
    branches:
      - main
    paths:
      - "CMakeLists.txt"
      - "Source/**"
      - ".github/workflows/installer.yml"

permissions:
  contents: read

jobs:
  build-installer:
    name: Build VST3 and Installer
    runs-on: windows-2022

    steps:
      - name: Download repository
        uses: actions/checkout@v4

      - name: Install CMake
        uses: lukka/get-cmake@latest

      - name: Configure JUCE project
        run: >
          cmake -B build
          -DCMAKE_BUILD_TYPE=Release
          -DJUCE_BUILD_EXAMPLES=OFF
          -DJUCE_BUILD_EXTRAS=OFF

      - name: Compile VST3
        run: cmake --build build --config Release --target LaPautaEQPro_VST3

      - name: Locate VST3
        shell: pwsh
        run: |
          $plugin = Get-ChildItem -Path build -Directory -Filter "*.vst3" -Recurse |
            Where-Object { $_.FullName -match "LaPautaEQPro" } |
            Select-Object -First 1

          if (-not $plugin) {
            throw "No se encontró el plugin VST3."
          }

          New-Item -ItemType Directory -Force -Path "package" | Out-Null
          Copy-Item $plugin.FullName "package\LaPauta EQ Pro.vst3" -Recurse

      - name: Install Inno Setup
        run: choco install innosetup --yes --no-progress

      - name: Create installer script
        shell: pwsh
        run: |
          @'
          #define MyAppName "La Pauta EQ Pro"
          #define MyAppVersion "1.0.0"
          #define MyPublisher "La Pauta"

          [Setup]
          AppId={{9D85C9C7-72F1-4D10-91F2-583104290001}
          AppName={#MyAppName}
          AppVersion={#MyAppVersion}
          AppPublisher={#MyPublisher}
          DefaultDirName={autopf}\Common Files\VST3
          DisableDirPage=yes
          DisableProgramGroupPage=yes
          OutputDir=installer
          OutputBaseFilename=LaPautaEQPro-Setup
          ArchitecturesInstallIn64BitMode=x64
          Compression=lzma2
          SolidCompression=yes
          WizardStyle=modern
          Uninstallable=yes

          [Files]
          Source: "package\La Pauta EQ Pro.vst3\*"; DestDir: "{app}\La Pauta EQ Pro.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

          [UninstallDelete]
          Type: filesandordirs; Name: "{app}\La Pauta EQ Pro.vst3"

          [Code]
          function InitializeSetup(): Boolean;
          begin
            Result := IsWin64;
            if not Result then
              MsgBox('Este instalador requiere Windows de 64 bits.', mbError, MB_OK);
          end;
          '@ | Set-Content -Path "installer.iss" -Encoding ASCII

      - name: Build EXE installer
        shell: pwsh
        run: |
          New-Item -ItemType Directory -Force -Path installer | Out-Null
          $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"

          if (!(Test-Path $iscc)) {
            throw "No se encontró Inno Setup."
          }

          & $iscc installer.iss

          if (!(Test-Path "installer\LaPautaEQPro-Setup.exe")) {
            throw "No se generó el instalador."
          }

      - name: Upload Windows Installer
        uses: actions/upload-artifact@v4
        with:
          name: LaPautaEQPro-Windows-Installer
          path: installer/LaPautaEQPro-Setup.exe
          if-no-files-found: error
