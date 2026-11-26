// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <cstdint>
#include <vector>

namespace jx {

// Pista decodificada y analizada. Inmutable una vez cargada (se comparte entre hilos).
struct Track {
    QString path;
    QString title;
    QString artist;
    QString album;
    QString genre;
    QString musicalKey;

    double sampleRate = 48000.0;      // igual a la frecuencia del motor
    int64_t frames = 0;               // cuadros estéreo
    std::vector<float> pcm;           // estéreo intercalado L,R,L,R...

    double bpm = 0.0;                 // 0 = desconocido
    double firstBeat = 0.0;           // en cuadros
    double beatLength() const { return bpm > 0 ? sampleRate * 60.0 / bpm : 0.0; }

    // Forma de onda por bandas (0..255) cada wfBinFrames cuadros
    int wfBinFrames = 256;
    std::vector<uint8_t> wfLow, wfMid, wfHigh;

    double durationSec() const { return frames / sampleRate; }
};

} // namespace jx
