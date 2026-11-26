// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "DeckController.h"

#include "analysis/Analyzer.h"
#include "analysis/KeyDetector.h"
#include "library/LibraryDb.h"

#include <QFutureWatcher>
#include <QSettings>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

namespace jx {

DeckController::DeckController(AudioEngine* engine, int index, QObject* parent)
    : QObject(parent), m_engine(engine), m_index(index), m_deck(engine->deck(index)),
      m_ch(engine->channel(index)), m_fx(engine->effects(index)) {
    for (double& h : m_hot) h = -1;
    setVolume(m_volume);
}

QString DeckController::title() const { return m_track ? m_track->title : QString(); }
QString DeckController::artist() const { return m_track ? m_track->artist : QString(); }
QString DeckController::musicalKey() const { return m_track ? m_track->musicalKey : QString(); }
QString DeckController::keyName() const { return m_track ? keyFromCamelot(m_track->musicalKey).name() : QString(); }
bool DeckController::keyCompatibleWith(const QString& camelot) const {
    return m_track && keysCompatible(m_track->musicalKey, normalizeKeyToCamelot(camelot));
}
QString DeckController::filePath() const { return m_track ? m_track->path : QString(); }
double DeckController::duration() const { return m_track ? m_track->durationSec() : 0.0; }
double DeckController::trackBpm() const { return m_track ? m_track->bpm : 0.0; }
double DeckController::bpm() const { return m_track ? m_track->bpm * m_deck.baseRate() : 0.0; }

double DeckController::position() const {
    return m_track ? m_deck.position.load() / m_track->sampleRate : 0.0;
}
double DeckController::progress() const {
    return (m_track && m_track->frames > 0) ? m_deck.position.load() / static_cast<double>(m_track->frames) : 0.0;
}
double DeckController::beatPhase() const {
    if (!m_track || m_track->bpm <= 0) return 0.0;
    const double bl = m_track->beatLength();
    double ph = std::fmod((m_deck.position.load() - m_track->firstBeat) / bl, 1.0);
    if (ph < 0) ph += 1.0;
    return ph;
}

int DeckController::beatIndex() const {
    if (!m_track || m_track->bpm <= 0) return 0;
    return static_cast<int>(std::floor((m_deck.position.load() - m_track->firstBeat) / m_track->beatLength()));
}

QVariantList DeckController::hotCues() const {
    QVariantList l;
    const double sr = m_track ? m_track->sampleRate : 48000.0;
    for (double h : m_hot) l << (h < 0 ? -1.0 : h / sr);
    return l;
}

double DeckController::loopStart() const { return m_track ? m_deck.loopStart.load() / m_track->sampleRate : 0; }
double DeckController::loopEnd() const { return m_track ? m_deck.loopEnd.load() / m_track->sampleRate : 0; }

// ------------------------------------------------------------------ carga
void DeckController::load(const QString& pathOrUrl) {
    QString path = pathOrUrl;
    if (path.startsWith(QLatin1String("file:"))) path = QUrl(pathOrUrl).toLocalFile();
    if (path.isEmpty()) return;

    const int serial = ++m_loadSerial;
    m_loading = true;
    emit loadingChanged();
    if (!m_error.isEmpty()) { m_error.clear(); emit errorChanged(); }

    const double sr = m_engine->sampleRate();
    auto* watcher = new QFutureWatcher<std::shared_ptr<Track>>(this);
    auto errPtr = std::make_shared<QString>();
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, serial, errPtr]() {
        watcher->deleteLater();
        if (serial != m_loadSerial) return;
        m_loading = false;
        emit loadingChanged();
        std::shared_ptr<Track> t = watcher->result();
        if (!t) {
            m_error = *errPtr;
            emit errorChanged();
            return;
        }
        // Mantener viva la pista anterior (y sus stems) un momento: el hilo de audio podría estar leyéndola.
        std::shared_ptr<Track> old = m_track;
        std::shared_ptr<StemData> oldStems = m_stems;
        if (m_stemJob) m_stemJob->cancel = true;
        m_stemJob.reset();
        m_stems.reset();
        m_stemsState = 0;
        m_stemsProgress = 0;
        m_stemsError.clear();
        m_track = t;
        m_deck.setTrack(m_track.get());
        if (old || oldStems) QTimer::singleShot(2000, this, [old, oldStems]() {});

        // Restaurar estado guardado en la biblioteca (beatgrid editado, cues, hot cues)
        // (la biblioteca guarda posiciones en segundos; el motor trabaja en cuadros)
        const double sr = m_track->sampleRate;
        const TrackState st = LibraryDb::state(m_track->path);
        if (st.gridLocked && st.bpm > 0) {
            m_track->bpm = st.bpm;
            m_track->firstBeat = std::max(0.0, st.firstBeat) * sr;
        } else {
            TrackTags tg;
            tg.title = m_track->title;
            tg.artist = m_track->artist;
            LibraryDb::saveAnalysis(m_track->path, m_track->bpm, m_track->firstBeat / sr, m_track->durationSec(), tg,
                                    m_track->musicalKey);
        }
        m_cue = st.cue >= 0 ? st.cue * sr : m_track->firstBeat;
        for (int i = 0; i < 8; ++i) {
            const double h = st.hotCues.value(i, -1.0);
            m_hot[i] = h >= 0 ? h * sr : -1.0;
        }
        m_historyLogged = false;
        m_deck.requestSeek(m_cue);
        m_loopBeats = 4;
        emit trackChanged();
        emit cuesChanged();
        emit loopChanged();
        emit tempoChanged();
        emit trackLoaded(m_track->path, m_track->bpm, m_track->musicalKey);
        emit stemsChanged();
        if (stemsAvailable() && QSettings().value("stems/auto", true).toBool()) prepareStems();
    });
    watcher->setFuture(QtConcurrent::run([path, sr, errPtr]() -> std::shared_ptr<Track> {
        QString err;
        auto t = loadAndAnalyze(path, sr, &err);
        if (!t) { *errPtr = err; return nullptr; }
        return std::shared_ptr<Track>(t.release());
    }));
}

