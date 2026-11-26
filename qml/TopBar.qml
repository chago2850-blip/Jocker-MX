// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    signal openMidi()
    implicitHeight: 48
    color: Theme.bg2
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 14
        spacing: 12
        Image { source: "qrc:/JockerMX/resources/logo-3d-256.png"; Layout.preferredWidth: 40; Layout.preferredHeight: 40; smooth: true; mipmap: true }
        Row {
            spacing: 0
            Text { text: "JOCKER"; color: Theme.gold; font.pixelSize: 20; font.weight: Font.Black; font.letterSpacing: 1 }
            Text { text: "-MX"; color: Theme.magenta; font.pixelSize: 20; font.weight: Font.Black; font.letterSpacing: 1 }
        }
        Text { text: appVersion; color: Theme.faint; font.pixelSize: 11; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 12 }
        Item { Layout.fillWidth: true }

        Rectangle {     // Grabación
            Layout.preferredHeight: 28; Layout.preferredWidth: recTxt.implicitWidth + 34
            radius: 6
            color: recorder.recording ? Qt.rgba(1, 0.3, 0.37, 0.18) : (recMa.containsMouse ? Theme.panel2 : "transparent")
            border.color: recorder.recording ? Theme.red : Theme.border
            Rectangle {
                id: recDot
                width: 10; height: 10; radius: 5
                anchors.left: parent.left; anchors.leftMargin: 9; anchors.verticalCenter: parent.verticalCenter
                color: Theme.red
                opacity: recorder.recording ? 1 : 0.45
                SequentialAnimation on opacity {
                    running: recorder.recording; loops: Animation.Infinite
                    NumberAnimation { to: 0.25; duration: 500 } NumberAnimation { to: 1; duration: 500 }
                }
            }
            Text {
                id: recTxt
                anchors.left: recDot.right; anchors.leftMargin: 7; anchors.verticalCenter: parent.verticalCenter
                text: recorder.recording ? ("REC  " + Theme.formatTime(recorder.seconds).slice(0, 5) + (recorder.trackCount ? "  ·  " + recorder.trackCount + " pistas" : "")) : "REC"
                color: recorder.recording ? Theme.red : Theme.dim
                font.pixelSize: 11; font.weight: Font.Bold
            }
            MouseArea { id: recMa; anchors.fill: parent; hoverEnabled: true; onClicked: recorder.toggle() }
        }
        Rectangle {     // MIDI: dispositivos y actividad
            id: midiBtn
            Layout.preferredHeight: 28; Layout.preferredWidth: midiTxt.implicitWidth + 30
            radius: 6
            color: midiMa.containsMouse ? Theme.panel2 : "transparent"
            border.color: midi.devices.length ? Theme.green : Theme.border
            Rectangle {
                id: led
                width: 8; height: 8; radius: 4
                anchors.left: parent.left; anchors.leftMargin: 9; anchors.verticalCenter: parent.verticalCenter
                color: Theme.green; opacity: 0.25
                Connections { target: midi; function onActivity() { flash.restart() } }
                SequentialAnimation { id: flash
                    PropertyAction { target: led; property: "opacity"; value: 1 }
                    NumberAnimation { target: led; property: "opacity"; to: 0.25; duration: 250 } }
            }
            Text { id: midiTxt; anchors.left: led.right; anchors.leftMargin: 7; anchors.verticalCenter: parent.verticalCenter
                   text: "MIDI" + (midi.devices.length ? " · " + midi.devices.length : ""); color: midi.devices.length ? Theme.green : Theme.dim
                   font.pixelSize: 11; font.weight: Font.Bold }
            MouseArea { id: midiMa; anchors.fill: parent; hoverEnabled: true; onClicked: root.openMidi() }
        }
        Rectangle {
            visible: mixer.noAudioDevice
            radius: 6; color: Qt.rgba(1, 0.3, 0.37, 0.15); border.color: Theme.red
            Layout.preferredHeight: 26; Layout.preferredWidth: noDev.implicitWidth + 20
            Text { id: noDev; anchors.centerIn: parent; text: "Sin dispositivo de audio"; color: Theme.red; font.pixelSize: 11; font.weight: Font.DemiBold }
        }
        Text {
            text: (mixer.noAudioDevice ? "" : mixer.backend + " · ") + (mixer.sampleRate / 1000).toFixed(1) + " kHz · " + mixer.latencyMs.toFixed(1) + " ms · " + mixer.outputChannels + " canales" + (mixer.headphonesAvailable ? " · 🎧" : "")
            color: Theme.dim; font.pixelSize: 11
        }
        Rectangle { width: 1; Layout.preferredHeight: 24; color: Theme.border }
        Column {     // VU maestro horizontal compacto
            spacing: 3
            Repeater {
                model: 2
                Item {
                    width: 120; height: 6
                    readonly property real lvl: index === 0 ? mixer.levelL : mixer.levelR
                    readonly property real db: lvl > 0.00001 ? 20 * Math.log(lvl) / Math.LN10 : -90
                    Rectangle { anchors.fill: parent; radius: 3; color: Theme.raised }
                    Rectangle {
                        height: parent.height; radius: 3
                        width: parent.width * Math.max(0, Math.min(1, (parent.db + 48) / 48))
                        gradient: Gradient { orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: Theme.green } GradientStop { position: 0.8; color: Theme.gold } GradientStop { position: 1; color: Theme.red } }
                    }
                }
            }
        }
        Text {
            id: clock
            color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold
            Timer { interval: 1000; running: true; repeat: true; triggeredOnStart: true
                    onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm") }
        }
    }
}
