// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "LocalRadio.h"

#include "miniaudio.h"

#include <QDateTime>
#include <QFile>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <algorithm>
#include <chrono>

namespace jx {

namespace {
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }
constexpr qint64 kBurstBytesMp3 = 40 * 1024;   // ≈ 2,5 s a 128 kbps
constexpr qint64 kMaxBacklog = 1024 * 1024;    // oyente que no recibe: se le desconecta
}

// ================================================================== servidor HTTP (hilo de red)
class LocalRadio::Http : public QObject {
    Q_OBJECT
public:
    explicit Http(LocalRadio* owner) : m_owner(owner) {}

    struct Client { QTcpSocket* s = nullptr; QByteArray req; int kind = 0; };   // kind: 0 pidiendo, 1 mp3, 2 ogg

public slots:
    void listen(int port, int maxListeners, QString name, bool* ok, int* actualPort, QString* err) {
        m_max = maxListeners;
        m_name = name;
        m_server = new QTcpServer(this);
        if (!m_server->listen(QHostAddress::Any, quint16(port))) {
            *ok = false;
            *err = QStringLiteral("No se pudo abrir el puerto %1 (%2)").arg(port).arg(m_server->errorString());
            return;
        }
        *ok = true;
        *actualPort = m_server->serverPort();
        connect(m_server, &QTcpServer::newConnection, this, &Http::accept);
    }

    void shutdown() {
        for (Client& c : m_clients) { c.s->abort(); c.s->deleteLater(); }
        m_clients.clear();
        if (m_server) m_server->close();
        m_owner->m_listeners.store(0);
    }

    void setOggHeader(const QByteArray& h) { m_oggHeader = h; }

    void onDeliver(const QByteArray& mp3, const QByteArray& ogg) {
        push(m_mp3Recent, m_mp3Bytes, mp3, kBurstBytesMp3);
        push(m_oggRecent, m_oggBytes, ogg, kBurstBytesMp3 * 3 / 4);
        for (int i = int(m_clients.size()) - 1; i >= 0; --i) {
            Client& c = m_clients[i];
            const QByteArray& d = c.kind == 1 ? mp3 : c.kind == 2 ? ogg : QByteArray();
            if (d.isEmpty()) continue;
            if (c.s->bytesToWrite() > kMaxBacklog) { drop(i); continue; }   // conexión lenta o colgada
            c.s->write(d);
            m_owner->m_bytes.fetch_add(d.size());
        }
    }

private:
    static void push(std::deque<QByteArray>& q, qint64& total, const QByteArray& d, qint64 limit) {
        if (d.isEmpty()) return;
        q.push_back(d);
        total += d.size();
        while (total > limit && q.size() > 1) { total -= q.front().size(); q.pop_front(); }
    }

    void accept() {
        while (QTcpSocket* s = m_server->nextPendingConnection()) {
            m_clients.push_back({s, {}, 0});
            connect(s, &QTcpSocket::readyRead, this, [this, s]() { onRead(s); });
            connect(s, &QTcpSocket::disconnected, this, [this, s]() {
                for (int i = 0; i < m_clients.size(); ++i) if (m_clients[i].s == s) { drop(i); break; }
            });
        }
    }

    void drop(int i) {
        Client c = m_clients.takeAt(i);
        c.s->disconnect(this);
        c.s->abort();
        c.s->deleteLater();
        updateCount();
    }

    void updateCount() {
        int n = 0;
        for (const Client& c : m_clients) if (c.kind != 0) ++n;
        if (m_owner->m_listeners.exchange(n) != n) emit m_owner->listenersChanged();
    }

