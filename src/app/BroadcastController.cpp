// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "BroadcastController.h"

#include "DeckController.h"
#include "Mixer.h"
#include "engine/AudioEngine.h"

#include <QDateTime>
#include <QSettings>
#include <algorithm>

namespace jx {

BroadcastController::BroadcastController(AudioEngine* engine, DeckController* a, DeckController* b, Mixer* mixer, QObject* parent)
    : QObject(parent), m_engine(engine), m_a(a), m_b(b), m_mixer(mixer) {
    QSettings s;
    s.beginGroup("broadcast");
    m_cfg.server = s.value("server", m_cfg.server).toInt();
    m_cfg.host = s.value("host", m_cfg.host).toString();
    m_cfg.port = s.value("port", m_cfg.port).toInt();
    m_cfg.mount = s.value("mount", m_cfg.mount).toString();
    m_cfg.user = s.value("user", m_cfg.user).toString();
    m_cfg.password = s.value("password").toString();
    m_cfg.format = s.value("format", m_cfg.format).toInt();
    m_cfg.bitrate = s.value("bitrate", m_cfg.bitrate).toInt();
    m_cfg.name = s.value("name", m_cfg.name).toString();
    m_cfg.description = s.value("description").toString();
    m_cfg.genre = s.value("genre").toString();
    m_cfg.url = s.value("url").toString();
    m_cfg.isPublic = s.value("public", false).toBool();
    m_sendTitles = s.value("sendTitles", true).toBool();
    s.endGroup();
    m_timer.setInterval(500);
    connect(&m_timer, &QTimer::timeout, this, &BroadcastController::tick);
}

BroadcastController::~BroadcastController() {
    if (m_engine) m_engine->setTap(0, nullptr);
    m_bc.stop();
}

void BroadcastController::save() {
    QSettings s;
    s.beginGroup("broadcast");
    s.setValue("server", m_cfg.server);
    s.setValue("host", m_cfg.host);
    s.setValue("port", m_cfg.port);
    s.setValue("mount", m_cfg.mount);
    s.setValue("user", m_cfg.user);
    s.setValue("password", m_cfg.password);
    s.setValue("format", m_cfg.format);
    s.setValue("bitrate", m_cfg.bitrate);
    s.setValue("name", m_cfg.name);
    s.setValue("description", m_cfg.description);
    s.setValue("genre", m_cfg.genre);
    s.setValue("url", m_cfg.url);
    s.setValue("public", m_cfg.isPublic);
    s.setValue("sendTitles", m_sendTitles);
    s.endGroup();
    emit configChanged();
}

void BroadcastController::setSendTitles(bool on) {
    if (on == m_sendTitles) return;
    m_sendTitles = on;
    save();
}

QString BroadcastController::listenUrl() const {
    if (m_cfg.server == Broadcaster::Shoutcast) return QStringLiteral("http://%1:%2/").arg(m_cfg.host).arg(m_cfg.port);
    QString m = m_cfg.mount.trimmed();
    if (!m.startsWith(QLatin1Char('/'))) m.prepend(QLatin1Char('/'));
    return QStringLiteral("http://%1:%2%3").arg(m_cfg.host).arg(m_cfg.port).arg(m);
}

QString BroadcastController::stateText() const {
    if (!m_startError.isEmpty()) return QStringLiteral("No se pudo iniciar");
    switch (m_bc.state()) {
    case Broadcaster::Off: return QStringLiteral("Desconectado");
    case Broadcaster::Connecting: return QStringLiteral("Conectando…");
    case Broadcaster::Live: return QStringLiteral("EN VIVO");
    case Broadcaster::Error: return QStringLiteral("Sin conexión · reintentando");
    case Broadcaster::Retrying: return QStringLiteral("Reconectando…");
    }
    return {};
}

bool BroadcastController::start() {
    m_startError.clear();
    QString err;
    if (!m_bc.start(m_cfg, m_engine->sampleRate(), &err)) {
        m_startError = err;
        emit statusChanged();
        return false;
    }
    m_engine->setTap(0, &m_bc);
    m_lastBytes = 0;
    m_lastMs = QDateTime::currentMSecsSinceEpoch();
    m_kbps = 0;
    m_title.clear();
    tick();
    m_timer.start();
    emit statusChanged();
    return true;
}

void BroadcastController::stop() {
    m_engine->setTap(0, nullptr);
    m_bc.stop();
    m_timer.stop();
    m_kbps = 0;
    emit statusChanged();
}

// La pista que más se oye: suena y pesa más según volumen × crossfader (misma curva que el mezclador).
QString BroadcastController::currentTitle() const {
    const double xf = m_mixer ? m_mixer->crossfader() : 0.0;
    const double ga = (m_a->playing() ? 1.0 : 0.0) * m_a->volume() * (xf <= 0 ? 1.0 : 1.0 - xf);
    const double gb = (m_b->playing() ? 1.0 : 0.0) * m_b->volume() * (xf >= 0 ? 1.0 : 1.0 + xf);
    DeckController* d = ga >= gb ? m_a : m_b;
    if (std::max(ga, gb) <= 0.01 || !d->loaded()) return {};
    return d->artist().isEmpty() ? d->title() : d->artist() + QStringLiteral(" - ") + d->title();
}

void BroadcastController::tick() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int64_t bytes = m_bc.bytesSent();
    if (now - m_lastMs >= 1000) {
        const double k = (bytes - m_lastBytes) * 8.0 / ((now - m_lastMs) / 1000.0) / 1000.0;
        m_kbps = m_kbps <= 0 ? k : m_kbps * 0.6 + k * 0.4;
        m_lastBytes = bytes;
        m_lastMs = now;
    }
    if (m_sendTitles) {
        const QString t = currentTitle();
        if (!t.isEmpty() && t != m_title) { m_title = t; m_bc.setTitle(t); }
    }
    emit statusChanged();
}

} // namespace jx
