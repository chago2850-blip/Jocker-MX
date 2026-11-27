// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>
#include <memory>
#include <vector>

struct OpusEncoder;

namespace jx {

// Codificador para transmisión: recibe estéreo float intercalado a inputRate() y devuelve bytes listos
// para enviar. start() reinicia el flujo y devuelve sus cabeceras (cada conexión nueva empieza con ellas).
class StreamEncoder {
public:
    enum Format { Opus = 0, Mp3 = 1 };
    virtual ~StreamEncoder() = default;
    virtual QByteArray start(const QString& title) = 0;
    virtual QByteArray encode(const float* stereo, int frames) = 0;
    virtual QByteArray finish() = 0;           // vacía lo pendiente (fin de flujo)
    virtual int inputRate() const = 0;
    virtual QString mimeType() const = 0;
    virtual QString name() const = 0;

    // engineRate decide la frecuencia del MP3 (44,1 o 48 kHz); Opus siempre trabaja a 48 kHz.
    static std::unique_ptr<StreamEncoder> create(Format f, int bitrateKbps, double engineRate, QString* error);
};

// Páginas Ogg (RFC 3533) con su CRC.
class OggWriter {
public:
    void reset(uint32_t serial);
    // Agrega un paquete; si flush, cierra la página. Devuelve las páginas completas producidas.
    QByteArray packet(const QByteArray& data, int64_t granule, bool bos, bool eos, bool flush);
    QByteArray flushPage(bool eos = false);
    static uint32_t crc(const uint8_t* data, size_t n);
private:
    QByteArray pageFrom(bool eos);
    uint32_t m_serial = 0, m_seq = 0;
    std::vector<uint8_t> m_lacing;
    QByteArray m_body;
    int64_t m_granule = 0;
    bool m_bos = false, m_continued = false, m_ended = false;
};

class OpusStreamEncoder : public StreamEncoder {
public:
    explicit OpusStreamEncoder(int bitrateKbps);
    ~OpusStreamEncoder() override;
    bool ok() const { return m_enc != nullptr; }
    QByteArray start(const QString& title) override;
    QByteArray encode(const float* stereo, int frames) override;
    QByteArray finish() override;
    int inputRate() const override { return 48000; }
    QString mimeType() const override { return QStringLiteral("audio/ogg"); }
    QString name() const override { return QStringLiteral("Opus"); }
    int preSkip() const { return m_preSkip; }
private:
    ::OpusEncoder* m_enc = nullptr;
    int m_bitrate;
    int m_preSkip = 312;
    OggWriter m_ogg;
    std::vector<float> m_pending;   // muestras que no completan un bloque de 20 ms
    int64_t m_granule = 0;
    int m_packetsInPage = 0;
};

class Mp3StreamEncoder : public StreamEncoder {
public:
    Mp3StreamEncoder(int bitrateKbps, int sampleRate);
    ~Mp3StreamEncoder() override;
    bool ok() const { return m_shine != nullptr; }
    QByteArray start(const QString& title) override;
    QByteArray encode(const float* stereo, int frames) override;
    QByteArray finish() override;
    int inputRate() const override { return m_rate; }
    QString mimeType() const override { return QStringLiteral("audio/mpeg"); }
    QString name() const override { return QStringLiteral("MP3"); }
private:
    void open();
    void close();
    void* m_shine = nullptr;   // shine_t
    int m_bitrate, m_rate;
    int m_perPass = 1152;
    std::vector<int16_t> m_pending;
};

} // namespace jx
