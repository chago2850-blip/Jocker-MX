// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import JockerMX

ApplicationWindow {
    id: win
    width: 1480
    height: 1000
    minimumWidth: uiPrefs.touch ? 320 : 1180
    minimumHeight: uiPrefs.touch ? 320 : 860
    visibility: uiPrefs.android ? Window.FullScreen : Window.AutomaticVisibility
    visible: true
    title: "Jocker-MX"
    color: Theme.bg

    // Atajos de teclado (estilo de los programas DJ más usados)
    Shortcut { sequence: "Q"; onActivated: deckA.playPause() }
    Shortcut { sequence: "P"; onActivated: deckB.playPause() }
    Shortcut { sequence: "W"; onActivated: deckA.sync() }
    Shortcut { sequence: "O"; onActivated: deckB.sync() }
    Shortcut { sequence: "1"; onActivated: deckA.hotCue(0) }
    Shortcut { sequence: "2"; onActivated: deckA.hotCue(1) }
    Shortcut { sequence: "3"; onActivated: deckA.hotCue(2) }
    Shortcut { sequence: "4"; onActivated: deckA.hotCue(3) }
    Shortcut { sequence: "7"; onActivated: deckB.hotCue(0) }
    Shortcut { sequence: "8"; onActivated: deckB.hotCue(1) }
    Shortcut { sequence: "9"; onActivated: deckB.hotCue(2) }
    Shortcut { sequence: "0"; onActivated: deckB.hotCue(3) }
    Shortcut { sequence: "A"; onActivated: deckA.autoLoop(4) }
    Shortcut { sequence: "L"; onActivated: deckB.autoLoop(4) }
    Shortcut { sequence: "Z"; onActivated: sampler.press(0) }
    Shortcut { sequence: "X"; onActivated: sampler.press(1) }
    Shortcut { sequence: "C"; onActivated: sampler.press(2) }
    Shortcut { sequence: "V"; onActivated: sampler.press(3) }
    Shortcut { sequence: "B"; onActivated: sampler.press(4) }
    Shortcut { sequence: "N"; onActivated: sampler.press(5) }
    Shortcut { sequence: "M"; onActivated: sampler.press(6) }
    Shortcut { sequence: ","; onActivated: sampler.press(7) }
    Shortcut { sequence: "Ctrl+R"; onActivated: recorder.toggle() }
    Shortcut { sequence: "Ctrl+M"; onActivated: automix.toggle() }
    Shortcut { sequence: "Ctrl+K"; onActivated: karaoke.startNext() }
    Shortcut { sequences: ["Ctrl+Return", "Ctrl+Enter"]; onActivated: automix.mixNow() }
    Shortcut { sequence: "Shift+Q"; onActivated: deckA.acapella() }
    Shortcut { sequence: "Shift+W"; onActivated: deckA.instrumental() }
    Shortcut { sequence: "Shift+O"; onActivated: deckB.acapella() }
    Shortcut { sequence: "Shift+P"; onActivated: deckB.instrumental() }
    Shortcut { sequence: "Left"; onActivated: mixer.crossfader = Math.max(-1, mixer.crossfader - 0.1) }
    Shortcut { sequence: "Right"; onActivated: mixer.crossfader = Math.min(1, mixer.crossfader + 0.1) }

    MidiDialog { id: midiDialog }
    Component.onCompleted: if (startupPanel === "midi") midiDialog.open()

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0; color: "#120b20" }
            GradientStop { position: 1; color: Theme.bg }
        }
    }

    // Interfaz: táctil (Android, pantallas pequeñas, TV) o de escritorio. Se puede cambiar en caliente.
    Loader {
        id: uiLoader
        anchors.fill: parent
        focus: true
        sourceComponent: uiPrefs.touch ? touchUi : desktopUi
    }
    Component { id: desktopUi; DesktopUi { onOpenMidi: midiDialog.open() } }
    Component { id: touchUi; TouchUi { onOpenMidi: midiDialog.open() } }

    KaraokeScreen {}   // pantalla del público (karaoke / video)
}
