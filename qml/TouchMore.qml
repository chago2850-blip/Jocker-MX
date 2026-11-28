// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// «Más»: video, transmisión en vivo, grabación, MIDI, pantalla del público y modo de interfaz.
Item {
    id: root
    property int sub: 0          // 0 menú, 1 video, 2 en vivo, 3 radio local
    signal openMidi()

    function back() { if (sub !== 0) { sub = 0; return true } return false }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.sub
        Rectangle {
            color: Theme.panel; radius: 12; border.color: Theme.border
            GridLayout {
                id: grid
                anchors.fill: parent
                anchors.margins: 12
                columns: width > 700 ? 4 : (width > 420 ? 3 : 2)
                rowSpacing: 10; columnSpacing: 10
                component Tile: TouchButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 64
                    fontSize: 17
                }
                Tile { text: "🎬  Video"; sub: deckA.hasVideo || deckB.hasVideo ? "hay video cargado" : "mezcla de video"; onClicked: root.sub = 1 }
                Tile { text: "📡  En vivo"; sub: broadcast.onAir ? broadcast.stateText : "Icecast / SHOUTcast"
                       accent: Theme.red; active: broadcast.onAir; onClicked: root.sub = 2 }
                Tile { text: "📻  Radio local"; sub: radio.running ? radio.listeners + " escuchando" : "por la Wi-Fi, con QR"
                       accent: Theme.green; active: radio.running; onClicked: root.sub = 3 }
                Tile { text: recorder.recording ? "⏹  Detener" : "⏺  Grabar"
                       sub: recorder.recording ? Theme.formatTime(recorder.seconds).slice(0, 5) : "la mezcla en WAV"
                       accent: Theme.red; active: recorder.recording; onClicked: recorder.toggle() }
                Tile { text: "📺  Pantalla"; sub: Stage.audienceVisible ? "para el público: abierta" : "para el público"
                       accent: Theme.cyan; active: Stage.audienceVisible; onClicked: Stage.audienceVisible = !Stage.audienceVisible }
                Tile { text: "🎹  MIDI"; sub: "controladores"; onClicked: root.openMidi() }
                Tile { text: "🖥  Escritorio"; sub: "interfaz completa"; onClicked: uiPrefs.touch = false }
                Tile { text: "Jocker-MX"; sub: appVersion + " · GPL 3"; enabledLook: false }
            }
        }
        VideoPanel {}
        BroadcastPanel {}
        RadioPanel {}
    }
}