    void onRead(QTcpSocket* s) {
        int i = 0;
        for (; i < m_clients.size(); ++i) if (m_clients[i].s == s) break;
        if (i == m_clients.size()) return;
        Client& c = m_clients[i];
        if (c.kind != 0) { s->readAll(); return; }   // los oyentes no mandan nada más
        c.req += s->readAll();
        if (c.req.size() > 16384) { drop(i); return; }
        if (!c.req.contains("\r\n\r\n") && !c.req.contains("\n\n")) return;
        const QList<QByteArray> first = c.req.left(c.req.indexOf('\n')).trimmed().split(' ');
        const QByteArray method = first.value(0), target = first.value(1);
        const QByteArray path = target.split('?').first();
        const bool head = method == "HEAD";
        auto reply = [&](const QByteArray& status, const QByteArray& type, const QByteArray& body) {
            QByteArray r = "HTTP/1.0 " + status + "\r\nContent-Type: " + type + "\r\nContent-Length: " + QByteArray::number(body.size()) +
                           "\r\nCache-Control: no-cache\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
            if (!head) r += body;
            s->write(r);
            s->disconnectFromHost();
        };
        if (method != "GET" && !head) { reply("405 Method Not Allowed", "text/plain", "Sólo GET"); return; }
        if (path == "/stream.mp3" || path == "/stream.ogg" || path == "/stream") {
            const bool ogg = path == "/stream.ogg";
            int listeners = 0;
            for (const Client& x : m_clients) if (x.kind != 0) ++listeners;
            if (listeners >= m_max) { reply("503 Service Unavailable", "text/plain; charset=utf-8", "La radio está llena"); return; }
            QByteArray h = "HTTP/1.0 200 OK\r\nContent-Type: " + QByteArray(ogg ? "audio/ogg" : "audio/mpeg") +
                           "\r\nCache-Control: no-cache, no-store\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n"
                           "icy-name: " + m_name.toUtf8().replace('\r', ' ').replace('\n', ' ') + "\r\n\r\n";
            s->write(h);
            if (head) { s->disconnectFromHost(); return; }
            if (ogg) s->write(m_oggHeader);
            for (const QByteArray& b : (ogg ? m_oggRecent : m_mp3Recent)) s->write(b);   // arranque inmediato
            c.kind = ogg ? 2 : 1;
            c.req.clear();
            s->setSocketOption(QAbstractSocket::LowDelayOption, 1);
            updateCount();
            return;
        }
        if (path == "/" || path == "/index.html") { reply("200 OK", "text/html; charset=utf-8", pageHtml(m_name)); return; }
        if (path == "/status.json") {
            QJsonObject o{{"name", m_name}, {"title", m_owner->title()}, {"listeners", m_owner->listeners()},
                          {"uptime", int(m_owner->uptime())}};
            reply("200 OK", "application/json; charset=utf-8", QJsonDocument(o).toJson(QJsonDocument::Compact));
            return;
        }
        if (path == "/logo.png") {
            QFile f(QStringLiteral(":/JockerMX/resources/icon-256.png"));
            if (f.open(QIODevice::ReadOnly)) { reply("200 OK", "image/png", f.readAll()); return; }
        }
        reply("404 Not Found", "text/plain; charset=utf-8", "No encontrado");
    }

    LocalRadio* m_owner;
    QTcpServer* m_server = nullptr;
    QList<Client> m_clients;
    std::deque<QByteArray> m_mp3Recent, m_oggRecent;
    qint64 m_mp3Bytes = 0, m_oggBytes = 0;
    QByteArray m_oggHeader;
    int m_max = 100;
    QString m_name;
};

// ================================================================== LocalRadio
LocalRadio::LocalRadio() = default;
LocalRadio::~LocalRadio() { stop(); }

double LocalRadio::uptime() const {
    const qint64 s = m_startMs.load();
    return m_running.load() && s > 0 ? (nowMs() - s) / 1000.0 : 0.0;
}

void LocalRadio::setTitle(const QString& t) { QMutexLocker l(&m_mutex); m_title = t; }
QString LocalRadio::title() const { QMutexLocker l(&m_mutex); return m_title; }