void DeckController::eject() {
    if (playing()) return;
    std::shared_ptr<Track> old = m_track;
    std::shared_ptr<StemData> oldStems = m_stems;
    if (m_stemJob) m_stemJob->cancel = true;
    m_stemJob.reset();
    m_stems.reset();
    m_stemsState = 0;
    m_deck.setTrack(nullptr);
    m_track.reset();
    if (old || oldStems) QTimer::singleShot(2000, this, [old, oldStems]() {});
    emit stemsChanged();
    emit trackChanged();
}

// ------------------------------------------------------------------ transporte
void DeckController::playPause() {
    if (!m_track) return;
    m_cuePreview = false;
    if (m_deck.reachedEnd.load()) { m_deck.reachedEnd.store(false); m_deck.requestSeek(m_cue); }
    m_deck.playing.store(!m_deck.playing.load());
    if (m_deck.playing.load() && !m_historyLogged) {
        m_historyLogged = true;
        LibraryDb::addHistory(m_track->path, m_index);
        emit played(m_track->path);
    }
    emit playingChanged();
}

void DeckController::saveHotCues() {
    if (!m_track) return;
    QVector<double> h(8);
    for (int i = 0; i < 8; ++i) h[i] = m_hot[i] >= 0 ? m_hot[i] / m_track->sampleRate : -1.0;
    LibraryDb::saveHotCues(m_track->path, h);
}

