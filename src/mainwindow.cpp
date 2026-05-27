#include "mainwindow.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QDialog>
#include <QGridLayout>
#include <QSettings>
#include <QResizeEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Lumen Music");
    resize(1100, 720);
    setMinimumSize(720, 480);

    m_model = new TrackModel(this);

    // ── Central widget ──────────────────────────────────────
    auto *central = new QWidget(this);
    setCentralWidget(central);
    central->setStyleSheet(Theme::globalStyleSheet());

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // ── Sidebar ─────────────────────────────────────────────
    auto *sidebar = new QWidget();
    sidebar->setFixedWidth(260);
    sidebar->setStyleSheet(QString("background-color: %1;").arg(Theme::surface().name()));
    buildSidebar(sidebar);
    bodyLayout->addWidget(sidebar);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet(QString("color: %1;").arg(Theme::border().name()));
    sep->setFixedWidth(1);
    bodyLayout->addWidget(sep);

    // ── Stacked pages ───────────────────────────────────────
    m_stack = new QStackedWidget();
    m_stack->setStyleSheet("background: transparent;");

    m_homePage = new HomePage(m_model, this);
    m_addPage = new AddMusicPage(m_model, this);
    m_foldersPage = new FoldersPage(m_model, this);
    m_folderDetailPage = new FolderDetailPage(m_model, this);
    m_likedPage = new LikedPage(m_model, this);

    m_stack->addWidget(m_homePage);       // 0
    m_stack->addWidget(m_addPage);        // 1
    m_stack->addWidget(m_foldersPage);    // 2
    m_stack->addWidget(m_folderDetailPage); // 3
    m_stack->addWidget(m_likedPage);      // 4

    bodyLayout->addWidget(m_stack, 1);
    mainLayout->addLayout(bodyLayout, 1);

    // ── Player bar ──────────────────────────────────────────
    m_playerBar = new PlayerBar(m_model, this);
    mainLayout->addWidget(m_playerBar);

    // Queue page needs the player bar to read the live queue.
    m_queuePage = new QueuePage(m_model, m_playerBar, this);
    m_stack->addWidget(m_queuePage);   // 5

    // ── Connections ─────────────────────────────────────────
    // Home page
    connect(m_homePage, &HomePage::playRequested, this, &MainWindow::onTrackPlay);
    connect(m_homePage, &HomePage::likeToggled, this, [this](int id) {
        m_model->toggleLike(id);
        refreshCurrentPage();
    });
    connect(m_homePage, &HomePage::navigateTo, this, &MainWindow::navigateTo);
    connect(m_homePage, &HomePage::enqueueRequested, this, [this](const Track &t) {
        m_playerBar->enqueue(t);
        showToast("Adicionado à fila");
    });
    connect(m_homePage, &HomePage::editTrackRequested, this, [this](const Track &t) {
        showEditTrackDialog(t);
    });
    connect(m_homePage, &HomePage::deleteRequested, this, [this](int id) {
        if (Track *t = m_model->findTrack(id)) confirmDeleteTrack(*t);
    });

    // Add page
    connect(m_addPage, &AddMusicPage::trackAdded, this, [this](const Track &) {
        refreshCurrentPage();
        refreshSidebarFolders();
    });
    connect(m_addPage, &AddMusicPage::navigateBack, this, [this]() { navigateTo("home"); });

    // Folders page
    connect(m_foldersPage, &FoldersPage::folderSelected, this, [this](const QString &name) {
        navigateTo("folder", name);
    });

    // Folder detail
    connect(m_folderDetailPage, &FolderDetailPage::playRequested, this, &MainWindow::onTrackPlay);
    connect(m_folderDetailPage, &FolderDetailPage::likeToggled, this, [this](int id) {
        m_model->toggleLike(id);
        refreshCurrentPage();
    });
    connect(m_folderDetailPage, &FolderDetailPage::deleteRequested, this, [this](int id) {
        if (Track *t = m_model->findTrack(id)) confirmDeleteTrack(*t);
    });
    connect(m_folderDetailPage, &FolderDetailPage::editTrackRequested, this, [this](const Track &t) {
        showEditTrackDialog(t);
    });
    connect(m_folderDetailPage, &FolderDetailPage::navigateBack, this, [this]() { navigateTo("folders"); });
    connect(m_folderDetailPage, &FolderDetailPage::enqueueRequested, this, [this](const Track &t) {
        m_playerBar->enqueue(t);
        showToast("Adicionado à fila");
    });

    // Liked page
    connect(m_likedPage, &LikedPage::playRequested, this, &MainWindow::onTrackPlay);
    connect(m_likedPage, &LikedPage::likeToggled, this, [this](int id) {
        m_model->toggleLike(id);
        refreshCurrentPage();
    });
    connect(m_likedPage, &LikedPage::navigateBack, this, [this]() { navigateTo("home"); });
    connect(m_likedPage, &LikedPage::navigateToFolder, this, [this](const QString &name) {
        navigateTo("folder", name);
    });
    connect(m_likedPage, &LikedPage::enqueueRequested, this, [this](const Track &t) {
        m_playerBar->enqueue(t);
        showToast("Adicionado à fila");
    });
    connect(m_likedPage, &LikedPage::editTrackRequested, this, [this](const Track &t) {
        showEditTrackDialog(t);
    });
    connect(m_likedPage, &LikedPage::deleteRequested, this, [this](int id) {
        if (Track *t = m_model->findTrack(id)) confirmDeleteTrack(*t);
    });

    // Queue page
    connect(m_queuePage, &QueuePage::playContext, this, [this](const Track &t) {
        m_playerBar->playKeepingContext(t);
        refreshCurrentPage();
    });
    connect(m_queuePage, &QueuePage::playFromQueue, this, [this](int index) {
        Track t;
        if (m_playerBar->takeFromQueue(index, t)) {
            m_playerBar->playKeepingContext(t);
        }
        refreshCurrentPage();
    });
    connect(m_queuePage, &QueuePage::removeFromQueueRequested, this, [this](int index) {
        m_playerBar->removeFromQueue(index);
    });
    connect(m_queuePage, &QueuePage::likeToggled, this, [this](int id) {
        m_model->toggleLike(id);
        refreshCurrentPage();
    });
    connect(m_queuePage, &QueuePage::navigateBack, this, [this]() { navigateTo("home"); });

    // Player bar
    connect(m_playerBar, &PlayerBar::trackChanged, this, [this](int) {
        refreshCurrentPage();
    });
    connect(m_playerBar, &PlayerBar::queueRequested, this, [this]() { navigateTo("queue"); });
    connect(m_playerBar, &PlayerBar::queueChanged, this, [this]() { refreshCurrentPage(); });

    // Model changes — keep the count label, the visible page, and the sidebar
    // in sync so edits (cover image/colors, rename, etc.) reflect immediately.
    connect(m_model, &TrackModel::tracksChanged, this, [this]() {
        m_trackCountLabel->setText(QString("%1 faixa%2 na biblioteca")
            .arg(m_model->tracks().size())
            .arg(m_model->tracks().size() != 1 ? "s" : ""));
        refreshCurrentPage();
        refreshSidebarFolders();
    });

    // Initial state
    navigateTo("home");
}

