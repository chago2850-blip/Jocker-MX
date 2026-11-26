// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "VideoItem.h"

#include <QPainter>
#include <algorithm>

namespace jx {

void drawFitted(QPainter* p, const QImage& img, const QRectF& area, bool fill) {
    if (img.isNull() || area.isEmpty()) return;
    const double sx = area.width() / img.width(), sy = area.height() / img.height();
    const double s = fill ? std::max(sx, sy) : std::min(sx, sy);
    const QRectF r(area.center().x() - img.width() * s / 2, area.center().y() - img.height() * s / 2,
                   img.width() * s, img.height() * s);
    p->save();
    p->setClipRect(area, Qt::IntersectClip);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);
    p->drawImage(r, img);
    p->restore();
}

// ------------------------------------------------------------------ deck
VideoItem::VideoItem(QQuickItem* parent) : QQuickPaintedItem(parent) { setOpaquePainting(true); }

void VideoItem::setDeck(DeckController* d) {
    if (d == m_deck) return;
    if (m_deck) disconnect(m_deck->video(), nullptr, this, nullptr);
    m_deck = d;
    if (m_deck) {
        connect(m_deck->video(), &DeckVideo::frameChanged, this, [this]() { update(); });
        connect(m_deck->video(), &DeckVideo::hasVideoChanged, this, [this]() { update(); });
    }
    emit changed();
    update();
}

void VideoItem::paint(QPainter* p) {
    p->fillRect(boundingRect(), Qt::black);
    if (m_deck && m_deck->hasVideo()) drawFitted(p, m_deck->video()->image(), boundingRect(), m_fill);
}

// ------------------------------------------------------------------ mezcla
VideoMixItem::VideoMixItem(QQuickItem* parent) : QQuickPaintedItem(parent) { setOpaquePainting(true); }

void VideoMixItem::hook(QPointer<DeckController>& slot, DeckController* d) {
    if (d == slot) return;
    if (slot) disconnect(slot->video(), nullptr, this, nullptr);
    slot = d;
    if (slot) {
        connect(slot->video(), &DeckVideo::frameChanged, this, [this]() { update(); });
        connect(slot->video(), &DeckVideo::hasVideoChanged, this, [this]() { update(); });
    }
    emit changed();
    update();
}

void VideoMixItem::paint(QPainter* p) {
    const QRectF area = boundingRect();
    p->fillRect(area, Qt::black);
    const bool va = m_a && m_a->hasVideo(), vb = m_b && m_b->hasVideo();
    if (!va && !vb) return;
    // Si sólo un deck tiene video, se muestra ese (la otra pista es sólo audio).
    if (va != vb) {
        drawFitted(p, (va ? m_a : m_b)->video()->image(), area, m_fill);
        return;
    }
    const double w = DeckVideo::mixWeightB(m_xf, m_transition);
    const QImage& ia = m_a->video()->image();
    const QImage& ib = m_b->video()->image();
    if (m_transition == 1) {   // barrido de izquierda a derecha
        const double split = area.width() * w;
        p->save();
        p->setClipRect(QRectF(area.left() + split, area.top(), area.width() - split, area.height()));
        drawFitted(p, ia, area, m_fill);
        p->setClipRect(QRectF(area.left(), area.top(), split, area.height()));
        drawFitted(p, ib, area, m_fill);
        p->restore();
        return;
    }
    if (w < 1.0) drawFitted(p, ia, area, m_fill);
    if (w > 0.0) {
        p->setOpacity(w);
        drawFitted(p, ib, area, m_fill);
        p->setOpacity(1.0);
    }
}

} // namespace jx
