// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <atomic>
#include <memory>
#include <vector>

namespace jx {

struct Sample {
    QString name;
    std::vector<float> pcm;     // estéreo intercalado, frecuencia del motor
    int64_t frames = 0;
};

// Sampler de 8 pads mezclado al master. Control sin bloqueos: contadores de disparo atómicos.
class Sampler {
public:
    static constexpr int kSlots = 8;
    enum Mode { OneShot = 0, Loop = 1, Hold = 2 };

    void prepare(double sampleRate) { m_sr = sampleRate; }

    // Control (hilo de UI). El dueño mantiene viva la muestra (ver SamplerController).
    void setSample(int slot, const Sample* s);
    void trigger(int slot) { m_slots[slot].triggers.fetch_add(1, std::memory_order_release); }
    void stop(int slot) { m_slots[slot].stops.fetch_add(1, std::memory_order_release); }

    std::atomic<int> mode[kSlots] = {};
    std::atomic<float> gain[kSlots];
    std::atomic<float> volume{0.8f};
    std::atomic<bool> pfl{false};

    // Estado publicado
    bool isPlaying(int slot) const { return m_slots[slot].playingOut.load(std::memory_order_relaxed); }
    double progress(int slot) const { return m_slots[slot].progressOut.load(std::memory_order_relaxed); }

    // Hilo de audio: suma al master y (si pfl) a los audífonos
    void process(float* master, float* cue, int n);

    Sampler() { for (auto& g : gain) g.store(1.f); }

private:
    struct Slot {
        std::atomic<const Sample*> sample{nullptr};
        std::atomic<unsigned> triggers{0}, stops{0};
        std::atomic<bool> playingOut{false};
        std::atomic<double> progressOut{0};
        // hilo de audio
        const Sample* cur = nullptr;
        unsigned seenTriggers = 0, seenStops = 0;
        int64_t pos = 0;
        bool playing = false;
        float fade = 1.f;          // micro-fundido al reiniciar/parar
        bool fadingOut = false;
    };
    Slot m_slots[kSlots];
    double m_sr = 48000.0;
};

// Sonidos incluidos, sintetizados (sin archivos externos ni licencias de terceros).
std::unique_ptr<Sample> makeBuiltinSample(int index, double sampleRate);
int builtinSampleCount();

} // namespace jx
