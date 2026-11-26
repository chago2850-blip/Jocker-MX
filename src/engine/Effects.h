// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <vector>

namespace jx {

// Unidad de efectos de un canal, sincronizada al beat del deck. Implementación propia.
class EffectUnit {
public:
    enum Type { None = 0, Echo, Reverb, Flanger, Phaser, Crusher, Gate, Count };

    void prepare(double sampleRate);

    // Parámetros (escritos por la UI)
    std::atomic<int> type{None};
    std::atomic<float> wet{0.5f};      // 0..1 cantidad / mezcla
    std::atomic<float> beats{1.0f};    // duración musical: ¼ … 8 beats

    // Sincronía (la escribe el motor cada bloque): posición en beats y duración de un beat en muestras de salida
    // Procesa en sitio un bloque estéreo intercalado. Hilo de audio.
    void process(float* buf, int n, double beatPos, double beatSamples);

    static const char* name(int t);

private:
    void reset();
    double m_sr = 48000.0;
    int m_lastType = None;
    float m_wetSmooth = 0.f;

    // Eco
    std::vector<float> m_delay;     // estéreo intercalado
    int m_delayW = 0;
    float m_fbLp[2] = {0, 0};
    // Flanger
    std::vector<float> m_short;
    int m_shortW = 0;
    // Phaser
    float m_apState[2][6] = {};
    float m_phaserFb[2] = {0, 0};
    // Reverb (estilo Freeverb: 8 combs + 4 allpass por canal)
    struct Comb { std::vector<float> b; int i = 0; float store = 0; };
    struct Allpass { std::vector<float> b; int i = 0; };
    Comb m_combs[2][8];
    Allpass m_aps[2][4];
    // Bitcrusher
    float m_hold[2] = {0, 0};
    double m_holdPhase = 0;
    // Gate
    float m_gateEnv = 1.f;
};

} // namespace jx