// ------------------------------------------------------------------ beatgrid
void DeckController::setGrid(double bpm, double firstBeat) {
    if (!m_track || bpm <= 0) return;
    // La pista es inmutable para el hilo de audio salvo bpm/firstBeat, que sólo lee la UI.
    m_track->bpm = bpm;
    m_track->firstBeat = std::max(0.0, firstBeat);
    LibraryDb::saveGrid(m_track->path, m_track->bpm, m_track->firstBeat / m_track->sampleRate);
    emit tempoChanged();
    emit trackChanged();
    emit trackLoaded(m_track->path, m_track->bpm, m_track->musicalKey);
}
void DeckController::bpmDouble() { if (m_track && m_track->bpm > 0) setGrid(m_track->bpm * 2, m_track->firstBeat); }
void DeckController::bpmHalve() { if (m_track && m_track->bpm > 0) setGrid(m_track->bpm / 2, m_track->firstBeat); }
void DeckController::gridShift(double ms) {
    if (!m_track || m_track->bpm <= 0) return;
    double fb = m_track->firstBeat + ms / 1000.0 * m_track->sampleRate;
    const double bl = m_track->beatLength();
    while (fb < 0) fb += bl;
    setGrid(m_track->bpm, fb);
}
void DeckController::gridSetBeatHere() {
    if (!m_track || m_track->bpm <= 0) return;
    const double bl = m_track->beatLength();
    setGrid(m_track->bpm, std::fmod(m_deck.position.load(), bl));
}
void DeckController::tapBpm() {
    const qint64 t = m_tapTimer.isValid() ? m_tapTimer.restart() : (m_tapTimer.start(), 0);
    if (t <= 0 || t > 2000) { m_taps.clear(); return; }
    m_taps.append(t);
    if (m_taps.size() > 8) m_taps.removeFirst();
    if (m_taps.size() >= 3 && m_track) {
        double avg = 0;
        for (qint64 v : m_taps) avg += static_cast<double>(v);
        avg /= m_taps.size();
        const double bpmNative = 60000.0 / avg / m_deck.baseRate();
        setGrid(std::round(bpmNative * 100) / 100, m_track->firstBeat);
    }
}

void DeckController::stop() {
    m_deck.playing.store(false);
    m_deck.requestSeek(m_cue);
    emit playingChanged();
}

double DeckController::quantized(double frame) const {
    if (!m_quantize || !m_track || m_track->bpm <= 0) return frame;
    const double bl = m_track->beatLength();
    const double n = std::round((frame - m_track->firstBeat) / bl);
    return std::max(0.0, m_track->firstBeat + n * bl);
}

void DeckController::cueDown() {
    if (!m_track) return;
    m_cueHeld = true;
    if (m_deck.playing.load()) {
        m_deck.playing.store(false);
        m_deck.requestSeek(m_cue);
    } else {
        const double pos = m_deck.position.load();
        if (std::abs(pos - m_cue) < m_track->sampleRate * 0.02) {
            m_cuePreview = true;                       // vista previa mientras se mantiene
            m_deck.playing.store(true);
        } else {
            m_cue = quantized(pos);
            m_deck.requestSeek(m_cue);
            LibraryDb::saveCue(m_track->path, m_cue / m_track->sampleRate);
            emit cuesChanged();
        }
    }
    emit playingChanged();
}

void DeckController::cueUp() {
    m_cueHeld = false;
    if (m_cuePreview) {
        m_cuePreview = false;
        m_deck.playing.store(false);
        m_deck.requestSeek(m_cue);
        emit playingChanged();
    }
}

void DeckController::seekFraction(double f) {
    if (!m_track) return;
    m_deck.reachedEnd.store(false);
    m_deck.requestSeek(std::clamp(f, 0.0, 1.0) * static_cast<double>(m_track->frames));
}

void DeckController::hotCue(int i) {
    if (!m_track || i < 0 || i >= 8) return;
    if (m_hot[i] < 0) {
        m_hot[i] = quantized(m_deck.position.load());
        saveHotCues();
        emit cuesChanged();
    } else {
        m_deck.reachedEnd.store(false);
        m_deck.requestSeek(m_hot[i]);
        if (!m_deck.playing.load()) {
            m_deck.playing.store(true);
            emit playingChanged();
        }
    }
}

