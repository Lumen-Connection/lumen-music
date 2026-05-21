; Inno Setup script para o Lumen Player.
; Pré-requisito: gerar o pacote em dist\LumenPlayer\ (build Release + windeployqt).
; Compile com:  ISCC.exe installer\lumen-player.iss
; Gera:         dist\LumenPlayer-v1.0.0-setup.exe

#define MyAppName "Lumen Player"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Lumen Connection"
#define MyAppExeName "LumenPlayer.exe"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Lumen Player
DefaultGroupName=Lumen Player
UninstallDisplayIcon={app}\{#MyAppExeName}
OutputDir=..\dist
OutputBaseFilename=LumenPlayer-v{#MyAppVersion}-setup
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
Source: "..\dist\LumenPlayer\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\Lumen Player"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Desinstalar Lumen Player"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Lumen Player"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir o Lumen Player agora"; Flags: nowait postinstall skipifsilent
