QT       += core gui widgets multimedia sql

TARGET = VinilPlayer
TEMPLATE = app

CONFIG += c++17

INCLUDEPATH += \
    src \
    src/pages \
    src/player \
    src/widgets \
    src/database

SOURCES += \
    src/main.cpp \
    src/database/database.cpp \
    src/mainwindow.cpp \
    src/pages/homepage.cpp \
    src/pages/addmusicpage.cpp \
    src/pages/folderspage.cpp \
    src/pages/folderdetailpage.cpp \
    src/pages/likedpage.cpp \
    src/pages/queuepage.cpp \
    src/player/playerbar.cpp \
    src/player/trackmodel.cpp \
    src/widgets/vinylwidget.cpp \
    src/widgets/clickableslider.cpp

HEADERS += \
    src/mainwindow.h \
    src/database/database.h \
    src/pages/homepage.h \
    src/pages/addmusicpage.h \
    src/pages/folderspage.h \
    src/pages/folderdetailpage.h \
    src/pages/likedpage.h \
    src/pages/queuepage.h \
    src/player/playerbar.h \
    src/player/trackmodel.h \
    src/widgets/vinylwidget.h \
    src/widgets/clickableslider.h \
    src/widgets/theme.h \
    src/widgets/hoverplayfilter.h \
    src/widgets/reorderablelist.h \
    src/widgets/lumenlogo.h

RESOURCES += \
    resources/resources.qrc
