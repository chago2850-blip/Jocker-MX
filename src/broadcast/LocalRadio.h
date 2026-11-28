// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Encoders.h"
#include "engine/AudioEngine.h"

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>
#include <atomic>
#include <deque>
#include <memory>
#include <thread>
#include <vector>

class QThread;
class QTcpServer;
class QTcpSocket;

namespace jx {

// Radio en la red local: Jocker-MX sirve la mezcla por HTTP sin servidor externo.
//   /             página para el celular (botón de reproducir, qué suena, oyentes)
//   /stream.mp3   MP3 (cualquier navegador o reproductor)
//   /stream.ogg   Ogg/Opus (VLC, Android, navegadores de escritorio)
//   /status.json  título, oyentes y tiempo
// El audio se codifica una sola vez y se reparte a todos; cada oyente nuevo recibe ~2 s para arrancar al instante.
class LocalRadio : public QObject, public AudioTap {
    Q_OBJECT
public:
    struct Config {
        int port = 8000;               // 0 = el que el sistema dé libre
        int mp3Bitrate = 128;
        int opusBitrate = 96;
        QString name = QStringLiteral("Jocker-MX en vivo");
        int maxListeners = 100;
    };

    LocalRadio();
    ~LocalRadio() override;

    bool start(const Config& cfg, double engineRate, QString* error);
    void stop();
    bool running() const { return m_running.load(); }
    int port() const { return m_port.load(); }
    int listeners() const { return m_listeners.load(); }
    int64_t bytesServed() const { return m_bytes.load(); }
    double uptime() const;
    void setTitle(const QString& t);
    QString title() const;

    void push(const float* stereo, int frames) override;   // hilo de audio

    static QStringList lanAddresses();   // IPv4 de la red local, la más probable primero
    static QByteArray pageHtml(const QString& name);

signals:
    void deliver(const QByteArray& mp3, const QByteArray& ogg);   // del hilo codificador al de red
    void listenersChanged();

private:
    class Http;
    void encodeLoop();
    size_t pull(std::vector<float>& out);

    Config m_cfg;
    double m_engineRate = 48000;
    std::unique_ptr<QThread> m_netThread;
    Http* m_http = nullptr;            // vive en m_netThread
    std::thread m_encThread;
    std::atomic<bool> m_running{false}, m_stop{false}, m_accept{false};
    std::atomic<int> m_port{0}, m_listeners{0};
    std::atomic<int64_t> m_bytes{0}, m_startMs{0};

    std::vector<float> m_ring;
    size_t m_capacity = 0;
    std::atomic<size_t> m_head{0}, m_tail{0};

    mutable QMutex m_mutex;
    QString m_title;
};

} // namespace jx
