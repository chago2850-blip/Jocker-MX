// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#0a0712"
    readonly property color bg2: "#110c1c"
    readonly property color panel: "#161022"
    readonly property color panel2: "#1e1730"
    readonly property color raised: "#2a2140"
    readonly property color border: "#2f2549"
    readonly property color text: "#efe9f8"
    readonly property color dim: "#b0a7c8"
    readonly property color faint: "#7a7096"
    readonly property color gold: "#f5b942"
    readonly property color goldDeep: "#c98a2e"
    readonly property color magenta: "#ff2d8c"
    readonly property color cyan: "#3fe0ff"
    readonly property color green: "#55e08a"
    readonly property color red: "#ff4d5e"
    readonly property color orange: "#ff9a3c"
    readonly property var hotColors: ["#ff4646", "#ffa028", "#fae63c", "#50e678", "#3cc8ff", "#5a6eff", "#be5aff", "#ff5ac8"]

    function deckColor(i) { return i === 0 ? magenta : cyan }
    // Colores de las secciones: intro, cuerpo, break, drop, outro
    function sectionColor(k) { return ["#3fe0ff", "#6b7bff", "#55e08a", "#ff2d8c", "#f5b942"][k] || faint }

    function formatTime(sec) {
        if (sec < 0 || isNaN(sec)) sec = 0
        var m = Math.floor(sec / 60)
        var s = Math.floor(sec % 60)
        var d = Math.floor((sec * 10) % 10)
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s + "." + d
    }
}
