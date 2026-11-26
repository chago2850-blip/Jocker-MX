// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Fader de pitch vertical (arriba = más lento, como en los platos; centro = 0 %).
Item {
    id: root
    property real value: 0            // -1..1
    property color accent: Theme.gold
    signal moved(real v)
    implicitWidth: 34
    implicitHeight: 200

    Rectangle {   // riel
        anchors.horizontalCenter: parent.horizontalCenter
        width: 4; radius: 2
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.topMargin: 10; anchors.bottomMargin: 10
        color: Theme.bg
        border.color: Theme.border
    }
    Repeater {     // marcas
        model: 9
        Rectangle {
            x: index === 4 ? 2 : 8
            width: index === 4 ? root.width - 4 : root.width - 16
            height: 1
            y: 10 + index * (root.height - 20) / 8
            color: index === 4 ? Theme.gold : Theme.faint
            opacity: index === 4 ? 0.9 : 0.5
        }
    }
    Rectangle {    // perilla
        id: cap
        width: root.width; height: 18; radius: 4
        y: 10 + (root.value + 1) / 2 * (root.height - 20) - height / 2
        gradient: Gradient {
            GradientStop { position: 0; color: "#4a3d6b" }
            GradientStop { position: 1; color: "#251c3a" }
        }
        border.color: Math.abs(root.value) < 0.001 ? root.accent : Theme.faint
        Rectangle { anchors.centerIn: parent; width: parent.width - 8; height: 2; color: root.accent }
    }
    MouseArea {
        anchors.fill: parent
        property real grabOffset: 0
        onPressed: function(m) { grabOffset = m.y - (cap.y + cap.height / 2) ; if (Math.abs(grabOffset) > 12) grabOffset = 0 }
        onPositionChanged: function(m) {
            var v = ((m.y - grabOffset) - 10) / (root.height - 20) * 2 - 1
            v = Math.max(-1, Math.min(1, v))
            if (Math.abs(v) < 0.015) v = 0
            root.moved(v)
        }
        onDoubleClicked: root.moved(0)
    }
}
