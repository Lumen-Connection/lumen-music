#include "folderdetailpage.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QMenu>
#include <QGridLayout>
#include <QListWidget>
#include "hoverplayfilter.h"
#include "reorderablelist.h"

FolderDetailPage::FolderDetailPage(TrackModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(content);
    m_contentLayout->setContentsMargins(32, 28, 32, 28);
    m_contentLayout->setSpacing(8);

    scroll->setWidget(content);
    outerLayout->addWidget(scroll);
}

void FolderDetailPage::setFolder(const QString &folderName) {
    m_folderName = folderName;
    // Look up folder ID
    m_folderId = 0;
    if (!folderName.isEmpty()) {
        for (auto &f : m_model->folders()) {
            if (f.name == folderName) {
                m_folderId = f.id;
                break;
            }
        }
    }
}

void FolderDetailPage::refresh(int currentTrackId, bool isPlaying) {
    QLayoutItem *item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    bool isStandalone = m_folderName.isEmpty();

    // Back button
    auto *backBtn = new QPushButton("\uE0A6");
    backBtn->setFixedSize(34, 34);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFont(Theme::iconFont(12));
    backBtn->setStyleSheet(QString(
        "QPushButton { background: rgba(255,255,255,0.05); color: %1; border: none; border-radius: 17px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.1); }"
    ).arg(Theme::text().name()));
    connect(backBtn, &QPushButton::clicked, this, &FolderDetailPage::navigateBack);
    m_contentLayout->addWidget(backBtn, 0, Qt::AlignLeft);

    auto tracks = isStandalone ? m_model->standaloneTracks() : m_model->tracksInFolder(m_folderName);
    qint64 total = 0;
    for (auto &t : tracks) total += t.durationMs;

    // Header
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(20);

    auto *cover = new QWidget();
    cover->setFixedSize(140, 140);
    if (isStandalone) {
        if (tracks.size() >= 4) {
            auto *coverGrid = new QGridLayout(cover);
            coverGrid->setSpacing(2);
            coverGrid->setContentsMargins(0, 0, 0, 0);
            for (int i = 0; i < 4; ++i) {
                auto *cell = new QWidget();
                cell->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 4px;")
                    .arg(tracks[i].cover.c1.name(), tracks[i].cover.c2.name()));
                coverGrid->addWidget(cell, i / 2, i % 2);
            }
        } else {
            auto g = tracks.isEmpty() ? Theme::randomPalette() : tracks[0].cover;
            cover->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 10px;")
                .arg(g.c1.name(), g.c2.name()));
        }
    } else {
        // Cover image (if set) > mosaic (4+ tracks) > folder gradient
        Theme::GradientPair folderCover;
        QString folderImage;
        for (auto &f : m_model->folders()) {
            if (f.name == m_folderName) { folderCover = f.cover; folderImage = f.coverImage; break; }
        }
        QPixmap coverPix = folderImage.isEmpty() ? QPixmap()
                                                 : Theme::roundedCover(folderImage, 140, 140, 10);
        if (!coverPix.isNull()) {
            auto *imgLabel = new QLabel(cover);
            imgLabel->setGeometry(0, 0, 140, 140);
            imgLabel->setPixmap(coverPix);
            imgLabel->setStyleSheet("background: transparent;");

            // Click the cover image to view it enlarged (like Spotify).
            cover->setCursor(Qt::PointingHandCursor);
            cover->setToolTip("Ver imagem");
            auto *zoom = new QPushButton(cover);
            zoom->setGeometry(0, 0, 140, 140);
            zoom->setStyleSheet("background: transparent; border: none;");
            zoom->setCursor(Qt::PointingHandCursor);
            QString img = folderImage;
            connect(zoom, &QPushButton::clicked, [this, img]() { showCoverLightbox(img); });
        } else if (tracks.size() >= 4) {
            auto *coverGrid = new QGridLayout(cover);
            coverGrid->setSpacing(2);
            coverGrid->setContentsMargins(0, 0, 0, 0);
            for (int i = 0; i < 4; ++i) {
                auto *cell = new QWidget();
                cell->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 4px;")
                    .arg(tracks[i].cover.c1.name(), tracks[i].cover.c2.name()));
                coverGrid->addWidget(cell, i / 2, i % 2);
            }
        } else {
            auto g = (folderCover.c1.isValid() && folderCover.c1 != QColor()) ? folderCover : (tracks.isEmpty() ? Theme::randomPalette() : tracks[0].cover);
            cover->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 10px;")
                .arg(g.c1.name(), g.c2.name()));
        }
    }
    headerLayout->addWidget(cover);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->addStretch();
    auto *typeLabel = new QLabel(isStandalone ? "MÚSICAS AVULSAS" : "PLAYLIST");
    typeLabel->setFont(Theme::bodyFont(10));
    typeLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold; letter-spacing: 1px;").arg(Theme::textMuted().name()));
    infoLayout->addWidget(typeLabel);

    auto *nameRow = new QHBoxLayout();
    auto *nameLabel = new QLabel(isStandalone ? "Músicas avulsas" : m_folderName);
    nameLabel->setFont(Theme::titleFont(28));
    nameLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::text().name()));
    nameRow->addWidget(nameLabel);

    // Edit button (only for real playlists)
    if (!isStandalone) {
        auto *editBtn = new QPushButton("\uE70F");
        editBtn->setFixedSize(32, 32);
        editBtn->setCursor(Qt::PointingHandCursor);
        editBtn->setFont(Theme::iconFont(12));
        editBtn->setToolTip("Editar playlist");
        editBtn->setStyleSheet(QString(
            "QPushButton { background: rgba(255,255,255,0.05); color: %1; border: none; border-radius: 16px; font-family: \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { background: rgba(255,255,255,0.12); color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::accent().name()));
        connect(editBtn, &QPushButton::clicked, this, &FolderDetailPage::showEditDialog);
        nameRow->addWidget(editBtn);

        // Delete button
        auto *deletePlaylistBtn = new QPushButton("\uE107");
        deletePlaylistBtn->setFixedSize(32, 32);
        deletePlaylistBtn->setCursor(Qt::PointingHandCursor);
        deletePlaylistBtn->setFont(Theme::iconFont(12));
        deletePlaylistBtn->setToolTip("Excluir playlist");
        deletePlaylistBtn->setStyleSheet(QString(
            "QPushButton { background: rgba(255,255,255,0.05); color: %1; border: none; border-radius: 16px; font-family: \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { background: rgba(255,255,255,0.12); color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::danger().name()));
        int fid = m_folderId;
        QString fname = m_folderName;
        connect(deletePlaylistBtn, &QPushButton::clicked, [this, fid, fname]() {
            auto *dlg = new QMessageBox(this);
            dlg->setWindowTitle("Excluir Playlist");
            dlg->setText(QString("Excluir a playlist \"%1\"?").arg(fname));
            dlg->setInformativeText("As músicas não serão apagadas — ficarão como músicas avulsas.");
            dlg->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
            dlg->setDefaultButton(QMessageBox::Cancel);
            dlg->setStyleSheet(QString(
                "QMessageBox { background: %1; color: %2; } QLabel { color: %2; background: transparent; }"
                "QPushButton { background: %3; color: %2; border: 1px solid %4; border-radius: 8px; padding: 6px 16px; min-width: 70px; }"
            ).arg(Theme::surface().name(), Theme::text().name(), Theme::card().name(), Theme::border().name()));
            if (dlg->exec() == QMessageBox::Yes) {
                m_model->deletePlaylist(fid);
                emit navigateBack();
            }
        });
        nameRow->addWidget(deletePlaylistBtn);
    }
    nameRow->addStretch();
    infoLayout->addLayout(nameRow);

    auto *statsLabel = new QLabel(QString("%1 faixa%2%3")
        .arg(tracks.size())
        .arg(tracks.size() != 1 ? "s" : "")
        .arg(total > 0 ? QString(" • %1").arg(Theme::formatTime(total)) : ""));
    statsLabel->setFont(Theme::bodyFont(12));
    statsLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
    infoLayout->addWidget(statsLabel);
    infoLayout->addStretch();

    headerLayout->addLayout(infoLayout, 1);

    auto *headerWidget = new QWidget();
    headerWidget->setLayout(headerLayout);
    headerWidget->setStyleSheet("background: transparent;");
    m_contentLayout->addWidget(headerWidget);
    m_contentLayout->addSpacing(12);

    // Play button
    if (!tracks.isEmpty()) {
        auto *playBtn = new QPushButton("\uE102");
        playBtn->setFixedSize(48, 48);
        playBtn->setCursor(Qt::PointingHandCursor);
        playBtn->setFont(Theme::iconFont(16));
        playBtn->setStyleSheet(QString(
            "QPushButton { background: %1; color: %2; border: none; border-radius: 24px; font-family: \"Segoe MDL2 Assets\"; font-size: 16px; }"
            "QPushButton:hover { background: %3; }"
        ).arg(Theme::accent().name(), Theme::bg().name(), Theme::accent().lighter(110).name()));
        Track first = tracks[0];
        connect(playBtn, &QPushButton::clicked, [this, first]() { emit playRequested(first); });
        m_contentLayout->addWidget(playBtn, 0, Qt::AlignLeft);
        m_contentLayout->addSpacing(8);
    }

    // Track list — a QListWidget so rows can be reordered by drag-and-drop.
    m_hoverItem = nullptr;
    auto *list = new ReorderableList();
    m_trackList = list;
    list->setFrameShape(QFrame::NoFrame);
    list->setStyleSheet("QListWidget { background: transparent; border: none; }"
                        "QListWidget::item { border: none; }"
                        "QListWidget::item:selected { background: transparent; }");
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setSpacing(4);
    list->setMouseTracking(true);
    list->viewport()->setMouseTracking(true);
    list->viewport()->installEventFilter(this);   // catch Leave to reset hover
    // Drag-to-reorder only makes sense inside a real playlist.
    if (!isStandalone) {
        list->setDragDropMode(QAbstractItemView::InternalMove);
        list->setSelectionMode(QAbstractItemView::SingleSelection);
        list->setDefaultDropAction(Qt::MoveAction);
    } else {
        list->setSelectionMode(QAbstractItemView::NoSelection);
    }

    for (int i = 0; i < tracks.size(); ++i) {
        auto &track = tracks[i];
        bool active = (track.id == currentTrackId);

        auto *row = new QWidget();
        row->setObjectName("trackRow");
        row->setFixedHeight(52);
        row->setAttribute(Qt::WA_TransparentForMouseEvents);  // list handles click/drag
        row->setStyleSheet(QString(
            "QWidget#trackRow { background: %1; border-radius: 8px; border-left: 3px solid %2; }"
        ).arg(active ? Theme::accentRgba(0.12) : QStringLiteral("transparent"),
              active ? Theme::accent().name() : "transparent"));

        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(12, 4, 12, 4);
        layout->setSpacing(12);

        QString idxNum = QString("%1").arg(i + 1, 2, 10, QChar('0'));
        auto *idx = new QLabel(active ? QStringLiteral("\uE102") : idxNum);
        idx->setFont(Theme::monoFont(12));
        idx->setFixedWidth(28);
        idx->setAlignment(Qt::AlignCenter);
        idx->setStyleSheet(QString("color: %1; background: transparent; font-family: \"Segoe MDL2 Assets\", Consolas;").arg(active ? Theme::accent().name() : Theme::textMuted().name()));
        idx->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(idx);
        // Stash hover info so itemEntered can swap the number for a play glyph.
        row->setProperty("idxLabel", QVariant::fromValue<QObject *>(idx));
        row->setProperty("idxNum", idxNum);
        row->setProperty("rowActive", active);

        auto *swatch = new QWidget();
        swatch->setFixedSize(38, 38);
        swatch->setAttribute(Qt::WA_TransparentForMouseEvents);
        swatch->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 6px;")
            .arg(track.cover.c1.name(), track.cover.c2.name()));
        layout->addWidget(swatch);

        auto *titleLabel = new QLabel(track.title);
        titleLabel->setFont(Theme::bodyFont(13));
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        titleLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(active ? Theme::accent().name() : Theme::text().name()));
        layout->addWidget(titleLabel, 1);

        // Add to queue button
        auto *enqueueBtn = new QPushButton(QStringLiteral("\uE710"));
        enqueueBtn->setFixedSize(28, 28);
        enqueueBtn->setCursor(Qt::PointingHandCursor);
        enqueueBtn->setFont(Theme::iconFont(11));
        enqueueBtn->setToolTip("Adicionar à fila");
        enqueueBtn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: none; font-family: \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::accent().name()));
        Track eqt = track;
        connect(enqueueBtn, &QPushButton::clicked, [this, eqt]() { emit enqueueRequested(eqt); });
        layout->addWidget(enqueueBtn);

        // Move to playlist button
        auto *moveBtn = new QPushButton("\uE188");
        moveBtn->setFixedSize(28, 28);
        moveBtn->setCursor(Qt::PointingHandCursor);
        moveBtn->setFont(Theme::iconFont(11));
        moveBtn->setToolTip(isStandalone ? "Adicionar à playlist" : "Mover para outra playlist");
        moveBtn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: none; font-family: \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::accent().name()));
        int mid = track.id;
        connect(moveBtn, &QPushButton::clicked, [this, mid]() { showMoveDialog(mid); });
        layout->addWidget(moveBtn);

        auto *likeBtn = new QPushButton(track.liked ? "\uE00B" : "\uE006");
        likeBtn->setFixedSize(28, 28);
        likeBtn->setCursor(Qt::PointingHandCursor);
        likeBtn->setStyleSheet(QString("QPushButton { background: transparent; color: %1; border: none; font-size: 14px; font-family: \"Segoe MDL2 Assets\"; }").arg(
            track.liked ? Theme::accent().name() : Theme::textMuted().name()));
        int lid = track.id;
        connect(likeBtn, &QPushButton::clicked, [this, lid]() { emit likeToggled(lid); });
        layout->addWidget(likeBtn);

        auto *dur = new QLabel(Theme::formatTime(track.durationMs));
        dur->setFont(Theme::monoFont(11));
        dur->setFixedWidth(40);
        dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        dur->setAttribute(Qt::WA_TransparentForMouseEvents);
        dur->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
        layout->addWidget(dur);

        auto *editBtn = new QPushButton(QStringLiteral("\uE70F"));
        editBtn->setFixedSize(24, 24);
        editBtn->setCursor(Qt::PointingHandCursor);
        editBtn->setToolTip("Editar m\u00FAsica");
        editBtn->setStyleSheet(QString("QPushButton { background: transparent; color: %1; border: none; font-size: 11px; font-family: \"Segoe MDL2 Assets\"; } QPushButton:hover { color: %2; }").arg(
            Theme::textMuted().name(), Theme::accent().name()));
        Track et = track;
        connect(editBtn, &QPushButton::clicked, [this, et]() { emit editTrackRequested(et); });
        layout->addWidget(editBtn);

        auto *delBtn = new QPushButton("\uE107");
        delBtn->setFixedSize(24, 24);
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setStyleSheet(QString("QPushButton { background: transparent; color: %1; border: none; font-size: 11px; font-family: \"Segoe MDL2 Assets\"; } QPushButton:hover { color: %2; }").arg(
            Theme::textMuted().name(), Theme::danger().name()));
        int did = track.id;
        connect(delBtn, &QPushButton::clicked, [this, did]() { emit deleteRequested(did); });
        layout->addWidget(delBtn);

        auto *listItem = new QListWidgetItem(list);
        listItem->setSizeHint(QSize(0, 52));
        listItem->setData(Qt::UserRole, track.id);
        list->setItemWidget(listItem, row);
    }

    // Fit the list inside the page's scroll area (no inner scrollbar).
    list->setFixedHeight(tracks.size() * 60 + 12);

    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
        int id = it->data(Qt::UserRole).toInt();
        if (Track *t = m_model->findTrack(id)) emit playRequested(*t);
    });
    connect(list, &QListWidget::itemEntered, this, [this](QListWidgetItem *it) {
        if (m_hoverItem == it) return;
        restoreHover();
        m_hoverItem = it;
        QWidget *w = m_trackList->itemWidget(it);
        if (!w || w->property("rowActive").toBool()) return;
        if (auto *lbl = qobject_cast<QLabel *>(w->property("idxLabel").value<QObject *>()))
            lbl->setText(QStringLiteral(""));   // play glyph
    });
    connect(list, &ReorderableList::moveRequested, this, [this](int from, int to) {
        QList<int> ids;
        for (int i = 0; i < m_trackList->count(); ++i)
            ids << m_trackList->item(i)->data(Qt::UserRole).toInt();
        if (from < 0 || from >= ids.size()) return;
        ids.move(from, to);
        m_model->reorderPlaylist(m_folderName, ids);
    });

    m_contentLayout->addWidget(list);
    m_contentLayout->addStretch();
}

