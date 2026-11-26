// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Analyzer.h"

#include "KeyDetector.h"
#include "PlatformDecoder.h"
#include "library/TagReader.h"
#include "miniaudio.h"

#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace jx {

void guessTitleArtist(const QString& path, QString* title, QString* artist) {
    QString base = QFileInfo(path).completeBaseName();
    base.replace('_', ' ');
    const int sep = base.indexOf(" - ");
    if (sep > 0) {
        *artist = base.left(sep).trimmed();
        *title = base.mid(sep + 3).trimmed();
    } else {
        *artist = QString();
        *title = base.trimmed();
    }
}

bool decodeToStereo(const QString& path, double sampleRate, std::vector<float>& pcm, QString* error, double maxSeconds) {
    pcm.clear();
    ma_decoder dec;
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 2, static_cast<ma_uint32>(sampleRate));
    cfg.resampling.linear.lpfOrder = 8;
#ifdef _WIN32
    const ma_result r = ma_decoder_init_file_w(reinterpret_cast<const wchar_t*>(path.utf16()), &cfg, &dec);
#else
    const ma_result r = ma_decoder_init_file(path.toUtf8().constData(), &cfg, &dec);
#endif
    if (r != MA_SUCCESS) {
        // AAC/M4A, ALAC, WMA…: decodificador del sistema
        if (platformDecode(path, sampleRate, pcm, error)) {
            const size_t maxSamples = static_cast<size_t>(sampleRate * 2 * maxSeconds);
            if (pcm.size() > maxSamples) pcm.resize(maxSamples);
            return true;
        }
        if (error && error->isEmpty()) *error = QStringLiteral("Formato no soportado o archivo dañado");
        return false;
    }
    ma_uint64 total = 0;
    if (ma_decoder_get_length_in_pcm_frames(&dec, &total) == MA_SUCCESS && total > 0)
        pcm.reserve(static_cast<size_t>(total) * 2 + 8192);
    std::vector<float> chunk(16384 * 2);
    const size_t maxSamples = static_cast<size_t>(sampleRate * 2 * maxSeconds);
    for (;;) {
        ma_uint64 got = 0;
        const ma_result rr = ma_decoder_read_pcm_frames(&dec, chunk.data(), 16384, &got);
        if (got > 0) pcm.insert(pcm.end(), chunk.begin(), chunk.begin() + static_cast<long>(got * 2));
        if (rr != MA_SUCCESS || got == 0 || pcm.size() >= maxSamples) break;
    }
    ma_decoder_uninit(&dec);
    if (pcm.empty() && error) *error = QStringLiteral("El archivo no contiene audio");
    return !pcm.empty();
}

std::unique_ptr<Track> loadAndAnalyze(const QString& path, double sampleRate, QString* error) {
    auto t = std::make_unique<Track>();
    t->path = path;
    t->sampleRate = sampleRate;
    if (!decodeToStereo(path, sampleRate, t->pcm, error, 4 * 3600.0)) return nullptr;
    guessTitleArtist(path, &t->title, &t->artist);
    const TrackTags tags = readTags(path);
    if (!tags.title.isEmpty()) t->title = tags.title;
    if (!tags.artist.isEmpty()) t->artist = tags.artist;
    t->album = tags.album;
    t->genre = tags.genre;
    t->musicalKey = normalizeKeyToCamelot(tags.key);

    t->frames = static_cast<int64_t>(t->pcm.size() / 2);
    if (t->frames < static_cast<int64_t>(sampleRate * 0.1)) {
        if (error) *error = QStringLiteral("El archivo no contiene audio");
        return nullptr;
    }
    computeWaveform(*t);
    const BeatInfo bi = detectBeats(t->pcm.data(), t->frames, sampleRate);
    t->bpm = bi.bpm;
    t->firstBeat = bi.firstBeat;
    if (t->musicalKey.isEmpty()) t->musicalKey = detectKey(t->pcm.data(), t->frames, sampleRate).camelot();
    return t;
}

void computeWaveform(Track& t) {
    const int bin = t.wfBinFrames;
    const size_t bins = static_cast<size_t>((t.frames + bin - 1) / bin);
    std::vector<float> lo(bins), mi(bins), hi(bins);
    // Filtros de un polo: graves < ~200 Hz, agudos > ~2.5 kHz, medios = resto.
    const double sr = t.sampleRate;
    const float aLow = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265 * 200.0 / sr));
    const float aHigh = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265 * 2500.0 / sr));
    float lp1 = 0, lp2 = 0;
    for (size_t b = 0; b < bins; ++b) {
        float sl = 0, sm = 0, sh = 0;
        const int64_t start = static_cast<int64_t>(b) * bin;
        const int64_t end = std::min<int64_t>(start + bin, t.frames);
        for (int64_t i = start; i < end; ++i) {
            const float x = 0.5f * (t.pcm[i * 2] + t.pcm[i * 2 + 1]);
            lp1 += aLow * (x - lp1);
            lp2 += aHigh * (x - lp2);
            const float l = lp1, h = x - lp2, m = lp2 - lp1;
            sl = std::max(sl, std::abs(l));
            sm = std::max(sm, std::abs(m));
            sh = std::max(sh, std::abs(h));
        }
        lo[b] = sl; mi[b] = sm; hi[b] = sh;
    }
    auto norm = [&](const std::vector<float>& v, std::vector<uint8_t>& out, float boost) {
        float mx = 1e-6f;
        for (float f : v) mx = std::max(mx, f);
        out.resize(v.size());
        for (size_t i = 0; i < v.size(); ++i)
            out[i] = static_cast<uint8_t>(std::clamp(std::pow(v[i] / mx, 0.8f) * 255.f * boost, 0.f, 255.f));
    };
    norm(lo, t.wfLow, 1.0f);
    norm(mi, t.wfMid, 0.95f);
    norm(hi, t.wfHigh, 0.9f);
}

