// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Medidor de nivel segmentado (vertical).
Item {
    id: root
    property real level: 0          // amplitud lineal 0..1+
    property int segments: 18
    implicitWidth: 8
    implicitHeight: 120
    readonly property real db: level > 0.00001 ? 20 * Math.log(level) / Math.LN10 : -90
    readonly property real fill: Math.max(0, Math.min(1, (db + 48) / 48))

    Column {
        anchors.fill: parent
        spacing: 2
        Repeater {
            model: root.segments
            Rectangle {
                readonly property int idx: root.segments - 1 - index
                readonly property bool lit: (idx + 0.5) / root.segments < root.fill
                width: root.width
                height: (root.height - (root.segments - 1) * 2) / root.segments
                radius: 1
                color: idx >= root.segments - 2 ? Theme.red : (idx >= root.segments - 5 ? Theme.gold : Theme.green)
                opacity: lit ? 1.0 : 0.12
            }
        }
    }
}
