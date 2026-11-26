// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "StemModel.h"

#define EIGEN_MPL2_ONLY
#include <Eigen/Core>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <cmath>
#include <cstring>
#include <thread>

namespace jx {

namespace {
using RowMat = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MapC = Eigen::Map<const RowMat>;
using Map = Eigen::Map<RowMat>;

float halfToFloat(uint16_t h) {
    const uint32_t s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    uint32_t f;
    if (e == 0) {
        if (m == 0) f = s << 31;
        else {                                  // subnormal
            int ee = -1; uint32_t mm = m;
            do { ++ee; mm <<= 1; } while ((mm & 0x400) == 0);
            f = (s << 31) | (static_cast<uint32_t>(127 - 15 - ee) << 23) | ((mm & 0x3FF) << 13);
        }
    } else if (e == 31) f = (s << 31) | 0x7F800000 | (m << 13);
    else f = (s << 31) | ((e + 112) << 23) | (m << 13);
    float out;
    std::memcpy(&out, &f, 4);
    return out;
}

inline float elu(float x) { return x > 0.f ? x : std::expm1(x); }

// Conv2D 5×5, paso 2, relleno "same" (TF: 1 arriba/izquierda, 2 abajo/derecha). NHWC.
void conv5s2(const float* in, int H, int W, int Ci, const std::vector<float>& k, const std::vector<float>& b, int Co,
             std::vector<float>& out, std::vector<float>& col) {
    const int Ho = H / 2, Wo = W / 2, K = 25 * Ci;
    col.assign(static_cast<size_t>(Ho) * Wo * K, 0.f);
    for (int oy = 0; oy < Ho; ++oy)
        for (int ox = 0; ox < Wo; ++ox) {
            float* row = &col[(static_cast<size_t>(oy) * Wo + ox) * K];
            for (int ky = 0; ky < 5; ++ky) {
                const int iy = oy * 2 + ky - 1;
                if (iy < 0 || iy >= H) continue;
                for (int kx = 0; kx < 5; ++kx) {
                    const int ix = ox * 2 + kx - 1;
                    if (ix < 0 || ix >= W) continue;
                    std::memcpy(row + (ky * 5 + kx) * Ci, in + (static_cast<size_t>(iy) * W + ix) * Ci, sizeof(float) * Ci);
                }
            }
        }
    out.resize(static_cast<size_t>(Ho) * Wo * Co);
    Map O(out.data(), static_cast<Eigen::Index>(Ho) * Wo, Co);
    O.noalias() = MapC(col.data(), static_cast<Eigen::Index>(Ho) * Wo, K) * MapC(k.data(), K, Co);
    O.rowwise() += Eigen::Map<const Eigen::RowVectorXf>(b.data(), Co);
}

// Conv2DTranspose 5×5, paso 2, "same": salida 2H × 2W. Kernel TF [5,5,Co,Ci] reordenado a [Ci, 25·Co].
void deconv5s2(const float* in, int H, int W, int Ci, const std::vector<float>& kT, const std::vector<float>& b, int Co,
               std::vector<float>& out, std::vector<float>& tmp) {
    const int H2 = H * 2, W2 = W * 2, N = 25 * Co;
    tmp.resize(static_cast<size_t>(H) * W * N);
    Map P(tmp.data(), static_cast<Eigen::Index>(H) * W, N);
    P.noalias() = MapC(in, static_cast<Eigen::Index>(H) * W, Ci) * MapC(kT.data(), Ci, N);
    out.assign(static_cast<size_t>(H2) * W2 * Co, 0.f);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const float* p = &tmp[(static_cast<size_t>(y) * W + x) * N];
            for (int ky = 0; ky < 5; ++ky) {
                const int oy = y * 2 + ky - 1;
                if (oy < 0 || oy >= H2) continue;
                for (int kx = 0; kx < 5; ++kx) {
                    const int ox = x * 2 + kx - 1;
                    if (ox < 0 || ox >= W2) continue;
                    float* o = &out[(static_cast<size_t>(oy) * W2 + ox) * Co];
                    const float* pp = p + (ky * 5 + kx) * Co;
                    for (int c = 0; c < Co; ++c) o[c] += pp[c];
                }
            }
        }
    const size_t px = static_cast<size_t>(H2) * W2;
    for (size_t i = 0; i < px; ++i)
        for (int c = 0; c < Co; ++c) out[i * Co + c] += b[c];
}
} // namespace

