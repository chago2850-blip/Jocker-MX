// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "StemSeparator.h"

#include "analysis/PlatformDecoder.h"
#include "miniaudio.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace jx {

namespace {
constexpr int kN = 4096;         // tamaño de ventana STFT (Spleeter)
constexpr int kHop = 1024;
constexpr int kBins = kN / 2 + 1;
constexpr double kModelRate = 44100.0;
constexpr float kEps = 1e-10f;
using cf = std::complex<float>;

// FFT compleja radix-2 de tamaño fijo con tablas precalculadas.
class Fft {
public:
    explicit Fft(int n) : m_n(n), m_rev(n), m_tw(n / 2) {
        int bits = 0;
        while ((1 << bits) < n) ++bits;
        for (int i = 0; i < n; ++i) {
            int r = 0;
            for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            m_rev[i] = r;
        }
        for (int i = 0; i < n / 2; ++i) m_tw[i] = std::polar(1.0f, static_cast<float>(-2.0 * 3.14159265358979323846 * i / n));
    }
    void forward(cf* a) const { run(a, false); }
    void inverse(cf* a) const {   // sin normalizar
        run(a, true);
    }
private:
    void run(cf* a, bool inv) const {
        for (int i = 0; i < m_n; ++i) if (i < m_rev[i]) std::swap(a[i], a[m_rev[i]]);
        for (int len = 2; len <= m_n; len <<= 1) {
            const int half = len / 2, step = m_n / len;
            for (int i = 0; i < m_n; i += len)
                for (int j = 0; j < half; ++j) {
                    cf w = m_tw[j * step];
                    if (inv) w = std::conj(w);
                    const cf u = a[i + j], v = a[i + j + half] * w;
                    a[i + j] = u + v;
                    a[i + j + half] = u - v;
                }
        }
    }
    int m_n;
    std::vector<int> m_rev;
    std::vector<cf> m_tw;
};

inline int16_t toI16(float v) { return static_cast<int16_t>(std::lround(std::clamp(v, -1.f, 1.f) * 32767.f)); }
} // namespace

