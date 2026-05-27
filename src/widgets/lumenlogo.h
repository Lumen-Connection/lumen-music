#ifndef LUMENLOGO_H
#define LUMENLOGO_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QIcon>

// Draws the Lumen Music mark — a broken diamond outline (four corner chevrons)
// in the brand orange-red, with a solid coral diamond at its centre — into the
// given w×h area.
inline void paintLumenMark(QPainter &p, qreal w, qreal h) {
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF c(w / 2.0, h / 2.0);
    const qreal r = qMin(w, h) / 2.0 * 0.82;          // distance to outer vertices

    const QPointF T(c.x(), c.y() - r), R(c.x() + r, c.y()),
                  B(c.x(), c.y() + r), L(c.x() - r, c.y());

    // Inner filled diamond (coral).
    const qreal ir = r * 0.5;
    QPainterPath inner;
    inner.moveTo(c.x(), c.y() - ir);
    inner.lineTo(c.x() + ir, c.y());
    inner.lineTo(c.x(), c.y() + ir);
    inner.lineTo(c.x() - ir, c.y());
    inner.closeSubpath();
    p.fillPath(inner, QColor("#e0703c"));

    // Outer broken outline: a chevron at each vertex, with gaps at edge midpoints.
    QPen pen(QColor("#ee4914"));
    pen.setWidthF(qMax(2.0, r * 0.2));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::MiterJoin);
    p.setPen(pen);

    auto along = [](QPointF a, QPointF b, qreal t) {
        return QPointF(a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t);
    };
    const qreal armEnd = 0.32;   // how far each arm reaches toward the next vertex
    auto chevron = [&](QPointF v, QPointF n1, QPointF n2) {
        QPainterPath path;
        path.moveTo(along(v, n1, armEnd));
        path.lineTo(v);
        path.lineTo(along(v, n2, armEnd));
        p.drawPath(path);
    };
    chevron(T, R, L);
    chevron(R, T, B);
    chevron(B, L, R);
    chevron(L, B, T);
}

inline QPixmap lumenLogoPixmap(int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintLumenMark(p, size, size);
    return pm;
}

// Multi-resolution app/window icon.
inline QIcon lumenLogoIcon() {
    QIcon icon;
    for (int s : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(lumenLogoPixmap(s));
    return icon;
}

// Sidebar logo widget.
class LumenLogo : public QWidget {
public:
    explicit LumenLogo(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        paintLumenMark(p, width(), height());
    }
};

#endif // LUMENLOGO_H
