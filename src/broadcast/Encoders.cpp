// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Encoders.h"

#include <opus.h>
extern "C" {
#include "layer3.h"
}

#include <QList>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace jx {

// ================================================================== Ogg
namespace {
uint32_t crcTable[256];
bool crcReady = [] {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t r = i << 24;
        for (int k = 0; k < 8; ++k) r = (r & 0x80000000u) ? (r << 1) ^ 0x04c11db7u : (r << 1);
        crcTable[i] = r;
    }
    return true;
}();
void le16(QByteArray& b, uint16_t v) { b.append(char(v & 0xFF)); b.append(char(v >> 8)); }
void le32(QByteArray& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.append(char((v >> (8 * i)) & 0xFF)); }
void le64(QByteArray& b, uint64_t v) { for (int i = 0; i < 8; ++i) b.append(char((v >> (8 * i)) & 0xFF)); }
} // namespace

uint32_t OggWriter::crc(const uint8_t* d, size_t n) {
    (void)crcReady;
    uint32_t c = 0;
    for (size_t i = 0; i < n; ++i) c = (c << 8) ^ crcTable[((c >> 24) & 0xFF) ^ d[i]];
    return c;
}

void OggWriter::reset(uint32_t serial) {
    m_serial = serial;
    m_seq = 0;
    m_lacing.clear();
    m_body.clear();
    m_granule = 0;
    m_bos = false;
    m_continued = false;
    m_ended = false;
}

QByteArray OggWriter::pageFrom(bool eos) {
    QByteArray p;
    p.append("OggS", 4);
    p.append(char(0));
    p.append(char((m_continued ? 1 : 0) | (m_bos ? 2 : 0) | (eos ? 4 : 0)));
    le64(p, uint64_t(m_granule));
    le32(p, m_serial);
    le32(p, m_seq++);
    le32(p, 0);   // CRC después
    p.append(char(m_lacing.size()));
    for (uint8_t l : m_lacing) p.append(char(l));
    p.append(m_body);
    const uint32_t c = crc(reinterpret_cast<const uint8_t*>(p.constData()), size_t(p.size()));
    for (int i = 0; i < 4; ++i) p[22 + i] = char((c >> (8 * i)) & 0xFF);
    m_lacing.clear();
    m_body.clear();
    m_bos = false;
    m_continued = false;
    m_ended = false;
    return p;
}

QByteArray OggWriter::flushPage(bool eos) {
    if (m_lacing.empty() && !eos) return {};
    return pageFrom(eos);
}

QByteArray OggWriter::packet(const QByteArray& data, int64_t granule, bool bos, bool eos, bool flush) {
    QByteArray out;
    if (bos) { out += flushPage(); m_bos = true; }
    int remaining = int(data.size()), pos = 0;
    for (;;) {
        // 255 segmentos por página como máximo
        if (m_lacing.size() == 255) {
            if (!m_ended) m_granule = -1;   // ningún paquete termina en esta página
            out += pageFrom(false);
            m_continued = true;
        }
        const int seg = std::min(remaining, 255);
        m_lacing.push_back(uint8_t(seg));
        m_body.append(data.constData() + pos, seg);
        pos += seg;
        remaining -= seg;
        if (seg < 255) break;      // un segmento < 255 termina el paquete (también 0 si era múltiplo de 255)
    }
    m_granule = granule;
    m_ended = true;
    if (flush || eos || bos) out += pageFrom(eos);
    return out;
}

// ================================================================== Opus
OpusStreamEncoder::OpusStreamEncoder(int bitrateKbps) : m_bitrate(std::clamp(bitrateKbps, 32, 256)) {
    int err = 0;
    m_enc = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK) { m_enc = nullptr; return; }
    opus_encoder_ctl(m_enc, OPUS_SET_BITRATE(m_bitrate * 1000));
    opus_encoder_ctl(m_enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
    opus_encoder_ctl(m_enc, OPUS_SET_COMPLEXITY(9));
    opus_int32 la = 0;
    opus_encoder_ctl(m_enc, OPUS_GET_LOOKAHEAD(&la));
    m_preSkip = int(la);
}

OpusStreamEncoder::~OpusStreamEncoder() { if (m_enc) opus_encoder_destroy(m_enc); }

QByteArray OpusStreamEncoder::start(const QString& title) {
    opus_encoder_ctl(m_enc, OPUS_RESET_STATE);
    m_ogg.reset(QRandomGenerator::global()->generate());
    m_pending.clear();
    m_granule = 0;
    m_packetsInPage = 0;
    // OpusHead (RFC 7845)
    QByteArray head("OpusHead", 8);
    head.append(char(1));
    head.append(char(2));
    le16(head, uint16_t(m_preSkip));
    le32(head, 48000);
    le16(head, 0);
    head.append(char(0));
    // OpusTags
    QByteArray tags("OpusTags", 8);
    const QByteArray vendor = QByteArray("Jocker-MX (") + opus_get_version_string() + ")";
    le32(tags, uint32_t(vendor.size()));
    tags.append(vendor);
    QList<QByteArray> comments{QByteArray("ENCODER=Jocker-MX")};
    if (!title.isEmpty()) comments << ("TITLE=" + title.toUtf8());
    le32(tags, uint32_t(comments.size()));
    for (const QByteArray& c : comments) { le32(tags, uint32_t(c.size())); tags.append(c); }
    QByteArray out = m_ogg.packet(head, 0, true, false, true);
    out += m_ogg.packet(tags, 0, false, false, true);
    return out;
}

