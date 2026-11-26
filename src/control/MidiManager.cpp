// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "MidiManager.h"

#include "app/DeckController.h"
#include "app/Mixer.h"
#include "app/RecordingController.h"
#include "app/SamplerController.h"

#include "RtMidi.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>
#include <cmath>

namespace jx {

struct MidiManager::Input {
    std::unique_ptr<RtMidiIn> in;
    QString name;
};

namespace {
struct ActionDef { const char* id; const char* label; };
// Acciones por deck (prefijo "A." / "B.")
const ActionDef kDeckActions[] = {
    {"play", "Play / pausa"}, {"cue", "Cue"}, {"sync", "Sync"}, {"synclock", "Sync permanente (LOCK)"},
    {"keylock", "Key lock"}, {"quantize", "Cuantizar"}, {"pfl", "Audífonos (PFL)"},
    {"jog", "Plato (jog, relativo)"}, {"jogtouch", "Toque del plato (scratch)"},
    {"pitch", "Fader de pitch"}, {"nudgeminus", "Empujón −"}, {"nudgeplus", "Empujón +"},
    {"volume", "Fader de canal"}, {"gain", "Gain"}, {"eqhigh", "EQ agudos"}, {"eqmid", "EQ medios"},
    {"eqlow", "EQ graves"}, {"filter", "Filtro"},
    {"hotcue1", "Hot cue 1"}, {"hotcue2", "Hot cue 2"}, {"hotcue3", "Hot cue 3"}, {"hotcue4", "Hot cue 4"},
    {"hotcue5", "Hot cue 5"}, {"hotcue6", "Hot cue 6"}, {"hotcue7", "Hot cue 7"}, {"hotcue8", "Hot cue 8"},
    {"loop1", "Loop 1 beat"}, {"loop2", "Loop 2 beats"}, {"loop4", "Loop 4 beats"}, {"loop8", "Loop 8 beats"},
    {"loophalve", "Loop ÷2"}, {"loopdouble", "Loop ×2"}, {"loopexit", "Salir del loop"},
    {"jumpback", "Saltar 4 beats atrás"}, {"jumpfwd", "Saltar 4 beats adelante"},
    {"fx1", "Efecto: eco"}, {"fx2", "Efecto: reverb"}, {"fx3", "Efecto: flanger"}, {"fx4", "Efecto: phaser"},
    {"fx5", "Efecto: crush"}, {"fx6", "Efecto: gate"}, {"fxwet", "Cantidad del efecto"},
    {"stem1", "Stem: voz"}, {"stem2", "Stem: batería"}, {"stem3", "Stem: bajo"}, {"stem4", "Stem: melodía"},
    {"acapella", "Acapella (sólo voz)"}, {"instrumental", "Instrumental (sin voz)"},
};
const ActionDef kGlobalActions[] = {
    {"xfader", "Crossfader"}, {"master", "Volumen master"}, {"cuemix", "Mezcla cue / master"},
    {"phones", "Volumen de audífonos"}, {"rec", "Grabar mezcla"}, {"samplervol", "Volumen del sampler"},
    {"sampler1", "Pad de sampler 1"}, {"sampler2", "Pad de sampler 2"}, {"sampler3", "Pad de sampler 3"},
    {"sampler4", "Pad de sampler 4"}, {"sampler5", "Pad de sampler 5"}, {"sampler6", "Pad de sampler 6"},
    {"sampler7", "Pad de sampler 7"}, {"sampler8", "Pad de sampler 8"},
};
double centered(double v) { const double x = v * 2.0 - 1.0; return std::abs(x) < 0.02 ? 0.0 : x; }
} // namespace

MidiManager::MidiManager(QList<DeckController*> decks, Mixer* mixer, SamplerController* sampler,
                         RecordingController* recorder, QObject* parent)
    : QObject(parent), m_decks(std::move(decks)), m_mixer(mixer), m_sampler(sampler), m_recorder(recorder) {
    loadMapping();
    refreshDevices();
    m_pollTimer.setInterval(2500);
    connect(&m_pollTimer, &QTimer::timeout, this, &MidiManager::refreshDevices);
    m_pollTimer.start();
    m_jogTimer.setInterval(16);
    m_jogTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_jogTimer, &QTimer::timeout, this, &MidiManager::jogTick);
    m_jogTimer.start();
}

