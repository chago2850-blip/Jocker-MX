// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Navegador de música: vistas, carpeta, búsqueda, columnas ordenables y carga a decks.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    function loadToFreeDeck(path) {
        if (!deckA.loaded || !deckA.playing) deckA.load(path)
        else if (!deckB.playing) deckB.load(path)
    }
    function fmtDur(s) {
        if (!s || s <= 0) return "—"
        var m = Math.floor(s / 60), x = Math.floor(s % 60)
        return m + ":" + (x < 10 ? "0" : "") + x
    }
    function fmtWhen(t) {
        if (!t) return ""
        var d = new Date(t * 1000)
        return Qt.formatDateTime(d, "dd/MM hh:mm")
    }

    FolderDialog {
        id: folderDialog
        title: "Elige tu carpeta de música"
        onAccepted: library.folder = selectedFolder.toString()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // ---------- Barra lateral
        ColumnLayout {
            Layout.preferredWidth: 170
            Layout.fillHeight: true
            spacing: 4
            Text { text: "BIBLIOTECA"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5; Layout.bottomMargin: 4 }
            Repeater {
                model: [ { t: "🎵  Toda la música", v: 0 }, { t: "🕘  Historial", v: 1 }, { t: "🔥  Más tocadas", v: 2 }, { t: "✨  Agregadas", v: 3 } ]
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    radius: 6
                    color: library.view === modelData.v ? Qt.rgba(0.96, 0.73, 0.26, 0.15) : (ma.containsMouse ? Theme.panel2 : "transparent")
                    border.color: library.view === modelData.v ? Theme.gold : "transparent"
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 10; text: modelData.t; color: library.view === modelData.v ? Theme.gold : Theme.text; font.pixelSize: 13 }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: library.view = modelData.v }
                }
            }
            Item { Layout.fillHeight: true }
            JxButton { text: "📁  Carpeta…"; Layout.fillWidth: true; onClicked: folderDialog.open() }
            Text {
                Layout.fillWidth: true
                text: library.folder
                color: Theme.faint; font.pixelSize: 10; elide: Text.ElideMiddle
            }
            Text {
                Layout.fillWidth: true
                text: library.scanning ? ("Escaneando… " + library.scanned) : (library.totalCount + " pistas")
                color: library.scanning ? Theme.gold : Theme.dim; font.pixelSize: 11
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }

        // ---------- Lista
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 32
                    radius: 16; color: Theme.bg2; border.color: search.activeFocus ? Theme.gold : Theme.border
                    Text { anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "🔍"; font.pixelSize: 13 }
                    TextInput {
                        id: search
                        anchors.fill: parent; anchors.leftMargin: 34; anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.text; font.pixelSize: 13
                        selectByMouse: true
                        onTextChanged: library.filter = text
                        Keys.onEscapePressed: { text = ""; focus = false }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "Buscar título, artista, álbum, género…  (ej. bpm:120-128, key:8A)"; color: Theme.faint; visible: !search.text.length && !search.activeFocus; font.pixelSize: 13 }
                    }
                }
                Text { text: library.count + " resultados"; color: Theme.dim; font.pixelSize: 11 }
                JxButton { text: "⟳"; Layout.preferredWidth: 34; onClicked: library.rescan() }
            }

            // Encabezados ordenables
            RowLayout {
                id: header
                Layout.fillWidth: true
                Layout.leftMargin: 8; Layout.rightMargin: 18
                spacing: 10
                component Head: Text {
                    property string key: ""
                    color: library.sortKey === key && key.length ? Theme.gold : Theme.faint
                    font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1
                    text: label + (library.sortKey === key && key.length ? (library.ascending ? " ▲" : " ▼") : "")
                    property string label: ""
                    elide: Text.ElideRight
                    MouseArea { anchors.fill: parent; enabled: parent.key.length > 0; cursorShape: Qt.PointingHandCursor; onClicked: library.sortBy(parent.key) }
                }
                Head { label: "CARGAR"; Layout.preferredWidth: 66 }
                Head { label: "TÍTULO"; key: "title"; Layout.fillWidth: true; Layout.preferredWidth: 30 }
                Head { label: "ARTISTA"; key: "artist"; Layout.fillWidth: true; Layout.preferredWidth: 22 }
                Head { label: "ÁLBUM"; key: "album"; Layout.fillWidth: true; Layout.preferredWidth: 18 }
                Head { label: "GÉNERO"; key: "genre"; Layout.preferredWidth: 84 }
                Head { label: "BPM"; key: "bpm"; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight }
                Head { label: "TONO"; key: "key"; Layout.preferredWidth: 40 }
                Head { label: "TIEMPO"; key: "duration"; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight }
                Head { label: library.view === 1 ? "TOCADA" : "VECES"; key: library.view === 1 ? "" : "plays"; Layout.preferredWidth: library.view === 1 ? 80 : 46; horizontalAlignment: Text.AlignRight }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: library
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds
                reuseItems: true
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Rectangle {
                    id: row
                    width: ListView.view.width - 10
                    height: 30
                    radius: 6
                    readonly property bool onA: deckA.filePath === model.path
                    readonly property bool onB: deckB.filePath === model.path
                    color: ListView.isCurrentItem ? Theme.raised : (index % 2 ? "transparent" : Qt.rgba(1, 1, 1, 0.025))
                    border.color: onA ? Theme.magenta : (onB ? Theme.cyan : "transparent")

                    Drag.active: dragArea.drag.active
                    Drag.dragType: Drag.Automatic
                    Drag.supportedActions: Qt.CopyAction
                    Drag.mimeData: { "text/plain": model.path }

                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        drag.target: row
                        onClicked: list.currentIndex = index
                        onDoubleClicked: root.loadToFreeDeck(model.path)
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 10
                        Row {
                            Layout.preferredWidth: 66
                            spacing: 4
                            JxButton { width: 30; height: 22; text: "A"; fontSize: 11; accent: Theme.magenta; active: row.onA; onClicked: deckA.load(model.path) }
                            JxButton { width: 30; height: 22; text: "B"; fontSize: 11; accent: Theme.cyan; active: row.onB; onClicked: deckB.load(model.path) }
                        }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 30; text: model.title; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 22; text: model.artist; color: Theme.dim; font.pixelSize: 13; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 18; text: model.album; color: Theme.faint; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { Layout.preferredWidth: 84; text: model.genre; color: Theme.faint; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; text: model.bpm > 0 ? model.bpm.toFixed(1) : "—"; color: model.bpm > 0 ? Theme.gold : Theme.faint; font.pixelSize: 13 }
                        Text { Layout.preferredWidth: 40; text: model.musicalKey; color: Theme.cyan; font.pixelSize: 12 }
                        Text { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; text: root.fmtDur(model.duration); color: Theme.dim; font.pixelSize: 12 }
                        Text { Layout.preferredWidth: library.view === 1 ? 80 : 46; horizontalAlignment: Text.AlignRight
                               text: library.view === 1 ? root.fmtWhen(model.lastPlayed) : (model.plays > 0 ? model.plays : "")
                               color: Theme.faint; font.pixelSize: 11 }
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: list.count === 0 && !library.scanning
                    horizontalAlignment: Text.AlignHCenter
                    text: library.filter.length ? ("Sin resultados para «" + library.filter + "»")
                          : (library.view === 1 ? "Aún no has tocado nada en esta sesión."
                          : (library.view === 2 ? "Aquí aparecerán tus pistas más tocadas."
                          : "No hay música en esta carpeta.\nPulsa «Carpeta…» para elegir dónde está tu música (MP3, WAV, FLAC)."))
                    color: Theme.faint; font.pixelSize: 13
                }
            }
        }
    }
}
