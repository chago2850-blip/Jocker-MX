// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Perilla giratoria. Arrastre vertical para ajustar, doble clic para regresar al valor por defecto.
Item {
    id: root
    property real value: 0
    property real from: -1
    property real to: 1
    property real defaultValue: 0
    property string label: ""
    property color accent: Theme.gold
    property bool bipolar: from < 0
    property int size: 40
    signal moved(real v)

    implicitWidth: size + 8
    implicitHeight: size + (label.length ? 16 : 0)

    function norm(v) { return (v - from) / (to - from) }

    Canvas {
        id: cv
        width: root.size; height: root.size
        anchors.horizontalCenter: parent.horizontalCenter
        property real v: root.value
        onVChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var c = width / 2, r = width / 2 - 3
            var a0 = Math.PI * 0.75, a1 = Math.PI * 2.25
            ctx.lineWidth = 3.5
            ctx.lineCap = "round"
            ctx.strokeStyle = "#2f2549"
            ctx.beginPath(); ctx.arc(c, c, r, a0, a1); ctx.stroke()
            var n = root.norm(root.value)
            var start = root.bipolar ? Math.PI * 1.5 : a0
            var end = a0 + (a1 - a0) * n
            ctx.strokeStyle = root.accent
            ctx.beginPath()
            if (end >= start) ctx.arc(c, c, r, start, end); else ctx.arc(c, c, r, end, start)
            ctx.stroke()
            // cuerpo
            var g = ctx.createRadialGradient(c - 4, c - 4, 2, c, c, r - 4)
            g.addColorStop(0, "#3a2f55"); g.addColorStop(1, "#191226")
            ctx.fillStyle = g
            ctx.beginPath(); ctx.arc(c, c, r - 5, 0, Math.PI * 2); ctx.fill()
            // indicador
            ctx.strokeStyle = "#efe9f8"; ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(c + Math.cos(end) * (r - 14), c + Math.sin(end) * (r - 14))
            ctx.lineTo(c + Math.cos(end) * (r - 6), c + Math.sin(end) * (r - 6))
            ctx.stroke()
        }
        MouseArea {
            anchors.fill: parent
            property real startY: 0
            property real startV: 0
            cursorShape: Qt.SizeVerCursor
            onPressed: function(m) { startY = m.y; startV = root.value }
            onPositionChanged: function(m) {
                var v = startV + (startY - m.y) / 150 * (root.to - root.from)
                v = Math.max(root.from, Math.min(root.to, v))
                if (root.bipolar && Math.abs(v - root.defaultValue) < 0.02 * (root.to - root.from)) v = root.defaultValue
                root.moved(v)
            }
            onDoubleClicked: root.moved(root.defaultValue)
            onWheel: function(w) {
                var v = root.value + (w.angleDelta.y > 0 ? 1 : -1) * 0.04 * (root.to - root.from)
                root.moved(Math.max(root.from, Math.min(root.to, v)))
            }
        }
    }
    Text {
        visible: root.label.length > 0
        anchors.top: cv.bottom; anchors.topMargin: 1
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        color: Theme.dim
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1
    }
}