void MainWindow::buildSidebar(QWidget *sidebar) {
    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Logo
    auto *logoWidget = new QWidget();
    logoWidget->setStyleSheet("background: transparent;");
    auto *logoLayout = new QHBoxLayout(logoWidget);
    logoLayout->setContentsMargins(20, 20, 20, 16);
    logoLayout->setSpacing(8);

    auto *logoIcon = new QLabel();
    logoIcon->setFixedSize(30, 30);
    logoIcon->setScaledContents(true);
    logoIcon->setStyleSheet("background: transparent;");
    logoIcon->setPixmap(QIcon(":/icon.png").pixmap(30, 30));
    logoLayout->addWidget(logoIcon);

    auto *logoText = new QLabel("Lumen");
    logoText->setFont(Theme::titleFont(18));
    logoText->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::text().name()));
    logoLayout->addWidget(logoText);

    auto *badge = new QLabel("MUSIC");
    badge->setFont(Theme::bodyFont(9));
    QColor ac = Theme::accent();
    badge->setStyleSheet(QString("color: %1; background: rgba(%2,%3,%4,0.16); border-radius: 4px; padding: 2px 6px; font-weight: bold; letter-spacing: 1px;")
        .arg(Theme::accent().name()).arg(ac.red()).arg(ac.green()).arg(ac.blue()));
    logoLayout->addWidget(badge);
    logoLayout->addStretch();

    layout->addWidget(logoWidget);

    // Navigation buttons
    auto *navWidget = new QWidget();
    navWidget->setStyleSheet("background: transparent;");
    auto *navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(10, 0, 10, 0);
    navLayout->setSpacing(2);

    auto makeNavBtn = [this](const QString &text, const QString &icon) -> QPushButton* {
        auto *btn = new QPushButton(QString("  %1  %2").arg(icon, text));
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(Theme::bodyFont(13));
        btn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: none; border-radius: 10px; text-align: left; padding-left: 14px; font-family: \"Segoe UI\", \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { background: rgba(255,255,255,0.05); color: %2; }"
        ).arg(Theme::textSoft().name(), Theme::text().name()));
        return btn;
    };

    m_navHome    = makeNavBtn("Início",    "\uE10F");
    m_navAdd     = makeNavBtn("Adicionar", "\uE109");
    m_navFolders = makeNavBtn("Playlists", "\uE188");

    connect(m_navHome, &QPushButton::clicked, [this]() { navigateTo("home"); });
    connect(m_navAdd, &QPushButton::clicked, [this]() { navigateTo("add"); });
    connect(m_navFolders, &QPushButton::clicked, [this]() { navigateTo("folders"); });

    navLayout->addWidget(m_navHome);
    navLayout->addWidget(m_navAdd);
    navLayout->addWidget(m_navFolders);

    layout->addWidget(navWidget);

    // Sidebar folders
    auto *foldersHeader = new QLabel("  SUAS PLAYLISTS");
    foldersHeader->setFont(Theme::bodyFont(10));
    foldersHeader->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold; letter-spacing: 1px; padding: 16px 20px 4px;")
        .arg(Theme::textMuted().name()));
    layout->addWidget(foldersHeader);

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    m_sidebarFoldersContainer = new QWidget();
    m_sidebarFoldersContainer->setStyleSheet("background: transparent;");
    m_sidebarFoldersLayout = new QVBoxLayout(m_sidebarFoldersContainer);
    m_sidebarFoldersLayout->setContentsMargins(10, 0, 10, 0);
    m_sidebarFoldersLayout->setSpacing(1);
    m_sidebarFoldersLayout->addStretch();

    scrollArea->setWidget(m_sidebarFoldersContainer);
    layout->addWidget(scrollArea, 1);

    // Footer
    auto *footerSep = new QFrame();
    footerSep->setFrameShape(QFrame::HLine);
    footerSep->setStyleSheet(QString("color: %1;").arg(Theme::border().name()));
    footerSep->setFixedHeight(1);
    layout->addWidget(footerSep);

    auto *footerWidget = new QWidget();
    footerWidget->setStyleSheet("background: transparent;");
    auto *footerLayout = new QHBoxLayout(footerWidget);
    footerLayout->setContentsMargins(20, 8, 12, 12);
    footerLayout->setSpacing(4);

    m_trackCountLabel = new QLabel("0 faixas na biblioteca");
    m_trackCountLabel->setFont(Theme::bodyFont(10));
    m_trackCountLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    footerLayout->addWidget(m_trackCountLabel, 1);

    auto *themeBtn = new QPushButton("\uE790");
    themeBtn->setFixedSize(28, 28);
    themeBtn->setCursor(Qt::PointingHandCursor);
    themeBtn->setFont(Theme::iconFont(12));
    themeBtn->setToolTip("Escolher tema");
    themeBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none; border-radius: 6px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.08); color: %2; }"
    ).arg(Theme::textMuted().name(), Theme::accent().name()));
    connect(themeBtn, &QPushButton::clicked, this, &MainWindow::showThemePicker);
    footerLayout->addWidget(themeBtn);

    layout->addWidget(footerWidget);
}

