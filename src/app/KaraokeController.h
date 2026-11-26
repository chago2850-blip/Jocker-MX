// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace jx {

class DeckController;
class Mixer;

// Rotación de cantantes: cada turno tiene nombre, canción y tono (semitonos).
// «Siguiente cantante» carga la canción en el deck libre desde el principio y aplica su tono.
class KaraokeController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Lo crea la aplicación")
    Q_PROPERTY(QVariantList singers READ singers NOTIFY singersChanged)   // [{name, path, title, artist, key, karaoke}]
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentChanged)
    Q_PROPERTY(int karaokeDeck READ karaokeDeck NOTIFY currentChanged)   // -1 ninguno, 0 A, 1 B
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY settingsChanged)
    Q_PROPERTY(int sungCount READ sungCount NOTIFY currentChanged)

public:
    KaraokeController(DeckController* a, DeckController* b, Mixer* mixer, QObject* parent = nullptr);

    QVariantList singers() const;
    QString currentName() const { return m_currentName; }
    QString currentTitle() const { return m_currentTitle; }
    int karaokeDeck() const { return m_deck; }
    bool autoPlay() const { return m_autoPlay; }
    void setAutoPlay(bool on);
    int sungCount() const { return m_sung; }
    int count() const { return int(m_list.size()); }

    Q_INVOKABLE void addSinger(const QString& name, const QString& path, int key = 0);
    Q_INVOKABLE void removeSinger(int i);
    Q_INVOKABLE void moveSinger(int from, int to);
    Q_INVOKABLE void setSingerKey(int i, int key);
    Q_INVOKABLE void setSingerName(int i, const QString& name);
    Q_INVOKABLE bool startNext();          // carga el siguiente turno; false si no hay
    Q_INVOKABLE void clear();
    Q_INVOKABLE static bool hasKaraokeFiles(const QString& audioPath);   // ¿tiene .cdg o .lrc al lado?

signals:
    void singersChanged();
    void currentChanged();
    void settingsChanged();

private:
    struct Singer { QString name, path; int key = 0; };
    void save();
    DeckController* deck(int i) const;

    DeckController* m_a;
    DeckController* m_b;
    Mixer* m_mixer;
    QList<Singer> m_list;
    QString m_currentName, m_currentTitle;
    int m_deck = -1;
    bool m_autoPlay = false;
    int m_sung = 0;
    QMetaObject::Connection m_pending;
};

} // namespace jx
