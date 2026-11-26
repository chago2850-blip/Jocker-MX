// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "DeckVideo.h"

#include <QElapsedTimer>
#include <algorithm>
#include <QFileInfo>
#include <QUrl>
#include <cmath>

#ifdef JX_HAVE_VIDEO
#include <QMediaPlayer>
#include <QVideoFrame>
#include <QVideoSink>
#include <QGuiApplication>
#endif

namespace jx {

struct DeckVideo::Impl {
#ifdef JX_HAVE_VIDEO
    std::unique_ptr<QMediaPlayer> player;
    std::unique_ptr<QVideoSink> sink;   // se crea al abrir el primer video (no iniciar el backend sin necesidad)
    QVideoFrame frame;
#endif
    QElapsedTimer sinceSeek;
    double lastRate = 1.0;
    bool wasPlaying = false;
};

DeckVideo::DeckVideo(QObject* parent) : QObject(parent), d(std::make_unique<Impl>()) {
    d->sinceSeek.start();
}

DeckVideo::~DeckVideo() = default;

void DeckVideo::prepareBackend() {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    static bool done = false;
    if (done) return;
    done = true;
    QByteArray ranks = qgetenv("GST_PLUGIN_FEATURE_RANK");
    for (const char* f : {"dfbvideosink:NONE", "kmssink:NONE", "fbdevsink:NONE"}) {
        const QByteArray name = QByteArray(f).split(':').first();
        if (ranks.contains(name)) continue;
        if (!ranks.isEmpty()) ranks += ',';
        ranks += f;
    }
    qputenv("GST_PLUGIN_FEATURE_RANK", ranks);
#endif
}

bool DeckVideo::available() {
#ifdef JX_HAVE_VIDEO
    return true;
#else
    return false;
#endif
}

bool DeckVideo::isVideoFile(const QString& path) {
    static const QStringList exts = {"mp4", "m4v", "mov", "mkv", "webm", "avi", "mpg", "mpeg", "wmv", "flv"};
    return exts.contains(QFileInfo(path).suffix().toLower());
}

double DeckVideo::mixWeightB(double xf, int transition) {
    const double t = std::clamp((xf + 1.0) / 2.0, 0.0, 1.0);
    if (transition == 2) return t < 0.5 ? 0.0 : 1.0;   // corte en el centro
    return t;                                          // fundido / barrido
}

void DeckVideo::open(const QString& path) {
    const bool had = m_hasVideo;
    m_hasVideo = false;
    m_error.clear();
    m_image = QImage();
    m_imageDirty = false;
    m_frames = 0;
#ifdef JX_HAVE_VIDEO
    d->frame = QVideoFrame();
    if (d->player) {
        d->player->stop();
        d->player.reset();
    }
    if (!path.isEmpty() && isVideoFile(path)) {
        if (!d->sink) {
            prepareBackend();
            d->sink = std::make_unique<QVideoSink>();
            connect(d->sink.get(), &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& f) {
                if (!f.isValid()) return;
                d->frame = f;
                m_imageDirty = true;
                ++m_frames;
                emit frameChanged();
            });
        }
        d->player = std::make_unique<QMediaPlayer>();
        d->player->setVideoSink(d->sink.get());   // sin salida de audio: el sonido es del motor
        connect(d->player.get(), &QMediaPlayer::hasVideoChanged, this, [this](bool v) {
            if (v == m_hasVideo) return;
            m_hasVideo = v;
            emit hasVideoChanged();
        });
        connect(d->player.get(), &QMediaPlayer::errorOccurred, this,
                [this](QMediaPlayer::Error, const QString& msg) { m_error = msg; });
        d->player->setSource(QUrl::fromLocalFile(path));
        d->player->pause();                  // mostrar el primer cuadro
        d->wasPlaying = false;
        d->lastRate = 1.0;
    }
#else
    Q_UNUSED(path);
#endif
    if (had != m_hasVideo) emit hasVideoChanged();
    emit frameChanged();
}

double DeckVideo::videoSeconds() const {
#ifdef JX_HAVE_VIDEO
    return d->player ? d->player->position() / 1000.0 : 0.0;
#else
    return 0.0;
#endif
}

