// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import JockerMX

// Mezclador táctil: EQ y filtro por deck, volúmenes y crossfader.
// horizontal = franja central entre los decks (pantalla apaisada); si no, banda entre deck A (arriba) y B (abajo).
Rectangle {
    id: root
    property bool horizontal: true
    radius: 12
    color: Theme.panel
    border.color: Theme.border
    // Tamaño de las perillas según el espacio: en la franja central caben dos columnas de EQ, dos faders y los VU.
    readonly property int knob: horizontal ? Math.max(24, Math.min(40, (height - 110) / 4.4, (width - 12 - 80 - 12 - 20) / 2))
                                           : Math.max(24, Math.min(40, (width - 40) / 8 - 4))

    component EqColumn: Column {
        id: eq
        property var deck
        property int idx: 0
        spacing: 2
        Repeater {
            model: [["AGU", "eqHigh"], ["MED", "eqMid"], ["GRA", "eqLow"], ["FIL", "filter"]]
            Knob {
                size: root.knob
                label: modelData[0]
                from: -1; to: 1; defaultValue: 0
                value: eq.deck[modelData[1]]
                accent: modelData[1] === "filter" ? Theme.cyan : Theme.deckColor(eq.idx)
                onMoved: function(v) { eq.deck[modelData[1]] = v }
            }
        }
    }
    component EqRow: Row {
        id: eqr
        property var deck
        property int idx: 0
        spacing: 4
        Repeater {
            model: [["AGU", "eqHigh"], ["MED", "eqMid"], ["GRA", "eqLow"], ["FIL", "filter"]]
            Knob {
                size: root.knob
                label: modelData[0]
                from: -1; to: 1; defaultValue: 0
                value: eqr.deck[modelData[1]]
                accent: modelData[1] === "filter" ? Theme.cyan : Theme.deckColor(eqr.idx)
                onMoved: function(v) { eqr.deck[modelData[1]] = v }
            }
        }
    }

    // ---- Franja central (apaisado)
    ColumnLayout {
        visible: root.horizontal
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            EqColumn { deck: deckA; idx: 0; Layout.alignment: Qt.AlignTop }
            TouchFader {
                Layout.fillHeight: true; Layout.preferredWidth: 40
                from: 0; to: 1; defaultValue: 1
                value: deckA.volume; accent: Theme.deckColor(0)
                onMoved: function(v) { deckA.volume = v }
            }
            VuMeter { Layout.fillHeight: true; Layout.preferredWidth: 6; level: deckA.level }
            VuMeter { Layout.fillHeight: true; Layout.preferredWidth: 6; level: deckB.level }
            TouchFader {
                Layout.fillHeight: true; Layout.preferredWidth: 40
                from: 0; to: 1; defaultValue: 1
                value: deckB.volume; accent: Theme.deckColor(1)
                onMoved: function(v) { deckB.volume = v }
            }
            EqColumn { deck: deckB; idx: 1; Layout.alignment: Qt.AlignTop }
        }
        TouchFader {
            Layout.fillWidth: true; Layout.preferredHeight: 44
            vertical: false; centerMark: true
            from: -1; to: 1; defaultValue: 0
            value: mixer.crossfader; accent: Theme.gold
            onMoved: function(v) { mixer.crossfader = v }
        }
    }

    // ---- Banda (vertical): EQ en fila, volúmenes horizontales y crossfader
    ColumnLayout {
        visible: !root.horizontal
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            EqRow { deck: deckA; idx: 0 }
            Item { Layout.fillWidth: true }
            EqRow { deck: deckB; idx: 1 }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            TouchFader {
                Layout.fillWidth: true; Layout.preferredHeight: 36
                vertical: false; from: 0; to: 1; defaultValue: 1
                value: deckA.volume; accent: Theme.deckColor(0); label: "VOL A"
                onMoved: function(v) { deckA.volume = v }
            }
            TouchFader {
                Layout.fillWidth: true; Layout.preferredHeight: 36
                vertical: false; from: 0; to: 1; defaultValue: 1
                value: deckB.volume; accent: Theme.deckColor(1); label: "VOL B"
                onMoved: function(v) { deckB.volume = v }
            }
        }
        TouchFader {
            Layout.fillWidth: true; Layout.preferredHeight: 42
            vertical: false; centerMark: true
            from: -1; to: 1; defaultValue: 0
            value: mixer.crossfader; accent: Theme.gold
            onMoved: function(v) { mixer.crossfader = v }
        }
    }
}
