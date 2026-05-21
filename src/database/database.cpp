#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

Database &Database::instance() {
    static Database db;
    return db;
}

bool Database::open() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dir + "/vinil.db");

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qWarning() << "Database::open failed:" << m_lastError;
        return false;
    }

    applySchema();
    return true;
}

void Database::applySchema() {
    QSqlQuery q;
    q.exec("PRAGMA journal_mode = WAL");
    q.exec("PRAGMA foreign_keys = ON");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS folders (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            name         TEXT    NOT NULL UNIQUE,
            cover_color1 TEXT    NOT NULL DEFAULT '#e8a44a',
            cover_color2 TEXT    NOT NULL DEFAULT '#d45d5d',
            cover_image  TEXT    NOT NULL DEFAULT '',
            created_at   INTEGER NOT NULL DEFAULT (strftime('%s','now')*1000)
        )
    )");

    // Migration for databases created before cover_image existed.
    // Fails harmlessly when the column is already present.
    q.exec("ALTER TABLE folders ADD COLUMN cover_image TEXT NOT NULL DEFAULT ''");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS tracks (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            title        TEXT    NOT NULL,
            artist       TEXT    NOT NULL DEFAULT 'Desconhecido',
            folder_id    INTEGER NOT NULL DEFAULT 0,
            file_path    TEXT    NOT NULL,
            duration_ms  INTEGER NOT NULL DEFAULT 0,
            cover_color1 TEXT    NOT NULL DEFAULT '#e8a44a',
            cover_color2 TEXT    NOT NULL DEFAULT '#d45d5d',
            liked        INTEGER NOT NULL DEFAULT 0 CHECK (liked IN (0,1)),
            added_at     INTEGER NOT NULL DEFAULT (strftime('%s','now')*1000),
            play_count   INTEGER NOT NULL DEFAULT 0,
            last_played_at INTEGER NOT NULL DEFAULT 0,
            position     INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE SET DEFAULT
        )
    )");

    // Migrations for databases created before these columns existed.
    // Each fails harmlessly when the column is already present.
    q.exec("ALTER TABLE tracks ADD COLUMN last_played_at INTEGER NOT NULL DEFAULT 0");
    q.exec("ALTER TABLE tracks ADD COLUMN position INTEGER NOT NULL DEFAULT 0");
    // Seed positions with added_at so existing playlists keep a stable order.
    q.exec("UPDATE tracks SET position = added_at WHERE position = 0");

    q.exec("CREATE INDEX IF NOT EXISTS idx_tracks_folder ON tracks(folder_id)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_tracks_liked  ON tracks(liked) WHERE liked = 1");
    q.exec("CREATE INDEX IF NOT EXISTS idx_tracks_added  ON tracks(added_at DESC)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_tracks_played ON tracks(play_count DESC) WHERE play_count > 0");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS playback_state (
            id               INTEGER PRIMARY KEY CHECK (id = 1),
            current_track_id INTEGER NOT NULL DEFAULT 0,
            position_ms      INTEGER NOT NULL DEFAULT 0,
            volume           REAL    NOT NULL DEFAULT 0.7 CHECK (volume >= 0.0 AND volume <= 1.0),
            shuffle          INTEGER NOT NULL DEFAULT 0 CHECK (shuffle IN (0,1)),
            repeat_mode      INTEGER NOT NULL DEFAULT 0 CHECK (repeat_mode IN (0,1))
        )
    )");

    q.exec("INSERT OR IGNORE INTO playback_state (id) VALUES (1)");
    q.exec("INSERT OR IGNORE INTO folders (id, name, cover_color1, cover_color2) VALUES (0, '', '#e8a44a', '#d45d5d')");
}


int Database::findOrCreateFolder(const QString &name, const QColor &c1, const QColor &c2) {
    QSqlQuery q;
    q.prepare("SELECT id FROM folders WHERE name = ?");
    q.addBindValue(name);
    q.exec();
    if (q.next()) return q.value(0).toInt();

    q.prepare("INSERT INTO folders (name, cover_color1, cover_color2) VALUES (?, ?, ?)");
    q.addBindValue(name);
    q.addBindValue(c1.name());
    q.addBindValue(c2.name());
    q.exec();
    return q.lastInsertId().toInt();
}

