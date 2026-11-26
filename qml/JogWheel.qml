// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Plato virtual. Centro: scratch (como tocar el vinilo). Aro exterior: empujar/frenar (pitch bend).
Item {
    id: root
    property var deck
    property color accent: Theme.magenta
    implicitWidth: 190
    implicitHeight: 190

    readonly property real secondsPerTurn: 1.8          // 33⅓ RPM
    readonly property real angle: deck ? (deck.position / secondsPerTurn * 360) % 360 : 0

    // Aro exterior con brillo de acento
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#0d0a16"
        border.width: 2
        border.color: deck && deck.playing ? root.accent : Theme.border
        Behavior on border.color { ColorAnimation { duration: 200 } }
    }
    // Marcas del aro (giran con la pista)
    Item {
        anchors.fill: parent
        rotation: root.angle
        Repeater {
            model: 36
            Rectangle {
                width: 2; height: index % 9 === 0 ? 10 : 5
                radius: 1
                color: index % 9 === 0 ? root.accent : Theme.faint
                x: root.width / 2 - 1
                y: 4
                transform: Rotation { origin.x: 1; origin.y: root.height / 2 - 4; angle: index * 10 }
            }
        }
    }
    // Disco central con etiqueta (logo)
    Rectangle {
        id: platter
        anchors.centerIn: parent
        width: parent.width * 0.78; height: width; radius: width / 2
        rotation: root.angle
        gradient: Gradient {
            GradientStop { position: 0; color: "#24202e" }
            GradientStop { position: 1; color: "#09080d" }
        }
        border.color: "#3a3150"
        Repeater {   // surcos
            model: 6
            Rectangle {
                anchors.centerIn: parent
                width: platter.width * (0.92 - index * 0.07); height: width; radius: width / 2
                color: "transparent"
                border.color: Qt.rgba(1, 1, 1, 0.04 + (index % 2) * 0.02)
            }
        }
        Image {
            anchors.centerIn: parent
            width: parent.width * 0.62; height: width
            source: "qrc:/JockerMX/resources/logo-3d-256.png"
            smooth: true
            mipmap: true
            opacity: 0.95
        }
        Rectangle {   // marca de posición
            width: 4; height: platter.height * 0.16; radius: 2
            color: root.accent
            anchors.horizontalCenter: parent.horizontalCenter
            y: 6
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        property real lastAngle: 0
        property real accum: 0
        property bool scratching: false
        property real lastT: 0

        function angleAt(x, y) { return Math.atan2(y - height / 2, x - width / 2) * 180 / Math.PI }

        onPressed: function(m) {
            if (!root.deck || !root.deck.loaded) return
            var dx = m.x - width / 2, dy = m.y - height / 2
            scratching = Math.sqrt(dx * dx + dy * dy) < width * 0.39
            lastAngle = angleAt(m.x, m.y)
            accum = 0
            lastT = Date.now()
            if (scratching) root.deck.scratchBegin()
            tick.start()
        }
        onPositionChanged: function(m) {
            var a = angleAt(m.x, m.y)
            var d = a - lastAngle
            if (d > 180) d -= 360
            if (d < -180) d += 360
            accum += d
            lastAngle = a
        }
        onReleased: {
            tick.stop()
            if (!root.deck) return
            if (scratching) root.deck.scratchEnd(); else root.deck.nudge(0)
        }
        onWheel: function(w) { if (root.deck) root.deck.beatJump(w.angleDelta.y > 0 ? 0.25 : -0.25) }

        Timer {
            id: tick
            interval: 16; repeat: true
            onTriggered: {
                var now = Date.now()
                var dt = Math.max(1, now - ma.lastT) / 1000
                ma.lastT = now
                var turns = ma.accum / 360
                ma.accum = 0
                if (!root.deck) return
                if (ma.scratching) {
                    root.deck.scratchMove(turns * root.secondsPerTurn / dt)
                } else {
                    root.deck.nudge(Math.max(-0.25, Math.min(0.25, turns / dt * 0.15)))
                }
            }
        }
    }
}
