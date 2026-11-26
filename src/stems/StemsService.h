// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "StemSeparator.h"
#include "engine/Track.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QThreadPool>
#include <atomic>
#include <functional>
#include <memory>

namespace jx {

// Servicio global de separación de stems: carga el modelo una vez (bajo demanda) y procesa
// las pistas de una en una, en segundo plano y con prioridad baja para no afectar la mezcla.
class StemsService : public QObject {
    Q_OBJECT
public:
    struct Job {
        std::atomic<bool> cancel{false};
        std::atomic<double> progress{0.0};
    };
    using Done = std::function<void(std::shared_ptr<StemData>, QString error)>;

    static StemsService& instance();

    bool modelAvailable() const { return !m_modelPath.isEmpty(); }
    QString modelPath() const { return m_modelPath; }
    void setModelPath(const QString& p) { m_modelPath = p; }

    // `done` se ejecuta en el hilo de `context` (normalmente la UI).
    std::shared_ptr<Job> separate(std::shared_ptr<Track> track, QObject* context, Done done);

    // Bloqueante (pruebas): separa en el hilo actual.
    std::shared_ptr<StemData> separateNow(const Track& t, QString* error);

private:
    StemsService();
    bool ensureModel(QString* error);

    QString m_modelPath;
    StemModel m_model;
    QMutex m_modelMutex;
    QThreadPool m_pool;
    QMutex m_cacheMutex;
    QHash<QString, std::weak_ptr<StemData>> m_cache;   // pistas ya separadas aún en memoria
};

} // namespace jx
