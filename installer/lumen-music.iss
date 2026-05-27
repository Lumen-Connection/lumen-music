; Inno Setup script para o Lumen Music.
; Pré-requisito: gerar o pacote em dist\LumenMusic\ (build Release + windeployqt).
; Compile com:  ISCC.exe installer\lumen-music.iss
; Gera:         dist\LumenMusic-v1.0.0-setup.exe

#define MyAppName "Lumen Music"
#define MyAppPublisher "Lumen Connection"
#define MyAppExeName "LumenMusic.exe"
#define MyAppSource "..\dist\LumenMusic"
; Single source of truth: read the version straight from the built exe's
; VERSIONINFO (embedded by CMake from PROJECT_VERSION).
#define MyAppVersion GetVersionNumbersString(MyAppSource + "\" + MyAppExeName)

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Lumen Music
DefaultGroupName=Lumen Music
UninstallDisplayIcon={app}\{#MyAppExeName}
OutputDir=..\dist
OutputBaseFilename=LumenMusic-v{#MyAppVersion}-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar um atalho na Área de Trabalho"; GroupDescription: "Atalhos adicionais:"

[Files]
; Copia toda a pasta empacotada (exe + DLLs do Qt + plugins).
Source: "{#MyAppSource}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\Lumen Music"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Desinstalar Lumen Music"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Lumen Music"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir o Lumen Music agora"; Flags: nowait postinstall skipifsilent
