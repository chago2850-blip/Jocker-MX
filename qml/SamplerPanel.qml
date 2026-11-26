// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

// Sampler de 8 pads: clic = disparar; modo ONE / LOOP / HOLD; carga de sonidos propios (clic derecho o arrastrar).
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    property int loadSlot: -1
    readonly property var modeNames: ["ONE", "LOOP", "HOLD"]
    readonly property var padColors: ["#ff4d5e", "#ff9a3c", "#f5b942", "#55e08a", "#3fe0ff", "#5a6eff", "#be5aff", "#ff2d8c"]

    FileDialog {
        id: fileDialog
        title: "Elige un sonido para el pad"
        nameFilters: ["Audio (*.wav *.mp3 *.flac *.ogg)", "Todos (*)"]
        onAccepted: sampler.loadFile(root.loadSlot, selectedFile.toString())
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 14

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 4000
            columns: 4
            rowSpacing: 10; columnSpacing: 10
            Repeater {
                model: 8
                Rectangle {
                    id: pad
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 60
                    radius: 12
                    readonly property color c: root.padColors[index]
                    readonly property bool on: sampler.playing[index] === true
                    color: padArea.pressed || on ? Qt.rgba(c.r, c.g, c.b, 0.35) : Qt.rgba(c.r, c.g, c.b, 0.10)
                    border.color: on ? Qt.lighter(c, 1.3) : Qt.rgba(c.r, c.g, c.b, 0.6)
                    border.width: on ? 2 : 1
                    Behavior on color { ColorAnimation { duration: 80 } }

                    Rectangle {    // progreso
                        visible: pad.on
                        anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 6
                        height: 3; radius: 2
                        width: (parent.width - 12) * (sampler.progress[index] || 0)
                        color: Qt.lighter(pad.c, 1.3)
                    }
                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: index + 1; color: Qt.lighter(pad.c, 1.2); font.pixelSize: 11; font.weight: Font.Black }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: sampler.names[index] || "—"; color: Theme.text; font.pixelSize: 14; font.weight: Font.Bold }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; visible: sampler.custom[index] === true; text: "propio"; color: Theme.faint; font.pixelSize: 9 }
                    }
                    DropArea {
                        anchors.fill: parent
                        onDropped: function(drop) {
                            if (drop.hasUrls && drop.urls.length) sampler.loadFile(index, drop.urls[0].toString())
                            else if (drop.hasText) sampler.loadFile(index, drop.text)
                        }
                    }
                    MouseArea {
                        id: padArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onPressed: function(m) { if (m.button === Qt.LeftButton) sampler.press(index) }
                        onReleased: function(m) { if (m.button === Qt.LeftButton) sampler.release(index) }
                        onClicked: function(m) { if (m.button === Qt.RightButton) { root.loadSlot = index; fileDialog.open() } }
                    }
                    Row {   // modo y restaurar
                        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 6
                        spacing: 4
                        JxButton { width: 46; height: 20; fontSize: 9; text: root.modeNames[sampler.modes[index] || 0]
                                   accent: pad.c; active: (sampler.modes[index] || 0) !== 0; onClicked: sampler.cycleMode(index) }
                        JxButton { visible: sampler.custom[index] === true; width: 20; height: 20; fontSize: 9; text: "↺"; onClicked: sampler.resetSlot(index) }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 170
            Layout.maximumWidth: 170
            Layout.fillHeight: true
            spacing: 8
            Text { text: "SAMPLER"; color: Theme.gold; font.pixelSize: 11; font.weight: Font.Black; font.letterSpacing: 1.5 }
            Knob { Layout.alignment: Qt.AlignHCenter; label: "VOLUMEN"; from: 0; to: 1; defaultValue: 0.8; bipolar: false
                   accent: Theme.gold; size: 44; value: sampler.volume; onMoved: function(v) { sampler.volume = v } }
            JxButton { Layout.fillWidth: true; text: "🎧  Audífonos"; accent: Theme.orange; active: sampler.pfl; onClicked: sampler.pfl = !sampler.pfl }
            JxButton { Layout.fillWidth: true; text: "■  Detener todo"; accent: Theme.red; onClicked: sampler.stopAll() }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: sampler.error.length ? sampler.error : "Clic derecho en un pad o arrastra un archivo para usar tu propio sonido. Teclas Z X C V B N M ,"
                color: sampler.error.length ? Theme.red : Theme.faint
                font.pixelSize: 10
            }
            Item { Layout.fillHeight: true }
        }
    }
}
