<div align="center">

# 🔶 Lumen Music

**Um player de música de desktop, leve e elegante feito em C++ com Qt.**

Biblioteca local, playlists com capa, fila de reprodução, histórico de reprodução e temas personalizáveis.

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/Windows-x64-0078D6?logo=windows&logoColor=white)
![SQLite](https://img.shields.io/badge/SQLite-local-003B57?logo=sqlite&logoColor=white)

</div>

---

## Sobre

O **Lumen Music** é um reprodutor de áudio local com foco em uma experiência limpa e fluida.
Você importa seus arquivos, organiza em playlists, curte faixas e controla tudo por uma
interface escura e minimalista. Sem nuvem, sem cadastro, tudo guardado localmente.

## Funcionalidades

### Biblioteca e organização
- **Importar músicas** de arquivos de áudio locais
- **Playlists** — criar, renomear e excluir
- **Capa de playlist** com gradiente de cores **ou imagem** (com zoom em lightbox)
- **Reordenar faixas** dentro da playlist **arrastando e soltando** (ordem persistida)
- **Curtidas** como uma coleção própria, com botão de play
- **Editar** nome da música e do artista, e **excluir** faixas
- Clicar na tag da playlist leva direto para ela

### Reprodução
- **Player completo**: play/pause, anterior/próxima, shuffle, repeat, volume e mudo
- **Fila estilo Spotify**: adicionar à fila, visualizar e remover; o que está na fila
  toca antes de continuar o contexto
- **Reprodução por contexto** — cada playlist, as curtidas e a biblioteca tocam dentro
  de si mesmas (o "próxima/anterior" respeita de onde você começou)
- **Tocadas recentemente** — histórico baseado no que foi realmente reproduzido

### Aparência
- **Temas**: Lumen (padrão), Vinil Quente, Oceano, Floresta, Roxo Noturno e Cinza Moderno
- Interface escura, minimalista e responsiva

## Tecnologias

- **Linguagem:** C++17
- **Framework:** Qt 6 (Widgets, Multimedia, SQL)
- **Build:** CMake + Ninja + MinGW
- **Persistência:** SQLite (banco local)

## Compilando o projeto

### Pré-requisitos
- [Qt 6.11](https://www.qt.io/download) com o kit **MinGW 64-bit** e os módulos
  **Multimedia** e **SQL**

### Opção A — Qt Creator (mais simples)
1. Abra o arquivo `CMakeLists.txt` no Qt Creator
2. Selecione o kit **Desktop Qt 6.x MinGW 64-bit**
3. Clique em **Run**

### Opção B — Linha de comando (PowerShell)
O projeto usa **CMake** com **presets** (`CMakePresets.json`). Ajuste os caminhos
do Qt/MinGW dentro dos presets para os da sua instalação, se forem diferentes.

```powershell
# Coloque o CMake/Ninja/MinGW do Qt no PATH (ajuste para a sua versão):
$env:Path = "D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.11.0\mingw_64\bin;" + $env:Path

cmake --preset release        # configura
cmake --build --preset release  # compila
```

> **Caminho com acentos/espaços:** as ferramentas do MinGW (moc/windres) não
> lidam bem com diretórios de build contendo caracteres não-ASCII (ex.: "Área de
> Trabalho"). Por isso os presets geram o build em
> `%LOCALAPPDATA%\LumenMusic-build\<preset>` (caminho ASCII), e não dentro do
> repositório. O executável `LumenMusic.exe` fica nessa pasta.

Para compilar sem presets, aponte o diretório de build para um caminho sem acentos:

```powershell
cmake -S . -B "$env:LOCALAPPDATA\LumenMusic-build\release" -G Ninja `
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.11.0/mingw_64"
cmake --build "$env:LOCALAPPDATA\LumenMusic-build\release"
```

## Gerando um pacote distribuível

O empacotamento é automático: o `install` do CMake chama o `windeployqt`, então
um `cmake --install` ou um `cpack` já produzem uma pasta/zip que roda num PC
**sem o Qt instalado**.

### Pasta autossuficiente (`cmake --install`)

```powershell
$build = "$env:LOCALAPPDATA\LumenMusic-build\release"
cmake --install $build --prefix dist\LumenMusic
```

Gera `dist\LumenMusic\` com `LumenMusic.exe`, `yt-dlp.exe`, as DLLs do Qt e os
plugins. Teste rodando `dist\LumenMusic\LumenMusic.exe` e reproduzindo uma música
(valida os plugins de multimídia e o driver SQLite).

### ZIP de release (`cpack`)

```powershell
$build = "$env:LOCALAPPDATA\LumenMusic-build\release"
cpack --config "$build\CPackConfig.cmake" -G ZIP -B dist
```

Gera `dist\LumenMusic-v<versão>-win64.zip`. A versão vem de uma única fonte — o
`project(... VERSION ...)` no `CMakeLists.txt` —, que também é embutida no
`.exe` (VERSIONINFO) e lida automaticamente pelo instalador Inno Setup.

### Instalador (Inno Setup, opcional)

Com a pasta `dist\LumenMusic` pronta, compile `installer\lumen-music.iss` com o
`ISCC.exe`. A versão é lida direto do `.exe`, então não há número para manter à mão.

## Licença

Projeto feito pela [Lumen Connection](https://lumenconnection.com.br), distribuído sob a licença
[AGPL-3.0](LICENSE)

---

<div align="center">
Feito com C++ e Qt • <strong>🄯 Lumen Connection</strong>
</div>