void FolderDetailPage::restoreHover() {
    if (!m_hoverItem || !m_trackList) { m_hoverItem = nullptr; return; }
    if (QWidget *w = m_trackList->itemWidget(m_hoverItem)) {
        if (!w->property("rowActive").toBool())
            if (auto *lbl = qobject_cast<QLabel *>(w->property("idxLabel").value<QObject *>()))
                lbl->setText(w->property("idxNum").toString());
    }
    m_hoverItem = nullptr;
}

bool FolderDetailPage::eventFilter(QObject *obj, QEvent *event) {
    if (m_trackList && obj == m_trackList->viewport() && event->type() == QEvent::Leave)
        restoreHover();
    return QWidget::eventFilter(obj, event);
}

void FolderDetailPage::showEditDialog() {
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle("Editar Playlist");
    dlg->setFixedSize(380, 300);
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
    layout->setSpacing(12);

    auto *nameLabel = new QLabel("Nome da playlist");
    nameLabel->setFont(Theme::bodyFont(12));
    layout->addWidget(nameLabel);

    auto *nameEdit = new QLineEdit(m_folderName);
    nameEdit->setFont(Theme::bodyFont(13));
    nameEdit->selectAll();
    layout->addWidget(nameEdit);

    // Get current cover
    Theme::GradientPair currentCover;
    QString currentImage;
    for (auto &f : m_model->folders()) {
        if (f.name == m_folderName) { currentCover = f.cover; currentImage = f.coverImage; break; }
    }

    auto *colorLabel = new QLabel("Cores da capa:");
    colorLabel->setFont(Theme::bodyFont(12));
    layout->addWidget(colorLabel);

    QColor *c1 = new QColor(currentCover.c1.isValid() ? currentCover.c1 : Theme::accent());
    QColor *c2 = new QColor(currentCover.c2.isValid() ? currentCover.c2 : Theme::danger());

    auto *colorRow = new QHBoxLayout();
    colorRow->setSpacing(8);

    auto *preview = new QWidget();
    preview->setFixedSize(50, 32);
    preview->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 6px;")
        .arg(c1->name(), c2->name()));
    colorRow->addWidget(preview);

    auto updatePrev = [preview, c1, c2]() {
        preview->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 6px;")
            .arg(c1->name(), c2->name()));
    };

    auto *btn1 = new QPushButton("Cor 1");
    btn1->setFixedSize(60, 32);
    btn1->setCursor(Qt::PointingHandCursor);
    btn1->setFont(Theme::bodyFont(11));
    btn1->setStyleSheet(QString("background: %1; border: 2px solid rgba(255,255,255,0.3); border-radius: 6px; color: white;").arg(c1->name()));
    connect(btn1, &QPushButton::clicked, [btn1, c1, updatePrev, dlg]() {
        QColor chosen = QColorDialog::getColor(*c1, dlg, "Cor 1");
        if (chosen.isValid()) {
            *c1 = chosen;
            btn1->setStyleSheet(QString("background: %1; border: 2px solid rgba(255,255,255,0.3); border-radius: 6px; color: white;").arg(c1->name()));
            updatePrev();
        }
    });
    colorRow->addWidget(btn1);

    auto *btn2 = new QPushButton("Cor 2");
    btn2->setFixedSize(60, 32);
    btn2->setCursor(Qt::PointingHandCursor);
    btn2->setFont(Theme::bodyFont(11));
    btn2->setStyleSheet(QString("background: %1; border: 2px solid rgba(255,255,255,0.3); border-radius: 6px; color: white;").arg(c2->name()));
    connect(btn2, &QPushButton::clicked, [btn2, c2, updatePrev, dlg]() {
        QColor chosen = QColorDialog::getColor(*c2, dlg, "Cor 2");
        if (chosen.isValid()) {
            *c2 = chosen;
            btn2->setStyleSheet(QString("background: %1; border: 2px solid rgba(255,255,255,0.3); border-radius: 6px; color: white;").arg(c2->name()));
            updatePrev();
        }
    });
    colorRow->addWidget(btn2);
    colorRow->addStretch();
    layout->addLayout(colorRow);

    // Optional cover image (overrides the gradient when set)
    auto *imageLabel = new QLabel("Imagem da capa:");
    imageLabel->setFont(Theme::bodyFont(12));
    layout->addWidget(imageLabel);

    QString *imagePath = new QString(currentImage);
    auto *imageRow = new QHBoxLayout();
    imageRow->setSpacing(8);

    auto *imgPreview = new QLabel();
    imgPreview->setFixedSize(50, 32);
    imgPreview->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(Theme::bg().name()));
    if (!currentImage.isEmpty()) {
        QPixmap pm = Theme::roundedCover(currentImage, 50, 32, 6);
        if (!pm.isNull()) imgPreview->setPixmap(pm);
    }
    imageRow->addWidget(imgPreview);

    auto *pickImageBtn = new QPushButton("Escolher imagem");
    pickImageBtn->setFont(Theme::bodyFont(11));
    pickImageBtn->setFixedHeight(32);
    pickImageBtn->setCursor(Qt::PointingHandCursor);
    pickImageBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; border-radius: 8px; padding: 0 10px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); }"
    ).arg(Theme::textSoft().name(), Theme::border().name()));
    connect(pickImageBtn, &QPushButton::clicked, [dlg, imagePath, imgPreview]() {
        QString file = QFileDialog::getOpenFileName(dlg, "Escolher imagem da capa", QString(),
            "Imagens (*.png *.jpg *.jpeg *.bmp *.webp)");
        if (file.isEmpty()) return;
        *imagePath = file;
        QPixmap pm = Theme::roundedCover(file, 50, 32, 6);
        if (!pm.isNull()) imgPreview->setPixmap(pm);
    });
    imageRow->addWidget(pickImageBtn);

    auto *clearImageBtn = new QPushButton("Remover");
    clearImageBtn->setFont(Theme::bodyFont(11));
    clearImageBtn->setFixedHeight(32);
    clearImageBtn->setCursor(Qt::PointingHandCursor);
    clearImageBtn->setToolTip("Voltar a usar o gradiente de cores");
    clearImageBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; border-radius: 8px; padding: 0 10px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); }"
    ).arg(Theme::textSoft().name(), Theme::border().name()));
    connect(clearImageBtn, &QPushButton::clicked, [imagePath, imgPreview]() {
        imagePath->clear();
        imgPreview->setPixmap(QPixmap());
        imgPreview->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(Theme::bg().name()));
    });
    imageRow->addWidget(clearImageBtn);
    imageRow->addStretch();
    layout->addLayout(imageRow);

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
    connect(cancelBtn, &QPushButton::clicked, [dlg, c1, c2, imagePath]() { delete c1; delete c2; delete imagePath; dlg->reject(); });
    btnRow->addWidget(cancelBtn);

    auto *saveBtn = new QPushButton("Salvar");
    saveBtn->setFont(Theme::bodyFont(12));
    saveBtn->setFixedHeight(36);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 18px; padding: 0 20px; font-weight: bold; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::accent().name(), Theme::bg().name(), Theme::accent().lighter(110).name()));
    connect(saveBtn, &QPushButton::clicked, [this, dlg, nameEdit, c1, c2, imagePath, currentImage]() {
        QString newName = nameEdit->text().trimmed();
        if (newName.isEmpty()) return;
        m_model->updatePlaylistCover(m_folderId, *c1, *c2);
        // Only touch the image when it changed, to avoid re-copying the file.
        if (*imagePath != currentImage)
            m_model->updatePlaylistCoverImage(m_folderId, *imagePath);
        if (newName != m_folderName) {
            // Update m_folderName first: renamePlaylist emits tracksChanged,
            // which refreshes this page and must use the new name.
            m_folderName = newName;
            m_model->renamePlaylist(m_folderId, newName);
        }
        delete c1; delete c2; delete imagePath;
        dlg->accept();
    });
    btnRow->addWidget(saveBtn);
    layout->addLayout(btnRow);

    connect(nameEdit, &QLineEdit::returnPressed, saveBtn, &QPushButton::click);
    dlg->exec();
}