void MainWindow::refreshSidebarFolders() {
    // Clear
    QLayoutItem *item;
    while ((item = m_sidebarFoldersLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    auto folders = m_model->folders();
    if (folders.isEmpty()) {
        auto *emptyLabel = new QLabel("Nenhuma playlist");
        emptyLabel->setFont(Theme::bodyFont(11));
        emptyLabel->setStyleSheet(QString("color: %1; background: transparent; padding: 4px 14px;").arg(Theme::textMuted().name()));
        m_sidebarFoldersLayout->addWidget(emptyLabel);
    } else {
        for (auto &f : folders) {
            int count = m_model->tracksInFolder(f.name).size();
            auto *btn = new QPushButton();
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(34);
            btn->setFont(Theme::bodyFont(12));

            auto *btnLayout = new QHBoxLayout(btn);
            btnLayout->setContentsMargins(14, 0, 14, 0);
            auto *nameLabel = new QLabel(f.name);
            nameLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
            nameLabel->setFont(Theme::bodyFont(12));
            auto *countLabel = new QLabel(QString::number(count));
            countLabel->setFont(Theme::monoFont(10));
            countLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
            btnLayout->addWidget(nameLabel);
            btnLayout->addStretch();
            btnLayout->addWidget(countLabel);

            bool isActive = (m_currentPage == "folder" && m_folderDetailPage->property("folderName").toString() == f.name);
            btn->setStyleSheet(QString(
                "QPushButton { background: %1; border: none; border-radius: 8px; }"
                "QPushButton:hover { background: rgba(255,255,255,0.05); }"
            ).arg(isActive ? Theme::accentRgba(0.12) : QStringLiteral("transparent")));

            QString folderName = f.name;
            connect(btn, &QPushButton::clicked, [this, folderName]() { navigateTo("folder", folderName); });

            m_sidebarFoldersLayout->addWidget(btn);
        }
    }

    m_sidebarFoldersLayout->addStretch();
}

void MainWindow::showEditTrackDialog(const Track &track) {
    int id = track.id;
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle("Editar Música");
    dlg->setFixedSize(380, 200);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet(QString(
        "QDialog { background: %1; }"
        "QLabel { background: transparent; color: %2; }"
        "QLineEdit { background: %3; color: %2; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; }"
        "QLineEdit:focus { border-color: %5; }"
    ).arg(Theme::surface().name(), Theme::text().name(), Theme::bg().name(),
          Theme::border().name(), Theme::accent().name()));

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel("Nome da música");
    titleLabel->setFont(Theme::bodyFont(12));
    layout->addWidget(titleLabel);
    auto *titleEdit = new QLineEdit(track.title);
    titleEdit->setFont(Theme::bodyFont(13));
    layout->addWidget(titleEdit);

    auto *artistLabel = new QLabel("Artista");
    artistLabel->setFont(Theme::bodyFont(12));
    layout->addWidget(artistLabel);
    auto *artistEdit = new QLineEdit(track.artist);
    artistEdit->setFont(Theme::bodyFont(13));
    layout->addWidget(artistEdit);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton("Cancelar");
    cancelBtn->setFont(Theme::bodyFont(12));
    cancelBtn->setFixedHeight(36);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; border-radius: 18px; padding: 0 16px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); }"
    ).arg(Theme::textSoft().name(), Theme::border().name()));
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto *saveBtn = new QPushButton("Salvar");
    saveBtn->setFont(Theme::bodyFont(12));
    saveBtn->setFixedHeight(36);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 18px; padding: 0 20px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::accent().name(), Theme::bg().name(), Theme::accent().lighter(110).name()));
    connect(saveBtn, &QPushButton::clicked, [this, dlg, titleEdit, artistEdit, id]() {
        QString title = titleEdit->text().trimmed();
        if (title.isEmpty()) return;
        m_model->updateTrack(id, title, artistEdit->text().trimmed());
        dlg->accept();
    });
    btnRow->addWidget(saveBtn);
    layout->addLayout(btnRow);

    connect(artistEdit, &QLineEdit::returnPressed, saveBtn, &QPushButton::click);
    dlg->exec();
}

