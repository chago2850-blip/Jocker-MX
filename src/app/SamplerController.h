// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "engine/AudioEngine.h"

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace jx {

class SamplerController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QStringList names READ names NOTIFY slotsChanged)
    Q_PROPERTY(QVariantList modes READ modes NOTIFY slotsChanged)
    Q_PROPERTY(QVariantList custom READ custom NOTIFY slotsChanged)
    Q_PROPERTY(QVariantList playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(QVariantList progress READ progress NOTIFY playingChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY settingsChanged)
    Q_PROPERTY(bool pfl READ pfl WRITE setPfl NOTIFY settingsChanged)
    Q_PROPERTY(QString error READ error NOTIFY slotsChanged)

public:
    explicit SamplerController(AudioEngine* engine, QObject* parent = nullptr);

    QStringList names() const;
    QVariantList modes() const;
    QVariantList custom() const;
    QVariantList playing() const { return m_playing; }
    QVariantList progress() const { return m_progress; }
    double volume() const { return m_s.volume.load(); }
    void setVolume(double v);
    bool pfl() const { return m_s.pfl.load(); }
    void setPfl(bool b);
    QString error() const { return m_error; }

    Q_INVOKABLE void press(int slot);
    Q_INVOKABLE void release(int slot);
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void setMode(int slot, int mode);
    Q_INVOKABLE void cycleMode(int slot) { setMode(slot, (m_s.mode[slot].load() + 1) % 3); }
    Q_INVOKABLE bool loadFile(int slot, const QString& pathOrUrl);
    Q_INVOKABLE void resetSlot(int slot);   // vuelve al sonido incluido

signals:
    void slotsChanged();
    void playingChanged();
    void settingsChanged();

private:
    void install(int slot, std::shared_ptr<Sample> s);
    void tick();

    AudioEngine* m_engine;
    Sampler& m_s;
    std::shared_ptr<Sample> m_samples[Sampler::kSlots];
    QString m_paths[Sampler::kSlots];
    QVariantList m_playing, m_progress;
    QString m_error;
    QTimer m_timer;
};

} // namespace jx
