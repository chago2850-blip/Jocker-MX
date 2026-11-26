// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "app/DeckController.h"

#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Dibuja una imagen ajustada (sin deformar) o recortada para llenar.
void drawFitted(QPainter* p, const QImage& img, const QRectF& area, bool fill);

// Video de un deck.
class VideoItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(jx::DeckController* deck READ deck WRITE setDeck NOTIFY changed)
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY changed)
public:
    explicit VideoItem(QQuickItem* parent = nullptr);
    DeckController* deck() const { return m_deck; }
    void setDeck(DeckController* d);
    bool fill() const { return m_fill; }
    void setFill(bool f) { m_fill = f; emit changed(); update(); }
    void paint(QPainter* p) override;
signals:
    void changed();
private:
    QPointer<DeckController> m_deck;
    bool m_fill = false;
};

// Mezcla de video de los dos decks siguiendo el crossfader (-1 = A, +1 = B).
class VideoMixItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(jx::DeckController* deckA READ deckA WRITE setDeckA NOTIFY changed)
    Q_PROPERTY(jx::DeckController* deckB READ deckB WRITE setDeckB NOTIFY changed)
    Q_PROPERTY(double crossfader READ crossfader WRITE setCrossfader NOTIFY changed)
    Q_PROPERTY(int transition READ transition WRITE setTransition NOTIFY changed)   // 0 fundido, 1 barrido, 2 corte
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY changed)
public:
    explicit VideoMixItem(QQuickItem* parent = nullptr);
    DeckController* deckA() const { return m_a; }
    DeckController* deckB() const { return m_b; }
    void setDeckA(DeckController* d) { hook(m_a, d); }
    void setDeckB(DeckController* d) { hook(m_b, d); }
    double crossfader() const { return m_xf; }
    void setCrossfader(double x) { if (x != m_xf) { m_xf = x; emit changed(); update(); } }
    int transition() const { return m_transition; }
    void setTransition(int t) { m_transition = t; emit changed(); update(); }
    bool fill() const { return m_fill; }
    void setFill(bool f) { m_fill = f; emit changed(); update(); }
    void paint(QPainter* p) override;

signals:
    void changed();
private:
    void hook(QPointer<DeckController>& slot, DeckController* d);
    QPointer<DeckController> m_a, m_b;
    double m_xf = 0;
    int m_transition = 0;
    bool m_fill = false;
};

} // namespace jx
