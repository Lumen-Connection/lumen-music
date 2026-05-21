#include "addmusicpage.h"
#include <QLabel>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QScrollArea>
#include <QFrame>
#include <QUuid>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QProcessEnvironment>
#include <algorithm>

AddMusicPage::AddMusicPage(TrackModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setAcceptDrops(true);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(12);

    auto *backBtn = new QPushButton("←");
    backBtn->setFixedSize(34, 34);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QString(
        "QPushButton { background: rgba(255,255,255,0.05); color: %1; border: none; border-radius: 17px; font-size: 16px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.1); }"
    ).arg(Theme::text().name()));
    connect(backBtn, &QPushButton::clicked, this, &AddMusicPage::navigateBack);
    layout->addWidget(backBtn, 0, Qt::AlignLeft);

    auto *title = new QLabel("Inserção de Músicas");
    title->setFont(Theme::titleFont(28));
    title->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::text().name()));
    layout->addWidget(title);

    auto *subtitle = new QLabel("Arraste seus arquivos de áudio ou clique para selecionar");
    subtitle->setFont(Theme::bodyFont(13));
    subtitle->setStyleSheet(QString("color: %1; background: transparent; padding-bottom: 12px;").arg(Theme::textSoft().name()));
    layout->addWidget(subtitle);

    auto *urlCard = new QWidget();
    urlCard->setObjectName("urlCard");
    urlCard->setStyleSheet(QString("QWidget#urlCard { background: %1; border-radius: 12px; }")
        .arg(Theme::card().name()));

    auto *urlCardLayout = new QVBoxLayout(urlCard);
    urlCardLayout->setContentsMargins(16, 14, 16, 14);
    urlCardLayout->setSpacing(8);

    auto *urlHeader = new QLabel("\uE774  Download via YouTube");
    urlHeader->setFont(Theme::bodyFont(12));
    urlHeader->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;")
        .arg(Theme::accent().name()));
    urlCardLayout->addWidget(urlHeader);

    auto *urlRow = new QHBoxLayout();
    urlRow->setSpacing(8);

    m_urlEdit = new QLineEdit();
    m_urlEdit->setPlaceholderText("https://www.youtube.com/watch?v=...");
    m_urlEdit->setFont(Theme::bodyFont(12));
    m_urlEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: %1; color: %2; border: 1px solid %3;
            border-radius: 8px; padding: 8px 12px;
        }
        QLineEdit:focus { border-color: %4; }
    )").arg(Theme::surface().name(), Theme::text().name(),
            Theme::border().name(), Theme::accent().name()));
    urlRow->addWidget(m_urlEdit, 1);

    m_downloadBtn = new QPushButton("Baixar");
    m_downloadBtn->setFixedSize(80, 36);
    m_downloadBtn->setCursor(Qt::PointingHandCursor);
    m_downloadBtn->setFont(Theme::bodyFont(12));
    m_downloadBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 8px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }"
        "QPushButton:disabled { background: %4; color: %5; }"
    ).arg(Theme::accent().name(), Theme::bg().name(),
          Theme::accent().lighter(110).name(),
          Theme::border().name(), Theme::textMuted().name()));
    connect(m_downloadBtn, &QPushButton::clicked, this, &AddMusicPage::startDownload);
    connect(m_urlEdit, &QLineEdit::returnPressed,  this, &AddMusicPage::startDownload);
    urlRow->addWidget(m_downloadBtn);

    urlCardLayout->addLayout(urlRow);

    m_downloadStatus = new QLabel();
    m_downloadStatus->setFont(Theme::bodyFont(11));
    m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;")
        .arg(Theme::textMuted().name()));
    m_downloadStatus->hide();
    urlCardLayout->addWidget(m_downloadStatus);

    layout->addWidget(urlCard);

    // Drop zone
    m_dropZone = new QWidget();
    m_dropZone->setFixedHeight(180);
    m_dropZone->setCursor(Qt::PointingHandCursor);
    m_dropZone->setStyleSheet(QString(
        "QWidget { border: 2px dashed %1; border-radius: 16px; background: rgba(255,255,255,0.01); }"
    ).arg(Theme::border().name()));

    auto *dropLayout = new QVBoxLayout(m_dropZone);
    dropLayout->setAlignment(Qt::AlignCenter);
    dropLayout->setSpacing(10);

    auto *uploadIcon = new QLabel("⬆");
    uploadIcon->setFont(QFont("Segoe UI", 32));
    uploadIcon->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    uploadIcon->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(uploadIcon);

    m_dropLabel = new QLabel("Clique ou arraste arquivos de áudio");
    m_dropLabel->setFont(Theme::bodyFont(14));
    m_dropLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
    m_dropLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(m_dropLabel);

    auto *formatLabel = new QLabel("Somente arquivos OPUS");
    formatLabel->setFont(Theme::bodyFont(11));
    formatLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    formatLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(formatLabel);

    // Make drop zone clickable
    m_dropZone->installEventFilter(this);
    // We'll handle click via mouse press
    layout->addWidget(m_dropZone);

    // Browse button
    auto *browseBtn = new QPushButton("Procurar Arquivos");
    browseBtn->setFixedSize(180, 40);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFont(Theme::bodyFont(13));
    browseBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 20px; font-weight: 600; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::card().name(), Theme::textSoft().name(), Theme::border().name(), Theme::cardHover().name()));
    connect(browseBtn, &QPushButton::clicked, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "Selecionar Músicas", QString(),
            "Opus (*.opus)");
        if (!files.isEmpty()) processFiles(files);
    });
    layout->addWidget(browseBtn, 0, Qt::AlignLeft);
    layout->addSpacing(8);

    // File list container
    m_fileListContainer = new QWidget();
    m_fileListContainer->setStyleSheet("background: transparent;");
    m_fileListLayout = new QVBoxLayout(m_fileListContainer);
    m_fileListLayout->setContentsMargins(0, 0, 0, 0);
    m_fileListLayout->setSpacing(6);
    m_fileListContainer->hide();
    layout->addWidget(m_fileListContainer);

    // Folder section
    m_folderSection = new QWidget();
    m_folderSection->setStyleSheet("background: transparent;");
    auto *folderLayout = new QVBoxLayout(m_folderSection);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLayout->setSpacing(8);

    auto *folderLabel = new QLabel("PLAYLIST DE DESTINO");
    folderLabel->setFont(Theme::bodyFont(11));
    folderLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold; letter-spacing: 1px;").arg(Theme::textSoft().name()));
    folderLayout->addWidget(folderLabel);

    auto *folderRow = new QHBoxLayout();
    folderRow->setSpacing(8);

    m_folderCombo = new QComboBox();
    m_folderCombo->setFont(Theme::bodyFont(13));
    m_folderCombo->setMinimumWidth(200);
    m_folderCombo->setStyleSheet(QString(R"(
        QComboBox {
            background: %1; color: %2; border: 1px solid %3;
            border-radius: 8px; padding: 10px 14px;
        }
        QComboBox::drop-down { border: none; width: 30px; }
        QComboBox::down-arrow { image: none; border: none; }
        QComboBox QAbstractItemView {
            background: %4; color: %2; border: 1px solid %3;
            selection-background-color: %5;
        }
    )").arg(Theme::surface().name(), Theme::text().name(), Theme::border().name(),
            Theme::card().name(), Theme::cardHover().name()));
    folderRow->addWidget(m_folderCombo);

    m_newFolderEdit = new QLineEdit();
    m_newFolderEdit->setPlaceholderText("Nova playlist...");
    m_newFolderEdit->setFont(Theme::bodyFont(13));
    m_newFolderEdit->setMinimumWidth(160);
    m_newFolderEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: %1; color: %2; border: 1px solid %3;
            border-radius: 8px; padding: 10px 14px;
        }
        QLineEdit:focus { border-color: %4; }
    )").arg(Theme::surface().name(), Theme::text().name(), Theme::border().name(), Theme::accent().name()));
    folderRow->addWidget(m_newFolderEdit);
    folderRow->addStretch();

    folderLayout->addLayout(folderRow);

    m_addBtn = new QPushButton("Adicionar à Biblioteca");
    m_addBtn->setFont(Theme::bodyFont(14));
    m_addBtn->setFixedHeight(46);
    m_addBtn->setMinimumWidth(220);
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 23px; font-weight: bold; padding: 0 32px; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::accent().name(), Theme::bg().name(), Theme::accent().lighter(110).name()));
    connect(m_addBtn, &QPushButton::clicked, this, &AddMusicPage::addAllToLibrary);
    folderLayout->addSpacing(4);
    folderLayout->addWidget(m_addBtn, 0, Qt::AlignLeft);

    m_folderSection->hide();
    layout->addWidget(m_folderSection);
    layout->addStretch();

    scroll->setWidget(content);
    outerLayout->addWidget(scroll);
}

