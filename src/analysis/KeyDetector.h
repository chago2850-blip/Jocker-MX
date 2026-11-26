// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <cstdint>

namespace jx {

struct KeyResult {
    int tonic = -1;          // 0 = Do (C) … 11 = Si (B); -1 = desconocida
    bool minor = false;
    double confidence = 0;   // correlación del mejor perfil
    QString camelot() const;         // "8A"
    QString name() const;            // "La menor"
    QString openKey() const;         // "1m"
};

// Detecta la tonalidad con un cromagrama (FFT) y perfiles de tonalidad (Krumhansl-Kessler / Temperley).
// pcm: estéreo intercalado.
KeyResult detectKey(const float* pcm, int64_t frames, double sampleRate);

// Utilidades de notación
KeyResult keyFromCamelot(const QString& camelot);
// Normaliza etiquetas de tonalidad comunes ("Am", "A minor", "8A", "F#", "Gbm", "1m") a Camelot; vacío si no se reconoce.
QString normalizeKeyToCamelot(const QString& text);
// Compatibilidad armónica en la rueda de Camelot (misma, ±1 o relativa mayor/menor).
bool keysCompatible(const QString& camelotA, const QString& camelotB);

} // namespace jx
