#ifndef HOVERPLAYFILTER_H
#define HOVERPLAYFILTER_H

#include <QObject>
#include <QEvent>
#include <QLabel>

// Swaps a track row's index label between its number and a play glyph while the
// mouse hovers the row — the Spotify-style "play icon next to the number".
// No Q_OBJECT macro is needed: only the virtual eventFilter is overridden.
class HoverPlayFilter : public QObject {
public:
    HoverPlayFilter(QLabel *idxLabel, const QString &number, QObject *parent)
        : QObject(parent), m_idx(idxLabel), m_number(number) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Enter)
            m_idx->setText(QStringLiteral("\uE102"));   // Play glyph
        else if (event->type() == QEvent::Leave)
            m_idx->setText(m_number);
        return QObject::eventFilter(obj, event);
    }

private:
    QLabel *m_idx;
    QString m_number;
};

#endif // HOVERPLAYFILTER_H
