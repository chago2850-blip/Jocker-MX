// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "SamplerController.h"

#include "analysis/Analyzer.h"

#include <QFileInfo>
#include <QSettings>
#include <QUrl>

namespace jx {

SamplerController::SamplerController(AudioEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine), m_s(engine->sampler()) {
    QSettings st;
    for (int i = 0; i < Sampler::kSlots; ++i) {
        const QString path = st.value(QStringLiteral("sampler/%1/path").arg(i)).toString();
        const int mode = st.value(QStringLiteral("sampler/%1/mode").arg(i), i == 5 ? 0 : 0).toInt();
        m_s.mode[i].store(std::clamp(mode, 0, 2));
        if (path.isEmpty() || !loadFile(i, path)) resetSlot(i);
        m_playing << false;
        m_progress << 0.0;
    }
    m_s.volume.store(static_cast<float>(st.value("sampler/volume", 0.8).toDouble()));
    m_error.clear();
    m_timer.setInterval(40);
    connect(&m_timer, &QTimer::timeout, this, &SamplerController::tick);
    m_timer.start();
}

QStringList SamplerController::names() const {
    QStringList l;
    for (const auto& s : m_samples) l << (s ? s->name : QString());
    return l;
}

QVariantList SamplerController::modes() const {
    QVariantList l;
    for (const auto& m : m_s.mode) l << m.load();
    return l;
}

QVariantList SamplerController::custom() const {
    QVariantList l;
    for (const QString& p : m_paths) l << !p.isEmpty();
    return l;
}

void SamplerController::setVolume(double v) {
    m_s.volume.store(static_cast<float>(std::clamp(v, 0.0, 1.0)));
    QSettings().setValue("sampler/volume", v);
    emit settingsChanged();
}

void SamplerController::setPfl(bool b) {
    m_s.pfl.store(b);
    emit settingsChanged();
}

void SamplerController::install(int slot, std::shared_ptr<Sample> s) {
    std::shared_ptr<Sample> old = m_samples[slot];
    m_samples[slot] = std::move(s);
    m_s.setSample(slot, m_samples[slot].get());
    if (old) QTimer::singleShot(2000, this, [old]() {});   // el hilo de audio podría estar leyéndola
}

void SamplerController::press(int slot) {
    if (slot < 0 || slot >= Sampler::kSlots) return;
    m_s.trigger(slot);
}

void SamplerController::release(int slot) {
    if (slot < 0 || slot >= Sampler::kSlots) return;
    if (m_s.mode[slot].load() == Sampler::Hold) m_s.stop(slot);
}

void SamplerController::stopAll() {
    for (int i = 0; i < Sampler::kSlots; ++i) m_s.stop(i);
}

void SamplerController::setMode(int slot, int mode) {
    if (slot < 0 || slot >= Sampler::kSlots) return;
    m_s.mode[slot].store(std::clamp(mode, 0, 2));
    QSettings().setValue(QStringLiteral("sampler/%1/mode").arg(slot), mode);
    emit slotsChanged();
}

bool SamplerController::loadFile(int slot, const QString& pathOrUrl) {
    if (slot < 0 || slot >= Sampler::kSlots) return false;
    const QString path = pathOrUrl.startsWith(QLatin1String("file:")) ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    auto s = std::make_shared<Sample>();
    QString err;
    if (!decodeToStereo(path, m_engine->sampleRate(), s->pcm, &err)) {
        m_error = QFileInfo(path).fileName() + ": " + err;
        emit slotsChanged();
        return false;
    }
    s->frames = static_cast<int64_t>(s->pcm.size() / 2);
    s->name = QFileInfo(path).completeBaseName().left(14).toUpper();
    install(slot, s);
    m_paths[slot] = path;
    QSettings().setValue(QStringLiteral("sampler/%1/path").arg(slot), path);
    m_error.clear();
    emit slotsChanged();
    return true;
}

void SamplerController::resetSlot(int slot) {
    if (slot < 0 || slot >= Sampler::kSlots) return;
    install(slot, std::shared_ptr<Sample>(makeBuiltinSample(slot % builtinSampleCount(), m_engine->sampleRate()).release()));
    m_paths[slot].clear();
    QSettings().remove(QStringLiteral("sampler/%1/path").arg(slot));
    emit slotsChanged();
}

void SamplerController::tick() {
    bool changed = false;
    for (int i = 0; i < Sampler::kSlots; ++i) {
        const bool p = m_s.isPlaying(i);
        const double pr = m_s.progress(i);
        if (m_playing[i].toBool() != p) { m_playing[i] = p; changed = true; }
        if (p && std::abs(m_progress[i].toDouble() - pr) > 0.005) { m_progress[i] = pr; changed = true; }
    }
    if (changed) emit playingChanged();
}

} // namespace jx
