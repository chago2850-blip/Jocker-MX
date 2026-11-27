// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Encoders.h"
#include "engine/AudioEngine.h"

#include <QMutex>
#include <QString>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class QTcpSocket;

namespace jx {

// Transmisión en vivo del master a un servidor Icecast 2 o SHOUTcast (v1 / v2 en modo heredado).
// El hilo de audio sólo copia al búfer circular; un hilo propio convierte la frecuencia, codifica,
// envía y se reconecta solo si se cae la conexión.
class Broadcaster : public AudioTap {
public:
    enum Server { Icecast = 0, Shoutcast = 1 };
    enum State { Off = 0, Connecting, Live, Error, Retrying };

    struct Config {
        int server = Icecast;
        QString host = QStringLiteral("localhost");
        int port = 8000;              // puerto del servidor (SHOUTcast: el de los oyentes; se envía al +1)
        QString mount = QStringLiteral("/jocker");
        QString user = QStringLiteral("source");
        QString password;
        int format = StreamEncoder::Opus;
        int bitrate = 128;            // kbps
        QString name = QStringLiteral("Jocker-MX en vivo");
        QString description, genre, url;
        bool isPublic = false;
        int retrySeconds = 5;
    };

    Broadcaster() = default;
    ~Broadcaster() override;

    // Hilo de control
    bool start(const Config& cfg, double engineRate, QString* error);
    void stop();
    bool running() const { return m_running.load(); }
    void setTitle(const QString& title);   // «Artista — Título» de lo que suena

    State state() const { return State(m_state.load()); }
    QString lastError() const;
    QString serverReply() const;
    int64_t bytesSent() const { return m_bytes.load(); }
    double secondsLive() const;            // desde la última conexión correcta
    int reconnects() const { return m_reconnects.load(); }
    int64_t droppedFrames() const { return m_dropped.load(); }
    QString formatName() const { return m_formatName; }
    int effectiveBitrate() const { return m_cfgBitrate; }

    // Hilo de audio
    void push(const float* stereo, int frames) override;

    // Para pruebas y para la interfaz
    static QByteArray icecastRequest(const Config& c, const QString& mime, int sampleRate, bool legacySource);
    static QString describeHttpStatus(int code);

private:
    void loop();
    bool connectAndHandshake(QTcpSocket& s, QString* err);
    void sendMetadata(const QString& title);
    void setError(const QString& e);
    size_t pull(std::vector<float>& out);

    Config m_cfg;
    double m_engineRate = 48000;
    std::unique_ptr<StreamEncoder> m_enc;
    QString m_formatName;
    int m_cfgBitrate = 0;

    std::vector<float> m_ring;
    size_t m_capacity = 0;
    std::atomic<size_t> m_head{0}, m_tail{0};
    std::atomic<bool> m_accept{false};
    std::atomic<int64_t> m_dropped{0};

    std::thread m_thread;
    std::atomic<bool> m_running{false}, m_stop{false};
    std::atomic<int> m_state{Off};
    std::atomic<int64_t> m_bytes{0}, m_liveSinceMs{0};
    std::atomic<int> m_reconnects{0};
    mutable QMutex m_mutex;
    QString m_error, m_reply, m_title, m_sentTitle;
};

} // namespace jx