MidiManager::~MidiManager() {
    for (auto& i : m_inputs) {
        try { i->in->closePort(); } catch (...) {}
    }
}

QString MidiManager::mappingFile() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/mapeo-midi.json";
}

void MidiManager::refreshDevices() {
    QStringList names;
    std::unique_ptr<RtMidiIn> probe;
    try {
        probe = std::make_unique<RtMidiIn>();
        const unsigned n = probe->getPortCount();
        for (unsigned i = 0; i < n; ++i) names << QString::fromStdString(probe->getPortName(i));
    } catch (const std::exception& e) {
        static bool warned = false;
        if (!warned) { qInfo() << "MIDI no disponible:" << e.what(); warned = true; }
        return;
    }
    if (names == m_deviceNames) return;

    for (auto& i : m_inputs) { try { i->in->closePort(); } catch (...) {} }
    m_inputs.clear();
    for (int p = 0; p < names.size(); ++p) {
        // Evitar el puerto "Through" del sistema y nuestros propios puertos
        if (names[p].contains("Through", Qt::CaseInsensitive)) continue;
        try {
            auto inp = std::make_unique<Input>();
            inp->in = std::make_unique<RtMidiIn>();
            inp->name = names[p];
            inp->in->ignoreTypes(true, true, true);     // sin sysex, timing ni active sensing
            inp->in->setCallback(
                [](double, std::vector<unsigned char>* msg, void* user) {
                    auto* self = static_cast<MidiManager*>(user);
                    const std::vector<unsigned char> copy = *msg;
                    QMetaObject::invokeMethod(self, [self, copy]() { self->handle(copy); }, Qt::QueuedConnection);
                },
                this);
            inp->in->openPort(static_cast<unsigned>(p), "Jocker-MX");
            m_inputs.push_back(std::move(inp));
        } catch (const std::exception& e) {
            qWarning() << "No se pudo abrir" << names[p] << e.what();
        }
    }
    m_deviceNames = names;
    emit devicesChanged();
}

QVariantList MidiManager::actions() const {
    QVariantList out;
    for (int d = 0; d < m_decks.size(); ++d) {
        const QString pre = QString(QChar('A' + d)) + '.';
        for (const ActionDef& a : kDeckActions)
            out << QVariantMap{{"id", pre + a.id}, {"label", QString::fromUtf8(a.label)}, {"group", "Deck " + QString(QChar('A' + d))}};
    }
    for (const ActionDef& a : kGlobalActions)
        out << QVariantMap{{"id", QString::fromLatin1(a.id)}, {"label", QString::fromUtf8(a.label)},
                           {"group", QString::fromLatin1(a.id).startsWith("sampler") ? "Sampler" : "Mezclador y grabación"}};
    return out;
}

QString MidiManager::describe(int key) {
    const int type = (key >> 16) & 0xFF, ch = (key >> 8) & 0xFF, num = key & 0xFF;
    QString t = type == 0x90 ? QStringLiteral("Nota %1").arg(num)
              : type == 0xB0 ? QStringLiteral("CC %1").arg(num)
              : type == 0xE0 ? QStringLiteral("Pitch bend") : QStringLiteral("0x%1").arg(type, 0, 16);
    return t + QStringLiteral(" · canal %1").arg(ch + 1);
}

QVariantMap MidiManager::bindings() const {
    QVariantMap m;
    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        const QString prev = m.value(it.value()).toString();
        m.insert(it.value(), prev.isEmpty() ? describe(it.key()) : prev + ", " + describe(it.key()));
    }
    return m;
}

void MidiManager::setTicksPerTurn(int t) {
    t = std::clamp(t, 16, 8192);
    if (t == m_ticksPerTurn) return;
    m_ticksPerTurn = t;
    saveMapping();
    emit settingsChanged();
}

void MidiManager::learn(const QString& action) { m_learning = action; emit learningChanged(); }
void MidiManager::cancelLearn() { m_learning.clear(); emit learningChanged(); }

