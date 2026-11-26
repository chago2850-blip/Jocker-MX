// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "analysis/Structure.h"

#include "engine/AudioEngine.h"
#include "stems/StemsService.h"

#include <QObject>
#include <QElapsedTimer>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace jx {

// Puente entre un Deck del motor y la interfaz QML.
class DeckController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")

    Q_PROPERTY(int index READ index CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool loaded READ loaded NOTIFY trackChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString title READ title NOTIFY trackChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY trackChanged)
    Q_PROPERTY(QString musicalKey READ musicalKey NOTIFY trackChanged)
    Q_PROPERTY(QString keyName READ keyName NOTIFY trackChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY trackChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(double duration READ duration NOTIFY trackChanged)
    Q_PROPERTY(double trackBpm READ trackBpm NOTIFY trackChanged)
    Q_PROPERTY(double bpm READ bpm NOTIFY tempoChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double progress READ progress NOTIFY positionChanged)
    Q_PROPERTY(double beatPhase READ beatPhase NOTIFY positionChanged)
    Q_PROPERTY(int beatIndex READ beatIndex NOTIFY positionChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY tempoChanged)          // -1..1 del rango
    Q_PROPERTY(double pitchRange READ pitchRange WRITE setPitchRange NOTIFY tempoChanged)
    Q_PROPERTY(double pitchPercent READ pitchPercent NOTIFY tempoChanged)
    Q_PROPERTY(bool keyLock READ keyLock WRITE setKeyLock NOTIFY keyLockChanged)
    Q_PROPERTY(bool syncLock READ syncLock WRITE setSyncLock NOTIFY syncLockChanged)
    Q_PROPERTY(double cuePoint READ cuePoint NOTIFY cuesChanged)
    Q_PROPERTY(QVariantList hotCues READ hotCues NOTIFY cuesChanged)
    Q_PROPERTY(bool loopActive READ loopActive NOTIFY loopChanged)
    Q_PROPERTY(double loopBeats READ loopBeats NOTIFY loopChanged)
    Q_PROPERTY(double loopStart READ loopStart NOTIFY loopChanged)
    Q_PROPERTY(double loopEnd READ loopEnd NOTIFY loopChanged)
    Q_PROPERTY(bool quantize READ quantize WRITE setQuantize NOTIFY quantizeChanged)

    // Canal del mezclador
    Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY mixerChanged)       // -1..1
    Q_PROPERTY(double eqHigh READ eqHigh WRITE setEqHigh NOTIFY mixerChanged) // -1..1
    Q_PROPERTY(double eqMid READ eqMid WRITE setEqMid NOTIFY mixerChanged)
    Q_PROPERTY(double eqLow READ eqLow WRITE setEqLow NOTIFY mixerChanged)
    Q_PROPERTY(double filter READ filter WRITE setFilter NOTIFY mixerChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY mixerChanged) // 0..1
    Q_PROPERTY(bool pfl READ pfl WRITE setPfl NOTIFY mixerChanged)
    Q_PROPERTY(double level READ level NOTIFY levelChanged)
    // Efectos
    Q_PROPERTY(int fxType READ fxType WRITE setFxType NOTIFY fxChanged)
    Q_PROPERTY(double fxWet READ fxWet WRITE setFxWet NOTIFY fxChanged)
    Q_PROPERTY(double fxBeats READ fxBeats WRITE setFxBeats NOTIFY fxChanged)
    Q_PROPERTY(QStringList fxNames READ fxNames CONSTANT)
    // Stems (IA)
    Q_PROPERTY(bool stemsAvailable READ stemsAvailable CONSTANT)
    Q_PROPERTY(int stemsState READ stemsState NOTIFY stemsChanged)          // 0 nada, 1 separando, 2 listos, 3 error
    Q_PROPERTY(double stemsProgress READ stemsProgress NOTIFY stemsProgressChanged)
    Q_PROPERTY(QString stemsError READ stemsError NOTIFY stemsChanged)
    Q_PROPERTY(QVariantList stemOn READ stemOn NOTIFY stemsChanged)
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)   // [{start, end, kind, name, bars}]
    Q_PROPERTY(int sectionIndex READ sectionIndex NOTIFY sectionIndexChanged)

