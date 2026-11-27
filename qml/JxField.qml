// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Layouts

// Campo de texto con etiqueta, al estilo de Jocker-MX. Guarda al terminar de editar.
ColumnLayout {
    id: f
    property string label: ""
    property string text: ""
    property bool password: false
    property bool numeric: false
    property string placeholder: ""
    property bool enabledField: true
    signal edited(string value)
    spacing: 3
    Text { text: f.label; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1 }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 28
        radius: 6
        color: Theme.bg2
        border.color: input.activeFocus ? Theme.gold : Theme.border
        opacity: f.enabledField ? 1 : 0.5
        TextInput {
            id: input
            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
            verticalAlignment: TextInput.AlignVCenter
            text: f.text
            enabled: f.enabledField
            echoMode: f.password ? TextInput.Password : TextInput.Normal
            inputMethodHints: f.numeric ? Qt.ImhDigitsOnly : Qt.ImhNone
            validator: f.numeric ? intVal : null
            color: Theme.text; font.pixelSize: 13; selectByMouse: true; clip: true
            onEditingFinished: if (text !== f.text) f.edited(text)
            IntValidator { id: intVal; bottom: 1; top: 65535 }
            Text { anchors.verticalCenter: parent.verticalCenter; visible: !input.text.length && !input.activeFocus
                   text: f.placeholder; color: Theme.faint; font.pixelSize: 12 }
        }
    }
}
