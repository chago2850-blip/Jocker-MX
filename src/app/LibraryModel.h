// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QtQml/qqmlregistration.h>

namespace jx {

// Navegador de música: escanea una carpeta (recursiva) y filtra por texto.
// Versión inicial; en la fase F2 se sustituye por la base de datos SQLite.
class LibraryModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int count READ rowCountQml NOTIFY countsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    enum Roles { PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, BpmRole, FormatRole, FileNameRole };

    struct Entry {
        QString path, title, artist, format, search;
        double bpm = 0;
    };

    explicit LibraryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString folder() const { return m_folder; }
    void setFolder(const QString& f);
    QString filter() const { return m_filter; }
    void setFilter(const QString& f);
    int totalCount() const { return static_cast<int>(m_all.size()); }
    int rowCountQml() const { return static_cast<int>(m_view.size()); }
    bool scanning() const { return m_scanning; }

    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void sortBy(const QString& key);

public slots:
    void setBpm(const QString& path, double bpm);

signals:
    void folderChanged();
    void filterChanged();
    void countsChanged();
    void scanningChanged();

private:
    void rebuildView();
    QString m_folder, m_filter;
    QList<Entry> m_all;
    QList<int> m_view;
    bool m_scanning = false;
    QString m_sortKey = QStringLiteral("artist");
    QFutureWatcher<QList<Entry>> m_watcher;
};

} // namespace jx
