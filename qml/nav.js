// SPDX-License-Identifier: GPL-3.0-or-later
// Navegación espacial con el control remoto (D-pad): mueve el foco al elemento más cercano
// en la dirección pulsada, entre los que aceptan foco y están visibles.
.pragma library

function move(item, dir) {
    if (!item) return false
    var p = item.mapToItem(null, item.width / 2, item.height / 2)
    var horiz = dir === "left" || dir === "right"
    var best = null, bestScore = 1e12, guard = 0
    var it = item.nextItemInFocusChain(true)
    while (it && it !== item && guard++ < 800) {
        if (it.visible && it.enabled && it.width > 0 && it.height > 0) {
            var q = it.mapToItem(null, it.width / 2, it.height / 2)
            var dx = q.x - p.x, dy = q.y - p.y
            var ok = dir === "left" ? dx < -4 : dir === "right" ? dx > 4 : dir === "up" ? dy < -4 : dy > 4
            if (ok) {
                var main = horiz ? Math.abs(dx) : Math.abs(dy)
                var cross = horiz ? Math.abs(dy) : Math.abs(dx)
                var s = main + cross * 2.5
                if (s < bestScore) { bestScore = s; best = it }
            }
        }
        it = it.nextItemInFocusChain(true)
    }
    if (best) best.forceActiveFocus(Qt.TabFocusReason)
    return best !== null
}

// Maneja flechas: devuelve true si movió el foco.
function handleArrows(item, event) {
    var d = event.key === Qt.Key_Left ? "left" : event.key === Qt.Key_Right ? "right"
          : event.key === Qt.Key_Up ? "up" : event.key === Qt.Key_Down ? "down" : ""
    if (!d) return false
    move(item, d)
    return true
}

function isOk(key) {
    return key === Qt.Key_Return || key === Qt.Key_Enter || key === Qt.Key_Select || key === Qt.Key_Space
}
