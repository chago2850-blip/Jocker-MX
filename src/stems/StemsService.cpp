// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "StemsService.h"

#include <QPointer>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

namespace jx {

StemsService& StemsService::instance() {
    static StemsService s;
    return s;
}

StemsService::StemsService() {
    m_modelPath = StemModel::findModelFile();
    m_pool.setMaxThreadCount(1);              // una pista a la vez
    m_pool.setThreadPriority(QThread::LowPriority);
}

// Al cerrar la app: cancelar la separación en curso y esperarla antes de destruir la caché.
StemsService::~StemsService() {
    {
        QMutexLocker lock(&m_cacheMutex);
        for (const auto& w : m_jobs)
            if (auto j = w.lock()) j->cancel.store(true);
    }
    m_pool.clear();
    m_pool.waitForDone();
}

bool StemsService::ensureModel(QString* error) {
    QMutexLocker lock(&m_modelMutex);
    if (m_model.isLoaded()) return true;
    if (m_modelPath.isEmpty()) {
        if (error) *error = QStringLiteral("Falta el modelo de stems (jx-stems-4.bin)");
        return false;
    }
    return m_model.load(m_modelPath, error);
}

std::shared_ptr<StemData> StemsService::separateNow(const Track& t, QString* error) {
    if (!ensureModel(error)) return nullptr;
    const int threads = std::clamp(QThread::idealThreadCount() - 1, 1, 4);
    return std::shared_ptr<StemData>(separateStems(m_model, t.pcm.data(), t.frames, t.sampleRate, threads, nullptr,
                                                   nullptr, error).release());
}

std::shared_ptr<StemsService::Job> StemsService::separate(std::shared_ptr<Track> track, QObject* context, Done done) {
    auto job = std::make_shared<Job>();
    {   // ¿Ya separada (por ejemplo, la misma pista en el otro deck)?
        QMutexLocker lock(&m_cacheMutex);
        const auto it = m_cache.find(track->path + QString::number(track->sampleRate));
        if (it != m_cache.end()) {
            if (auto sd = it.value().lock(); sd && sd->frames == track->frames) {
                job->progress = 1.0;
                QMetaObject::invokeMethod(context, [done, sd]() { done(sd, QString()); }, Qt::QueuedConnection);
                return job;
            }
        }
    }
    {
        QMutexLocker lock(&m_cacheMutex);
        m_jobs.removeIf([](const std::weak_ptr<Job>& w) { return w.expired(); });
        m_jobs << job;
    }
    QPointer<QObject> ctx(context);
    QtConcurrent::run(&m_pool, [this, track, job, ctx, done]() {
        QString err;
        std::shared_ptr<StemData> sd;
        if (!job->cancel.load() && ensureModel(&err)) {
            const int threads = std::clamp(QThread::idealThreadCount() - 1, 1, 4);
            sd = std::shared_ptr<StemData>(
                separateStems(m_model, track->pcm.data(), track->frames, track->sampleRate, threads,
                              [job](double p) { job->progress.store(p); }, &job->cancel, &err).release());
        }
        if (sd && !job->cancel.load()) {
            QMutexLocker lock(&m_cacheMutex);
            m_cache.insert(track->path + QString::number(track->sampleRate), sd);
        }
        if (job->cancel.load()) return;
        if (ctx) QMetaObject::invokeMethod(ctx.data(), [done, sd, err]() { done(sd, err); }, Qt::QueuedConnection);
    });
    return job;
}

} // namespace jx
