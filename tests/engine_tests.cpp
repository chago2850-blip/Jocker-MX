// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pruebas del motor: decodificación, BPM/beatgrid, reproducción, key lock, mezclador y sync.
// Genera sus propias pistas sintéticas (no requiere archivos externos).
#include "analysis/Analyzer.h"
#include "app/DeckController.h"
#include "engine/AudioEngine.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, msg, ...)                                                                \
    do {                                                                                     \
        if (cond) { ++g_pass; std::printf("  ✔ " msg "\n", ##__VA_ARGS__); }                 \
        else { ++g_fail; std::printf("  ✘ FALLO: " msg "\n", ##__VA_ARGS__); }               \
    } while (0)

// Pista sintética: bombo en cada beat, hi-hat a contratiempo y bajo.
static void writeTestWav(const QString& path, double bpm, double firstBeat, double secs, int sr = 44100) {
    const int n = static_cast<int>(secs * sr);
    std::vector<float> l(n, 0.f), r(n, 0.f);
    std::mt19937 rng(42);
    std::normal_distribution<float> noise(0.f, 1.f);
    const double beat = 60.0 / bpm;
    for (int k = 0;; ++k) {
        const double tb = firstBeat + k * beat;
        if (tb >= secs) break;
        const int s = static_cast<int>(tb * sr);
        double ph = 0;
        for (int i = 0; i < sr / 4 && s + i < n; ++i) {
            const double t = static_cast<double>(i) / sr;
            ph += 2 * M_PI * (50 + 120 * std::exp(-t * 25)) / sr;
            const float v = static_cast<float>(std::sin(ph) * std::exp(-t * 9) * 0.8);
            l[s + i] += v; r[s + i] += v;
        }
        const int h = static_cast<int>((tb + beat / 2) * sr);
        float prev = 0;
        for (int i = 0; i < sr / 20 && h + i < n; ++i) {
            const float x = noise(rng) * 0.15f * static_cast<float>(std::exp(-i / (double)sr * 80));
            l[h + i] += x - prev; r[h + i] += (x - prev) * 0.8f; prev = x;
        }
        for (int i = 0; i < static_cast<int>(beat / 2 * sr * 0.9) && h + i < n; ++i) {
            const double t = static_cast<double>(i) / sr;
            const float v = static_cast<float>((std::sin(2 * M_PI * 55 * t) > 0 ? 0.1 : -0.1) * std::exp(-t * 4));
            l[h + i] += v; r[h + i] += v;
        }
    }
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    auto w32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto w16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    f.write("RIFF"); w32(36 + n * 4); f.write("WAVEfmt "); w32(16); w16(1); w16(2); w32(sr); w32(sr * 4); w16(4); w16(16);
    f.write("data"); w32(n * 4);
    for (int i = 0; i < n; ++i) {
        w16(static_cast<uint16_t>(static_cast<int16_t>(std::clamp(l[i], -1.f, 1.f) * 32000)));
        w16(static_cast<uint16_t>(static_cast<int16_t>(std::clamp(r[i], -1.f, 1.f) * 32000)));
    }
}

static double rms(const std::vector<float>& v) {
    double s = 0;
    for (float x : v) s += x * x;
    return std::sqrt(s / std::max<size_t>(1, v.size()));
}

static bool waitLoaded(jx::DeckController& d, int ms = 20000) {
    QElapsedTimer t; t.start();
    while ((d.loading() || !d.loaded()) && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return d.loaded();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Jocker-MX-tests");
    QCoreApplication::setApplicationName("jx-tests");
    QTemporaryDir tmp;
    const double sr = 48000.0;

    std::printf("\n[1] Análisis de BPM y beatgrid\n");
    struct Case { double bpm, first; };
    const Case cases[] = {{124, 0.37}, {128, 0.10}, {174, 0.52}, {95, 0.25}, {140, 0.0}};
    for (const Case& c : cases) {
        const QString p = tmp.filePath(QStringLiteral("t%1.wav").arg(c.bpm));
        writeTestWav(p, c.bpm, c.first, 60);
        QString err;
        auto t = jx::loadAndAnalyze(p, sr, &err);
        CHECK(t != nullptr, "decodifica WAV (%s)", qPrintable(err));
        if (!t) continue;
        CHECK(std::abs(t->bpm - c.bpm) < 0.1, "BPM %.2f detectado como %.2f", c.bpm, t->bpm);
        const double beatLen = 60.0 / c.bpm;
        double d = std::fmod(t->firstBeat / sr - c.first, beatLen);
        if (d > beatLen / 2) d -= beatLen;
        if (d < -beatLen / 2) d += beatLen;
        CHECK(std::abs(d) < 0.015, "fase del beatgrid: error %.1f ms", d * 1000);
        CHECK(std::abs(t->durationSec() - 60.0) < 0.05, "duración %.2f s tras remuestrear a 48 kHz", t->durationSec());
        CHECK(!t->wfLow.empty() && t->wfLow.size() == t->wfMid.size(), "forma de onda (%zu bins)", t->wfLow.size());
    }

    std::printf("\n[2] Reproducción, pitch y key lock\n");
    jx::AudioEngine eng(2);
    eng.prepareOffline(sr);
    jx::DeckController a(&eng, 0), b(&eng, 1);
    a.setPartner(&b); b.setPartner(&a);
    a.load(tmp.filePath("t124.wav"));
    b.load(tmp.filePath("t128.wav"));
    CHECK(waitLoaded(a) && waitLoaded(b), "carga asíncrona de los dos decks");

    const int block = 512;
    std::vector<float> out(block * 2);
    auto renderSecs = [&](double secs, std::vector<float>* capture = nullptr) {
        const int blocks = static_cast<int>(secs * sr / block);
        for (int i = 0; i < blocks; ++i) {
            eng.render(out.data(), block);
            if (capture) capture->insert(capture->end(), out.begin(), out.end());
        }
    };
    renderSecs(0.05);  // aplica el seek inicial al cue
    const double start = a.positionFrames();
    a.playPause();
    std::vector<float> cap;
    renderSecs(2.0, &cap);
    const double adv = (a.positionFrames() - start) / sr;
    CHECK(std::abs(adv - 2.0) < 0.05, "avance a tempo normal: %.3f s en 2 s", adv);
    CHECK(rms(cap) > 0.02, "salida con audio (RMS %.3f)", rms(cap));

    a.setPitchRange(0.08);
    a.setPitch(1.0);  // +8 %
    double p0 = a.positionFrames();
    renderSecs(2.0);
    double rate = (a.positionFrames() - p0) / sr / 2.0;
    CHECK(std::abs(rate - 1.08) < 0.01, "pitch +8 %% sin key lock → velocidad %.3f", rate);

    a.setKeyLock(true);
    renderSecs(0.3);
    p0 = a.positionFrames();
    cap.clear();
    renderSecs(2.0, &cap);
    rate = (a.positionFrames() - p0) / sr / 2.0;
    CHECK(std::abs(rate - 1.08) < 0.02, "pitch +8 %% con key lock (Rubber Band) → velocidad %.3f", rate);
    CHECK(rms(cap) > 0.02, "key lock produce audio (RMS %.3f)", rms(cap));
    a.setKeyLock(false);
    a.setPitch(0);

    std::printf("\n[3] Mezclador\n");
    a.setEqLow(-1.0); a.setEqMid(-1.0); a.setEqHigh(-1.0);
    renderSecs(0.2);
    cap.clear(); renderSecs(1.0, &cap);
    CHECK(rms(cap) < 0.01, "kill de las 3 bandas silencia el canal (RMS %.4f)", rms(cap));
    a.setEqLow(0); a.setEqMid(0); a.setEqHigh(0);
    a.setVolume(0);
    renderSecs(0.1);
    cap.clear(); renderSecs(0.5, &cap);
    CHECK(rms(cap) < 0.001, "fader en 0 silencia el canal");
    a.setVolume(1);
    eng.crossfader.store(1.f);
    renderSecs(0.1);
    cap.clear(); renderSecs(0.5, &cap);
    CHECK(rms(cap) < 0.001, "crossfader a B silencia el deck A");
    eng.crossfader.store(0.f);
    a.setGain(1.0);
    eng.masterVolume.store(1.f);
    cap.clear(); renderSecs(1.0, &cap);
    float mx = 0; for (float v : cap) mx = std::max(mx, std::abs(v));
    CHECK(mx <= 0.98f, "limitador maestro: pico %.3f ≤ 0.98", mx);
    a.setGain(0);
    eng.masterVolume.store(0.85f);

    std::printf("\n[4] Sync\n");
    b.playPause();
    renderSecs(1.0);
    a.sync();
    renderSecs(0.5);
    CHECK(std::abs(a.bpm() - b.bpm()) < 0.01, "tempo igualado: A %.2f / B %.2f BPM", a.bpm(), b.bpm());
    double dphase = a.beatPhase() - b.beatPhase();
    if (dphase > 0.5) dphase -= 1; if (dphase < -0.5) dphase += 1;
    CHECK(std::abs(dphase) < 0.03, "fases alineadas (diferencia %.3f beat)", dphase);
    renderSecs(10.0);
    dphase = a.beatPhase() - b.beatPhase();
    if (dphase > 0.5) dphase -= 1; if (dphase < -0.5) dphase += 1;
    CHECK(std::abs(dphase) < 0.03, "siguen en fase tras 10 s (diferencia %.3f beat)", dphase);

    std::printf("\n[5] Loops y cues\n");
    a.autoLoop(4);
    const double ls = a.loopStart(), le = a.loopEnd();
    CHECK(std::abs((le - ls) - 4 * 60.0 / 124) < 0.002, "loop de 4 beats = %.3f s", le - ls);
    renderSecs(8.0);
    CHECK(a.position() >= ls - 0.01 && a.position() <= le + 0.01, "el cabezal permanece dentro del loop (%.2f s)", a.position());
    a.loopExit();
    a.hotCue(0);
    const double hc = a.hotCues()[0].toDouble();
    CHECK(hc >= 0, "hot cue 1 fijado en %.2f s", hc);
    a.clearHotCue(0);

    std::printf("\nResultado: %d correctas, %d fallidas\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
