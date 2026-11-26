// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "LibraryModel.h"

#include "analysis/Analyzer.h"
#include "analysis/KeyDetector.h"
#include "library/TagReader.h"

#include <QDirIterator>
#include <algorithm>
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

namespace jx {

LibraryModel::LibraryModel(QObject* parent) : QAbstractListModel(parent) {
    connect(&m_watcher, &QFutureWatcherBase::finished, this, [this]() {
        m_progressTimer.stop();
        m_scanning = false;
        emit scanningChanged();
        emit scanProgress();
        refresh();
    });
    m_filterDebounce.setSingleShot(true);
    m_filterDebounce.setInterval(120);
    connect(&m_filterDebounce, &QTimer::timeout, this, &LibraryModel::refresh);
    m_progressTimer.setInterval(250);
    connect(&m_progressTimer, &QTimer::timeout, this, [this]() { emit scanProgress(); emit analysisChanged(); });
    connect(&m_analysisWatcher, &QFutureWatcherBase::finished, this, [this]() {
        m_analyzing = false;
        if (!m_scanning) m_progressTimer.stop();
        emit analysisChanged();
        refresh();
    });

    QSettings s;
    QString f = s.value("library/folder").toString();
    if (f.isEmpty()) f = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_sortKey = s.value("library/sort", "artist").toString();
    m_asc = s.value("library/asc", true).toBool();
    setFolder(f);
    reloadPlaylists();
}

int LibraryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const TrackRow& r = m_rows[index.row()];
    switch (role) {
    case PathRole: return r.path;
    case TitleRole: {
        if (!r.title.isEmpty()) return r.title;
        QString t, a;
        guessTitleArtist(r.path, &t, &a);
        return t;
    }
    case ArtistRole: {
        if (!r.artist.isEmpty()) return r.artist;
        QString t, a;
        guessTitleArtist(r.path, &t, &a);
        return a;
    }
    case AlbumRole: return r.album;
    case GenreRole: return r.genre;
    case KeyRole: return r.key;
    case BpmRole: return r.bpm;
    case DurationRole: return r.duration;
    case FormatRole: return r.format;
    case PlaysRole: return r.playCount;
    case LastPlayedRole: return r.lastPlayed;
    case FileNameRole: return QFileInfo(r.path).fileName();
    default: return {};
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const {
    return {{PathRole, "path"},     {TitleRole, "title"},    {ArtistRole, "artist"},   {AlbumRole, "album"},
            {GenreRole, "genre"},   {KeyRole, "musicalKey"}, {BpmRole, "bpm"},         {DurationRole, "duration"},
            {FormatRole, "format"}, {PlaysRole, "plays"},    {LastPlayedRole, "lastPlayed"}, {FileNameRole, "fileName"}};
}

void LibraryModel::setFolder(const QString& f) {
    QString path = f.startsWith(QLatin1String("file:")) ? QUrl(f).toLocalFile() : f;
    while (path.size() > 1 && (path.endsWith('/') || path.endsWith('\\'))) path.chop(1);
    if (path == m_folder) return;
    m_folder = path;
    QSettings().setValue("library/folder", m_folder);
    emit folderChanged();
    refresh();
    rescan();
}

void LibraryModel::setFilter(const QString& f) {
    if (f == m_filter) return;
    m_filter = f;
    emit filterChanged();
    m_filterDebounce.start();
}

void LibraryModel::setView(int v) {
    if (v == m_view) return;
    if (m_view == 4 && m_sortKey == QLatin1String("pos")) {   // al salir de una lista, volver al orden guardado
        QSettings s;
        m_sortKey = s.value("library/sort", "artist").toString();
        m_asc = s.value("library/asc", true).toBool();
        emit sortChanged();
    }
    m_view = v;
    emit viewChanged();
    refresh();
}

void LibraryModel::setPlaylistId(qint64 id) {
    if (id == m_playlistId && m_view == 4) return;
    m_playlistId = id;
    m_view = 4;
    m_sortKey = QStringLiteral("pos");   // orden propio de la lista
    m_asc = true;
    emit sortChanged();
    emit viewChanged();
    refresh();
}

void LibraryModel::reloadPlaylists() {
    m_playlists.clear();
    for (const PlaylistInfo& p : LibraryDb::playlists())
        m_playlists << QVariantMap{{"id", p.id}, {"name", p.name}, {"count", p.count}};
    emit playlistsChanged();
}

QStringList LibraryModel::visiblePaths() const {
    QStringList out;
    out.reserve(m_rows.size());
    for (const TrackRow& r : m_rows) out << r.path;
    return out;
}

QStringList LibraryModel::playlistPaths(qint64 id) const { return LibraryDb::playlistPaths(id); }

qint64 LibraryModel::createPlaylist(const QString& name, const QStringList& paths) {
    const qint64 id = LibraryDb::createPlaylist(name);
    if (id && !paths.isEmpty()) LibraryDb::setPlaylistPaths(id, paths);
    reloadPlaylists();
    return id;
}

void LibraryModel::renamePlaylist(qint64 id, const QString& name) {
    if (LibraryDb::renamePlaylist(id, name)) reloadPlaylists();
}

void LibraryModel::deletePlaylist(qint64 id) {
    LibraryDb::deletePlaylist(id);
    if (m_view == 4 && m_playlistId == id) setView(0);
    reloadPlaylists();
}

void LibraryModel::addToPlaylist(qint64 id, const QStringList& paths) {
    QStringList clean;
    for (const QString& p : paths)
        clean << (p.startsWith(QLatin1String("file:")) ? QUrl(p).toLocalFile() : p);
    LibraryDb::addToPlaylist(id, clean);
    reloadPlaylists();
    if (m_view == 4 && m_playlistId == id) refresh();
}

void LibraryModel::removeFromPlaylist(int row) {
    if (m_view != 4 || row < 0 || row >= m_rows.size()) return;
    QStringList paths = LibraryDb::playlistPaths(m_playlistId);
    const int pos = m_rows[row].position;
    if (pos < 0 || pos >= paths.size()) return;
    paths.removeAt(pos);
    LibraryDb::setPlaylistPaths(m_playlistId, paths);
    reloadPlaylists();
    refresh();
}

void LibraryModel::movePlaylistItem(int from, int to) {
    if (m_view != 4 || from < 0 || from >= m_rows.size()) return;
    QStringList paths = LibraryDb::playlistPaths(m_playlistId);
    const int a = m_rows[from].position;
    const int b = std::clamp(to, 0, static_cast<int>(paths.size()) - 1);
    if (a < 0 || a >= paths.size() || a == b) return;
    paths.move(a, b);
    LibraryDb::setPlaylistPaths(m_playlistId, paths);
    refresh();
}

void LibraryModel::sortBy(const QString& key) {
    if (key == m_sortKey) m_asc = !m_asc;
    else { m_sortKey = key; m_asc = key != QLatin1String("plays"); }
    QSettings s;
    s.setValue("library/sort", m_sortKey);
    s.setValue("library/asc", m_asc);
    emit sortChanged();
    refresh();
}

void LibraryModel::refresh() {
    const QString prefix = m_folder.isEmpty() ? QString() : m_folder + '/';
    beginResetModel();
    const auto view = static_cast<LibraryDb::View>(m_view);
    // En una lista se respeta su orden; ordenar por columna sólo cambia la vista.
    const QString sort = view == LibraryDb::View::Playlist && m_sortKey == QLatin1String("pos") ? QString() : m_sortKey;
    m_rows = LibraryDb::query(view, prefix, m_filter, view == LibraryDb::View::Playlist ? sort : m_sortKey,
                              view == LibraryDb::View::Playlist && sort.isEmpty() ? true : m_asc, 20000, m_playlistId);
    endResetModel();
    m_total = prefix.isEmpty() ? 0 : LibraryDb::count(prefix);
    emit countsChanged();
}

void LibraryModel::rescan() {
    if (m_folder.isEmpty() || m_watcher.isRunning()) return;
    m_scanning = true;
    m_scannedCount = 0;
    emit scanningChanged();
    m_progressTimer.start();
    const QString root = m_folder;
    std::atomic<int>* counter = &m_scannedCount;
    QPointer<LibraryModel> self(this);
    m_watcher.setFuture(QtConcurrent::run([root, counter, self]() {
        const QString prefix = root + '/';
        const QHash<QString, qint64> known = LibraryDb::knownMtimes(prefix);
        static const QStringList exts = {"*.mp3", "*.wav", "*.flac", "*.wave", "*.ogg", "*.oga", "*.m4a", "*.aac", "*.mp4", "*.aif", "*.aiff",
                                         "*.MP3", "*.WAV", "*.FLAC", "*.OGG", "*.M4A", "*.AAC", "*.AIF", "*.AIFF"};
        QDirIterator it(root, exts, QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        QSet<QString> present;
        QList<ScanEntry> batch;
        int batches = 0;
        while (it.hasNext() && present.size() < 200000) {
            const QString p = it.next();
            const QFileInfo fi = it.fileInfo();
            present.insert(p);
            counter->fetch_add(1);
            const qint64 mtime = fi.lastModified().toSecsSinceEpoch();
            if (known.value(p, -1) == mtime) continue;       // sin cambios
            ScanEntry e;
            e.path = p;
            e.size = fi.size();
            e.mtime = mtime;
            e.tags = readTags(p);
            const QString cam = normalizeKeyToCamelot(e.tags.key);
            if (!cam.isEmpty()) e.tags.key = cam;
            batch << e;
            if (batch.size() >= 250) {
                LibraryDb::upsertScanned(batch);
                batch.clear();
                if (++batches % 4 == 1 && self)
                    QMetaObject::invokeMethod(self.data(), "refresh", Qt::QueuedConnection);
            }
        }
        LibraryDb::upsertScanned(batch);
        LibraryDb::removeMissing(prefix, present);
    }));
}

QString LibraryModel::pathAt(int row) const {
    return (row >= 0 && row < m_rows.size()) ? m_rows[row].path : QString();
}

void LibraryModel::setAnalysis(const QString& path, double bpm, const QString& key) {
    for (int r = 0; r < m_rows.size(); ++r) {
        if (m_rows[r].path != path) continue;
        m_rows[r].bpm = bpm;
        if (m_rows[r].key.isEmpty()) m_rows[r].key = key;
        emit dataChanged(index(r), index(r), {BpmRole, KeyRole});
    }
}

void LibraryModel::analyzeAll() {
    if (m_analyzing || m_folder.isEmpty()) return;
    const QStringList paths = LibraryDb::pathsNeedingAnalysis(m_folder + '/');
    if (paths.isEmpty()) return;
    m_analyzing = true;
    m_cancelAnalysis = false;
    m_analyzedCount = 0;
    m_analysisTotal = static_cast<int>(paths.size());
    emit analysisChanged();
    m_progressTimer.start();
    std::atomic<int>* done = &m_analyzedCount;
    std::atomic<bool>* cancel = &m_cancelAnalysis;
    QPointer<LibraryModel> self(this);
    // Un solo hilo de fondo: no compite con el audio mientras se mezcla
    m_analysisWatcher.setFuture(QtConcurrent::run([paths, done, cancel, self]() {
        int sinceRefresh = 0;
        for (const QString& p : paths) {
            if (cancel->load()) break;
            QString err;
            auto t = loadAndAnalyze(p, 44100.0, &err);
            if (t) {
                TrackTags tg;
                tg.title = t->title;
                tg.artist = t->artist;
                LibraryDb::saveAnalysis(p, t->bpm, t->firstBeat / t->sampleRate, t->durationSec(), tg, t->musicalKey);
            } else {
                LibraryDb::saveAnalysis(p, -1, -1, 0, TrackTags());      // marcar como no analizable
            }
            done->fetch_add(1);
            if (++sinceRefresh % 10 == 0 && self) QMetaObject::invokeMethod(self.data(), "refresh", Qt::QueuedConnection);
        }
    }));
}

void LibraryModel::onPlayed(const QString& path) {
    if (m_view != 0) { refresh(); return; }
    for (int r = 0; r < m_rows.size(); ++r) {
        if (m_rows[r].path != path) continue;
        m_rows[r].playCount++;
        emit dataChanged(index(r), index(r), {PlaysRole});
    }
}

} // namespace jx
