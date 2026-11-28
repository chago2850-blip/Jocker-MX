// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JockerMX

// Karaoke táctil: vista previa, tono grande, siguiente cantante, pantalla del público y rotación.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property bool wide: width > 640
    readonly property var kd: Stage.karaokeDeck() || (karaoke.karaokeDeck === 1 ? deckB : deckA)
    property int renameRow: -1

    JxPrompt {
        id: renamePrompt
        onAccepted: function(name) { karaoke.setSingerName(root.renameRow, name) }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: root.wide ? 2 : 1
        columnSpacing: 10; rowSpacing: 8

        ColumnLayout {
            Layout.preferredWidth: root.wide ? root.width * 0.45 : -1
            Layout.fillWidth: !root.wide
            Layout.fillHeight: true
            spacing: 6
            KaraokeView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 110
                radius: 10
                deck: root.kd
                scale: 0.9
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 6
                TouchButton { Layout.preferredWidth: 52; Layout.preferredHeight: 46; text: "−"; fontSize: 22; onClicked: root.kd.keyShift = root.kd.keyShift - 1 }
                TouchButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 46
                    text: "TONO " + (root.kd.keyShift > 0 ? "+" : "") + root.kd.keyShift
                    sub: "deck " + root.kd.name + " · toca para 0"
                    fontSize: 15; accent: Theme.gold; active: root.kd.keyShift !== 0
                    onClicked: root.kd.keyShift = 0
                }
                TouchButton { Layout.preferredWidth: 52; Layout.preferredHeight: 46; text: "+"; fontSize: 22; onClicked: root.kd.keyShift = root.kd.keyShift + 1 }
                TouchButton { Layout.preferredWidth: 60; Layout.preferredHeight: 46; text: "📺"; fontSize: 20; accent: Theme.cyan
                              active: Stage.audienceVisible; onClicked: Stage.audienceVisible = !Stage.audienceVisible }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            TouchButton {
                Layout.fillWidth: true; Layout.preferredHeight: 54
                text: karaoke.singers.length ? "🎤 SIGUIENTE: " + karaoke.singers[0].name.toUpperCase() : "🎤 SIN CANTANTES"
                sub: karaoke.currentName.length ? "Canta ahora: " + karaoke.currentName : "En 🎵 Música mantén pulsada una canción → Karaoke"
                fontSize: 16; accent: Theme.magenta; active: karaoke.singers.length > 0
                enabledLook: karaoke.singers.length > 0
                onClicked: karaoke.startNext()
            }
            ListView {
                id: sl
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: karaoke.singers
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { width: 8; policy: ScrollBar.AsNeeded }
                delegate: Rectangle {
                    id: sr
                    required property int index
                    required property var modelData
                    width: sl.width - 10
                    height: 52
                    radius: 10
                    color: index === 0 ? Qt.rgba(1, 0.18, 0.55, 0.12) : (index % 2 ? Theme.bg2 : Qt.rgba(1, 1, 1, 0.03))
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 6
                        spacing: 6
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 0
                            Text { Layout.fillWidth: true; elide: Text.ElideRight; text: sr.modelData.name
                                   color: sr.index === 0 ? Theme.magenta : Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                                   MouseArea { anchors.fill: parent; onPressAndHold: { root.renameRow = sr.index; renamePrompt.ask("Cambiar nombre", sr.modelData.name, "Guardar") } } }
                            Text { Layout.fillWidth: true; elide: Text.ElideRight
                                   text: (sr.modelData.karaoke ? "🎤 " : "⚠ ") + sr.modelData.title; color: Theme.dim; font.pixelSize: 11 }
                        }
                        TouchButton { Layout.preferredWidth: 38; Layout.preferredHeight: 40; text: "−"; fontSize: 16
                                      onClicked: karaoke.setSingerKey(sr.index, sr.modelData.key - 1) }
                        Text { Layout.preferredWidth: 26; horizontalAlignment: Text.AlignHCenter
                               text: (sr.modelData.key > 0 ? "+" : "") + sr.modelData.key
                               color: sr.modelData.key !== 0 ? Theme.gold : Theme.dim; font.pixelSize: 13; font.weight: Font.Bold }
                        TouchButton { Layout.preferredWidth: 38; Layout.preferredHeight: 40; text: "+"; fontSize: 16
                                      onClicked: karaoke.setSingerKey(sr.index, sr.modelData.key + 1) }
                        TouchButton { Layout.preferredWidth: 38; Layout.preferredHeight: 40; text: "▲"; fontSize: 11
                                      enabledLook: sr.index > 0; onClicked: karaoke.moveSinger(sr.index, sr.index - 1) }
                        TouchButton { Layout.preferredWidth: 38; Layout.preferredHeight: 40; text: "✕"; fontSize: 12; accent: Theme.red
                                      onClicked: karaoke.removeSinger(sr.index) }
                    }
                }
                Text {
                    anchors.centerIn: parent; width: parent.width - 30
                    visible: sl.count === 0
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    text: "Para agregar un cantante: en 🎵 Música mantén pulsada su canción y elige «🎤 Karaoke»."
                    color: Theme.faint; font.pixelSize: 14
                }
            }
        }
    }
}
