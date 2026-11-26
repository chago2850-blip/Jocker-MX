// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "StemModel.h"

#include <QString>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace jx {

// Stems de una pista a la frecuencia del motor. Se guardan voz, batería y bajo (int16 estéreo);
// "melodía/otros" = mezcla − (voz + batería + bajo), porque las máscaras suman 1.
struct StemData {
    enum { Vocals = 0, Drums = 1, Bass = 2, Other = 3, Count = 4 };
    int64_t frames = 0;
    std::vector<int16_t> stem[3];
    static constexpr float kScale = 1.f / 32767.f;
};

// Separa `pcm` (estéreo intercalado a `sampleRate`) con el modelo. Bloquea el hilo que la llama.
// progress recibe 0..1. Devuelve nullptr si se cancela o falla.
std::unique_ptr<StemData> separateStems(const StemModel& model, const float* pcm, int64_t frames, double sampleRate,
                                        int threads, const std::function<void(double)>& progress,
                                        const std::atomic<bool>* cancel, QString* error);

} // namespace jx
