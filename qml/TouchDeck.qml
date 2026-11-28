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

        // ---- Hot cues y loop
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Repeater {
                model: 4
                TouchButton {
                    Layout.fillWidth: true; Layout.preferredHeight: root.tight ? 38 : 42
                    readonly property bool isSet: root.deck.hotCues.length > index && root.deck.hotCues[index] >= 0
                    text: (index + 1).toString(); fontSize: 14
                    accent: ["#ff2d8c", "#3fe0ff", "#55e08a", "#f5b942"][index]
                    active: isSet
                    onClicked: root.deck.hotCue(index)
                    onLongPressed: root.deck.clearHotCue(index)
                }
            }
            TouchButton {
                Layout.preferredWidth: 70; Layout.preferredHeight: root.tight ? 38 : 42
                text: root.deck.loopActive ? "LOOP " + root.deck.loopBeats : "LOOP 4"
                fontSize: 12; accent: Theme.green; active: root.deck.loopActive
                onClicked: root.deck.loopActive ? root.deck.loopExit() : root.deck.autoLoop(4)
                onLongPressed: root.deck.loopActive ? root.deck.loopHalve() : root.deck.autoLoop(8)
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
