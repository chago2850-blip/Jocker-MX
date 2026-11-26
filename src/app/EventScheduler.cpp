// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "EventScheduler.h"

#include "AutomixController.h"
#include "DeckController.h"
#include "Mixer.h"
#include "RecordingController.h"
#include "library/LibraryDb.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <algorithm>

namespace jx {

EventScheduler::EventScheduler(AutomixController* automix, DeckController* a, DeckController* b, Mixer* mixer,
                               RecordingController* recorder, QObject* parent)
    : QObject(parent), m_automix(automix), m_a(a), m_b(b), m_mixer(mixer), m_recorder(recorder) {
    m_clock = []() { return QDateTime::currentDateTime(); };
    load();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &EventScheduler::check);
    m_timer.start();

    // Fundido del volumen maestro (acción «Bajar volumen y parar»)
    m_fadeTimer.setInterval(40);
    connect(&m_fadeTimer, &QTimer::timeout, this, [this]() {
        const double p = std::clamp((QDateTime::currentMSecsSinceEpoch() - m_fadeStartMs) / (m_fadeSecs * 1000.0), 0.0, 1.0);
        m_mixer->setMasterVolume(m_fadeFrom * (1.0 - p) * (1.0 - p));
        if (p < 1.0) return;
        m_fadeTimer.stop();
        if (m_automix) m_automix->setEnabled(false);
        m_a->pause();
        m_b->pause();
        m_mixer->setMasterVolume(m_fadeFrom);   // listo para la próxima vez
        emit fadingChanged();
    });
}

QString EventScheduler::actionName(int a) {
    switch (a) {
    case AutomixOn: return QStringLiteral("Encender Automix");
    case AutomixOff: return QStringLiteral("Apagar Automix");
    case PlayPlaylist: return QStringLiteral("Reproducir lista en Automix");
    case MixNow: return QStringLiteral("Automix: mezclar ya");
    case RecStart: return QStringLiteral("Empezar a grabar");
    case RecStop: return QStringLiteral("Detener la grabación");
    case FadeOutStop: return QStringLiteral("Bajar volumen y parar");
    default: return {};
    }
}

QStringList EventScheduler::actionNames() const {
    QStringList l;
    for (int i = 0; i < ActionCount; ++i) l << actionName(i);
    return l;
}

QString EventScheduler::describe(const Event& e) const {
    QString s = actionName(e.action);
    if (e.action == PlayPlaylist) {
        QString name;
        for (const PlaylistInfo& p : LibraryDb::playlists()) if (p.id == e.param) name = p.name;
        s += name.isEmpty() ? QStringLiteral(" (elige una lista)") : QStringLiteral(" «%1»").arg(name);
    } else if (e.action == FadeOutStop) {
        s += QStringLiteral(" (%1 s)").arg(e.param > 0 ? e.param : 20);
    }
    return s;
}

QVariantList EventScheduler::events() const {
    QVariantList out;
    for (const Event& e : m_events) {
        QString paramLabel;
        if (e.action == PlayPlaylist) {
            for (const PlaylistInfo& p : LibraryDb::playlists()) if (p.id == e.param) paramLabel = p.name;
            if (paramLabel.isEmpty()) paramLabel = QStringLiteral("Elegir lista…");
        } else if (e.action == FadeOutStop) {
            paramLabel = QStringLiteral("%1 s").arg(e.param > 0 ? e.param : 20);
        }
        out << QVariantMap{{"enabled", e.enabled}, {"time", e.time.toString("HH:mm")}, {"action", e.action},
                           {"actionName", actionName(e.action)}, {"param", e.param}, {"paramLabel", paramLabel},
                           {"once", e.once}, {"description", describe(e)}};
    }
    return out;
}

QString EventScheduler::nextText() const {
    const QDateTime now = m_clock();
    qint64 best = -1;
    const Event* next = nullptr;
    for (const Event& e : m_events) {
        if (!e.enabled) continue;
        QDateTime at(now.date(), e.time);
        if (at <= now || e.lastFired == now.date()) at = at.addDays(1);
        const qint64 d = now.secsTo(at);
        if (best < 0 || d < best) { best = d; next = &e; }
    }
    if (!next) return QStringLiteral("Sin eventos activos");
    return QStringLiteral("Próximo: %1 · %2").arg(next->time.toString("HH:mm"), describe(*next));
}

// ------------------------------------------------------------------ edición
int EventScheduler::addEvent() {
    Event e;
    const QTime t = m_clock().time();
    e.time = QTime((t.hour() + 1) % 24, 0);   // la próxima hora en punto
    m_events << e;
    save();
    return int(m_events.size()) - 1;
}

void EventScheduler::removeEvent(int i) {
    if (i < 0 || i >= m_events.size()) return;
    m_events.removeAt(i);
    save();
}

