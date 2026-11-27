// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "broadcast/Broadcaster.h"

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace jx {

class AudioEngine;
class DeckController;
class Mixer;

// Transmisión en vivo para la interfaz: configuración guardada, estado y título automático.
class BroadcastController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(int server READ server WRITE setServer NOTIFY configChanged)         // 0 Icecast, 1 SHOUTcast
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY configChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY configChanged)
    Q_PROPERTY(QString mount READ mount WRITE setMount NOTIFY configChanged)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY configChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY configChanged)
    Q_PROPERTY(int format READ format WRITE setFormat NOTIFY configChanged)         // 0 Opus, 1 MP3
    Q_PROPERTY(int bitrate READ bitrate WRITE setBitrate NOTIFY configChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY configChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY configChanged)
    Q_PROPERTY(QString genre READ genre WRITE setGenre NOTIFY configChanged)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY configChanged)
    Q_PROPERTY(bool isPublic READ isPublic WRITE setIsPublic NOTIFY configChanged)
    Q_PROPERTY(bool sendTitles READ sendTitles WRITE setSendTitles NOTIFY configChanged)

    Q_PROPERTY(bool onAir READ onAir NOTIFY statusChanged)          // transmisión iniciada (aunque esté reconectando)
    Q_PROPERTY(int state READ state NOTIFY statusChanged)           // Broadcaster::State
    Q_PROPERTY(QString stateText READ stateText NOTIFY statusChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY statusChanged)
    Q_PROPERTY(double seconds READ seconds NOTIFY statusChanged)
    Q_PROPERTY(double kbps READ kbps NOTIFY statusChanged)
    Q_PROPERTY(double megabytes READ megabytes NOTIFY statusChanged)
    Q_PROPERTY(int reconnects READ reconnects NOTIFY statusChanged)
    Q_PROPERTY(QString nowPlaying READ nowPlaying NOTIFY statusChanged)
    Q_PROPERTY(QString listenUrl READ listenUrl NOTIFY configChanged)

public:
    BroadcastController(AudioEngine* engine, DeckController* a, DeckController* b, Mixer* mixer, QObject* parent = nullptr);
    ~BroadcastController() override;

#define JX_PROP(T, n, N) T n() const { return m_cfg.n; } void set##N(const T& v) { if (m_cfg.n == v) return; m_cfg.n = v; save(); }
    JX_PROP(int, server, Server)
    JX_PROP(QString, host, Host)
    JX_PROP(int, port, Port)
    JX_PROP(QString, mount, Mount)
    JX_PROP(QString, user, User)
    JX_PROP(QString, password, Password)
    JX_PROP(int, format, Format)
    JX_PROP(int, bitrate, Bitrate)
    JX_PROP(QString, name, Name)
    JX_PROP(QString, description, Description)
    JX_PROP(QString, genre, Genre)
    JX_PROP(QString, url, Url)
    JX_PROP(bool, isPublic, IsPublic)
#undef JX_PROP
    bool sendTitles() const { return m_sendTitles; }
    void setSendTitles(bool on);

    bool onAir() const { return m_bc.running(); }
    int state() const { return m_bc.state(); }
    QString stateText() const;
    QString errorText() const { return m_startError.isEmpty() ? m_bc.lastError() : m_startError; }
    double seconds() const { return m_bc.secondsLive(); }
    double kbps() const { return m_kbps; }
    double megabytes() const { return m_bc.bytesSent() / 1048576.0; }
    int reconnects() const { return m_bc.reconnects(); }
    QString nowPlaying() const { return m_title; }
    QString listenUrl() const;

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle() { onAir() ? stop() : (void)start(); }

    Broadcaster& broadcaster() { return m_bc; }
    QString currentTitle() const;   // pista que más se oye ahora (para pruebas)

signals:
    void configChanged();
    void statusChanged();

private:
    void save();
    void tick();

    AudioEngine* m_engine;
    DeckController* m_a;
    DeckController* m_b;
    Mixer* m_mixer;
    Broadcaster m_bc;
    Broadcaster::Config m_cfg;
    bool m_sendTitles = true;
    QTimer m_timer;
    QString m_startError, m_title;
    double m_kbps = 0;
    int64_t m_lastBytes = 0;
    qint64 m_lastMs = 0;
};

} // namespace jx