void DeckVideo::sync(double deckSeconds, bool playing, double rate) {
#ifdef JX_HAVE_VIDEO
    QMediaPlayer* p = d->player.get();
    if (!p || p->mediaStatus() == QMediaPlayer::LoadingMedia || p->mediaStatus() == QMediaPlayer::NoMedia) return;
    const double dur = p->duration() / 1000.0;
    const double target = dur > 0 ? std::clamp(deckSeconds, 0.0, std::max(0.0, dur - 0.04)) : deckSeconds;
    const double drift = p->position() / 1000.0 - target;
    const bool canSeek = d->sinceSeek.elapsed() > 250;   // no saturar al decodificador con búsquedas

    if (playing && rate > 0.05) {
        if (std::abs(rate - d->lastRate) > 0.003) { p->setPlaybackRate(rate); d->lastRate = rate; }
        if (!d->wasPlaying || p->playbackState() != QMediaPlayer::PlayingState) {
            p->setPosition(qint64(target * 1000));
            p->play();
            d->sinceSeek.restart();
            ++m_seeks;
        } else if (std::abs(drift) > 0.12 && canSeek) {
            // Se adelanta un poco para compensar lo que tarda en decodificar tras la búsqueda.
            p->setPosition(qint64((target + 0.03 * rate) * 1000));
            d->sinceSeek.restart();
            ++m_seeks;
        }
        d->wasPlaying = true;
    } else {
        if (p->playbackState() == QMediaPlayer::PlayingState) p->pause();
        // En pausa / scratch: mostrar el cuadro exacto de la posición del deck.
        if (std::abs(drift) > 0.035 && (canSeek || d->wasPlaying)) {
            p->setPosition(qint64(target * 1000));
            d->sinceSeek.restart();
            ++m_seeks;
        }
        d->wasPlaying = false;
    }
#else
    Q_UNUSED(deckSeconds); Q_UNUSED(playing); Q_UNUSED(rate);
#endif
}

#ifdef JX_HAVE_VIDEO
namespace {
inline uint8_t clamp8(int v) { return uint8_t(v < 0 ? 0 : (v > 255 ? 255 : v)); }

// YUV (rango limitado) → RGB. BT.709 para HD, BT.601 para SD.
struct YuvMatrix { int rv, gu, gv, bu; };
inline QRgb yuv(int y, int u, int v, const YuvMatrix& m) {
    const int c = (y - 16) * 298, d = u - 128, e = v - 128;
    return qRgb(clamp8((c + m.rv * e + 128) >> 8), clamp8((c - m.gu * d - m.gv * e + 128) >> 8),
                clamp8((c + m.bu * d + 128) >> 8));
}

// Convierte el cuadro sin depender del renderizador de Qt (que exige aplicación gráfica en Qt 6.4).
// Reduce a la mitad los cuadros de más de 1280 px de ancho: suficiente para mostrar y mucho más rápido.
QImage convertFrame(QVideoFrame f) {
    if (!f.map(QVideoFrame::ReadOnly)) return {};
    const int w = f.width(), h = f.height();
    const int step = w > 1280 ? 2 : 1;
    const int ow = w / step, oh = h / step;
    const YuvMatrix m = h >= 720 ? YuvMatrix{459, 55, 136, 541} : YuvMatrix{409, 100, 208, 516};
    QImage out;
    switch (f.pixelFormat()) {
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YV12: {
        const bool yv12 = f.pixelFormat() == QVideoFrameFormat::Format_YV12;
        const uchar* Y = f.bits(0);
        const uchar* U = f.bits(yv12 ? 2 : 1);
        const uchar* V = f.bits(yv12 ? 1 : 2);
        const int ys = f.bytesPerLine(0), us = f.bytesPerLine(yv12 ? 2 : 1), vs = f.bytesPerLine(yv12 ? 1 : 2);
        out = QImage(ow, oh, QImage::Format_RGB32);
        for (int oy = 0; oy < oh; ++oy) {
            const int y = oy * step;
            auto* line = reinterpret_cast<QRgb*>(out.scanLine(oy));
            for (int ox = 0; ox < ow; ++ox) {
                const int x = ox * step;
                line[ox] = yuv(Y[y * ys + x], U[(y / 2) * us + x / 2], V[(y / 2) * vs + x / 2], m);
            }
        }
        break;
    }
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21: {
        const bool nv21 = f.pixelFormat() == QVideoFrameFormat::Format_NV21;
        const uchar* Y = f.bits(0);
        const uchar* UV = f.bits(1);
        const int ys = f.bytesPerLine(0), uvs = f.bytesPerLine(1);
        out = QImage(ow, oh, QImage::Format_RGB32);
        for (int oy = 0; oy < oh; ++oy) {
            const int y = oy * step;
            auto* line = reinterpret_cast<QRgb*>(out.scanLine(oy));
            for (int ox = 0; ox < ow; ++ox) {
                const int x = ox * step;
                const uchar* c = UV + (y / 2) * uvs + (x / 2) * 2;
                line[ox] = yuv(Y[y * ys + x], nv21 ? c[1] : c[0], nv21 ? c[0] : c[1], m);
            }
        }
        break;
    }
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        // bytes B,G,R,A = QImage::Format_RGB32 en máquinas little-endian
        out = QImage(f.bits(0), w, h, f.bytesPerLine(0), QImage::Format_RGB32).copy();
        break;
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
        out = QImage(f.bits(0), w, h, f.bytesPerLine(0), QImage::Format_RGBX8888).convertToFormat(QImage::Format_RGB32);
        break;
    default:
        break;
    }
    f.unmap();
    if (out.isNull() && qobject_cast<QGuiApplication*>(QCoreApplication::instance())) out = f.toImage();
    return out;
}
} // namespace
#endif

const QImage& DeckVideo::image() {
#ifdef JX_HAVE_VIDEO
    if (m_imageDirty) {
        QImage img = convertFrame(d->frame);
        if (!img.isNull()) m_image = img;
        m_imageDirty = false;
    }
#endif
    return m_image;
}

} // namespace jx
