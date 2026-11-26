// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QImage>
#include <QObject>
#include <QString>
#include <memory>

class QMediaPlayer;
class QVideoSink;

namespace jx {

// Video de un deck. El audio lo sigue decodificando y reproduciendo el motor de Jocker-MX;
// este reproductor (sin audio) sólo dibuja la imagen y la mantiene pegada a la posición del deck:
// misma velocidad (tempo), pausa, scratch/búsquedas y cue.
class DeckVideo : public QObject {
    Q_OBJECT
public:
    explicit DeckVideo(QObject* parent = nullptr);
    ~DeckVideo() override;

    static bool available();                       // ¿se compiló con Qt Multimedia?
    // Ajusta GStreamer (Linux) antes de iniciarlo: descarta salidas de video que no sirven y pueden
    // colgar la app (DirectFB, KMS, framebuffer). Se llama sola; idempotente.
    static void prepareBackend();
    static bool isVideoFile(const QString& path);  // por extensión
    // Peso del deck B (0..1) en la mezcla de video. transition: 0 fundido, 1 barrido, 2 corte.
    static double mixWeightB(double crossfader, int transition);

    void open(const QString& path);                // vacío = cerrar
    void close() { open(QString()); }
    bool hasVideo() const { return m_hasVideo; }
    QString error() const { return m_error; }

    // Llamar ~30–60 veces por segundo con el estado del deck.
    void sync(double deckSeconds, bool playing, double rate);

    const QImage& image();                         // último cuadro (convertido una sola vez)
    qint64 frameCount() const { return m_frames; }
    double videoSeconds() const;                   // posición del reproductor de video
    qint64 seekCount() const { return m_seeks; }

signals:
    void frameChanged();
    void hasVideoChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
    bool m_hasVideo = false;
    QString m_error;
    QImage m_image;
    bool m_imageDirty = false;
    qint64 m_frames = 0, m_seeks = 0;
};

} // namespace jx
