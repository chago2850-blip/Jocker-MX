// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "AudioEngine.h"

#include "miniaudio.h"

#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace jx {

namespace {
constexpr int kMaxBlock = 4096;
constexpr float kHalfPi = 1.57079632679f;

void dataCallback(ma_device* dev, void* out, const void*, ma_uint32 frames) {
    auto* eng = static_cast<AudioEngine*>(dev->pUserData);
    float* o = static_cast<float*>(out);
    // Procesa en bloques acotados para no exceder buffers preasignados.
    ma_uint32 done = 0;
    const int ch = static_cast<int>(dev->playback.channels);
    while (done < frames) {
        const ma_uint32 n = std::min<ma_uint32>(frames - done, kMaxBlock);
        eng->render(o + static_cast<size_t>(done) * ch, static_cast<int>(n));
        done += n;
    }
}
} // namespace

AudioEngine::AudioEngine(int numDecks) {
    for (int i = 0; i < numDecks; ++i) {
        m_decks.push_back(std::make_unique<Deck>(i));
        m_channels.push_back(std::make_unique<ChannelParams>());
        m_fx.push_back(std::make_unique<EffectUnit>());
    }
    m_dsp.resize(numDecks);
    m_deckBuf.assign(numDecks, std::vector<float>(kMaxBlock * 2, 0.f));
    m_master.assign(kMaxBlock * 2, 0.f);
    m_cue.assign(kMaxBlock * 2, 0.f);
}

AudioEngine::~AudioEngine() { m_recorder.stop(); stop(); }

bool AudioEngine::start() {
    if (m_dev) return true;
    m_ctx = new ma_context;
    m_dev = new ma_device;

    auto tryInit = [&](const ma_backend* backends, ma_uint32 count) -> bool {
        if (ma_context_init(backends, count, nullptr, m_ctx) != MA_SUCCESS) return false;
        ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
        cfg.playback.format = ma_format_f32;
        cfg.playback.channels = 0;            // canales nativos (4+ habilita audífonos)
        cfg.sampleRate = 0;                   // frecuencia nativa
        cfg.periodSizeInFrames = 256;
        cfg.performanceProfile = ma_performance_profile_low_latency;
        cfg.dataCallback = dataCallback;
        cfg.pUserData = this;
        if (ma_device_init(m_ctx, &cfg, m_dev) != MA_SUCCESS) {
            ma_context_uninit(m_ctx);
            return false;
        }
        return true;
    };

    bool ok = tryInit(nullptr, 0);
    if (!ok) {
        const ma_backend nullBackend[] = {ma_backend_null};
        ok = tryInit(nullBackend, 1);
        m_null = ok;
    }
    if (!ok) {
        delete m_dev; m_dev = nullptr;
        delete m_ctx; m_ctx = nullptr;
        return false;
    }

    m_sr = m_dev->sampleRate;
    m_outCh = static_cast<int>(m_dev->playback.channels);
    if (m_outCh < 1) m_outCh = 2;
    m_block = static_cast<int>(m_dev->playback.internalPeriodSizeInFrames);
    m_deviceName = QString::fromUtf8(m_dev->playback.name);
    m_backend = QString::fromUtf8(ma_get_backend_name(m_ctx->backend));
    for (auto& d : m_decks) d->prepare(m_sr, kMaxBlock);
    for (auto& f : m_fx) f->prepare(m_sr);
    m_sampler.prepare(m_sr);

    if (ma_device_start(m_dev) != MA_SUCCESS) {
        qWarning() << "No se pudo iniciar el dispositivo de audio";
        return false;
    }
    qInfo().noquote() << "Audio:" << m_backend << "|" << m_deviceName << "|" << m_sr << "Hz |"
                      << m_outCh << "canales | periodo" << m_block;
    return true;
}

void AudioEngine::prepareOffline(double sampleRate) {
    m_sr = sampleRate;
    m_outCh = 2;
    m_null = true;
    for (auto& d : m_decks) d->prepare(m_sr, kMaxBlock);
    for (auto& f : m_fx) f->prepare(m_sr);
    m_sampler.prepare(m_sr);
}

