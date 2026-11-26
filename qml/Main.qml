// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JockerMX

ApplicationWindow {
    id: win
    width: 1480
    height: 980
    minimumWidth: 1180
    minimumHeight: 860
    visible: true
    title: "Jocker-MX"
    color: Theme.bg

    // Atajos de teclado (estilo de los programas DJ más usados)
    Shortcut { sequence: "Q"; onActivated: deckA.playPause() }
    Shortcut { sequence: "P"; onActivated: deckB.playPause() }
    Shortcut { sequence: "W"; onActivated: deckA.sync() }
    Shortcut { sequence: "O"; onActivated: deckB.sync() }
    Shortcut { sequence: "1"; onActivated: deckA.hotCue(0) }
    Shortcut { sequence: "2"; onActivated: deckA.hotCue(1) }
    Shortcut { sequence: "3"; onActivated: deckA.hotCue(2) }
    Shortcut { sequence: "4"; onActivated: deckA.hotCue(3) }
    Shortcut { sequence: "7"; onActivated: deckB.hotCue(0) }
    Shortcut { sequence: "8"; onActivated: deckB.hotCue(1) }
    Shortcut { sequence: "9"; onActivated: deckB.hotCue(2) }
    Shortcut { sequence: "0"; onActivated: deckB.hotCue(3) }
    Shortcut { sequence: "A"; onActivated: deckA.autoLoop(4) }
    Shortcut { sequence: "L"; onActivated: deckB.autoLoop(4) }
    Shortcut { sequence: "Z"; onActivated: sampler.press(0) }
    Shortcut { sequence: "X"; onActivated: sampler.press(1) }
    Shortcut { sequence: "C"; onActivated: sampler.press(2) }
    Shortcut { sequence: "V"; onActivated: sampler.press(3) }
    Shortcut { sequence: "B"; onActivated: sampler.press(4) }
    Shortcut { sequence: "N"; onActivated: sampler.press(5) }
    Shortcut { sequence: "M"; onActivated: sampler.press(6) }
    Shortcut { sequence: ","; onActivated: sampler.press(7) }
    Shortcut { sequence: "Ctrl+R"; onActivated: recorder.toggle() }
    Shortcut { sequence: "Left"; onActivated: mixer.crossfader = Math.max(-1, mixer.crossfader - 0.1) }
    Shortcut { sequence: "Right"; onActivated: mixer.crossfader = Math.min(1, mixer.crossfader + 0.1) }

    MidiDialog { id: midiDialog }
    Component.onCompleted: if (startupPanel === "midi") midiDialog.open()

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0; color: "#120b20" }
            GradientStop { position: 1; color: Theme.bg }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TopBar { Layout.fillWidth: true; onOpenMidi: midiDialog.open() }

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
            Layout.preferredHeight: 490
            Layout.maximumHeight: 490
            Layout.minimumHeight: 490
            Layout.margins: 10
            spacing: 10
            DeckPanel { deck: deckA; Layout.fillWidth: true; Layout.fillHeight: true }
            MixerPanel { Layout.preferredWidth: 300; Layout.fillHeight: true }
            DeckPanel { deck: deckB; mirrored: true; Layout.fillWidth: true; Layout.fillHeight: true }
        }

        // Pestañas inferiores: biblioteca / sampler
        Row {
            Layout.leftMargin: 14
            spacing: 6
            Repeater {
                model: ["🎵  Biblioteca", "🥁  Sampler"]
                Rectangle {
                    width: 128; height: 26; radius: 6
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
            currentIndex: startupPanel === "sampler" ? 1 : 0
            LibraryPanel {}
            SamplerPanel {}
        }
    }
}
