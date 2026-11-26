// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>
#include <array>
#include <cstdint>

namespace jx {

// Reproductor CD+G (gráficos de karaoke): 300 paquetes de 24 bytes por segundo,
// pantalla de 300×216 con 16 colores (zona visible 288×192 más el borde).
class CdgPlayer {
public:
    static constexpr int kWidth = 300, kHeight = 216;
    static constexpr int kPacketsPerSecond = 300;

    bool load(const QString& path, QString* error = nullptr);
    bool loadData(const QByteArray& data);
    bool isLoaded() const { return !m_data.isEmpty(); }
    double duration() const { return double(m_data.size() / 24) / kPacketsPerSecond; }

    // Avanza hasta el instante dado (hacia atrás vuelve a empezar: es barato) y devuelve el cuadro.
    const QImage& frameAt(double seconds);
    bool changedSinceLastFrame() const { return m_changed; }
    int colorIndexAt(int x, int y) const { return m_px[y * kWidth + x]; }
    QRgb paletteColor(int i) const { return m_pal[i & 15]; }
    QRgb borderColor() const { return m_pal[m_border & 15]; }

private:
    void reset();
    void execute(const uint8_t* p);
    void tileBlock(const uint8_t* d, bool xorMode);
    void scroll(const uint8_t* d, bool copy);
    void loadColors(const uint8_t* d, int offset);
    void renderImage();

    QByteArray m_data;
    std::array<uint8_t, kWidth * kHeight> m_px{};
    std::array<QRgb, 16> m_pal{};
    int m_border = 0;
    qint64 m_next = 0;        // siguiente paquete por ejecutar
    bool m_dirty = true, m_changed = true;
    QImage m_img;
};

} // namespace jx
