// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "LibraryModel.h"

#include "analysis/Analyzer.h"
#include "library/TagReader.h"

#include <QDirIterator>
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
    connect(&m_progressTimer, &QTimer::timeout, this, [this]() { emit scanProgress(); });

    QSettings s;
    QString f = s.value("library/folder").toString();
    if (f.isEmpty()) f = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_sortKey = s.value("library/sort", "artist").toString();
    m_asc = s.value("library/asc", true).toBool();
    setFolder(f);
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
    m_view = v;
    emit viewChanged();
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
    m_rows = LibraryDb::query(static_cast<LibraryDb::View>(m_view), prefix, m_filter, m_sortKey, m_asc);
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
        static const QStringList exts = {"*.mp3", "*.wav", "*.flac", "*.wave", "*.MP3", "*.WAV", "*.FLAC"};
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

void LibraryModel::setBpm(const QString& path, double bpm) {
    for (int r = 0; r < m_rows.size(); ++r) {
        if (m_rows[r].path != path) continue;
        m_rows[r].bpm = bpm;
        emit dataChanged(index(r), index(r), {BpmRole});
    }
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
