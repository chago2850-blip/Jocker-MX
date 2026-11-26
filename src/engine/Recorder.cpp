// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Recorder.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace jx {

namespace {
void putLe(std::FILE* f, uint32_t v, int bytes) {
    for (int i = 0; i < bytes; ++i) std::fputc(static_cast<int>((v >> (8 * i)) & 0xFF), f);
}
void writeHeader(std::FILE* f, uint32_t sr, uint32_t dataBytes) {
    std::fseek(f, 0, SEEK_SET);
    std::fwrite("RIFF", 1, 4, f);
    putLe(f, 36 + dataBytes, 4);
    std::fwrite("WAVEfmt ", 1, 8, f);
    putLe(f, 16, 4);
    putLe(f, 1, 2);            // PCM
    putLe(f, 2, 2);            // estéreo
    putLe(f, sr, 4);
    putLe(f, sr * 2 * 3, 4);   // bytes por segundo (24 bits)
    putLe(f, 6, 2);            // alineación de bloque
    putLe(f, 24, 2);
    std::fwrite("data", 1, 4, f);
    putLe(f, dataBytes, 4);
}
std::FILE* openFile(const QString& path) {
#ifdef _WIN32
    return _wfopen(reinterpret_cast<const wchar_t*>(path.utf16()), L"wb");
#else
    return std::fopen(path.toUtf8().constData(), "wb");
#endif
}
} // namespace

Recorder::~Recorder() { stop(); }

bool Recorder::start(const QString& path, double sampleRate) {
    if (m_active.load()) return false;
    QDir().mkpath(QFileInfo(path).absolutePath());
    std::FILE* f = openFile(path);
    if (!f) return false;
    m_file = f;
    m_path = path;
    m_sr = sampleRate;
    m_capacity = static_cast<size_t>(sampleRate * 8);   // 8 s de margen
    m_ring.assign(m_capacity * 2, 0.f);
    m_head = 0;
    m_tail = 0;
    m_written = 0;
    m_dropped = 0;
    m_stopping = false;
    writeHeader(f, static_cast<uint32_t>(sampleRate), 0);
    m_active.store(true, std::memory_order_release);
    m_thread = std::thread(&Recorder::writerLoop, this);
    return true;
}

void Recorder::push(const float* stereo, int frames) {
    if (!m_active.load(std::memory_order_acquire)) return;
    const size_t head = m_head.load(std::memory_order_relaxed);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    const size_t free = m_capacity - (head - tail);
    const size_t n = std::min<size_t>(static_cast<size_t>(frames), free);
    for (size_t i = 0; i < n; ++i) {
        const size_t idx = ((head + i) % m_capacity) * 2;
        m_ring[idx] = stereo[2 * i];
        m_ring[idx + 1] = stereo[2 * i + 1];
    }
    m_head.store(head + n, std::memory_order_release);
    if (n < static_cast<size_t>(frames)) m_dropped.fetch_add(static_cast<int64_t>(frames - n));
}

void Recorder::writerLoop() {
    auto* f = static_cast<std::FILE*>(m_file);
    std::vector<unsigned char> out;
    for (;;) {
        const size_t head = m_head.load(std::memory_order_acquire);
        size_t tail = m_tail.load(std::memory_order_relaxed);
        const size_t avail = head - tail;
        if (avail == 0) {
            if (m_stopping.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        out.resize(avail * 6);
        for (size_t i = 0; i < avail; ++i) {
            const size_t idx = ((tail + i) % m_capacity) * 2;
            for (int c = 0; c < 2; ++c) {
                const float v = std::clamp(m_ring[idx + c], -1.f, 1.f);
                const int32_t s = static_cast<int32_t>(std::lround(v * 8388607.0));
                unsigned char* p = &out[i * 6 + c * 3];
                p[0] = static_cast<unsigned char>(s & 0xFF);
                p[1] = static_cast<unsigned char>((s >> 8) & 0xFF);
                p[2] = static_cast<unsigned char>((s >> 16) & 0xFF);
            }
        }
        std::fwrite(out.data(), 1, out.size(), f);
        m_tail.store(tail + avail, std::memory_order_release);
        m_written.fetch_add(static_cast<int64_t>(avail));
    }
    const int64_t bytes = m_written.load() * 6;
    writeHeader(f, static_cast<uint32_t>(m_sr), static_cast<uint32_t>(std::min<int64_t>(bytes, 0xFFFFFFF0LL)));
    std::fclose(f);
    m_file = nullptr;
}

void Recorder::stop() {
    if (!m_active.load()) return;
    m_active.store(false, std::memory_order_release);   // el hilo de audio deja de escribir
    m_stopping = true;
    if (m_thread.joinable()) m_thread.join();
}

} // namespace jx
