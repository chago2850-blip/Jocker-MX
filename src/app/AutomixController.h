// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "AutomixPlan.h"
#include "library/LibraryDb.h"

#include <QAbstractListModel>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace jx {

class DeckController;
class Mixer;

// Cola de reproducción del Automix.
class AutomixQueue : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles { PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, BpmRole, KeyRole, DurationRole };
    explicit AutomixQueue(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_rows.size()); }
    QStringList paths() const;
    void setPaths(const QStringList& paths);
    void insert(int row, const QStringList& paths);
    QString takeFirst();
    void removeAt(int row);
    void move(int from, int to);
    void clear();
    void shuffle();
    double totalDuration() const;

signals:
    void countChanged();

private:
    static TrackRow info(const QString& path);
    QList<TrackRow> m_rows;
};

// Mezcla automática entre los decks A y B: precarga la siguiente pista en el deck libre,
// sale al inicio de un compás, sincroniza tempo y fase, cruza el crossfader e intercambia los graves.
class AutomixController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY stateChanged)
    Q_PROPERTY(QObject* queue READ queueObject CONSTANT)
    Q_PROPERTY(int transitionBeats READ transitionBeats WRITE setTransitionBeats NOTIFY settingsChanged)
    Q_PROPERTY(bool bassSwap READ bassSwap WRITE setBassSwap NOTIFY settingsChanged)
    Q_PROPERTY(bool tempoReturn READ tempoReturn WRITE setTempoReturn NOTIFY settingsChanged)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY settingsChanged)
    Q_PROPERTY(int activeDeck READ activeDeck NOTIFY stateChanged)       // -1 ninguno, 0 A, 1 B
    Q_PROPERTY(bool mixing READ mixing NOTIFY stateChanged)
    Q_PROPERTY(double mixProgress READ mixProgress NOTIFY progressChanged)
    Q_PROPERTY(double secondsToMix READ secondsToMix NOTIFY progressChanged)  // < 0 si no aplica
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString nextTitle READ nextTitle NOTIFY stateChanged)
    Q_PROPERTY(bool nextSynced READ nextSynced NOTIFY stateChanged)
    Q_PROPERTY(QString planText READ planText NOTIFY stateChanged)       // cómo será la próxima mezcla
    Q_PROPERTY(double queueSeconds READ queueSeconds NOTIFY queueChanged)

public:
    AutomixController(DeckController* a, DeckController* b, Mixer* mixer, QObject* parent = nullptr);
    ~AutomixController() override;

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    QObject* queueObject() { return &m_queue; }
    AutomixQueue& queue() { return m_queue; }
    int transitionBeats() const { return m_beats; }
    void setTransitionBeats(int b);
    bool bassSwap() const { return m_bassSwap; }
    void setBassSwap(bool on);
    bool tempoReturn() const { return m_tempoReturn; }
    void setTempoReturn(bool on);
    bool repeat() const { return m_repeat; }
    void setRepeat(bool on);
    int activeDeck() const { return m_cur; }
    bool mixing() const { return m_state == State::Mixing; }
    double mixProgress() const { return m_progress; }
    double secondsToMix() const { return m_secondsToMix; }
    QString status() const { return m_status; }
    QString nextTitle() const { return m_nextTitle; }
    bool nextSynced() const { return m_nextSynced; }
    QString planText() const { return m_planText; }
    double queueSeconds() const { return m_queue.totalDuration(); }

    Q_INVOKABLE void toggle() { setEnabled(!m_enabled); }
    Q_INVOKABLE void mixNow();                       // mezclar en el siguiente compás
    Q_INVOKABLE void skipNext();                     // descarta la pista precargada
    Q_INVOKABLE void add(const QStringList& paths);  // al final
    Q_INVOKABLE void addNext(const QStringList& paths);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void shuffle();
    Q_INVOKABLE QStringList queuePaths() const { return m_queue.paths(); }

    void step();   // un paso de la máquina de estados (lo llama el temporizador; público para pruebas)

signals:
    void stateChanged();
    void progressChanged();
    void settingsChanged();
    void queueChanged();

private:
    enum class State { Idle, Starting, Playing, Mixing };
    DeckController* deck(int i) const { return i == 0 ? m_a : m_b; }
    void setStatus(const QString& s);
    void saveQueue();
    void startFirst();
    void prepareNext();
    void beginMix(bool now);
    void updateMix();
    void finishMix();
    void restoreEq();
    double side(int d) const { return d == 0 ? -1.0 : 1.0; }

    DeckController* m_a;
    DeckController* m_b;
    Mixer* m_mixer;
    AutomixQueue m_queue;
    QTimer m_timer;

    bool m_enabled = false;
    int m_beats = 0;                    // 0 = automático por estructura
    bool m_bassSwap = true, m_tempoReturn = true, m_repeat = false;
    QStringList m_played;               // para el modo repetir

    State m_state = State::Idle;
    int m_cur = -1;                     // deck que suena
    QString m_startPath;                // pista que se carga para arrancar
    QString m_nextPath;                 // pista que se precarga en el otro deck
    bool m_nextReady = false;
    bool m_nextSynced = false;
    MixPoints m_curPts, m_nextPts;
    bool m_mixNowRequested = false;
    double m_mixNowAt = -1;             // posición (s) del compás donde empezar
    double m_mixStartPos = 0, m_mixLen = 0;
    int m_phaseFixTicks = 0;
    double m_savedEqLow[2] = {0, 0};
    bool m_eqTouched = false;

    double m_progress = 0, m_secondsToMix = -1;
    QString m_status, m_nextTitle, m_planText;
};

} // namespace jx
