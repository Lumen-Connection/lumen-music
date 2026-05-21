#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include "trackmodel.h"
#include "playerbar.h"
#include "homepage.h"
#include "addmusicpage.h"
#include "folderspage.h"
#include "folderdetailpage.h"
#include "likedpage.h"
#include "queuepage.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:
    void themeChangeRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void navigateTo(const QString &page, const QString &data = "");
    void refreshCurrentPage();
    void onTrackPlay(const Track &track);
    void showThemePicker();

private:
    void buildSidebar(QWidget *sidebar);
    void refreshSidebarFolders();
    void showToast(const QString &text);
    void repositionToast();
    void showEditTrackDialog(const Track &track);
    void confirmDeleteTrack(const Track &track);

    TrackModel *m_model;
    PlayerBar *m_playerBar;

    QStackedWidget *m_stack;
    HomePage *m_homePage;
    AddMusicPage *m_addPage;
    FoldersPage *m_foldersPage;
    FolderDetailPage *m_folderDetailPage;
    LikedPage *m_likedPage;
    QueuePage *m_queuePage;

    // Sidebar
    QPushButton *m_navHome;
    QPushButton *m_navAdd;
    QPushButton *m_navFolders;
    QVBoxLayout *m_sidebarFoldersLayout;
    QWidget *m_sidebarFoldersContainer;
    QLabel *m_trackCountLabel;

    QString m_currentPage;

    QLabel *m_toast = nullptr;
    QTimer *m_toastTimer = nullptr;
};

#endif // MAINWINDOW_H