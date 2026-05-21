#include "homepage.h"
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QPainter>
#include <QLinearGradient>
#include <QDateTime>
#include <QFrame>
#include "hoverplayfilter.h"

HomePage::HomePage(TrackModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(content);
    m_contentLayout->setContentsMargins(32, 28, 32, 28);
    m_contentLayout->setSpacing(12);

    m_scroll->setWidget(content);
    outerLayout->addWidget(m_scroll);
}

void HomePage::refresh(int currentTrackId, bool isPlaying) {
    // Clear existing
    QLayoutItem *item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        if (item->layout()) {
            QLayoutItem *sub;
            while ((sub = item->layout()->takeAt(0)) != nullptr) {
                if (sub->widget()) sub->widget()->deleteLater();
                delete sub;
            }
        }
        delete item;
    }

    // Greeting
    int hour = QTime::currentTime().hour();
    QString greeting = hour < 12 ? "Bom dia" : (hour < 18 ? "Boa tarde" : "Boa noite");
    auto *greetLabel = new QLabel(greeting);
    greetLabel->setFont(Theme::titleFont(28));
    greetLabel->setStyleSheet(QString("color: %1; background: transparent; padding-bottom: 8px;").arg(Theme::text().name()));
    m_contentLayout->addWidget(greetLabel);

    if (m_model->tracks().isEmpty()) {
        // Empty state
        auto *emptyWidget = new QWidget();
        emptyWidget->setStyleSheet("background: transparent;");
        auto *emptyLayout = new QVBoxLayout(emptyWidget);
        emptyLayout->setAlignment(Qt::AlignCenter);
        emptyLayout->setSpacing(12);

        auto *msg = new QLabel("Sua biblioteca está vazia");
        msg->setFont(Theme::bodyFont(16));
        msg->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
        msg->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(msg);

        auto *sub = new QLabel("Adicione seus arquivos de áudio para começar");
        sub->setFont(Theme::bodyFont(12));
        sub->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
        sub->setAlignment(Qt::AlignCenter);
        emptyLayout->addWidget(sub);

        auto *addBtn = new QPushButton("Adicionar Músicas");
        addBtn->setFont(Theme::bodyFont(14));
        addBtn->setFixedSize(200, 44);
        addBtn->setCursor(Qt::PointingHandCursor);
        addBtn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: none; border-radius: 22px; font-weight: bold; }"
            "QPushButton:hover { background-color: %3; }"
        ).arg(Theme::accent().name(), Theme::bg().name(), Theme::accent().lighter(110).name()));
        connect(addBtn, &QPushButton::clicked, [this]() { emit navigateTo("add"); });
        emptyLayout->addWidget(addBtn, 0, Qt::AlignCenter);

        emptyWidget->setMinimumHeight(350);
        m_contentLayout->addWidget(emptyWidget);
        m_contentLayout->addStretch();
        return;
    }

    // ── Folder chips ────────────────────────────────────────
    auto folders = m_model->folders();
    if (!folders.isEmpty()) {
        auto *grid = new QGridLayout();
        grid->setSpacing(8);
        int col = 0, row = 0;
        for (auto &f : folders) {
            int count = m_model->tracksInFolder(f.name).size();
            auto *chip = createFolderChip(f, count);
            grid->addWidget(chip, row, col);
            col++;
            if (col >= 3) { col = 0; row++; }
        }
        // Liked chip
        auto liked = m_model->likedTracks();
        if (!liked.isEmpty()) {
            auto *likedChip = new QPushButton();
            likedChip->setFixedHeight(54);
            likedChip->setCursor(Qt::PointingHandCursor);
            likedChip->setText(QString("  Curtidas    %1").arg(liked.size()));
            likedChip->setFont(Theme::bodyFont(13));
            likedChip->setStyleSheet(QString(
                "QPushButton { background: %1; color: %2; border: none; border-radius: 8px; text-align: left; padding-left: 12px; font-weight: bold; }"
                "QPushButton:hover { background: %3; }"
            ).arg(Theme::card().name(), Theme::text().name(), Theme::cardHover().name()));
            connect(likedChip, &QPushButton::clicked, [this]() { emit navigateTo("liked"); });
            grid->addWidget(likedChip, row, col);
        }
        auto *gridWidget = new QWidget();
        gridWidget->setLayout(grid);
        gridWidget->setStyleSheet("background: transparent;");
        m_contentLayout->addWidget(gridWidget);
        m_contentLayout->addSpacing(16);
    }

    // ── Recently played (listening history) ─────────────────
    auto played = m_model->recentlyPlayed(8);
    if (!played.isEmpty()) {
        auto *playedLabel = new QLabel("Tocadas recentemente");
        playedLabel->setFont(Theme::titleFont(18));
        playedLabel->setStyleSheet(QString("color: %1; background: transparent; padding-top: 8px;").arg(Theme::text().name()));
        m_contentLayout->addWidget(playedLabel);

        for (int i = 0; i < played.size(); ++i) {
            m_contentLayout->addWidget(createTrackRow(played[i], i, currentTrackId, isPlaying));
        }
        m_contentLayout->addSpacing(16);
    }

    // ── Recent tracks ───────────────────────────────────────
    auto recent = m_model->recentTracks(8);
    if (!recent.isEmpty()) {
        auto *recentLabel = new QLabel("Adicionadas recentemente");
        recentLabel->setFont(Theme::titleFont(18));
        recentLabel->setStyleSheet(QString("color: %1; background: transparent; padding-top: 8px;").arg(Theme::text().name()));
        m_contentLayout->addWidget(recentLabel);

        for (int i = 0; i < recent.size(); ++i) {
            m_contentLayout->addWidget(createTrackRow(recent[i], i, currentTrackId, isPlaying));
        }
        m_contentLayout->addSpacing(16);
    }

    // ── All tracks ──────────────────────────────────────────
    auto *allLabel = new QLabel("Biblioteca Completa");
    allLabel->setFont(Theme::titleFont(18));
    allLabel->setStyleSheet(QString("color: %1; background: transparent; padding-top: 8px;").arg(Theme::text().name()));
    m_contentLayout->addWidget(allLabel);

    auto &all = m_model->tracks();
    for (int i = 0; i < all.size(); ++i) {
        m_contentLayout->addWidget(createTrackRow(all[i], i, currentTrackId, isPlaying));
    }
    m_contentLayout->addStretch();
}

