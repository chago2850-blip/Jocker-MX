// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// Sampler táctil: 8 pads grandes. Tocar = disparar (en modo HOLD suena mientras se mantiene);
// mantener pulsado un pad cambia su modo (ONE → LOOP → HOLD).
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property var modeNames: ["UNA VEZ", "BUCLE", "MANTENER"]
    readonly property var colors: ["#ff2d8c", "#f5b942", "#3fe0ff", "#55e08a", "#a77bff", "#ff7a45", "#2ee6c5", "#ffd84d"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text { text: "PADS"; color: Theme.faint; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1.5 }
            Text { Layout.fillWidth: true; elide: Text.ElideRight; text: "Mantén pulsado un pad para cambiar su modo"; color: Theme.dim; font.pixelSize: 12 }
            TouchFader {
                Layout.preferredWidth: Math.min(260, root.width * 0.35); Layout.preferredHeight: 40
                vertical: false; from: 0; to: 1; defaultValue: 0.8
                value: sampler.volume; accent: Theme.gold; label: "VOLUMEN"
                onMoved: function(v) { sampler.volume = v }
            }
            TouchButton { Layout.preferredWidth: 96; Layout.preferredHeight: 40; text: "■ PARAR"; fontSize: 12; accent: Theme.red
                          onClicked: sampler.stopAll() }
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: root.width >= root.height ? 4 : 2
            rowSpacing: 8; columnSpacing: 8
            Repeater {
                model: 8
                TouchButton {
                    id: pad
                    Layout.fillWidth: true; Layout.fillHeight: true
                    readonly property bool on: sampler.playing.length > index && sampler.playing[index] === true
                    accent: root.colors[index]
                    active: on
                    fontSize: 16
                    text: sampler.names.length > index ? sampler.names[index] : ""
                    sub: root.modeNames[sampler.modes[index] || 0]
                    onPressed: sampler.press(index)
                    onReleased: sampler.release(index)
                    onLongPressed: sampler.cycleMode(index)
                    Rectangle {   // avance de reproducción
                        anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 3
                        height: 4; radius: 2
                        width: (parent.width - 6) * (pad.on && sampler.progress.length > index ? sampler.progress[index] : 0)
                        color: pad.accent
                    }
                }
            }
        }
    }
}