void MidiManager::clearBinding(const QString& action) {
    for (auto it = m_map.begin(); it != m_map.end();) it = it.value() == action ? m_map.erase(it) : std::next(it);
    saveMapping();
    emit bindingsChanged();
}

void MidiManager::clearAll() {
    m_map.clear();
    saveMapping();
    emit bindingsChanged();
}

void MidiManager::injectMessage(int status, int data1, int data2) {
    handle({static_cast<unsigned char>(status), static_cast<unsigned char>(data1), static_cast<unsigned char>(data2)});
}

void MidiManager::handle(const std::vector<unsigned char>& msg) {
    if (msg.size() < 2) return;
    const int status = msg[0];
    int type = status & 0xF0;
    const int ch = status & 0x0F;
    const int d1 = msg[1];
    const int d2 = msg.size() > 2 ? msg[2] : 0;
    bool pressed = false;
    int number = d1;
    if (type == 0x80) { type = 0x90; pressed = false; }
    else if (type == 0x90) pressed = d2 > 0;
    else if (type == 0xB0) pressed = d2 > 0;
    else if (type == 0xE0) number = 0;
    else return;
    const int key = keyOf(type, ch, number);
    m_lastMessage = describe(key) + QStringLiteral(" = %1").arg(type == 0xE0 ? ((d2 << 7) | d1) : d2);
    emit activity();

    if (!m_learning.isEmpty()) {
        if (type == 0x90 && !pressed) return;                 // aprender con la pulsación, no al soltar
        for (auto it = m_map.begin(); it != m_map.end();)
            it = (it.value() == m_learning) ? m_map.erase(it) : std::next(it);
        m_map.insert(key, m_learning);
        m_learning.clear();
        saveMapping();
        emit bindingsChanged();
        emit learningChanged();
        return;
    }
    const auto it = m_map.find(key);
    if (it == m_map.end()) return;
    dispatch(it.value(), type, d2, (d2 << 7) | d1, pressed);
}

void MidiManager::dispatch(const QString& action, int type, int value, int value14, bool pressed) {
    const double v = type == 0xE0 ? value14 / 16383.0 : value / 127.0;
    const int dot = action.indexOf('.');
    if (dot < 0) {
        if (action == "xfader") m_mixer->setCrossfader(centered(v));
        else if (action == "master") m_mixer->setMasterVolume(v);
        else if (action == "cuemix") m_mixer->setCueMix(v);
        else if (action == "phones") m_mixer->setHeadphoneVolume(v);
        else if (action == "samplervol" && m_sampler) m_sampler->setVolume(v);
        else if (action == "rec" && m_recorder && pressed) m_recorder->toggle();
        else if (action.startsWith("sampler") && m_sampler) {
            const int slot = action.mid(7).toInt() - 1;
            pressed ? m_sampler->press(slot) : m_sampler->release(slot);
        }
        return;
    }
    const int di = action.at(0).unicode() - 'A';
    if (di < 0 || di >= m_decks.size()) return;
    DeckController* d = m_decks[di];
    const QString a = action.mid(dot + 1);

    if (a == "jog") {
        int delta = value < 64 ? value : value - 128;               // complemento a dos
        if (value >= 57 && value <= 71) delta = value - 64;         // codificación centrada en 64
        m_jog[di].accum += delta;
        return;
    }
    if (a == "jogtouch") {
        m_jog[di].touch = pressed;
        if (pressed) { d->scratchBegin(); m_jog[di].scratching = true; }
        else { d->scratchEnd(); m_jog[di].scratching = false; }
        return;
    }
    // Controles continuos
    if (a == "volume") { d->setVolume(v); return; }
    if (a == "gain") { d->setGain(centered(v)); return; }
    if (a == "eqhigh") { d->setEqHigh(centered(v)); return; }
    if (a == "eqmid") { d->setEqMid(centered(v)); return; }
    if (a == "eqlow") { d->setEqLow(centered(v)); return; }
    if (a == "filter") { d->setFilter(centered(v)); return; }
    if (a == "pitch") { d->setSyncLock(false); d->setPitch(centered(v)); return; }
    if (a == "fxwet") { d->setFxWet(v); return; }

    // Botones
    if (a == "cue") { pressed ? d->cueDown() : d->cueUp(); return; }
    if (a == "nudgeminus") { d->nudge(pressed ? -0.04 : 0.0); return; }
    if (a == "nudgeplus") { d->nudge(pressed ? 0.04 : 0.0); return; }
    if (!pressed) return;
    if (a == "play") d->playPause();
    else if (a == "sync") d->sync();
    else if (a == "synclock") d->setSyncLock(!d->syncLock());
    else if (a == "keylock") d->setKeyLock(!d->keyLock());
    else if (a == "quantize") d->setQuantize(!d->quantize());
    else if (a == "pfl") d->setPfl(!d->pfl());
    else if (a.startsWith("hotcue")) d->hotCue(a.mid(6).toInt() - 1);
    else if (a.startsWith("loop") && a.mid(4).toInt() > 0) d->autoLoop(a.mid(4).toInt());
    else if (a == "loophalve") d->loopHalve();
    else if (a == "loopdouble") d->loopDouble();
    else if (a == "loopexit") d->loopExit();
    else if (a == "jumpback") d->beatJump(-4);
    else if (a == "jumpfwd") d->beatJump(4);
    else if (a.startsWith("fx") && a.mid(2).toInt() > 0) d->toggleFx(a.mid(2).toInt());
    else if (a.startsWith("stem")) d->toggleStem(a.mid(4).toInt() - 1);
    else if (a == "acapella") d->acapella();
    else if (a == "instrumental") d->instrumental();
}

