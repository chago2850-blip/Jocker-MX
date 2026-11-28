// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import JockerMX
import "nav.js" as Nav

// Biblioteca táctil: búsqueda, vistas, listas y filas grandes con «A», «B» y «+ Automix».
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border
    signal loaded()      // se cargó una pista (la interfaz vuelve a los decks)

    function fmt(s) { return s > 0 ? Math.floor(s / 60) + ":" + ("0" + Math.floor(s % 60)).slice(-2) : "" }
    function loadTo(d, path) { d.load(path); root.loaded() }
    function loadFree(path) {
        if (!deckA.loaded || !deckA.playing) loadTo(deckA, path)
        else if (!deckB.playing) loadTo(deckB, path)
        else loadTo(deckA, path)
    }

    FolderDialog {
        id: folderDialog
        title: "Elige tu carpeta de música"
        onAccepted: library.folder = selectedFolder.toString()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 10
                color: Theme.bg2; border.color: search.activeFocus ? Theme.gold : Theme.border
                TextInput {
                    id: search
                    anchors.fill: parent; anchors.leftMargin: 40; anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text; font.pixelSize: 16; clip: true
                    text: library.filter
                    onTextChanged: library.filter = text
                    Keys.onPressed: function(e) { if (e.key === Qt.Key_Down) { Nav.move(search, "down"); e.accepted = true } }
                }
                Text { x: 12; anchors.verticalCenter: parent.verticalCenter; text: "🔍"; font.pixelSize: 16 }
                Text { x: 40; anchors.verticalCenter: parent.verticalCenter; visible: !search.text.length && !search.activeFocus
                       text: "Buscar canción, artista, BPM…"; color: Theme.faint; font.pixelSize: 15 }
            }
            TouchButton { Layout.preferredWidth: 54; Layout.preferredHeight: 44; text: "📁"; fontSize: 18; onClicked: folderDialog.open() }
            TouchButton { Layout.preferredWidth: 54; Layout.preferredHeight: 44; text: library.analyzing ? "⏳" : "⚡"; fontSize: 18
                          onClicked: library.analyzeAll() }
        }

        // Vistas y listas (desplazable)
        ListView {
            id: chips
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: {
                var m = [{ t: "Todo", v: 0, id: 0 }, { t: "Historial", v: 1, id: 0 }, { t: "Más tocadas", v: 2, id: 0 }, { t: "Recientes", v: 3, id: 0 }]
                var pl = library.playlists
                for (var i = 0; i < pl.length; ++i) m.push({ t: "📃 " + pl[i].name, v: 4, id: pl[i].id })
                return m
            }
            delegate: TouchButton {
                width: Math.max(90, implicitW.implicitWidth + 28); height: 38
                fontSize: 13
                text: modelData.t
                accent: Theme.cyan
                active: library.view === modelData.v && (modelData.v !== 4 || library.playlistId === modelData.id)
                onClicked: modelData.v === 4 ? (library.playlistId = modelData.id) : (library.view = modelData.v)
                Text { id: implicitW; visible: false; text: modelData.t; font.pixelSize: 13; font.weight: Font.Bold }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: library
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { width: 8; policy: ScrollBar.AsNeeded }
            delegate: Rectangle {
                id: row
                required property int index
                required property string path
                required property string title
                required property string artist
                required property real bpm
                required property string musicalKey
                required property real duration
                required property bool karaoke
                readonly property bool onA: deckA.filePath === path
                readonly property bool onB: deckB.filePath === path
                width: list.width - 10
                height: 56
                radius: 10
                color: rowMa.pressed ? Theme.raised : (index % 2 ? Theme.bg2 : Qt.rgba(1, 1, 1, 0.03))
                border.color: onA ? Theme.deckColor(0) : (onB ? Theme.deckColor(1) : "transparent")
                MouseArea { id: rowMa; anchors.fill: parent; onClicked: root.loadFree(row.path) }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 6
                    spacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true; elide: Text.ElideRight
                            text: (row.karaoke ? "🎤 " : "") + (/\.(mp4|m4v|mov|mkv|webm|avi)$/i.test(row.path) ? "🎬 " : "") + row.title
                            color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true; elide: Text.ElideRight
                            text: row.artist + (row.duration > 0 ? "  ·  " + root.fmt(row.duration) : "")
                            color: Theme.dim; font.pixelSize: 12
                        }
                    }
                    Column {
                        Text { anchors.right: parent.right; text: row.bpm > 0 ? row.bpm.toFixed(1) : ""; color: Theme.gold; font.pixelSize: 13; font.weight: Font.Bold }
                        Text { anchors.right: parent.right; text: row.musicalKey; color: Theme.cyan; font.pixelSize: 12 }
                    }
                    TouchButton { Layout.preferredWidth: 46; Layout.preferredHeight: 42; text: "A"; accent: Theme.deckColor(0); active: row.onA
                                  onClicked: root.loadTo(deckA, row.path) }
                    TouchButton { Layout.preferredWidth: 46; Layout.preferredHeight: 42; text: "B"; accent: Theme.deckColor(1); active: row.onB
                                  onClicked: root.loadTo(deckB, row.path) }
                    TouchButton { Layout.preferredWidth: 46; Layout.preferredHeight: 42; text: "+"; fontSize: 20; accent: Theme.green
                                  onClicked: automix.add([row.path]) }
                }
            }
            Text {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: list.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: library.filter.length ? "Nada coincide con «" + library.filter + "»."
                      : "No hay música aquí todavía.\nToca 📁 para elegir tu carpeta de música."
                color: Theme.faint; font.pixelSize: 15
            }
        }
    }
}
