// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "LibraryModel.h"

#include "analysis/Analyzer.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

namespace jx {

LibraryModel::LibraryModel(QObject* parent) : QAbstractListModel(parent) {
    connect(&m_watcher, &QFutureWatcherBase::finished, this, [this]() {
        beginResetModel();
        m_all = m_watcher.result();
        QSettings s;
        for (auto& e : m_all) e.bpm = s.value("bpm/" + e.path, 0.0).toDouble();
        endResetModel();
        m_scanning = false;
        emit scanningChanged();
        rebuildView();
    });
    QSettings s;
    QString f = s.value("library/folder").toString();
    if (f.isEmpty()) f = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    setFolder(f);
}

int LibraryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_view.size());
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_view.size()) return {};
    const Entry& e = m_all[m_view[index.row()]];
    switch (role) {
    case PathRole: return e.path;
    case TitleRole: return e.title;
    case ArtistRole: return e.artist;
    case BpmRole: return e.bpm;
    case FormatRole: return e.format;
    case FileNameRole: return QFileInfo(e.path).fileName();
    default: return {};
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const {
    return {{PathRole, "path"}, {TitleRole, "title"}, {ArtistRole, "artist"},
            {BpmRole, "bpm"},   {FormatRole, "format"}, {FileNameRole, "fileName"}};
}

void LibraryModel::setFolder(const QString& f) {
    QString path = f.startsWith(QLatin1String("file:")) ? QUrl(f).toLocalFile() : f;
    if (path == m_folder) return;
    m_folder = path;
    QSettings().setValue("library/folder", m_folder);
    emit folderChanged();
    rescan();
}

void LibraryModel::rescan() {
    if (m_folder.isEmpty() || m_watcher.isRunning()) return;
    m_scanning = true;
    emit scanningChanged();
    const QString root = m_folder;
    m_watcher.setFuture(QtConcurrent::run([root]() {
        QList<Entry> out;
        static const QStringList exts = {"*.mp3", "*.wav", "*.flac", "*.MP3", "*.WAV", "*.FLAC",
                                         "*.wave", "*.aif", "*.aiff"};
        QDirIterator it(root, exts, QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext() && out.size() < 50000) {
            const QString p = it.next();
            Entry e;
            e.path = p;
            guessTitleArtist(p, &e.title, &e.artist);
            e.format = QFileInfo(p).suffix().toUpper();
            e.search = (e.title + ' ' + e.artist + ' ' + QFileInfo(p).fileName()).toLower();
            out << e;
        }
        return out;
    }));
}

void LibraryModel::setFilter(const QString& f) {
    if (f == m_filter) return;
    m_filter = f;
    emit filterChanged();
    rebuildView();
}

void LibraryModel::sortBy(const QString& key) {
    m_sortKey = key;
    rebuildView();
}

void LibraryModel::rebuildView() {
    beginResetModel();
    m_view.clear();
    const QStringList words = m_filter.toLower().split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < m_all.size(); ++i) {
        bool ok = true;
        for (const QString& w : words) if (!m_all[i].search.contains(w)) { ok = false; break; }
        if (ok) m_view << i;
    }
    const QString key = m_sortKey;
    std::stable_sort(m_view.begin(), m_view.end(), [this, key](int a, int b) {
        const Entry& x = m_all[a];
        const Entry& y = m_all[b];
        if (key == QLatin1String("bpm")) return x.bpm < y.bpm;
        if (key == QLatin1String("title")) return QString::localeAwareCompare(x.title, y.title) < 0;
        const int c = QString::localeAwareCompare(x.artist, y.artist);
        return c != 0 ? c < 0 : QString::localeAwareCompare(x.title, y.title) < 0;
    });
    endResetModel();
    emit countsChanged();
}

QString LibraryModel::pathAt(int row) const {
    return (row >= 0 && row < m_view.size()) ? m_all[m_view[row]].path : QString();
}

void LibraryModel::setBpm(const QString& path, double bpm) {
    QSettings().setValue("bpm/" + path, bpm);
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_all[i].path != path) continue;
        m_all[i].bpm = bpm;
        const int r = static_cast<int>(m_view.indexOf(i));
        if (r >= 0) emit dataChanged(index(r), index(r), {BpmRole});
    }
}

} // namespace jx