void AddMusicPage::refresh() {
    // Remove downloaded files that were never committed to library
    QString downloadsDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/downloads";
    for (auto &pf : m_pendingFiles) {
        if (pf.filePath.startsWith(downloadsDir)) {
            QFile::remove(pf.filePath);
        }
    }
    m_pendingFiles.clear();
    refreshFileList();

    if (m_downloadStatus) {
        m_downloadStatus->hide();
        m_downloadStatus->clear();
    }
    if (m_urlEdit) m_urlEdit->clear();

    m_folderCombo->clear();
    m_folderCombo->addItem("Sem playlist");
    auto folders = m_model->folders();
    for (auto &f : folders) {
        m_folderCombo->addItem(f.name);
    }
}

void AddMusicPage::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        m_isDragOver = true;
        m_dropZone->setStyleSheet(QString(
            "QWidget { border: 2px dashed %1; border-radius: 16px; background: %2; }"
        ).arg(Theme::accent().name(), Theme::accentRgba(0.12)));
        m_dropLabel->setText("Solte os arquivos aqui");
    }
}

void AddMusicPage::dropEvent(QDropEvent *event) {
    m_isDragOver = false;
    m_dropZone->setStyleSheet(QString(
        "QWidget { border: 2px dashed %1; border-radius: 16px; background: rgba(255,255,255,0.01); }"
    ).arg(Theme::border().name()));
    m_dropLabel->setText("Clique ou arraste arquivos de áudio");

    QStringList paths;
    for (auto &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) paths.append(url.toLocalFile());
    }
    if (!paths.isEmpty()) processFiles(paths);
}

