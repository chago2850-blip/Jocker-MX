// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

namespace rt { namespace midi { class RtMidiIn; } }

namespace jx {

class DeckController;
class Mixer;

// Controladores MIDI: entradas de todos los dispositivos, mapeo de mensajes a acciones y MIDI learn.
class MidiManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QStringList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList actions READ actions CONSTANT)
    Q_PROPERTY(QVariantMap bindings READ bindings NOTIFY bindingsChanged)
    Q_PROPERTY(QString learning READ learning NOTIFY learningChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY activity)
    Q_PROPERTY(int ticksPerTurn READ ticksPerTurn WRITE setTicksPerTurn NOTIFY settingsChanged)
    Q_PROPERTY(QString mappingFile READ mappingFile CONSTANT)

public:
    MidiManager(QList<DeckController*> decks, Mixer* mixer, QObject* parent = nullptr);
    ~MidiManager() override;

    QStringList devices() const { return m_deviceNames; }
    QVariantList actions() const;
    QVariantMap bindings() const;          // acción → descripción del mensaje ("CC 7 · canal 1")
    QString learning() const { return m_learning; }
    QString lastMessage() const { return m_lastMessage; }
    int ticksPerTurn() const { return m_ticksPerTurn; }
    void setTicksPerTurn(int t);
    QString mappingFile() const;

    Q_INVOKABLE void learn(const QString& action);     // el próximo mensaje se asigna a la acción
    Q_INVOKABLE void cancelLearn();
    Q_INVOKABLE void clearBinding(const QString& action);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void refreshDevices();
    // Inyecta un mensaje como si viniera de un dispositivo (pruebas, controladores virtuales).
    Q_INVOKABLE void injectMessage(int status, int data1, int data2);

    bool loadMapping(const QString& file = QString());
    bool saveMapping(const QString& file = QString()) const;

signals:
    void devicesChanged();
    void bindingsChanged();
    void learningChanged();
    void activity();
    void settingsChanged();

private:
    struct Input;   // contiene rt::midi::RtMidiIn
    static int keyOf(int type, int channel, int number) { return (type << 16) | (channel << 8) | number; }
    void handle(const std::vector<unsigned char>& msg);
    void dispatch(const QString& action, int type, int value, int value14, bool pressed);
    void jogTick();
    static QString describe(int key);

    QList<DeckController*> m_decks;
    Mixer* m_mixer;
    std::vector<std::unique_ptr<Input>> m_inputs;
    QStringList m_deviceNames;
    QHash<int, QString> m_map;            // clave del mensaje → acción
    QString m_learning;
    QString m_lastMessage;
    int m_ticksPerTurn = 400;
    QTimer m_pollTimer, m_jogTimer;

    struct JogState { double accum = 0; bool touch = false; bool scratching = false; bool nudging = false; qint64 lastMs = 0; };
    JogState m_jog[4];
};

} // namespace jx
