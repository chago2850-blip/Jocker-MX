// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls
import QtQuick.Layouts

// Ajustes de controladores MIDI: dispositivos, MIDI learn por acción y sensibilidad del plato.
Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 80 : 900, 900)
    height: Math.min(parent ? parent.height - 80 : 700, 720)
    padding: 0
    onClosed: midi.cancelLearn()

    background: Rectangle { color: Theme.panel; radius: 14; border.color: Theme.gold; border.width: 1 }
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text { text: "🎛  Controladores MIDI"; color: Theme.text; font.pixelSize: 20; font.weight: Font.Bold }
            Item { Layout.fillWidth: true }
            JxButton { text: "Cerrar"; Layout.preferredWidth: 90; onClicked: root.close() }
        }

        // Dispositivos y actividad
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: devCol.implicitHeight + 20
            radius: 10; color: Theme.bg2; border.color: Theme.border
            ColumnLayout {
                id: devCol
                anchors.fill: parent; anchors.margins: 10
                spacing: 4
                Text { text: "DISPOSITIVOS CONECTADOS"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1 }
                Text {
                    Layout.fillWidth: true
                    text: midi.devices.length ? midi.devices.join("   ·   ") : "Ningún dispositivo MIDI detectado. Conecta tu controlador por USB; aparecerá aquí en unos segundos."
                    color: midi.devices.length ? Theme.green : Theme.dim; font.pixelSize: 13; wrapMode: Text.WordWrap
                }
                RowLayout {
                    spacing: 8
                    Text { text: "Último mensaje:"; color: Theme.faint; font.pixelSize: 12 }
                    Text { id: last; text: midi.lastMessage.length ? midi.lastMessage : "—"; color: Theme.gold; font.pixelSize: 12; font.family: "monospace" }
                    Item { width: 20 }
                    Text { text: "Pasos por vuelta del plato:"; color: Theme.faint; font.pixelSize: 12 }
                    JxButton { text: "−"; width: 24; height: 22; onClicked: midi.ticksPerTurn = Math.round(midi.ticksPerTurn / 1.25) }
                    Text { text: midi.ticksPerTurn; color: Theme.text; font.pixelSize: 12 }
                    JxButton { text: "+"; width: 24; height: 22; onClicked: midi.ticksPerTurn = Math.round(midi.ticksPerTurn * 1.25) }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.dim; font.pixelSize: 12
            text: midi.learning.length
                  ? "🎯  Mueve o pulsa ahora el control de tu controlador para «" + midi.learning + "»…  (Esc para cancelar)"
                  : "Pulsa «Aprender» junto a una acción y luego mueve el botón, perilla o fader de tu controlador. Para el plato asigna «Plato (jog)» girándolo y «Toque del plato» tocándolo."
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: midi.actions
            section.property: "group"
            section.delegate: Text {
                width: list.width; height: 30
                verticalAlignment: Text.AlignBottom
                text: section.toUpperCase()
                color: section === "Deck A" ? Theme.magenta : (section === "Deck B" ? Theme.cyan : Theme.gold)
                font.pixelSize: 11; font.weight: Font.Black; font.letterSpacing: 1.5
            }
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: list.width - 12
                height: 32
                radius: 6
                readonly property bool isLearning: midi.learning === modelData.id
                readonly property string bound: midi.bindings[modelData.id] || ""
                color: isLearning ? Qt.rgba(0.96, 0.73, 0.26, 0.18) : (index % 2 ? "transparent" : Qt.rgba(1, 1, 1, 0.03))
                border.color: isLearning ? Theme.gold : "transparent"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 6
                    spacing: 10
                    Text { Layout.preferredWidth: 260; text: modelData.label; color: Theme.text; font.pixelSize: 13 }
                    Text { Layout.fillWidth: true; text: bound.length ? bound : "sin asignar"; color: bound.length ? Theme.green : Theme.faint; font.pixelSize: 12; elide: Text.ElideRight }
                    JxButton { text: isLearning ? "Esperando…" : "Aprender"; Layout.preferredWidth: 100; Layout.preferredHeight: 24; fontSize: 11
                               accent: Theme.gold; active: isLearning
                               onClicked: isLearning ? midi.cancelLearn() : midi.learn(modelData.id) }
                    JxButton { text: "Quitar"; Layout.preferredWidth: 64; Layout.preferredHeight: 24; fontSize: 11; enabledLook: bound.length > 0
                               onClicked: midi.clearBinding(modelData.id) }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: "Mapeo guardado en: " + midi.mappingFile; color: Theme.faint; font.pixelSize: 10; elide: Text.ElideMiddle }
            JxButton { text: "Borrar todo"; Layout.preferredWidth: 110; accent: Theme.red; onClicked: midi.clearAll() }
        }
    }
    Shortcut { sequence: "Escape"; enabled: root.opened && midi.learning.length > 0; onActivated: midi.cancelLearn() }
}
