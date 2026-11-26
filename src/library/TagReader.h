// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

namespace jx {

// Metadatos básicos de un archivo de audio.
struct TrackTags {
    QString title, artist, album, genre, key, year, comment;
    double bpm = 0.0;          // etiqueta TBPM / BPM (no el análisis)
    double duration = 0.0;     // segundos (estimado sin decodificar)
    bool ok = false;           // se pudo leer el archivo
};

// Lector propio, sin dependencias: ID3v1/ID3v2.2-2.4 (MP3 y WAV), Vorbis comment (FLAC)
// y LIST/INFO (WAV). Rápido: sólo lee encabezados.
TrackTags readTags(const QString& path);

} // namespace jx