void DeckController::clearHotCue(int i) {
    if (!m_track || i < 0 || i >= 8) return;
    m_hot[i] = -1;
    saveHotCues();
    emit cuesChanged();
}

// ------------------------------------------------------------------ tempo y sync
void DeckController::applyPitch() {
    m_deck.pitch.store(m_pitch * m_range);
    emit tempoChanged();
}

void DeckController::setPitch(double p) {
    p = std::clamp(p, -1.0, 1.0);
    if (p == m_pitch) return;
    m_pitch = p;
    applyPitch();
}

void DeckController::setPitchRange(double r) {
    if (r <= 0 || r == m_range) return;
    const double abs = m_pitch * m_range;          // conservar el tempo real
    m_range = r;
    m_pitch = std::clamp(abs / m_range, -1.0, 1.0);
    applyPitch();
}

void DeckController::setKeyLock(bool on) {
    if (on == m_deck.keyLock.load()) return;
    m_deck.keyLock.store(on);
    emit keyLockChanged();
}

void DeckController::setSyncLock(bool on) {
    if (on == m_syncLock) return;
    m_syncLock = on;
    if (on) sync();
    emit syncLockChanged();
}

void DeckController::sync() {
    if (!m_track || !m_partner || !m_partner->m_track || m_track->bpm <= 0 || m_partner->m_track->bpm <= 0)
        return;
    const double target = m_partner->bpm();
    double ratio = target / m_track->bpm;
    while (ratio > 1.5) ratio /= 2.0;     // medio / doble tiempo
    while (ratio < 0.67) ratio *= 2.0;
    const double absPitch = ratio - 1.0;
    static const double ranges[] = {0.08, 0.16, 0.25, 0.5, 1.0};
    if (std::abs(absPitch) > m_range) {
        for (double r : ranges) if (r >= std::abs(absPitch)) { m_range = r; break; }
    }
    m_pitch = std::clamp(absPitch / m_range, -1.0, 1.0);
    applyPitch();

    // Alinear fase si el otro deck suena
    if (m_partner->playing()) {
        double d = m_partner->beatPhase() - beatPhase();
        if (d > 0.5) d -= 1.0;
        if (d < -0.5) d += 1.0;
        const double newPos = m_deck.position.load() + d * m_track->beatLength();
        if (newPos >= 0) m_deck.requestSeek(newPos);
    }
}

void DeckController::nudge(double amount) { m_deck.nudge.store(amount); }

void DeckController::scratchBegin() {
    m_deck.scratchRate.store(0.0);
    m_deck.scratchActive.store(true);
}
void DeckController::scratchMove(double rate) { m_deck.scratchRate.store(std::clamp(rate, -8.0, 8.0)); }
void DeckController::scratchEnd() { m_deck.scratchActive.store(false); }

// ------------------------------------------------------------------ loops
void DeckController::setLoop(double start, double beats) {
    if (!m_track) return;
    const double bl = m_track->bpm > 0 ? m_track->beatLength() : m_track->sampleRate * 0.5;
    m_loopBeats = beats;
    m_deck.loopStart.store(start);
    m_deck.loopEnd.store(start + beats * bl);
    m_deck.loopActive.store(true);
    emit loopChanged();
}

void DeckController::autoLoop(double beats) {
    if (!m_track) return;
    if (m_deck.loopActive.load() && std::abs(m_loopBeats - beats) < 1e-6) { loopExit(); return; }
    double start = m_deck.position.load();
    if (m_quantize && m_track->bpm > 0) {
        const double bl = m_track->beatLength();
        start = m_track->firstBeat + std::floor((start - m_track->firstBeat) / bl + 0.02) * bl;
        if (start < 0) start = 0;
    }
    setLoop(start, beats);
}

