// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls

MenuItem {
    id: item
    implicitHeight: 28
    leftPadding: 10; rightPadding: 10
    contentItem: Text {
        text: item.text
        color: !item.enabled ? Theme.faint : (item.highlighted ? Theme.gold : Theme.text)
        font.pixelSize: 13
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
    arrow: Text {
        visible: item.subMenu !== null
        x: item.width - width - 8; anchors.verticalCenter: parent.verticalCenter
        text: "▸"; color: Theme.dim; font.pixelSize: 12
    }
    background: Rectangle {
        radius: 5
        color: item.highlighted ? Theme.raised : "transparent"
    }
}
