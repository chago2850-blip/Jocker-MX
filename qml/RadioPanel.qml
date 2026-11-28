// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// Radio local: los asistentes escanean el QR con el celular (misma Wi-Fi) y escuchan la mezcla.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property bool wide: width > 560

    function hms(s) {
        s = Math.floor(s)
        const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), x = s % 60
        return (h > 0 ? h + ":" + (m < 10 ? "0" : "") : "") + m + ":" + (x < 10 ? "0" : "") + x
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10
        columns: root.wide ? 2 : 1
        columnSpacing: 14; rowSpacing: 10

        // ---- Controles
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: root.wide
            spacing: 8
            TouchButton {
                Layout.fillWidth: true; Layout.preferredHeight: 56
                text: radio.running ? "📻  RADIO ENCENDIDA" : "📻  ENCENDER RADIO LOCAL"
                sub: radio.running ? (radio.listeners + (radio.listeners === 1 ? " persona escuchando" : " personas escuchando") + " · " + root.hms(radio.seconds))
                                   : "sin internet: por la Wi-Fi del lugar"
                fontSize: 16; accent: Theme.green; active: radio.running
                onClicked: radio.toggle()
            }
            Text { Layout.fillWidth: true; visible: radio.errorText.length > 0; text: radio.errorText; wrapMode: Text.WordWrap
                   color: Theme.gold; font.pixelSize: 12 }
            Text { Layout.fillWidth: true; visible: radio.running && radio.nowPlaying.length > 0; elide: Text.ElideRight
                   text: "♪  " + radio.nowPlaying; color: Theme.cyan; font.pixelSize: 13 }
            RowLayout {
                Layout.fillWidth: true; spacing: 10
                JxField { Layout.fillWidth: true; label: "NOMBRE DE LA RADIO"; text: radio.name
                          onEdited: function(v) { radio.name = v } }
                JxField { Layout.preferredWidth: 90; label: "PUERTO"; text: radio.port; numeric: true; enabledField: !radio.running
                          onEdited: function(v) { radio.port = parseInt(v) || 8000 } }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 6
                Text { text: "CALIDAD"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                Repeater {
                    model: [96, 128, 192, 256]
                    TouchButton { Layout.preferredWidth: 56; Layout.preferredHeight: 34; text: modelData; fontSize: 12; accent: Theme.gold
                                  active: radio.bitrate === modelData; enabledLook: !radio.running
                                  onClicked: if (!radio.running) radio.bitrate = modelData }
                }
                Text { text: "kbps"; color: Theme.dim; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.faint; font.pixelSize: 11
                text: "Quien esté en la misma red Wi-Fi escanea el código con la cámara del celular y toca ▶. "
                      + "También sirve en VLC u otro reproductor con la dirección + «stream.mp3» o «stream.ogg». "
                      + "Si no abre, revisa que el cortafuegos permita el puerto " + radio.port + "."
            }
            Repeater {
                model: radio.urls.length > 1 ? radio.urls.slice(1) : []
                Text { text: "También: " + modelData; color: Theme.dim; font.pixelSize: 11 }
            }
            Item { Layout.fillHeight: true; visible: root.wide }
        }

        // ---- Código QR
        ColumnLayout {
            Layout.fillWidth: !root.wide
            Layout.preferredWidth: root.wide ? Math.min(root.height - 20, root.width * 0.42) : -1
            Layout.fillHeight: true
            spacing: 6
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width, Math.max(120, root.wide ? root.height - 70 : 260))
                Layout.preferredHeight: Layout.preferredWidth
                radius: 12
                color: radio.running ? "white" : Theme.bg2
                border.color: radio.running ? Theme.green : Theme.border
                QrItem {
                    anchors.fill: parent; anchors.margins: 6
                    visible: radio.running
                    text: radio.url
                }
                Text {
                    anchors.centerIn: parent
                    visible: !radio.running
                    horizontalAlignment: Text.AlignHCenter
                    text: "📱\nEnciende la radio\npara ver el código"
                    color: Theme.faint; font.pixelSize: 14
                }
            }
            TextEdit {
                Layout.alignment: Qt.AlignHCenter
                readOnly: true; selectByMouse: true
                text: radio.url
                color: Theme.gold; font.pixelSize: 15; font.weight: Font.Bold
            }
        }
    }
}
