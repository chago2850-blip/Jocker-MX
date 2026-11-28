// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "QrItem.h"

#include "qrcodegen.hpp"

#include <QPainter>

namespace jx {

QrItem::QrItem(QQuickItem* parent) : QQuickPaintedItem(parent) {}

std::vector<std::vector<bool>> QrItem::encode(const QString& text) {
    std::vector<std::vector<bool>> m;
    if (text.isEmpty()) return m;
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int n = qr.getSize();
    m.assign(size_t(n), std::vector<bool>(size_t(n), false));
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) m[size_t(y)][size_t(x)] = qr.getModule(x, y);
    return m;
}

void QrItem::setText(const QString& t) {
    if (t == m_text) return;
    m_text = t;
    m_modules = encode(t);
    emit textChanged();
    update();
}

void QrItem::paint(QPainter* p) {
    p->fillRect(boundingRect(), Qt::white);
    const int n = int(m_modules.size());
    if (n == 0) return;
    const double side = std::min(width(), height());
    const double cell = side / (n + 8);   // 4 módulos de margen a cada lado
    const double ox = (width() - cell * n) / 2, oy = (height() - cell * n) / 2;
    p->setPen(Qt::NoPen);
    p->setBrush(m_color);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            if (m_modules[size_t(y)][size_t(x)]) p->drawRect(QRectF(ox + x * cell, oy + y * cell, cell + 0.5, cell + 0.5));
}

} // namespace jx