bool LocalRadio::start(const Config& cfg, double engineRate, QString* error) {
    stop();
    m_cfg = cfg;
    m_engineRate = engineRate;
    m_netThread = std::make_unique<QThread>();
    m_netThread->setObjectName(QStringLiteral("jx-radio-red"));
    m_http = new Http(this);
    m_http->moveToThread(m_netThread.get());
    connect(m_netThread.get(), &QThread::finished, m_http, &QObject::deleteLater);
    connect(this, &LocalRadio::deliver, m_http, &Http::onDeliver, Qt::QueuedConnection);
    m_netThread->start();
    bool ok = false;
    int port = 0;
    QString err;
    QMetaObject::invokeMethod(m_http, [&]() { m_http->listen(cfg.port, cfg.maxListeners, cfg.name, &ok, &port, &err); },
                              Qt::BlockingQueuedConnection);
    if (!ok) {
        m_netThread->quit();
        m_netThread->wait();
        m_netThread.reset();
        m_http = nullptr;
        if (error) *error = err;
        return false;
    }
    m_port.store(port);
    m_capacity = size_t(engineRate * 10);
    m_ring.assign(m_capacity * 2, 0.f);
    m_head.store(0);
    m_tail.store(0);
    m_bytes.store(0);
    m_listeners.store(0);
    m_stop.store(false);
    m_running.store(true);
    m_startMs.store(nowMs());
    m_accept.store(true, std::memory_order_release);
    m_encThread = std::thread([this]() { encodeLoop(); });
    return true;
}

void LocalRadio::stop() {
    m_accept.store(false, std::memory_order_release);
    m_stop.store(true);
    if (m_encThread.joinable()) m_encThread.join();
    if (m_netThread) {
        if (m_http) QMetaObject::invokeMethod(m_http, [this]() { m_http->shutdown(); }, Qt::BlockingQueuedConnection);
        m_netThread->quit();
        m_netThread->wait();
        m_netThread.reset();
        m_http = nullptr;
    }
    const bool was = m_running.exchange(false);
    m_port.store(0);
    if (m_listeners.exchange(0) != 0 || was) emit listenersChanged();
}

void LocalRadio::push(const float* stereo, int frames) {
    if (!m_accept.load(std::memory_order_acquire) || m_capacity == 0) return;
    const size_t head = m_head.load(std::memory_order_relaxed);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    const size_t n = std::min<size_t>(size_t(frames), m_capacity - (head - tail));
    for (size_t i = 0; i < n; ++i) {
        const size_t idx = ((head + i) % m_capacity) * 2;
        m_ring[idx] = stereo[2 * i];
        m_ring[idx + 1] = stereo[2 * i + 1];
    }
    m_head.store(head + n, std::memory_order_release);
}

size_t LocalRadio::pull(std::vector<float>& out) {
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

void LocalRadio::encodeLoop() {
    constexpr int kRate = 48000;
    Mp3StreamEncoder mp3(m_cfg.mp3Bitrate, kRate);
    OpusStreamEncoder opus(m_cfg.opusBitrate);
    mp3.start({});
    const QByteArray header = opus.start(m_cfg.name);
    QMetaObject::invokeMethod(m_http, [h = m_http, header]() { h->setOggHeader(header); }, Qt::QueuedConnection);

    const bool resample = std::abs(m_engineRate - kRate) > 0.5;
    ma_resampler rs;
    if (resample) {
        ma_resampler_config rc = ma_resampler_config_init(ma_format_f32, 2, ma_uint32(m_engineRate), kRate, ma_resample_algorithm_linear);
        rc.linear.lpfOrder = 8;
        ma_resampler_init(&rc, nullptr, &rs);
    }
    std::vector<float> in, out;
    while (!m_stop.load()) {
        const size_t n = pull(in);
        if (n == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(15)); continue; }
        const float* src = in.data();
        ma_uint64 frames = n;
        if (resample) {
            ma_uint64 cap = ma_uint64(double(n) * kRate / m_engineRate) + 64;
            out.resize(size_t(cap) * 2);
            ma_uint64 inF = n, outF = cap;
            ma_resampler_process_pcm_frames(&rs, in.data(), &inF, out.data(), &outF);
            src = out.data();
            frames = outF;
        }
        const QByteArray a = mp3.encode(src, int(frames));
        const QByteArray b = opus.encode(src, int(frames));
        if (!a.isEmpty() || !b.isEmpty()) emit deliver(a, b);
    }
    if (resample) ma_resampler_uninit(&rs, nullptr);
}

