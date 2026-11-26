// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace jx {

// Grabador del master a WAV (PCM 24 bits). El hilo de audio sólo copia a un búfer circular
// sin bloqueos; un hilo aparte escribe al disco.
class Recorder {
public:
    Recorder() = default;
    ~Recorder();

    bool start(const QString& path, double sampleRate);   // hilo de control
    void stop();                                           // hilo de control (espera a vaciar el búfer)
    bool isRecording() const { return m_active.load(std::memory_order_acquire); }

    // Hilo de audio: estéreo intercalado
    void push(const float* stereo, int frames);

    double seconds() const { return m_written.load() / m_sr; }
    int64_t droppedFrames() const { return m_dropped.load(); }
    QString path() const { return m_path; }

private:
    void writerLoop();
    std::vector<float> m_ring;
    size_t m_capacity = 0;                    // en cuadros
    std::atomic<size_t> m_head{0}, m_tail{0}; // cuadros escritos / leídos
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_stopping{false};
    std::atomic<int64_t> m_written{0};
    std::atomic<int64_t> m_dropped{0};
    std::thread m_thread;
    QString m_path;
    double m_sr = 48000.0;
    void* m_file = nullptr;                   // FILE*
};

} // namespace jx