bool StemModel::load(const QString& path, QString* error) {
    m_nets.clear();
    m_names.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("No se encontró el modelo de stems (%1)").arg(path);
        return false;
    }
    const QByteArray all = f.readAll();
    const char* p = all.constData();
    const char* end = p + all.size();
    auto need = [&](size_t n) { return p + n <= end; };
    auto u32 = [&]() { uint32_t v; std::memcpy(&v, p, 4); p += 4; return v; };
    if (!need(12) || std::memcmp(p, "JXSTEMS1", 8) != 0) {
        if (error) *error = QStringLiteral("Archivo de modelo inválido");
        return false;
    }
    p += 8;
    const uint32_t nInst = u32();
    auto readTensor = [&](Tensor& t) -> bool {
        if (!need(5)) return false;
        const uint8_t type = static_cast<uint8_t>(*p++);
        const uint32_t nd = u32();
        if (nd > 4 || !need(4 * nd)) return false;
        t.shape.resize(nd);
        size_t count = 1;
        for (uint32_t i = 0; i < nd; ++i) { t.shape[i] = static_cast<int>(u32()); count *= t.shape[i]; }
        t.data.resize(count);
        if (type == 0) {
            if (!need(count * 2)) return false;
            for (size_t i = 0; i < count; ++i) { uint16_t h; std::memcpy(&h, p + 2 * i, 2); t.data[i] = halfToFloat(h); }
            p += count * 2;
        } else {
            if (!need(count * 4)) return false;
            std::memcpy(t.data.data(), p, count * 4);
            p += count * 4;
        }
        return true;
    };
    for (uint32_t n = 0; n < nInst; ++n) {
        if (!need(4)) break;
        const uint32_t len = u32();
        if (!need(len + 4)) break;
        m_names << QString::fromUtf8(p, static_cast<int>(len));
        p += len;
        const uint32_t nt = u32();
        if (nt != 50) { m_nets.clear(); break; }
        Net net;
        bool ok = true;
        for (auto& l : net.enc) ok = ok && readTensor(l.kernel) && readTensor(l.bias) && readTensor(l.scale) && readTensor(l.shift);
        for (auto& l : net.dec) {
            ok = ok && readTensor(l.kernel) && readTensor(l.bias) && readTensor(l.scale) && readTensor(l.shift);
            if (!ok) break;
            // [5,5,Co,Ci] → [Ci, 25·Co] para el GEMM de la convolución transpuesta
            const int Co = l.kernel.shape[2], Ci = l.kernel.shape[3];
            std::vector<float> t(static_cast<size_t>(Ci) * 25 * Co);
            for (int k = 0; k < 25; ++k)
                for (int co = 0; co < Co; ++co)
                    for (int ci = 0; ci < Ci; ++ci)
                        t[static_cast<size_t>(ci) * 25 * Co + k * Co + co] = l.kernel.data[(static_cast<size_t>(k) * Co + co) * Ci + ci];
            l.kernel.data.swap(t);
        }
        ok = ok && readTensor(net.finalK) && readTensor(net.finalB);
        if (!ok) { m_nets.clear(); break; }
        m_nets.push_back(std::move(net));
    }
    if (m_nets.empty()) {
        m_names.clear();
        if (error) *error = QStringLiteral("Modelo de stems dañado o incompleto");
        return false;
    }
    return true;
}

