#ifndef FOLDERDETAILPAGE_H
#define FOLDERDETAILPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include "trackmodel.h"

class QListWidget;
class QListWidgetItem;

class FolderDetailPage : public QWidget {
    Q_OBJECT
public:
    explicit FolderDetailPage(TrackModel *model, QWidget *parent = nullptr);
    void setFolder(const QString &folderName);
    void refresh(int currentTrackId, bool isPlaying);

signals:
    void playRequested(const Track &track);
    void likeToggled(int id);
    void deleteRequested(int id);
    void enqueueRequested(const Track &track);
    void editTrackRequested(const Track &track);
    void navigateBack();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void showEditDialog();
    void showMoveDialog(int trackId);
    void showCoverLightbox(const QString &imagePath);
    void restoreHover();

    TrackModel *m_model;
    QString m_folderName;
    int m_folderId = 0;
    QVBoxLayout *m_contentLayout;
    QListWidget *m_trackList = nullptr;
    QListWidgetItem *m_hoverItem = nullptr;
};

#endif // FOLDERDETAILPAGE_H
