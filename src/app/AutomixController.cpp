// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "AutomixController.h"

#include "DeckController.h"
#include "Mixer.h"
#include "analysis/Analyzer.h"
#include "engine/Track.h"

#include <QFileInfo>

#include <QRandomGenerator>
#include <QSettings>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace jx {

namespace {
QString localPath(const QString& p) {
    return p.startsWith(QLatin1String("file:")) ? QUrl(p).toLocalFile() : p;
}
double smoothstep(double x) {
    x = std::clamp(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}
QString mmss(double s) {
    const int t = int(std::max(0.0, s) + 0.5);
    return QStringLiteral("%1:%2").arg(t / 60).arg(t % 60, 2, 10, QLatin1Char('0'));
}
} // namespace

// ================================================================== cola
QVariant AutomixQueue::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const TrackRow& r = m_rows[index.row()];
    switch (role) {
    case PathRole: return r.path;
    case TitleRole: return r.title;
    case ArtistRole: return r.artist;
    case BpmRole: return r.bpm;
    case KeyRole: return r.key;
    case DurationRole: return r.duration;
    default: return {};
    }
}

QHash<int, QByteArray> AutomixQueue::roleNames() const {
    return {{PathRole, "path"}, {TitleRole, "title"},       {ArtistRole, "artist"},
            {BpmRole, "bpm"},   {KeyRole, "musicalKey"},    {DurationRole, "duration"}};
}

QStringList AutomixQueue::paths() const {
    QStringList out;
    for (const TrackRow& r : m_rows) out << r.path;
    return out;
}

TrackRow AutomixQueue::info(const QString& path) {
    TrackRow r = LibraryDb::trackInfo(localPath(path));
    QString t, a;
    guessTitleArtist(r.path, &t, &a);
    if (r.artist.isEmpty() && !a.isEmpty() && r.title == QFileInfo(r.path).completeBaseName()) { r.title = t; r.artist = a; }
    return r;
}

void AutomixQueue::setPaths(const QStringList& paths) {
    beginResetModel();
    m_rows.clear();
    for (const QString& p : paths) m_rows << info(p);
    endResetModel();
    emit countChanged();
}

void AutomixQueue::insert(int row, const QStringList& paths) {
    if (paths.isEmpty()) return;
    row = std::clamp(row, 0, int(m_rows.size()));
    beginInsertRows({}, row, row + int(paths.size()) - 1);
    for (int i = 0; i < paths.size(); ++i) m_rows.insert(row + i, info(paths[i]));
    endInsertRows();
    emit countChanged();
}

QString AutomixQueue::takeFirst() {
    if (m_rows.isEmpty()) return {};
    beginRemoveRows({}, 0, 0);
    const QString p = m_rows.takeFirst().path;
    endRemoveRows();
    emit countChanged();
    return p;
}

void AutomixQueue::removeAt(int row) {
    if (row < 0 || row >= m_rows.size()) return;
    beginRemoveRows({}, row, row);
    m_rows.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void AutomixQueue::move(int from, int to) {
    const int n = int(m_rows.size());
    if (from < 0 || from >= n) return;
    to = std::clamp(to, 0, n - 1);
    if (from == to) return;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_rows.move(from, to);
    endMoveRows();
}

void AutomixQueue::clear() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit countChanged();
}

void AutomixQueue::shuffle() {
    beginResetModel();
    for (int i = int(m_rows.size()) - 1; i > 0; --i)
        m_rows.swapItemsAt(i, QRandomGenerator::global()->bounded(i + 1));
    endResetModel();
}

double AutomixQueue::totalDuration() const {
    double s = 0;
    for (const TrackRow& r : m_rows) s += r.duration;
    return s;
}

// ================================================================== controlador
AutomixController::AutomixController(DeckController* a, DeckController* b, Mixer* mixer, QObject* parent)
    : QObject(parent), m_a(a), m_b(b), m_mixer(mixer) {
    QSettings s;
    m_beats = s.value("automix/beats", 0).toInt();
    m_beats = m_beats <= 0 ? 0 : std::clamp(m_beats, 4, 64);
    m_bassSwap = s.value("automix/bassSwap", true).toBool();
    m_tempoReturn = s.value("automix/tempoReturn", true).toBool();
    m_repeat = s.value("automix/repeat", false).toBool();
    m_queue.setPaths(s.value("automix/queue").toStringList());
    connect(&m_queue, &AutomixQueue::countChanged, this, [this]() { saveQueue(); emit queueChanged(); });
    connect(&m_queue, &QAbstractItemModel::rowsMoved, this, [this]() { saveQueue(); });
    connect(&m_queue, &QAbstractItemModel::modelReset, this, [this]() { saveQueue(); });
    m_timer.setInterval(25);
    connect(&m_timer, &QTimer::timeout, this, &AutomixController::step);
    setStatus(QStringLiteral("Automix apagado"));
}

