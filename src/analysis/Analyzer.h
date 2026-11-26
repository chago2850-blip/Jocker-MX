// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "engine/Track.h"

#include <QString>
#include <memory>
#include <vector>

namespace jx {

struct BeatInfo {
    double bpm = 0.0;
    double firstBeat = 0.0;   // cuadros
    double confidence = 0.0;
};

// Decodifica un archivo (WAV, FLAC, MP3) a estéreo float con la frecuencia del motor
// y calcula forma de onda + BPM/beatgrid. Seguro para llamarse desde un hilo de trabajo.
std::unique_ptr<Track> loadAndAnalyze(const QString& path, double sampleRate, QString* error);

// Sólo decodifica a estéreo float (para muestras del sampler).
// Decodificación con Qt Multimedia (respaldo; false si no se compiló con video).
bool qtMultimediaDecode(const QString& path, double sampleRate, std::vector<float>& pcm, QString* error);
bool decodeToStereo(const QString& path, double sampleRate, std::vector<float>& pcm, QString* error,
                    double maxSeconds = 600.0);

// Detección de tempo y fase del primer beat sobre audio estéreo intercalado.
BeatInfo detectBeats(const float* pcm, int64_t frames, double sampleRate);

// Calcula la forma de onda de 3 bandas.
void computeWaveform(Track& t);

// Separa "Artista - Título" del nombre de archivo.
void guessTitleArtist(const QString& path, QString* title, QString* artist);

} // namespace jx
