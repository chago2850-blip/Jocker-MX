// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "KeyDetector.h"

#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace jx {

namespace {
constexpr double kPi = 3.14159265358979323846;

// Número de Camelot de cada tónica (C=0 … B=11)
const int kCamelotMajor[12] = {8, 3, 10, 5, 12, 7, 2, 9, 4, 11, 6, 1};   // C=8B, Db=3B, D=10B …
const int kCamelotMinor[12] = {5, 12, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10};   // Cm=5A, C#m=12A … Am=8A
const char* kNotesEs[12] = {"Do", "Do♯", "Re", "Mi♭", "Mi", "Fa", "Fa♯", "Sol", "La♭", "La", "Si♭", "Si"};

// Perfiles de tonalidad de Krumhansl-Kessler (1982)
const double kMajor[12] = {6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88};
const double kMinor[12] = {6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17};

void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2 * kPi / static_cast<double>(len);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t j = 0; j < len / 2; ++j) {
                const auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

double correlate(const double* chroma, const double* profile, int shift) {
    double mc = 0, mp = 0;
    for (int i = 0; i < 12; ++i) { mc += chroma[i]; mp += profile[i]; }
    mc /= 12; mp /= 12;
    double num = 0, dc = 0, dp = 0;
    for (int i = 0; i < 12; ++i) {
        const double c = chroma[(i + shift) % 12] - mc;
        const double p = profile[i] - mp;
        num += c * p; dc += c * c; dp += p * p;
    }
    return (dc > 0 && dp > 0) ? num / std::sqrt(dc * dp) : 0;
}
} // namespace

QString KeyResult::camelot() const {
    if (tonic < 0) return {};
    return QString::number(minor ? kCamelotMinor[tonic] : kCamelotMajor[tonic]) + (minor ? "A" : "B");
}

QString KeyResult::name() const {
    if (tonic < 0) return {};
    return QString::fromUtf8(kNotesEs[tonic]) + (minor ? " menor" : " mayor");
}

QString KeyResult::openKey() const {
    if (tonic < 0) return {};
    // Open Key: 1d = Do mayor; Camelot n → Open Key (n + 4) mod 12 (+1)
    const int cam = minor ? kCamelotMinor[tonic] : kCamelotMajor[tonic];
    const int ok = ((cam - 8 + 12) % 12) + 1;
    return QString::number(ok) + (minor ? "m" : "d");
}

KeyResult detectKey(const float* pcm, int64_t frames, double sr) {
    KeyResult r;
    const int N = 8192;
    const int hop = 8192;
    if (frames < N * 4) return r;

    // Ventana de Hann y mapa bin → clase de altura (55 Hz – 2 kHz)
    std::vector<double> win(N);
    for (int i = 0; i < N; ++i) win[i] = 0.5 - 0.5 * std::cos(2 * kPi * i / (N - 1));
    std::vector<int> pc(N / 2, -1);
    std::vector<double> wgt(N / 2, 0.0);
    for (int b = 1; b < N / 2; ++b) {
        const double f = b * sr / N;
        if (f < 55.0 || f > 2000.0) continue;
        const double midi = 69.0 + 12.0 * std::log2(f / 440.0);
        const double nearest = std::round(midi);
        if (std::abs(midi - nearest) > 0.4) continue;      // descarta bins entre semitonos
        pc[b] = static_cast<int>(static_cast<long long>(nearest) % 12);
        wgt[b] = f < 140.0 ? 1.6 : 1.0;                      // el bajo suele marcar la tónica
    }

    double chroma[12] = {0};
    std::vector<std::complex<double>> buf(N);
    const int64_t maxStart = std::min<int64_t>(frames - N, static_cast<int64_t>(sr * 240));
    // Saltarse los primeros segundos (intros percusivas) si la pista es larga
    const int64_t start0 = frames > sr * 60 ? static_cast<int64_t>(sr * 8) : 0;
    for (int64_t s = start0; s < maxStart; s += hop) {
        double energy = 0;
        for (int i = 0; i < N; ++i) {
            const double x = 0.5 * (pcm[(s + i) * 2] + pcm[(s + i) * 2 + 1]);
            energy += x * x;
            buf[i] = std::complex<double>(x * win[i], 0.0);
        }
        if (energy / N < 1e-6) continue;                    // silencio
        fft(buf);
        double frame[12] = {0};
        for (int b = 1; b < N / 2; ++b) {
            if (pc[b] < 0) continue;
            frame[pc[b]] += wgt[b] * std::log1p(std::abs(buf[b]));
        }
        double mx = 0;
        for (double v : frame) mx = std::max(mx, v);
        if (mx <= 0) continue;
        for (int i = 0; i < 12; ++i) chroma[i] += frame[i] / mx;
    }

    double best = -2;
    for (int t = 0; t < 12; ++t) {
        const double cMaj = correlate(chroma, kMajor, t);
        const double cMin = correlate(chroma, kMinor, t);
        if (cMaj > best) { best = cMaj; r.tonic = t; r.minor = false; }
        if (cMin > best) { best = cMin; r.tonic = t; r.minor = true; }
    }
    r.confidence = best;
    if (best < 0.2) r.tonic = -1;
    return r;
}

