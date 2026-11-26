// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <vector>

namespace jx {

struct Track;

// Secciones musicales de una pista, alineadas a compases (4 beats).
struct Section {
    enum Kind { Intro, Body, Break, Drop, Outro };
    Kind kind = Body;
    double start = 0;   // segundos de la pista
    double end = 0;
    int bars = 0;
    double energy = 0;  // 0..1 relativa a la parte más fuerte
    double bass = 0;    // 0..1 relativa
    static QString name(Kind k);
};

// Analiza la estructura con la forma de onda por bandas y el beatgrid.
// Sin BPM devuelve una lista vacía. Es rápido (milisegundos): se puede recalcular al corregir el beatgrid.
std::vector<Section> analyzeStructure(const Track& t);

// Ayudas para el Automix
const Section* firstOf(const std::vector<Section>& s, Section::Kind k);
const Section* lastOf(const std::vector<Section>& s, Section::Kind k);

} // namespace jx
