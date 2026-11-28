// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "RadioController.h"

#include "DeckController.h"
#include "Mixer.h"
#include "engine/AudioEngine.h"

#include <QSettings>
#include <algorithm>

namespace jx {

RadioController::RadioController(AudioEngine* engine, DeckController* a, DeckController* b, Mixer* mixer, QObject* parent)
    : QObject(parent), m_engine(engine), m_a(a), m_b(b), m_mixer(mixer) {
    QSettings s;
    m_cfg.port = s.value("radio/port", 8000).toInt();
    m_cfg.mp3Bitrate = s.value("radio/bitrate", 128).toInt();
    m_cfg.name = s.value("radio/name", QStringLiteral("Jocker-MX en vivo")).toString();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &RadioController::tick);
    connect(&m_radio, &LocalRadio::listenersChanged, this, &RadioController::statusChanged, Qt::QueuedConnection);
}

RadioController::~RadioController() {
    if (m_engine) m_engine->setTap(1, nullptr);
    m_radio.stop();
}

void RadioController::save() {
    QSettings s;
    s.setValue("radio/port", m_cfg.port);
    s.setValue("radio/bitrate", m_cfg.mp3Bitrate);
    s.setValue("radio/name", m_cfg.name);
    emit configChanged();
}

void RadioController::setPort(int p) { p = std::clamp(p, 1024, 65535); if (p != m_cfg.port) { m_cfg.port = p; save(); } }
void RadioController::setBitrate(int b) { b = std::clamp(b, 64, 320); if (b != m_cfg.mp3Bitrate) { m_cfg.mp3Bitrate = b; save(); } }
void RadioController::setName(const QString& n) { if (!n.trimmed().isEmpty() && n != m_cfg.name) { m_cfg.name = n.trimmed(); save(); } }

QStringList RadioController::urls() const {
    QStringList out;
    if (!running()) return out;
    for (const QString& ip : LocalRadio::lanAddresses()) out << QStringLiteral("http://%1:%2/").arg(ip).arg(m_radio.port());
    if (out.isEmpty()) out << QStringLiteral("http://localhost:%1/").arg(m_radio.port());
    return out;
}

QString RadioController::url() const { const QStringList u = urls(); return u.isEmpty() ? QString() : u.first(); }

bool RadioController::start() {
    m_error.clear();
    LocalRadio::Config c = m_cfg;
    c.opusBitrate = std::clamp(m_cfg.mp3Bitrate * 3 / 4, 64, 192);
    if (!m_radio.start(c, m_engine->sampleRate(), &m_error)) { emit statusChanged(); return false; }
    m_engine->setTap(1, &m_radio);
    m_title.clear();
    tick();
    m_timer.start();
    emit statusChanged();
    return true;
}

void RadioController::stop() {
    m_engine->setTap(1, nullptr);
    m_radio.stop();
    m_timer.stop();
    emit statusChanged();
}

QString RadioController::currentTitle() const {
    const double xf = m_mixer ? m_mixer->crossfader() : 0.0;
    const double ga = (m_a->playing() ? 1.0 : 0.0) * m_a->volume() * (xf <= 0 ? 1.0 : 1.0 - xf);
    const double gb = (m_b->playing() ? 1.0 : 0.0) * m_b->volume() * (xf >= 0 ? 1.0 : 1.0 + xf);
    DeckController* d = ga >= gb ? m_a : m_b;
    if (std::max(ga, gb) <= 0.01 || !d->loaded()) return {};
    return d->artist().isEmpty() ? d->title() : d->artist() + QStringLiteral(" - ") + d->title();
}

void RadioController::tick() {
    const QString t = currentTitle();
    if (t != m_title) { m_title = t; m_radio.setTitle(t); }
    emit statusChanged();
}

} // namespace jx
