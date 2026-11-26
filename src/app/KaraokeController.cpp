// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "KaraokeController.h"

#include "DeckController.h"
#include "Mixer.h"
#include "analysis/Analyzer.h"
#include "library/LibraryDb.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QUrl>
#include <algorithm>

namespace jx {

KaraokeController::KaraokeController(DeckController* a, DeckController* b, Mixer* mixer, QObject* parent)
    : QObject(parent), m_a(a), m_b(b), m_mixer(mixer) {
    QSettings s;
    m_autoPlay = s.value("karaoke/autoPlay", false).toBool();
    const QJsonArray arr = QJsonDocument::fromJson(s.value("karaoke/singers").toString().toUtf8()).array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        m_list << Singer{o.value("name").toString(), o.value("path").toString(), o.value("key").toInt()};
    }
}

DeckController* KaraokeController::deck(int i) const { return i == 0 ? m_a : m_b; }

bool KaraokeController::hasKaraokeFiles(const QString& audioPath) {
    const QFileInfo fi(audioPath.startsWith(QLatin1String("file:")) ? QUrl(audioPath).toLocalFile() : audioPath);
    const QString base = fi.dir().filePath(fi.completeBaseName());
    for (const char* ext : {".cdg", ".CDG", ".lrc", ".LRC"})
        if (QFileInfo::exists(base + ext)) return true;
    return false;
}

QVariantList KaraokeController::singers() const {
    QVariantList out;
    for (const Singer& s : m_list) {
        const TrackRow r = LibraryDb::trackInfo(s.path);
        QString title = r.title, artist = r.artist;
        if (artist.isEmpty()) guessTitleArtist(s.path, &title, &artist);
        out << QVariantMap{{"name", s.name}, {"path", s.path}, {"title", title}, {"artist", artist},
                           {"key", s.key}, {"karaoke", hasKaraokeFiles(s.path)}};
    }
    return out;
}

void KaraokeController::save() {
    QJsonArray arr;
    for (const Singer& s : m_list) arr.append(QJsonObject{{"name", s.name}, {"path", s.path}, {"key", s.key}});
    QSettings().setValue("karaoke/singers", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    emit singersChanged();
}

void KaraokeController::setAutoPlay(bool on) {
    if (on == m_autoPlay) return;
    m_autoPlay = on;
    QSettings().setValue("karaoke/autoPlay", on);
    emit settingsChanged();
}

void KaraokeController::addSinger(const QString& name, const QString& path, int key) {
    const QString p = path.startsWith(QLatin1String("file:")) ? QUrl(path).toLocalFile() : path;
    if (p.isEmpty()) return;
    m_list << Singer{name.trimmed().isEmpty() ? QStringLiteral("Cantante %1").arg(m_sung + m_list.size() + 1) : name.trimmed(),
                     p, std::clamp(key, -6, 6)};
    save();
}

void KaraokeController::removeSinger(int i) {
    if (i < 0 || i >= m_list.size()) return;
    m_list.removeAt(i);
    save();
}

void KaraokeController::moveSinger(int from, int to) {
    if (from < 0 || from >= m_list.size()) return;
    to = std::clamp(to, 0, int(m_list.size()) - 1);
    if (from == to) return;
    m_list.move(from, to);
    save();
}

void KaraokeController::setSingerKey(int i, int key) {
    if (i < 0 || i >= m_list.size()) return;
    m_list[i].key = std::clamp(key, -6, 6);
    save();
}

void KaraokeController::setSingerName(int i, const QString& name) {
    if (i < 0 || i >= m_list.size() || name.trimmed().isEmpty()) return;
    m_list[i].name = name.trimmed();
    save();
}

void KaraokeController::clear() {
    m_list.clear();
    save();
}

bool KaraokeController::startNext() {
    if (m_list.isEmpty()) return false;
    // Deck libre: el que no suena (si los dos están parados, el del turno anterior no; si no, A).
    int d;
    if (m_a->playing() && !m_b->playing()) d = 1;
    else if (m_b->playing() && !m_a->playing()) d = 0;
    else if (!m_a->playing() && !m_b->playing()) d = m_deck == 0 ? 1 : 0;
    else return false;   // los dos están sonando: no pisar una pista en curso

    const Singer s = m_list.takeFirst();
    save();
    m_deck = d;
    m_currentName = s.name;
    QString title, artist;
    guessTitleArtist(s.path, &title, &artist);
    const TrackRow r = LibraryDb::trackInfo(s.path);
    m_currentTitle = !r.artist.isEmpty() ? r.title : title;
    ++m_sung;
    emit currentChanged();

    DeckController* dc = deck(d);
    if (m_pending) disconnect(m_pending);
    const int key = s.key;
    const QString path = s.path;
    m_pending = connect(dc, &DeckController::trackLoaded, this, [this, dc, key, path, d](const QString& loaded) {
        if (loaded != path) return;
        disconnect(m_pending);
        dc->seekSeconds(0.0);          // el karaoke empieza desde el principio (intro de los gráficos)
        dc->setKeyShift(key);
        if (m_autoPlay) {
            m_mixer->setCrossfader(d == 0 ? -1.0 : 1.0);
            dc->play();
        }
    });
    dc->load(path);
    return true;
}

} // namespace jx
