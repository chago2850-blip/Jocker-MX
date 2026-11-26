// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QVector>

namespace jx {

// Letras sincronizadas en formato LRC (también el «mejorado» con tiempos por palabra <mm:ss.xx>).
struct LrcLine {
    double time = 0;                    // segundos
    QString text;
    QVector<QPair<double, int>> words;  // (tiempo, índice del carácter donde empieza la palabra)
};

class Lyrics {
public:
    bool load(const QString& path, QString* error = nullptr);
    void parse(const QString& text);
    bool isEmpty() const { return m_lines.isEmpty(); }
    const QVector<LrcLine>& lines() const { return m_lines; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }

    int lineAt(double seconds) const;                 // -1 antes de la primera línea
    double progressAt(double seconds, int line) const; // 0..1 dentro de la línea (por palabras si las hay)

private:
    QVector<LrcLine> m_lines;
    QString m_title, m_artist;
};

} // namespace jx