void MainWindow::confirmDeleteTrack(const Track &track) {
    auto *dlg = new QMessageBox(this);
    dlg->setWindowTitle("Excluir Música");
    dlg->setText(QString("Excluir \"%1\"?").arg(track.title));
    dlg->setInformativeText("A música será removida da biblioteca.");
    dlg->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    dlg->setDefaultButton(QMessageBox::Cancel);
    dlg->setStyleSheet(QString(
        "QMessageBox { background: %1; color: %2; } QLabel { color: %2; background: transparent; }"
        "QPushButton { background: %3; color: %2; border: 1px solid %4; border-radius: 8px; padding: 6px 16px; min-width: 70px; }"
        "QPushButton:hover { background: %5; }"
    ).arg(Theme::surface().name(), Theme::text().name(), Theme::card().name(),
          Theme::border().name(), Theme::cardHover().name()));
    if (dlg->exec() == QMessageBox::Yes) {
        m_model->removeTrack(track.id);
        refreshSidebarFolders();
    }
}

void MainWindow::showToast(const QString &text) {
    if (!m_toast) {
        m_toast = new QLabel(this);
        m_toast->setAlignment(Qt::AlignCenter);
        m_toast->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_toastTimer = new QTimer(this);
        m_toastTimer->setSingleShot(true);
        connect(m_toastTimer, &QTimer::timeout, this, [this]() { if (m_toast) m_toast->hide(); });
    }
    // Light pill with dark text, like Spotify's "Added to queue".
    m_toast->setFont(Theme::bodyFont(12));
    m_toast->setStyleSheet(QString(
        "background: %1; color: %2; border-radius: 8px; padding: 10px 18px; font-weight: 600;")
        .arg(Theme::text().name(), Theme::bg().name()));
    m_toast->setText(text);
    m_toast->adjustSize();
    repositionToast();
    m_toast->show();
    m_toast->raise();
    m_toastTimer->start(1800);
}

