// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "LibraryDb.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>
#include <QDebug>

namespace jx {

namespace {
QString g_path;
QMutex g_mutex;

const char* kSchema[] = {
    "CREATE TABLE IF NOT EXISTS tracks ("
    " id INTEGER PRIMARY KEY, path TEXT UNIQUE NOT NULL, folder TEXT, title TEXT, artist TEXT, album TEXT,"
    " genre TEXT, musical_key TEXT, year TEXT, format TEXT, duration REAL DEFAULT 0, tag_bpm REAL DEFAULT 0,"
    " bpm REAL DEFAULT 0, first_beat REAL DEFAULT -1, grid_locked INTEGER DEFAULT 0, cue REAL DEFAULT -1,"
    " hotcues TEXT, size INTEGER DEFAULT 0, mtime INTEGER DEFAULT 0, added INTEGER DEFAULT 0,"
    " play_count INTEGER DEFAULT 0, last_played INTEGER DEFAULT 0, rating INTEGER DEFAULT 0, search TEXT)",
    "CREATE INDEX IF NOT EXISTS idx_tracks_folder ON tracks(folder)",
    "CREATE TABLE IF NOT EXISTS history (id INTEGER PRIMARY KEY, track_id INTEGER, played_at INTEGER, deck INTEGER)",
    "CREATE INDEX IF NOT EXISTS idx_history_track ON history(track_id)",
    "CREATE TABLE IF NOT EXISTS meta (k TEXT PRIMARY KEY, v TEXT)",
    "INSERT OR IGNORE INTO meta(k, v) VALUES('schema', '1')",
};

QString hotToText(const QVector<double>& h) {
    QStringList l;
    for (double v : h) l << QString::number(v, 'f', 4);
    return l.join(',');
}
QVector<double> textToHot(const QString& s) {
    QVector<double> h(8, -1.0);
    const QStringList l = s.split(',', Qt::SkipEmptyParts);
    for (int i = 0; i < l.size() && i < 8; ++i) h[i] = l[i].toDouble();
    return h;
}
qint64 now() { return QDateTime::currentSecsSinceEpoch(); }

// Minúsculas y sin acentos: "Canción Ñandú" → "cancion nandu"
QString fold(const QString& in) {
    const QString d = in.normalized(QString::NormalizationForm_D);
    QString out;
    out.reserve(d.size());
    for (const QChar c : d)
        if (c.category() != QChar::Mark_NonSpacing) out.append(c.toLower());
    return out;
}
} // namespace

bool LibraryDb::open(const QString& p) {
    {
        QMutexLocker lock(&g_mutex);
        if (p.isEmpty()) {
            const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(dir);
            g_path = dir + "/biblioteca.sqlite";
        } else {
            g_path = p;
        }
    }
    QSqlDatabase d = db();
    if (!d.isOpen()) return false;
    QSqlQuery q(d);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    for (const char* s : kSchema) {
        if (!q.exec(QString::fromLatin1(s))) {
            qWarning() << "Error de esquema:" << q.lastError().text();
            return false;
        }
    }
    q.exec("ALTER TABLE tracks ADD COLUMN search TEXT");   // migración 0.2 (falla sin daño si ya existe)
    return true;
}

QString LibraryDb::path() { QMutexLocker lock(&g_mutex); return g_path; }

QSqlDatabase LibraryDb::db() {
    const QString name = QStringLiteral("jx-%1").arg(reinterpret_cast<quintptr>(QThread::currentThread()));
    if (QSqlDatabase::contains(name)) {
        QSqlDatabase d = QSqlDatabase::database(name);
        if (d.isOpen()) return d;
    }
    QSqlDatabase d = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    d.setDatabaseName(path());
    d.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    if (!d.open()) qWarning() << "No se pudo abrir la biblioteca:" << d.lastError().text();
    return d;
}

QHash<QString, qint64> LibraryDb::knownMtimes(const QString& folderPrefix) {
    QHash<QString, qint64> out;
    QSqlQuery q(db());
    q.prepare("SELECT path, mtime FROM tracks WHERE path LIKE ? ESCAPE '\\'");
    QString pre = folderPrefix;
    pre.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
    q.addBindValue(pre + "%");
    q.exec();
    while (q.next()) out.insert(q.value(0).toString(), q.value(1).toLongLong());
    return out;
}

void LibraryDb::upsertScanned(const QList<ScanEntry>& entries) {
    if (entries.isEmpty()) return;
    QSqlDatabase d = db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare("INSERT INTO tracks(path, folder, title, artist, album, genre, musical_key, year, format, duration,"
              " tag_bpm, size, mtime, added, search) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
              " ON CONFLICT(path) DO UPDATE SET folder=excluded.folder, title=excluded.title, artist=excluded.artist,"
              " album=excluded.album, genre=excluded.genre, musical_key=excluded.musical_key, year=excluded.year,"
              " format=excluded.format, duration=CASE WHEN excluded.duration > 0 THEN excluded.duration ELSE tracks.duration END,"
              " tag_bpm=excluded.tag_bpm, size=excluded.size, mtime=excluded.mtime, search=excluded.search");
    const qint64 t = now();
    for (const ScanEntry& e : entries) {
        const QFileInfo fi(e.path);
        q.addBindValue(e.path);
        q.addBindValue(fi.absolutePath());
        q.addBindValue(e.tags.title);
        q.addBindValue(e.tags.artist);
        q.addBindValue(e.tags.album);
        q.addBindValue(e.tags.genre);
        q.addBindValue(e.tags.key);
        q.addBindValue(e.tags.year);
        q.addBindValue(fi.suffix().toUpper());
        q.addBindValue(e.tags.duration);
        q.addBindValue(e.tags.bpm);
        q.addBindValue(e.size);
        q.addBindValue(e.mtime);
        q.addBindValue(t);
        q.addBindValue(fold(e.tags.title + ' ' + e.tags.artist + ' ' + e.tags.album + ' ' + e.tags.genre + ' ' +
                            fi.completeBaseName()));
        if (!q.exec()) qWarning() << "Error al guardar" << e.path << q.lastError().text();
    }
    d.commit();
}

int LibraryDb::removeMissing(const QString& folderPrefix, const QSet<QString>& present) {
    const QHash<QString, qint64> known = knownMtimes(folderPrefix);
    QSqlDatabase d = db();
    d.transaction();
    QSqlQuery q(d);
    q.prepare("DELETE FROM tracks WHERE path = ?");
    int n = 0;
    for (auto it = known.begin(); it != known.end(); ++it) {
        if (present.contains(it.key())) continue;
        q.addBindValue(it.key());
        q.exec();
        ++n;
    }
    d.commit();
    return n;
}

QList<TrackRow> LibraryDb::query(View view, const QString& folderPrefix, const QString& filter,
                                 const QString& sortKey, bool ascending, int limit) {
    QStringList where;
    QVariantList binds;
    if (!folderPrefix.isEmpty() && view == View::All) {
        QString pre = folderPrefix;
        pre.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
        where << "t.path LIKE ? ESCAPE '\\'";
        binds << pre + "%";
    }
    static const QRegularExpression bpmRe(QStringLiteral("^bpm:(\\d+(?:\\.\\d+)?)(?:-(\\d+(?:\\.\\d+)?))?$"));
    for (const QString& w : filter.split(' ', Qt::SkipEmptyParts)) {
        const auto m = bpmRe.match(w.toLower());
        if (m.hasMatch()) {
            const double a = m.captured(1).toDouble();
            const double b = m.captured(2).isEmpty() ? a + 0.999 : m.captured(2).toDouble();
            where << "(CASE WHEN t.bpm > 0 THEN t.bpm ELSE t.tag_bpm END) BETWEEN ? AND ?";
            binds << a << b;
        } else if (w.startsWith("key:", Qt::CaseInsensitive)) {
            where << "t.musical_key LIKE ?";
            binds << w.mid(4) + "%";
        } else {
            where << "(t.search LIKE ? OR t.path LIKE ?)";
            binds << "%" + fold(w) + "%" << "%" + w + "%";
        }
    }

    QString sql = "SELECT t.id, t.path, t.title, t.artist, t.album, t.genre, t.musical_key, t.year, t.format,"
                  " t.duration, CASE WHEN t.bpm > 0 THEN t.bpm ELSE t.tag_bpm END, t.play_count, t.last_played, t.added";
    QString order;
    const QString dir = ascending ? " ASC" : " DESC";
    if (sortKey == "bpm") order = "11" + dir;
    else if (sortKey == "title") order = "t.title COLLATE NOCASE" + dir;
    else if (sortKey == "album") order = "t.album COLLATE NOCASE" + dir;
    else if (sortKey == "genre") order = "t.genre COLLATE NOCASE" + dir;
    else if (sortKey == "duration") order = "t.duration" + dir;
    else if (sortKey == "key") order = "t.musical_key" + dir;
    else if (sortKey == "plays") order = "t.play_count" + dir;
    else order = "t.artist COLLATE NOCASE" + dir + ", t.title COLLATE NOCASE";

    switch (view) {
    case View::History:
        sql += ", h.played_at FROM history h JOIN tracks t ON t.id = h.track_id";
        order = "h.played_at DESC, h.id DESC";
        break;
    case View::MostPlayed:
        sql += ", 0 FROM tracks t";
        where << "t.play_count > 0";
        order = "t.play_count DESC, t.last_played DESC";
        break;
    case View::RecentlyAdded:
        sql += ", 0 FROM tracks t";
        order = "t.added DESC, t.id DESC";
        break;
    case View::All:
        sql += ", 0 FROM tracks t";
        break;
    }
    if (!where.isEmpty()) sql += " WHERE " + where.join(" AND ");
    sql += " ORDER BY " + order + QStringLiteral(" LIMIT %1").arg(limit);

    QList<TrackRow> rows;
    QSqlQuery q(db());
    q.prepare(sql);
    for (const QVariant& b : binds) q.addBindValue(b);
    if (!q.exec()) {
        qWarning() << "Consulta fallida:" << q.lastError().text();
        return rows;
    }
    while (q.next()) {
        TrackRow r;
        r.id = q.value(0).toLongLong();
        r.path = q.value(1).toString();
        r.title = q.value(2).toString();
        r.artist = q.value(3).toString();
        r.album = q.value(4).toString();
        r.genre = q.value(5).toString();
        r.key = q.value(6).toString();
        r.year = q.value(7).toString();
        r.format = q.value(8).toString();
        r.duration = q.value(9).toDouble();
        r.bpm = q.value(10).toDouble();
        r.playCount = q.value(11).toInt();
        r.lastPlayed = view == View::History ? q.value(14).toLongLong() : q.value(12).toLongLong();
        r.added = q.value(13).toLongLong();
        rows << r;
    }
    return rows;
}

int LibraryDb::count(const QString& folderPrefix) {
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM tracks WHERE path LIKE ? ESCAPE '\\'");
    QString pre = folderPrefix;
    pre.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
    q.addBindValue(pre + "%");
    return q.exec() && q.next() ? q.value(0).toInt() : 0;
}

QStringList LibraryDb::pathsNeedingAnalysis(const QString& folderPrefix) {
    QStringList out;
    QSqlQuery q(db());
    q.prepare("SELECT path FROM tracks WHERE path LIKE ? ESCAPE '\\' AND bpm = 0 AND grid_locked = 0 ORDER BY artist, title");
    QString pre = folderPrefix;
    pre.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
    q.addBindValue(pre + "%");
    if (q.exec()) while (q.next()) out << q.value(0).toString();
    return out;
}

void LibraryDb::ensureTrack(QSqlDatabase& d, const QString& path) {
    QSqlQuery q(d);
    q.prepare("INSERT OR IGNORE INTO tracks(path, folder, format, added) VALUES(?,?,?,?)");
    const QFileInfo fi(path);
    q.addBindValue(path);
    q.addBindValue(fi.absolutePath());
    q.addBindValue(fi.suffix().toUpper());
    q.addBindValue(now());
    q.exec();
}

TrackState LibraryDb::state(const QString& path) {
    TrackState s;
    QSqlQuery q(db());
    q.prepare("SELECT bpm, first_beat, cue, hotcues, grid_locked FROM tracks WHERE path = ?");
    q.addBindValue(path);
    if (q.exec() && q.next()) {
        s.found = true;
        s.bpm = q.value(0).toDouble();
        s.firstBeat = q.value(1).toDouble();
        s.cue = q.value(2).toDouble();
        s.hotCues = textToHot(q.value(3).toString());
        s.gridLocked = q.value(4).toInt() != 0;
    }
    return s;
}

void LibraryDb::saveAnalysis(const QString& path, double bpm, double firstBeat, double duration, const TrackTags& tags,
                             const QString& camelotKey) {
    QSqlDatabase d = db();
    ensureTrack(d, path);
    QSqlQuery q(d);
    q.prepare("UPDATE tracks SET duration = ?,"
              " bpm = CASE WHEN grid_locked = 1 THEN bpm ELSE ? END,"
              " first_beat = CASE WHEN grid_locked = 1 THEN first_beat ELSE ? END,"
              " title = CASE WHEN IFNULL(title,'') = '' THEN ? ELSE title END,"
              " artist = CASE WHEN IFNULL(artist,'') = '' THEN ? ELSE artist END,"
              " musical_key = CASE WHEN IFNULL(musical_key,'') = '' THEN ? ELSE musical_key END"
              " WHERE path = ?");
    q.addBindValue(duration);
    q.addBindValue(bpm);
    q.addBindValue(firstBeat);
    q.addBindValue(tags.title);
    q.addBindValue(tags.artist);
    q.addBindValue(camelotKey);
    q.addBindValue(path);
    q.exec();
}

void LibraryDb::saveCue(const QString& path, double cue) {
    QSqlDatabase d = db();
    ensureTrack(d, path);
    QSqlQuery q(d);
    q.prepare("UPDATE tracks SET cue = ? WHERE path = ?");
    q.addBindValue(cue);
    q.addBindValue(path);
    q.exec();
}

void LibraryDb::saveHotCues(const QString& path, const QVector<double>& hot) {
    QSqlDatabase d = db();
    ensureTrack(d, path);
    QSqlQuery q(d);
    q.prepare("UPDATE tracks SET hotcues = ? WHERE path = ?");
    q.addBindValue(hotToText(hot));   // segundos
    q.addBindValue(path);
    q.exec();
}

void LibraryDb::saveGrid(const QString& path, double bpm, double firstBeat) {
    QSqlDatabase d = db();
    ensureTrack(d, path);
    QSqlQuery q(d);
    q.prepare("UPDATE tracks SET bpm = ?, first_beat = ?, grid_locked = 1 WHERE path = ?");
    q.addBindValue(bpm);
    q.addBindValue(firstBeat);
    q.addBindValue(path);
    q.exec();
}

void LibraryDb::addHistory(const QString& path, int deck) {
    QSqlDatabase d = db();
    ensureTrack(d, path);
    QSqlQuery q(d);
    q.prepare("UPDATE tracks SET play_count = play_count + 1, last_played = ? WHERE path = ?");
    q.addBindValue(now());
    q.addBindValue(path);
    q.exec();
    q.prepare("INSERT INTO history(track_id, played_at, deck) SELECT id, ?, ? FROM tracks WHERE path = ?");
    q.addBindValue(now());
    q.addBindValue(deck);
    q.addBindValue(path);
    q.exec();
}

} // namespace jx
