// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Effects.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace jx {

namespace {
constexpr double kTwoPi = 6.283185307179586;
// Longitudes de Freeverb (a 44.1 kHz), se escalan a la frecuencia real
const int kCombLen[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
const int kApLen[4] = {556, 441, 341, 225};
constexpr int kStereoSpread = 23;
} // namespace

const char* EffectUnit::name(int t) {
    switch (t) {
    case Echo: return "ECO";
    case Reverb: return "REVERB";
    case Flanger: return "FLANGER";
    case Phaser: return "PHASER";
    case Crusher: return "CRUSH";
    case Gate: return "GATE";
    default: return "—";
    }
}

void EffectUnit::prepare(double sr) {
    m_sr = sr;
    m_delay.assign(static_cast<size_t>(sr * 8.5) * 2, 0.f);    // hasta 8 beats a 60 BPM
    m_short.assign(static_cast<size_t>(sr * 0.03) * 2, 0.f);
    const double scale = sr / 44100.0;
    for (int c = 0; c < 2; ++c) {
        for (int k = 0; k < 8; ++k) m_combs[c][k].b.assign(static_cast<size_t>((kCombLen[k] + c * kStereoSpread) * scale), 0.f);
        for (int k = 0; k < 4; ++k) m_aps[c][k].b.assign(static_cast<size_t>((kApLen[k] + c * kStereoSpread) * scale), 0.f);
    }
    reset();
}

void EffectUnit::reset() {
    std::fill(m_delay.begin(), m_delay.end(), 0.f);
    std::fill(m_short.begin(), m_short.end(), 0.f);
    m_delayW = m_shortW = 0;
    m_fbLp[0] = m_fbLp[1] = 0;
    std::memset(m_apState, 0, sizeof(m_apState));
    m_phaserFb[0] = m_phaserFb[1] = 0;
    for (auto& ch : m_combs) for (auto& cb : ch) { std::fill(cb.b.begin(), cb.b.end(), 0.f); cb.i = 0; cb.store = 0; }
    for (auto& ch : m_aps) for (auto& ap : ch) { std::fill(ap.b.begin(), ap.b.end(), 0.f); ap.i = 0; }
    m_hold[0] = m_hold[1] = 0;
    m_holdPhase = 0;
    m_gateEnv = 1.f;
}

void EffectUnit::process(float* buf, int n, double beatPos, double beatSamples) {
    const int t = type.load(std::memory_order_relaxed);
    const float targetWet = (t == None) ? 0.f : wet.load(std::memory_order_relaxed);
    if (t != None && t != m_lastType) {      // cambio de efecto: limpiar buffers
        reset();
        m_lastType = t;
    }
    if (t == None && m_wetSmooth < 1e-4f) { m_wetSmooth = 0; return; }
    if (m_delay.empty()) return;

    const double bs = beatSamples > 100 ? beatSamples : m_sr * 0.5;   // 120 BPM si no hay rejilla
    const float beatsLen = std::clamp(beats.load(std::memory_order_relaxed), 0.0625f, 8.f);
    const float wStep = (targetWet - m_wetSmooth) / n;
    const int active = m_lastType;            // al apagar, el último efecto se desvanece

    switch (active) {
    case Echo: {
        const int dl = static_cast<int>(std::clamp(bs * beatsLen, 64.0, m_delay.size() / 2.0 - 2));
        const int size = static_cast<int>(m_delay.size() / 2);
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            const int r = (m_delayW - dl + size) % size;
            for (int c = 0; c < 2; ++c) {
                const float x = buf[2 * i + c];
                const float d = m_delay[2 * r + c];
                m_fbLp[c] += 0.35f * (d - m_fbLp[c]);                    // eco que se oscurece
                // Entrada sólo mientras el efecto está activo: al apagarlo, la cola se desvanece
                const float in = t == Echo ? x : 0.f;
                m_delay[2 * m_delayW + c] = in + m_fbLp[c] * (0.35f + 0.4f * m_wetSmooth);
                buf[2 * i + c] = x + d * m_wetSmooth;
            }
            m_delayW = (m_delayW + 1) % size;
        }
        break;
    }
    case Reverb: {
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            const float room = 0.70f + 0.27f * m_wetSmooth, damp = 0.3f;
            const float input = (buf[2 * i] + buf[2 * i + 1]) * 0.015f;
            for (int c = 0; c < 2; ++c) {
                float out = 0;
                for (auto& cb : m_combs[c]) {
                    const float y = cb.b[cb.i];
                    cb.store = y * (1 - damp) + cb.store * damp;
                    cb.b[cb.i] = input + cb.store * room;
                    if (++cb.i >= static_cast<int>(cb.b.size())) cb.i = 0;
                    out += y;
                }
                for (auto& ap : m_aps[c]) {
                    const float bo = ap.b[ap.i];
                    ap.b[ap.i] = out + bo * 0.5f;
                    out = bo - out;
                    if (++ap.i >= static_cast<int>(ap.b.size())) ap.i = 0;
                }
                buf[2 * i + c] = buf[2 * i + c] * (1.f - 0.3f * m_wetSmooth) + out * 1.6f * m_wetSmooth;
            }
        }
        break;
    }
    case Flanger: {
        const int size = static_cast<int>(m_short.size() / 2);
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            const double ph = (beatPos + i / bs) / beatsLen;
            const double lfo = 0.5 - 0.5 * std::cos(kTwoPi * ph);              // 0..1 sincronizado al beat
            const double dSamples = (0.0007 + 0.0055 * lfo) * m_sr;
            double rp = m_shortW - dSamples;
            while (rp < 0) rp += size;
            const int r0 = static_cast<int>(rp);
            const float fr = static_cast<float>(rp - r0);
            const int r1 = (r0 + 1) % size;
            for (int c = 0; c < 2; ++c) {
                const float d = m_short[2 * r0 + c] * (1 - fr) + m_short[2 * r1 + c] * fr;
                const float x = buf[2 * i + c];
                m_short[2 * m_shortW + c] = x + d * 0.65f;
                buf[2 * i + c] = x * (1 - 0.5f * m_wetSmooth) + d * 0.8f * m_wetSmooth;
            }
            m_shortW = (m_shortW + 1) % size;
        }
        break;
    }
    case Phaser: {
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            const double ph = (beatPos + i / bs) / beatsLen;
            const double lfo = 0.5 - 0.5 * std::cos(kTwoPi * ph);
            const double f = 300.0 * std::pow(12.0, lfo);                    // 300 Hz – 3.6 kHz
            const float a = static_cast<float>((1 - std::tan(kTwoPi * f / (2 * m_sr))) / (1 + std::tan(kTwoPi * f / (2 * m_sr))));
            for (int c = 0; c < 2; ++c) {
                const float x = buf[2 * i + c];
                float y = x + m_phaserFb[c] * 0.55f;
                for (int s = 0; s < 6; ++s) {                                 // 6 etapas pasa-todo
                    const float out = -a * y + m_apState[c][s];
                    m_apState[c][s] = y + a * out;
                    y = out;
                }
                m_phaserFb[c] = y;
                buf[2 * i + c] = x * (1 - 0.5f * m_wetSmooth) + y * 0.7f * m_wetSmooth;
            }
        }
        break;
    }
    case Crusher: {
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            const float bits = 16.f - 12.f * m_wetSmooth;                     // 16 → 4 bits
            const float q = std::pow(2.f, bits - 1);
            const double down = 1.0 + 15.0 * m_wetSmooth;                     // reducción de frecuencia
            m_holdPhase += 1.0;
            if (m_holdPhase >= down) {
                m_holdPhase -= down;
                for (int c = 0; c < 2; ++c) m_hold[c] = std::round(buf[2 * i + c] * q) / q;
            }
            for (int c = 0; c < 2; ++c) buf[2 * i + c] = buf[2 * i + c] * (1 - m_wetSmooth) + m_hold[c] * m_wetSmooth;
        }
        break;
    }
    case Gate: {
        const float att = static_cast<float>(1.0 - std::exp(-1.0 / (0.002 * m_sr)));
        for (int i = 0; i < n; ++i) {
            m_wetSmooth += wStep;
            double ph = std::fmod((beatPos + i / bs) / beatsLen, 1.0);
            if (ph < 0) ph += 1.0;
            const float target = ph < 0.5 ? 1.f : 1.f - m_wetSmooth;          // corta la segunda mitad de cada paso
            m_gateEnv += (target - m_gateEnv) * att;
            buf[2 * i] *= m_gateEnv;
            buf[2 * i + 1] *= m_gateEnv;
        }
        break;
    }
    default:
        m_wetSmooth = 0;
        break;
    }
}

} // namespace jx