public:
    DeckController(AudioEngine* engine, int index, QObject* parent = nullptr);

    void setPartner(DeckController* other) { m_partner = other; }
    const Track* track() const { return m_track.get(); }

    int index() const { return m_index; }
    QString name() const { return QString(QChar('A' + m_index)); }
    bool loaded() const { return m_track != nullptr; }
    bool loading() const { return m_loading; }
    QString title() const;
    QString artist() const;
    QString musicalKey() const;
    QString keyName() const;
    Q_INVOKABLE bool keyCompatibleWith(const QString& camelot) const;
    QString filePath() const;
    QString error() const { return m_error; }
    double duration() const;
    double trackBpm() const;
    double bpm() const;
    double position() const;          // segundos
    double progress() const;          // 0..1
    double beatPhase() const;         // 0..1 dentro del beat
    int beatIndex() const;            // número de beat desde el primero
    double positionFrames() const { return m_deck.position.load(); }
    bool playing() const { return m_deck.playing.load(); }
    double pitch() const { return m_pitch; }
    void setPitch(double p);
    double pitchRange() const { return m_range; }
    void setPitchRange(double r);
    double pitchPercent() const { return m_pitch * m_range * 100.0; }
    bool keyLock() const { return m_deck.keyLock.load(); }
    void setKeyLock(bool on);
    bool syncLock() const { return m_syncLock; }
    void setSyncLock(bool on);
    double cuePoint() const { return m_cue; }
    QVariantList hotCues() const;
    bool loopActive() const { return m_deck.loopActive.load(); }
    double loopBeats() const { return m_loopBeats; }
    double loopStart() const;
    double loopEnd() const;
    bool quantize() const { return m_quantize; }
    void setQuantize(bool q) { if (q != m_quantize) { m_quantize = q; emit quantizeChanged(); } }

    double gain() const { return m_gain; }
    void setGain(double v);
    double eqHigh() const { return m_eqHigh; }
    void setEqHigh(double v);
    double eqMid() const { return m_eqMid; }
    void setEqMid(double v);
    double eqLow() const { return m_eqLow; }
    void setEqLow(double v);
    double filter() const { return m_filter; }
    void setFilter(double v);
    double volume() const { return m_volume; }
    void setVolume(double v);
    bool pfl() const { return m_ch.pfl.load(); }
    void setPfl(bool on);
    double level() const { return m_level; }
    int fxType() const { return m_fx.type.load(); }
    void setFxType(int t);
    double fxWet() const { return m_fx.wet.load(); }
    void setFxWet(double w);
    double fxBeats() const { return m_fx.beats.load(); }
    void setFxBeats(double b);
    QStringList fxNames() const;
    Q_INVOKABLE void toggleFx(int t) { setFxType(fxType() == t ? 0 : t); }

    bool stemsAvailable() const { return StemsService::instance().modelAvailable(); }
    int stemsState() const { return m_stemsState; }
    double stemsProgress() const { return m_stemsProgress; }
    QString stemsError() const { return m_stemsError; }
    QVariantList stemOn() const;
    Q_INVOKABLE void toggleStem(int i);
    Q_INVOKABLE void setStem(int i, bool on);
    Q_INVOKABLE void acapella();          // sólo voz (otra vez: todo)
    Q_INVOKABLE void instrumental();      // sin voz (otra vez: todo)
    Q_INVOKABLE void prepareStems();      // separar ahora
    bool waitForStems(int ms);            // pruebas

    Q_INVOKABLE void load(const QString& pathOrUrl);
    Q_INVOKABLE void eject();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void cueDown();
    Q_INVOKABLE void cueUp();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void sync();
    Q_INVOKABLE void seekFraction(double f);
    Q_INVOKABLE void seekSeconds(double s);
    Q_INVOKABLE void jumpToSection(int i);   // salta al inicio de la sección (respeta el compás si suena)
    QVariantList sections() const { return m_sectionsVar; }
    int sectionIndex() const { return m_sectionIndex; }
    const std::vector<Section>& sectionList() const { return m_sections; }
    Q_INVOKABLE void play();               // reproducir (no alterna)
    Q_INVOKABLE void pause();              // pausar sin volver al cue
    Q_INVOKABLE void matchTempo();         // sólo tempo, sin mover la fase
    bool reachedEnd() const { return m_deck.reachedEnd.load(); }
    Q_INVOKABLE void hotCue(int i);         // fija o salta
    Q_INVOKABLE void clearHotCue(int i);
    Q_INVOKABLE void autoLoop(double beats);
    Q_INVOKABLE void loopHalve();
    Q_INVOKABLE void loopDouble();
    Q_INVOKABLE void loopExit();
    Q_INVOKABLE void beatJump(double beats);
    Q_INVOKABLE void nudge(double amount);  // 0 para soltar
    Q_INVOKABLE void scratchBegin();
    Q_INVOKABLE void scratchMove(double rate);
    Q_INVOKABLE void scratchEnd();
    Q_INVOKABLE void resetPitch() { setPitch(0); }
    Q_INVOKABLE void bpmDouble();
    Q_INVOKABLE void bpmHalve();
    Q_INVOKABLE void gridShift(double ms);
    Q_INVOKABLE void gridSetBeatHere();
    Q_INVOKABLE void tapBpm();
    Q_INVOKABLE void setGrid(double bpm, double firstBeat);

    void tick();   // llamado ~60 Hz por la aplicación