void EventScheduler::setEventEnabled(int i, bool on) {
    if (i < 0 || i >= m_events.size()) return;
    m_events[i].enabled = on;
    if (on) m_events[i].lastFired = {};
    save();
}

bool EventScheduler::setEventTime(int i, const QString& hhmm) {
    if (i < 0 || i >= m_events.size()) return false;
    const QTime t = QTime::fromString(hhmm.trimmed(), "H:mm");
    if (!t.isValid()) { emit eventsChanged(); return false; }
    m_events[i].time = t;
    m_events[i].lastFired = {};
    save();
    return true;
}

void EventScheduler::setEventAction(int i, int action) {
    if (i < 0 || i >= m_events.size() || action < 0 || action >= ActionCount) return;
    m_events[i].action = action;
    if (action == FadeOutStop && m_events[i].param <= 0) m_events[i].param = 20;
    if (action != PlayPlaylist && action != FadeOutStop) m_events[i].param = 0;
    save();
}

void EventScheduler::setEventParam(int i, qint64 param) {
    if (i < 0 || i >= m_events.size()) return;
    m_events[i].param = param;
    save();
}

void EventScheduler::setEventOnce(int i, bool once) {
    if (i < 0 || i >= m_events.size()) return;
    m_events[i].once = once;
    save();
}

void EventScheduler::runNow(int i) {
    if (i < 0 || i >= m_events.size()) return;
    Event copy = m_events[i];
    fire(copy);   // no cuenta como disparo programado
}

// ------------------------------------------------------------------ disparo
void EventScheduler::check() {
    const QDateTime now = m_clock();
    bool changed = false;
    for (Event& e : m_events) {
        if (!e.enabled || e.lastFired == now.date()) continue;
        const QDateTime at(now.date(), e.time);
        const qint64 late = at.secsTo(now);
        if (late < 0 || late > 90) continue;   // sólo dentro del minuto y medio siguiente a la hora
        e.lastFired = now.date();
        if (e.once) e.enabled = false;
        fire(e);
        changed = true;
    }
    if (changed) save();
}

void EventScheduler::fire(Event& e) {
    switch (e.action) {
    case AutomixOn:
        if (m_automix) m_automix->setEnabled(true);
        break;
    case AutomixOff:
        if (m_automix) m_automix->setEnabled(false);
        break;
    case PlayPlaylist:
        if (m_automix) {
            const QStringList paths = LibraryDb::playlistPaths(e.param);
            if (paths.isEmpty()) {
                m_lastMessage = QStringLiteral("%1 · la lista está vacía o ya no existe").arg(e.time.toString("HH:mm"));
                emit fired(m_lastMessage);
                return;
            }
            // Si ya está sonando algo, el Automix mezclará desde ahí hacia la lista.
            m_automix->clear();
            m_automix->add(paths);
            m_automix->setEnabled(true);
        }
        break;
    case MixNow:
        if (m_automix) m_automix->mixNow();
        break;
    case RecStart:
        if (m_recorder && !m_recorder->recording()) m_recorder->start();
        break;
    case RecStop:
        if (m_recorder && m_recorder->recording()) m_recorder->stop();
        break;
    case FadeOutStop:
        if (!m_fadeTimer.isActive()) {
            m_fadeFrom = m_mixer->masterVolume();
            m_fadeSecs = std::max<qint64>(1, e.param > 0 ? e.param : 20);
            m_fadeStartMs = QDateTime::currentMSecsSinceEpoch();
            m_fadeTimer.start();
            emit fadingChanged();
        }
        break;
    default:
        return;
    }
    m_lastMessage = QStringLiteral("%1 · %2").arg(e.time.toString("HH:mm"), describe(e));
    emit fired(m_lastMessage);
}

// ------------------------------------------------------------------ persistencia
void EventScheduler::save() {
    QJsonArray arr;
    for (const Event& e : m_events)
        arr.append(QJsonObject{{"enabled", e.enabled}, {"time", e.time.toString("HH:mm")}, {"action", e.action},
                               {"param", QString::number(e.param)}, {"once", e.once},
                               {"lastFired", e.lastFired.toString(Qt::ISODate)}});
    QSettings().setValue("scheduler/events", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    emit eventsChanged();
}

void EventScheduler::load() {
    m_events.clear();
    const QJsonArray arr = QJsonDocument::fromJson(QSettings().value("scheduler/events").toString().toUtf8()).array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        Event e;
        e.enabled = o.value("enabled").toBool(true);
        e.time = QTime::fromString(o.value("time").toString(), "HH:mm");
        if (!e.time.isValid()) continue;
        e.action = std::clamp(o.value("action").toInt(), 0, int(ActionCount) - 1);
        e.param = o.value("param").toString().toLongLong();
        e.once = o.value("once").toBool(false);
        e.lastFired = QDate::fromString(o.value("lastFired").toString(), Qt::ISODate);
        m_events << e;
    }
}

} // namespace jx
