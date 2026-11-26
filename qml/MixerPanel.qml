// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Layouts

// Mezclador de 2 canales: gain, EQ de 3 bandas con kill, filtro, fader, PFL, crossfader y master.
Rectangle {
    id: root
    color: Theme.panel
    radius: 12
    border.color: Theme.border

    component ChannelFader: Item {
        id: cf
        property real value: 0.85
        property color accent: Theme.gold
        signal moved(real v)
        implicitWidth: 30
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 4; radius: 2; color: Theme.bg; border.color: Theme.border
                    anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.topMargin: 9; anchors.bottomMargin: 9 }
        Rectangle {   // relleno de nivel
            anchors.horizontalCenter: parent.horizontalCenter; width: 4; radius: 2
            color: cf.accent; opacity: 0.6
            anchors.bottom: parent.bottom; anchors.bottomMargin: 9
            height: (cf.height - 18) * cf.value
        }
        Rectangle {
            id: capF
            width: cf.width; height: 16; radius: 3
            y: 9 + (1 - cf.value) * (cf.height - 18) - height / 2
            gradient: Gradient { GradientStop { position: 0; color: "#5a4c80" } GradientStop { position: 1; color: "#2a2142" } }
            border.color: Theme.faint
            Rectangle { anchors.centerIn: parent; width: parent.width - 8; height: 2; color: Theme.text }
        }
        MouseArea {
            anchors.fill: parent
            onPositionChanged: function(m) { cf.moved(Math.max(0, Math.min(1, 1 - (m.y - 9) / (cf.height - 18)))) }
            onPressed: function(m) { cf.moved(Math.max(0, Math.min(1, 1 - (m.y - 9) / (cf.height - 18)))) }
            onWheel: function(w) { cf.moved(Math.max(0, Math.min(1, cf.value + (w.angleDelta.y > 0 ? 0.05 : -0.05)))) }
        }
    }

    component Strip: ColumnLayout {
        id: st
        property var deck
        readonly property color accent: Theme.deckColor(deck.index)
        spacing: 0
        Text { Layout.alignment: Qt.AlignHCenter; text: st.deck.name; color: st.accent; font.pixelSize: 15; font.weight: Font.Black }
        Knob { Layout.alignment: Qt.AlignHCenter; size: 35; label: "GAIN"; value: st.deck.gain; accent: Theme.text; onMoved: function(v) { st.deck.gain = v } }
        Knob { Layout.alignment: Qt.AlignHCenter; size: 35; label: "HIGH"; value: st.deck.eqHigh; accent: st.deck.eqHigh <= -0.97 ? Theme.red : Theme.cyan; onMoved: function(v) { st.deck.eqHigh = v } }
        Knob { Layout.alignment: Qt.AlignHCenter; size: 35; label: "MID"; value: st.deck.eqMid; accent: st.deck.eqMid <= -0.97 ? Theme.red : Theme.gold; onMoved: function(v) { st.deck.eqMid = v } }
        Knob { Layout.alignment: Qt.AlignHCenter; size: 35; label: "LOW"; value: st.deck.eqLow; accent: st.deck.eqLow <= -0.97 ? Theme.red : Theme.magenta; onMoved: function(v) { st.deck.eqLow = v } }
        Knob { Layout.alignment: Qt.AlignHCenter; size: 35; label: "FILTER"; value: st.deck.filter; accent: Theme.green; onMoved: function(v) { st.deck.filter = v } }
        JxButton { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 46; Layout.preferredHeight: 24; text: "🎧"; fontSize: 12
                   accent: Theme.orange; active: st.deck.pfl; onClicked: st.deck.pfl = !st.deck.pfl }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            spacing: 6
            VuMeter { Layout.fillHeight: true; Layout.preferredWidth: 6; level: st.deck.level }
            ChannelFader { Layout.fillHeight: true; Layout.preferredWidth: 30; value: st.deck.volume; accent: st.accent; onMoved: function(v) { st.deck.volume = v } }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            Strip { deck: deckA; Layout.fillHeight: true; Layout.fillWidth: true }

            ColumnLayout {      // master
                Layout.fillHeight: true
                Layout.preferredWidth: 70
                spacing: 4
                Text { Layout.alignment: Qt.AlignHCenter; text: "MASTER"; color: Theme.gold; font.pixelSize: 10; font.weight: Font.Black; font.letterSpacing: 1 }
                Knob { Layout.alignment: Qt.AlignHCenter; size: 36; label: "VOL"; from: 0; to: 1; defaultValue: 0.85; value: mixer.masterVolume; accent: Theme.gold; onMoved: function(v) { mixer.masterVolume = v } }
                Knob { Layout.alignment: Qt.AlignHCenter; label: "CUE/MST"; from: 0; to: 1; defaultValue: 0.5; bipolar: false; value: mixer.cueMix; accent: Theme.orange; size: 30; onMoved: function(v) { mixer.cueMix = v } }
                Knob { Layout.alignment: Qt.AlignHCenter; label: "PHONES"; from: 0; to: 1; defaultValue: 0.8; value: mixer.headphoneVolume; accent: Theme.orange; size: 30; onMoved: function(v) { mixer.headphoneVolume = v } }
                JxButton { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 58; Layout.preferredHeight: 22; text: "SPLIT"; fontSize: 10
                           accent: Theme.orange; active: mixer.splitCue; onClicked: mixer.splitCue = !mixer.splitCue }
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 52; height: 16; radius: 4
                    color: mixer.limiting ? Theme.red : Theme.raised
                    Text { anchors.centerIn: parent; text: "LIMIT"; color: mixer.limiting ? "white" : Theme.faint; font.pixelSize: 9; font.weight: Font.Bold }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    spacing: 3
                    VuMeter { Layout.fillHeight: true; Layout.preferredWidth: 8; level: mixer.levelL; segments: 22 }
                    VuMeter { Layout.fillHeight: true; Layout.preferredWidth: 8; level: mixer.levelR; segments: 22 }
                }
            }

            Strip { deck: deckB; Layout.fillHeight: true; Layout.fillWidth: true }
        }

        // Crossfader
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Rectangle { anchors.verticalCenter: parent.verticalCenter; x: 14; width: parent.width - 28; height: 4; radius: 2; color: Theme.bg; border.color: Theme.border }
            Text { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; text: "A"; color: Theme.magenta; font.weight: Font.Black }
            Text { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "B"; color: Theme.cyan; font.weight: Font.Black }
            Rectangle {
                id: xcap
                width: 26; height: 30; radius: 4
                anchors.verticalCenter: parent.verticalCenter
                x: 14 + (mixer.crossfader + 1) / 2 * (parent.width - 28) - width / 2
                gradient: Gradient { GradientStop { position: 0; color: "#5a4c80" } GradientStop { position: 1; color: "#2a2142" } }
                border.color: Math.abs(mixer.crossfader) < 0.01 ? Theme.gold : Theme.faint
                Rectangle { anchors.centerIn: parent; width: 2; height: parent.height - 8; color: Theme.text }
            }
            MouseArea {
                anchors.fill: parent
                function setX(x) {
                    var v = (x - 14) / (width - 28) * 2 - 1
                    v = Math.max(-1, Math.min(1, v))
                    if (Math.abs(v) < 0.03) v = 0
                    mixer.crossfader = v
                }
                onPressed: function(m) { setX(m.x) }
                onPositionChanged: function(m) { setX(m.x) }
                onDoubleClicked: mixer.crossfader = 0
            }
        }
    }
}
