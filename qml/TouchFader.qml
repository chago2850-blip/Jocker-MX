// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import "nav.js" as Nav

// Fader grande (vertical u horizontal). Doble toque: valor por omisión.
// Con el control remoto: en su eje cambia el valor; en el otro eje mueve el foco.
Item {
    id: f
    property real value: 0
    property real from: 0
    property real to: 1
    property real defaultValue: from
    property bool vertical: true
    property color accent: Theme.gold
    property string label: ""
    property bool centerMark: false
    signal moved(real v)

    implicitWidth: vertical ? 44 : 200
    implicitHeight: vertical ? 160 : 44
    activeFocusOnTab: true
    readonly property real frac: (value - from) / (to - from)

    Rectangle {   // carril
        anchors.centerIn: parent
        width: f.vertical ? 8 : parent.width - 20
        height: f.vertical ? parent.height - 20 : 8
        radius: 4
        color: Theme.bg2
        border.color: f.activeFocus ? "#ffffff" : Theme.border
        Rectangle {   // relleno
            color: Qt.rgba(f.accent.r, f.accent.g, f.accent.b, 0.55)
            radius: 4
            x: 0
            width: f.vertical ? parent.width : parent.width * f.frac
            height: f.vertical ? parent.height * f.frac : parent.height
            y: f.vertical ? parent.height - height : 0
            visible: !f.centerMark
        }
        Rectangle {   // marca central
            visible: f.centerMark
            color: Theme.faint
            width: f.vertical ? parent.width + 10 : 2
            height: f.vertical ? 2 : parent.height + 10
            anchors.centerIn: parent
        }
    }
    Rectangle {   // tirador
        width: f.vertical ? 40 : 26
        height: f.vertical ? 26 : 40
        radius: 7
        x: f.vertical ? (f.width - width) / 2 : 10 + (f.width - 20) * f.frac - width / 2
        y: f.vertical ? 10 + (f.height - 20) * (1 - f.frac) - height / 2 : (f.height - height) / 2
        color: area.pressed ? f.accent : Theme.raised
        border.color: f.activeFocus ? "#ffffff" : f.accent
        border.width: f.activeFocus ? 2.5 : 1.5
        Rectangle { anchors.centerIn: parent; color: f.accent
                    width: f.vertical ? parent.width - 12 : 2; height: f.vertical ? 2 : parent.height - 12 }
    }
    Text {
        visible: f.label.length > 0
        anchors.horizontalCenter: f.vertical ? parent.horizontalCenter : undefined
        anchors.left: f.vertical ? undefined : parent.left
        anchors.top: f.vertical ? parent.bottom : undefined
        anchors.bottom: f.vertical ? undefined : parent.top
        anchors.leftMargin: 6
        text: f.label; color: Theme.faint; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1
    }
    function setFromPos(px, py) {
        var t = f.vertical ? 1 - (py - 10) / (f.height - 20) : (px - 10) / (f.width - 20)
        t = Math.max(0, Math.min(1, t))
        f.moved(f.from + t * (f.to - f.from))
    }
    MouseArea {
        id: area
        anchors.fill: parent
        preventStealing: true
        property real grab: 0
        onPressed: function(m) {
            // Si se toca el tirador se arrastra desde donde está; si no, salta al punto tocado.
            var knobPos = f.vertical ? 10 + (f.height - 20) * (1 - f.frac) : 10 + (f.width - 20) * f.frac
            var p = f.vertical ? m.y : m.x
            grab = Math.abs(p - knobPos) < 24 ? p - knobPos : 0
            f.setFromPos(m.x - (f.vertical ? 0 : grab), m.y - (f.vertical ? grab : 0))
        }
        onPositionChanged: function(m) { if (pressed) f.setFromPos(m.x - (f.vertical ? 0 : grab), m.y - (f.vertical ? grab : 0)) }
        onDoubleClicked: f.moved(f.defaultValue)
    }
    Keys.onPressed: function(e) {
        var step = (f.to - f.from) / 20
        if (f.vertical && e.key === Qt.Key_Up) { f.moved(Math.min(f.to, f.value + step)); e.accepted = true }
        else if (f.vertical && e.key === Qt.Key_Down) { f.moved(Math.max(f.from, f.value - step)); e.accepted = true }
        else if (!f.vertical && e.key === Qt.Key_Right) { f.moved(Math.min(f.to, f.value + step)); e.accepted = true }
        else if (!f.vertical && e.key === Qt.Key_Left) { f.moved(Math.max(f.from, f.value - step)); e.accepted = true }
        else if (Nav.isOk(e.key)) { f.moved(f.defaultValue); e.accepted = true }
        else if (Nav.handleArrows(f, e)) e.accepted = true
    }
}