QList<Folder> Database::allFolders() {
    QList<Folder> result;
    QSqlQuery q("SELECT id, name, cover_color1, cover_color2, cover_image FROM folders WHERE id != 0 ORDER BY name");
    while (q.next()) {
        Folder f;
        f.id         = q.value(0).toInt();
        f.name       = q.value(1).toString();
        f.cover.c1   = QColor(q.value(2).toString());
        f.cover.c2   = QColor(q.value(3).toString());
        f.coverImage = q.value(4).toString();
        result.append(f);
    }
    return result;
}

int Database::createFolder(const QString &name, const QColor &c1, const QColor &c2,
                           const QString &coverImage) {
    QSqlQuery q;
    q.prepare("INSERT OR IGNORE INTO folders (name, cover_color1, cover_color2, cover_image) VALUES (?, ?, ?, ?)");
    q.addBindValue(name);
    q.addBindValue(c1.name());
    q.addBindValue(c2.name());
    // Bind a non-null empty string; cover_image is NOT NULL and a null QString
    // would bind as SQL NULL and fail the constraint.
    q.addBindValue(coverImage.isEmpty() ? QString("") : coverImage);
    q.exec();
    if (q.lastInsertId().toInt() > 0) return q.lastInsertId().toInt();
    // already existed - return its id
    q.prepare("SELECT id FROM folders WHERE name = ?");
    q.addBindValue(name);
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

void Database::renameFolder(int id, const QString &newName) {
    QSqlQuery q;
    q.prepare("UPDATE folders SET name = ? WHERE id = ?");
    q.addBindValue(newName);
    q.addBindValue(id);
    q.exec();
    // Update all tracks that referenced this folder name
    q.prepare("UPDATE tracks SET folder_id = folder_id WHERE folder_id = ?");
    q.addBindValue(id);
    q.exec();
}

void Database::updateFolderCover(int id, const QColor &c1, const QColor &c2) {
    QSqlQuery q;
    q.prepare("UPDATE folders SET cover_color1 = ?, cover_color2 = ? WHERE id = ?");
    q.addBindValue(c1.name());
    q.addBindValue(c2.name());
    q.addBindValue(id);
    q.exec();
}

void Database::updateFolderCoverImage(int id, const QString &imagePath) {
    QSqlQuery q;
    q.prepare("UPDATE folders SET cover_image = ? WHERE id = ?");
    // Bind a non-null empty string when clearing; cover_image is NOT NULL, so a
    // null QString would bind as SQL NULL and the UPDATE would silently fail.
    q.addBindValue(imagePath.isEmpty() ? QString("") : imagePath);
    q.addBindValue(id);
    q.exec();
}

QString Database::importCoverImage(const QString &sourcePath) {
    if (sourcePath.isEmpty()) return QString();

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/covers";
    QDir().mkpath(dir);

    QFileInfo fi(sourcePath);
    QString dest = QString("%1/cover_%2.%3")
        .arg(dir)
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(fi.suffix().isEmpty() ? "png" : fi.suffix());

    if (QFile::copy(sourcePath, dest)) return dest;
    return sourcePath;  // fall back to referencing the original file
}

void Database::deleteFolder(int id) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET folder_id = 0 WHERE folder_id = ?");
    q.addBindValue(id);
    q.exec();
    q.prepare("DELETE FROM folders WHERE id = ?");
    q.addBindValue(id);
    q.exec();
}

void Database::moveTrackToFolder(int trackId, int folderId) {
    QSqlQuery q;
    // New position lands the track at the end of the target playlist.
    q.prepare("UPDATE tracks SET folder_id = ?, position = ? WHERE id = ?");
    q.addBindValue(folderId);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(trackId);
    q.exec();
}

void Database::setTrackPosition(int id, qint64 position) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET position = ? WHERE id = ?");
    q.addBindValue(position);
    q.addBindValue(id);
    q.exec();
}


