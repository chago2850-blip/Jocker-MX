// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

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

// Transición entre dos pistas concretas.
struct TransitionPlan {
    double outStart = 0;      // segundos de la pista que sale donde empieza la mezcla
    double inStart = 0;       // segundos de la pista que entra donde arranca
    double length = 0;        // duración, en segundos de la pista que sale
    int beats = 0;            // duración en beats (0 si no hay beatgrid)
    bool structural = false;  // calculada con intro/outro
    QString describe;         // texto para la interfaz ("outro 16 → intro 16 compases")
};

// beats = 0 → automático por estructura: la outro de la que sale se mezcla con la intro de la que entra,
// de forma que el cuerpo/drop de la nueva empiece justo cuando la anterior termina.
// Si no hay estructura útil (o no están sincronizadas) usa los puntos fijos ya calculados.
TransitionPlan planTransition(const Track& out, const MixPoints& outPts, const Track& in, const MixPoints& inPts,
                              int beats, bool synced);

// ¿Se pueden sincronizar los tempos dentro de maxStretch (0.08 = ±8 %), contando medio/doble tiempo?
bool tempoCompatible(double bpmA, double bpmB, double maxStretch = 0.08);

} // namespace jx
