// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls
import QtQuick.Layouts

// Automix: cola de reproducción + mezcla automática sincronizada entre los decks A y B.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    function fmtDur(s) {
        if (!s || s <= 0) return "—"
        var m = Math.floor(s / 60), x = Math.floor(s % 60)
        return m + ":" + (x < 10 ? "0" : "") + x
    }
    function fmtLong(s) {
        var h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60)
        return h > 0 ? (h + " h " + m + " min") : (m > 0 ? m + " min" : Math.round(s) + " s")
    }

    property int leftView: startupPanel === "programacion" ? 1 : 0   // 0 cola, 1 programación
    property int editRow: -1          // evento cuyo menú está abierto

    JxMenu {
        id: actionMenu
        Instantiator {
            model: scheduler.actionNames
            delegate: JxMenuItem { text: modelData; onTriggered: scheduler.setEventAction(root.editRow, index) }
            onObjectAdded: function(i, o) { actionMenu.insertItem(i, o) }
            onObjectRemoved: function(i, o) { actionMenu.removeItem(o) }
        }
    }
    JxMenu {
        id: playlistPick
        JxMenuItem { text: "(no tienes listas: créalas en la biblioteca)"; enabled: false; visible: library.playlists.length === 0; height: visible ? implicitHeight : 0 }
        Instantiator {
            model: library.playlists
            delegate: JxMenuItem { text: modelData.name + "  (" + modelData.count + ")"; onTriggered: scheduler.setEventParam(root.editRow, modelData.id) }
            onObjectAdded: function(i, o) { playlistPick.insertItem(i + 1, o) }
            onObjectRemoved: function(i, o) { playlistPick.removeItem(o) }
        }
    }
    JxMenu {
        id: fadePick
        Repeater {
            model: [10, 20, 30, 60, 120]
            JxMenuItem { text: "Fundido de " + modelData + " s"; onTriggered: scheduler.setEventParam(root.editRow, modelData) }
        }
    }

    JxPrompt {
        id: savePrompt
        onAccepted: function(name) { library.createPlaylist(name, automix.queuePaths()) }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---------- Cola
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                JxButton { text: "COLA"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 70; accent: Theme.gold
                           active: root.leftView === 0; onClicked: root.leftView = 0 }
                JxButton { text: "⏰  PROGRAMACIÓN" + (scheduler.events.length ? "  " + scheduler.events.length : ""); fontSize: 11
                           Layout.preferredHeight: 26; Layout.preferredWidth: 150; accent: Theme.gold
                           active: root.leftView === 1; onClicked: root.leftView = 1 }
                Text { visible: root.leftView === 0
                       text: automix.queue.count + (automix.queue.count === 1 ? " pista" : " pistas") + (automix.queueSeconds > 0 ? " · " + root.fmtLong(automix.queueSeconds) : ""); color: Theme.dim; font.pixelSize: 11 }
                Text { visible: root.leftView === 1; Layout.fillWidth: true; elide: Text.ElideRight
                       text: scheduler.nextText; color: Theme.dim; font.pixelSize: 11 }
                Item { Layout.fillWidth: true; visible: root.leftView === 0 }
                JxButton { visible: root.leftView === 1; text: "➕  Nuevo evento"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 132
                           accent: Theme.green; onClicked: scheduler.addEvent() }
                JxButton { visible: root.leftView === 0; text: "➕  Agregar lo visible"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 150
                           onClicked: automix.add(library.visiblePaths()) }
                JxButton { visible: root.leftView === 0; text: "🔀  Al azar"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 92; onClicked: automix.shuffle() }
                JxButton { visible: root.leftView === 0; text: "💾  Guardar lista"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 124
                           enabledLook: automix.queue.count > 0
                           onClicked: if (automix.queue.count) savePrompt.ask("Guardar la cola como lista", "Automix " + Qt.formatDate(new Date(), "dd/MM"), "Guardar") }
                JxButton { visible: root.leftView === 0; text: "Vaciar"; fontSize: 11; Layout.preferredHeight: 26; Layout.preferredWidth: 70; accent: Theme.red
                           onClicked: automix.clear() }
            }

            Rectangle {
                visible: root.leftView === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: drop.containsDrag ? Qt.rgba(0.96, 0.73, 0.26, 0.08) : Theme.bg2
                border.color: drop.containsDrag ? Theme.gold : Theme.border

                DropArea {
                    id: drop
                    anchors.fill: parent
                    onDropped: function(d) {
                        if (d.hasUrls && d.urls.length) { var l = []; for (var i = 0; i < d.urls.length; ++i) l.push(d.urls[i].toString()); automix.add(l) }
                        else if (d.hasText) automix.add([d.text])
                    }
                }

                ListView {
                    id: queueList
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    model: automix.queue
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    header: Rectangle {     // lo que suena ahora y lo que ya está preparado
                        width: queueList.width - 10
                        height: automix.enabled && automix.activeDeck >= 0 ? (automix.nextTitle.length ? 56 : 30) : 0
                        visible: height > 0
                        color: "transparent"
                        Column {
                            anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
                            spacing: 6
                            Text {
                                readonly property var d: automix.activeDeck === 1 ? deckB : deckA
                                text: "▶  " + d.name + " · " + (d.artist.length ? d.artist + " — " : "") + d.title
                                color: Theme.deckColor(automix.activeDeck); font.pixelSize: 13; font.weight: Font.Bold
                            }
                            Text {
                                visible: automix.nextTitle.length > 0
                                text: "⏭  " + (automix.activeDeck === 1 ? "A" : "B") + " · " + automix.nextTitle +
                                      (automix.planText.length ? "   ·  " + automix.planText : "")
                                color: Theme.dim; font.pixelSize: 12
                            }
                        }
                    }
                    delegate: Rectangle {
                        id: qrow
                        required property int index
                        required property string path
                        required property string title
                        required property string artist
                        required property real bpm
                        required property string musicalKey
                        required property real duration
                        width: ListView.view.width - 10
                        height: 30
                        radius: 6
                        color: qma.containsMouse ? Theme.panel2 : (index % 2 ? "transparent" : Qt.rgba(1, 1, 1, 0.025))
                        MouseArea { id: qma; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 6
                            spacing: 10
                            Text { Layout.preferredWidth: 26; text: qrow.index + 1; color: Theme.faint; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                            Text { Layout.fillWidth: true; Layout.preferredWidth: 30; text: qrow.title; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; Layout.preferredWidth: 22; text: qrow.artist; color: Theme.dim; font.pixelSize: 13; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 46; horizontalAlignment: Text.AlignRight; text: qrow.bpm > 0 ? qrow.bpm.toFixed(1) : "—"
                                   color: qrow.bpm > 0 ? Theme.gold : Theme.faint; font.pixelSize: 12 }
                            Text { Layout.preferredWidth: 32; text: qrow.musicalKey; color: Theme.cyan; font.pixelSize: 12 }
                            Text { Layout.preferredWidth: 42; horizontalAlignment: Text.AlignRight; text: root.fmtDur(qrow.duration); color: Theme.dim; font.pixelSize: 12 }
                            Row {
                                spacing: 3
                                JxButton { width: 24; height: 22; text: "▲"; fontSize: 9; enabledLook: qrow.index > 0; onClicked: automix.move(qrow.index, qrow.index - 1) }
                                JxButton { width: 24; height: 22; text: "▼"; fontSize: 9; enabledLook: qrow.index < automix.queue.count - 1; onClicked: automix.move(qrow.index, qrow.index + 1) }
                                JxButton { width: 24; height: 22; text: "⤒"; fontSize: 10; onClicked: automix.move(qrow.index, 0) }
                                JxButton { width: 24; height: 22; text: "✕"; fontSize: 10; accent: Theme.red; onClicked: automix.removeAt(qrow.index) }
                            }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: queueList.count === 0
                        horizontalAlignment: Text.AlignHCenter
                        text: "La cola está vacía.\nArrastra pistas aquí, usa «＋» en la biblioteca o «Agregar lo visible»."
                        color: Theme.faint; font.pixelSize: 13
                    }
                }
            }

            // ---------- Programación de eventos
            Rectangle {
                visible: root.leftView === 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Theme.bg2
                border.color: Theme.border
                ListView {
                    id: evList
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    spacing: 4
                    model: scheduler.events
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    footer: Text {
                        visible: scheduler.lastMessage.length > 0
                        height: visible ? 26 : 0
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 6
                        text: "Último disparo: " + scheduler.lastMessage
                        color: Theme.green; font.pixelSize: 11
                    }
                    delegate: Rectangle {
                        id: ev
                        required property int index
                        required property var modelData
                        width: evList.width - 10
                        height: 34
                        radius: 6
                        color: ev.modelData.enabled ? Theme.panel2 : "transparent"
                        border.color: Theme.border
                        opacity: ev.modelData.enabled ? 1.0 : 0.55
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 6
                            spacing: 8
                            Rectangle {     // activado
                                width: 30; height: 16; radius: 8
                                color: ev.modelData.enabled ? Qt.rgba(0.33, 0.88, 0.54, 0.35) : Theme.bg2
                                border.color: ev.modelData.enabled ? Theme.green : Theme.border
                                Rectangle { width: 12; height: 12; radius: 6; y: 2; x: ev.modelData.enabled ? 16 : 2
                                            color: ev.modelData.enabled ? Theme.green : Theme.faint }
                                MouseArea { anchors.fill: parent; anchors.margins: -4; cursorShape: Qt.PointingHandCursor
                                            onClicked: scheduler.setEventEnabled(ev.index, !ev.modelData.enabled) }
                            }
                            Rectangle {     // hora
                                Layout.preferredWidth: 62; Layout.preferredHeight: 26; radius: 6
                                color: Theme.bg2; border.color: timeIn.activeFocus ? Theme.gold : Theme.border
                                TextInput {
                                    id: timeIn
                                    anchors.fill: parent
                                    horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
                                    text: ev.modelData.time
                                    inputMask: "99:99"
                                    color: Theme.gold; font.pixelSize: 15; font.weight: Font.Bold; font.family: "monospace"
                                    selectByMouse: true
                                    onEditingFinished: if (text !== ev.modelData.time) scheduler.setEventTime(ev.index, text)
                                }
                            }
                            JxButton {      // acción
                                Layout.fillWidth: true; Layout.preferredHeight: 26
                                text: ev.modelData.actionName + "  ▾"; fontSize: 12
                                onClicked: { root.editRow = ev.index; actionMenu.popup() }
                            }
                            JxButton {      // parámetro (lista o duración del fundido)
                                visible: ev.modelData.paramLabel.length > 0
                                Layout.preferredWidth: 150; Layout.preferredHeight: 26
                                text: ev.modelData.paramLabel + "  ▾"; fontSize: 11; accent: Theme.cyan
                                onClicked: { root.editRow = ev.index; (ev.modelData.action === 2 ? playlistPick : fadePick).popup() }
                            }
                            JxButton { Layout.preferredWidth: 58; Layout.preferredHeight: 26; text: ev.modelData.once ? "1 vez" : "diario"; fontSize: 10
                                       accent: Theme.gold; active: ev.modelData.once; onClicked: scheduler.setEventOnce(ev.index, !ev.modelData.once) }
                            JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 26; text: "▶"; fontSize: 11; accent: Theme.green
                                       onClicked: scheduler.runNow(ev.index) }
                            JxButton { Layout.preferredWidth: 30; Layout.preferredHeight: 26; text: "✕"; fontSize: 11; accent: Theme.red
                                       onClicked: scheduler.removeEvent(ev.index) }
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: evList.count === 0
                        horizontalAlignment: Text.AlignHCenter
                        text: "No hay eventos programados.\nEj.: 21:00 reproducir la lista «Cena» · 23:30 empezar a grabar · 02:00 bajar volumen y parar.\nPulsa «Nuevo evento» para crear uno."
                        color: Theme.faint; font.pixelSize: 13
                    }
                }
            }
        }

        Rectangle { Layout.fillHeight: true; width: 1; color: Theme.border }


        // ---------- Controles (desplazables si el panel es bajo)
        Flickable {
            Layout.preferredWidth: 244
            Layout.maximumWidth: 244
            Layout.fillHeight: true
            clip: true
            contentHeight: ctl.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
        ColumnLayout {
            id: ctl
            width: 236
            spacing: 8

            Rectangle {   // gran botón de encendido
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 10
                color: automix.enabled ? Qt.rgba(0.33, 0.88, 0.54, 0.18) : Theme.panel2
                border.color: automix.enabled ? Theme.green : Theme.border
                border.width: automix.enabled ? 2 : 1
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Rectangle { width: 12; height: 12; radius: 6; anchors.verticalCenter: parent.verticalCenter
                                color: automix.enabled ? Theme.green : Theme.faint
                                SequentialAnimation on opacity { running: automix.mixing; loops: Animation.Infinite
                                    NumberAnimation { to: 0.3; duration: 300 } NumberAnimation { to: 1; duration: 300 } } }
                    Text { text: "AUTOMIX"; color: automix.enabled ? Theme.green : Theme.text; font.pixelSize: 18; font.weight: Font.Black; font.letterSpacing: 2 }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: automix.toggle() }
            }

            Text { Layout.fillWidth: true; text: automix.status; color: automix.mixing ? Theme.gold : Theme.dim
                   font.pixelSize: 12; font.weight: Font.DemiBold; elide: Text.ElideRight }
            Rectangle {   // progreso de la mezcla
                Layout.fillWidth: true; Layout.preferredHeight: 6; radius: 3; color: Theme.bg2
                Rectangle {
                    height: parent.height; radius: 3
                    width: parent.width * (automix.mixing ? automix.mixProgress : 0)
                    gradient: Gradient { orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Theme.deckColor(automix.activeDeck) }
                        GradientStop { position: 1; color: Theme.deckColor(1 - automix.activeDeck) } }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                JxButton { Layout.fillWidth: true; Layout.preferredHeight: 30; text: "⏩  Mezclar ya"; accent: Theme.gold
                           enabledLook: automix.enabled && !automix.mixing; onClicked: automix.mixNow() }
                JxButton { Layout.preferredWidth: 80; Layout.preferredHeight: 30; text: "Saltar"; fontSize: 11
                           enabledLook: automix.enabled; onClicked: automix.skipNext() }
            }

            Text { text: "DURACIÓN DE LA MEZCLA"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2; Layout.topMargin: 4 }
            Row {
                spacing: 4
                Repeater {
                    model: [0, 8, 16, 32]
                    JxButton { width: 54; height: 26; fontSize: 11; text: modelData === 0 ? "AUTO" : modelData + " b"; accent: Theme.cyan
                               active: automix.transitionBeats === modelData; onClicked: automix.transitionBeats = modelData }
                }
            }

            Text { text: "ESTILO DE TRANSICIÓN"; color: Theme.faint; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2; Layout.topMargin: 2 }
            Grid {
                columns: 3
                spacing: 4
                Repeater {
                    model: automix.styleNames
                    JxButton { width: 74; height: 26; fontSize: 10; text: modelData.toUpperCase(); accent: Theme.magenta
                               active: automix.style === index; onClicked: automix.style = index }
                }
            }

            component Toggle: Rectangle {
                id: tg
                property string label: ""
                property bool on: false
                signal toggled()
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: "transparent"
                Rectangle {
                    id: sw; width: 30; height: 16; radius: 8; anchors.verticalCenter: parent.verticalCenter
                    color: tg.on ? Qt.rgba(0.96, 0.73, 0.26, 0.35) : Theme.bg2; border.color: tg.on ? Theme.gold : Theme.border
                    Rectangle { width: 12; height: 12; radius: 6; y: 2; x: tg.on ? 16 : 2; color: tg.on ? Theme.gold : Theme.faint
                                Behavior on x { NumberAnimation { duration: 120 } } }
                }
                Text { anchors.left: sw.right; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
                       text: tg.label; color: Theme.text; font.pixelSize: 12 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: tg.toggled() }
            }
            Toggle { label: "Intercambiar graves"; on: automix.bassSwap; onToggled: automix.bassSwap = !automix.bassSwap }
            Toggle { label: "Volver al tempo original"; on: automix.tempoReturn; onToggled: automix.tempoReturn = !automix.tempoReturn }
            Toggle { label: "Repetir la cola"; on: automix.repeat; onToggled: automix.repeat = !automix.repeat }

            Text {
                Layout.fillWidth: true
                Layout.bottomMargin: 6
                wrapMode: Text.WordWrap
                text: ["Auto: mezcla si los tempos encajan; si no, eco.", "Mezcla: crossfade sincronizado con intercambio de graves.",
                       "Filtro: la que sale se queda sin graves mientras la nueva se abre.", "Eco: la que sale se repite un compás y la nueva entra de golpe.",
                       "Corte: cambio seco en el compás justo."][automix.style] + "  Ctrl+M on/off · Ctrl+Intro mezclar ya."
                color: Theme.faint; font.pixelSize: 10
            }
        }
        }
    }
}
