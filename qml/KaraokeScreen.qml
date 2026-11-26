// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Pantalla del público (TV, proyector o segundo monitor).
// Muestra el karaoke si hay un cantante o una pista con gráficos/letra; si no, la mezcla de video;
// si no hay nada, la pantalla de espera. Doble clic: pantalla completa · Esc: salir de ella.
Window {
    id: win
    title: "Jocker-MX · Pantalla del público"
    width: 960; height: 540
    color: "black"
    visible: Stage.audienceVisible
    onClosing: Stage.audienceVisible = false
    readonly property var kdeck: Stage.karaokeDeck()
    readonly property bool showKaraoke: kdeck !== null && (karaoke.karaokeDeck >= 0 || kdeck.playing)
    readonly property bool showVideo: !showKaraoke && (deckA.hasVideo || deckB.hasVideo)
    readonly property var dA: deckA
    readonly property var dB: deckB

    VideoMixItem {
        anchors.fill: parent
        visible: win.showVideo
        deckA: win.showVideo ? win.dA : null
        deckB: win.showVideo ? win.dB : null
        crossfader: mixer.crossfader
        transition: Stage.videoTransition
        fill: Stage.videoFill
    }
    KaraokeView {
        anchors.fill: parent
        visible: !win.showVideo
        deck: win.showKaraoke ? win.kdeck : null
        scale: Math.max(1.4, Math.min(win.width / 480, win.height / 300))
    }
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: win.visibility = (win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen)
    }
    Shortcut { sequence: "Escape"; onActivated: win.visibility = Window.Windowed }
}
