// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Cdg.h"

#include <QFile>
#include <algorithm>
#include <cstring>

namespace jx {

namespace {
constexpr uint8_t kCmdGraphics = 0x09;
enum Instr : uint8_t {
    MemoryPreset = 1, BorderPreset = 2, TileBlock = 6, ScrollPreset = 20, ScrollCopy = 24,
    DefineTransparent = 28, LoadColorsLow = 30, LoadColorsHigh = 31, TileBlockXor = 38
};
} // namespace

bool CdgPlayer::load(const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    return loadData(f.readAll());
}

bool CdgPlayer::loadData(const QByteArray& data) {
    m_data = data;
    reset();
    return m_data.size() >= 24;
}

void CdgPlayer::reset() {
    m_px.fill(0);
    m_pal.fill(qRgb(0, 0, 0));
    m_border = 0;
    m_next = 0;
    m_dirty = true;
}

const QImage& CdgPlayer::frameAt(double seconds) {
    const qint64 total = m_data.size() / 24;
    const qint64 target = std::clamp<qint64>(qint64(seconds * kPacketsPerSecond), 0, total);
    if (target < m_next) reset();   // retroceso: repetir desde el principio
    const auto* base = reinterpret_cast<const uint8_t*>(m_data.constData());
    for (; m_next < target; ++m_next) execute(base + m_next * 24);
    m_changed = m_dirty || m_img.isNull();
    if (m_changed) renderImage();
    return m_img;
}

void CdgPlayer::execute(const uint8_t* p) {
    if ((p[0] & 0x3F) != kCmdGraphics) return;
    const uint8_t* d = p + 4;
    switch (p[1] & 0x3F) {
    case MemoryPreset: {
        const uint8_t c = d[0] & 0x0F;
        m_px.fill(c);
        m_dirty = true;
        break;
    }
    case BorderPreset: {
        m_border = d[0] & 0x0F;
        for (int y = 0; y < kHeight; ++y)
            for (int x = 0; x < kWidth; ++x)
                if (x < 6 || x >= kWidth - 6 || y < 12 || y >= kHeight - 12) m_px[y * kWidth + x] = uint8_t(m_border);
        m_dirty = true;
        break;
    }
    case TileBlock: tileBlock(d, false); break;
    case TileBlockXor: tileBlock(d, true); break;
    case ScrollPreset: scroll(d, false); break;
    case ScrollCopy: scroll(d, true); break;
    case LoadColorsLow: loadColors(d, 0); break;
    case LoadColorsHigh: loadColors(d, 8); break;
    case DefineTransparent: default: break;
    }
}

void CdgPlayer::tileBlock(const uint8_t* d, bool xorMode) {
    const uint8_t c0 = d[0] & 0x0F, c1 = d[1] & 0x0F;
    const int row = d[2] & 0x1F, col = d[3] & 0x3F;
    if (row >= 18 || col >= 50) return;
    const int x0 = col * 6, y0 = row * 12;
    for (int y = 0; y < 12; ++y) {
        const uint8_t bits = d[4 + y] & 0x3F;
        for (int x = 0; x < 6; ++x) {
            const uint8_t c = (bits >> (5 - x)) & 1 ? c1 : c0;
            uint8_t& px = m_px[(y0 + y) * kWidth + x0 + x];
            px = xorMode ? uint8_t((px ^ c) & 0x0F) : c;
        }
    }
    m_dirty = true;
}

void CdgPlayer::scroll(const uint8_t* d, bool copy) {
    const uint8_t fill = d[0] & 0x0F;
    const int hCmd = (d[1] & 0x30) >> 4, vCmd = (d[2] & 0x30) >> 4;
    const int dx = hCmd == 1 ? 6 : hCmd == 2 ? -6 : 0;   // 1 = derecha, 2 = izquierda
    const int dy = vCmd == 1 ? 12 : vCmd == 2 ? -12 : 0; // 1 = abajo, 2 = arriba
    if (!dx && !dy) return;
    std::array<uint8_t, kWidth * kHeight> src = m_px;
    for (int y = 0; y < kHeight; ++y)
        for (int x = 0; x < kWidth; ++x) {
            int sx = x - dx, sy = y - dy;
            uint8_t c;
            if (copy) {
                sx = (sx + kWidth) % kWidth;
                sy = (sy + kHeight) % kHeight;
                c = src[sy * kWidth + sx];
            } else {
                c = (sx < 0 || sx >= kWidth || sy < 0 || sy >= kHeight) ? fill : src[sy * kWidth + sx];
            }
            m_px[y * kWidth + x] = c;
        }
    m_dirty = true;
}

void CdgPlayer::loadColors(const uint8_t* d, int offset) {
    for (int i = 0; i < 8; ++i) {
        const uint8_t a = d[2 * i] & 0x3F, b = d[2 * i + 1] & 0x3F;
        const int r = (a >> 2) & 0x0F;
        const int g = ((a & 0x03) << 2) | ((b >> 4) & 0x03);
        const int bl = b & 0x0F;
        m_pal[offset + i] = qRgb(r * 17, g * 17, bl * 17);
    }
    m_dirty = true;
}

void CdgPlayer::renderImage() {
    if (m_img.isNull()) m_img = QImage(kWidth, kHeight, QImage::Format_RGB32);
    for (int y = 0; y < kHeight; ++y) {
        auto* line = reinterpret_cast<QRgb*>(m_img.scanLine(y));
        for (int x = 0; x < kWidth; ++x) line[x] = m_pal[m_px[y * kWidth + x]];
    }
    m_dirty = false;
}

} // namespace jx
