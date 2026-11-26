// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Sampler.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace jx {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

void Sampler::setSample(int slot, const Sample* s) {
    if (slot < 0 || slot >= kSlots) return;
    m_slots[slot].sample.store(s, std::memory_order_release);
}

void Sampler::process(float* master, float* cue, int n) {
    const float vol = volume.load(std::memory_order_relaxed);
    const bool toCue = pfl.load(std::memory_order_relaxed);
    const float fadeStep = 1.f / static_cast<float>(std::max(1.0, m_sr * 0.004));   // 4 ms
    for (int s = 0; s < kSlots; ++s) {
        Slot& sl = m_slots[s];
        const Sample* smp = sl.sample.load(std::memory_order_acquire);
        if (smp != sl.cur) { sl.cur = smp; sl.playing = false; sl.pos = 0; }
        const unsigned tr = sl.triggers.load(std::memory_order_acquire);
        const unsigned st = sl.stops.load(std::memory_order_acquire);
        const int md = mode[s].load(std::memory_order_relaxed);
        if (tr != sl.seenTriggers) {
            sl.seenTriggers = tr;
            if (md == Loop && sl.playing && !sl.fadingOut) sl.fadingOut = true;   // en modo loop, otro toque detiene
            else if (smp) { sl.playing = true; sl.pos = 0; sl.fade = 0.f; sl.fadingOut = false; }
        }
        if (st != sl.seenStops) {
            sl.seenStops = st;
            if (sl.playing) sl.fadingOut = true;
        }
        if (!sl.playing || !smp || smp->frames == 0) {
            sl.playingOut.store(false, std::memory_order_relaxed);
            continue;
        }
        const float g = gain[s].load(std::memory_order_relaxed) * vol;
        for (int i = 0; i < n; ++i) {
            if (sl.pos >= smp->frames) {
                if (md == Loop && !sl.fadingOut) sl.pos = 0;
                else { sl.playing = false; break; }
            }
            if (sl.fadingOut) {
                sl.fade -= fadeStep;
                if (sl.fade <= 0) { sl.playing = false; sl.fadingOut = false; break; }
            } else if (sl.fade < 1.f) {
                sl.fade = std::min(1.f, sl.fade + fadeStep);
            }
            const float l = smp->pcm[sl.pos * 2] * g * sl.fade;
            const float r = smp->pcm[sl.pos * 2 + 1] * g * sl.fade;
            master[2 * i] += l;
            master[2 * i + 1] += r;
            if (toCue) { cue[2 * i] += l; cue[2 * i + 1] += r; }
            ++sl.pos;
        }
        sl.playingOut.store(sl.playing, std::memory_order_relaxed);
        sl.progressOut.store(smp->frames ? static_cast<double>(sl.pos) / smp->frames : 0, std::memory_order_relaxed);
    }
}

int builtinSampleCount() { return 8; }

