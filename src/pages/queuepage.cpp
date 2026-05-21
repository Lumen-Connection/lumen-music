#include "queuepage.h"
#include "playerbar.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>

QueuePage::QueuePage(TrackModel *model, PlayerBar *player, QWidget *parent)
    : QWidget(parent), m_model(model), m_player(player)
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

void QueuePage::refresh(int currentTrackId, bool /*isPlaying*/) {
    QLayoutItem *item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Back button
    auto *backBtn = new QPushButton("");
    backBtn->setFixedSize(34, 34);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFont(Theme::iconFont(12));
    backBtn->setStyleSheet(QString(
        "QPushButton { background: rgba(255,255,255,0.05); color: %1; border: none; border-radius: 17px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.1); }"
    ).arg(Theme::text().name()));
    connect(backBtn, &QPushButton::clicked, this, &QueuePage::navigateBack);
    m_contentLayout->addWidget(backBtn, 0, Qt::AlignLeft);

    auto *title = new QLabel("Fila");
    title->setFont(Theme::titleFont(28));
    title->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::text().name()));
    m_contentLayout->addWidget(title);
    m_contentLayout->addSpacing(8);

    Track current = m_player->currentTrack();
    QList<Track> userQueue = m_player->userQueue();
    QList<Track> upcoming   = m_player->upcomingContext();

    if (current.id == 0 && userQueue.isEmpty() && upcoming.isEmpty()) {
        auto *empty = new QLabel("A fila está vazia\nReproduza uma música ou adicione faixas à fila");
        empty->setFont(Theme::bodyFont(14));
        empty->setStyleSheet(QString("color: %1; background: transparent; padding-top: 40px;").arg(Theme::textMuted().name()));
        empty->setAlignment(Qt::AlignCenter);
        m_contentLayout->addWidget(empty);
        m_contentLayout->addStretch();
        return;
    }

    auto addSectionLabel = [this](const QString &text) {
        auto *label = new QLabel(text);
        label->setFont(Theme::bodyFont(11));
        label->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold; letter-spacing: 1px; padding-top: 10px;")
            .arg(Theme::textMuted().name()));
        m_contentLayout->addWidget(label);
    };

    // Now playing
    if (current.id != 0) {
        addSectionLabel("TOCANDO AGORA");
        m_contentLayout->addWidget(createRow(current, QString(), true, -1));
    }

    // Manually queued tracks
    if (!userQueue.isEmpty()) {
        addSectionLabel("PRÓXIMAS NA FILA");
        for (int i = 0; i < userQueue.size(); ++i)
            m_contentLayout->addWidget(createRow(userQueue[i], QString::number(i + 1), false, i));
    }

    // Rest of the current context
    if (!upcoming.isEmpty()) {
        addSectionLabel("A SEGUIR");
        for (int i = 0; i < upcoming.size(); ++i)
            m_contentLayout->addWidget(createRow(upcoming[i], QString(), upcoming[i].id == currentTrackId, -1));
    }

    m_contentLayout->addStretch();
}

QWidget *QueuePage::createRow(const Track &track, const QString &position, bool active, int queueIndex) {
    auto *row = new QWidget();
    row->setObjectName("trackRow");
    row->setFixedHeight(52);
    row->setCursor(Qt::PointingHandCursor);
    row->setStyleSheet(QString(
        "QWidget#trackRow { background: %1; border-radius: 8px; border-left: 3px solid %2; }"
    ).arg(active ? Theme::accentRgba(0.12) : QStringLiteral("transparent"),
          active ? Theme::accent().name() : "transparent"));

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(12);

    QString idxText = active ? QStringLiteral("") : position;
    auto *idx = new QLabel(idxText);
    idx->setFont(active ? Theme::iconFont(12) : Theme::monoFont(12));
    idx->setFixedWidth(28);
    idx->setAlignment(Qt::AlignCenter);
    idx->setStyleSheet(QString("color: %1; background: transparent; font-family: \"Segoe MDL2 Assets\", Consolas;")
        .arg(active ? Theme::accent().name() : Theme::textMuted().name()));
    idx->setAttribute(Qt::WA_TransparentForMouseEvents);  // let clicks reach the play overlay
    layout->addWidget(idx);

    auto *swatch = new QWidget();
    swatch->setFixedSize(38, 38);
    swatch->setAttribute(Qt::WA_TransparentForMouseEvents);
    swatch->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2); border-radius: 6px;")
        .arg(track.cover.c1.name(), track.cover.c2.name()));
    layout->addWidget(swatch);

    auto *infoCol = new QVBoxLayout();
    infoCol->setSpacing(1);
    auto *titleLabel = new QLabel(track.title);
    titleLabel->setFont(Theme::bodyFont(13));
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;")
        .arg(active ? Theme::accent().name() : Theme::text().name()));
    auto *artistLabel = new QLabel(track.artist);
    artistLabel->setFont(Theme::bodyFont(11));
    artistLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    artistLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSoft().name()));
    infoCol->addWidget(titleLabel);
    infoCol->addWidget(artistLabel);
    layout->addLayout(infoCol, 1);

    // Remove-from-queue button (only for manually queued items)
    if (queueIndex >= 0) {
        auto *removeBtn = new QPushButton("");
        removeBtn->setFixedSize(28, 28);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setFont(Theme::iconFont(11));
        removeBtn->setToolTip("Remover da fila");
        removeBtn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: none; font-family: \"Segoe MDL2 Assets\"; }"
            "QPushButton:hover { color: %2; }"
        ).arg(Theme::textMuted().name(), Theme::danger().name()));
        connect(removeBtn, &QPushButton::clicked, [this, queueIndex]() { emit removeFromQueueRequested(queueIndex); });
        layout->addWidget(removeBtn);
    }

    auto *dur = new QLabel(Theme::formatTime(track.durationMs));
    dur->setFont(Theme::monoFont(11));
    dur->setFixedWidth(40);
    dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dur->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textMuted().name()));
    layout->addWidget(dur);

    // Click overlay to play (not for the currently-playing row)
    if (!active) {
        Track t = track;
        auto *overlay = new QPushButton(row);
        overlay->setGeometry(0, 0, 9999, 52);
        overlay->setStyleSheet("background: transparent; border: none;");
        overlay->setCursor(Qt::PointingHandCursor);
        overlay->lower();
        if (queueIndex >= 0)
            connect(overlay, &QPushButton::clicked, [this, queueIndex]() { emit playFromQueue(queueIndex); });
        else
            connect(overlay, &QPushButton::clicked, [this, t]() { emit playContext(t); });
    }

    return row;
}
