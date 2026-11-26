// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX

// Pantalla del público (TV o segundo monitor). Doble clic: pantalla completa · Esc: salir de ella.
Window {
    id: win
    title: "Jocker-MX · Karaoke"
    width: 960; height: 600
    color: "black"
    property var deck: null
    KaraokeView {
        anchors.fill: parent
        deck: win.deck
        scale: Math.max(1.4, Math.min(win.width / 480, win.height / 300))
    }
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: win.visibility = (win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen)
    }
    Shortcut { sequence: "Escape"; onActivated: win.visibility = Window.Windowed }
}
