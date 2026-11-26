// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "WaveformItem.h"

#include <QPainter>
#include <cmath>

namespace jx {

namespace {
const QColor kLow(255, 45, 140);    // graves: magenta
const QColor kMid(255, 186, 72);    // medios: oro
const QColor kHigh(90, 228, 255);   // agudos: cian
const QColor kCue(255, 255, 255);
const QColor kHot[8] = {QColor(255, 70, 70), QColor(255, 160, 40), QColor(250, 230, 60), QColor(80, 230, 120),
                        QColor(60, 200, 255), QColor(90, 110, 255), QColor(190, 90, 255), QColor(255, 90, 200)};
} // namespace

WaveformItem::WaveformItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(false);
    setOpaquePainting(false);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

void WaveformItem::setDeck(DeckController* d) {
    if (m_deck == d) return;
    if (m_deck) disconnect(m_deck, nullptr, this, nullptr);
    m_deck = d;
    if (m_deck) {
        auto repaint = [this]() { update(); };
        connect(m_deck, &DeckController::positionChanged, this, repaint);
        connect(m_deck, &DeckController::cuesChanged, this, repaint);
        connect(m_deck, &DeckController::loopChanged, this, repaint);
        connect(m_deck, &DeckController::trackChanged, this, [this]() { m_cache = QImage(); update(); });
    }
    m_cache = QImage();
    emit deckChanged();
    update();
}

void WaveformItem::geometryChange(const QRectF& n, const QRectF& o) {
    QQuickPaintedItem::geometryChange(n, o);
    m_cache = QImage();
    update();
}

void WaveformItem::drawColumn(QPainter* p, int x, float cy, float halfH, uint8_t lo, uint8_t mi, uint8_t hi,
                              float alpha) {
    auto bar = [&](uint8_t v, const QColor& c, float scale) {
        if (v == 0) return;
        const float h = halfH * (v / 255.f) * scale;
        QColor cc = c;
        cc.setAlphaF(alpha);
        if (m_mirrored) p->fillRect(QRectF(x, cy - h, 1, 2 * h), cc);
        else p->fillRect(QRectF(x, cy - h, 1, 2 * h), cc);
    };
    bar(lo, kLow, 1.0f);
    bar(mi, kMid, 0.78f);
    bar(hi, kHigh, 0.55f);
}

void WaveformItem::paint(QPainter* p) {
    const Track* t = m_deck ? m_deck->track() : nullptr;
    if (!t || t->wfLow.empty()) {
        p->setPen(QColor(255, 255, 255, 40));
        p->drawLine(QPointF(0, height() / 2), QPointF(width(), height() / 2));
        return;
    }
    if (m_scrolling) paintScrolling(p, t);
    else paintOverview(p, t);
}

void WaveformItem::paintOverview(QPainter* p, const Track* t) {
    const int W = static_cast<int>(width()), H = static_cast<int>(height());
    if (W <= 0 || H <= 0) return;
    if (m_cache.isNull() || m_cacheTrack != t || m_cache.width() != W || m_cache.height() != H) {
        m_cache = QImage(W, H, QImage::Format_ARGB32_Premultiplied);
        m_cache.fill(Qt::transparent);
        QPainter cp(&m_cache);
        const size_t bins = t->wfLow.size();
        for (int x = 0; x < W; ++x) {
            const size_t a = bins * x / W, b = std::max(a + 1, bins * (x + 1) / W);
            uint8_t lo = 0, mi = 0, hi = 0;
            for (size_t i = a; i < b && i < bins; ++i) {
                lo = std::max(lo, t->wfLow[i]); mi = std::max(mi, t->wfMid[i]); hi = std::max(hi, t->wfHigh[i]);
            }
            drawColumn(&cp, x, H / 2.f, H / 2.f - 1, lo, mi, hi, 1.0f);
        }
        m_cacheTrack = t;
    }
    p->drawImage(0, 0, m_cache);

    const double prog = m_deck->progress();
    const double px = prog * W;
    p->fillRect(QRectF(0, 0, px, H), QColor(8, 6, 16, 150));     // parte ya reproducida

    const double sr = t->sampleRate, dur = static_cast<double>(t->frames);
    if (m_deck->loopActive()) {
        const double x0 = m_deck->loopStart() * sr / dur * W, x1 = m_deck->loopEnd() * sr / dur * W;
        p->fillRect(QRectF(x0, 0, std::max(2.0, x1 - x0), H), QColor(80, 228, 120, 70));
    }
    const QVariantList hc = m_deck->hotCues();
    for (int i = 0; i < hc.size(); ++i) {
        const double s = hc[i].toDouble();
        if (s < 0) continue;
        const double x = s * sr / dur * W;
        p->fillRect(QRectF(x - 1, 0, 2, H), kHot[i % 8]);
    }
    const double cx = m_deck->cuePoint() / dur * W;
    p->fillRect(QRectF(cx - 1, 0, 2, H * 0.35), kCue);
    p->fillRect(QRectF(px - 1, 0, 2, H), m_accent.lighter(130));
}

void WaveformItem::paintScrolling(QPainter* p, const Track* t) {
    const int W = static_cast<int>(width()), H = static_cast<int>(height());
    if (W <= 0 || H <= 0) return;
    const double sr = t->sampleRate;
    const double pos = m_deck->positionFrames();
    const double fpp = m_secs * sr / W;                 // cuadros por píxel
    const double startF = pos - (W / 2.0) * fpp;
    const size_t bins = t->wfLow.size();
    const int bf = t->wfBinFrames;

    // Fondo de loop
    if (m_deck->loopActive()) {
        const double x0 = (m_deck->loopStart() * sr - startF) / fpp;
        const double x1 = (m_deck->loopEnd() * sr - startF) / fpp;
        p->fillRect(QRectF(x0, 0, x1 - x0, H), QColor(80, 228, 120, 45));
    }

    // Rejilla de beats
    if (t->bpm > 0) {
        const double bl = t->beatLength();
        const double k0 = std::ceil((startF - t->firstBeat) / bl);
        for (double k = k0;; k += 1) {
            const double f = t->firstBeat + k * bl;
            const double x = (f - startF) / fpp;
            if (x > W) break;
            const bool bar = static_cast<long long>(std::llround(k)) % 4 == 0;
            p->fillRect(QRectF(x, 0, 1, H), bar ? QColor(255, 255, 255, 70) : QColor(255, 255, 255, 25));
        }
    }

    for (int x = 0; x < W; ++x) {
        const double f0 = startF + x * fpp, f1 = f0 + fpp;
        if (f1 < 0 || f0 >= static_cast<double>(t->frames)) continue;
        const long long b0 = std::max<long long>(0, static_cast<long long>(f0 / bf));
        const long long b1 = std::min<long long>(static_cast<long long>(bins) - 1, static_cast<long long>(f1 / bf));
        uint8_t lo = 0, mi = 0, hi = 0;
        for (long long i = b0; i <= b1; ++i) {
            lo = std::max(lo, t->wfLow[i]); mi = std::max(mi, t->wfMid[i]); hi = std::max(hi, t->wfHigh[i]);
        }
        drawColumn(p, x, H / 2.f, H / 2.f - 2, lo, mi, hi, x < W / 2 ? 0.55f : 1.0f);
    }

    // Marcadores
    auto marker = [&](double frame, const QColor& c, const QString& label) {
        const double x = (frame - startF) / fpp;
        if (x < -20 || x > W + 20) return;
        p->fillRect(QRectF(x - 1, 0, 2, H), c);
        QPolygonF tri;
        tri << QPointF(x - 6, 0) << QPointF(x + 6, 0) << QPointF(x, 8);
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        p->drawPolygon(tri);
        if (!label.isEmpty()) {
            p->fillRect(QRectF(x + 2, H - 15, 13, 14), c);
            p->setPen(QColor(10, 8, 20));
            QFont fnt = p->font(); fnt.setPixelSize(10); fnt.setBold(true); p->setFont(fnt);
            p->drawText(QRectF(x + 2, H - 15, 13, 14), Qt::AlignCenter, label);
        }
    };
    marker(m_deck->cuePoint(), kCue, QString());
    const QVariantList hc = m_deck->hotCues();
    for (int i = 0; i < hc.size(); ++i) {
        const double s = hc[i].toDouble();
        if (s >= 0) marker(s * sr, kHot[i % 8], QString::number(i + 1));
    }

    // Cabezal
    p->fillRect(QRectF(W / 2.0 - 1, 0, 3, H), QColor(255, 255, 255, 235));
    QColor glow = m_accent; glow.setAlpha(90);
    p->fillRect(QRectF(W / 2.0 - 4, 0, 9, H), glow);
}

} // namespace jx
