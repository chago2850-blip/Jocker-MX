// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "Lrc.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>
#include <cmath>
#include <algorithm>

namespace jx {

namespace {
double parseStamp(const QString& m, const QString& s, const QString& frac) {
    double t = m.toInt() * 60.0 + s.toInt();
    if (!frac.isEmpty()) t += frac.toInt() / std::pow(10.0, frac.size());
    return t;
}
} // namespace

bool Lyrics::load(const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    const QByteArray raw = f.readAll();
    // UTF-8 si es válido; si no, Latin-1 (muchos .lrc antiguos vienen así)
    QStringDecoder utf8(QStringDecoder::Utf8);
    QString text = utf8.decode(raw);
    if (utf8.hasError()) text = QString::fromLatin1(raw);
    parse(text);
    return !m_lines.isEmpty();
}

void Lyrics::parse(const QString& text) {
    m_lines.clear();
    m_title.clear();
    m_artist.clear();
    static const QRegularExpression stamp(QStringLiteral(R"(\[(\d{1,3}):(\d{1,2})(?:[.:](\d{1,3}))?\])"));
    static const QRegularExpression tag(QStringLiteral(R"(^\[([a-zA-Z]+):(.*)\]\s*$)"));
    static const QRegularExpression word(QStringLiteral(R"(<(\d{1,3}):(\d{1,2})(?:[.:](\d{1,3}))?>)"));
    double offset = 0;
    const QStringList rows = text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")));
    for (const QString& rowRaw : rows) {
        const QString row = rowRaw.trimmed();
        if (row.isEmpty()) continue;
        if (auto tm = tag.match(row); tm.hasMatch() && !stamp.match(row).hasMatch()) {
            const QString k = tm.captured(1).toLower(), v = tm.captured(2).trimmed();
            if (k == QLatin1String("offset")) offset = v.toDouble() / 1000.0;   // ms; positivo = antes
            else if (k == QLatin1String("ti")) m_title = v;
            else if (k == QLatin1String("ar")) m_artist = v;
            continue;
        }
        // Una o varias marcas de tiempo al principio de la línea
        QVector<double> times;
        int pos = 0;
        auto it = stamp.globalMatch(row);
        while (it.hasNext()) {
            auto m = it.next();
            if (m.capturedStart() != pos) break;
            times << parseStamp(m.captured(1), m.captured(2), m.captured(3));
            pos = m.capturedEnd();
        }
        if (times.isEmpty()) continue;
        QString body = row.mid(pos);
        // Tiempos por palabra
        LrcLine base;
        QString clean;
        int last = 0;
        auto wi = word.globalMatch(body);
        while (wi.hasNext()) {
            auto m = wi.next();
            clean += body.mid(last, m.capturedStart() - last);
            base.words << qMakePair(parseStamp(m.captured(1), m.captured(2), m.captured(3)) - offset, int(clean.size()));
            last = m.capturedEnd();
        }
        clean += body.mid(last);
        base.text = clean.trimmed();
        const int lead = clean.size() - clean.trimmed().size() > 0 ? int(clean.indexOf(base.text)) : 0;
        for (auto& w : base.words) w.second = std::clamp(w.second - lead, 0, int(base.text.size()));
        for (double t : times) {
            LrcLine l = base;
            l.time = std::max(0.0, t - offset);
            m_lines << l;
        }
    }
    std::stable_sort(m_lines.begin(), m_lines.end(), [](const LrcLine& a, const LrcLine& b) { return a.time < b.time; });
}

int Lyrics::lineAt(double s) const {
    auto it = std::upper_bound(m_lines.begin(), m_lines.end(), s,
                               [](double v, const LrcLine& l) { return v < l.time; });
    return int(it - m_lines.begin()) - 1;
}

double Lyrics::progressAt(double s, int line) const {
    if (line < 0 || line >= m_lines.size()) return 0.0;
    const LrcLine& l = m_lines[line];
    const double end = line + 1 < m_lines.size() ? m_lines[line + 1].time : l.time + 5.0;
    const int n = std::max<int>(1, int(l.text.size()));
    if (!l.words.isEmpty()) {
        // Avance por palabra, interpolando dentro de la palabra actual
        int chars = 0;
        for (int i = 0; i < l.words.size(); ++i) {
            const double t0 = l.words[i].first;
            if (s < t0) break;
            const double t1 = i + 1 < l.words.size() ? l.words[i + 1].first : end;
            const int c0 = l.words[i].second, c1 = i + 1 < l.words.size() ? l.words[i + 1].second : n;
            const double f = t1 > t0 ? std::clamp((s - t0) / (t1 - t0), 0.0, 1.0) : 1.0;
            chars = int(c0 + (c1 - c0) * f);
        }
        return std::clamp(double(chars) / n, 0.0, 1.0);
    }
    // Sin tiempos por palabra: el barrido ocupa el 85 % del tiempo de la línea
    const double span = std::max(0.3, (end - l.time) * 0.85);
    return std::clamp((s - l.time) / span, 0.0, 1.0);
}

} // namespace jx