void DeckController::loopHalve() {
    if (!m_deck.loopActive.load() || m_loopBeats <= 1.0 / 32) return;
    setLoop(m_deck.loopStart.load(), m_loopBeats / 2);
    const double pos = m_deck.position.load();
    if (pos >= m_deck.loopEnd.load())
        m_deck.requestSeek(m_deck.loopStart.load() + std::fmod(pos - m_deck.loopStart.load(),
                                                              m_deck.loopEnd.load() - m_deck.loopStart.load()));
}

void DeckController::loopDouble() {
    if (!m_deck.loopActive.load() || m_loopBeats >= 64) return;
    setLoop(m_deck.loopStart.load(), m_loopBeats * 2);
}

void DeckController::loopExit() {
    m_deck.loopActive.store(false);
    emit loopChanged();
}

void DeckController::beatJump(double beats) {
    if (!m_track) return;
    const double bl = m_track->bpm > 0 ? m_track->beatLength() : m_track->sampleRate * 0.5;
    const double np = m_deck.position.load() + beats * bl;
    m_deck.requestSeek(std::clamp(np, 0.0, static_cast<double>(m_track->frames)));
}

// ------------------------------------------------------------------ mezclador
float DeckController::knobToEqDb(double v) {
    if (v <= -0.97) return -60.f;                         // kill
    if (v < 0) return static_cast<float>(-26.0 * std::pow(-v, 1.3));
    return static_cast<float>(6.0 * v);
}

void DeckController::setGain(double v) {
    m_gain = std::clamp(v, -1.0, 1.0);
    m_ch.gainDb.store(static_cast<float>(12.0 * m_gain));
    emit mixerChanged();
}
void DeckController::setEqHigh(double v) {
    m_eqHigh = std::clamp(v, -1.0, 1.0);
    m_ch.eqHighDb.store(knobToEqDb(m_eqHigh));
    emit mixerChanged();
}
void DeckController::setEqMid(double v) {
    m_eqMid = std::clamp(v, -1.0, 1.0);
    m_ch.eqMidDb.store(knobToEqDb(m_eqMid));
    emit mixerChanged();
}
void DeckController::setEqLow(double v) {
    m_eqLow = std::clamp(v, -1.0, 1.0);
    m_ch.eqLowDb.store(knobToEqDb(m_eqLow));
    emit mixerChanged();
}
void DeckController::setFilter(double v) {
    m_filter = std::clamp(v, -1.0, 1.0);
    m_ch.filter.store(static_cast<float>(m_filter));
    emit mixerChanged();
}
void DeckController::setVolume(double v) {
    m_volume = std::clamp(v, 0.0, 1.0);
    m_ch.fader.store(static_cast<float>(m_volume));
    emit mixerChanged();
}
void DeckController::setPfl(bool on) {
    m_ch.pfl.store(on);
    emit mixerChanged();
}

// ------------------------------------------------------------------ efectos
void DeckController::setFxType(int t) {
    t = std::clamp(t, 0, static_cast<int>(EffectUnit::Count) - 1);
    if (t == m_fx.type.load()) return;
    m_fx.type.store(t);
    emit fxChanged();
}
void DeckController::setFxWet(double w) {
    m_fx.wet.store(static_cast<float>(std::clamp(w, 0.0, 1.0)));
    emit fxChanged();
}
void DeckController::setFxBeats(double b) {
    m_fx.beats.store(static_cast<float>(std::clamp(b, 0.0625, 8.0)));
    emit fxChanged();
}
QStringList DeckController::fxNames() const {
    QStringList l;
    for (int i = 0; i < EffectUnit::Count; ++i) l << QString::fromLatin1(EffectUnit::name(i));
    return l;
}

// ------------------------------------------------------------------ stems
QVariantList DeckController::stemOn() const {
    return {m_stemOn[0], m_stemOn[1], m_stemOn[2], m_stemOn[3]};
}

