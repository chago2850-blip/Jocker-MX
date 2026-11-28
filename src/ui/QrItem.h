// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>
#include <vector>

namespace jx {

// Código QR de un texto (p. ej. la dirección de la radio local), con margen blanco para leerlo bien.
class QrItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY textChanged)
public:
    explicit QrItem(QQuickItem* parent = nullptr);
    QString text() const { return m_text; }
    void setText(const QString& t);
    QColor color() const { return m_color; }
    void setColor(const QColor& c) { m_color = c; update(); }
    void paint(QPainter* p) override;

    // Módulos del QR (true = oscuro). Público para pruebas.
    static std::vector<std::vector<bool>> encode(const QString& text);
signals:
    void textChanged();
private:
    QString m_text;
    QColor m_color{20, 12, 34};
    std::vector<std::vector<bool>> m_modules;
};

} // namespace jx
