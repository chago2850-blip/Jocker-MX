// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Navegador de música: carpeta, búsqueda, lista y carga a decks.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    function loadToFreeDeck(path) {
        if (!deckA.playing || (!deckA.loaded)) deckA.load(path)
        else if (!deckB.playing) deckB.load(path)
    }

    FolderDialog {
        id: folderDialog
        title: "Elige tu carpeta de música"
        onAccepted: library.folder = selectedFolder.toString()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            JxButton { text: "📁  Carpeta"; Layout.preferredWidth: 110; onClicked: folderDialog.open() }
            Text {
                Layout.maximumWidth: 380
                text: library.folder
                color: Theme.dim; font.pixelSize: 11; elide: Text.ElideMiddle
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                Layout.preferredWidth: 320; Layout.preferredHeight: 32
                radius: 16; color: Theme.bg2; border.color: search.activeFocus ? Theme.gold : Theme.border
                Text { anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "🔍"; font.pixelSize: 13 }
                TextInput {
                    id: search
                    anchors.fill: parent; anchors.leftMargin: 34; anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text; font.pixelSize: 13
                    selectByMouse: true
                    onTextChanged: library.filter = text
                    Text { anchors.verticalCenter: parent.verticalCenter; text: "Buscar título o artista…"; color: Theme.faint; visible: !search.text.length && !search.activeFocus; font.pixelSize: 13 }
                }
            }
            Text {
                text: library.scanning ? "Escaneando…" : (library.count + " / " + library.totalCount + " pistas")
                color: Theme.dim; font.pixelSize: 11
            }
            JxButton { text: "⟳"; Layout.preferredWidth: 34; onClicked: library.rescan() }
        }

        // Encabezados
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8; Layout.rightMargin: 8
            spacing: 10
            component Head: Text { color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1
                                   property string key: ""
                                   MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: library.sortBy(parent.key) } }
            Head { text: "CARGAR"; Layout.preferredWidth: 66 }
            Head { text: "TÍTULO"; key: "title"; Layout.fillWidth: true; Layout.preferredWidth: 3 }
            Head { text: "ARTISTA"; key: "artist"; Layout.fillWidth: true; Layout.preferredWidth: 2 }
            Head { text: "BPM"; key: "bpm"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight }
            Head { text: "FORMATO"; Layout.preferredWidth: 60 }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: library
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            delegate: Rectangle {
                id: row
                width: ListView.view.width
                height: 32
                radius: 6
                readonly property bool onA: deckA.filePath === model.path
                readonly property bool onB: deckB.filePath === model.path
                color: ListView.isCurrentItem ? Theme.raised : (index % 2 ? "transparent" : Qt.rgba(1, 1, 1, 0.02))
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
                        JxButton { width: 30; height: 24; text: "A"; fontSize: 11; accent: Theme.magenta; active: row.onA; onClicked: deckA.load(model.path) }
                        JxButton { width: 30; height: 24; text: "B"; fontSize: 11; accent: Theme.cyan; active: row.onB; onClicked: deckB.load(model.path) }
                    }
                    Text { Layout.fillWidth: true; Layout.preferredWidth: 3; text: model.title; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight }
                    Text { Layout.fillWidth: true; Layout.preferredWidth: 2; text: model.artist; color: Theme.dim; font.pixelSize: 13; elide: Text.ElideRight }
                    Text { Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; text: model.bpm > 0 ? model.bpm.toFixed(1) : "—"; color: model.bpm > 0 ? Theme.gold : Theme.faint; font.pixelSize: 13 }
                    Text { Layout.preferredWidth: 60; text: model.format; color: Theme.faint; font.pixelSize: 11 }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: list.count === 0 && !library.scanning
                horizontalAlignment: Text.AlignHCenter
                text: library.totalCount === 0 ? "No hay música en esta carpeta.\nPulsa «Carpeta» para elegir dónde está tu música (MP3, WAV, FLAC)."
                                               : "Sin resultados para «" + library.filter + "»"
                color: Theme.faint; font.pixelSize: 13
            }
        }
    }
}