void FolderDetailPage::showMoveDialog(int trackId) {
    auto playlists = m_model->folders();

    auto *menu = new QMenu(this);
    menu->setStyleSheet(QString(
        "QMenu { background: %1; border: 1px solid %2; border-radius: 8px; padding: 4px; color: %3; }"
        "QMenu::item { padding: 8px 16px; border-radius: 4px; }"
        "QMenu::item:selected { background: %4; }"
        "QMenu::separator { height: 1px; background: %2; margin: 4px 0; }"
    ).arg(Theme::card().name(), Theme::border().name(), Theme::text().name(), Theme::cardHover().name()));

    bool addedAny = false;
    for (auto &f : playlists) {
        if (f.id == m_folderId) continue;
        QString targetName = f.name;
        int targetId = f.id;
        menu->addAction(f.name, [this, trackId, targetId, targetName]() {
            m_model->moveTrackToPlaylist(trackId, targetId, targetName);
        });
        addedAny = true;
    }

    if (m_folderId != 0) {
        if (addedAny) menu->addSeparator();
        menu->addAction("Remover da playlist", [this, trackId]() {
            m_model->moveTrackToPlaylist(trackId, 0, "");
        });
    }

    if (menu->actions().isEmpty()) {
        menu->addAction("Nenhuma playlist disponível")->setEnabled(false);
    }

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void FolderDetailPage::showCoverLightbox(const QString &imagePath) {
    QPixmap full(imagePath);
    if (full.isNull()) return;

    auto *dlg = new QDialog(this);
    dlg->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setAttribute(Qt::WA_TranslucentBackground);
    dlg->setModal(true);
    dlg->setStyleSheet("QDialog { background: rgba(0,0,0,0.88); }");

    // Cover the whole application window.
    QWidget *top = window();
    if (top) dlg->setGeometry(QRect(top->mapToGlobal(QPoint(0, 0)), top->size()));

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(18);

    // Backdrop click closes the lightbox.
    auto *backdrop = new QPushButton(dlg);
    backdrop->setGeometry(0, 0, dlg->width(), dlg->height());
    backdrop->setStyleSheet("background: transparent; border: none;");
    backdrop->setCursor(Qt::PointingHandCursor);
    backdrop->lower();
    connect(backdrop, &QPushButton::clicked, dlg, &QDialog::accept);

    layout->addStretch();

    int side = top ? qMin(top->height() - 200, top->width() - 120) : 520;
    side = qBound(240, side, 640);
    auto *imgLabel = new QLabel();
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet("background: transparent;");
    imgLabel->setPixmap(full.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(imgLabel, 0, Qt::AlignCenter);

    auto *closeBtn = new QPushButton("Close");
    closeBtn->setFont(Theme::bodyFont(13));
    closeBtn->setFixedHeight(38);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: none; padding: 0 24px; font-weight: bold; }"
        "QPushButton:hover { color: rgba(255,255,255,0.7); }");
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);

    layout->addStretch();
    dlg->exec();
}
