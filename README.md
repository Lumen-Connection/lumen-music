<div align="center">

# 🔶 Lumen Player

**Um player de música de desktop, leve e elegante — feito em C++ com Qt.**

Biblioteca local, playlists com capa, fila de reprodução, histórico de reprodução e temas personalizáveis.

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/Windows-x64-0078D6?logo=windows&logoColor=white)
![SQLite](https://img.shields.io/badge/SQLite-local-003B57?logo=sqlite&logoColor=white)

</div>

---

## Sobre

O **Lumen Player** é um reprodutor de áudio local com foco em uma experiência limpa e fluida.
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
- **Build:** qmake + MinGW
- **Persistência:** SQLite (banco local)

## Compilando o projeto

### Pré-requisitos
- [Qt 6.11](https://www.qt.io/download) com o kit **MinGW 64-bit** e os módulos
  **Multimedia** e **SQL**

### Opção A — Qt Creator (mais simples)
1. Abra o arquivo `VinilPlayer.pro` no Qt Creator
2. Selecione o kit **Desktop Qt 6.x MinGW 64-bit**
3. Clique em **Run**

### Opção B — Linha de comando (PowerShell)
Ajuste os caminhos do Qt/MinGW para os da sua instalação:

```powershell
$env:Path = "D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.11.0\mingw_64\bin;" + $env:Path

qmake -o Makefile VinilPlayer.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile.Release
```

O executável é gerado em `release/`.

## Gerando um pacote distribuível

Para rodar em um PC sem o Qt instalado, empacote com o `windeployqt`:

```powershell
$dist = "dist\LumenPlayer"
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item release\VinilPlayer.exe "$dist\LumenPlayer.exe"
windeployqt --release --compiler-runtime --no-translations "$dist\LumenPlayer.exe"
Compress-Archive -Path "$dist\*" -DestinationPath "dist\LumenPlayer-win64.zip" -Force
```

Teste rodando `dist\LumenPlayer.exe` e reproduzindo uma música (valida os plugins de
multimídia e o driver SQLite).

## Estrutura do projeto

```
src/
├── main.cpp              # Ponto de entrada, tema e ícone do app
├── mainwindow.*          # Janela principal, navegação e barra lateral
├── database/             # Camada de persistência (SQLite)
├── player/
│   ├── playerbar.*       # Controles de reprodução e fila
│   └── trackmodel.*      # Modelo de faixas/playlists
├── pages/                # Telas: início, playlists, detalhe, curtidas, fila, adicionar
└── widgets/              # Tema, logo, lista reordenável, slider e utilitários
```

## Roadmap

- [ ] Equalizador gráfico
- [ ] Escolher uma pasta e ler as músicas automaticamente
- [ ] Card "tocando agora" em destaque durante a reprodução
- [ ] Aviso ao adicionar uma música duplicada

## Licença

Projeto feito por Lumen Connection(Conheça mais em lumenconnection.com.br), distribuído sob a licença
[MIT](LICENSE) — Use, modifique e distribua à vontade.

---

<div align="center">
Feito com C++ e Qt • <strong>Lumen Connection</strong>
</div>
