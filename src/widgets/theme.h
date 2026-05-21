#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QFont>
#include <QRandomGenerator>
#include <QList>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

namespace Theme {

struct ThemeData {
    QString id;
    QString name;
    QColor bg, surface, card, cardHover;
    QColor accent, accentDim;
    QColor text, textSoft, textMuted, border, danger, vinylBlack;
};

inline ThemeData warmTheme() {
    return { "warm", "Vinil Quente",
        {"#1a1712"}, {"#221f19"}, {"#2a2620"}, {"#342f28"},
        {"#e8a44a"}, {"#c4883a"},
        {"#f0ece4"}, {"#b8b0a2"}, {"#7a7266"}, {"#3a352d"},
        {"#d45d5d"}, {"#111111"} };
}

inline ThemeData oceanTheme() {
    return { "ocean", "Oceano",
        {"#0d1520"}, {"#111c2d"}, {"#162338"}, {"#1d2e47"},
        {"#4aa8e8"}, {"#3a8ec4"},
        {"#e4f0f8"}, {"#a0c0d8"}, {"#5a7a90"}, {"#1e3048"},
        {"#e85d5d"}, {"#080d14"} };
}

inline ThemeData forestTheme() {
    return { "forest", "Floresta",
        {"#121a12"}, {"#182018"}, {"#1f281f"}, {"#283328"},
        {"#6bcf7f"}, {"#52b066"},
        {"#e8f0e8"}, {"#a8c0a8"}, {"#6a8a6a"}, {"#2a3a2a"},
        {"#d45d5d"}, {"#080f08"} };
}

inline ThemeData purpleTheme() {
    return { "purple", "Roxo Noturno",
        {"#15101a"}, {"#1d1525"}, {"#261c30"}, {"#30233c"},
        {"#c084fc"}, {"#a066d8"},
        {"#f0e8f8"}, {"#c0a8d8"}, {"#7a6090"}, {"#352545"},
        {"#f05d7a"}, {"#0d0810"} };
}

inline ThemeData grayTheme() {
    return { "gray", "Cinza Moderno",
        {"#141414"}, {"#1c1c1c"}, {"#242424"}, {"#2e2e2e"},
        {"#e0e0e0"}, {"#bdbdbd"},
        {"#f5f5f5"}, {"#bdbdbd"}, {"#757575"}, {"#333333"},
        {"#ef5350"}, {"#0a0a0a"} };
}

// Lumen — deep black with the brand orange/coral of the diamond logo.
// Minimalist and luminous, inspired by lumenconnection.com.br.
inline ThemeData lumenTheme() {
    return { "lumen", "Lumen",
        {"#0a0a0c"}, {"#101013"}, {"#16161a"}, {"#1f1f25"},
        {"#ee4914"}, {"#e0703c"},
        {"#f5f5f7"}, {"#b4b4be"}, {"#71717a"}, {"#26262d"},
        {"#e5484d"}, {"#050506"} };
}

inline ThemeData& activeTheme() {
    static ThemeData t = lumenTheme();
    return t;
}

inline void setActiveTheme(const ThemeData &t) { activeTheme() = t; }

inline QList<ThemeData> allThemes() {
    return { lumenTheme(), warmTheme(), oceanTheme(), forestTheme(), purpleTheme(), grayTheme() };
}

inline ThemeData themeById(const QString &id) {
    for (const auto &t : allThemes())
        if (t.id == id) return t;
    return lumenTheme();
}

inline QColor bg()          { return activeTheme().bg; }
inline QColor surface()     { return activeTheme().surface; }
inline QColor card()        { return activeTheme().card; }
inline QColor cardHover()   { return activeTheme().cardHover; }
inline QColor accent()      { return activeTheme().accent; }
inline QColor accentDim()   { return activeTheme().accentDim; }
inline QColor text()        { return activeTheme().text; }
inline QColor textSoft()    { return activeTheme().textSoft; }
inline QColor textMuted()   { return activeTheme().textMuted; }
inline QColor border()      { return activeTheme().border; }
inline QColor danger()      { return activeTheme().danger; }
inline QColor vinylBlack()  { return activeTheme().vinylBlack; }

// Translucent accent for highlights (active rows, chips), so they track the theme.
inline QString accentRgba(double alpha) {
    QColor a = activeTheme().accent;
    return QString("rgba(%1,%2,%3,%4)").arg(a.red()).arg(a.green()).arg(a.blue()).arg(alpha);
}

struct GradientPair {
    QColor c1, c2;
};

inline QList<GradientPair> palettes() {
    return {
        { QColor("#e8a44a"), QColor("#d45d5d") },
        { QColor("#6b8f71"), QColor("#2c3e50") },
        { QColor("#c9a959"), QColor("#8b5e3c") },
        { QColor("#5d7b93"), QColor("#2b2d42") },
        { QColor("#d4a373"), QColor("#9c6644") },
        { QColor("#b07156"), QColor("#4a3228") },
        { QColor("#a3b18a"), QColor("#344e41") },
        { QColor("#dda15e"), QColor("#bc6c25") },
        { QColor("#8ecae6"), QColor("#023047") },
        { QColor("#cdb4db"), QColor("#5a189a") },
    };
}

inline GradientPair randomPalette() {
    auto p = palettes();
    return p[QRandomGenerator::global()->bounded(p.size())];
}

inline QFont titleFont(int size = 28) {
    QFont f("Segoe UI", size);
    f.setWeight(QFont::Black);
    return f;
}

inline QFont bodyFont(int size = 14) {
    QFont f("Segoe UI", size);
    f.setWeight(QFont::Medium);
    return f;
}

inline QFont monoFont(int size = 11) {
    QFont f("Consolas", size);
    f.setStyleHint(QFont::Monospace);
    return f;
}

inline QFont iconFont(int size = 14) {
    return QFont("Segoe MDL2 Assets", size);
}

inline QString globalStyleSheet() {
    const auto &t = activeTheme();
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: %3;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: %4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QScrollBar:horizontal {
            height: 0px;
        }
        QToolTip {
            background-color: %5;
            color: %2;
            border: 1px solid %3;
            padding: 4px 8px;
            border-radius: 4px;
        }
    )").arg(t.bg.name(), t.text.name(), t.border.name(),
            t.textMuted.name(), t.card.name());
}

// Center-cropped, rounded-corner cover image. Returns a null pixmap if the
// file cannot be loaded so callers can fall back to gradient covers.
inline QPixmap roundedCover(const QString &path, int w, int h, int radius) {
    QPixmap src(path);
    if (src.isNull()) return QPixmap();

    QPixmap scaled = src.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap result(w, h);
    result.fill(Qt::transparent);

    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(0, 0, w, h, radius, radius);
    p.setClipPath(clip);
    int x = (scaled.width()  - w) / 2;
    int y = (scaled.height() - h) / 2;
    p.drawPixmap(-x, -y, scaled);
    return result;
}

inline QString formatTime(qint64 ms) {
    int totalSec = static_cast<int>(ms / 1000);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    return QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
}

}  // namespace Theme

#endif // THEME_H