Track Database::rowToTrack(const QSqlQuery &q) const {
    Track t;
    t.id         = q.value(0).toInt();
    t.title      = q.value(1).toString();
    t.artist     = q.value(2).toString();
    t.folderId   = q.value(3).toInt();
    t.audioUrl   = QUrl::fromLocalFile(q.value(4).toString());
    t.durationMs = q.value(5).toLongLong();
    t.cover.c1   = QColor(q.value(6).toString());
    t.cover.c2   = QColor(q.value(7).toString());
    t.liked      = q.value(8).toInt() == 1;
    t.addedAt    = q.value(9).toLongLong();
    t.playCount  = q.value(10).toInt();
    t.folder     = q.value(11).toString();
    t.lastPlayedAt = q.value(12).toLongLong();
    t.position   = q.value(13).toLongLong();
    return t;
}

int Database::insertTrack(const Track &t, int folderId) {
    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO tracks (title, artist, folder_id, file_path, cover_color1, cover_color2, position)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(t.title);
    q.addBindValue(t.artist);
    q.addBindValue(folderId);
    q.addBindValue(t.audioUrl.toLocalFile());
    q.addBindValue(t.cover.c1.name());
    q.addBindValue(t.cover.c2.name());
    // Timestamp position keeps new tracks at the end until manually reordered.
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.exec();
    return q.lastInsertId().toInt();
}

void Database::updateTrack(int id, const QString &title, const QString &artist) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET title = ?, artist = ? WHERE id = ?");
    q.addBindValue(title);
    q.addBindValue(artist.isEmpty() ? QString("Desconhecido") : artist);
    q.addBindValue(id);
    q.exec();
}

void Database::deleteTrack(int id) {
    QSqlQuery q;
    q.prepare("DELETE FROM tracks WHERE id = ?");
    q.addBindValue(id);
    q.exec();
}

void Database::setLiked(int id, bool liked) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET liked = ? WHERE id = ?");
    q.addBindValue(liked ? 1 : 0);
    q.addBindValue(id);
    q.exec();
}

void Database::setDuration(int id, qint64 ms) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET duration_ms = ? WHERE id = ?");
    q.addBindValue(ms);
    q.addBindValue(id);
    q.exec();
}

void Database::markPlayed(int id) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET play_count = play_count + 1, last_played_at = ? WHERE id = ?");
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(id);
    q.exec();
}

void Database::incrementPlayCount(int id) {
    QSqlQuery q;
    q.prepare("UPDATE tracks SET play_count = play_count + 1 WHERE id = ?");
    q.addBindValue(id);
    q.exec();
}

QList<Track> Database::allTracks() {
    QList<Track> result;
    QSqlQuery q(R"(
        SELECT t.id, t.title, t.artist, t.folder_id, t.file_path,
               t.duration_ms, t.cover_color1, t.cover_color2,
               t.liked, t.added_at, t.play_count,
               COALESCE(f.name, '') AS folder_name,
               t.last_played_at, t.position
        FROM   tracks t
        LEFT JOIN folders f ON f.id = t.folder_id
        ORDER  BY t.added_at DESC
    )");
    while (q.next()) result.append(rowToTrack(q));
    return result;
}


Database::PlaybackState Database::loadState() {
    PlaybackState s;
    QSqlQuery q("SELECT current_track_id, position_ms, volume, shuffle, repeat_mode "
                "FROM playback_state WHERE id = 1");
    if (q.next()) {
        s.trackId = q.value(0).toInt();
        s.posMs   = q.value(1).toLongLong();
        s.volume  = q.value(2).toDouble();
        s.shuffle = q.value(3).toInt() == 1;
        s.repeat  = q.value(4).toInt() == 1;
    }
    return s;
}

void Database::saveState(const PlaybackState &s) {
    QSqlQuery q;
    q.prepare(R"(
        UPDATE playback_state
        SET current_track_id = ?, position_ms = ?, volume = ?, shuffle = ?, repeat_mode = ?
        WHERE id = 1
    )");
    q.addBindValue(s.trackId);
    q.addBindValue(s.posMs);
    q.addBindValue(s.volume);
    q.addBindValue(s.shuffle ? 1 : 0);
    q.addBindValue(s.repeat  ? 1 : 0);
    q.exec();
}
