// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Deck.h"

#include <rubberband/RubberBandStretcher.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace jx {

namespace {
// Interpolación Hermite de 4 puntos
inline float hermite(float xm1, float x0, float x1, float x2, float t) {
    const float c = (x1 - xm1) * 0.5f;
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + (x2 - x0) * 0.5f;
    const float b = w + a;
    return ((((a * t) - b) * t + c) * t + x0);
}

inline float sampleAt(const Track* t, int64_t i, int ch) {
    if (i < 0 || i >= t->frames) return 0.0f;
    return t->pcm[static_cast<size_t>(i) * 2 + ch];
}
} // namespace

Deck::Deck(int index) : m_index(index) {}
Deck::~Deck() = default;

void Deck::prepare(double sampleRate, int maxBlock) {
    m_sr = sampleRate;
    m_maxBlock = std::max(maxBlock, 4096);
    using RB = RubberBand::RubberBandStretcher;
    m_rb = std::make_unique<RB>(static_cast<size_t>(sampleRate), 2,
                                RB::OptionProcessRealTime | RB::OptionEngineFaster |
                                    RB::OptionTransientsCrisp | RB::OptionPitchHighConsistency,
                                1.0, 1.0);
    m_rb->setMaxProcessSize(m_maxBlock);
    m_inL.assign(m_maxBlock, 0.f); m_inR.assign(m_maxBlock, 0.f);
    m_outL.assign(m_maxBlock, 0.f); m_outR.assign(m_maxBlock, 0.f);
}

void Deck::setTrack(const Track* t) {
    playing.store(false);
    loopActive.store(false);
    reachedEnd.store(false);
    m_track.store(t, std::memory_order_release);
}

void Deck::process(float* out, int n) {
    const Track* t = m_track.load(std::memory_order_acquire);
    if (t != m_cur) {
        m_cur = t;
        m_pos = 0.0;
        m_rate = 0.0;
        if (m_rb) m_rb->reset();
        position.store(0.0, std::memory_order_relaxed);
    }
    const double seek = m_seekRequest.exchange(-1.0, std::memory_order_acq_rel);
    if (seek >= 0.0) {
        m_pos = seek;
        if (m_rb) m_rb->reset();
    }
    if (!t || t->frames == 0) {
        std::memset(out, 0, sizeof(float) * n * 2);
        return;
    }

    const bool scratch = scratchActive.load(std::memory_order_relaxed);
    double target = 0.0;
    if (scratch) target = scratchRate.load(std::memory_order_relaxed);
    else if (playing.load(std::memory_order_relaxed))
        target = baseRate() * (1.0 + nudge.load(std::memory_order_relaxed));

    const bool stretch = !scratch && keyLock.load(std::memory_order_relaxed) && target > 0.2 &&
                         m_rb && n <= m_maxBlock;
    if (stretch) {
        if (!m_wasStretching) m_rb->reset();
        m_wasStretching = true;
        m_rate = target;
        processStretch(t, out, n, target);
    } else {
        m_wasStretching = false;
        processResample(t, out, n, target);
    }
    position.store(m_pos, std::memory_order_relaxed);
}

void Deck::processResample(const Track* t, float* out, int n, double targetRate) {
    // Suavizado de velocidad: arranque/frenado naturales y scratch sin chasquidos.
    const double coef = scratchActive.load(std::memory_order_relaxed) ? 0.004 : 0.02;
    const bool loop = loopActive.load(std::memory_order_relaxed);
    const double ls = loopStart.load(std::memory_order_relaxed);
    const double le = loopEnd.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        m_rate += (targetRate - m_rate) * coef;
        if (std::abs(m_rate) < 1e-5 && targetRate == 0.0) {
            m_rate = 0.0;
            out[2 * i] = out[2 * i + 1] = 0.f;
            continue;
        }
        const int64_t i0 = static_cast<int64_t>(std::floor(m_pos));
        const float frac = static_cast<float>(m_pos - static_cast<double>(i0));
        for (int ch = 0; ch < 2; ++ch) {
            out[2 * i + ch] = hermite(sampleAt(t, i0 - 1, ch), sampleAt(t, i0, ch),
                                      sampleAt(t, i0 + 1, ch), sampleAt(t, i0 + 2, ch), frac);
        }
        m_pos += m_rate;
        if (loop && m_rate > 0 && le > ls && m_pos >= le) m_pos -= (le - ls);
        if (m_pos < 0) m_pos = 0;
        if (m_pos >= static_cast<double>(t->frames)) {
            m_pos = static_cast<double>(t->frames);
            if (playing.load(std::memory_order_relaxed)) {
                playing.store(false);
                reachedEnd.store(true);
            }
        }
    }
}

void Deck::processStretch(const Track* t, float* out, int n, double rate) {
    m_rb->setTimeRatio(1.0 / rate);
    const bool loop = loopActive.load(std::memory_order_relaxed);
    const double ls = loopStart.load(std::memory_order_relaxed);
    const double le = loopEnd.load(std::memory_order_relaxed);
    int guard = 0;
    while (static_cast<int>(m_rb->available()) < n && guard++ < 64) {
        size_t req = std::min<size_t>(m_rb->getSamplesRequired(), static_cast<size_t>(m_maxBlock));
        if (req == 0) req = 256;
        for (size_t k = 0; k < req; ++k) {
            const int64_t idx = static_cast<int64_t>(m_pos);
            m_inL[k] = sampleAt(t, idx, 0);
            m_inR[k] = sampleAt(t, idx, 1);
            m_pos += 1.0;
            if (loop && le > ls && m_pos >= le) m_pos -= (le - ls);
        }
        const float* in[2] = {m_inL.data(), m_inR.data()};
        m_rb->process(in, req, false);
    }
    float* outs[2] = {m_outL.data(), m_outR.data()};
    const size_t got = m_rb->retrieve(outs, static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        out[2 * i] = i < static_cast<int>(got) ? m_outL[i] : 0.f;
        out[2 * i + 1] = i < static_cast<int>(got) ? m_outR[i] : 0.f;
    }
    if (m_pos >= static_cast<double>(t->frames)) {
        m_pos = static_cast<double>(t->frames);
        playing.store(false);
        reachedEnd.store(true);
    }
}

} // namespace jx