void DeckController::applyStemGains() {
    for (int i = 0; i < 4; ++i) m_deck.stemGain[i].store(m_stemOn[i] ? 1.f : 0.f);
    emit stemsChanged();
}

void DeckController::setStem(int i, bool on) {
    if (i < 0 || i > 3) return;
    m_stemOn[i] = on;
    applyStemGains();
    if (m_stemsState == 0 && m_track) prepareStems();
}

void DeckController::toggleStem(int i) { if (i >= 0 && i < 4) setStem(i, !m_stemOn[i]); }

void DeckController::acapella() {
    const bool only = m_stemOn[0] && !m_stemOn[1] && !m_stemOn[2] && !m_stemOn[3];
    for (int i = 0; i < 4; ++i) m_stemOn[i] = only ? true : (i == 0);
    applyStemGains();
    if (m_stemsState == 0 && m_track) prepareStems();
}

void DeckController::instrumental() {
    const bool inst = !m_stemOn[0] && m_stemOn[1] && m_stemOn[2] && m_stemOn[3];
    for (int i = 0; i < 4; ++i) m_stemOn[i] = inst ? true : (i != 0);
    applyStemGains();
    if (m_stemsState == 0 && m_track) prepareStems();
}

void DeckController::prepareStems() {
    if (!m_track || m_stemsState == 1 || m_stemsState == 2) return;
    if (!stemsAvailable()) {
        m_stemsState = 3;
        m_stemsError = QStringLiteral("Modelo de stems no instalado");
        emit stemsChanged();
        return;
    }
    m_stemsState = 1;
    m_stemsProgress = 0;
    m_stemsError.clear();
    emit stemsChanged();
    std::shared_ptr<Track> track = m_track;
    m_stemJob = StemsService::instance().separate(track, this, [this, track](std::shared_ptr<StemData> sd, QString err) {
        if (track != m_track) return;                 // la pista cambió mientras se separaba
        m_stemJob.reset();
        if (!sd) {
            m_stemsState = 3;
            m_stemsError = err.isEmpty() ? QStringLiteral("No se pudieron separar los stems") : err;
        } else {
            m_stems = sd;
            m_track->stems.store(m_stems.get(), std::memory_order_release);
            m_stemsState = 2;
            m_stemsProgress = 1;
        }
        emit stemsChanged();
        emit stemsProgressChanged();
    });
}

bool DeckController::waitForStems(int ms) {
    QElapsedTimer t;
    t.start();
    while (m_stemsState == 1 && t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return m_stemsState == 2;
}

// ------------------------------------------------------------------ refresco
void DeckController::tick() {
    if (m_stemJob && m_stemsState == 1) {
        const double p = m_stemJob->progress.load();
        if (std::abs(p - m_stemsProgress) > 0.005) { m_stemsProgress = p; emit stemsProgressChanged(); }
    }
    const double pos = m_deck.position.load();
    if (pos != m_lastPos) { m_lastPos = pos; emit positionChanged(); }
    const bool pl = m_deck.playing.load();
    if (pl != m_lastPlaying) { m_lastPlaying = pl; emit playingChanged(); }

    const double pk = m_ch.peak.load();
    const double nl = std::max(pk, m_level * 0.86);
    if (std::abs(nl - m_level) > 1e-4) { m_level = nl; emit levelChanged(); }

    if (m_syncLock && m_partner && m_partner->m_track && m_track && m_track->bpm > 0 &&
        (!m_partner->m_syncLock || m_index > m_partner->m_index)) {
        double ratio = m_partner->bpm() / m_track->bpm;
        while (ratio > 1.5) ratio /= 2.0;
        while (ratio < 0.67) ratio *= 2.0;
        const double want = std::clamp((ratio - 1.0) / m_range, -1.0, 1.0);
        if (std::abs(want - m_pitch) > 1e-9) { m_pitch = want; applyPitch(); }
    }
}

} // namespace jx
