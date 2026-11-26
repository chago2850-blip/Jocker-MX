// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "library/LibraryDb.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <atomic>

namespace jx {

// Navegador de música respaldado por la biblioteca SQLite.
class LibraryModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int view READ view WRITE setView NOTIFY viewChanged)   // 0 todas, 1 historial, 2 más tocadas, 3 recientes
    Q_PROPERTY(QString sortKey READ sortKey NOTIFY sortChanged)
    Q_PROPERTY(bool ascending READ ascending NOTIFY sortChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int count READ rowCountQml NOTIFY countsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int scanned READ scanned NOTIFY scanProgress)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, AlbumRole, GenreRole, KeyRole, BpmRole,
        DurationRole, FormatRole, PlaysRole, LastPlayedRole, FileNameRole
    };

    explicit LibraryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString folder() const { return m_folder; }
    void setFolder(const QString& f);
    QString filter() const { return m_filter; }
    void setFilter(const QString& f);
    int view() const { return m_view; }
    void setView(int v);
    QString sortKey() const { return m_sortKey; }
    bool ascending() const { return m_asc; }
    int totalCount() const { return m_total; }
    int rowCountQml() const { return static_cast<int>(m_rows.size()); }
    bool scanning() const { return m_scanning; }
    int scanned() const { return m_scannedCount.load(); }

    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void sortBy(const QString& key);
    Q_INVOKABLE void refresh();

public slots:
    void setBpm(const QString& path, double bpm);
    void onPlayed(const QString& path);

signals:
    void folderChanged();
    void filterChanged();
    void viewChanged();
    void sortChanged();
    void countsChanged();
    void scanningChanged();
    void scanProgress();

private:
    QString m_folder, m_filter;
    int m_view = 0;
    QString m_sortKey = QStringLiteral("artist");
    bool m_asc = true;
    QList<TrackRow> m_rows;
    int m_total = 0;
    bool m_scanning = false;
    std::atomic<int> m_scannedCount{0};
    QFutureWatcher<void> m_watcher;
    QTimer m_filterDebounce, m_progressTimer;
};

} // namespace jx
