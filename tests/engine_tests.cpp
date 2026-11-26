// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pruebas del motor: decodificación, BPM/beatgrid, reproducción, key lock, mezclador y sync.
// Genera sus propias pistas sintéticas (no requiere archivos externos).
#include "analysis/Analyzer.h"
#include "analysis/KeyDetector.h"
#include "analysis/PlatformDecoder.h"
#include "app/DeckController.h"
#include "app/Mixer.h"
#include "app/RecordingController.h"
#include "app/SamplerController.h"
#include "control/MidiManager.h"
#include "engine/AudioEngine.h"
#include "library/LibraryDb.h"
#include "library/TagReader.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <cstring>
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
    CHECK(jx::LibraryDb::open(tmp.filePath("biblioteca.sqlite")), "base de datos SQLite creada");

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

    std::printf("\n[6] Etiquetas (ID3, FLAC, WAV)\n");
    {
        // MP3: etiqueta ID3v2.4 (UTF-8) + una trama MPEG-1 Layer III a 320 kbps
        auto frame = [](const char* id, const QByteArray& text) {
            QByteArray body = QByteArray(1, '\x03') + text;
            QByteArray f(id);
            const int n = body.size();
            f.append(char((n >> 21) & 0x7f)); f.append(char((n >> 14) & 0x7f));
            f.append(char((n >> 7) & 0x7f)); f.append(char(n & 0x7f));
            f.append(2, '\0');
            return f + body;
        };
        QByteArray frames = frame("TIT2", "Canción de Prueba") + frame("TPE1", "Jocker Ñandú") +
                            frame("TALB", "Álbum MX") + frame("TCON", "(17)") + frame("TBPM", "126") + frame("TKEY", "8A");
        QByteArray id3("ID3\x04\x00\x00", 6);
        const int n = frames.size();
        id3.append(char((n >> 21) & 0x7f)); id3.append(char((n >> 14) & 0x7f));
        id3.append(char((n >> 7) & 0x7f)); id3.append(char(n & 0x7f));
        QByteArray mp3 = id3 + frames;
        QByteArray audio(400000, '\0');
        audio[0] = char(0xFF); audio[1] = char(0xFB); audio[2] = char(0xE0); audio[3] = char(0x44);   // 320 kbps, 44.1 kHz
        mp3 += audio;
        QFile f(tmp.filePath("prueba.mp3")); f.open(QIODevice::WriteOnly); f.write(mp3); f.close();
        const jx::TrackTags t = jx::readTags(f.fileName());
        CHECK(t.title == QStringLiteral("Canción de Prueba") && t.artist == QStringLiteral("Jocker Ñandú"),
              "ID3v2.4 título/artista con acentos: %s / %s", qPrintable(t.title), qPrintable(t.artist));
        CHECK(t.album == QStringLiteral("Álbum MX") && t.genre == "Rock" && t.bpm == 126 && t.key == "8A",
              "ID3 álbum, género (17 → Rock), BPM y tonalidad");
        CHECK(std::abs(t.duration - 400000 * 8.0 / 320000) < 0.05, "duración MP3 estimada: %.2f s", t.duration);

        // FLAC: STREAMINFO + VORBIS_COMMENT (sin audio)
        QByteArray fl("fLaC");
        QByteArray si(34, '\0');
        const uint32_t srate = 44100; const uint64_t total = 44100ull * 200;
        si[10] = char(srate >> 12); si[11] = char(srate >> 4); si[12] = char(((srate & 0xF) << 4) | (1 << 1));
        si[13] = char(0x10 | ((total >> 32) & 0x0F)); si[14] = char(total >> 24); si[15] = char(total >> 16);
        si[16] = char(total >> 8); si[17] = char(total);
        fl += QByteArray("\x00\x00\x00\x22", 4) + si;
        auto le = [](uint32_t v) { QByteArray b(4, '\0'); for (int i = 0; i < 4; ++i) b[i] = char(v >> (8 * i)); return b; };
        QByteArray vc = le(6) + "jocker";
        const QList<QByteArray> cs = {"TITLE=Sombra", "ARTIST=MX Crew", "GENRE=House", "BPM=124.5"};
        vc += le(static_cast<uint32_t>(cs.size()));
        for (const auto& c : cs) vc += le(static_cast<uint32_t>(c.size())) + c;
        const int vl = vc.size();
        fl += QByteArray(1, char(0x84)) + char(vl >> 16) + char(vl >> 8) + char(vl) + vc;
        QFile ff(tmp.filePath("prueba.flac")); ff.open(QIODevice::WriteOnly); ff.write(fl); ff.close();
        const jx::TrackTags tf = jx::readTags(ff.fileName());
        CHECK(tf.title == "Sombra" && tf.artist == "MX Crew" && tf.genre == "House" && std::abs(tf.bpm - 124.5) < 1e-6,
              "FLAC Vorbis comment: %s / %s / %s / %.1f", qPrintable(tf.title), qPrintable(tf.artist), qPrintable(tf.genre), tf.bpm);
        CHECK(std::abs(tf.duration - 200.0) < 0.01, "duración FLAC (STREAMINFO): %.2f s", tf.duration);

        const jx::TrackTags tw = jx::readTags(tmp.filePath("t124.wav"));
        CHECK(std::abs(tw.duration - 60.0) < 0.01, "duración WAV por encabezado: %.2f s", tw.duration);
    }

    std::printf("\n[7] Biblioteca\n");
    {
        QList<jx::ScanEntry> es;
        for (const QString n : {"prueba.mp3", "prueba.flac", "t124.wav"}) {
            jx::ScanEntry e; e.path = tmp.filePath(n); e.tags = jx::readTags(e.path); e.mtime = 1; es << e;
        }
        jx::LibraryDb::upsertScanned(es);
        const QString pre = tmp.path() + "/";
        auto all = jx::LibraryDb::query(jx::LibraryDb::View::All, pre, "", "artist", true);
        CHECK(all.size() >= 3, "pistas en la biblioteca: %lld", static_cast<long long>(all.size()));
        auto r1 = jx::LibraryDb::query(jx::LibraryDb::View::All, pre, "ñandú", "artist", true);
        CHECK(r1.size() == 1 && r1[0].title.startsWith(QStringLiteral("Canción")), "búsqueda sin acentos ni mayúsculas (ñandú → Ñandú)");
        CHECK(jx::LibraryDb::query(jx::LibraryDb::View::All, pre, "cancion", "artist", true).size() == 1, "«cancion» encuentra «Canción»");
        auto r2 = jx::LibraryDb::query(jx::LibraryDb::View::All, pre, "bpm:124-125", "bpm", true);
        bool hasFlac = false; for (auto& r : r2) hasFlac |= r.path.endsWith(".flac");
        CHECK(hasFlac, "filtro bpm:124-125");
        jx::LibraryDb::addHistory(tmp.filePath("prueba.flac"), 0);
        auto h = jx::LibraryDb::query(jx::LibraryDb::View::History, pre, "", "", true);
        CHECK(h.size() == 3 && h[0].path.endsWith(".flac"), "historial: %lld reproducciones, la más reciente primero", static_cast<long long>(h.size()));
        QVector<double> hot(8, -1.0); hot[2] = 12.3456;
        jx::LibraryDb::saveHotCues(tmp.filePath("prueba.flac"), hot);
        jx::LibraryDb::saveCue(tmp.filePath("prueba.flac"), 1234.0);
        const jx::TrackState st = jx::LibraryDb::state(tmp.filePath("prueba.flac"));
        CHECK(st.found && st.cue == 1234.0 && std::abs(st.hotCues[2] - 12.3456) < 1e-4 && st.hotCues[0] < 0, "cues guardados y restaurados");
        jx::LibraryDb::saveGrid(tmp.filePath("t124.wav"), 62.0, 100.0);
        CHECK(jx::LibraryDb::state(tmp.filePath("t124.wav")).gridLocked, "beatgrid editado queda bloqueado");
        QSet<QString> present = {tmp.filePath("prueba.mp3"), tmp.filePath("prueba.flac")};
        jx::LibraryDb::removeMissing(pre, present);
        CHECK(jx::LibraryDb::query(jx::LibraryDb::View::All, pre, "", "artist", true).size() == 2, "elimina archivos borrados");
    }

    std::printf("\n[8] Controladores MIDI\n");
    {
        jx::Mixer mx(&eng, {&a, &b});
        jx::MidiManager midi({&a, &b}, &mx);
        midi.clearAll();
        midi.learn("A.volume");
        midi.injectMessage(0xB0, 7, 100);           // CC 7, canal 1
        CHECK(midi.learning().isEmpty() && midi.bindings().value("A.volume").toString().startsWith("CC 7"),
              "MIDI learn asigna CC 7 al fader A (%s)", qPrintable(midi.bindings().value("A.volume").toString()));
        midi.injectMessage(0xB0, 7, 0);
        CHECK(a.volume() == 0.0, "CC 7 = 0 baja el fader A");
        midi.injectMessage(0xB0, 7, 127);
        CHECK(a.volume() == 1.0, "CC 7 = 127 sube el fader A");
        midi.learn("B.play");
        midi.injectMessage(0x91, 36, 127);           // nota 36, canal 2
        midi.injectMessage(0x81, 36, 0);
        const bool before = b.playing();
        midi.injectMessage(0x91, 36, 127);
        CHECK(b.playing() != before, "nota 36 (canal 2) alterna play en B");
        midi.learn("xfader");
        midi.injectMessage(0xB0, 8, 30);            // este mensaje sólo asigna
        midi.injectMessage(0xB0, 8, 0);
        CHECK(mx.crossfader() == -1.0, "crossfader por MIDI a la izquierda");
        midi.injectMessage(0xB0, 8, 64);
        CHECK(std::abs(mx.crossfader()) < 0.02, "crossfader centrado");
        midi.learn("A.eqlow");
        midi.injectMessage(0xB0, 9, 30);
        midi.injectMessage(0xB0, 9, 0);
        CHECK(a.eqLow() == -1.0, "EQ graves A al mínimo (kill)");
        midi.injectMessage(0xB0, 9, 64);
        CHECK(a.eqLow() == 0.0, "EQ graves A al centro");
        CHECK(midi.saveMapping(tmp.filePath("mapeo.json")), "mapeo guardado en JSON");
        midi.clearAll();
        CHECK(midi.loadMapping(tmp.filePath("mapeo.json")) && midi.bindings().size() == 4, "mapeo recargado (%lld acciones)", static_cast<long long>(midi.bindings().size()));
    }

    std::printf("\n[9] Tonalidad\n");
    {
        // Progresiones con acordes de 3 notas + bajo, 2 s por acorde
        auto synth = [&](const std::vector<std::vector<int>>& chords, double secs) {
            const int n = static_cast<int>(secs * sr);
            std::vector<float> pcm(static_cast<size_t>(n) * 2, 0.f);
            const int per = static_cast<int>(2.0 * sr);
            for (int i = 0; i < n; ++i) {
                const auto& ch = chords[(i / per) % chords.size()];
                double v = 0;
                for (size_t k = 0; k < ch.size(); ++k) {
                    const double f = 440.0 * std::pow(2.0, (ch[k] - 69) / 12.0);
                    double tone = std::sin(2 * M_PI * f * i / sr) + 0.3 * std::sin(4 * M_PI * f * i / sr);
                    v += tone * (k == 0 ? 0.5 : 0.25);
                }
                pcm[2 * i] = pcm[2 * i + 1] = static_cast<float>(v * 0.3);
            }
            return pcm;
        };
        struct KCase { const char* name; const char* expect; std::vector<std::vector<int>> chords; };
        const std::vector<KCase> kc = {
            {"La menor (Am-F-C-G)", "8A", {{45, 69, 72, 76}, {41, 65, 69, 72}, {48, 67, 72, 76}, {43, 67, 71, 74}, {45, 69, 72, 76}}},
            {"Do mayor (C-F-G-C)", "8B", {{48, 60, 64, 67}, {41, 65, 69, 72}, {43, 67, 71, 74}, {48, 64, 67, 72}}},
            {"Mi menor (Em-C-D-Em)", "9A", {{40, 64, 67, 71}, {48, 64, 67, 72}, {50, 66, 69, 74}, {40, 64, 67, 71}, {47, 66, 71, 74}}},
            {"Sol mayor (G-C-D-G)", "9B", {{43, 67, 71, 74}, {48, 64, 67, 72}, {50, 66, 69, 74}, {43, 67, 71, 74}}},
            {"Re menor (Dm-Bb-C-A-Dm)", "7A", {{50, 62, 65, 69}, {46, 62, 65, 70}, {48, 64, 67, 72}, {45, 61, 64, 69}, {50, 62, 65, 69}}},
            {"Mi mayor (E-A-B-E)", "12B", {{40, 64, 68, 71}, {45, 64, 69, 73}, {47, 63, 66, 71}, {40, 64, 68, 71}}},
            {"Fa♯ menor (F#m-D-E-F#m)", "11A", {{42, 66, 69, 73}, {50, 66, 69, 74}, {52, 64, 68, 71}, {42, 66, 69, 73}}},
        };
        for (const KCase& c : kc) {
            const auto pcm = synth(c.chords, 40);
            const jx::KeyResult k = jx::detectKey(pcm.data(), static_cast<int64_t>(pcm.size() / 2), sr);
            CHECK(k.camelot() == c.expect, "%s → %s (%s, confianza %.2f)", c.name, qPrintable(k.camelot()), qPrintable(k.name()), k.confidence);
        }
        CHECK(jx::normalizeKeyToCamelot("Am") == "8A" && jx::normalizeKeyToCamelot("F#m") == "11A" &&
              jx::normalizeKeyToCamelot("Bb") == "6B" && jx::normalizeKeyToCamelot("1m") == "8A" &&
              jx::normalizeKeyToCamelot("C major") == "8B", "conversión de notaciones (Am, F#m, Bb, Open Key) a Camelot");
        CHECK(jx::keysCompatible("8A", "9A") && jx::keysCompatible("8A", "8B") && jx::keysCompatible("12B", "1B") &&
              !jx::keysCompatible("8A", "10A") && !jx::keysCompatible("8A", "9B"), "compatibilidad armónica en la rueda de Camelot");
        CHECK(jx::keyFromCamelot("8A").openKey() == "1m" && jx::keyFromCamelot("8B").openKey() == "1d", "notación Open Key");
    }

    std::printf("\n[10] Efectos\n");
    {
        b.stop();
        a.setVolume(1); a.setEqLow(0); a.setEqMid(0); a.setEqHigh(0); a.setFilter(0); a.setGain(0);
        eng.crossfader.store(0.f);
        if (!a.playing()) a.playPause();
        renderSecs(0.5);
        std::vector<float> dry; renderSecs(1.0, &dry);
        const double dryRms = rms(dry);
        for (int t = 1; t < jx::EffectUnit::Count; ++t) {
            a.setFxType(t); a.setFxWet(0.8); a.setFxBeats(t == jx::EffectUnit::Gate ? 0.5 : 1.0);
            renderSecs(0.3);
            std::vector<float> wet; renderSecs(1.0, &wet);
            bool finite = true; float mx = 0;
            for (float v : wet) { finite &= std::isfinite(v); mx = std::max(mx, std::abs(v)); }
            double diff = 0; const size_t m = std::min(wet.size(), dry.size());
            for (size_t i = 0; i < m; ++i) diff += std::abs(wet[i] - dry[i]);
            CHECK(finite && mx <= 0.98f && rms(wet) > 0.01 && diff / m > 0.002,
                  "%s: activo, estable (pico %.2f, RMS %.3f vs %.3f seco)", jx::EffectUnit::name(t), mx, rms(wet), dryRms);
        }
        // El eco deja cola al pausar el deck
        a.setFxType(jx::EffectUnit::Echo); a.setFxWet(0.8); a.setFxBeats(0.5);
        renderSecs(1.0);
        a.playPause();   // pausa
        renderSecs(0.05);
        std::vector<float> tail; renderSecs(0.4, &tail);
        CHECK(rms(tail) > 0.005, "eco: la cola sigue sonando tras pausar (RMS %.3f)", rms(tail));
        a.setFxType(0);
        renderSecs(0.5);
        std::vector<float> off; renderSecs(0.3, &off);
        CHECK(rms(off) < 0.001, "efecto apagado y deck en pausa: silencio");
    }

    std::printf("\n[11] Sampler\n");
    {
        if (a.playing()) a.playPause();
        if (b.playing()) b.playPause();
        a.setFxType(0); b.setFxType(0);
        renderSecs(0.5);
        jx::SamplerController smp(&eng);
        for (int i = 0; i < 8; ++i) { smp.resetSlot(i); smp.setMode(i, 0); }
        CHECK(smp.names().size() == 8 && smp.names()[0] == "BOCINA" && smp.names()[3] == "BOMBO", "8 sonidos incluidos (%s…)", qPrintable(smp.names().mid(0, 4).join(", ")));
        std::vector<float> c1; renderSecs(0.3, &c1);
        CHECK(rms(c1) < 0.001, "sampler en silencio sin disparar");
        smp.press(3);
        std::vector<float> c2; renderSecs(0.3, &c2);
        CHECK(rms(c2) > 0.05, "pad 4 (BOMBO) suena al dispararlo (RMS %.3f)", rms(c2));
        renderSecs(1.0);
        std::vector<float> c3; renderSecs(0.2, &c3);
        CHECK(rms(c3) < 0.001, "modo ONE: termina solo");
        smp.setMode(4, jx::Sampler::Loop);
        smp.press(4);
        renderSecs(2.0);                                  // más largo que la muestra (0.5 s)
        std::vector<float> c4; renderSecs(0.3, &c4);
        CHECK(rms(c4) > 0.01, "modo LOOP: sigue sonando tras el final de la muestra");
        smp.press(4);
        renderSecs(0.1);
        std::vector<float> c5; renderSecs(0.2, &c5);
        CHECK(rms(c5) < 0.001, "modo LOOP: otro toque lo detiene");
        smp.setMode(1, jx::Sampler::Hold);
        smp.press(1);
        renderSecs(0.3);
        smp.release(1);
        renderSecs(0.05);
        std::vector<float> c6; renderSecs(0.2, &c6);
        CHECK(rms(c6) < 0.001, "modo HOLD: se detiene al soltar");
        CHECK(smp.loadFile(2, tmp.filePath("t124.wav")) && smp.names()[2] == "T124", "cargar sonido propio en un pad (%s)", qPrintable(smp.names()[2]));
        smp.resetSlot(2);
    }

    std::printf("\n[12] Grabación\n");
    {
        jx::RecordingController rec(&eng, {&a, &b});
        rec.setFolder(tmp.filePath("grabaciones"));
        a.playPause();                                   // ya sonando: entra al inicio de la lista
        CHECK(rec.start() && rec.recording(), "grabación iniciada");
        renderSecs(3.0);
        QElapsedTimer w; w.start();
        while (rec.seconds() < 2.9 && w.elapsed() < 3000) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        rec.stop();
        a.playPause();
        QFile f(rec.lastFile());
        const bool opened = f.open(QIODevice::ReadOnly);
        const QByteArray hdr = f.read(44);
        const qint64 size = f.size();
        auto le32 = [&](int off) { return static_cast<uint32_t>(static_cast<uchar>(hdr[off]) | (static_cast<uchar>(hdr[off + 1]) << 8) |
                                                              (static_cast<uchar>(hdr[off + 2]) << 16) | (static_cast<uchar>(hdr[off + 3]) << 24)); };
        const double secs = opened ? le32(40) / (sr * 6.0) : 0;
        CHECK(opened && hdr.startsWith("RIFF") && le32(40) + 44 == size, "WAV válido de 24 bits (%lld bytes)", static_cast<long long>(size));
        CHECK(std::abs(secs - 3.0) < 0.1, "duración grabada %.2f s (3 s mezclados)", secs);
        f.seek(44 + static_cast<qint64>(sr * 6));
        const QByteArray pcm = f.read(6000);
        int nonzero = 0; for (char c : pcm) nonzero += c != 0;
        CHECK(nonzero > 1000, "la grabación contiene audio");
        const QString base = rec.lastFile().left(rec.lastFile().size() - 4);
        QFile txt(base + ".txt"); txt.open(QIODevice::ReadOnly);
        const QString list = QString::fromUtf8(txt.readAll());
        QFile cue(base + ".cue"); cue.open(QIODevice::ReadOnly);
        const QString cueText = QString::fromUtf8(cue.readAll());
        CHECK(list.contains("t124") && cueText.contains("TRACK 01") && cueText.contains("INDEX 01 00:00"),
              "lista de canciones (.txt) y hoja .cue generadas");
    }

    std::printf("\n[13] AAC / M4A (%s)\n", jx::platformDecoderName());
    {
        const QString m4a = QStringLiteral(JX_TEST_DATA_DIR) + "/prueba-aac.m4a";
        const jx::TrackTags tg = jx::readTags(m4a);
        CHECK(tg.title == "Tono de Prueba" && tg.artist == "Jocker-MX" && tg.album == "Pruebas" && tg.genre == "House",
              "etiquetas MP4: %s / %s / %s / %s", qPrintable(tg.title), qPrintable(tg.artist), qPrintable(tg.album), qPrintable(tg.genre));
        CHECK(std::abs(tg.duration - 6.0) < 0.1, "duración MP4 (mvhd): %.2f s", tg.duration);
        if (std::strlen(jx::platformDecoderName()) > 0) {
            QString err;
            auto t = jx::loadAndAnalyze(m4a, sr, &err);
            CHECK(t != nullptr, "decodifica AAC con %s (%s)", jx::platformDecoderName(), qPrintable(err));
            if (t) {
                CHECK(std::abs(t->durationSec() - 6.0) < 0.15, "duración decodificada %.2f s", t->durationSec());
                CHECK(std::abs(t->bpm - 124.0) < 0.5, "BPM de la pista AAC: %.2f", t->bpm);
                std::vector<float> v(t->pcm.begin(), t->pcm.end());
                CHECK(rms(v) > 0.05, "audio AAC con señal (RMS %.3f)", rms(v));
            }
        } else {
            std::printf("  (sin decodificador del sistema en esta compilación)\n");
        }
        std::vector<float> mono = {0.5f, -0.5f, 0.25f, 0.f};
        std::vector<float> st;
        CHECK(jx::convertToStereo(mono.data(), 4, 1, 48000, 48000, st) && st.size() == 8 && st[0] == st[1],
              "conversión mono → estéreo");
    }

    const QByteArray extra = qgetenv("JX_EXTRA_AUDIO");
    if (!extra.isEmpty()) {
        std::printf("\n[10] Archivos externos (%s)\n", extra.constData());
        for (const QFileInfo& fi : QDir(QString::fromLocal8Bit(extra)).entryInfoList(QDir::Files)) {
            QString err;
            auto t = jx::loadAndAnalyze(fi.filePath(), sr, &err);
            const jx::TrackTags tg = jx::readTags(fi.filePath());
            CHECK(t != nullptr, "%s → %.2f BPM, %s, %.1f s, etiqueta «%s»", qPrintable(fi.fileName()), t ? t->bpm : 0.0,
                  t ? qPrintable(t->musicalKey) : "-", tg.duration, qPrintable(tg.title));
        }
    }

    std::printf("\nResultado: %d correctas, %d fallidas\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
