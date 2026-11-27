// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Biquad.h"
#include "Deck.h"
#include "Effects.h"
#include "Recorder.h"
#include "Sampler.h"

#include <QString>
#include <atomic>
#include <memory>
#include <vector>

struct ma_device;
struct ma_context;

namespace jx {

// Parámetros de un canal del mezclador (escritos por la UI, leídos por el hilo de audio).
struct ChannelParams {
    std::atomic<float> gainDb{0.f};     // -12..+12
    std::atomic<float> eqLowDb{0.f};    // -40 (kill)..+6
    std::atomic<float> eqMidDb{0.f};
    std::atomic<float> eqHighDb{0.f};
    std::atomic<float> filter{0.f};     // -1 = LPF, 0 = neutro, +1 = HPF
    std::atomic<float> fader{1.f};      // 0..1
    std::atomic<bool>  pfl{false};      // escucha previa en audífonos
    std::atomic<float> peak{0.f};       // VU (post-fader)
};

// Motor de audio: dispositivo, decks y mezclador maestro.
// Receptor del master final (transmisión en vivo, etc.). push() se llama desde el hilo de audio:
// debe ser sin bloqueos.
class AudioTap {
public:
    virtual ~AudioTap() = default;
    virtual void push(const float* stereo, int frames) = 0;
};

class AudioEngine {
public:
    explicit AudioEngine(int numDecks = 2);
    ~AudioEngine();

    bool start();          // abre el dispositivo por defecto (o el nulo si no hay audio)
    void stop();
    void prepareOffline(double sampleRate);   // pruebas / render sin dispositivo

    int numDecks() const { return static_cast<int>(m_decks.size()); }
    Deck& deck(int i) { return *m_decks[i]; }
    ChannelParams& channel(int i) { return *m_channels[i]; }
    EffectUnit& effects(int i) { return *m_fx[i]; }
    Sampler& sampler() { return m_sampler; }
    Recorder& recorder() { return m_recorder; }
    void setTap(AudioTap* t) { m_tap.store(t, std::memory_order_release); }   // nullptr = quitar

    double sampleRate() const { return m_sr; }
    int outputChannels() const { return m_outCh; }
    int blockSize() const { return m_block; }
    QString deviceName() const { return m_deviceName; }
    QString backendName() const { return m_backend; }
    bool isNullDevice() const { return m_null; }

    std::atomic<float> crossfader{0.f};      // -1 (A) .. +1 (B)
    std::atomic<float> masterVolume{0.85f};
    std::atomic<float> cueMix{0.5f};         // 0 = sólo cue, 1 = sólo master (audífonos)
    std::atomic<float> headphoneVolume{0.8f};
    std::atomic<bool>  splitCue{false};      // 2 canales: izquierda = cue, derecha = master (mono)
    std::atomic<float> masterPeakL{0.f}, masterPeakR{0.f};
    std::atomic<float> limiterGainReduction{0.f};

    // Hilo de audio
    void render(float* out, int frames);

private:
    struct ChannelDsp {
        Biquad low, mid, high, filt;
        float lastLow = 999, lastMid = 999, lastHigh = 999, lastFilt = 999;
        float curGain = 1.f, curFader = 1.f;
    };
    void updateDsp(ChannelDsp& d, ChannelParams& p);

    std::vector<std::unique_ptr<Deck>> m_decks;
    std::vector<std::unique_ptr<ChannelParams>> m_channels;
    std::vector<std::unique_ptr<EffectUnit>> m_fx;
    std::vector<ChannelDsp> m_dsp;
    std::vector<std::vector<float>> m_deckBuf;
    std::vector<float> m_master, m_cue;
    Sampler m_sampler;
    Recorder m_recorder;
    std::atomic<AudioTap*> m_tap{nullptr};

    ma_context* m_ctx = nullptr;
    ma_device* m_dev = nullptr;
    double m_sr = 48000.0;
    int m_outCh = 2;
    int m_block = 512;
    bool m_null = false;
    QString m_deviceName, m_backend;
    float m_limEnv = 1.f;
};

} // namespace jx
