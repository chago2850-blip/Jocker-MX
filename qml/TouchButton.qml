// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import "nav.js" as Nav

// Botón grande para pantallas táctiles y control remoto de TV (flechas mueven el foco, OK pulsa).
Rectangle {
    id: b
    property string text: ""
    property string sub: ""              // segunda línea pequeña
    property color accent: Theme.gold
    property bool active: false
    property bool enabledLook: true
    property int fontSize: 15
    property bool keyDown: false
    signal clicked()
    signal pressed()
    signal released()
    signal longPressed()

    implicitWidth: 72
    implicitHeight: 48
    radius: 10
    activeFocusOnTab: true
    opacity: enabledLook ? 1 : 0.4
    color: (ma.pressed || keyDown) ? Qt.rgba(accent.r, accent.g, accent.b, 0.45)
         : active ? Qt.rgba(accent.r, accent.g, accent.b, 0.20) : Theme.panel2
    border.color: activeFocus ? "#ffffff" : (active ? accent : Theme.border)
    border.width: activeFocus ? 2.5 : 1
    Behavior on color { ColorAnimation { duration: 80 } }

    Column {
        anchors.centerIn: parent
        width: parent.width - 8
        spacing: 1
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: b.text
            color: b.active ? b.accent : Theme.text
            font.pixelSize: b.fontSize
            font.weight: Font.Bold
            elide: Text.ElideRight
        }
        Text {
            visible: b.sub.length > 0
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: b.sub
            color: Theme.dim
            font.pixelSize: Math.max(9, b.fontSize - 5)
            elide: Text.ElideRight
        }
    }
    MouseArea {
        id: ma
        anchors.fill: parent
        onPressed: b.pressed()
        onReleased: b.released()
        onCanceled: b.released()
        onClicked: b.clicked()
        onPressAndHold: b.longPressed()
    }
    Keys.onPressed: function(e) {
        if (Nav.isOk(e.key)) {
            if (!e.isAutoRepeat) { b.keyDown = true; b.pressed() }
            e.accepted = true
        } else if (Nav.handleArrows(b, e)) {
            e.accepted = true
        }
    }
    Keys.onReleased: function(e) {
        if (Nav.isOk(e.key) && !e.isAutoRepeat && b.keyDown) {
            b.keyDown = false
            b.released()
            b.clicked()
            e.accepted = true
        }
    }
}
