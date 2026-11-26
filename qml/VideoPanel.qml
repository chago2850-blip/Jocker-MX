// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Layouts

// Video: mezcla de los dos decks según el crossfader, vistas de cada deck y salida al público.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property bool anyVideo: deckA.hasVideo || deckB.hasVideo
    readonly property var dA: deckA      // (dentro de VideoMixItem «deckA» sería su propia propiedad)
    readonly property var dB: deckB

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---------- Mezcla (lo que verá el público)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                Text { text: "MEZCLA DE VIDEO"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                Text { text: root.anyVideo ? "sigue al crossfader" : ""; color: Theme.dim; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                JxButton { Layout.preferredWidth: 180; Layout.preferredHeight: 26; fontSize: 11; accent: Theme.cyan
                           text: Stage.audienceVisible ? "📺  Cerrar pantalla público" : "📺  Pantalla del público"
                           active: Stage.audienceVisible; onClicked: Stage.audienceVisible = !Stage.audienceVisible }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: "black"
                border.color: Theme.border
                clip: true
                Item {   // 16:9 centrado
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 8, (parent.height - 8) * 16 / 9)
                    height: width * 9 / 16
                    VideoMixItem {
                        anchors.fill: parent
                        deckA: root.dA
                        deckB: root.dB
                        crossfader: mixer.crossfader
                        transition: Stage.videoTransition
                        fill: Stage.videoFill
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: !root.anyVideo
                    horizontalAlignment: Text.AlignHCenter
                    text: "Carga un video (MP4, MOV, MKV, WebM…) en un deck.\nLa imagen sigue al audio: tempo, pausa, cue, búsquedas y scratch."
                    color: Theme.faint; font.pixelSize: 13
                }
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }

        // ---------- Decks y opciones
        ColumnLayout {
            Layout.preferredWidth: 300
            Layout.maximumWidth: 300
            Layout.fillHeight: true
            spacing: 6
            Repeater {
                model: [deckA, deckB]
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300 * 9 / 16 * 0.62
                    radius: 6
                    color: "black"
                    border.color: modelData.hasVideo ? Theme.deckColor(index) : Theme.border
                    clip: true
                    VideoItem { anchors.fill: parent; anchors.margins: 1; deck: modelData; fill: Stage.videoFill }
                    Rectangle {
                        x: 6; y: 6; width: dl.implicitWidth + 10; height: 18; radius: 4
                        color: Qt.rgba(0, 0, 0, 0.6)
                        Text { id: dl; anchors.centerIn: parent; text: modelData.name + (modelData.hasVideo ? "" : " · sólo audio")
                               color: Theme.deckColor(index); font.pixelSize: 10; font.weight: Font.Bold }
                    }
                }
            }
            Text { text: "TRANSICIÓN"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2; Layout.topMargin: 2 }
            Row {
                spacing: 4
                Repeater {
                    model: Stage.transitionNames
                    JxButton { width: 96; height: 26; fontSize: 11; text: modelData; accent: Theme.magenta
                               active: Stage.videoTransition === index; onClicked: Stage.videoTransition = index }
                }
            }
            Row {
                spacing: 4
                JxButton { width: 146; height: 26; fontSize: 11; text: "Llenar pantalla"; accent: Theme.gold
                           active: Stage.videoFill; onClicked: Stage.videoFill = true }
                JxButton { width: 146; height: 26; fontSize: 11; text: "Ajustar (bandas)"; accent: Theme.gold
                           active: !Stage.videoFill; onClicked: Stage.videoFill = false }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