QStringList LocalRadio::lanAddresses() {
    QStringList priv, other;
    for (const QNetworkInterface& ifc : QNetworkInterface::allInterfaces()) {
        const auto f = ifc.flags();
        if (!(f & QNetworkInterface::IsUp) || !(f & QNetworkInterface::IsRunning) || (f & QNetworkInterface::IsLoopBack)) continue;
        for (const QNetworkAddressEntry& e : ifc.addressEntries()) {
            const QHostAddress a = e.ip();
            if (a.protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString s = a.toString();
            const bool lan = s.startsWith("192.168.") || s.startsWith("10.") ||
                             (s.startsWith("172.") && s.section('.', 1, 1).toInt() >= 16 && s.section('.', 1, 1).toInt() <= 31);
            (lan ? priv : other) << s;
        }
    }
    // Wi-Fi doméstica primero (192.168.x), luego otras privadas
    std::stable_sort(priv.begin(), priv.end(), [](const QString& a, const QString& b) {
        return a.startsWith("192.168.") && !b.startsWith("192.168.");
    });
    return priv + other;
}

QByteArray LocalRadio::pageHtml(const QString& name) {
    QString n = name.toHtmlEscaped();
    QString html = QStringLiteral(R"HTML(<!DOCTYPE html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%1</title>
<style>
  :root { color-scheme: dark; }
  body { margin:0; min-height:100vh; display:flex; flex-direction:column; align-items:center; justify-content:center;
         background: radial-gradient(circle at 50% 30%, #2a1648, #0d0818 70%); color:#f2f0ff;
         font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; text-align:center; }
  img { width:96px; height:96px; border-radius:22px; }
  h1 { font-size:1.6rem; margin:.6rem 1rem .2rem; color:#f5b942; letter-spacing:.04em; }
  .now { min-height:1.4em; margin:.2rem 1rem 1.4rem; color:#c9c7e8; font-size:1.05rem; }
  button { width:150px; height:150px; border-radius:50%; border:3px solid #ff2d8c; background:#1c1230; color:#fff;
           font-size:3.2rem; box-shadow:0 0 40px rgba(255,45,140,.35); cursor:pointer; }
  button.on { border-color:#55e08a; box-shadow:0 0 50px rgba(85,224,138,.45); }
  .info { margin-top:1.4rem; color:#8c88b0; font-size:.9rem; }
  .live { display:inline-block; padding:.2rem .7rem; border-radius:1rem; background:#ff4d5e22; border:1px solid #ff4d5e;
          color:#ff4d5e; font-weight:700; font-size:.8rem; margin-bottom:.6rem; }
  a { color:#3fe0ff; }
</style></head><body>
<img src="/logo.png" alt="">
<h1>%1</h1>
<div class="live">● EN VIVO</div>
<div class="now" id="now">&nbsp;</div>
<button id="b" aria-label="Escuchar">▶</button>
<div class="info"><span id="l">—</span> escuchando · <a href="/stream.mp3">MP3</a> · <a href="/stream.ogg">Opus</a></div>
<div class="info">Hecho con Jocker-MX · software libre</div>
<script>
const a = new Audio(); const b = document.getElementById('b');
b.onclick = () => {
  if (a.paused) { a.src = '/stream.mp3?t=' + Date.now(); a.play(); b.textContent = '❚❚'; b.classList.add('on'); }
  else { a.pause(); a.removeAttribute('src'); a.load(); b.textContent = '▶'; b.classList.remove('on'); }
};
async function st() {
  try { const r = await fetch('/status.json', {cache:'no-store'}); const s = await r.json();
        document.getElementById('now').textContent = s.title ? '♪ ' + s.title : '';
        document.getElementById('l').textContent = s.listeners; } catch (e) {}
}
st(); setInterval(st, 5000);
</script></body></html>)HTML").arg(n);
    return html.toUtf8();
}

} // namespace jx

#include "LocalRadio.moc"
