// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// Panel de un deck: información, vista general, plato, pitch, transporte, hot cues y loops.
Rectangle {
    id: root
    property var deck
    property bool mirrored: false
    readonly property color accent: Theme.deckColor(deck ? deck.index : 0)

    color: Theme.panel
    radius: 12
    border.color: dropArea.containsDrag ? accent : Theme.border
    border.width: dropArea.containsDrag ? 2 : 1

    DropArea {
        id: dropArea
        anchors.fill: parent
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) root.deck.load(drop.urls[0].toString())
            else if (drop.hasText) root.deck.load(drop.text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ---------- Encabezado
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Rectangle {
                width: 38; height: 38; radius: 8
                color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.18)
                border.color: root.accent
                Text { anchors.centerIn: parent; text: root.deck ? root.deck.name : ""; color: root.accent; font.pixelSize: 20; font.weight: Font.Black }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: root.deck.loading ? "Analizando…" : (root.deck.loaded ? root.deck.title : "Arrastra una pista aquí")
                    color: root.deck.loaded ? Theme.text : Theme.faint
                    font.pixelSize: 16; font.weight: Font.Bold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: root.deck.error.length ? root.deck.error : (root.deck.artist.length ? root.deck.artist : (root.deck.loaded ? "Artista desconocido" : "o usa los botones A / B del navegador"))
                    color: root.deck.error.length ? Theme.red : Theme.dim
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
            Rectangle {     // tonalidad (Camelot), verde si combina con el otro deck
                visible: root.deck.musicalKey.length > 0
                readonly property var other: root.deck.index === 0 ? deckB : deckA
                readonly property bool compatible: other.loaded && other.keyCompatibleWith(root.deck.musicalKey)
                width: keyTxt.implicitWidth + 16; height: 38; radius: 8
                color: compatible ? Qt.rgba(0.33, 0.88, 0.54, 0.15) : Theme.panel2
                border.color: compatible ? Theme.green : Theme.border
                Column {
                    anchors.centerIn: parent
                    Text { id: keyTxt; anchors.horizontalCenter: parent.horizontalCenter; text: root.deck.musicalKey
                           color: parent.parent.compatible ? Theme.green : Theme.cyan; font.pixelSize: 15; font.weight: Font.Black }
                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.deck.keyName; color: Theme.dim; font.pixelSize: 9 }
                }
            }
            ColumnLayout {
                spacing: 0
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.deck.bpm > 0 ? root.deck.bpm.toFixed(2) : "--.--"
                    color: Theme.text; font.pixelSize: 22; font.weight: Font.Bold
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: "BPM  " + (root.deck.pitchPercent >= 0 ? "+" : "") + root.deck.pitchPercent.toFixed(2) + " %"
                    color: Math.abs(root.deck.pitchPercent) < 0.005 ? Theme.dim : Theme.gold
                    font.pixelSize: 11; font.weight: Font.DemiBold
                }
            }
            Grid {      // herramientas de beatgrid
                columns: 3
                spacing: 3
                visible: root.deck.loaded
                JxButton { width: 30; height: 18; text: "½"; fontSize: 10; onClicked: root.deck.bpmHalve() }
                JxButton { width: 30; height: 18; text: "×2"; fontSize: 10; onClicked: root.deck.bpmDouble() }
                JxButton { width: 30; height: 18; text: "TAP"; fontSize: 8; onClicked: root.deck.tapBpm() }
                JxButton { width: 30; height: 18; text: "◂"; fontSize: 10; onClicked: root.deck.gridShift(-10) }
                JxButton { width: 30; height: 18; text: "⌖"; fontSize: 10; onClicked: root.deck.gridSetBeatHere() }
                JxButton { width: 30; height: 18; text: "▸"; fontSize: 10; onClicked: root.deck.gridShift(10) }
            }
        }

        // ---------- Tiempo + indicador de beat
        RowLayout {
            Layout.fillWidth: true
            Text { text: Theme.formatTime(root.deck.position); color: Theme.text; font.pixelSize: 13; font.family: "monospace" }
            Item { Layout.fillWidth: true
                Row {
                    anchors.centerIn: parent
                    spacing: 5
                    Repeater {
                        model: 4
                        Rectangle {
                            width: 18; height: 5; radius: 2
                            readonly property bool lit: root.deck.trackBpm > 0 && ((root.deck.beatIndex % 4) + 4) % 4 === index
                            color: lit ? (index === 0 ? root.accent : Theme.gold) : Theme.raised
                            opacity: lit && root.deck.playing ? (1.0 - root.deck.beatPhase * 0.6) : 1.0
                        }
                    }
                }
            }
            Text { text: "-" + Theme.formatTime(root.deck.duration - root.deck.position); color: (root.deck.loaded && root.deck.duration - root.deck.position < 30) ? Theme.red : Theme.dim; font.pixelSize: 13; font.family: "monospace" }
        }

        // ---------- Vista general
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            radius: 6
            color: Theme.bg2
            border.color: Theme.border
            WaveformItem {
                anchors.fill: parent; anchors.margins: 2
                deck: root.deck
                accent: root.accent
            }
            MouseArea {
                anchors.fill: parent
                onPressed: function(m) { root.deck.seekFraction(m.x / width) }
                onPositionChanged: function(m) { if (pressed) root.deck.seekFraction(m.x / width) }
            }
        }

        // ---------- Plato + pitch + controles
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            layoutDirection: root.mirrored ? Qt.RightToLeft : Qt.LeftToRight

            ColumnLayout {   // transporte
                spacing: 8
                Layout.alignment: Qt.AlignBottom
                JxButton { text: "CUE"; Layout.preferredWidth: 70; Layout.preferredHeight: 46; accent: Theme.gold; fontSize: 14
                           active: root.deck.loaded && !root.deck.playing
                           onPressed: root.deck.cueDown(); onReleased: root.deck.cueUp() }
                JxButton { text: root.deck.playing ? "❚❚" : "▶"; Layout.preferredWidth: 70; Layout.preferredHeight: 46; accent: Theme.green; fontSize: 18
                           active: root.deck.playing; onClicked: root.deck.playPause() }
            }
            JogWheel {
                deck: root.deck
                accent: root.accent
                Layout.preferredWidth: Math.min(230, parent.height)
                Layout.preferredHeight: Layout.preferredWidth
                Layout.alignment: Qt.AlignVCenter
            }
            ColumnLayout {
                spacing: 6
                Layout.alignment: Qt.AlignTop
                JxButton { text: "SYNC"; Layout.preferredWidth: 70; accent: root.accent; active: root.deck.syncLock
                           onClicked: root.deck.sync(); onRightClicked: root.deck.syncLock = !root.deck.syncLock }
                JxButton { text: "LOCK"; Layout.preferredWidth: 70; accent: root.accent; active: root.deck.syncLock
                           onClicked: root.deck.syncLock = !root.deck.syncLock }
                JxButton { text: "KEY"; Layout.preferredWidth: 70; accent: Theme.cyan; active: root.deck.keyLock
                           onClicked: root.deck.keyLock = !root.deck.keyLock }
                JxButton { text: "Q"; Layout.preferredWidth: 70; accent: Theme.green; active: root.deck.quantize
                           onClicked: root.deck.quantize = !root.deck.quantize }
                JxButton { text: "±" + Math.round(root.deck.pitchRange * 100) + "%"; Layout.preferredWidth: 70; bold: false
                           onClicked: {
                               var r = [0.08, 0.16, 0.25, 0.5, 1.0]
                               var i = r.indexOf(root.deck.pitchRange)
                               root.deck.pitchRange = r[(i + 1) % r.length]
                           } }
            }
            Item { Layout.fillWidth: true }
            ColumnLayout {
                spacing: 4
                Layout.fillHeight: true
                PitchFader {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 34
                    accent: root.accent
                    value: root.deck.pitch
                    onMoved: function(v) { root.deck.syncLock = false; root.deck.pitch = v }
                }
                Row {
                    spacing: 2
                    JxButton { text: "−"; width: 16; height: 20; fontSize: 11; onPressed: root.deck.nudge(-0.04); onReleased: root.deck.nudge(0) }
                    JxButton { text: "+"; width: 16; height: 20; fontSize: 11; onPressed: root.deck.nudge(0.04); onReleased: root.deck.nudge(0) }
                }
            }
        }

        // ---------- Hot cues
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "HOT CUE"; color: Theme.faint; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1; Layout.preferredWidth: 46 }
            Repeater {
                model: 8
                JxButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    readonly property bool isSet: root.deck.hotCues.length > index && root.deck.hotCues[index] >= 0
                    text: (index + 1)
                    accent: Theme.hotColors[index]
                    active: isSet
                    onClicked: root.deck.hotCue(index)
                    onRightClicked: root.deck.clearHotCue(index)
                }
            }
        }

        // ---------- Loops y salto de beats
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "LOOP"; color: Theme.faint; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1; Layout.preferredWidth: 46 }
            Repeater {
                model: [0.5, 1, 2, 4, 8, 16]
                JxButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    text: modelData === 0.5 ? "½" : modelData
                    accent: Theme.green
                    active: root.deck.loopActive && Math.abs(root.deck.loopBeats - modelData) < 0.001
                    onClicked: root.deck.autoLoop(modelData)
                }
            }
            JxButton { Layout.preferredWidth: 34; Layout.preferredHeight: 28; text: "÷2"; bold: false; onClicked: root.deck.loopHalve() }
            JxButton { Layout.preferredWidth: 34; Layout.preferredHeight: 28; text: "×2"; bold: false; onClicked: root.deck.loopDouble() }
            JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 28; text: "◀"; bold: false; onClicked: root.deck.beatJump(-4) }
            JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 28; text: "▶"; bold: false; onClicked: root.deck.beatJump(4) }
        }

        // ---------- Efectos sincronizados al beat
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "FX"; color: Theme.faint; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1; Layout.preferredWidth: 46 }
            Repeater {
                model: 6
                JxButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    text: root.deck.fxNames[index + 1]
                    fontSize: 10
                    accent: Theme.orange
                    active: root.deck.fxType === index + 1
                    onClicked: root.deck.toggleFx(index + 1)
                }
            }
            JxButton {
                Layout.preferredWidth: 44; Layout.preferredHeight: 28
                readonly property var steps: [0.25, 0.5, 1, 2, 4, 8]
                text: root.deck.fxBeats < 1 ? (root.deck.fxBeats === 0.25 ? "¼" : "½") : root.deck.fxBeats
                accent: Theme.orange; bold: true
                onClicked: { var i = steps.indexOf(root.deck.fxBeats); root.deck.fxBeats = steps[(i + 1) % steps.length] }
                onRightClicked: { var i = steps.indexOf(root.deck.fxBeats); root.deck.fxBeats = steps[(i + steps.length - 1) % steps.length] }
            }
            Knob { size: 28; from: 0; to: 1; defaultValue: 0.5; bipolar: false; accent: Theme.orange
                   value: root.deck.fxWet; onMoved: function(v) { root.deck.fxWet = v } }
        }
    }
}
