// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Mixer.h"

namespace jx {

Mixer::Mixer(AudioEngine* engine, QList<DeckController*> decks, QObject* parent)
    : QObject(parent), m_engine(engine), m_decks(std::move(decks)) {
    m_timer.setInterval(16);     // ~60 Hz
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &Mixer::tick);
    m_timer.start();
}

void Mixer::tick() {
    for (auto* d : m_decks) d->tick();
    const double l = std::max<double>(m_engine->masterPeakL.load(), m_l * 0.86);
    const double r = std::max<double>(m_engine->masterPeakR.load(), m_r * 0.86);
    const bool lim = m_engine->limiterGainReduction.load() > 0.01f;
    if (std::abs(l - m_l) > 1e-4 || std::abs(r - m_r) > 1e-4 || lim != m_limiting) {
        m_l = l; m_r = r; m_limiting = lim;
        emit levelsChanged();
    }
}

} // namespace jx