std::unique_ptr<Sample> makeBuiltinSample(int index, double sr) {
    auto s = std::make_unique<Sample>();
    std::mt19937 rng(1234 + index);
    std::uniform_real_distribution<float> uni(-1.f, 1.f);
    auto alloc = [&](double secs) {
        s->frames = static_cast<int64_t>(secs * sr);
        s->pcm.assign(static_cast<size_t>(s->frames) * 2, 0.f);
    };
    auto put = [&](int64_t i, float l, float r) { s->pcm[i * 2] = l; s->pcm[i * 2 + 1] = r; };

    switch (index) {
    case 0: {   // Bocina: tres ráfagas de dientes de sierra desafinados con vibrato
        s->name = "BOCINA";
        alloc(1.6);
        const double bursts[3][2] = {{0.0, 0.18}, {0.24, 0.42}, {0.5, 1.5}};
        double ph[3] = {0, 0, 0};
        const double det[3] = {1.0, 1.006, 0.994};
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            float env = 0;
            for (auto& b : bursts) {
                if (t >= b[0] && t < b[1]) {
                    const double a = std::min(1.0, (t - b[0]) / 0.01) * std::min(1.0, (b[1] - t) / 0.05);
                    env = static_cast<float>(a);
                }
            }
            const double f = 466.16 * (1 + 0.012 * std::sin(kTwoPi * 6 * t)) * (t > 0.5 ? 1 - 0.04 * std::min(1.0, (t - 0.5)) : 1.0);
            double v = 0;
            for (int k = 0; k < 3; ++k) {
                ph[k] += f * det[k] / sr;
                ph[k] -= std::floor(ph[k]);
                v += 2 * ph[k] - 1;
            }
            const float x = static_cast<float>(std::tanh(v * 0.9) * 0.55) * env;
            put(i, x, x);
        }
        break;
    }
    case 1: {   // Sirena
        s->name = "SIRENA";
        alloc(2.4);
        double ph = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            const double f = 700 + 500 * std::sin(kTwoPi * 0.9 * t - 1.57);
            ph += f / sr;
            const double env = std::min(1.0, t / 0.05) * std::min(1.0, (2.4 - t) / 0.2);
            const float x = static_cast<float>((std::sin(kTwoPi * ph) + 0.3 * std::sin(2 * kTwoPi * ph)) * 0.4 * env);
            put(i, x, x);
        }
        break;
    }
    case 2: {   // Láser
        s->name = "LÁSER";
        alloc(0.5);
        double ph = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            const double f = 2400 * std::exp(-t * 9) + 80;
            ph += f / sr;
            const float x = static_cast<float>((ph - std::floor(ph) < 0.5 ? 0.45 : -0.45) * std::exp(-t * 5));
            put(i, x, x * 0.9f);
        }
        break;
    }
    case 3: {   // Bombo 808
        s->name = "BOMBO";
        alloc(0.9);
        double ph = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            ph += (45 + 110 * std::exp(-t * 30)) / sr;
            const float x = static_cast<float>(std::tanh(std::sin(kTwoPi * ph) * 1.6) * std::exp(-t * 4.5) * 0.8);
            put(i, x, x);
        }
        break;
    }
    case 4: {   // Palmas
        s->name = "PALMAS";
        alloc(0.5);
        float lp = 0, hp = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            double env = 0;
            for (double o : {0.0, 0.011, 0.022}) if (t >= o) env = std::max(env, std::exp(-(t - o) * (o < 0.02 ? 140 : 18)));
            const float nz = uni(rng);
            lp += 0.35f * (nz - lp);
            hp = nz - lp;
            const float x = hp * static_cast<float>(env) * 0.7f;
            put(i, x, x);
        }
        break;
    }
    case 5: {   // Subida (riser): ruido filtrado que sube durante 4 s
        s->name = "SUBIDA";
        alloc(4.0);
        float y = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            const float a = static_cast<float>(std::min(0.95, 0.01 + 0.9 * std::pow(t / 4.0, 2.0)));
            y += a * (uni(rng) - y);
            const float env = static_cast<float>(std::pow(t / 4.0, 1.5) * 0.5);
            put(i, y * env, uni(rng) * 0.02f * env + y * env * 0.95f);
        }
        break;
    }
    case 6: {   // Impacto: golpe grave con cola
        s->name = "IMPACTO";
        alloc(2.5);
        double ph = 0;
        float lp = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            ph += (35 + 60 * std::exp(-t * 6)) / sr;
            lp += 0.05f * (uni(rng) - lp);
            const float x = static_cast<float>(std::sin(kTwoPi * ph) * std::exp(-t * 1.6) * 0.8 + lp * std::exp(-t * 3) * 1.2);
            put(i, x, x);
        }
        break;
    }
    case 7: {   // Rebobinado: barrido descendente rugoso
        s->name = "REWIND";
        alloc(1.2);
        double ph = 0;
        for (int64_t i = 0; i < s->frames; ++i) {
            const double t = i / sr;
            const double f = 900 * (1 - t / 1.2) + 60 + 300 * std::sin(kTwoPi * 18 * t);
            ph += f / sr;
            const float x = static_cast<float>((std::sin(kTwoPi * ph) * 0.5 + uni(rng) * 0.12) * std::min(1.0, (1.2 - t) / 0.1) * 0.6);
            put(i, x, x);
        }
        break;
    }
    default:
        return nullptr;
    }
    return s;
}

} // namespace jx
