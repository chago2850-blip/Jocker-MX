// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "AutomixPlan.h"

#include "engine/Track.h"

#include <algorithm>
#include <cmath>

namespace jx {

namespace {
// Nivel de la forma de onda (0..255) en un bin.
int binLevel(const Track& t, size_t i) {
    int v = 0;
    if (i < t.wfLow.size()) v = std::max<int>(v, t.wfLow[i]);
    if (i < t.wfMid.size()) v = std::max<int>(v, t.wfMid[i]);
    if (i < t.wfHigh.size()) v = std::max<int>(v, t.wfHigh[i]);
    return v;
}
} // namespace

bool tempoCompatible(double bpmA, double bpmB, double maxStretch) {
    if (bpmA <= 0 || bpmB <= 0) return false;
    double r = bpmA / bpmB;
    while (r > 1.5) r /= 2.0;
    while (r < 0.67) r *= 2.0;
    return std::abs(r - 1.0) <= maxStretch + 1e-9;
}

MixPoints computeMixPoints(const Track& t, int transitionBeats, double fallbackSeconds) {
    MixPoints m;
    const double sr = t.sampleRate > 0 ? t.sampleRate : 48000.0;
    const double dur = t.frames / sr;
    m.lastAudible = dur;

    // 1) Silencio al principio y al final: umbral relativo al pico de la pista.
    const size_t bins = std::max({t.wfLow.size(), t.wfMid.size(), t.wfHigh.size()});
    if (bins > 0 && t.wfBinFrames > 0) {
        int peak = 0;
        for (size_t i = 0; i < bins; ++i) peak = std::max(peak, binLevel(t, i));
        const int thr = std::max(3, peak / 12);   // ~ -22 dB respecto al pico
        size_t a = 0, b = bins;
        while (a < bins && binLevel(t, a) < thr) ++a;
        while (b > a && binLevel(t, b - 1) < thr) --b;
        if (a < b) {
            m.firstAudible = a * double(t.wfBinFrames) / sr;
            m.lastAudible = std::min(dur, b * double(t.wfBinFrames) / sr);
        }
    }
    m.mixIn = m.firstAudible;

    // 2) Con beatgrid: entrar en el primer beat audible y salir al inicio de un compás.
    if (t.bpm > 0 && transitionBeats > 0) {
        const double beat = 60.0 / t.bpm;
        const double fb = t.firstBeat / sr;
        const double inIdx = std::ceil((m.firstAudible - fb) / beat - 0.25);   // tolera un ataque algo adelantado
        m.mixIn = std::max(0.0, fb + inIdx * beat);

        double trans = transitionBeats * beat;
        const double usable = m.lastAudible - m.mixIn;
        // Pistas cortas: la transición nunca pasa de un tercio de lo que suena.
        while (trans > usable / 3.0 && transitionBeats > 4) { transitionBeats /= 2; trans = transitionBeats * beat; }
        const double lastBeatIdx = std::floor((m.lastAudible - fb) / beat + 1e-6);
        double outIdx = lastBeatIdx - transitionBeats;
        // Alinear al compás (4 beats) contado desde el primer beat de la pista.
        outIdx = std::floor(outIdx / 4.0) * 4.0;
        double out = fb + outIdx * beat;
        if (out < m.mixIn + beat) out = std::max(m.mixIn, m.lastAudible - trans);
        m.mixOut = out;
        m.transition = trans;
        m.beatAligned = true;
    } else {
        const double usable = std::max(0.0, m.lastAudible - m.mixIn);
        m.transition = std::min(fallbackSeconds, usable / 3.0);
        m.mixOut = std::max(m.mixIn, m.lastAudible - m.transition);
    }
    return m;
}

} // namespace jx
