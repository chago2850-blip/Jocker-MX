// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "CdgItem.h"

#include <QPainter>

namespace jx {

CdgItem::CdgItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setOpaquePainting(true);
}

void CdgItem::setDeck(DeckController* d) {
    if (d == m_deck) return;
    if (m_deck) disconnect(m_deck, nullptr, this, nullptr);
    m_deck = d;
    if (m_deck) {
        connect(m_deck, &DeckController::positionChanged, this, [this]() { if (m_deck && m_deck->hasCdg()) update(); });
        connect(m_deck, &DeckController::karaokeChanged, this, [this]() { update(); });
    }
    emit deckChanged();
    update();
}

void CdgItem::paint(QPainter* p) {
    if (!m_deck || !m_deck->hasCdg()) {
        p->fillRect(boundingRect(), Qt::black);
        return;
    }
    const QImage& img = m_deck->cdgFrame();
    p->fillRect(boundingRect(), QColor(m_deck->cdgBorderColor()));
    if (img.isNull()) return;
    const double s = std::min(width() / img.width(), height() / img.height());
    const QRectF r((width() - img.width() * s) / 2, (height() - img.height() * s) / 2, img.width() * s, img.height() * s);
    p->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);
    p->drawImage(r, img);
}

} // namespace jx
