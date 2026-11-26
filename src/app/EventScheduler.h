// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <functional>

namespace jx {

class AutomixController;
class DeckController;
class Mixer;
class RecordingController;

// Programador de eventos: acciones que se disparan solas a una hora (todos los días o una sola vez).
// Ej.: 21:00 reproducir la lista «Cena» en Automix · 23:30 empezar a grabar · 02:00 bajar el volumen y parar.
class EventScheduler : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QStringList actionNames READ actionNames CONSTANT)
    Q_PROPERTY(QString nextText READ nextText NOTIFY eventsChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY fired)
    Q_PROPERTY(bool fading READ fading NOTIFY fadingChanged)

public:
    enum Action { AutomixOn = 0, AutomixOff, PlayPlaylist, MixNow, RecStart, RecStop, FadeOutStop, ActionCount };

    struct Event {
        bool enabled = true;
        QTime time{21, 0};
        int action = AutomixOn;
        qint64 param = 0;        // id de lista, o segundos del fundido
        bool once = false;       // sólo una vez (se desactiva al dispararse)
        QDate lastFired;
    };

    EventScheduler(AutomixController* automix, DeckController* a, DeckController* b, Mixer* mixer,
                   RecordingController* recorder, QObject* parent = nullptr);

    QVariantList events() const;
    QStringList actionNames() const;
    static QString actionName(int a);
    QString nextText() const;
    QString lastMessage() const { return m_lastMessage; }
    bool fading() const { return m_fadeTimer.isActive(); }
    const QList<Event>& list() const { return m_events; }

    Q_INVOKABLE int addEvent();
    Q_INVOKABLE void removeEvent(int i);
    Q_INVOKABLE void setEventEnabled(int i, bool on);
    Q_INVOKABLE bool setEventTime(int i, const QString& hhmm);   // "HH:mm"; false si no es válida
    Q_INVOKABLE void setEventAction(int i, int action);
    Q_INVOKABLE void setEventParam(int i, qint64 param);
    Q_INVOKABLE void setEventOnce(int i, bool once);
    Q_INVOKABLE void runNow(int i);                              // probar la acción ya

    // Para pruebas: reloj inyectable y revisión manual.
    void setClock(std::function<QDateTime()> clock) { m_clock = std::move(clock); }
    void check();

signals:
    void eventsChanged();
    void fired(const QString& message);
    void fadingChanged();

private:
    void fire(Event& e);
    void save();
    void load();
    QString describe(const Event& e) const;

    AutomixController* m_automix;
    DeckController* m_a;
    DeckController* m_b;
    Mixer* m_mixer;
    RecordingController* m_recorder;
    QList<Event> m_events;
    QTimer m_timer, m_fadeTimer;
    std::function<QDateTime()> m_clock;
    QString m_lastMessage;
    double m_fadeFrom = 1, m_fadeSecs = 20;
    qint64 m_fadeStartMs = 0;
};

} // namespace jx
