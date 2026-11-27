// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls
import QtQuick.Layouts

// Transmisión en vivo a Icecast / SHOUTcast.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    readonly property bool live: broadcast.state === 2
    readonly property bool locked: broadcast.onAir          // no cambiar la configuración en vivo

    function hms(s) {
        s = Math.floor(s)
        const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), x = s % 60
        return (h > 0 ? h + ":" : "") + (m < 10 && h > 0 ? "0" : "") + m + ":" + (x < 10 ? "0" : "") + x
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---------- Configuración
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: form.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
            ColumnLayout {
                id: form
                width: parent.width - 10
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "SERVIDOR"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                    Repeater {
                        model: ["Icecast 2", "SHOUTcast"]
                        JxButton { Layout.preferredWidth: 104; Layout.preferredHeight: 26; fontSize: 11; text: modelData; accent: Theme.cyan
                                   active: broadcast.server === index; enabledLook: !root.locked
                                   onClicked: if (!root.locked) { broadcast.server = index; if (index === 1) broadcast.format = 1 } }
                    }
                    Item { Layout.fillWidth: true }
                    Text { text: "FORMATO"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                    Repeater {
                        model: ["Opus", "MP3"]
                        JxButton { Layout.preferredWidth: 64; Layout.preferredHeight: 26; fontSize: 11; text: modelData; accent: Theme.magenta
                                   active: broadcast.format === index
                                   enabledLook: !root.locked && !(index === 0 && broadcast.server === 1)
                                   onClicked: if (!root.locked && !(index === 0 && broadcast.server === 1)) broadcast.format = index }
                    }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 10
                    rowSpacing: 6
                    JxField { Layout.columnSpan: 2; Layout.fillWidth: true; label: "SERVIDOR (DIRECCIÓN)"; text: broadcast.host
                              placeholder: "radio.ejemplo.com"; enabledField: !root.locked; onEdited: function(v) { broadcast.host = v.trim() } }
                    JxField { Layout.preferredWidth: 90; label: "PUERTO"; text: broadcast.port; numeric: true
                              enabledField: !root.locked; onEdited: function(v) { broadcast.port = parseInt(v) || 8000 } }
                    JxField { Layout.fillWidth: true; label: broadcast.server === 1 ? "ID DE STREAM (SC 2)" : "PUNTO DE MONTAJE"
                              text: broadcast.mount; placeholder: broadcast.server === 1 ? "1" : "/jocker"
                              enabledField: !root.locked; onEdited: function(v) { broadcast.mount = v.trim() } }
                    JxField { Layout.fillWidth: true; Layout.columnSpan: 2; label: "USUARIO"; text: broadcast.user; placeholder: "source"
                              enabledField: !root.locked && broadcast.server === 0; onEdited: function(v) { broadcast.user = v.trim() } }
                    JxField { Layout.fillWidth: true; Layout.columnSpan: 2; label: "CONTRASEÑA"; text: broadcast.password; password: true
                              enabledField: !root.locked; onEdited: function(v) { broadcast.password = v } }
                    JxField { Layout.fillWidth: true; Layout.columnSpan: 2; label: "NOMBRE DE LA EMISIÓN"; text: broadcast.name
                              onEdited: function(v) { broadcast.name = v }; enabledField: !root.locked }
                    JxField { Layout.fillWidth: true; label: "GÉNERO"; text: broadcast.genre; placeholder: "House, Cumbia…"
                              onEdited: function(v) { broadcast.genre = v }; enabledField: !root.locked }
                    JxField { Layout.fillWidth: true; label: "SITIO WEB"; text: broadcast.url; placeholder: "https://…"
                              onEdited: function(v) { broadcast.url = v }; enabledField: !root.locked }
                    JxField { Layout.fillWidth: true; Layout.columnSpan: 4; label: "DESCRIPCIÓN"; text: broadcast.description
                              onEdited: function(v) { broadcast.description = v }; enabledField: !root.locked }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "CALIDAD"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                    Repeater {
                        model: broadcast.format === 0 ? [64, 96, 128, 160, 192, 256] : [96, 128, 160, 192, 256, 320]
                        JxButton { Layout.preferredWidth: 52; Layout.preferredHeight: 24; fontSize: 10; text: modelData; accent: Theme.gold
                                   active: broadcast.bitrate === modelData; enabledLook: !root.locked
                                   onClicked: if (!root.locked) broadcast.bitrate = modelData }
                    }
                    Text { text: "kbps"; color: Theme.dim; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 18
                    component Toggle: Row {
                        id: tg
                        property string label: ""
                        property bool on: false
                        signal toggled()
                        spacing: 8
                        Rectangle {
                            width: 30; height: 16; radius: 8; anchors.verticalCenter: parent.verticalCenter
                            color: tg.on ? Qt.rgba(0.96, 0.73, 0.26, 0.35) : Theme.bg2; border.color: tg.on ? Theme.gold : Theme.border
                            Rectangle { width: 12; height: 12; radius: 6; y: 2; x: tg.on ? 16 : 2; color: tg.on ? Theme.gold : Theme.faint }
                            MouseArea { anchors.fill: parent; anchors.margins: -4; cursorShape: Qt.PointingHandCursor; onClicked: tg.toggled() }
                        }
                        Text { text: tg.label; color: Theme.text; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Toggle { label: "Enviar el título de lo que suena"; on: broadcast.sendTitles; onToggled: broadcast.sendTitles = !broadcast.sendTitles }
                    Toggle { label: "Emisión pública (directorio)"; on: broadcast.isPublic; onToggled: if (!root.locked) broadcast.isPublic = !broadcast.isPublic }
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.faint; font.pixelSize: 10
                    text: broadcast.server === 1
                          ? "SHOUTcast recibe la fuente en el puerto siguiente al indicado (p. ej. 8000 → 8001) y sólo acepta MP3. Con SHOUTcast 2 escribe el ID del stream."
                          : "Opus suena mejor con menos datos (96–128 kbps recomendados); MP3 es compatible con cualquier reproductor. Los datos se guardan en este equipo."
                }
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }

        // ---------- Estado
        ColumnLayout {
            Layout.preferredWidth: 280
            Layout.maximumWidth: 280
            Layout.fillHeight: true
            spacing: 8
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                radius: 10
                color: broadcast.onAir ? Qt.rgba(1, 0.25, 0.35, root.live ? 0.22 : 0.10) : Theme.panel2
                border.color: broadcast.onAir ? Theme.red : Theme.border
                border.width: root.live ? 2 : 1
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Rectangle {
                        width: 14; height: 14; radius: 7; anchors.verticalCenter: parent.verticalCenter
                        color: root.live ? Theme.red : (broadcast.onAir ? Theme.gold : Theme.faint)
                        SequentialAnimation on opacity { running: root.live; loops: Animation.Infinite
                            NumberAnimation { to: 0.35; duration: 600 } NumberAnimation { to: 1; duration: 600 } }
                    }
                    Text { text: broadcast.onAir ? "DETENER" : "📡  SALIR EN VIVO"; color: broadcast.onAir ? Theme.red : Theme.text
                           font.pixelSize: 17; font.weight: Font.Black; font.letterSpacing: 1.5 }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: broadcast.toggle() }
            }
            Text { text: broadcast.stateText; color: root.live ? Theme.red : (broadcast.onAir ? Theme.gold : Theme.dim)
                   font.pixelSize: 15; font.weight: Font.Black }
            Text { Layout.fillWidth: true; visible: broadcast.errorText.length > 0; text: broadcast.errorText; wrapMode: Text.WordWrap
                   color: Theme.gold; font.pixelSize: 12 }
            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 4
                visible: broadcast.onAir
                Text { text: "Tiempo en vivo"; color: Theme.faint; font.pixelSize: 11 }
                Text { text: root.hms(broadcast.seconds); color: Theme.text; font.pixelSize: 13; font.family: "monospace" }
                Text { text: "Enviando"; color: Theme.faint; font.pixelSize: 11 }
                Text { text: broadcast.kbps.toFixed(0) + " kbps"; color: Theme.text; font.pixelSize: 13; font.family: "monospace" }
                Text { text: "Datos"; color: Theme.faint; font.pixelSize: 11 }
                Text { text: broadcast.megabytes.toFixed(1) + " MB"; color: Theme.text; font.pixelSize: 13; font.family: "monospace" }
                Text { text: "Reconexiones"; color: Theme.faint; font.pixelSize: 11 }
                Text { text: broadcast.reconnects; color: broadcast.reconnects ? Theme.gold : Theme.text; font.pixelSize: 13; font.family: "monospace" }
            }
            Text { Layout.fillWidth: true; visible: broadcast.onAir && broadcast.nowPlaying.length > 0; elide: Text.ElideRight
                   text: "♪  " + broadcast.nowPlaying; color: Theme.cyan; font.pixelSize: 12 }
            Item { Layout.fillHeight: true }
            Text { text: "PARA ESCUCHAR"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2 }
            TextEdit { Layout.fillWidth: true; readOnly: true; selectByMouse: true; wrapMode: TextEdit.WrapAnywhere
                       text: broadcast.listenUrl; color: Theme.gold; font.pixelSize: 12 }
        }
    }
}
