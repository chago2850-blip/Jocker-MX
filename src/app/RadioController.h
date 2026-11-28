// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "broadcast/LocalRadio.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace jx {

class AudioEngine;
class DeckController;
class Mixer;

// Radio local para la interfaz: encender/apagar, puerto, calidad, dirección para el celular y título automático.
class RadioController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(bool running READ running NOTIFY statusChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY configChanged)
    Q_PROPERTY(int bitrate READ bitrate WRITE setBitrate NOTIFY configChanged)     // MP3 (Opus usa 3/4)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY configChanged)
    Q_PROPERTY(int listeners READ listeners NOTIFY statusChanged)
    Q_PROPERTY(double seconds READ seconds NOTIFY statusChanged)
    Q_PROPERTY(QString url READ url NOTIFY statusChanged)                          // para el QR
    Q_PROPERTY(QStringList urls READ urls NOTIFY statusChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY statusChanged)
    Q_PROPERTY(QString nowPlaying READ nowPlaying NOTIFY statusChanged)
public:
    RadioController(AudioEngine* engine, DeckController* a, DeckController* b, Mixer* mixer, QObject* parent = nullptr);
    ~RadioController() override;

    bool running() const { return m_radio.running(); }
    int port() const { return m_cfg.port; }
    void setPort(int p);
    int bitrate() const { return m_cfg.mp3Bitrate; }
    void setBitrate(int b);
    QString name() const { return m_cfg.name; }
    void setName(const QString& n);
    int listeners() const { return m_radio.listeners(); }
    double seconds() const { return m_radio.uptime(); }
    QString url() const;
    QStringList urls() const;
    QString errorText() const { return m_error; }
    QString nowPlaying() const { return m_title; }

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle() { running() ? stop() : (void)start(); }

    LocalRadio& radio() { return m_radio; }

signals:
    void configChanged();
    void statusChanged();

private:
    void save();
    void tick();
    QString currentTitle() const;

    AudioEngine* m_engine;
    DeckController* m_a;
    DeckController* m_b;
    Mixer* m_mixer;
    LocalRadio m_radio;
    LocalRadio::Config m_cfg;
    QTimer m_timer;
    QString m_error, m_title;
};

} // namespace jx