QByteArray OpusStreamEncoder::encode(const float* stereo, int frames) {
    if (!m_enc) return {};
    m_pending.insert(m_pending.end(), stereo, stereo + size_t(frames) * 2);
    QByteArray out;
    constexpr int kFrame = 960;   // 20 ms
    unsigned char buf[4000];
    size_t used = 0;
    while (m_pending.size() - used >= size_t(kFrame) * 2) {
        const int n = opus_encode_float(m_enc, m_pending.data() + used, kFrame, buf, sizeof(buf));
        used += size_t(kFrame) * 2;
        if (n < 0) continue;
        m_granule += kFrame;
        // Una página cada 5 paquetes (100 ms): poca latencia sin demasiadas cabeceras
        const bool flush = ++m_packetsInPage >= 5;
        if (flush) m_packetsInPage = 0;
        out += m_ogg.packet(QByteArray(reinterpret_cast<const char*>(buf), n), m_granule, false, false, flush);
    }
    m_pending.erase(m_pending.begin(), m_pending.begin() + long(used));
    return out;
}

QByteArray OpusStreamEncoder::finish() {
    QByteArray out;
    if (!m_pending.empty()) {
        std::vector<float> pad(960 * 2 - m_pending.size() % (960 * 2), 0.f);
        out += encode(pad.data(), int(pad.size() / 2));
    }
    out += m_ogg.flushPage(true);
    return out;
}

// ================================================================== MP3 (Shine)
Mp3StreamEncoder::Mp3StreamEncoder(int bitrateKbps, int sampleRate) : m_bitrate(bitrateKbps), m_rate(sampleRate) { open(); }
Mp3StreamEncoder::~Mp3StreamEncoder() { close(); }

void Mp3StreamEncoder::open() {
    shine_config_t cfg;
    shine_set_config_mpeg_defaults(&cfg.mpeg);
    cfg.wave.channels = 2;
    cfg.wave.samplerate = m_rate;
    cfg.mpeg.mode = SHINE_JOINT_STEREO;
    cfg.mpeg.bitrate = m_bitrate;
    if (shine_check_config(m_rate, m_bitrate) < 0) return;
    shine_t s = shine_initialise(&cfg);
    m_shine = s;
    if (s) m_perPass = shine_samples_per_pass(s);
}

void Mp3StreamEncoder::close() {
    if (m_shine) shine_close(static_cast<shine_t>(m_shine));
    m_shine = nullptr;
}

QByteArray Mp3StreamEncoder::start(const QString&) {
    close();
    open();
    m_pending.clear();
    return {};
}

QByteArray Mp3StreamEncoder::encode(const float* stereo, int frames) {
    if (!m_shine) return {};
    for (int i = 0; i < frames * 2; ++i)
        m_pending.push_back(int16_t(std::lrint(std::clamp(stereo[i], -1.f, 1.f) * 32767.f)));
    QByteArray out;
    size_t used = 0;
    const size_t chunk = size_t(m_perPass) * 2;
    while (m_pending.size() - used >= chunk) {
        int written = 0;
        unsigned char* d = shine_encode_buffer_interleaved(static_cast<shine_t>(m_shine), m_pending.data() + used, &written);
        if (written > 0) out.append(reinterpret_cast<const char*>(d), written);
        used += chunk;
    }
    m_pending.erase(m_pending.begin(), m_pending.begin() + long(used));
    return out;
}

QByteArray Mp3StreamEncoder::finish() {
    if (!m_shine) return {};
    int written = 0;
    unsigned char* d = shine_flush(static_cast<shine_t>(m_shine), &written);
    return written > 0 ? QByteArray(reinterpret_cast<const char*>(d), written) : QByteArray();
}

// ================================================================== fábrica
std::unique_ptr<StreamEncoder> StreamEncoder::create(Format f, int kbps, double engineRate, QString* error) {
    if (f == Opus) {
        auto e = std::make_unique<OpusStreamEncoder>(kbps);
        if (!e->ok()) { if (error) *error = QStringLiteral("No se pudo crear el codificador Opus"); return nullptr; }
        return e;
    }
    static const int rates[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
    int br = 128;
    for (int r : rates) if (r <= kbps) br = r;
    const int sr = std::abs(engineRate - 44100.0) < 1.0 ? 44100 : 48000;
    auto e = std::make_unique<Mp3StreamEncoder>(br, sr);
    if (!e->ok()) { if (error) *error = QStringLiteral("No se pudo crear el codificador MP3"); return nullptr; }
    return e;
}

} // namespace jx
