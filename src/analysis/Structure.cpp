// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Structure.h"

#include "engine/Track.h"

#include <algorithm>
#include <cmath>

namespace jx {

QString Section::name(Kind k) {
    switch (k) {
    case Intro: return QStringLiteral("Intro");
    case Body: return QStringLiteral("Cuerpo");
    case Break: return QStringLiteral("Break");
    case Drop: return QStringLiteral("Drop");
    case Outro: return QStringLiteral("Outro");
    }
    return {};
}

const Section* firstOf(const std::vector<Section>& s, Section::Kind k) {
    for (const Section& x : s) if (x.kind == k) return &x;
    return nullptr;
}
const Section* lastOf(const std::vector<Section>& s, Section::Kind k) {
    for (auto it = s.rbegin(); it != s.rend(); ++it) if (it->kind == k) return &*it;
    return nullptr;
}

namespace {
double percentile(std::vector<double> v, double q) {
    if (v.empty()) return 0;
    const size_t i = std::min(v.size() - 1, size_t(q * (v.size() - 1) + 0.5));
    std::nth_element(v.begin(), v.begin() + i, v.end());
    return v[i];
}
} // namespace

std::vector<Section> analyzeStructure(const Track& t) {
    std::vector<Section> out;
    if (t.bpm <= 0 || t.sampleRate <= 0 || t.wfBinFrames <= 0 || t.wfLow.empty()) return out;
    const double sr = t.sampleRate;
    const double bar = 4.0 * sr * 60.0 / t.bpm;            // cuadros por compás
    const double fb = std::fmod(std::max(0.0, t.firstBeat), bar);  // primer compás completo
    const int nBars = int((t.frames - fb) / bar);
    const double dur = t.frames / sr;
    const size_t bins = t.wfLow.size();

    // 1) Rasgos por compás: graves (bombo + bajo) y energía total.
    std::vector<double> low(nBars, 0), all(nBars, 0);
    for (int b = 0; b < nBars; ++b) {
        const size_t i0 = size_t((fb + b * bar) / t.wfBinFrames);
        const size_t i1 = std::min(bins, size_t((fb + (b + 1) * bar) / t.wfBinFrames));
        double l = 0, e = 0;
        for (size_t i = i0; i < i1; ++i) {
            const double lo = t.wfLow[i], mi = i < t.wfMid.size() ? t.wfMid[i] : 0, hi = i < t.wfHigh.size() ? t.wfHigh[i] : 0;
            l += lo;
            e += (lo + mi + hi) / 3.0;
        }
        const double n = std::max<size_t>(1, i1 - i0);
        low[b] = l / n;
        all[b] = e / n;
    }
    auto single = [&]() {
        out.push_back({Section::Body, 0.0, dur, nBars, 1.0, 1.0});
        return out;
    };
    if (nBars < 12) return single();

    const double lRef = std::max(1.0, percentile(low, 0.9));
    const double eRef = std::max(1.0, percentile(all, 0.9));
    auto meanOf = [](const std::vector<double>& v, int a, int b) {
        a = std::max(a, 0); b = std::min<int>(b, int(v.size()));
        double s = 0; for (int i = a; i < b; ++i) s += v[i];
        return b > a ? s / (b - a) : 0.0;
    };

    // 2) Cambios de sección: novedad cada 4 compases (frases de 4/8/16 compases).
    std::vector<int> cand, bounds{0};
    std::vector<double> nov;
    for (int k = 4; k <= nBars - 4; k += 4) {
        const double dl = std::abs(meanOf(low, k - 4, k) - meanOf(low, k, k + 4)) / lRef;
        const double de = std::abs(meanOf(all, k - 4, k) - meanOf(all, k, k + 4)) / eRef;
        cand.push_back(k);
        nov.push_back(dl + de);
    }
    for (size_t i = 0; i < cand.size(); ++i) {
        const double prev = i > 0 ? nov[i - 1] : 0, next = i + 1 < nov.size() ? nov[i + 1] : 0;
        if (nov[i] > 0.22 && nov[i] >= prev && nov[i] >= next) bounds.push_back(cand[i]);
    }
    bounds.push_back(nBars);

    // 3) Rasgos por segmento y etiquetas.
    struct Seg { int a, b; double e, l; };
    std::vector<Seg> segs;
    for (size_t i = 0; i + 1 < bounds.size(); ++i) {
        const int a = bounds[i], b = bounds[i + 1];
        segs.push_back({a, b, std::min(1.0, meanOf(all, a, b) / eRef), std::min(1.0, meanOf(low, a, b) / lRef)});
    }
    if (segs.size() < 2) return single();
    auto loud = [](const Seg& s) { return s.e >= 0.7 && s.l >= 0.55; };

    std::vector<Section::Kind> kinds(segs.size(), Section::Body);
    for (size_t i = 0; i < segs.size(); ++i) {
        if (loud(segs[i])) {
            kinds[i] = (i > 0 && kinds[i - 1] == Section::Break) ? Section::Drop : Section::Body;
        } else if (i == 0) {
            kinds[i] = Section::Intro;
        } else if (i + 1 == segs.size()) {
            kinds[i] = Section::Outro;
        } else {
            kinds[i] = Section::Break;
        }
    }
    // Una "intro" que sigue sin cuerpo hasta el final, o varias secciones suaves seguidas al principio/fin.
    for (size_t i = 1; i < segs.size(); ++i)
        if (kinds[i] == Section::Break && kinds[i - 1] == Section::Intro) kinds[i] = Section::Intro;
    for (size_t i = segs.size() - 1; i-- > 0;)
        if (kinds[i] == Section::Break && kinds[i + 1] == Section::Outro) kinds[i] = Section::Outro;

    // 4) Unir vecinas iguales y pasar a segundos (la primera empieza en 0 y la última acaba al final).
    for (size_t i = 0; i < segs.size(); ++i) {
        const double s0 = (fb + segs[i].a * bar) / sr, s1 = (fb + segs[i].b * bar) / sr;
        if (!out.empty() && out.back().kind == kinds[i]) {
            Section& p = out.back();
            const double w0 = p.bars, w1 = segs[i].b - segs[i].a;
            p.energy = (p.energy * w0 + segs[i].e * w1) / (w0 + w1);
            p.bass = (p.bass * w0 + segs[i].l * w1) / (w0 + w1);
            p.end = s1;
            p.bars += segs[i].b - segs[i].a;
        } else {
            out.push_back({kinds[i], s0, s1, segs[i].b - segs[i].a, segs[i].e, segs[i].l});
        }
    }
    out.front().start = 0.0;
    out.back().end = dur;
    return out;
}

} // namespace jx