QWidget *HomePage::createTrackRow(const Track &track, int index, int currentId, bool isPlaying) {
    bool active = (track.id == currentId);
    auto *row = new QWidget();
    row->setObjectName("trackRow");
    row->setFixedHeight(52);
    row->setCursor(Qt::PointingHandCursor);
    row->setStyleSheet(QString(
        "QWidget#trackRow { background: %1; border-radius: 8px; border-left: 3px solid %2; }"
    ).arg(
        active ? Theme::accentRgba(0.12) : QStringLiteral("transparent"),
        active ? Theme::accent().name() : "transparent"
    ));

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(12);

    // Index — shows a play glyph on hover (and on the active row)
    QString idxNum = QString("%1").arg(index + 1, 2, 10, QChar('0'));
    auto *idx = new QLabel(active ? QStringLiteral("\uE102") : idxNum);
    idx->setFont(Theme::monoFont(12));
    idx->setFixedWidth(28);
    idx->setAlignment(Qt::AlignCenter);
    idx->setStyleSheet(QString("color: %1; background: transparent; font-family: \"Segoe MDL2 Assets\", Consolas;").arg(
        active ? Theme::accent().name() : Theme::textMuted().name()));
    idx->setAttribute(Qt::WA_TransparentForMouseEvents);  // let clicks reach the play overlay
    layout->addWidget(idx);

    // Cover swatch
    auto *swatch = new QWidget();
    swatch->setFixedSize(38, 38);
    swatch->setAttribute(Qt::WA_TransparentForMouseEvents);
    swatch->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1, stop:1 %2); border-radius: 6px;")
        .arg(track.cover.c1.name(), track.cover.c2.name()));
    layout->addWidget(swatch);

    // Info
    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(1);
    auto *title = new QLabel(track.title);
    title->setFont(Theme::bodyFont(13));
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    title->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(
        active ? Theme::accent().name() : Theme::text().name()));
    auto *artist = new QLabel(track.artist);
    artist->setFont(Theme::bodyFont(11));
    artist->setAttribute(Qt::WA_TransparentForMouseEvents);
    artist->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
    infoLayout->addWidget(title);
    infoLayout->addWidget(artist);
    layout->addLayout(infoLayout, 1);

    // Folder tag — clickable, navigates to the playlist
    if (!track.folder.isEmpty()) {
        auto *tag = new QPushButton(track.folder);
        tag->setFont(Theme::bodyFont(10));
        tag->setCursor(Qt::PointingHandCursor);
        tag->setToolTip(QString("Ir para a playlist \"%1\"").arg(track.folder));
        tag->setStyleSheet(QString(
            "QPushButton { color: %1; background: " + Theme::accentRgba(0.10) + "; border: none; border-radius: 10px; padding: 2px 8px; }"
            "QPushButton:hover { background: " + Theme::accentRgba(0.28) + "; color: %2; }"
        ).arg(Theme::accentDim().name(), Theme::text().name()));
        QString folderName = track.folder;
        connect(tag, &QPushButton::clicked, [this, folderName]() { emit navigateTo("folder", folderName); });
        layout->addWidget(tag);
    }

    // Add to queue button
    auto *queueBtn = new QPushButton(QStringLiteral("\uE710"));
    queueBtn->setFixedSize(28, 28);
    queueBtn->setCursor(Qt::PointingHandCursor);
    queueBtn->setToolTip("Adicionar \u00E0 fila");
    queueBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none; font-size: 13px; font-family: \"Segoe MDL2 Assets\"; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::textMuted().name(), Theme::accent().name()));
    Track qt = track;
    connect(queueBtn, &QPushButton::clicked, [this, qt]() { emit enqueueRequested(qt); });
    layout->addWidget(queueBtn);

    // Like button
    auto *likeBtn = new QPushButton(track.liked ? "\uE00B" : "\uE006");
    likeBtn->setFixedSize(28, 28);
    likeBtn->setCursor(Qt::PointingHandCursor);
    likeBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none; font-size: 14px; font-family: \"Segoe MDL2 Assets\"; }"
        "QPushButton:hover { color: %2; }"
    ).arg(track.liked ? Theme::accent().name() : Theme::textMuted().name(), Theme::accent().name()));
    int likeId = track.id;
    connect(likeBtn, &QPushButton::clicked, [this, likeId]() { emit likeToggled(likeId); });
    layout->addWidget(likeBtn);

    // Edit info button
    auto *editBtn = new QPushButton(QStringLiteral("\uE70F"));
    editBtn->setFixedSize(28, 28);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFont(Theme::iconFont(11));
    editBtn->setToolTip("Editar música");
    editBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none; font-family: \"Segoe MDL2 Assets\"; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::textMuted().name(), Theme::accent().name()));
    Track et = track;
    connect(editBtn, &QPushButton::clicked, [this, et]() { emit editTrackRequested(et); });
    layout->addWidget(editBtn);

    // Delete button
    auto *delBtn = new QPushButton(QStringLiteral("\uE107"));
    delBtn->setFixedSize(28, 28);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setFont(Theme::iconFont(11));
    delBtn->setToolTip("Excluir música");
    delBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: none; font-family: \"Segoe MDL2 Assets\"; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::textMuted().name(), Theme::danger().name()));
    int delId = track.id;
    connect(delBtn, &QPushButton::clicked, [this, delId]() { emit deleteRequested(delId); });
    layout->addWidget(delBtn);

    // Duration
    auto *dur = new QLabel(Theme::formatTime(track.durationMs));
    dur->setFont(Theme::monoFont(11));
    dur->setFixedWidth(40);
    dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dur->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    layout->addWidget(dur);

    // Click to play via transparent overlay button
    Track t = track;
    auto *overlay = new QPushButton(row);
    overlay->setGeometry(0, 0, 9999, 52);
    overlay->setStyleSheet("background: transparent; border: none;");
    overlay->setCursor(Qt::PointingHandCursor);
    overlay->lower();
    connect(overlay, &QPushButton::clicked, [this, t]() { emit playRequested(t); });
    // Hover over the row's main area swaps the index number for a play glyph.
    if (!active) overlay->installEventFilter(new HoverPlayFilter(idx, idxNum, overlay));

    return row;
}

QWidget *HomePage::createFolderChip(const Folder &folder, int trackCount) {
    auto *chip = new QPushButton();
    chip->setFixedHeight(54);
    chip->setCursor(Qt::PointingHandCursor);
    chip->setText(QString("  %1    %2").arg(folder.name).arg(trackCount));
    chip->setFont(Theme::bodyFont(13));
    chip->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 8px; text-align: left; padding-left: 12px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::card().name(), Theme::text().name(), Theme::cardHover().name()));

    QString name = folder.name;
    connect(chip, &QPushButton::clicked, [this, name]() { emit navigateTo("folder", name); });
    return chip;
}
