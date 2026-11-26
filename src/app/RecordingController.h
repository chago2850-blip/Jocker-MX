// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "engine/AudioEngine.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace jx {

class DeckController;

// Graba la mezcla (WAV 24 bits) y genera la lista de canciones (.txt) y un .cue con los tiempos.
class RecordingController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(double seconds READ seconds NOTIFY tick)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString lastFile READ lastFile NOTIFY recordingChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY tick)
    Q_PROPERTY(QString error READ error NOTIFY recordingChanged)

public:
    RecordingController(AudioEngine* engine, QList<DeckController*> decks, QObject* parent = nullptr);
    ~RecordingController() override;

    bool recording() const { return m_engine->recorder().isRecording(); }
    double seconds() const { return recording() ? m_engine->recorder().seconds() : 0.0; }
    QString folder() const { return m_folder; }
    void setFolder(const QString& f);
    QString lastFile() const { return m_lastFile; }
    int trackCount() const { return static_cast<int>(m_tracks.size()); }
    QString error() const { return m_error; }

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle() { recording() ? stop() : (void)start(); }

signals:
    void recordingChanged();
    void tick();
    void folderChanged();

private:
    void onPlayed(int deck, const QString& path);
    void writeTracklist();

    struct Entry { double at; QString artist, title, path; };
    AudioEngine* m_engine;
    QList<DeckController*> m_decks;
    QString m_folder, m_lastFile, m_error;
    QList<Entry> m_tracks;
    QTimer m_timer;
};

} // namespace jx