AutomixController::~AutomixController() = default;

void AutomixController::saveQueue() { QSettings().setValue("automix/queue", m_queue.paths()); }

void AutomixController::setStatus(const QString& s) {
    if (s == m_status) return;
    m_status = s;
    emit stateChanged();
}

void AutomixController::setTransitionBeats(int b) {
    b = b <= 0 ? 0 : std::clamp(b, 4, 64);
    if (b == m_beats) return;
    m_beats = b;
    QSettings().setValue("automix/beats", b);
    if (m_state == State::Playing && m_cur >= 0) {
        if (const Track* t = deck(m_cur)->track()) m_curPts = computeMixPoints(*t, m_beats);
        m_nextReady = false;   // volver a planear la transición con la pista ya cargada
    }
    emit settingsChanged();
}

void AutomixController::setBassSwap(bool on) {
    if (on == m_bassSwap) return;
    m_bassSwap = on;
    QSettings().setValue("automix/bassSwap", on);
    emit settingsChanged();
}

void AutomixController::setTempoReturn(bool on) {
    if (on == m_tempoReturn) return;
    m_tempoReturn = on;
    QSettings().setValue("automix/tempoReturn", on);
    emit settingsChanged();
}

void AutomixController::setRepeat(bool on) {
    if (on == m_repeat) return;
    m_repeat = on;
    QSettings().setValue("automix/repeat", on);
    emit settingsChanged();
}

void AutomixController::setEnabled(bool on) {
    if (on == m_enabled) return;
    m_enabled = on;
    if (on) {
        m_state = State::Idle;
        m_cur = -1;
        m_timer.start();
        step();
    } else {
        m_timer.stop();
        if (m_state == State::Mixing) restoreEq();
        // La pista precargada que no llegó a sonar vuelve al principio de la cola.
        if (!m_nextPath.isEmpty()) m_queue.insert(0, {m_nextPath});
        if (!m_startPath.isEmpty()) m_queue.insert(0, {m_startPath});
        m_nextPath.clear();
        m_startPath.clear();
        m_nextReady = m_nextSynced = false;
        m_nextTitle.clear();
        m_state = State::Idle;
        m_cur = -1;
        m_progress = 0;
        m_secondsToMix = -1;
        m_mixNowAt = -1;
        m_mixNowRequested = false;
        m_status = QStringLiteral("Automix apagado");
        emit progressChanged();
    }
    emit stateChanged();
}

// ------------------------------------------------------------------ cola (acciones de la interfaz)
void AutomixController::add(const QStringList& paths) { m_queue.insert(m_queue.count(), paths); }
void AutomixController::addNext(const QStringList& paths) { m_queue.insert(0, paths); }
void AutomixController::removeAt(int row) { m_queue.removeAt(row); }
void AutomixController::move(int from, int to) { m_queue.move(from, to); }
void AutomixController::clear() { m_queue.clear(); }
void AutomixController::shuffle() { m_queue.shuffle(); }

void AutomixController::mixNow() {
    if (!m_enabled) { setEnabled(true); }
    m_mixNowRequested = true;
}

void AutomixController::skipNext() {
    if (m_state == State::Mixing || m_cur < 0) { m_queue.removeAt(0); return; }
    m_nextPath.clear();          // prepareNext cargará la siguiente de la cola
    m_nextReady = m_nextSynced = false;
    m_nextTitle.clear();
    emit stateChanged();
}