signals:
    void trackChanged();
    void loadingChanged();
    void errorChanged();
    void positionChanged();
    void playingChanged();
    void tempoChanged();
    void keyLockChanged();
    void syncLockChanged();
    void cuesChanged();
    void loopChanged();
    void quantizeChanged();
    void mixerChanged();
    void levelChanged();
    void fxChanged();
    void stemsChanged();
    void stemsProgressChanged();
    void sectionsChanged();
    void sectionIndexChanged();
    void trackLoaded(const QString& path, double bpm, const QString& key);
    void played(const QString& path);

private:
    void applyPitch();
    void saveHotCues();
    double quantized(double frame) const;
    void setLoop(double start, double beats);
    static float knobToEqDb(double v);

    AudioEngine* m_engine;
    int m_index;
    Deck& m_deck;
    ChannelParams& m_ch;
    EffectUnit& m_fx;
    DeckController* m_partner = nullptr;
    std::shared_ptr<Track> m_track;
    bool m_loading = false;
    QString m_error;
    int m_loadSerial = 0;

    double m_pitch = 0.0, m_range = 0.08;
    bool m_syncLock = false;
    double m_cue = 0.0;
    double m_hot[8];
    double m_loopBeats = 4.0;
    bool m_quantize = true;
    bool m_cueHeld = false, m_cuePreview = false;
    bool m_lastPlaying = false;
    bool m_wasPlayingBeforeScratch = false;
    double m_lastPos = -1;
    bool m_historyLogged = false;
    QElapsedTimer m_tapTimer;
    QList<qint64> m_taps;

    double m_gain = 0, m_eqHigh = 0, m_eqMid = 0, m_eqLow = 0, m_filter = 0, m_volume = 0.85;
    double m_level = 0;

    void applyStemGains();
    void updateSections();
    std::vector<Section> m_sections;
    QVariantList m_sectionsVar;
    int m_sectionIndex = -1;
    std::shared_ptr<StemData> m_stems;
    std::shared_ptr<StemsService::Job> m_stemJob;
    int m_stemsState = 0;
    double m_stemsProgress = 0;
    QString m_stemsError;
    bool m_stemOn[4] = {true, true, true, true};
};

} // namespace jx
