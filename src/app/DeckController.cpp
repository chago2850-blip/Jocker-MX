// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "DeckController.h"

#include "analysis/Analyzer.h"

#include <QCryptographicHash>
#include <QFutureWatcher>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

namespace jx {

namespace {
QString cueKey(const QString& path) {
    return QStringLiteral("cues/") +
           QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex());
}
} // namespace

DeckController::DeckController(AudioEngine* engine, int index, QObject* parent)
    : QObject(parent), m_engine(engine), m_index(index), m_deck(engine->deck(index)),
      m_ch(engine->channel(index)) {
    for (double& h : m_hot) h = -1;
    setVolume(m_volume);
}

QString DeckController::title() const { return m_track ? m_track->title : QString(); }
QString DeckController::artist() const { return m_track ? m_track->artist : QString(); }
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
        // Mantener viva la pista anterior un momento: el hilo de audio podría estar leyéndola.
        std::shared_ptr<Track> old = m_track;
        m_track = t;
        m_deck.setTrack(m_track.get());
        if (old) QTimer::singleShot(2000, this, [old]() {});

        // Restaurar cues guardados
        QSettings s;
        s.beginGroup(cueKey(m_track->path));
        m_cue = s.value("cue", m_track->firstBeat).toDouble();
        for (int i = 0; i < 8; ++i) m_hot[i] = s.value(QStringLiteral("hot%1").arg(i), -1.0).toDouble();
        const double bpmOverride = s.value("bpm", 0.0).toDouble();
        s.endGroup();
        if (bpmOverride > 0) m_track->bpm = bpmOverride;
        m_deck.requestSeek(m_cue);
        m_loopBeats = 4;
        emit trackChanged();
        emit cuesChanged();
        emit loopChanged();
        emit tempoChanged();
        emit trackLoaded(m_track->path, m_track->bpm);
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
    m_deck.setTrack(nullptr);
    m_track.reset();
    if (old) QTimer::singleShot(2000, this, [old]() {});
    emit trackChanged();
}

// ------------------------------------------------------------------ transporte
void DeckController::playPause() {
    if (!m_track) return;
    m_cuePreview = false;
    if (m_deck.reachedEnd.load()) { m_deck.reachedEnd.store(false); m_deck.requestSeek(m_cue); }
    m_deck.playing.store(!m_deck.playing.load());
    emit playingChanged();
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
            QSettings s;
            s.setValue(cueKey(m_track->path) + "/cue", m_cue);
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
        QSettings s;
        s.setValue(cueKey(m_track->path) + QStringLiteral("/hot%1").arg(i), m_hot[i]);
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
    QSettings s;
    s.remove(cueKey(m_track->path) + QStringLiteral("/hot%1").arg(i));
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

// ------------------------------------------------------------------ refresco
void DeckController::tick() {
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