void AudioEngine::stop() {
    if (m_dev) {
        ma_device_uninit(m_dev);
        delete m_dev; m_dev = nullptr;
    }
    if (m_ctx) {
        ma_context_uninit(m_ctx);
        delete m_ctx; m_ctx = nullptr;
    }
}

void AudioEngine::updateDsp(ChannelDsp& d, ChannelParams& p) {
    const float lo = p.eqLowDb.load(std::memory_order_relaxed);
    const float mi = p.eqMidDb.load(std::memory_order_relaxed);
    const float hi = p.eqHighDb.load(std::memory_order_relaxed);
    const float fi = p.filter.load(std::memory_order_relaxed);
    if (lo != d.lastLow) { d.low.set(Biquad::Type::LowShelf, m_sr, 200.0, 0.7, lo); d.lastLow = lo; }
    if (mi != d.lastMid) { d.mid.set(Biquad::Type::Peak, m_sr, 1000.0, 0.7, mi); d.lastMid = mi; }
    if (hi != d.lastHigh) { d.high.set(Biquad::Type::HighShelf, m_sr, 4000.0, 0.7, hi); d.lastHigh = hi; }
    if (fi != d.lastFilt) {
        if (fi < -0.02f) {
            const double f = 20000.0 * std::pow(0.0075, -fi);   // 20 kHz → 150 Hz
            d.filt.set(Biquad::Type::LowPass, m_sr, f, 0.9, 0);
        } else if (fi > 0.02f) {
            const double f = 20.0 * std::pow(400.0, fi);         // 20 Hz → 8 kHz
            d.filt.set(Biquad::Type::HighPass, m_sr, f, 0.9, 0);
        }
        if ((d.lastFilt >= -0.02f && d.lastFilt <= 0.02f) || d.lastFilt == 999) d.filt.reset();
        d.lastFilt = fi;
    }
}

