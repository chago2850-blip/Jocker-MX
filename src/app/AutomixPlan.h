// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace jx {

struct Track;

// Puntos de mezcla de una pista para el Automix (todo en segundos de la pista).
struct MixPoints {
    double firstAudible = 0;   // primer sonido (salta silencio inicial)
    double lastAudible = 0;    // último sonido (ignora silencio final)
    double mixIn = 0;          // dónde arranca cuando entra (primer beat audible)
    double mixOut = 0;         // dónde empieza a salir (inicio de compás)
    double transition = 0;     // duración de la transición, en segundos de la pista
    bool beatAligned = false;  // hay beatgrid: entrada y salida caen en beat/compás
};

// Calcula los puntos a partir de la forma de onda y el beatgrid.
// transitionBeats: longitud deseada de la mezcla (p. ej. 16). fallbackSeconds: sin BPM.
MixPoints computeMixPoints(const Track& t, int transitionBeats, double fallbackSeconds = 8.0);

// ¿Se pueden sincronizar los tempos dentro de maxStretch (0.08 = ±8 %), contando medio/doble tiempo?
bool tempoCompatible(double bpmA, double bpmB, double maxStretch = 0.08);

} // namespace jx