void AddMusicPage::dragLeaveEvent(QDragLeaveEvent *) {
    m_isDragOver = false;
    m_dropZone->setStyleSheet(QString(
        "QWidget { border: 2px dashed %1; border-radius: 16px; background: rgba(255,255,255,0.01); }"
    ).arg(Theme::border().name()));
    m_dropLabel->setText("Clique ou arraste arquivos de áudio");
}

void AddMusicPage::processFiles(const QStringList &paths) {
    QStringList audioExts = {"opus"};

    for (auto &path : paths) {
        QFileInfo fi(path);
        if (!audioExts.contains(fi.suffix().toLower())) continue;

        PendingFile pf;
        pf.id = QUuid::createUuid().toString(QUuid::Id128).left(10);
        pf.filePath = path;
        pf.fileSize = fi.size();
        pf.palette = Theme::randomPalette();

        // Try to parse "Artist - Title" from filename
        QString baseName = fi.completeBaseName();
        QStringList parts = baseName.split(QRegularExpression("\\s*[-–—]\\s*"));
        if (parts.size() >= 2) {
            pf.artist = parts[0].trimmed();
            pf.title = parts.mid(1).join(" - ").trimmed();
        } else {
            pf.title = baseName;
            pf.artist = "Desconhecido";
        }

        m_pendingFiles.append(pf);
    }

    refreshFileList();
}