void MainWindow::repositionToast() {
    if (!m_toast) return;
    int x = (width() - m_toast->width()) / 2;
    int y = height() - m_playerBar->height() - m_toast->height() - 24;
    m_toast->move(x, y);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_toast && m_toast->isVisible()) repositionToast();
}

void MainWindow::navigateTo(const QString &page, const QString &data) {
    m_currentPage = page;

    // Update nav button styles
    auto activeStyle = [](bool active) {
        if (active) {
            return QString(
                "QPushButton { background: " + Theme::accentRgba(0.15) + "; color: %1; border: none; border-radius: 10px; text-align: left; padding-left: 14px; font-weight: bold; }"
            ).arg(Theme::accent().name());
        }
        return QString(
            "QPushButton { background: transparent; color: %1; border: none; border-radius: 10px; text-align: left; padding-left: 14px; }"
            "QPushButton:hover { background: rgba(255,255,255,0.05); color: %2; }"
        ).arg(Theme::textSoft().name(), Theme::text().name());
    };

    m_navHome->setStyleSheet(activeStyle(page == "home"));
    m_navAdd->setStyleSheet(activeStyle(page == "add"));
    m_navFolders->setStyleSheet(activeStyle(page == "folders" || page == "folder"));

    if (page == "home") {
        m_stack->setCurrentIndex(0);
    } else if (page == "add") {
        m_addPage->refresh();
        m_stack->setCurrentIndex(1);
    } else if (page == "folders") {
        m_stack->setCurrentIndex(2);
    } else if (page == "folder") {
        m_folderDetailPage->setFolder(data);
        m_folderDetailPage->setProperty("folderName", data);
        m_stack->setCurrentIndex(3);
    } else if (page == "liked") {
        m_stack->setCurrentIndex(4);
    } else if (page == "queue") {
        m_stack->setCurrentIndex(5);
    }

    refreshCurrentPage();
    refreshSidebarFolders();
}