// ------------------------------------------------------------------ máquina de estados
void AutomixController::step() {
    if (!m_enabled) return;
    switch (m_state) {
    case State::Idle:
    case State::Starting:
        startFirst();
        break;
    case State::Playing: {
        DeckController* d = deck(m_cur);
        const Track* t = d->track();
        if (!t) { m_state = State::Idle; m_cur = -1; break; }
        prepareNext();

        const double pos = d->position();
        const bool ended = d->reachedEnd() || pos >= t->durationSec() - 0.02;
        if (ended) {
            if (m_nextReady) { beginMix(true); finishMix(); }
            else if (m_nextPath.isEmpty() && m_queue.count() == 0) {
                m_state = State::Idle;
                m_cur = -1;
                setStatus(QStringLiteral("Fin de la cola"));
            }
            break;
        }
        if (!d->playing()) {
            setStatus(QStringLiteral("En pausa · deck %1").arg(d->name()));
            break;
        }
        if (m_mixNowRequested) {
            m_mixNowRequested = false;
            if (m_curPts.beatAligned && t->bpm > 0) {
                const double beat = 60.0 / t->bpm, fb = t->firstBeat / t->sampleRate;
                const double idx = std::ceil((pos - fb) / beat / 4.0 - 1e-6) * 4.0;   // siguiente compás
                m_mixNowAt = fb + idx * beat;
            } else {
                m_mixNowAt = pos;
            }
        }
        const double target = m_mixNowAt >= 0 ? m_mixNowAt : m_curPts.mixOut;
        const double rate = t->bpm > 0 && d->bpm() > 0 ? d->bpm() / t->bpm : 1.0;
        const double left = (target - pos) / std::max(0.1, rate);
        if (std::abs(left - m_secondsToMix) > 0.05) { m_secondsToMix = left; emit progressChanged(); }

        if (pos >= target) {
            if (m_nextReady) { beginMix(m_mixNowAt >= 0); break; }
            setStatus(m_nextPath.isEmpty() ? QStringLiteral("Última pista de la cola")
                                           : QStringLiteral("Cargando la siguiente…"));
            break;
        }

        // Regreso suave al tempo original de la pista (≈ 1 % cada 10 s) mientras no hay mezcla.
        if (m_tempoReturn && std::abs(d->pitchPercent()) > 0.01 && pos > m_curPts.mixIn + 4.0 && left > 20.0) {
            const double stepRate = 0.01 * (m_timer.interval() / 1000.0) / 10.0;      // en tanto por uno
            const double stepPitch = stepRate / std::max(0.01, d->pitchRange());
            const double p = d->pitch();
            d->setPitch(std::abs(p) <= stepPitch ? 0.0 : p - std::copysign(stepPitch, p));
        }
        setStatus(m_nextReady ? QStringLiteral("Mezcla en %1").arg(mmss(left))
                              : QStringLiteral("Sonando deck %1").arg(d->name()));
        break;
    }
    case State::Mixing:
        prepareNext();
        updateMix();
        break;
    }
}

void AutomixController::startFirst() {
    // Si ya suena algo, el Automix continúa desde ahí.
    if (m_startPath.isEmpty()) {
        const bool pa = m_a->playing() && m_a->track(), pb = m_b->playing() && m_b->track();
        if (pa || pb) {
            m_cur = (pa && pb) ? (m_mixer->crossfader() <= 0 ? 0 : 1) : (pa ? 0 : 1);
            m_curPts = computeMixPoints(*deck(m_cur)->track(), m_beats);
            m_state = State::Playing;
            emit stateChanged();
            return;
        }
        if (m_queue.count() == 0) {
            if (m_repeat && !m_played.isEmpty()) { m_queue.setPaths(m_played); m_played.clear(); }
            else { setStatus(QStringLiteral("Agrega pistas a la cola")); return; }
        }
        m_startPath = m_queue.takeFirst();
        m_cur = m_mixer->crossfader() > 0 ? 1 : 0;
        // Si el deck ya tiene esa pista cargada no hace falta volver a cargarla.
        if (deck(m_cur)->filePath() != m_startPath) deck(m_cur)->load(m_startPath);
        m_state = State::Starting;
        setStatus(QStringLiteral("Cargando…"));
        return;
    }
    DeckController* d = deck(m_cur);
    if (d->loading()) return;
    if (!d->error().isEmpty() || d->filePath() != m_startPath) {   // no se pudo cargar: siguiente
        m_startPath.clear();
        m_state = State::Idle;
        return;
    }
    m_curPts = computeMixPoints(*d->track(), m_beats);
    d->setSyncLock(false);
    d->seekSeconds(m_curPts.mixIn);
    d->play();
    m_mixer->setCrossfader(side(m_cur));
    if (m_repeat) m_played << m_startPath;
    m_startPath.clear();
    m_state = State::Playing;
    emit stateChanged();
}