void AddMusicPage::refreshFileList() {
    // Clear list
    QLayoutItem *item;
    while ((item = m_fileListLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (m_pendingFiles.isEmpty()) {
        m_fileListContainer->hide();
        m_folderSection->hide();
        return;
    }

    m_fileListContainer->show();
    m_folderSection->show();

    auto *header = new QLabel(QString("%1 arquivo%2 selecionado%3")
        .arg(m_pendingFiles.size())
        .arg(m_pendingFiles.size() > 1 ? "s" : "")
        .arg(m_pendingFiles.size() > 1 ? "s" : ""));
    header->setFont(Theme::bodyFont(14));
    header->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold; padding-top: 8px;").arg(Theme::text().name()));
    m_fileListLayout->addWidget(header);

    QString inputStyle = QString(R"(
        QLineEdit {
            background: %1; color: %2; border: 1px solid %3;
            border-radius: 6px; padding: 5px 10px;
        }
        QLineEdit:focus { border-color: %4; }
    )").arg(Theme::surface().name(), Theme::text().name(),
            Theme::border().name(), Theme::accent().name());

    QString labelStyle = QString("color: %1; background: transparent; font-weight: bold; letter-spacing: 0.5px;")
        .arg(Theme::textMuted().name());

    for (int i = 0; i < m_pendingFiles.size(); ++i) {
        auto &pf = m_pendingFiles[i];

        auto *card = new QWidget();
        card->setObjectName("fileCard");
        card->setStyleSheet(QString("QWidget#fileCard { background: %1; border-radius: 10px; }").arg(Theme::card().name()));

        auto *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(14);

        // Color swatch
        auto *swatch = new QWidget();
        swatch->setFixedSize(44, 44);
        swatch->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 8px;")
            .arg(pf.palette.c1.name(), pf.palette.c2.name()));
        cardLayout->addWidget(swatch, 0, Qt::AlignTop);

        // Fields
        auto *fieldsLayout = new QVBoxLayout();
        fieldsLayout->setSpacing(6);
        fieldsLayout->setContentsMargins(0, 0, 0, 0);

        // Title field
        auto *titleRow = new QVBoxLayout();
        titleRow->setSpacing(2);
        auto *titleLabel = new QLabel("Nome da música");
        titleLabel->setFont(Theme::bodyFont(10));
        titleLabel->setStyleSheet(labelStyle);
        auto *titleEdit = new QLineEdit(pf.title);
        titleEdit->setFont(Theme::bodyFont(12));
        titleEdit->setStyleSheet(inputStyle);
        titleEdit->setPlaceholderText("Título da música");
        int idx = i;
        connect(titleEdit, &QLineEdit::textChanged, [this, idx](const QString &text) {
            if (idx < m_pendingFiles.size()) m_pendingFiles[idx].title = text;
        });
        titleRow->addWidget(titleLabel);
        titleRow->addWidget(titleEdit);
        fieldsLayout->addLayout(titleRow);

        // Artist field
        auto *artistRow = new QVBoxLayout();
        artistRow->setSpacing(2);
        auto *artistLabel = new QLabel("Nome do artista");
        artistLabel->setFont(Theme::bodyFont(10));
        artistLabel->setStyleSheet(labelStyle);
        auto *artistEdit = new QLineEdit(pf.artist);
        artistEdit->setFont(Theme::bodyFont(12));
        artistEdit->setStyleSheet(inputStyle);
        artistEdit->setPlaceholderText("Nome do artista");
        connect(artistEdit, &QLineEdit::textChanged, [this, idx](const QString &text) {
            if (idx < m_pendingFiles.size()) m_pendingFiles[idx].artist = text;
        });
        artistRow->addWidget(artistLabel);
        artistRow->addWidget(artistEdit);
        fieldsLayout->addLayout(artistRow);

        cardLayout->addLayout(fieldsLayout, 1);

        // Right side: size + remove
        auto *rightLayout = new QVBoxLayout();
        rightLayout->setSpacing(4);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        auto *removeBtn = new QPushButton("✕");
        removeBtn->setFixedSize(28, 28);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: none; font-size: 14px; border-radius: 14px; }"
            "QPushButton:hover { background: rgba(255,255,255,0.08); color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::danger().name()));
        QString fileId = pf.id;
        connect(removeBtn, &QPushButton::clicked, [this, fileId]() {
            m_pendingFiles.erase(std::remove_if(m_pendingFiles.begin(), m_pendingFiles.end(),
                [&](const PendingFile &f) { return f.id == fileId; }), m_pendingFiles.end());
            refreshFileList();
        });
        rightLayout->addWidget(removeBtn, 0, Qt::AlignRight | Qt::AlignTop);

        auto *sizeLabel = new QLabel(QString("%1 MB").arg(pf.fileSize / (1024.0 * 1024.0), 0, 'f', 1));
        sizeLabel->setFont(Theme::monoFont(10));
        sizeLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
        sizeLabel->setAlignment(Qt::AlignRight);
        rightLayout->addWidget(sizeLabel, 0, Qt::AlignRight | Qt::AlignBottom);
        rightLayout->addStretch();

        cardLayout->addLayout(rightLayout);

        m_fileListLayout->addWidget(card);
    }

    m_addBtn->setText(QString("Adicionar %1 Música%2")
        .arg(m_pendingFiles.size())
        .arg(m_pendingFiles.size() > 1 ? "s" : ""));
}