KeyResult keyFromCamelot(const QString& camelot) {
    KeyResult r;
    static const QRegularExpression re(QStringLiteral("^(\\d{1,2})([ABab])$"));
    const auto m = re.match(camelot.trimmed());
    if (!m.hasMatch()) return r;
    const int n = m.captured(1).toInt();
    const bool minor = m.captured(2).toUpper() == "A";
    for (int t = 0; t < 12; ++t)
        if ((minor ? kCamelotMinor[t] : kCamelotMajor[t]) == n) { r.tonic = t; r.minor = minor; }
    return r;
}

QString normalizeKeyToCamelot(const QString& text) {
    QString s = text.trimmed();
    if (s.isEmpty()) return {};
    if (keyFromCamelot(s).tonic >= 0) return s.toUpper();
    // Open Key: "1m", "6d"
    static const QRegularExpression openRe(QStringLiteral("^(\\d{1,2})([dmDM])$"));
    auto om = openRe.match(s);
    if (om.hasMatch()) {
        const int ok = om.captured(1).toInt();
        if (ok >= 1 && ok <= 12) {
            const int cam = ((ok - 1 + 7) % 12) + 1;
            return QString::number(cam) + (om.captured(2).toLower() == "m" ? "A" : "B");
        }
    }
    // Notación musical: "A", "Am", "A minor", "F#m", "Gb", "Bbmin", "c#"
    static const QRegularExpression noteRe(
        QStringLiteral("^([A-Ga-g])([#♯b♭]?)\\s*(m|min|minor|moll|-|maj|major|dur)?$"), QRegularExpression::CaseInsensitiveOption);
    auto nm = noteRe.match(s);
    if (!nm.hasMatch()) return {};
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};   // A B C D E F G
    int t = base[nm.captured(1).toUpper().at(0).unicode() - 'A'];
    const QString acc = nm.captured(2);
    if (acc == "#" || acc == "♯") t = (t + 1) % 12;
    else if (acc == "b" || acc == "♭") t = (t + 11) % 12;
    const QString q = nm.captured(3).toLower();
    // "m" en minúscula indica menor; "M" aislada sería mayor en algunas etiquetas
    bool minor = q == "min" || q == "minor" || q == "moll" || q == "-" || (q == "m" && !nm.captured(3).startsWith('M'));
    if (nm.captured(1).at(0).isLower() && q.isEmpty()) minor = true;   // convención "a" = la menor
    KeyResult r;
    r.tonic = t;
    r.minor = minor;
    return r.camelot();
}

bool keysCompatible(const QString& a, const QString& b) {
    const KeyResult ka = keyFromCamelot(a), kb = keyFromCamelot(b);
    if (ka.tonic < 0 || kb.tonic < 0) return false;
    const int na = (ka.minor ? kCamelotMinor : kCamelotMajor)[ka.tonic];
    const int nb = (kb.minor ? kCamelotMinor : kCamelotMajor)[kb.tonic];
    if (ka.minor == kb.minor) {
        const int d = std::abs(na - nb);
        return d == 0 || d == 1 || d == 11;
    }
    return na == nb;   // relativa mayor/menor
}

} // namespace jx
