// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "app/DeckController.h"

#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Muestra los gráficos CD+G del deck, escalados y sin deformar, con el color de borde de fondo.
class CdgItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(jx::DeckController* deck READ deck WRITE setDeck NOTIFY deckChanged)
    Q_PROPERTY(bool smooth READ smoothScaling WRITE setSmoothScaling NOTIFY deckChanged)
public:
    explicit CdgItem(QQuickItem* parent = nullptr);
    DeckController* deck() const { return m_deck; }
    void setDeck(DeckController* d);
    bool smoothScaling() const { return m_smooth; }
    void setSmoothScaling(bool s) { m_smooth = s; update(); }
    void paint(QPainter* p) override;
signals:
    void deckChanged();
private:
    QPointer<DeckController> m_deck;
    bool m_smooth = false;
};

} // namespace jx