void StemModel::runNet(const Net& n, const float* in, std::vector<float>& out) const {
    static const int filters[6] = {16, 32, 64, 128, 256, 512};
    std::vector<float> c[7];      // salidas de conv (antes de BN), c[0] = entrada
    std::vector<float> col, act[2];
    int H = kT, W = kF, C = 2;
    const float* x = in;
    for (int i = 0; i < 6; ++i) {
        conv5s2(x, H, W, C, n.enc[i].kernel.data, n.enc[i].bias.data, filters[i], c[i + 1], col);
        H /= 2; W /= 2; C = filters[i];
        if (i < 5) {                       // r = ELU(BN(c)) alimenta la siguiente capa
            std::vector<float>& r = act[i % 2];
            r.resize(c[i + 1].size());
            const auto& s = n.enc[i].scale.data;
            const auto& sh = n.enc[i].shift.data;
            for (size_t j = 0; j < r.size(); ++j) r[j] = elu(c[i + 1][j] * s[j % C] + sh[j % C]);
            x = r.data();
        }
    }
    // Decodificador
    std::vector<float> cur = c[6], up, merged, tmp;
    static const int decF[6] = {256, 128, 64, 32, 16, 1};
    for (int i = 0; i < 6; ++i) {
        const int Co = decF[i];
        deconv5s2(cur.data(), H, W, C, n.dec[i].kernel.data, n.dec[i].bias.data, Co, up, tmp);
        H *= 2; W *= 2;
        const auto& s = n.dec[i].scale.data;
        const auto& sh = n.dec[i].shift.data;
        for (size_t j = 0; j < up.size(); ++j) up[j] = elu(up[j]) * s[j % Co] + sh[j % Co];   // BN(ELU(deconv))
        if (i < 5) {                       // concat([c_skip, b], canales)
            const std::vector<float>& skip = c[5 - i];
            const int Cs = filters[4 - i];
            const size_t px = static_cast<size_t>(H) * W;
            merged.resize(px * (Cs + Co));
            for (size_t q = 0; q < px; ++q) {
                std::memcpy(&merged[q * (Cs + Co)], &skip[q * Cs], sizeof(float) * Cs);
                std::memcpy(&merged[q * (Cs + Co) + Cs], &up[q * Co], sizeof(float) * Co);
            }
            cur.swap(merged);
            C = Cs + Co;
        } else {
            cur.swap(up);
            C = 1;
        }
    }
    // Conv final 4×4, dilatación 2, "same" (relleno 3), sigmoide, × entrada
    const auto& fk = n.finalK.data;   // [4,4,1,2]
    const auto& fb = n.finalB.data;
    out.resize(static_cast<size_t>(kT) * kF * 2);
    for (int y = 0; y < kT; ++y)
        for (int xq = 0; xq < kF; ++xq) {
            float a0 = fb[0], a1 = fb[1];
            for (int ky = 0; ky < 4; ++ky) {
                const int iy = y + ky * 2 - 3;
                if (iy < 0 || iy >= kT) continue;
                for (int kx = 0; kx < 4; ++kx) {
                    const int ix = xq + kx * 2 - 3;
                    if (ix < 0 || ix >= kF) continue;
                    const float v = cur[static_cast<size_t>(iy) * kF + ix];
                    a0 += v * fk[(ky * 4 + kx) * 2];
                    a1 += v * fk[(ky * 4 + kx) * 2 + 1];
                }
            }
            const size_t o = (static_cast<size_t>(y) * kF + xq) * 2;
            out[o] = in[o] / (1.f + std::exp(-a0));
            out[o + 1] = in[o + 1] / (1.f + std::exp(-a1));
        }
}

void StemModel::run(const float* in, std::vector<std::vector<float>>& out, int threads, const std::atomic<bool>* cancel) const {
    out.resize(m_nets.size());
    const int nThreads = std::max(1, std::min(threads, count()));
    std::atomic<int> next{0};
    auto worker = [&]() {
        for (;;) {
            const int i = next.fetch_add(1);
            if (i >= count() || (cancel && cancel->load())) return;
            runNet(m_nets[i], in, out[i]);
        }
    };
    std::vector<std::thread> pool;
    for (int t = 1; t < nThreads; ++t) pool.emplace_back(worker);
    worker();
    for (auto& t : pool) t.join();
}

QString StemModel::findModelFile() {
    const QString name = QStringLiteral("jx-stems-4.bin");
    QStringList candidates;
    const QByteArray env = qgetenv("JX_STEMS_MODEL");
    if (!env.isEmpty()) candidates << QString::fromLocal8Bit(env);
    const QString app = QCoreApplication::applicationDirPath();
    candidates << app + "/models/" + name                       // Windows / portátil
               << app + "/../Resources/models/" + name          // macOS (.app)
               << app + "/../share/jocker-mx/models/" + name    // Linux (/usr/bin → /usr/share)
               << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models/" + name
               << QStringLiteral("assets:/models/") + name;     // Android
    for (const QString& c : candidates) {
        if (!QFileInfo::exists(c)) continue;
        return c.startsWith(QLatin1String("assets:")) ? c : QFileInfo(c).absoluteFilePath();
    }
    return {};
}

} // namespace jx
