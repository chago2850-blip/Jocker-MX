// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <vector>

namespace jx {

// Decodificador del sistema para formatos que miniaudio no lee (AAC/M4A/ALAC/WMA…):
//   macOS: AudioToolbox (ExtAudioFile) · Windows: Media Foundation · Linux: FFmpeg (libavcodec)
// Devuelve estéreo float intercalado a `sampleRate`.
bool platformDecode(const QString& path, double sampleRate, std::vector<float>& stereo, QString* error);

// Nombre del decodificador disponible en esta compilación ("" si no hay).
const char* platformDecoderName();

// Convierte audio intercalado de N canales y cualquier frecuencia a estéreo a `sampleRate` (miniaudio).
bool convertToStereo(const float* in, size_t frames, int channels, int rate, double sampleRate, std::vector<float>& out);

} // namespace jx
