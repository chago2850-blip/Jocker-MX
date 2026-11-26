// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>
#include <atomic>
#include <vector>

namespace jx {

// Motor de inferencia propio para la red U-Net de Spleeter (pesos de Deezer, MIT).
// Sin dependencias externas salvo Eigen (MPL-2.0, sólo cabeceras). Tensores NHWC, lote 1.
class StemModel {
public:
    static constexpr int kT = 512;     // cuadros STFT por bloque
    static constexpr int kF = 1024;    // bins de frecuencia de entrada

    bool load(const QString& path, QString* error);
    bool isLoaded() const { return !m_nets.empty(); }
    QStringList names() const { return m_names; }
    int count() const { return static_cast<int>(m_nets.size()); }

    // in: kT × kF × 2 (magnitudes L/R). out[i]: estimación del instrumento i, misma forma.
    // Ejecuta los instrumentos en paralelo con hasta `threads` hilos.
    void run(const float* in, std::vector<std::vector<float>>& out, int threads,
             const std::atomic<bool>* cancel = nullptr) const;

    static QString findModelFile();      // busca jx-stems-4.bin junto a la app / datos

private:
    struct Tensor { std::vector<int> shape; std::vector<float> data; };
    struct Layer { Tensor kernel, bias, scale, shift; };
    struct Net { Layer enc[6], dec[6]; Tensor finalK, finalB; };
    void runNet(const Net& n, const float* in, std::vector<float>& out) const;

    std::vector<Net> m_nets;
    QStringList m_names;
};

} // namespace jx
