// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import JockerMX
import QtQuick.Controls

// Menú contextual con el estilo oscuro de Jocker-MX.
Menu {
    id: menu
    padding: 4
    implicitWidth: 230
    background: Rectangle {
        implicitWidth: 230
        color: Theme.panel2
        radius: 8
        border.color: Theme.border
    }
    delegate: JxMenuItem {}
}
