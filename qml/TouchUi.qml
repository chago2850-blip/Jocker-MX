// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX
import "nav.js" as Nav

// Interfaz táctil (teléfonos, tabletas y TV Box): barra superior fina, una página a la vez y
// pestañas grandes abajo. Se acomoda en horizontal y en vertical. Botón «Atrás» de Android/TV:
// vuelve a los decks. Con el control remoto, las flechas mueven el foco y OK pulsa.
Item {
    id: t
    signal openMidi()
    property int page: startupPanel === "biblioteca" ? 1 : startupPanel === "automix" ? 2 : startupPanel === "sampler" ? 3
                     : startupPanel === "karaoke" ? 4 : (startupPanel === "video" || startupPanel === "envivo" || startupPanel === "radio" || startupPanel === "mas") ? 5 : 0
    readonly property bool landscape: width >= height
    focus: true

    Component.onCompleted: {
        if (startupPanel === "video") more.sub = 1
        else if (startupPanel === "envivo") more.sub = 2
        else if (startupPanel === "radio") more.sub = 3
        navRep.itemAt(t.page).forceActiveFocus()
    }

    // «Atrás»: cierra la subpágina o vuelve a los decks; en los decks hay que pulsarlo dos veces para salir
    // (que no se cierre la app por accidente en plena fiesta).
    Keys.onReleased: function(e) {
        if (e.key !== Qt.Key_Back && e.key !== Qt.Key_Escape) return
        e.accepted = true
        if (t.page === 5 && more.back()) return
        if (t.page !== 0) { t.page = 0; return }
        if (e.key === Qt.Key_Back) {
            if (exitTimer.running) Qt.quit()
            else exitTimer.start()
        }
    }
    Timer { id: exitTimer; interval: 2200 }
    Rectangle {
        z: 10
        visible: exitTimer.running
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 72
        width: exitTxt.implicitWidth + 32; height: 40; radius: 20
        color: "#e6201733"; border.color: Theme.gold
        Text { id: exitTxt; anchors.centerIn: parent; text: "Pulsa «Atrás» otra vez para salir"; color: Theme.text; font.pixelSize: 14 }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Barra superior
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#0f0a1c"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                spacing: 8
                Image { source: "qrc:/JockerMX/resources/icon-256.png"; Layout.preferredWidth: 28; Layout.preferredHeight: 28; fillMode: Image.PreserveAspectFit; smooth: true }
                Text {
                    text: "JOCKER<font color='" + Theme.magenta + "'>-MX</font>"; textFormat: Text.StyledText
                    color: Theme.gold; font.pixelSize: 16; font.weight: Font.Black; font.letterSpacing: 1
                    visible: t.width > 420
                }
                Item { Layout.fillWidth: true }
                component Pill: Rectangle {
                    property string label: ""
                    property color tint: Theme.green
                    property bool on: false
                    signal tapped()
                    visible: on
                    Layout.preferredHeight: 26; Layout.preferredWidth: pl.implicitWidth + 18
                    radius: 13
                    color: Qt.rgba(tint.r, tint.g, tint.b, 0.18); border.color: tint
                    Text { id: pl; anchors.centerIn: parent; text: parent.label; color: parent.tint; font.pixelSize: 11; font.weight: Font.Black }
                    MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: parent.tapped() }
                }
                Pill { label: automix.mixing ? "AUTO · mezclando" : "AUTO"; on: automix.enabled; tint: Theme.green; onTapped: t.page = 2 }
                Pill { label: "REC " + Theme.formatTime(recorder.seconds).slice(0, 5); on: recorder.recording; tint: Theme.red; onTapped: { t.page = 5; more.sub = 0 } }
                Pill { label: "📻 " + radio.listeners; on: radio.running; tint: Theme.green; onTapped: { t.page = 5; more.sub = 3 } }
                Pill { label: broadcast.state === 2 ? "EN VIVO" : "RECONECTANDO"; on: broadcast.onAir; tint: broadcast.state === 2 ? Theme.red : Theme.gold
                       onTapped: { t.page = 5; more.sub = 2 } }
                VuMeter { Layout.preferredWidth: 46; Layout.preferredHeight: 8; level: Math.max(deckA.level, deckB.level); visible: t.width > 360 }
                Text { id: clock; color: Theme.dim; font.pixelSize: 13; font.family: "monospace"
                       Timer { interval: 10000; running: true; repeat: true; triggeredOnStart: true
                               onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm") } }
            }
        }

        // ---- Páginas
        StackLayout {
            id: pages
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
            currentIndex: t.page

            // 0 · Decks + mezclador
            Item {
                GridLayout {
                    anchors.fill: parent
                    flow: t.landscape ? GridLayout.LeftToRight : GridLayout.TopToBottom
                    rows: t.landscape ? 1 : 3
                    columns: t.landscape ? 3 : 1
                    rowSpacing: 6; columnSpacing: 6
                    TouchDeck { deck: deckA; index: 0; Layout.fillWidth: true; Layout.fillHeight: true; onOpenLibrary: t.page = 1 }
                    TouchMixer {
                        horizontal: t.landscape
                        Layout.fillHeight: t.landscape
                        Layout.fillWidth: !t.landscape
                        Layout.preferredWidth: t.landscape ? Math.min(250, Math.max(190, t.width * 0.22)) : -1
                        Layout.preferredHeight: t.landscape ? -1 : 150
                    }
                    TouchDeck { deck: deckB; index: 1; Layout.fillWidth: true; Layout.fillHeight: true; onOpenLibrary: t.page = 1 }
                }
            }
            TouchLibrary { onLoaded: t.page = 0 }
            TouchAutomix {}
            TouchPads {}
            TouchKaraoke {}
            TouchMore { id: more; onOpenMidi: t.openMidi() }
        }

        // ---- Pestañas
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            color: "#0f0a1c"
            Row {
                id: navRow
                anchors.fill: parent
                anchors.margins: 5
                spacing: 5
                Repeater {
                    id: navRep
                    model: [["🎛", "Decks"], ["🎵", "Música"], ["🔁", "Automix"], ["🥁", "Pads"], ["🎤", "Karaoke"], ["⋯", "Más"]]
                    TouchButton {
                        width: (navRow.width - 25) / 6
                        height: navRow.height
                        text: modelData[0]
                        sub: t.width > 520 ? modelData[1] : ""
                        fontSize: 18
                        accent: Theme.gold
                        active: t.page === index
                        onClicked: { if (index === 5 && t.page === 5) more.sub = 0; t.page = index }
                    }
                }
            }
        }
    }
}