void AudioEngine::render(float* out, int n) {
    const int outCh = m_outCh;
    std::fill(m_master.begin(), m_master.begin() + n * 2, 0.f);
    std::fill(m_cue.begin(), m_cue.begin() + n * 2, 0.f);

    const float xf = crossfader.load(std::memory_order_relaxed);
    // Curva de crossfader suave con meseta: ambos al 100 % en el centro.
    const float xA = xf <= 0.f ? 1.f : std::cos(xf * kHalfPi);
    const float xB = xf >= 0.f ? 1.f : std::cos(-xf * kHalfPi);

    for (int d = 0; d < numDecks(); ++d) {
        float* buf = m_deckBuf[d].data();
        m_decks[d]->process(buf, n);
        ChannelParams& p = *m_channels[d];
        ChannelDsp& dsp = m_dsp[d];
        updateDsp(dsp, p);

        // Posición musical del deck para sincronizar los efectos
        double beatPos = 0, beatSamples = 0;
        if (const Track* tr = m_decks[d]->track(); tr && tr->bpm > 0) {
            const double bl = tr->sampleRate * 60.0 / tr->bpm;
            const double rate = std::max(0.05, m_decks[d]->baseRate());
            beatPos = (m_decks[d]->position.load(std::memory_order_relaxed) - tr->firstBeat) / bl;
            beatSamples = bl / rate;
        }

        const float targetGain = std::pow(10.f, p.gainDb.load(std::memory_order_relaxed) / 20.f);
        const float fader = p.fader.load(std::memory_order_relaxed);
        const float xg = numDecks() >= 2 ? (d % 2 == 0 ? xA : xB) : 1.f;
        const float targetFader = fader * fader * xg;   // curva cuadrática del fader
        const bool pfl = p.pfl.load(std::memory_order_relaxed);
        const float fi = p.filter.load(std::memory_order_relaxed);
        const bool filtOn = fi < -0.02f || fi > 0.02f;

        float peak = 0.f;
        const float gStep = (targetGain - dsp.curGain) / n;
        const float fStep = (targetFader - dsp.curFader) / n;
        // 1) Gain + EQ + filtro (en sitio)
        for (int i = 0; i < n; ++i) {
            dsp.curGain += gStep;
            for (int c = 0; c < 2; ++c) {
                float s = buf[2 * i + c] * dsp.curGain;
                s = dsp.low.process(s, c);
                s = dsp.mid.process(s, c);
                s = dsp.high.process(s, c);
                if (filtOn) s = dsp.filt.process(s, c);
                buf[2 * i + c] = s;
            }
        }
        // 2) Efectos sincronizados al beat (pre-fader)
        m_fx[d]->process(buf, n, beatPos, beatSamples);
        // 3) Audífonos, fader y suma al master
        for (int i = 0; i < n; ++i) {
            dsp.curFader += fStep;
            for (int c = 0; c < 2; ++c) {
                const float s = buf[2 * i + c];
                if (pfl) m_cue[2 * i + c] += s;
                const float post = s * dsp.curFader;
                m_master[2 * i + c] += post;
                peak = std::max(peak, std::abs(post));
            }
        }
        dsp.curGain = targetGain;
        dsp.curFader = targetFader;
        p.peak.store(peak, std::memory_order_relaxed);
    }

    // Sampler directo al master (y a audífonos si está en PFL)
    m_sampler.process(m_master.data(), m_cue.data(), n);

    // Volumen maestro + limitador (ataque instantáneo, liberación ~120 ms)
    const float mv = masterVolume.load(std::memory_order_relaxed);
    const float rel = std::exp(-1.f / (0.12f * static_cast<float>(m_sr)));
    const float ceiling = 0.97f;
    float pkL = 0.f, pkR = 0.f, minG = 1.f;
    for (int i = 0; i < n; ++i) {
        float l = m_master[2 * i] * mv, r = m_master[2 * i + 1] * mv;
        const float pk = std::max(std::abs(l), std::abs(r));
        const float need = pk > ceiling ? ceiling / pk : 1.f;
        m_limEnv = need < m_limEnv ? need : (1.f - (1.f - m_limEnv) * rel);
        m_limEnv = std::min(m_limEnv, 1.f);
        l *= m_limEnv; r *= m_limEnv;
        minG = std::min(minG, m_limEnv);
        m_master[2 * i] = l; m_master[2 * i + 1] = r;
        pkL = std::max(pkL, std::abs(l)); pkR = std::max(pkR, std::abs(r));
    }
    m_recorder.push(m_master.data(), n);     // grabación del master final (sin bloqueos)
    if (AudioTap* tap = m_tap.load(std::memory_order_acquire)) tap->push(m_master.data(), n);
    masterPeakL.store(pkL, std::memory_order_relaxed);
    masterPeakR.store(pkR, std::memory_order_relaxed);
    limiterGainReduction.store(1.f - minG, std::memory_order_relaxed);

    // Salida: master en 1-2; audífonos en 3-4 si existen; o "split cue" en estéreo.
    const float cm = cueMix.load(std::memory_order_relaxed);
    const float hv = headphoneVolume.load(std::memory_order_relaxed);
    const bool split = splitCue.load(std::memory_order_relaxed) && outCh >= 2;
    for (int i = 0; i < n; ++i) {
        float* o = out + static_cast<size_t>(i) * outCh;
        const float l = m_master[2 * i], r = m_master[2 * i + 1];
        const float hl = (m_cue[2 * i] * (1.f - cm) + l * cm) * hv;
        const float hr = (m_cue[2 * i + 1] * (1.f - cm) + r * cm) * hv;
        if (outCh == 1) {
            o[0] = 0.5f * (l + r);
        } else if (split) {
            o[0] = std::clamp(0.5f * (hl + hr), -1.f, 1.f);
            o[1] = 0.5f * (l + r);
            for (int c = 2; c < outCh; ++c) o[c] = 0.f;
        } else {
            o[0] = l; o[1] = r;
            if (outCh >= 4) {
                o[2] = std::clamp(hl, -1.f, 1.f);
                o[3] = std::clamp(hr, -1.f, 1.f);
                for (int c = 4; c < outCh; ++c) o[c] = 0.f;
            } else {
                for (int c = 2; c < outCh; ++c) o[c] = 0.f;
            }
        }
    }
}

} // namespace jx
