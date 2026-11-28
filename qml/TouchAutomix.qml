// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JockerMX

// Automix táctil: encendido grande, qué suena y qué sigue, mezclar ya, estilo y duración, y la cola.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property bool wide: width > 640

    function fmt(s) { return s > 0 ? Math.floor(s / 60) + ":" + ("0" + Math.floor(s % 60)).slice(-2) : "" }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: root.wide ? 2 : 1
        columnSpacing: 10; rowSpacing: 8

        // ---- Controles
        ColumnLayout {
            Layout.preferredWidth: root.wide ? Math.min(320, root.width * 0.4) : -1
            Layout.fillWidth: !root.wide
            Layout.fillHeight: root.wide
            spacing: 6
            TouchButton {
                Layout.fillWidth: true; Layout.preferredHeight: 54
                text: automix.enabled ? (automix.mixing ? "● MEZCLANDO" : "● AUTOMIX") : "AUTOMIX"
                sub: automix.status
                fontSize: 18; accent: Theme.green; active: automix.enabled
                onClicked: automix.toggle()
            }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 6; radius: 3; color: Theme.bg2
                Rectangle { height: parent.height; radius: 3; color: Theme.green
                            width: parent.width * (automix.mixing ? automix.mixProgress : 0) }
            }
            Text {
                Layout.fillWidth: true; elide: Text.ElideRight
                visible: automix.nextTitle.length > 0
                text: "⏭ " + automix.nextTitle + (automix.planText.length ? "  ·  " + automix.planText : "")
                color: Theme.dim; font.pixelSize: 12
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 6
                TouchButton { Layout.fillWidth: true; Layout.preferredHeight: 44; text: "⏩ Mezclar ya"; fontSize: 13; accent: Theme.gold
                              enabledLook: automix.enabled && !automix.mixing; onClicked: automix.mixNow() }
                TouchButton { Layout.preferredWidth: 90; Layout.preferredHeight: 44; text: "Saltar"; fontSize: 13
                              enabledLook: automix.enabled; onClicked: automix.skipNext() }
            }
            Flow {
                Layout.fillWidth: true; spacing: 5
                Repeater {
                    model: automix.styleNames
                    TouchButton { width: 70; height: 36; text: modelData; fontSize: 11; accent: Theme.magenta
                                  active: automix.style === index; onClicked: automix.style = index }
                }
            }
            Flow {
                Layout.fillWidth: true; spacing: 5
                Repeater {
                    model: [0, 8, 16, 32]
                    TouchButton { width: 70; height: 36; text: modelData === 0 ? "AUTO" : modelData + " b"; fontSize: 11; accent: Theme.cyan
                                  active: automix.transitionBeats === modelData; onClicked: automix.transitionBeats = modelData }
                }
            }
            Item { Layout.fillHeight: root.wide; visible: root.wide }
        }

        // ---- Cola
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true; spacing: 6
                Text { Layout.fillWidth: true; elide: Text.ElideRight
                       text: "COLA · " + automix.queue.count + (automix.queue.count === 1 ? " pista" : " pistas")
                       color: Theme.faint; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                TouchButton { Layout.preferredWidth: 110; Layout.preferredHeight: 38; text: "+ Lo visible"; fontSize: 12
                              onClicked: automix.add(library.visiblePaths()) }
                TouchButton { Layout.preferredWidth: 48; Layout.preferredHeight: 38; text: "🔀"; onClicked: automix.shuffle() }
                TouchButton { Layout.preferredWidth: 48; Layout.preferredHeight: 38; text: "🗑"; accent: Theme.red; onClicked: automix.clear() }
            }
            ListView {
                id: q
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: automix.queue
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { width: 8; policy: ScrollBar.AsNeeded }
                delegate: Rectangle {
                    id: qr
                    required property int index
                    required property string title
                    required property string artist
                    required property real bpm
                    required property real duration
                    width: q.width - 10
                    height: 52
                    radius: 10
                    color: index % 2 ? Theme.bg2 : Qt.rgba(1, 1, 1, 0.03)
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 6
                        spacing: 6
                        Text { text: qr.index + 1; color: Theme.faint; font.pixelSize: 12 }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 0
                            Text { Layout.fillWidth: true; elide: Text.ElideRight; text: qr.title; color: Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; elide: Text.ElideRight
                                   text: qr.artist + (qr.bpm > 0 ? "  ·  " + qr.bpm.toFixed(1) + " BPM" : "") + (qr.duration > 0 ? "  ·  " + root.fmt(qr.duration) : "")
                                   color: Theme.dim; font.pixelSize: 11 }
                        }
                        TouchButton { Layout.preferredWidth: 42; Layout.preferredHeight: 40; text: "▲"; fontSize: 12
                                      enabledLook: qr.index > 0; onClicked: automix.move(qr.index, qr.index - 1) }
                        TouchButton { Layout.preferredWidth: 42; Layout.preferredHeight: 40; text: "▼"; fontSize: 12
                                      enabledLook: qr.index < automix.queue.count - 1; onClicked: automix.move(qr.index, qr.index + 1) }
                        TouchButton { Layout.preferredWidth: 42; Layout.preferredHeight: 40; text: "✕"; fontSize: 13; accent: Theme.red
                                      onClicked: automix.removeAt(qr.index) }
                    }
                }
                Text {
                    anchors.centerIn: parent; width: parent.width - 30
                    visible: q.count === 0
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    text: "La cola está vacía.\nEn 🎵 Música toca «+» en una canción, o usa «+ Lo visible»."
                    color: Theme.faint; font.pixelSize: 14
                }
            }
        }
    }
}
