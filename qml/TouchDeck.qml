// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// Deck para pantallas táctiles: título, forma de onda (arrastrar = scratch), transporte grande,
// hot cues, loop y tempo. Se compacta solo cuando hay poca altura (teléfono en horizontal).
Rectangle {
    id: root
    property var deck
    property int index: 0
    readonly property color accent: Theme.deckColor(index)
    readonly property bool tight: height < 300
    signal openLibrary()

    radius: 12
    color: Theme.panel
    border.color: deck.playing ? accent : Theme.border

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: root.tight ? 5 : 7

        // ---- Cabecera
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Rectangle {
                Layout.preferredWidth: 30; Layout.preferredHeight: 30; radius: 15
                color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.2); border.color: root.accent
                Text { anchors.centerIn: parent; text: root.deck.name; color: root.accent; font.pixelSize: 15; font.weight: Font.Black }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true; elide: Text.ElideRight
                    text: root.deck.loading ? "Cargando…" : (root.deck.loaded ? root.deck.title : "Sin pista")
                    color: Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                }
                Text {
                    Layout.fillWidth: true; elide: Text.ElideRight
                    text: root.deck.loaded ? (root.deck.artist.length ? root.deck.artist : " ") + (root.deck.keyName.length ? "  ·  " + root.deck.keyName : "")
                                           : "Toca para elegir música"
                    color: Theme.dim; font.pixelSize: 12
                }
            }
            ColumnLayout {
                spacing: 0
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.deck.bpm > 0 ? root.deck.bpm.toFixed(1) : "—"
                    color: Theme.gold; font.pixelSize: 19; font.weight: Font.Black
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.deck.loaded ? "-" + Theme.formatTime(Math.max(0, root.deck.duration - root.deck.position)).slice(0, 5) : ""
                    color: Theme.dim; font.pixelSize: 12; font.family: "monospace"
                }
            }
        }

        // ---- Forma de onda (arrastrar = scratch) y vista general (tocar = ir ahí)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 44
            radius: 8
            color: Theme.bg2
            clip: true
            WaveformItem {
                id: wf
                anchors.fill: parent
                anchors.bottomMargin: 14
                deck: root.deck
                scrolling: true
                accent: root.accent
                secondsVisible: 6
            }
            WaveformItem {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: 12
                deck: root.deck
                scrolling: false
                accent: root.accent
                MouseArea { anchors.fill: parent; onPressed: function(m) { root.deck.seekFraction(m.x / width) } }
            }
            MouseArea {
                anchors.fill: parent
                anchors.bottomMargin: 14
                enabled: root.deck.loaded
                property real lastX: 0
                property real lastT: 0
                onPressed: function(m) { lastX = m.x; lastT = Date.now(); root.deck.scratchBegin() }
                onPositionChanged: function(m) {
                    var now = Date.now(), dt = Math.max(8, now - lastT) / 1000
                    root.deck.scratchMove(-(m.x - lastX) / width * wf.secondsVisible / dt)
                    lastX = m.x; lastT = now
                    hold.restart()
                }
                onReleased: root.deck.scratchEnd()
                onCanceled: root.deck.scratchEnd()
                Timer { id: hold; interval: 60; onTriggered: if (parent.pressed) root.deck.scratchMove(0) }
            }
            TouchButton {
                visible: !root.deck.loaded && !root.deck.loading
                anchors.centerIn: parent
                width: 180; height: 42
                text: "🎵  Elegir música"
                accent: root.accent
                onClicked: root.openLibrary()
            }
        }

        // ---- Transporte
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TouchButton {
                Layout.preferredWidth: 64; Layout.preferredHeight: 48
                text: "CUE"; accent: Theme.gold
                onPressed: root.deck.cueDown()
                onReleased: root.deck.cueUp()
            }
            TouchButton {
                Layout.fillWidth: true; Layout.preferredHeight: 48
                text: root.deck.playing ? "❚❚" : "▶"; fontSize: 22
                accent: root.accent; active: root.deck.playing
                onClicked: root.deck.playPause()
            }
            TouchButton {
                Layout.preferredWidth: 64; Layout.preferredHeight: 48
                text: "SYNC"; accent: Theme.cyan; active: root.deck.syncLock
                onClicked: root.deck.sync()
                onLongPressed: root.deck.syncLock = !root.deck.syncLock
            }
        }

        // ---- Modo de los pads: CUES · LOOP · FX · STEMS
        Row {
            id: modeRow
            Layout.fillWidth: true
            spacing: 4
            property int mode: 0
            Repeater {
                model: ["CUES", "LOOP", "FX", "STEMS"]
                TouchButton {
                    width: (modeRow.width - 12) / 4
                    height: root.tight ? 26 : 30
                    text: index === 3 && root.deck.stemsState === 1 ? "⏳ " + Math.round(root.deck.stemsProgress * 100) + "%"
                          : modelData + (index === 2 && root.deck.fxType > 0 ? " ●" : "")
                    fontSize: 11
                    accent: Theme.gold
                    active: modeRow.mode === index
                    onClicked: modeRow.mode = index
                }
            }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.tight ? 38 : 42
            currentIndex: modeRow.mode
            // CUES
            RowLayout {
                spacing: 5
                Repeater {
                    model: 4
                    TouchButton {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        readonly property bool isSet: root.deck.hotCues.length > index && root.deck.hotCues[index] >= 0
                        text: (index + 1).toString(); fontSize: 14
                        accent: ["#ff2d8c", "#3fe0ff", "#55e08a", "#f5b942"][index]
                        active: isSet
                        onClicked: root.deck.hotCue(index)
                        onLongPressed: root.deck.clearHotCue(index)
                    }
                }
                TouchButton {
                    Layout.preferredWidth: 70; Layout.fillHeight: true
                    text: root.deck.loopActive ? "LOOP " + root.deck.loopBeats : "LOOP 4"
                    fontSize: 12; accent: Theme.green; active: root.deck.loopActive
                    onClicked: root.deck.loopActive ? root.deck.loopExit() : root.deck.autoLoop(4)
                    onLongPressed: root.deck.loopActive ? root.deck.loopHalve() : root.deck.autoLoop(8)
                }
            }
            // LOOP
            RowLayout {
                spacing: 5
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "◀◀"; fontSize: 13; onClicked: root.deck.beatJump(-4) }
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "½"; onClicked: root.deck.loopActive ? root.deck.loopHalve() : root.deck.autoLoop(2) }
                Repeater {
                    model: [1, 4, 8]
                    TouchButton {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        text: modelData.toString(); fontSize: 14; accent: Theme.green
                        active: root.deck.loopActive && root.deck.loopBeats === modelData
                        onClicked: (root.deck.loopActive && root.deck.loopBeats === modelData) ? root.deck.loopExit() : root.deck.autoLoop(modelData)
                    }
                }
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "×2"; onClicked: root.deck.loopActive ? root.deck.loopDouble() : root.deck.autoLoop(16) }
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "▶▶"; fontSize: 13; onClicked: root.deck.beatJump(4) }
            }
            // FX: tocar un efecto lo enciende/apaga; el fader controla la mezcla
            RowLayout {
                spacing: 4
                Repeater {
                    model: 6
                    TouchButton {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        text: ["ECO", "REV", "FLA", "PHA", "CRU", "GAT"][index]; fontSize: 11
                        accent: Theme.magenta
                        active: root.deck.fxType === index + 1
                        onClicked: root.deck.toggleFx(index + 1)
                    }
                }
                TouchFader {
                    visible: root.width > 420      // en decks estrechos el efecto usa la mezcla guardada
                    Layout.preferredWidth: root.width * 0.26; Layout.fillHeight: true
                    vertical: false; from: 0; to: 1; defaultValue: 0.5
                    value: root.deck.fxWet; accent: Theme.magenta
                    onMoved: function(v) { root.deck.fxWet = v }
                }
            }
            // STEMS
            RowLayout {
                spacing: 4
                Repeater {
                    model: [["VOZ", "#ff2d8c"], ["BAT", "#f5b942"], ["BAJO", "#3fe0ff"], ["MEL", "#55e08a"]]
                    TouchButton {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        text: modelData[0]; fontSize: 11; accent: modelData[1]
                        enabledLook: root.deck.stemsState === 2
                        active: root.deck.stemsState === 2 && root.deck.stemOn.length > index && root.deck.stemOn[index]
                        onClicked: if (root.deck.stemsState === 2) root.deck.toggleStem(index)
                                   else if (root.deck.stemsState === 0 || root.deck.stemsState === 3) root.deck.prepareStems()
                    }
                }
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "ACAP"; fontSize: 11; accent: "#ff2d8c"
                              enabledLook: root.deck.stemsState === 2; onClicked: root.deck.acapella() }
                TouchButton { Layout.fillWidth: true; Layout.fillHeight: true; text: "INST"; fontSize: 11; accent: Theme.cyan
                              enabledLook: root.deck.stemsState === 2; onClicked: root.deck.instrumental() }
            }
        }

        // ---- Tempo (se oculta en pantallas muy bajas)
        RowLayout {
            visible: !root.tight
            Layout.fillWidth: true
            spacing: 6
            TouchButton {
                Layout.preferredWidth: 40; Layout.preferredHeight: 40
                text: "−"; fontSize: 20
                onPressed: root.deck.nudge(-0.04)
                onReleased: root.deck.nudge(0)
            }
            TouchFader {
                Layout.fillWidth: true; Layout.preferredHeight: 40
                vertical: false
                from: -1; to: 1; defaultValue: 0
                centerMark: true
                value: root.deck.pitch
                accent: root.accent
                onMoved: function(v) { root.deck.pitch = v }
            }
            TouchButton {
                Layout.preferredWidth: 40; Layout.preferredHeight: 40
                text: "+"; fontSize: 20
                onPressed: root.deck.nudge(0.04)
                onReleased: root.deck.nudge(0)
            }
            TouchButton {
                Layout.preferredWidth: 62; Layout.preferredHeight: 40
                text: (root.deck.pitchPercent >= 0 ? "+" : "") + root.deck.pitchPercent.toFixed(1) + "%"
                sub: root.deck.keyLock ? "KEY" : ""
                fontSize: 12; accent: Theme.magenta; active: root.deck.keyLock
                onClicked: root.deck.keyLock = !root.deck.keyLock
                onLongPressed: root.deck.resetPitch()
            }
        }
    }
}
