// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Broadcaster.h"

#include "miniaudio.h"

#include <QDateTime>
#include <QTcpSocket>
#include <QThread>
#include <QUrl>
#include <algorithm>
#include <chrono>

namespace jx {

namespace {
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

QByteArray basicAuth(const QString& user, const QString& pass) {
    return "Basic " + (user + QLatin1Char(':') + pass).toUtf8().toBase64();
}

QByteArray headerLine(const char* name, const QString& value) {
    QString v = value;
    v.remove(QLatin1Char('\r')).remove(QLatin1Char('\n'));   // nada de cabeceras inyectadas
    return QByteArray(name) + ": " + v.toUtf8() + "\r\n";
}

QString cleanMount(QString m) {
    m = m.trimmed();
    if (m.isEmpty()) m = QStringLiteral("/jocker");
    if (!m.startsWith(QLatin1Char('/'))) m.prepend(QLatin1Char('/'));
    return m;
}

// Lee la respuesta del servidor hasta una línea en blanco (o hasta que deje de llegar).
QByteArray readReply(QTcpSocket& s, int timeoutMs) {
    QByteArray r;
    const qint64 until = nowMs() + timeoutMs;
    while (nowMs() < until) {
        if (s.bytesAvailable() == 0 && !s.waitForReadyRead(int(std::max<qint64>(1, until - nowMs())))) break;
        r += s.readAll();
        if (r.contains("\r\n\r\n") || r.contains("\n\n")) break;
        if (r.startsWith("OK2") && r.contains('\n')) {   // SHOUTcast responde con una línea y luego cabeceras
            s.waitForReadyRead(150);
            r += s.readAll();
            break;
        }
        if (r.size() > 8192) break;
    }
    return r;
}
} // namespace

Broadcaster::~Broadcaster() { stop(); }

QString Broadcaster::describeHttpStatus(int code) {
    switch (code) {
    case 200: return QStringLiteral("Conectado");
    case 400: return QStringLiteral("El servidor no entendió la conexión (¿es Icecast 2?)");
    case 401: return QStringLiteral("Usuario o contraseña incorrectos");
    case 403: return QStringLiteral("El punto de montaje está en uso o no está permitido");
    case 404: return QStringLiteral("El punto de montaje no existe");
    case 405: return QStringLiteral("El servidor no acepta este método de envío");
    case 500: return QStringLiteral("Error interno del servidor");
    default: return QStringLiteral("El servidor respondió %1").arg(code);
    }
}

QByteArray Broadcaster::icecastRequest(const Config& c, const QString& mime, int sampleRate, bool legacy) {
    const QByteArray mount = QUrl::toPercentEncoding(cleanMount(c.mount), "/");
    QByteArray r = legacy ? ("SOURCE " + mount + " ICE/1.0\r\n") : ("PUT " + mount + " HTTP/1.1\r\n");
    r += "Host: " + c.host.toUtf8() + ":" + QByteArray::number(c.port) + "\r\n";
    r += "Authorization: " + basicAuth(c.user.isEmpty() ? QStringLiteral("source") : c.user, c.password) + "\r\n";
    r += "User-Agent: Jocker-MX/" JOCKER_BROADCAST_VERSION "\r\n";
    r += headerLine("Content-Type", mime);
    r += headerLine("Ice-Public", c.isPublic ? QStringLiteral("1") : QStringLiteral("0"));
    r += headerLine("Ice-Name", c.name);
    if (!c.description.isEmpty()) r += headerLine("Ice-Description", c.description);
    if (!c.genre.isEmpty()) r += headerLine("Ice-Genre", c.genre);
    if (!c.url.isEmpty()) r += headerLine("Ice-Url", c.url);
    r += headerLine("Ice-Audio-Info", QStringLiteral("ice-bitrate=%1;ice-channels=2;ice-samplerate=%2").arg(c.bitrate).arg(sampleRate));
    r += "\r\n";
    return r;
}

bool Broadcaster::start(const Config& cfg, double engineRate, QString* error) {
    stop();
    m_cfg = cfg;
    m_engineRate = engineRate;
    if (cfg.host.trimmed().isEmpty()) { if (error) *error = QStringLiteral("Falta el servidor"); return false; }
    if (cfg.server == Shoutcast && cfg.format == StreamEncoder::Opus) {
        if (error) *error = QStringLiteral("SHOUTcast sólo acepta MP3: elige el formato MP3");
        return false;
    }
    QString err;
    m_enc = StreamEncoder::create(StreamEncoder::Format(cfg.format), cfg.bitrate, engineRate, &err);
    if (!m_enc) { if (error) *error = err; return false; }
    m_formatName = m_enc->name();
    m_cfgBitrate = cfg.bitrate;
    m_capacity = size_t(engineRate * 10);   // 10 s de margen
    m_ring.assign(m_capacity * 2, 0.f);
    m_head.store(0);
    m_tail.store(0);
    m_dropped.store(0);
    m_bytes.store(0);
    m_reconnects.store(0);
    {
        QMutexLocker l(&m_mutex);
        m_error.clear();
        m_reply.clear();
        m_sentTitle.clear();
    }
    m_stop.store(false);
    m_running.store(true);
    m_state.store(Connecting);
    m_accept.store(true, std::memory_order_release);
    m_thread = std::thread([this]() { loop(); });
    return true;
}

void Broadcaster::stop() {
    m_accept.store(false, std::memory_order_release);
    m_stop.store(true);
    if (m_thread.joinable()) m_thread.join();
    m_running.store(false);
    m_state.store(Off);
}

void Broadcaster::setTitle(const QString& t) {
    QMutexLocker l(&m_mutex);
    m_title = t;
}

QString Broadcaster::lastError() const { QMutexLocker l(&m_mutex); return m_error; }
QString Broadcaster::serverReply() const { QMutexLocker l(&m_mutex); return m_reply; }

double Broadcaster::secondsLive() const {
    const qint64 s = m_liveSinceMs.load();
    return state() == Live && s > 0 ? (nowMs() - s) / 1000.0 : 0.0;
}

void Broadcaster::setError(const QString& e) {
    QMutexLocker l(&m_mutex);
    m_error = e;
}

// ------------------------------------------------------------------ hilo de audio
void Broadcaster::push(const float* stereo, int frames) {
    if (!m_accept.load(std::memory_order_acquire) || m_capacity == 0) return;
    const size_t head = m_head.load(std::memory_order_relaxed);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    const size_t free = m_capacity - (head - tail);
    const size_t n = std::min<size_t>(size_t(frames), free);
    for (size_t i = 0; i < n; ++i) {
        const size_t idx = ((head + i) % m_capacity) * 2;
        m_ring[idx] = stereo[2 * i];
        m_ring[idx + 1] = stereo[2 * i + 1];
    }
    if (n < size_t(frames)) m_dropped.fetch_add(int64_t(size_t(frames) - n), std::memory_order_relaxed);
    m_head.store(head + n, std::memory_order_release);
}

size_t Broadcaster::pull(std::vector<float>& out) {
    const size_t head = m_head.load(std::memory_order_acquire);
    const size_t tail = m_tail.load(std::memory_order_relaxed);
    const size_t n = head - tail;
    out.resize(n * 2);
    for (size_t i = 0; i < n; ++i) {
        const size_t idx = ((tail + i) % m_capacity) * 2;
        out[2 * i] = m_ring[idx];
        out[2 * i + 1] = m_ring[idx + 1];
    }
    m_tail.store(tail + n, std::memory_order_release);
    return n;
}

// ------------------------------------------------------------------ red
bool Broadcaster::connectAndHandshake(QTcpSocket& s, QString* err) {
    const bool shout = m_cfg.server == Shoutcast;
    const int port = shout ? m_cfg.port + 1 : m_cfg.port;   // SHOUTcast recibe la fuente en puerto+1
    for (int attempt = 0; attempt < 2; ++attempt) {
        const bool legacy = attempt == 1;
        s.abort();
        s.connectToHost(m_cfg.host.trimmed(), quint16(port));
        if (!s.waitForConnected(6000)) {
            *err = QStringLiteral("No se pudo conectar a %1:%2 (%3)").arg(m_cfg.host).arg(port).arg(s.errorString());
            return false;
        }
        s.setSocketOption(QAbstractSocket::LowDelayOption, 1);
        if (shout) {
            QString pass = m_cfg.password;
            const QString sid = cleanMount(m_cfg.mount).mid(1);
            bool isNum = false;
            sid.toInt(&isNum);
            if (isNum) pass += QStringLiteral(":#") + sid;   // SHOUTcast 2: contraseña:#id_de_stream
            s.write(pass.toUtf8() + "\r\n");
            s.waitForBytesWritten(3000);
            const QByteArray r = readReply(s, 6000);
            { QMutexLocker l(&m_mutex); m_reply = QString::fromUtf8(r.left(200)).trimmed(); }
            if (!r.startsWith("OK2")) {
                *err = r.toLower().contains("invalid") ? QStringLiteral("Contraseña incorrecta")
                     : r.isEmpty() ? QStringLiteral("El servidor no respondió") : QStringLiteral("Respuesta inesperada del servidor");
                return false;
            }
            QByteArray h;
            h += headerLine("icy-name", m_cfg.name);
            h += headerLine("icy-genre", m_cfg.genre);
            h += headerLine("icy-url", m_cfg.url);
            h += headerLine("icy-pub", m_cfg.isPublic ? QStringLiteral("1") : QStringLiteral("0"));
            h += headerLine("icy-br", QString::number(m_cfg.bitrate));
            h += headerLine("content-type", m_enc->mimeType());
            h += "\r\n";
            s.write(h);
            s.waitForBytesWritten(3000);
            return true;
        }
        s.write(icecastRequest(m_cfg, m_enc->mimeType(), m_enc->inputRate(), legacy));
        s.waitForBytesWritten(3000);
        const QByteArray r = readReply(s, 6000);
        const QByteArray first = r.left(r.indexOf('\n')).trimmed();
        { QMutexLocker l(&m_mutex); m_reply = QString::fromUtf8(first); }
        const QList<QByteArray> parts = first.split(' ');
        const int code = parts.size() >= 2 ? parts[1].toInt() : 0;
        if (code == 200 || code == 100) return true;
        if (!legacy && (code == 405 || code == 400 || code == 501)) continue;   // Icecast antiguo: probar SOURCE
        *err = code ? describeHttpStatus(code) : (r.isEmpty() ? QStringLiteral("El servidor no respondió")
                                                              : QStringLiteral("Respuesta inesperada: %1").arg(QString::fromUtf8(first)));
        return false;
    }
    return false;
}

void Broadcaster::sendMetadata(const QString& title) {
    if (m_cfg.format != StreamEncoder::Mp3) return;   // en Ogg/Opus el título viaja en el propio flujo
    QTcpSocket s;
    s.connectToHost(m_cfg.host.trimmed(), quint16(m_cfg.port));
    if (!s.waitForConnected(3000)) return;
    const QByteArray song = QUrl::toPercentEncoding(title);
    QByteArray req;
    if (m_cfg.server == Shoutcast) {
        req = "GET /admin.cgi?pass=" + QUrl::toPercentEncoding(m_cfg.password) + "&mode=updinfo&song=" + song +
              " HTTP/1.0\r\nUser-Agent: Mozilla/5.0 (Jocker-MX)\r\n\r\n";
    } else {
        req = "GET /admin/metadata?mount=" + QUrl::toPercentEncoding(cleanMount(m_cfg.mount)) + "&mode=updinfo&song=" + song +
              " HTTP/1.0\r\nAuthorization: " + basicAuth(m_cfg.user.isEmpty() ? QStringLiteral("source") : m_cfg.user, m_cfg.password) +
              "\r\nUser-Agent: Jocker-MX/" JOCKER_BROADCAST_VERSION "\r\n\r\n";
    }
    s.write(req);
    s.waitForBytesWritten(2000);
    s.waitForReadyRead(2000);
    s.disconnectFromHost();
}

void Broadcaster::loop() {
    const int outRate = m_enc->inputRate();
    const bool resample = std::abs(m_engineRate - outRate) > 0.5;
    ma_resampler rs;
    if (resample) {
        ma_resampler_config rc = ma_resampler_config_init(ma_format_f32, 2, ma_uint32(m_engineRate), ma_uint32(outRate),
                                                          ma_resample_algorithm_linear);
        rc.linear.lpfOrder = 8;
        ma_resampler_init(&rc, nullptr, &rs);
    }
    std::vector<float> in, out;
    auto sleepMs = [this](int ms) {
        for (int t = 0; t < ms && !m_stop.load(); t += 20) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    };

    bool first = true;
    while (!m_stop.load()) {
        m_state.store(first ? Connecting : Retrying);
        QTcpSocket s;
        QString err;
        if (!connectAndHandshake(s, &err)) {
            setError(err);
            m_state.store(Error);
            pull(in);                        // descartar lo acumulado: al volver se transmite en directo
            sleepMs(std::max(1, m_cfg.retrySeconds) * 1000);
            pull(in);
            if (!m_stop.load()) m_reconnects.fetch_add(1);
            first = false;
            continue;
        }
        first = false;
        setError(QString());
        pull(in);                            // empezar con audio actual
        QString title;
        { QMutexLocker l(&m_mutex); title = m_title; m_sentTitle = title; }
        QByteArray hdr = m_enc->start(title);
        if (!hdr.isEmpty()) { s.write(hdr); s.waitForBytesWritten(3000); m_bytes.fetch_add(hdr.size()); }
        m_liveSinceMs.store(nowMs());
        m_state.store(Live);
        if (!title.isEmpty()) sendMetadata(title);

        bool lost = false;
        while (!m_stop.load()) {
            const size_t n = pull(in);
            if (n == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                if (s.state() != QAbstractSocket::ConnectedState) { lost = true; break; }
                continue;
            }
            const float* src = in.data();
            ma_uint64 frames = n;
            if (resample) {
                ma_uint64 cap = ma_uint64(double(n) * outRate / m_engineRate) + 64;
                out.resize(size_t(cap) * 2);
                ma_uint64 inF = n, outF = cap;
                ma_resampler_process_pcm_frames(&rs, in.data(), &inF, out.data(), &outF);
                src = out.data();
                frames = outF;
            }
            const QByteArray bytes = m_enc->encode(src, int(frames));
            if (!bytes.isEmpty()) {
                s.write(bytes);
                const qint64 until = nowMs() + 8000;
                while (s.bytesToWrite() > 0 && nowMs() < until && s.state() == QAbstractSocket::ConnectedState)
                    s.waitForBytesWritten(200);
                if (s.bytesToWrite() > 0 || s.state() != QAbstractSocket::ConnectedState) { lost = true; break; }
                m_bytes.fetch_add(bytes.size());
            }
            if (s.bytesAvailable()) s.readAll();   // ignorar lo que mande el servidor
            QString t;
            { QMutexLocker l(&m_mutex); if (m_title != m_sentTitle) { t = m_title; m_sentTitle = m_title; } }
            if (!t.isEmpty()) sendMetadata(t);
        }
        if (m_stop.load() && !lost) {
            const QByteArray tail = m_enc->finish();
            if (!tail.isEmpty()) { s.write(tail); s.waitForBytesWritten(1000); }
            s.disconnectFromHost();
            if (s.state() != QAbstractSocket::UnconnectedState) s.waitForDisconnected(1000);
        }
        if (lost && !m_stop.load()) {
            setError(QStringLiteral("Se perdió la conexión con el servidor; reconectando…"));
            m_state.store(Error);
            m_reconnects.fetch_add(1);
            sleepMs(1000);
        }
    }
    if (resample) ma_resampler_uninit(&rs, nullptr);
}

} // namespace jx
