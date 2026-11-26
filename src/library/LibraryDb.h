// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "TagReader.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

namespace jx {

struct TrackRow {
    qint64 id = 0;
    QString path, title, artist, album, genre, key, year, format;
    double duration = 0, bpm = 0;
    int playCount = 0;
    qint64 lastPlayed = 0, added = 0;
    int position = 0;          // posición dentro de una lista
};

struct PlaylistInfo {
    qint64 id = 0;
    QString name;
    int count = 0;
};

struct TrackState {        // lo que un deck restaura al cargar
    bool found = false;
    double bpm = 0, firstBeat = -1, cue = -1;
    bool gridLocked = false;  // el usuario editó el beatgrid
    QVector<double> hotCues = QVector<double>(8, -1.0);
};

struct ScanEntry {
    QString path;
    qint64 size = 0, mtime = 0;
    TrackTags tags;
};

// Biblioteca persistente en SQLite. Cada hilo usa su propia conexión (connectionForThread).
class LibraryDb {
public:
    // Abre (o crea) la base de datos. Si path está vacío usa la carpeta de datos de la aplicación.
    static bool open(const QString& path = QString());
    static QString path();

    // Escaneo
    static QHash<QString, qint64> knownMtimes(const QString& folderPrefix);
    static void upsertScanned(const QList<ScanEntry>& entries);
    static int removeMissing(const QString& folderPrefix, const QSet<QString>& present);

    // Consultas
    enum class View { All, History, MostPlayed, RecentlyAdded, Playlist };
    static QList<TrackRow> query(View view, const QString& folderPrefix, const QString& filter,
                                 const QString& sortKey, bool ascending, int limit = 20000, qint64 playlistId = 0);
    static TrackRow trackInfo(const QString& path);   // datos básicos (o el nombre del archivo)

    // Listas de reproducción (guardan rutas, sobreviven a reescaneos)
    static QList<PlaylistInfo> playlists();
    static qint64 createPlaylist(const QString& name);
    static bool renamePlaylist(qint64 id, const QString& name);
    static void deletePlaylist(qint64 id);
    static QStringList playlistPaths(qint64 id);
    static void setPlaylistPaths(qint64 id, const QStringList& paths);
    static void addToPlaylist(qint64 id, const QStringList& paths);
    static int count(const QString& folderPrefix);
    static QStringList pathsNeedingAnalysis(const QString& folderPrefix);

    // Estado por pista
    static TrackState state(const QString& path);
    static void saveAnalysis(const QString& path, double bpm, double firstBeat, double duration,
                             const TrackTags& tags, const QString& camelotKey = QString());
    static void saveCue(const QString& path, double cue);
    static void saveHotCues(const QString& path, const QVector<double>& hot);
    static void saveGrid(const QString& path, double bpm, double firstBeat);
    static void addHistory(const QString& path, int deck);

private:
    static QSqlDatabase db();
    static void ensureTrack(QSqlDatabase& d, const QString& path);
};

} // namespace jx
