// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Vista de karaoke: gráficos CD+G, o la letra .lrc con barrido de color, o una pantalla de espera.
Rectangle {
    id: kv
    property var deck: null
    property real scale: 1.0              // 1 = vista previa; 2–3 = pantalla del público
    property bool showBanner: true
    readonly property bool cdg: deck !== null && deck.hasCdg
    readonly property bool lyrics: deck !== null && !cdg && deck.hasLyrics
    readonly property var next: karaoke.singers.length ? karaoke.singers[0] : null
    color: "black"
    clip: true

    CdgItem {
        anchors.fill: parent
        anchors.bottomMargin: kv.showBanner ? banner.height : 0
        visible: kv.cdg
        deck: kv.cdg ? kv.deck : null
        smooth: kv.scale > 1.5
    }

    // ---- Letra con barrido
    Item {
        id: lyr
        visible: kv.lyrics
        anchors.fill: parent
        anchors.bottomMargin: kv.showBanner ? banner.height : 0
        readonly property int cur: kv.lyrics ? kv.deck.lyricLine : -1
        readonly property var lines: kv.lyrics ? kv.deck.lyricsLines : []
        Column {
            anchors.centerIn: parent
            width: parent.width * 0.92
            spacing: 14 * kv.scale
            Text {
                width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                text: lyr.cur > 0 ? lyr.lines[lyr.cur - 1] : ""
                color: "#5c5a78"; font.pixelSize: 15 * kv.scale
            }
            Item {    // línea actual: base blanca + copia dorada recortada según el avance
                width: parent.width
                height: base.implicitHeight
                readonly property string txt: lyr.cur >= 0 ? lyr.lines[lyr.cur]
                                              : (lyr.lines.length ? "♪  " + lyr.lines[0] : "")
                Text {
                    id: base
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: parent.txt; color: "white"
                    font.pixelSize: 26 * kv.scale; font.weight: Font.Bold
                    width: Math.min(implicitWidth, parent.width); elide: Text.ElideRight
                }
                Item {
                    x: base.x; y: base.y
                    height: base.height
                    width: base.width * (lyr.cur >= 0 ? kv.deck.lyricProgress : 0)
                    clip: true
                    Text {
                        text: base.text; color: Theme.gold
                        font: base.font; width: base.width; elide: Text.ElideRight
                        style: Text.Outline; styleColor: Qt.rgba(1, 0.4, 0.6, 0.35)
                    }
                }
            }
            Repeater {
                model: 2
                Text {
                    width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    readonly property int i: lyr.cur + 1 + index
                    text: i >= 0 && i < lyr.lines.length ? lyr.lines[i] : ""
                    color: index === 0 ? "#c9c7e8" : "#6f6c90"
                    font.pixelSize: (index === 0 ? 20 : 16) * kv.scale
                }
            }
        }
    }

    // ---- Espera
    Column {
        visible: !kv.cdg && !kv.lyrics
        anchors.centerIn: parent
        anchors.verticalCenterOffset: kv.showBanner ? -banner.height / 2 : 0
        spacing: 10 * kv.scale
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "🎤  JOCKER-MX  KARAOKE"
            color: Theme.gold; font.pixelSize: 26 * kv.scale; font.weight: Font.Black; font.letterSpacing: 3 * kv.scale
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: kv.deck && kv.deck.loaded ? "Esta canción no tiene gráficos ni letra (.cdg / .lrc)"
                  : (kv.next ? "Prepárate, " + kv.next.name + "…" : "Pide tu canción al DJ")
            color: "#c9c7e8"; font.pixelSize: 16 * kv.scale
        }
    }

    // ---- Quién canta / quién sigue
    Rectangle {
        id: banner
        visible: kv.showBanner
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 30 * kv.scale
        color: "#14122a"
        Text {
            anchors.left: parent.left; anchors.leftMargin: 12 * kv.scale; anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.58; elide: Text.ElideRight
            text: karaoke.currentName.length ? "🎤  " + karaoke.currentName + (karaoke.currentTitle.length ? "  ·  " + karaoke.currentTitle : "") : ""
            color: Theme.magenta; font.pixelSize: 14 * kv.scale; font.weight: Font.Bold
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 12 * kv.scale; anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.4; horizontalAlignment: Text.AlignRight; elide: Text.ElideLeft
            text: kv.next ? "Siguiente: " + kv.next.name : ""
            color: Theme.cyan; font.pixelSize: 13 * kv.scale
        }
    }
}