static QString findFfmpeg() {
    QString found = QStandardPaths::findExecutable("ffmpeg");
    if (!found.isEmpty()) return found;

    QString localAppData = qgetenv("LOCALAPPDATA");
    QString userProfile  = qgetenv("USERPROFILE");

    QStringList candidates;

    // WinGet (Gyan.FFmpeg)
    QDir winget(localAppData + "/Microsoft/WinGet/Packages");
    for (const auto &pkg : winget.entryList({"Gyan.FFmpeg*"}, QDir::Dirs)) {
        QDir pkgDir(winget.filePath(pkg));
        for (const auto &sub : pkgDir.entryList({"ffmpeg*"}, QDir::Dirs))
            candidates << pkgDir.filePath(sub) + "/bin/ffmpeg.exe";
    }

    // Scoop
    candidates << userProfile + "/scoop/apps/ffmpeg/current/bin/ffmpeg.exe";
    // Chocolatey
    candidates << "C:/ProgramData/chocolatey/bin/ffmpeg.exe";
    // Manual
    candidates << "C:/ffmpeg/bin/ffmpeg.exe";

    for (const auto &c : candidates)
        if (QFile::exists(c)) return c;
    return {};
}

static QString findYtDlp() {
    // 1. Check PATH via Qt
    QString found = QStandardPaths::findExecutable("yt-dlp");
    if (!found.isEmpty()) return found;

    // 2. Search common Windows Python install locations
    QString localAppData = qgetenv("LOCALAPPDATA");
    QString appData      = qgetenv("APPDATA");

    QStringList scriptsDirs;

    // Python from Microsoft Store (Packages)
    QDir packagesDir(localAppData + "/Packages");
    for (const auto &entry : packagesDir.entryList({"PythonSoftwareFoundation.Python*"}, QDir::Dirs)) {
        QString base = localAppData + "/Packages/" + entry + "/LocalCache/local-packages";
        for (const auto &ver : QDir(base).entryList({"Python3*"}, QDir::Dirs))
            scriptsDirs << base + "/" + ver + "/Scripts";
    }

    // Regular Python install
    QDir programsDir(localAppData + "/Programs/Python");
    for (const auto &ver : programsDir.entryList({"Python3*"}, QDir::Dirs))
        scriptsDirs << localAppData + "/Programs/Python/" + ver + "/Scripts";

    // User site-packages (pip install --user)
    QDir appDataPy(appData + "/Python");
    for (const auto &ver : appDataPy.entryList({"Python3*"}, QDir::Dirs))
        scriptsDirs << appData + "/Python/" + ver + "/Scripts";

    for (const auto &dir : scriptsDirs) {
        QString candidate = dir + "/yt-dlp.exe";
        if (QFile::exists(candidate)) return candidate;
    }
    return {};
}