BeatInfo detectBeats(const float* pcm, int64_t frames, double sr) {
    BeatInfo info;
    const int hop = 512;
    const int64_t nh = frames / hop;
    if (nh < 64) return info;

    // 1) Envolvente de inicios (onset): flujo de energía de graves + banda completa.
    std::vector<float> onset(static_cast<size_t>(nh), 0.f);
    const float aLow = static_cast<float>(1.0 - std::exp(-2.0 * 3.14159265 * 150.0 / sr));
    float lp = 0, prevLow = 0, prevAll = 0;
    for (int64_t h = 0; h < nh; ++h) {
        double eLow = 0, eAll = 0;
        for (int k = 0; k < hop; ++k) {
            const int64_t i = h * hop + k;
            const float x = 0.5f * (pcm[i * 2] + pcm[i * 2 + 1]);
            lp += aLow * (x - lp);
            eLow += lp * lp;
            eAll += x * x;
        }
        const float lLow = std::log1p(1000.f * static_cast<float>(eLow / hop));
        const float lAll = std::log1p(1000.f * static_cast<float>(eAll / hop));
        onset[h] = std::max(0.f, lLow - prevLow) * 1.5f + std::max(0.f, lAll - prevAll);
        prevLow = lLow; prevAll = lAll;
    }
    // Quitar la media local (realza picos).
    {
        std::vector<float> o2(onset.size());
        const int w = 8;
        double acc = 0;
        std::vector<double> cs(onset.size() + 1, 0.0);
        for (size_t i = 0; i < onset.size(); ++i) cs[i + 1] = cs[i] + onset[i];
        for (size_t i = 0; i < onset.size(); ++i) {
            const size_t a = i >= static_cast<size_t>(w) ? i - w : 0;
            const size_t b = std::min(onset.size(), i + w + 1);
            acc = (cs[b] - cs[a]) / static_cast<double>(b - a);
            o2[i] = std::max(0.f, onset[i] - static_cast<float>(acc));
        }
        onset.swap(o2);
    }

    const double hopsPerSec = sr / hop;
    auto lagForBpm = [&](double bpm) { return hopsPerSec * 60.0 / bpm; };

    // 2) Autocorrelación en el rango 60-200 BPM con preferencia suave alrededor de 120.
    const int minLag = static_cast<int>(std::floor(lagForBpm(200.0)));
    const int maxLag = static_cast<int>(std::ceil(lagForBpm(60.0)));
    const int64_t useN = std::min<int64_t>(nh, static_cast<int64_t>(hopsPerSec * 240)); // hasta 4 min
    std::vector<double> ac(maxLag + 2, 0.0);
    for (int lag = minLag; lag <= maxLag + 1; ++lag) {
        double s = 0;
        for (int64_t i = 0; i + lag < useN; ++i) s += onset[i] * onset[i + lag];
        ac[lag] = s / static_cast<double>(useN - lag);
    }
    double best = -1; int bestLag = minLag;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        const double bpm = 60.0 * hopsPerSec / lag;
        const double pref = std::exp(-0.5 * std::pow(std::log2(bpm / 120.0) / 0.9, 2.0));
        // Reforzar con armónicos (2x lag)
        double v = ac[lag];
        if (2 * lag <= maxLag + 1) v += 0.5 * ac[2 * lag];
        v *= pref;
        if (v > best) { best = v; bestLag = lag; }
    }
    double bpm = 60.0 * hopsPerSec / bestLag;

    // 3) Refinamiento fino de tempo + fase con un filtro de peine.
    auto combScore = [&](double periodHops, double phase) {
        double s = 0; int cnt = 0;
        for (double p = phase; p < static_cast<double>(nh - 1); p += periodHops) {
            const int64_t i = static_cast<int64_t>(p);
            const double f = p - i;
            s += onset[i] * (1 - f) + onset[i + 1] * f;
            ++cnt;
        }
        return cnt ? s / cnt : 0.0;
    };
    double bestScore = -1, bestBpm = bpm, bestPhase = 0;
    for (double b = bpm * 0.97; b <= bpm * 1.03; b += 0.02) {
        const double period = lagForBpm(b);
        for (double ph = 0; ph < period; ph += 0.5) {
            const double sc = combScore(period, ph);
            if (sc > bestScore) { bestScore = sc; bestBpm = b; bestPhase = ph; }
        }
    }
    bpm = bestBpm;
    // Llevar a un rango de DJ habitual (78-180)
    while (bpm < 78.0) bpm *= 2.0;
    while (bpm >= 180.0) bpm /= 2.0;
    // Ajuste a enteros si está muy cerca (la mayoría de pistas electrónicas)
    if (std::abs(bpm - std::round(bpm)) < 0.06) bpm = std::round(bpm);

    // Fase final con el BPM definitivo
    const double period = lagForBpm(bpm);
    bestScore = -1;
    for (double ph = 0; ph < period; ph += 0.25) {
        const double sc = combScore(period, ph);
        if (sc > bestScore) { bestScore = sc; bestPhase = ph; }
    }
    info.bpm = bpm;
    // Corrección empírica del retardo de la envolvente de onsets.
    info.firstBeat = std::max(0.0, (bestPhase + 0.2) * hop);
    double mean = std::accumulate(onset.begin(), onset.end(), 0.0) / static_cast<double>(onset.size());
    info.confidence = mean > 0 ? bestScore / mean : 0;
    return info;
}

} // namespace jx