void AutomixController::prepareNext() {
    if (m_cur < 0) return;
    DeckController* cur = deck(m_cur);
    DeckController* o = deck(1 - m_cur);
    if (m_state == State::Mixing) return;   // el otro deck está sonando

    if (m_nextPath.isEmpty()) {
        if (o->loading() || o->playing()) return;
        if (m_queue.count() == 0 && m_repeat && !m_played.isEmpty()) { m_queue.setPaths(m_played); m_played.clear(); }
        if (m_queue.count() == 0) return;
        m_nextPath = m_queue.takeFirst();
        m_nextReady = m_nextSynced = false;
        if (o->filePath() != m_nextPath) o->load(m_nextPath);
        return;
    }
    if (m_nextReady || o->loading()) return;
    if (o->filePath() != m_nextPath) {
        if (!o->error().isEmpty()) {           // archivo dañado o inexistente: saltar
            m_nextPath.clear();
        } else {
            o->load(m_nextPath);               // alguien cargó otra cosa: volver a cargar la nuestra
        }
        return;
    }
    const Track* t = o->track();
    if (!t) return;
    m_nextPts = computeMixPoints(*t, m_beats);
    o->setSyncLock(false);
    o->resetPitch();
    const Track* ct = cur->track();
    if (ct) m_curPts = computeMixPoints(*ct, m_beats);
    m_nextSynced = ct && m_curPts.beatAligned && m_nextPts.beatAligned && tempoCompatible(cur->bpm(), t->bpm, 0.08);
    m_planText.clear();
    if (ct) {
        const TransitionPlan plan = planTransition(*ct, m_curPts, *t, m_nextPts, m_beats, m_nextSynced);
        if (plan.outStart > cur->position() + 0.5) {        // aún da tiempo de mezclar ahí
            m_curPts.mixOut = plan.outStart;
            m_curPts.transition = plan.length;
            m_nextPts.mixIn = plan.inStart;
        }
        m_planText = !m_nextSynced ? QStringLiteral("fundido · tempos distintos")
                   : plan.structural ? plan.describe
                   : QStringLiteral("%1 beats").arg(plan.beats);
    }
    o->seekSeconds(m_nextPts.mixIn);
    m_nextReady = true;
    m_nextTitle = o->artist().isEmpty() ? o->title() : o->artist() + QStringLiteral(" — ") + o->title();
    if (m_repeat && (m_played.isEmpty() || m_played.last() != m_nextPath)) m_played << m_nextPath;
    emit stateChanged();
}

void AutomixController::beginMix(bool now) {
    DeckController* d = deck(m_cur);
    DeckController* o = deck(1 - m_cur);
    const Track* t = d->track();
    const double pos = d->position();

    if (m_nextSynced) {
        o->matchTempo();
        // Entrar con la misma fase que el deck que sale (el compás de salida empieza en un beat).
        const double beatIn = 60.0 / o->track()->bpm;
        o->seekSeconds(m_nextPts.mixIn + d->beatPhase() * beatIn);
        m_phaseFixTicks = 4;   // luego se corrige la fase con el deck ya sonando
    } else {
        m_phaseFixTicks = 0;
    }
    o->play();

    m_savedEqLow[0] = m_a->eqLow();
    m_savedEqLow[1] = m_b->eqLow();
    m_eqTouched = m_bassSwap && m_nextSynced;
    if (m_eqTouched) o->setEqLow(-1.0);

    double len = m_curPts.transition;
    if (now) len = t && t->bpm > 0 ? (m_beats > 0 ? m_beats : 16) * 60.0 / t->bpm : 8.0;
    const double t0 = t ? t->durationSec() : pos;
    len = std::min(len, std::max(0.0, t0 - pos));   // nunca más allá del final
    m_mixStartPos = pos;
    m_mixLen = len;
    m_mixNowAt = -1;
    m_progress = 0;
    m_secondsToMix = -1;
    m_mixer->setCrossfader(side(m_cur));
    m_state = State::Mixing;
    setStatus(QStringLiteral("Mezclando %1 → %2").arg(d->name(), o->name()));
    emit progressChanged();
}

void AutomixController::updateMix() {
    DeckController* d = deck(m_cur);
    DeckController* o = deck(1 - m_cur);
    double p = 1.0;
    if (m_mixLen > 0.01 && d->playing() && !d->reachedEnd())
        p = std::clamp((d->position() - m_mixStartPos) / m_mixLen, 0.0, 1.0);
    if (m_phaseFixTicks > 0 && --m_phaseFixTicks == 0 && o->playing()) o->sync();

    m_mixer->setCrossfader(side(m_cur) * (1.0 - 2.0 * p));
    if (m_eqTouched) {
        const double q = smoothstep((p - 0.45) / 0.1);   // intercambio de graves a mitad de la mezcla
        const int oi = 1 - m_cur;
        o->setEqLow(-1.0 + (m_savedEqLow[oi] + 1.0) * q);
        d->setEqLow(m_savedEqLow[m_cur] + (-1.0 - m_savedEqLow[m_cur]) * q);
    }
    if (std::abs(p - m_progress) > 0.002 || p >= 1.0) { m_progress = p; emit progressChanged(); }
    if (p >= 1.0) finishMix();
}

void AutomixController::restoreEq() {
    if (!m_eqTouched) return;
    m_a->setEqLow(m_savedEqLow[0]);
    m_b->setEqLow(m_savedEqLow[1]);
    m_eqTouched = false;
}

void AutomixController::finishMix() {
    DeckController* d = deck(m_cur);
    d->pause();
    restoreEq();
    const int next = 1 - m_cur;
    m_mixer->setCrossfader(side(next));
    m_cur = next;
    m_curPts = m_nextPts;
    m_nextPath.clear();
    m_nextReady = m_nextSynced = false;
    m_nextTitle.clear();
    m_planText.clear();
    m_progress = 1.0;
    m_state = State::Playing;
    emit progressChanged();
    emit stateChanged();
}

} // namespace jx
