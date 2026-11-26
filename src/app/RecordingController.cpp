// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "RecordingController.h"

#include "DeckController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

namespace jx {

RecordingController::RecordingController(AudioEngine* engine, QList<DeckController*> decks, QObject* parent)
    : QObject(parent), m_engine(engine), m_decks(std::move(decks)) {
    QSettings s;
    m_folder = s.value("recording/folder").toString();
    if (m_folder.isEmpty()) {
#ifdef Q_OS_ANDROID
        // En Android las apps sólo pueden escribir libremente en su propia carpeta
        m_folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Grabaciones";
#else
        m_folder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/Jocker-MX Grabaciones";
#endif
    }
    for (DeckController* d : m_decks) {
        connect(d, &DeckController::played, this, [this, d](const QString& p) { onPlayed(d->index(), p); });
    }
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &RecordingController::tick);
}

RecordingController::~RecordingController() { stop(); }

void RecordingController::setFolder(const QString& f) {
    const QString p = f.startsWith(QLatin1String("file:")) ? QUrl(f).toLocalFile() : f;
    if (p == m_folder || p.isEmpty()) return;
    m_folder = p;
    QSettings().setValue("recording/folder", m_folder);
    emit folderChanged();
}

bool RecordingController::start() {
    if (recording()) return true;
    const QString name = QStringLiteral("Mezcla %1.wav").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH.mm.ss"));
    const QString path = QDir(m_folder).filePath(name);
    if (!m_engine->recorder().start(path, m_engine->sampleRate())) {
        m_error = QStringLiteral("No se pudo crear %1").arg(path);
        emit recordingChanged();
        return false;
    }
    m_error.clear();
    m_lastFile = path;
    m_tracks.clear();
    // Las pistas que ya suenan entran al inicio de la lista
    for (DeckController* d : m_decks)
        if (d->playing() && d->loaded()) m_tracks.append({0.0, d->artist(), d->title(), d->filePath()});
    m_timer.start();
    emit recordingChanged();
    return true;
}

void RecordingController::stop() {
    if (!recording()) return;
    m_engine->recorder().stop();
    m_timer.stop();
    writeTracklist();
    emit recordingChanged();
    emit tick();
}

void RecordingController::onPlayed(int deck, const QString& path) {
    Q_UNUSED(path);
    if (!recording()) return;
    DeckController* d = m_decks.value(deck);
    if (!d) return;
    m_tracks.append({m_engine->recorder().seconds(), d->artist(), d->title(), d->filePath()});
}

void RecordingController::writeTracklist() {
    if (m_lastFile.isEmpty()) return;
    const QFileInfo wav(m_lastFile);
    const QString base = wav.absolutePath() + "/" + wav.completeBaseName();
    auto stamp = [](double s) {
        const int t = static_cast<int>(s);
        return QStringLiteral("%1:%2:%3").arg(t / 3600, 2, 10, QChar('0')).arg((t / 60) % 60, 2, 10, QChar('0')).arg(t % 60, 2, 10, QChar('0'));
    };
    {   // Lista legible
        QFile f(base + ".txt");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream o(&f);
            o << "Jocker-MX — " << wav.completeBaseName() << "\n\n";
            int n = 1;
            for (const Entry& e : m_tracks)
                o << QStringLiteral("%1. [%2] ").arg(n++, 2, 10, QChar('0')).arg(stamp(e.at))
                  << (e.artist.isEmpty() ? QString() : e.artist + " - ") << e.title << "\n";
        }
    }
    {   // Hoja CUE (para quemar CD o dividir la mezcla por pistas)
        QFile f(base + ".cue");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream o(&f);
            o << "PERFORMER \"Jocker-MX\"\nTITLE \"" << wav.completeBaseName() << "\"\nFILE \"" << wav.fileName() << "\" WAVE\n";
            int n = 1;
            for (const Entry& e : m_tracks) {
                const int frames75 = static_cast<int>(e.at * 75);
                o << QStringLiteral("  TRACK %1 AUDIO\n").arg(n++, 2, 10, QChar('0'));
                o << "    TITLE \"" << QString(e.title).replace('"', '\'') << "\"\n";
                if (!e.artist.isEmpty()) o << "    PERFORMER \"" << QString(e.artist).replace('"', '\'') << "\"\n";
                o << QStringLiteral("    INDEX 01 %1:%2:%3\n").arg(frames75 / 75 / 60, 2, 10, QChar('0'))
                         .arg((frames75 / 75) % 60, 2, 10, QChar('0')).arg(frames75 % 75, 2, 10, QChar('0'));
            }
        }
    }
}

} // namespace jx
