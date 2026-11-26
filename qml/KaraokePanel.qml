// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls
import QtQuick.Layouts

// Karaoke: vista previa (CD+G o letra), tono del cantante, pantalla del público y rotación de cantantes.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    // Deck que se muestra: el del turno actual; si no, el que suena con karaoke; si no, A.
    readonly property var shownDeck: {
        if (karaoke.karaokeDeck === 0) return deckA
        if (karaoke.karaokeDeck === 1) return deckB
        if (deckB.playing && (deckB.hasCdg || deckB.hasLyrics) && !(deckA.playing && (deckA.hasCdg || deckA.hasLyrics))) return deckB
        return deckA
    }
    property string pendingPath: ""
    property int renameRow: -1

    JxPrompt {
        id: namePrompt
        onAccepted: function(name) { karaoke.addSinger(name, root.pendingPath, 0); root.pendingPath = "" }
    }
    JxPrompt {
        id: renamePrompt
        onAccepted: function(name) { karaoke.setSingerName(root.renameRow, name) }
    }
    function askSinger(path) { pendingPath = path; namePrompt.ask("¿Quién canta?", "", "Agregar") }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---------- Vista previa y controles
        ColumnLayout {
            Layout.preferredWidth: 420
            Layout.maximumWidth: 420
            Layout.fillHeight: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                Text { text: "VISTA PREVIA · DECK " + root.shownDeck.name; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.shownDeck.hasCdg ? "CD+G" : (root.shownDeck.hasLyrics ? "LETRA .LRC" : "")
                    color: Theme.green; font.pixelSize: 10; font.weight: Font.Bold
                }
            }
            KaraokeView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 120
                radius: 8
                deck: root.shownDeck
                scale: 0.8
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text { text: "TONO"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 26; text: "−"; fontSize: 14
                           onClicked: root.shownDeck.keyShift = root.shownDeck.keyShift - 1 }
                Text {
                    Layout.preferredWidth: 36; horizontalAlignment: Text.AlignHCenter
                    text: (root.shownDeck.keyShift > 0 ? "+" : "") + root.shownDeck.keyShift
                    color: root.shownDeck.keyShift !== 0 ? Theme.gold : Theme.text; font.pixelSize: 15; font.weight: Font.Bold
                }
                JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 26; text: "+"; fontSize: 14
                           onClicked: root.shownDeck.keyShift = root.shownDeck.keyShift + 1 }
                JxButton { Layout.preferredWidth: 44; Layout.preferredHeight: 26; text: "0"; fontSize: 11
                           onClicked: root.shownDeck.keyShift = 0 }
                Item { Layout.fillWidth: true }
                JxButton { Layout.preferredWidth: 170; Layout.preferredHeight: 26; fontSize: 11; accent: Theme.cyan
                           text: Stage.audienceVisible ? "📺  Cerrar pantalla público" : "📺  Pantalla del público"
                           active: Stage.audienceVisible
                           onClicked: Stage.audienceVisible = !Stage.audienceVisible }
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }

        // ---------- Rotación de cantantes
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text { text: "ROTACIÓN"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.5 }
                Text { text: karaoke.singers.length + (karaoke.singers.length === 1 ? " cantante" : " cantantes")
                             + (karaoke.sungCount ? " · " + karaoke.sungCount + " ya cantaron" : ""); color: Theme.dim; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Rectangle {   // reproducir al llamar al siguiente
                    Layout.preferredWidth: apRow.implicitWidth + 12; Layout.preferredHeight: 24; color: "transparent"
                    Row {
                        id: apRow; anchors.verticalCenter: parent.verticalCenter; spacing: 6
                        Rectangle { width: 30; height: 16; radius: 8; anchors.verticalCenter: parent.verticalCenter
                                    color: karaoke.autoPlay ? Qt.rgba(0.96, 0.73, 0.26, 0.35) : Theme.bg2
                                    border.color: karaoke.autoPlay ? Theme.gold : Theme.border
                                    Rectangle { width: 12; height: 12; radius: 6; y: 2; x: karaoke.autoPlay ? 16 : 2
                                                color: karaoke.autoPlay ? Theme.gold : Theme.faint } }
                        Text { text: "Reproducir al llamar"; color: Theme.text; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: karaoke.autoPlay = !karaoke.autoPlay }
                }
                JxButton { text: "Vaciar"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 70; accent: Theme.red
                           onClicked: karaoke.clear() }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: 8
                color: karaoke.singers.length ? Qt.rgba(1, 0.18, 0.55, 0.14) : Theme.panel2
                border.color: karaoke.singers.length ? Theme.magenta : Theme.border
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Text { text: "🎤"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: karaoke.singers.length ? "SIGUIENTE CANTANTE: " + karaoke.singers[0].name.toUpperCase() : "AGREGA CANTANTES A LA ROTACIÓN"
                        color: karaoke.singers.length ? Theme.magenta : Theme.faint
                        font.pixelSize: 15; font.weight: Font.Black; font.letterSpacing: 1
                    }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; enabled: karaoke.singers.length > 0
                            onClicked: karaoke.startNext() }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: drop.containsDrag ? Qt.rgba(1, 0.18, 0.55, 0.08) : Theme.bg2
                border.color: drop.containsDrag ? Theme.magenta : Theme.border
                DropArea {
                    id: drop
                    anchors.fill: parent
                    onDropped: function(d) {
                        if (d.hasUrls && d.urls.length) root.askSinger(d.urls[0].toString())
                        else if (d.hasText) root.askSinger(d.text)
                    }
                }
                ListView {
                    id: singerList
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    spacing: 2
                    model: karaoke.singers
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Rectangle {
                        id: srow
                        required property int index
                        required property var modelData
                        width: singerList.width - 10
                        height: 32
                        radius: 6
                        color: index === 0 ? Qt.rgba(1, 0.18, 0.55, 0.10) : (index % 2 ? "transparent" : Qt.rgba(1, 1, 1, 0.025))
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 6
                            spacing: 8
                            Text { Layout.preferredWidth: 22; text: srow.index + 1; color: Theme.faint; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                            Text {
                                Layout.preferredWidth: 130; elide: Text.ElideRight
                                text: srow.modelData.name; color: srow.index === 0 ? Theme.magenta : Theme.text
                                font.pixelSize: 13; font.weight: Font.Bold
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                            onDoubleClicked: { root.renameRow = srow.index; renamePrompt.ask("Cambiar nombre", srow.modelData.name, "Guardar") } }
                            }
                            Text { Layout.fillWidth: true; elide: Text.ElideRight; font.pixelSize: 12
                                   text: (srow.modelData.artist.length ? srow.modelData.artist + " — " : "") + srow.modelData.title
                                   color: Theme.dim }
                            Text { text: srow.modelData.karaoke ? "🎤" : "⚠"; font.pixelSize: 12
                                   color: srow.modelData.karaoke ? Theme.green : Theme.red
                                   ToolTip.visible: kMa.containsMouse; ToolTip.text: srow.modelData.karaoke ? "Tiene gráficos o letra" : "Sin .cdg ni .lrc: sólo audio"
                                   MouseArea { id: kMa; anchors.fill: parent; hoverEnabled: true } }
                            Row {
                                spacing: 2
                                JxButton { width: 22; height: 22; text: "−"; fontSize: 12; onClicked: karaoke.setSingerKey(srow.index, srow.modelData.key - 1) }
                                Text { width: 28; height: 22; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                       text: (srow.modelData.key > 0 ? "+" : "") + srow.modelData.key
                                       color: srow.modelData.key !== 0 ? Theme.gold : Theme.dim; font.pixelSize: 12; font.weight: Font.Bold }
                                JxButton { width: 22; height: 22; text: "+"; fontSize: 12; onClicked: karaoke.setSingerKey(srow.index, srow.modelData.key + 1) }
                            }
                            Row {
                                spacing: 3
                                JxButton { width: 24; height: 22; text: "▲"; fontSize: 9; enabledLook: srow.index > 0; onClicked: karaoke.moveSinger(srow.index, srow.index - 1) }
                                JxButton { width: 24; height: 22; text: "▼"; fontSize: 9; enabledLook: srow.index < karaoke.singers.length - 1
                                           onClicked: karaoke.moveSinger(srow.index, srow.index + 1) }
                                JxButton { width: 24; height: 22; text: "✕"; fontSize: 10; accent: Theme.red; onClicked: karaoke.removeSinger(srow.index) }
                            }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: singerList.count === 0
                        horizontalAlignment: Text.AlignHCenter
                        text: "Arrastra una canción aquí, o en la biblioteca: clic derecho → «🎤 Agregar a karaoke…».\nLas canciones con 🎤 tienen su .cdg o .lrc al lado (mismo nombre)."
                        color: Theme.faint; font.pixelSize: 13
                    }
                }
            }
        }
    }

}
