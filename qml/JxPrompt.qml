// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls

// Pequeño diálogo para escribir un nombre (listas, etc.).
Popup {
    id: pop
    property string title: ""
    property string value: ""
    property string okText: "Aceptar"
    signal accepted(string text)

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: 360
    padding: 16
    background: Rectangle { color: Theme.panel; radius: 12; border.color: Theme.gold }
    onOpened: { input.text = value; input.selectAll(); input.forceActiveFocus() }

    function ask(t, v, ok) { title = t; value = v || ""; okText = ok || "Aceptar"; open() }
    function finish() { if (input.text.trim().length) { pop.accepted(input.text.trim()); pop.close() } }

    Column {
        width: parent.width
        spacing: 12
        Text { text: pop.title; color: Theme.gold; font.pixelSize: 14; font.weight: Font.Bold }
        Rectangle {
            width: parent.width; height: 34; radius: 8
            color: Theme.bg2; border.color: input.activeFocus ? Theme.gold : Theme.border
            TextInput {
                id: input
                anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.text; font.pixelSize: 14; selectByMouse: true
                Keys.onReturnPressed: pop.finish()
                Keys.onEnterPressed: pop.finish()
                Keys.onEscapePressed: pop.close()
            }
        }
        Row {
            anchors.right: parent.right
            spacing: 8
            JxButton { width: 100; height: 30; text: "Cancelar"; onClicked: pop.close() }
            JxButton { width: 110; height: 30; text: pop.okText; accent: Theme.gold; active: true; onClicked: pop.finish() }
        }
    }
}
