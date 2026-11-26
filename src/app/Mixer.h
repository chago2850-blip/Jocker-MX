// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "DeckController.h"
#include "engine/AudioEngine.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Sección maestra del mezclador + información del dispositivo. Impulsa el refresco de la UI.
class Mixer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(double crossfader READ crossfader WRITE setCrossfader NOTIFY changed)
    Q_PROPERTY(double masterVolume READ masterVolume WRITE setMasterVolume NOTIFY changed)
    Q_PROPERTY(double cueMix READ cueMix WRITE setCueMix NOTIFY changed)
    Q_PROPERTY(double headphoneVolume READ headphoneVolume WRITE setHeadphoneVolume NOTIFY changed)
    Q_PROPERTY(bool splitCue READ splitCue WRITE setSplitCue NOTIFY changed)
    Q_PROPERTY(double levelL READ levelL NOTIFY levelsChanged)
    Q_PROPERTY(double levelR READ levelR NOTIFY levelsChanged)
    Q_PROPERTY(bool limiting READ limiting NOTIFY levelsChanged)
    Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
    Q_PROPERTY(QString backend READ backend CONSTANT)
    Q_PROPERTY(double sampleRate READ sampleRate CONSTANT)
    Q_PROPERTY(int outputChannels READ outputChannels CONSTANT)
    Q_PROPERTY(double latencyMs READ latencyMs CONSTANT)
    Q_PROPERTY(bool noAudioDevice READ noAudioDevice CONSTANT)
    Q_PROPERTY(bool headphonesAvailable READ headphonesAvailable CONSTANT)

public:
    Mixer(AudioEngine* engine, QList<DeckController*> decks, QObject* parent = nullptr);

    double crossfader() const { return m_engine->crossfader.load(); }
    void setCrossfader(double v) { m_engine->crossfader.store(static_cast<float>(std::clamp(v, -1.0, 1.0))); emit changed(); }
    double masterVolume() const { return m_engine->masterVolume.load(); }
    void setMasterVolume(double v) { m_engine->masterVolume.store(static_cast<float>(std::clamp(v, 0.0, 1.0))); emit changed(); }
    double cueMix() const { return m_engine->cueMix.load(); }
    void setCueMix(double v) { m_engine->cueMix.store(static_cast<float>(std::clamp(v, 0.0, 1.0))); emit changed(); }
    double headphoneVolume() const { return m_engine->headphoneVolume.load(); }
    void setHeadphoneVolume(double v) { m_engine->headphoneVolume.store(static_cast<float>(std::clamp(v, 0.0, 1.0))); emit changed(); }
    bool splitCue() const { return m_engine->splitCue.load(); }
    void setSplitCue(bool b) { m_engine->splitCue.store(b); emit changed(); }

    double levelL() const { return m_l; }
    double levelR() const { return m_r; }
    bool limiting() const { return m_limiting; }
    QString deviceName() const { return m_engine->deviceName(); }
    QString backend() const { return m_engine->backendName(); }
    double sampleRate() const { return m_engine->sampleRate(); }
    int outputChannels() const { return m_engine->outputChannels(); }
    double latencyMs() const { return 1000.0 * m_engine->blockSize() / m_engine->sampleRate(); }
    bool noAudioDevice() const { return m_engine->isNullDevice(); }
    bool headphonesAvailable() const { return m_engine->outputChannels() >= 4; }

signals:
    void changed();
    void levelsChanged();

private:
    void tick();
    AudioEngine* m_engine;
    QList<DeckController*> m_decks;
    QTimer m_timer;
    double m_l = 0, m_r = 0;
    bool m_limiting = false;
};

} // namespace jx
