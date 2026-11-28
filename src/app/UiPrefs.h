// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QSettings>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Preferencias de la interfaz: táctil (botones grandes, pestañas abajo, control remoto de TV) o escritorio.
class UiPrefs : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(bool touch READ touch WRITE setTouch NOTIFY touchChanged)
    Q_PROPERTY(bool android READ android CONSTANT)
public:
    explicit UiPrefs(QObject* parent = nullptr) : QObject(parent) {
        m_touch = QSettings().value("ui/touch", android()).toBool();
    }
    static constexpr bool isAndroid() {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
    }
    bool android() const { return isAndroid(); }
    bool touch() const { return m_touch; }
    void setTouch(bool t) {
        if (t == m_touch) return;
        m_touch = t;
        QSettings().setValue("ui/touch", t);
        emit touchChanged();
    }
    void forceTouch(bool t) { m_touch = t; emit touchChanged(); }   // desde la línea de comandos, sin guardar
signals:
    void touchChanged();
private:
    bool m_touch = false;
};

} // namespace jx
