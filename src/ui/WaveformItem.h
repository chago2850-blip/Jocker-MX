// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "app/DeckController.h"

#include <QColor>
#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Forma de onda de 3 bandas: vista general (toda la pista) o desplazable (centrada en el cabezal).
class WaveformItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(jx::DeckController* deck READ deck WRITE setDeck NOTIFY deckChanged)
    Q_PROPERTY(bool scrolling READ scrolling WRITE setScrolling NOTIFY changed)
    Q_PROPERTY(double secondsVisible READ secondsVisible WRITE setSecondsVisible NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY changed)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY changed)

public:
    explicit WaveformItem(QQuickItem* parent = nullptr);

    DeckController* deck() const { return m_deck; }
    void setDeck(DeckController* d);
    bool scrolling() const { return m_scrolling; }
    void setScrolling(bool s) { m_scrolling = s; m_cache = QImage(); emit changed(); update(); }
    double secondsVisible() const { return m_secs; }
    void setSecondsVisible(double s) { m_secs = std::clamp(s, 1.0, 60.0); emit changed(); update(); }
    QColor accent() const { return m_accent; }
    void setAccent(const QColor& c) { m_accent = c; m_cache = QImage(); emit changed(); update(); }
    bool mirrored() const { return m_mirrored; }
    void setMirrored(bool m) { m_mirrored = m; emit changed(); update(); }

    void paint(QPainter* p) override;

signals:
    void deckChanged();
    void changed();

protected:
    void geometryChange(const QRectF& n, const QRectF& o) override;

private:
    void paintOverview(QPainter* p, const Track* t);
    void paintScrolling(QPainter* p, const Track* t);
    void drawColumn(QPainter* p, int x, float cy, float halfH, uint8_t lo, uint8_t mi, uint8_t hi, float alpha);

    QPointer<DeckController> m_deck;
    bool m_scrolling = false;
    bool m_mirrored = false;
    double m_secs = 8.0;
    QColor m_accent{0xe0, 0x21, 0x8a};
    QImage m_cache;
    const Track* m_cacheTrack = nullptr;
};

} // namespace jx
