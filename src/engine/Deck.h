// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Track.h"

#include <atomic>
#include <memory>
#include <vector>

namespace RubberBand { class RubberBandStretcher; }

namespace jx {

// Reproductor de un deck. Los métodos process*/prepare se llaman SOLO desde el hilo de audio;
// los atómicos son la interfaz con el hilo de control (UI / MIDI).
class Deck {
public:
    explicit Deck(int index);
    ~Deck();

    void prepare(double sampleRate, int maxBlock);   // antes de iniciar el dispositivo

    // --- Control (cualquier hilo) ---
    void setTrack(const Track* t);                     // el dueño garantiza la vida del objeto
    const Track* track() const { return m_track.load(std::memory_order_acquire); }
    void requestSeek(double frame) { m_seekRequest.store(frame, std::memory_order_release); }

    std::atomic<bool>   playing{false};
    std::atomic<double> pitch{0.0};          // fracción: 0.08 = +8 %
    std::atomic<double> nudge{0.0};          // empujón temporal (±)
    std::atomic<bool>   keyLock{false};
    std::atomic<bool>   scratchActive{false};
    std::atomic<double> scratchRate{0.0};    // velocidad de plato durante scratch (puede ser negativa)

    std::atomic<float>  stemGain[4] = {1.f, 1.f, 1.f, 1.f};   // voz, batería, bajo, otros

    std::atomic<bool>   loopActive{false};
    std::atomic<double> loopStart{0.0};
    std::atomic<double> loopEnd{0.0};

    // --- Estado publicado por el hilo de audio ---
    std::atomic<double> position{0.0};       // cuadros
    std::atomic<bool>   reachedEnd{false};

    int index() const { return m_index; }

    // Llena `out` (estéreo intercalado, n cuadros). Hilo de audio.
    void process(float* out, int n);

    double baseRate() const { return 1.0 + pitch.load(std::memory_order_relaxed); }

private:
    void processResample(const Track* t, float* out, int n, double targetRate);
    void processStretch(const Track* t, float* out, int n, double rate);

    int m_index;
    double m_sr = 48000.0;
    std::atomic<const Track*> m_track{nullptr};
    std::atomic<double> m_seekRequest{-1.0};

    // Estado exclusivo del hilo de audio
    const Track* m_cur = nullptr;
    double m_pos = 0.0;
    double m_rate = 0.0;
    bool m_wasStretching = false;
    std::unique_ptr<RubberBand::RubberBandStretcher> m_rb;
    std::vector<float> m_inL, m_inR, m_outL, m_outR;
    int m_maxBlock = 4096;

    // Mezcla de stems (hilo de audio)
    inline float at(const Track* t, int64_t i, int ch) const;
    inline void stepStemGains();
    const StemData* m_sd = nullptr;
    float m_g[4] = {1.f, 1.f, 1.f, 1.f};
    float m_tg[4] = {1.f, 1.f, 1.f, 1.f};
    bool m_stemMix = false;
};

} // namespace jx