std::unique_ptr<StemData> separateStems(const StemModel& model, const float* pcm, int64_t frames, double sampleRate,
                                        int threads, const std::function<void(double)>& progress,
                                        const std::atomic<bool>* cancel, QString* error) {
    if (!model.isLoaded() || model.count() < 4 || frames <= 0) {
        if (error) *error = QStringLiteral("Modelo de stems no disponible");
        return nullptr;
    }
    // 1) A 44.1 kHz (frecuencia de entrenamiento del modelo)
    std::vector<float> x44;
    if (std::abs(sampleRate - kModelRate) < 1.0) x44.assign(pcm, pcm + frames * 2);
    else if (!convertToStereo(pcm, static_cast<size_t>(frames), 2, static_cast<int>(sampleRate), kModelRate, x44)) {
        if (error) *error = QStringLiteral("No se pudo remuestrear la pista");
        return nullptr;
    }
    const int64_t len = static_cast<int64_t>(x44.size() / 2);
    // Relleno de kN ceros al inicio (como Spleeter) y al final
    const int64_t total = len + 2 * kN;
    auto sample = [&](int64_t i, int ch) -> float {
        const int64_t j = i - kN;
        return (j >= 0 && j < len) ? x44[static_cast<size_t>(j) * 2 + ch] : 0.f;
    };
    const int64_t nFrames = (total - kN) / kHop + 1;
    const int T = StemModel::kT, F = StemModel::kF;

    std::vector<float> win(kN);
    for (int n = 0; n < kN; ++n) win[n] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * n / kN));   // Hann periódica
    const Fft fft(kN);

    // Acumuladores de superposición (OLA) a 44.1 kHz y salida final en int16 a 44.1 kHz
    std::vector<float> ola[3];
    std::vector<float> wsum;
    for (auto& o : ola) o.assign(static_cast<size_t>(total) * 2, 0.f);
    wsum.assign(static_cast<size_t>(total), 0.f);

    std::vector<cf> spec(static_cast<size_t>(T) * kBins * 2);     // X[cuadro][bin][canal]
    std::vector<float> mag(static_cast<size_t>(T) * F * 2);
    std::vector<std::vector<float>> est;
    std::vector<cf> buf(kN);
    const int64_t nBlocks = (nFrames + T - 1) / T;

    for (int64_t blk = 0; blk < nBlocks; ++blk) {
        if (cancel && cancel->load()) return nullptr;
        const int64_t f0 = blk * T;
        const int fCount = static_cast<int>(std::min<int64_t>(T, nFrames - f0));
        // 2) STFT (L y R a la vez con una FFT compleja: z = L + iR)
        std::fill(mag.begin(), mag.end(), 0.f);
        for (int f = 0; f < fCount; ++f) {
            const int64_t start = (f0 + f) * kHop;
            for (int n = 0; n < kN; ++n) buf[n] = cf(sample(start + n, 0) * win[n], sample(start + n, 1) * win[n]);
            fft.forward(buf.data());
            for (int k = 0; k < kBins; ++k) {
                const cf zk = buf[k], zn = std::conj(buf[(kN - k) % kN]);
                const cf L = 0.5f * (zk + zn);
                const cf R = cf(0, -0.5f) * (zk - zn);
                spec[(static_cast<size_t>(f) * kBins + k) * 2] = L;
                spec[(static_cast<size_t>(f) * kBins + k) * 2 + 1] = R;
                if (k < F) {
                    mag[(static_cast<size_t>(f) * F + k) * 2] = std::abs(L);
                    mag[(static_cast<size_t>(f) * F + k) * 2 + 1] = std::abs(R);
                }
            }
        }
        // 3) Red neuronal: espectrogramas estimados de voz, batería, bajo y otros
        model.run(mag.data(), est, threads, cancel);
        if (cancel && cancel->load()) return nullptr;
        // 4) Máscaras tipo Wiener (exponente 2) e ISTFT de voz, batería y bajo
        for (int f = 0; f < fCount; ++f) {
            const int64_t start = (f0 + f) * kHop;
            float hiMask[3][2] = {{0}};
            // Extensión de la máscara por encima de 11 kHz: promedio de los bins altos
            for (int c = 0; c < 2; ++c)
                for (int k = F - 64; k < F; ++k) {
                    float sum = kEps;
                    float e2[4];
                    for (int s = 0; s < 4; ++s) { const float v = est[s][(static_cast<size_t>(f) * F + k) * 2 + c]; e2[s] = v * v; sum += e2[s]; }
                    for (int s = 0; s < 3; ++s) hiMask[s][c] += (e2[s] + kEps / 4) / sum / 64.f;
                }
            for (int s = 0; s < 3; ++s) {
                for (int k = 0; k < kBins; ++k) {
                    float m[2];
                    for (int c = 0; c < 2; ++c) {
                        if (k < F) {
                            float sum = kEps, mine = 0;
                            for (int q = 0; q < 4; ++q) {
                                const float v = est[q][(static_cast<size_t>(f) * F + k) * 2 + c];
                                sum += v * v;
                                if (q == s) mine = v * v;
                            }
                            m[c] = (mine + kEps / 4) / sum;
                        } else {
                            m[c] = hiMask[s][c];
                        }
                    }
                    const cf L = spec[(static_cast<size_t>(f) * kBins + k) * 2] * m[0];
                    const cf R = spec[(static_cast<size_t>(f) * kBins + k) * 2 + 1] * m[1];
                    // z = L + iR con simetría hermitiana → parte real = l(t), imaginaria = r(t)
                    buf[k] = L + cf(0, 1) * R;
                    if (k > 0 && k < kN / 2) buf[kN - k] = std::conj(L) + cf(0, 1) * std::conj(R);
                }
                fft.inverse(buf.data());
                float* o = ola[s].data();
                for (int n = 0; n < kN; ++n) {
                    const size_t idx = static_cast<size_t>(start + n);
                    o[idx * 2] += buf[n].real() / kN * win[n];
                    o[idx * 2 + 1] += buf[n].imag() / kN * win[n];
                }
            }
            for (int n = 0; n < kN; ++n) wsum[static_cast<size_t>(start + n)] += win[n] * win[n];
        }
        if (progress) progress(static_cast<double>(blk + 1) / nBlocks * 0.92);
    }
    x44.clear();
    x44.shrink_to_fit();

    // 5) Normalizar la ventana, quitar el relleno y volver a la frecuencia del motor
    auto out = std::make_unique<StemData>();
    out->frames = frames;
    std::vector<float> s44(static_cast<size_t>(len) * 2), sOut;
    for (int s = 0; s < 3; ++s) {
        if (cancel && cancel->load()) return nullptr;
        for (int64_t i = 0; i < len; ++i) {
            const float w = wsum[static_cast<size_t>(i + kN)];
            const float inv = w > 1e-6f ? 1.f / w : 0.f;
            s44[static_cast<size_t>(i) * 2] = ola[s][static_cast<size_t>(i + kN) * 2] * inv;
            s44[static_cast<size_t>(i) * 2 + 1] = ola[s][static_cast<size_t>(i + kN) * 2 + 1] * inv;
        }
        ola[s].clear();
        ola[s].shrink_to_fit();
        const float* src = s44.data();
        if (std::abs(sampleRate - kModelRate) >= 1.0) {
            convertToStereo(s44.data(), static_cast<size_t>(len), 2, static_cast<int>(kModelRate), sampleRate, sOut);
            src = sOut.data();
        } else {
            sOut = s44;
            src = sOut.data();
        }
        const int64_t have = static_cast<int64_t>((src == sOut.data() ? sOut.size() : s44.size()) / 2);
        out->stem[s].assign(static_cast<size_t>(frames) * 2, 0);
        const int64_t n = std::min(have, frames);
        for (int64_t i = 0; i < n * 2; ++i) out->stem[s][static_cast<size_t>(i)] = toI16(src[i]);
        if (progress) progress(0.92 + 0.08 * (s + 1) / 3.0);
    }
    return out;
}

} // namespace jx