void MainWindow::refreshCurrentPage() {
    int curId = m_playerBar->currentTrackId();
    bool playing = m_playerBar->isPlaying();

    if (m_stack->currentIndex() == 0) {
        m_homePage->refresh(curId, playing);
    } else if (m_stack->currentIndex() == 2) {
        m_foldersPage->refresh();
    } else if (m_stack->currentIndex() == 3) {
        m_folderDetailPage->refresh(curId, playing);
    } else if (m_stack->currentIndex() == 4) {
        m_likedPage->refresh(curId, playing);
    } else if (m_stack->currentIndex() == 5) {
        m_queuePage->refresh(curId, playing);
    }
}

void MainWindow::onTrackPlay(const Track &track) {
    // Build the playback queue from the context the track was launched in, so
    // each playlist plays within itself instead of the whole library.
    QList<Track> queue;
    if (m_currentPage == "folder") {
        QString fname = m_folderDetailPage->property("folderName").toString();
        queue = fname.isEmpty() ? m_model->standaloneTracks()
                                : m_model->tracksInFolder(fname);
    } else if (m_currentPage == "liked") {
        queue = m_model->likedTracks();
    } else {
        queue = m_model->tracks();   // home / full library
    }
    m_playerBar->playTrack(track, queue);
    refreshCurrentPage();
}

void MainWindow::showThemePicker() {
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle("Escolher Tema");
    dlg->setFixedSize(356, 290);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet(QString(
        "QDialog { background: %1; }"
        "QLabel  { background: transparent; color: %2; }"
    ).arg(Theme::surface().name(), Theme::text().name()));

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *title = new QLabel("Paleta de Cores");
    title->setFont(Theme::titleFont(14));
    layout->addWidget(title);

    auto *grid = new QGridLayout();
    grid->setSpacing(8);

    const auto themes   = Theme::allThemes();
    const QString curId = Theme::activeTheme().id;

    for (int i = 0; i < themes.size(); ++i) {
        const auto &t = themes[i];

        auto *btn = new QPushButton();
        btn->setFixedSize(152, 72);
        btn->setCursor(Qt::PointingHandCursor);

        bool active = (t.id == curId);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            "    stop:0 %1, stop:1 %2);"
            "  border: 2px solid %3;"
            "  border-radius: 10px;"
            "}"
            "QPushButton:hover { border: 2px solid %4; }"
        ).arg(t.bg.name(), t.accent.name(),
              active ? t.accent.name() : t.border.name(),
              t.accent.name()));

        auto *btnLayout = new QVBoxLayout(btn);
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->setContentsMargins(0, 0, 0, 0);

        auto *nameLabel = new QLabel(t.name);
        nameLabel->setFont(Theme::bodyFont(10));
        nameLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(t.text.name()));
        nameLabel->setAlignment(Qt::AlignCenter);
        btnLayout->addWidget(nameLabel);

        connect(btn, &QPushButton::clicked, dlg, [this, t, dlg]() {
            QSettings s;
            s.setValue("theme", t.id);
            dlg->accept();
            emit themeChangeRequested();
        });

        grid->addWidget(btn, i / 2, i % 2);
    }

    layout->addLayout(grid);
    dlg->exec();
}