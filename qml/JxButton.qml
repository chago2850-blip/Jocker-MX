// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Botón estilo controlador: retroiluminado cuando está activo.
Rectangle {
    id: root
    property string text: ""
    property bool active: false
    property color accent: Theme.gold
    property int fontSize: 12
    property bool bold: true
    property bool enabledLook: true
    signal clicked()
    signal pressed()
    signal released()
    signal rightClicked()

    implicitWidth: 64
    implicitHeight: 32
    radius: 6
    color: area.pressed ? Qt.darker(accent, 1.6) : (active ? Qt.rgba(accent.r, accent.g, accent.b, 0.22) : Theme.panel2)
    border.width: 1
    border.color: active ? accent : (area.containsMouse ? Theme.faint : Theme.border)
    opacity: enabledLook ? 1.0 : 0.45

    Rectangle {   // brillo superior
        anchors.fill: parent; anchors.margins: 1; radius: parent.radius - 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.07) }
            GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.0) }
        }
    }
    Text {
        anchors.centerIn: parent
        text: root.text
        color: root.active ? Qt.lighter(root.accent, 1.25) : Theme.text
        font.pixelSize: root.fontSize
        font.weight: root.bold ? Font.DemiBold : Font.Normal
        font.letterSpacing: 0.5
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: function(m) { if (m.button === Qt.LeftButton) root.pressed() }
        onReleased: function(m) { if (m.button === Qt.LeftButton) root.released() }
        onClicked: function(m) { if (m.button === Qt.RightButton) root.rightClicked(); else root.clicked() }
    }
}
