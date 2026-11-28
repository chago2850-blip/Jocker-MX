// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JockerMX

// Interfaz de escritorio (ratón y teclado): decks completos, mezclador y paneles inferiores.
Item {
    id: desktop
    signal openMidi()

    // En pantallas pequeñas (teléfonos) la interfaz completa se desplaza verticalmente
    Flickable {
        id: scroller
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, 980)
        interactive: contentHeight > height + 1
        boundsBehavior: Flickable.StopAtBounds
        clip: true

    ColumnLayout {
        width: scroller.width
        height: scroller.contentHeight
        spacing: 0

        TopBar { Layout.fillWidth: true; onOpenMidi: desktop.openMidi() }

        // Formas de onda desplazables (A arriba, B abajo)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 128
            Layout.margins: 10
            Layout.bottomMargin: 0
            radius: 12
            color: Theme.bg2
            border.color: Theme.border
            clip: true
            Column {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4
                Repeater {
                    model: [deckA, deckB]
                    Item {
                        width: parent.width
                        height: (parent.height - 4) / 2
                        WaveformItem {
                            id: wf
                            anchors.fill: parent
                            deck: modelData
                            scrolling: true
                            accent: Theme.deckColor(index)
                            secondsVisible: zoomA.value
                        }
                        Text {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 4
                            text: modelData.name + (modelData.loaded ? "  ·  " + modelData.title : "")
                            color: Theme.deckColor(index); font.pixelSize: 11; font.weight: Font.Bold
                            style: Text.Outline; styleColor: "#000000"
                        }
                        MouseArea {   // arrastrar la forma de onda = scratch / búsqueda fina
                            anchors.fill: parent
                            property real lastX: 0
                            property real lastT: 0
                            onPressed: function(m) { lastX = m.x; lastT = Date.now(); modelData.scratchBegin() }
                            onPositionChanged: function(m) {
                                var now = Date.now(); var dt = Math.max(8, now - lastT) / 1000
                                var secs = -(m.x - lastX) / width * wf.secondsVisible
                                modelData.scratchMove(secs / dt)
                                lastX = m.x; lastT = now
                                stopTimer.restart()
                            }
                            onReleased: modelData.scratchEnd()
                            onWheel: function(w) { zoomA.value = Math.max(2, Math.min(30, zoomA.value * (w.angleDelta.y > 0 ? 0.85 : 1.18))) }
                            Timer { id: stopTimer; interval: 60; onTriggered: if (parent.pressed) modelData.scratchMove(0) }
                        }
                    }
                }
            }
            QtObject { id: zoomA; property real value: 8 }
        }

        // Decks + mezclador
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 526
            Layout.maximumHeight: 526
            Layout.minimumHeight: 526
            Layout.margins: 10
            spacing: 10
            DeckPanel { deck: deckA; Layout.fillWidth: true; Layout.fillHeight: true }
            MixerPanel { Layout.preferredWidth: 300; Layout.fillHeight: true }
            DeckPanel { deck: deckB; mirrored: true; Layout.fillWidth: true; Layout.fillHeight: true }
        }

        // Pestañas inferiores: biblioteca / sampler / automix
        Row {
            Layout.leftMargin: 14
            spacing: 6
            Repeater {
                model: ["🎵  Biblioteca", "🥁  Sampler", "🔁  Automix", "🎤  Karaoke", "🎬  Video", "📡  En vivo", "📻  Radio"]
                Rectangle {
                    width: 118; height: 26; radius: 6
                    color: bottomTabs.currentIndex === index ? Theme.panel : "transparent"
                    border.color: bottomTabs.currentIndex === index ? Theme.gold : Theme.border
                    Text { anchors.centerIn: parent; text: modelData; font.pixelSize: 12; font.weight: Font.DemiBold
                           color: bottomTabs.currentIndex === index ? Theme.gold : Theme.dim }
                    MouseArea { anchors.fill: parent; onClicked: bottomTabs.currentIndex = index }
                }
            }
        }
        StackLayout {
            id: bottomTabs
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            Layout.topMargin: 4
            currentIndex: startupPanel === "sampler" ? 1 : (startupPanel === "automix" || startupPanel === "programacion" ? 2 : (startupPanel === "karaoke" ? 3 : (startupPanel === "video" ? 4 : (startupPanel === "envivo" ? 5 : (startupPanel === "radio" ? 6 : 0)))))
            LibraryPanel {}
            SamplerPanel {}
            AutomixPanel {}
            KaraokePanel {}
            VideoPanel {}
            BroadcastPanel {}
            RadioPanel {}
        }
    }
    }

}