void MidiManager::jogTick() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < m_decks.size() && i < 4; ++i) {
        JogState& j = m_jog[i];
        DeckController* d = m_decks[i];
        const double dt = j.lastMs ? std::max<qint64>(4, now - j.lastMs) / 1000.0 : 0.016;
        j.lastMs = now;
        const double turns = j.accum / m_ticksPerTurn;
        const bool moving = j.accum != 0;
        j.accum = 0;
        if (j.touch || (!d->playing() && (moving || j.scratching))) {
            // Scratch: con el plato tocado, o búsqueda con el deck detenido
            if (!j.scratching) { d->scratchBegin(); j.scratching = true; }
            d->scratchMove(turns * 1.8 / dt);
            if (!j.touch && !moving) { d->scratchEnd(); j.scratching = false; }
        } else if (moving) {
            d->nudge(std::clamp(turns * 1.8 / dt * 0.25, -0.3, 0.3));   // pitch bend por el aro
            j.nudging = true;
        } else if (j.nudging) {
            d->nudge(0);
            j.nudging = false;
        }
    }
}

bool MidiManager::loadMapping(const QString& file) {
    QFile f(file.isEmpty() ? mappingFile() : file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    m_map.clear();
    for (const QJsonValue& v : root.value("bindings").toArray()) {
        const QJsonObject o = v.toObject();
        const QString t = o.value("type").toString();
        const int type = t == "note" ? 0x90 : t == "cc" ? 0xB0 : t == "pitchbend" ? 0xE0 : 0;
        if (!type) continue;
        m_map.insert(keyOf(type, o.value("channel").toInt() - 1, o.value("number").toInt()), o.value("action").toString());
    }
    m_ticksPerTurn = root.value("ticksPerTurn").toInt(400);
    emit bindingsChanged();
    return true;
}

bool MidiManager::saveMapping(const QString& file) const {
    QJsonArray arr;
    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        const int type = (it.key() >> 16) & 0xFF;
        arr.append(QJsonObject{{"type", type == 0x90 ? "note" : type == 0xB0 ? "cc" : "pitchbend"},
                               {"channel", ((it.key() >> 8) & 0xFF) + 1},
                               {"number", it.key() & 0xFF},
                               {"action", it.value()}});
    }
    QJsonObject root{{"format", "jocker-mx-midi"}, {"version", 1}, {"ticksPerTurn", m_ticksPerTurn}, {"bindings", arr}};
    QFile f(file.isEmpty() ? mappingFile() : file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson());
    return true;
}

} // namespace jx