void AddMusicPage::startDownload() {
    if (m_downloadProcess) return;

    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) return;

    if (!url.contains("youtube.com") && !url.contains("youtu.be")) {
        m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::danger().name()));
        m_downloadStatus->setText("Use um link do YouTube (youtube.com ou youtu.be).");
        m_downloadStatus->show();
        return;
    }

    QString outDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/downloads";
    QDir().mkpath(outDir);

    // Unique prefix per download session — avoids any before/after comparison issues
    m_downloadPrefix = QString::number(QDateTime::currentMSecsSinceEpoch());

    m_downloadBtn->setEnabled(false);
    m_lastDownloadOutput.clear();
    m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    m_downloadStatus->setText("Conectando...");
    m_downloadStatus->show();

    m_downloadProcess = new QProcess(this);
    m_downloadProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_downloadProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString out = QString::fromUtf8(m_downloadProcess->readAllStandardOutput()).trimmed();
        // buffer last meaningful line for error reporting
        for (const auto &line : out.split('\n')) {
            QString l = line.trimmed();
            if (!l.isEmpty()) m_lastDownloadOutput = l;
        }
        if (out.contains("[download]")) {
            QRegularExpression re(R"((\d+\.?\d*)%)");
            auto match = re.match(out);
            m_downloadStatus->setText(match.hasMatch()
                ? QString("Baixando... %1%").arg(match.captured(1))
                : "Baixando...");
        } else if (out.contains("ExtractAudio") || out.contains("ffmpeg")) {
            m_downloadStatus->setText("Convertendo para Opus...");
        }
    });

    connect(m_downloadProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) {
            m_downloadProcess = nullptr;
            m_downloadBtn->setEnabled(true);
            m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::danger().name()));
            m_downloadStatus->setText("yt-dlp não encontrado. Instale com: pip install yt-dlp");
        }
    });

    connect(m_downloadProcess, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_downloadProcess = nullptr;
        m_downloadBtn->setEnabled(true);

        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::danger().name()));
            QString detail = m_lastDownloadOutput.isEmpty()
                ? "Verifique o link ou tente novamente."
                : m_lastDownloadOutput;
            m_downloadStatus->setText("Erro: " + detail);
            m_downloadStatus->setWordWrap(true);
            return;
        }

        QString outDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/downloads";
        // Find files that match the unique prefix for this download session
        QStringList added;
        for (const auto &f : QDir(outDir).entryList({m_downloadPrefix + "_*.opus"}, QDir::Files))
            added.append(outDir + "/" + f);

        if (added.isEmpty()) {
            m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::danger().name()));
            m_downloadStatus->setText("Download concluído, mas nenhum arquivo .opus encontrado.");
            return;
        }

        m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::accent().name()));
        m_downloadStatus->setText(QString("Concluído! %1 arquivo%2 pronto%3 para adicionar.")
            .arg(added.size())
            .arg(added.size() > 1 ? "s" : "")
            .arg(added.size() > 1 ? "s" : ""));
        m_urlEdit->clear();
        processFiles(added);
    });

    QString ytDlp = findYtDlp();
    if (ytDlp.isEmpty()) {
        m_downloadProcess = nullptr;
        m_downloadBtn->setEnabled(true);
        m_downloadStatus->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::danger().name()));
        m_downloadStatus->setText("yt-dlp não encontrado. Instale com: pip install yt-dlp");
        m_downloadStatus->show();
        return;
    }

    // Add yt-dlp's directory to PATH
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", QFileInfo(ytDlp).absolutePath() + ";" + env.value("PATH"));
    m_downloadProcess->setProcessEnvironment(env);

    QStringList args = {
        "-x",
        "--audio-format", "opus",
        "--audio-quality", "0",
        "-o", outDir + "/" + m_downloadPrefix + "_%(title)s.%(ext)s"
    };

    // Pass ffmpeg location explicitly if found
    QString ffmpeg = findFfmpeg();
    if (!ffmpeg.isEmpty())
        args << "--ffmpeg-location" << QFileInfo(ffmpeg).absolutePath();

    args << url;
    m_downloadProcess->start(ytDlp, args);
}

void AddMusicPage::addAllToLibrary() {
    if (m_pendingFiles.isEmpty()) return;

    QString folder = m_newFolderEdit->text().trimmed();
    if (folder.isEmpty()) {
        folder = m_folderCombo->currentText();
        if (folder == "Sem playlist") folder = "";
    }

    for (auto &pf : m_pendingFiles) {
        Track t = Track::create(pf.title, pf.artist, folder, QUrl::fromLocalFile(pf.filePath));
        t.cover = pf.palette;
        m_model->addTrack(t);
        emit trackAdded(t);
    }

    m_pendingFiles.clear();
    m_newFolderEdit->clear();
    refreshFileList();
    emit navigateBack();
}