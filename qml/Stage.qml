// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

// Estado compartido de la salida al público (pantalla aparte) y de la mezcla de video.
QtObject {
    property bool audienceVisible: false
    property int videoTransition: 0        // 0 fundido, 1 barrido, 2 corte
    property bool videoFill: true          // llenar la pantalla (recortando) o ajustar con bandas
    readonly property var transitionNames: ["Fundido", "Barrido", "Corte"]

    // Deck del karaoke: el del turno actual; si no, el que suena con gráficos o letra.
    function karaokeDeck() {
        if (karaoke.karaokeDeck === 0) return deckA
        if (karaoke.karaokeDeck === 1) return deckB
        const ka = deckA.hasCdg || deckA.hasLyrics, kb = deckB.hasCdg || deckB.hasLyrics
        if (kb && deckB.playing && !(ka && deckA.playing)) return deckB
        if (ka) return deckA
        if (kb) return deckB
        return null
    }
}
