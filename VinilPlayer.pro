QT       += core gui widgets multimedia sql

TARGET = LumenPlayer
TEMPLATE = app

CONFIG += c++17

# Windows executable icon + metadata.
# Use a custom .rc with a relative icon path: windres (MinGW) can't open the
# absolute path because it contains a non-ASCII character ("Área").
win32 {
    RC_FILE = resources/app.rc
    VERSION = 1.1.0
    QMAKE_TARGET_PRODUCT = Lumen Player
    QMAKE_TARGET_DESCRIPTION = Lumen Player
    QMAKE_TARGET_COMPANY = Lumen Connection

    HELPER_SRC = src/thirdparty/yt-dlp.exe
    HELPER_DST = $$OUT_PWD/yt-dlp.exe

    copyhelper.input = HELPER_SRC
    copyhelper.output = HELPER_DST
    copyhelper.commands = $$QMAKE_COPY $$shell_path($$HELPER_SRC) $$shell_path($$HELPER_DST)
    copyhelper.CONFIG += target_predeps no_link
    copyhelper.name = YT-DLP
    QMAKE_EXTRA_COMPILERS += copyhelper

    helper.files = src/thirdparty/yt-dlp.exe
    helper.path = $$INSTALL_PREFIX
    INSTALLS += helper
}

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
