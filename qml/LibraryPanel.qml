// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
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

    readonly property var currentPlaylist: {
        var l = library.playlists
        for (var i = 0; i < l.length; ++i) if (l[i].id === library.playlistId) return l[i]
        return null
    }
    readonly property bool inPlaylist: library.view === 4 && currentPlaylist !== null

    // ---- Menú de una pista
    property string menuPath: ""
    property int menuRow: -1
    JxMenu {
        id: trackMenu
        JxMenuItem { text: "Cargar en deck A"; onTriggered: deckA.load(root.menuPath) }
        JxMenuItem { text: "Cargar en deck B"; onTriggered: deckB.load(root.menuPath) }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }
        JxMenuItem { text: "➕  Agregar a la cola de Automix"; onTriggered: automix.add([root.menuPath]) }
        JxMenuItem { text: "⤒  Automix: reproducir a continuación"; onTriggered: automix.addNext([root.menuPath]) }
        JxMenuItem { text: "🎤  Agregar a karaoke…"; onTriggered: singerPrompt.askFor(root.menuPath) }
        JxMenu {
            id: addToMenu
            title: "📃  Agregar a lista"
            JxMenuItem { text: "Nueva lista…"; onTriggered: newPrompt.askFor([root.menuPath]) }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }
            Instantiator {
                model: library.playlists
                delegate: JxMenuItem {
                    text: modelData.name + "  (" + modelData.count + ")"
                    onTriggered: library.addToPlaylist(modelData.id, [root.menuPath])
                }
                onObjectAdded: function(i, o) { addToMenu.insertItem(i + 2, o) }
                onObjectRemoved: function(i, o) { addToMenu.removeItem(o) }
            }
        }
        MenuSeparator { visible: root.inPlaylist; height: visible ? implicitHeight : 0; contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }
        JxMenuItem { visible: root.inPlaylist; height: visible ? implicitHeight : 0; text: "▲  Subir en la lista"
                     onTriggered: library.movePlaylistItem(root.menuRow, root.menuRow - 1) }
        JxMenuItem { visible: root.inPlaylist; height: visible ? implicitHeight : 0; text: "▼  Bajar en la lista"
                     onTriggered: library.movePlaylistItem(root.menuRow, root.menuRow + 1) }
        JxMenuItem { visible: root.inPlaylist; height: visible ? implicitHeight : 0; text: "✕  Quitar de la lista"
                     onTriggered: library.removeFromPlaylist(root.menuRow) }
    }
    function openTrackMenu(path, row) { menuPath = path; menuRow = row; trackMenu.popup() }

    // ---- Menú de una lista
    property var menuPlaylist: null
    JxMenu {
        id: playlistMenu
        JxMenuItem { text: "▶  Reproducir en Automix"; onTriggered: { automix.clear(); automix.add(library.playlistPaths(root.menuPlaylist.id)); automix.enabled = true } }
        JxMenuItem { text: "➕  Agregar a la cola de Automix"; onTriggered: automix.add(library.playlistPaths(root.menuPlaylist.id)) }
        JxMenuItem { text: "✎  Renombrar…"; onTriggered: renamePrompt.ask("Renombrar lista", root.menuPlaylist.name, "Renombrar") }
        JxMenuItem { text: "🗑  Borrar lista"; onTriggered: library.deletePlaylist(root.menuPlaylist.id) }
    }

    JxPrompt {
        id: newPrompt
        property var pending: []
        function askFor(paths) { pending = paths; ask("Nueva lista de reproducción", "", "Crear") }
        onAccepted: function(name) {
            var id = library.createPlaylist(name, pending)
            pending = []
            if (id > 0) library.playlistId = id
        }
    }
    JxPrompt {
        id: singerPrompt
        property string path: ""
        function askFor(p) { path = p; ask("¿Quién canta?", "", "Agregar") }
        onAccepted: function(name) { karaoke.addSinger(name, path, 0) }
    }
    JxPrompt {
        id: renamePrompt
        onAccepted: function(name) { if (root.menuPlaylist) library.renamePlaylist(root.menuPlaylist.id, name) }
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
                    Layout.preferredHeight: 26
                    radius: 6
                    color: library.view === modelData.v ? Qt.rgba(0.96, 0.73, 0.26, 0.15) : (ma.containsMouse ? Theme.panel2 : "transparent")
                    border.color: library.view === modelData.v ? Theme.gold : "transparent"
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 10; text: modelData.t; color: library.view === modelData.v ? Theme.gold : Theme.text; font.pixelSize: 13 }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: library.view = modelData.v }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Text { text: "LISTAS"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5; Layout.fillWidth: true }
                JxButton { width: 24; height: 20; Layout.preferredWidth: 24; Layout.preferredHeight: 20; text: "+"; fontSize: 12
                           onClicked: newPrompt.askFor([]) }
            }
            ListView {
                id: plList
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 28
                clip: true
                spacing: 2
                model: library.playlists
                boundsBehavior: Flickable.StopAtBounds
                delegate: Rectangle {
                    id: plRow
                    width: plList.width
                    height: 26
                    radius: 6
                    readonly property bool current: library.view === 4 && library.playlistId === modelData.id
                    color: plDrop.containsDrag ? Qt.rgba(0.33, 0.88, 0.54, 0.18)
                           : current ? Qt.rgba(0.96, 0.73, 0.26, 0.15) : (plMa.containsMouse ? Theme.panel2 : "transparent")
                    border.color: plDrop.containsDrag ? Theme.green : (current ? Theme.gold : "transparent")
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 10; width: parent.width - 44; elide: Text.ElideRight
                           text: "📃  " + modelData.name; color: plRow.current ? Theme.gold : Theme.text; font.pixelSize: 13 }
                    Text { anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 8
                           text: modelData.count; color: Theme.faint; font.pixelSize: 10 }
                    MouseArea {
                        id: plMa; anchors.fill: parent; hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(m) {
                            if (m.button === Qt.RightButton) { root.menuPlaylist = modelData; playlistMenu.popup() }
                            else library.playlistId = modelData.id
                        }
                    }
                    DropArea {   // soltar una pista sobre la lista la agrega
                        id: plDrop; anchors.fill: parent
                        onDropped: function(d) { if (d.hasText) library.addToPlaylist(modelData.id, [d.text]) }
                    }
                }
                Text { visible: plList.count === 0; text: "Pulsa + o arrastra\npistas para crear listas"; color: Theme.faint; font.pixelSize: 10; x: 10 }
            }
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
            JxButton {
                Layout.fillWidth: true
                text: library.analyzing ? ("Analizando " + library.analyzedCount + " / " + library.analysisTotal + "  ✕") : "⚡  Analizar todo"
                accent: Theme.cyan; active: library.analyzing; fontSize: 11
                onClicked: library.analyzing ? library.cancelAnalysis() : library.analyzeAll()
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }

        // ---------- Lista
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            RowLayout {     // cabecera de la lista abierta
                visible: root.inPlaylist
                Layout.fillWidth: true
                spacing: 8
                Text { text: "📃  " + (root.currentPlaylist ? root.currentPlaylist.name : ""); color: Theme.gold; font.pixelSize: 15; font.weight: Font.Bold }
                Text { text: (root.currentPlaylist ? root.currentPlaylist.count : 0) + " pistas"; color: Theme.dim; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                JxButton { text: "▶  Reproducir en Automix"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 176; accent: Theme.green; active: true
                           onClicked: { automix.clear(); automix.add(library.playlistPaths(library.playlistId)); automix.enabled = true } }
                JxButton { text: "➕  A la cola"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 100
                           onClicked: automix.add(library.playlistPaths(library.playlistId)) }
                JxButton { text: "⋯"; fontSize: 13; Layout.preferredHeight: 26; Layout.preferredWidth: 32
                           onClicked: { root.menuPlaylist = root.currentPlaylist; playlistMenu.popup() } }
            }

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
                Head { label: "CARGAR"; Layout.preferredWidth: 96 }
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
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(m) {
                            list.currentIndex = index
                            if (m.button === Qt.RightButton) root.openTrackMenu(model.path, index)
                        }
                        onDoubleClicked: function(m) { if (m.button === Qt.LeftButton) root.loadToFreeDeck(model.path) }
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8; anchors.rightMargin: 8
                        spacing: 10
                        Row {
                            Layout.preferredWidth: 96
                            spacing: 4
                            JxButton { width: 30; height: 22; text: "A"; fontSize: 11; accent: Theme.magenta; active: row.onA; onClicked: deckA.load(model.path) }
                            JxButton { width: 30; height: 22; text: "B"; fontSize: 11; accent: Theme.cyan; active: row.onB; onClicked: deckB.load(model.path) }
                            JxButton { width: 26; height: 22; text: "+"; fontSize: 12; accent: Theme.green; onClicked: automix.add([model.path]); onRightClicked: root.openTrackMenu(model.path, index) }
                        }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 30; text: (model.karaoke ? "🎤 " : "") + (/\.(mp4|m4v|mov|mkv|webm|avi)$/i.test(model.path) ? "🎬 " : "") + model.title; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 22; text: model.artist; color: Theme.dim; font.pixelSize: 13; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; Layout.preferredWidth: 18; text: model.album; color: Theme.faint; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { Layout.preferredWidth: 84; text: model.genre; color: Theme.faint; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; text: model.bpm > 0 ? model.bpm.toFixed(1) : "—"; color: model.bpm > 0 ? Theme.gold : Theme.faint; font.pixelSize: 13 }
                        Text { Layout.preferredWidth: 40; text: model.musicalKey; font.pixelSize: 12
                               font.weight: compat ? Font.Bold : Font.Normal
                               readonly property bool compat: model.musicalKey.length > 0 &&
                                   ((deckA.playing && deckA.keyCompatibleWith(model.musicalKey)) || (deckB.playing && deckB.keyCompatibleWith(model.musicalKey)))
                               color: compat ? Theme.green : Theme.cyan }
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
                          : (library.view === 4 ? "Esta lista está vacía.\nArrastra pistas sobre su nombre o usa clic derecho → Agregar a lista."
                          : (library.view === 2 ? "Aquí aparecerán tus pistas más tocadas."
                          : "No hay música en esta carpeta.\nPulsa «Carpeta…» para elegir dónde está tu música (MP3, WAV, FLAC, OGG).")))
                    color: Theme.faint; font.pixelSize: 13
                }
            }
        }
    }
}
