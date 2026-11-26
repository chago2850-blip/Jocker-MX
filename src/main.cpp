// SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/DeckController.h"
#include "app/LibraryModel.h"
#include "app/Mixer.h"
#include "app/RecordingController.h"
#include "app/AutomixController.h"
#include "app/EventScheduler.h"
#include "app/KaraokeController.h"
#include "app/SamplerController.h"
#include "control/MidiManager.h"
#include "engine/AudioEngine.h"
#include "library/LibraryDb.h"

#include <QCommandLineParser>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif

#ifndef JOCKER_VERSION
#define JOCKER_VERSION "0.11.0-dev"
#endif

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("Jocker-MX"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("jocker-mx.org"));
    QGuiApplication::setApplicationName(QStringLiteral("Jocker-MX"));
    QGuiApplication::setApplicationVersion(QStringLiteral(JOCKER_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/JockerMX/resources/icon-256.png")));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser cli;
    cli.setApplicationDescription(QStringLiteral("Jocker-MX — software DJ libre (GPL-3.0-or-later)"));
    cli.addHelpOption();
    cli.addVersionOption();
    QCommandLineOption optA({"a", "load-a"}, "Cargar archivo en el deck A", "archivo");
    QCommandLineOption optB({"b", "load-b"}, "Cargar archivo en el deck B", "archivo");
    QCommandLineOption optPlay("play", "Reproducir los decks cargados al iniciar");
    QCommandLineOption optShot("screenshot", "Guardar captura de la ventana y salir", "png");
    QCommandLineOption optWait("wait", "Milisegundos antes de la captura", "ms", "2500");
    QCommandLineOption optLib("library", "Carpeta de música para el navegador", "carpeta");
    QCommandLineOption optPanel("panel", "Abrir un panel al iniciar (midi, sampler, automix, programacion, karaoke, video)", "nombre");
    QCommandLineOption optQueue("queue", "Agregar un archivo a la cola de Automix (se puede repetir)", "archivo");
    QCommandLineOption optAutomix("automix", "Encender el Automix al iniciar");
    cli.addOptions({optA, optB, optPlay, optShot, optWait, optLib, optPanel, optQueue, optAutomix});
    cli.process(app);

    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/JockerMX/resources/fonts/Montserrat.ttf"));
    if (fontId >= 0) {
        QFont f(QFontDatabase::applicationFontFamilies(fontId).value(0));
        f.setPixelSize(13);
        f.setWeight(QFont::Medium);
        QGuiApplication::setFont(f);
    }

    if (!jx::LibraryDb::open()) qWarning("No se pudo abrir la base de datos de la biblioteca.");

    jx::AudioEngine engine(2);
    if (!engine.start()) qWarning("Sin dispositivo de audio: la interfaz funcionará en silencio.");

    jx::DeckController deckA(&engine, 0), deckB(&engine, 1);
    deckA.setPartner(&deckB);
    deckB.setPartner(&deckA);
    jx::Mixer mixer(&engine, {&deckA, &deckB});
    jx::LibraryModel library;
    jx::SamplerController sampler(&engine);
    jx::RecordingController recorder(&engine, {&deckA, &deckB});
    jx::MidiManager midi({&deckA, &deckB}, &mixer, &sampler, &recorder);
    jx::AutomixController automix(&deckA, &deckB, &mixer);
    midi.setAutomix(&automix);
    jx::EventScheduler scheduler(&automix, &deckA, &deckB, &mixer, &recorder);
    jx::KaraokeController karaoke(&deckA, &deckB, &mixer);
    if (cli.isSet(optQueue)) { automix.clear(); automix.add(cli.values(optQueue)); }
    if (cli.isSet(optLib)) library.setFolder(cli.value(optLib));
    QObject::connect(&deckA, &jx::DeckController::trackLoaded, &library, &jx::LibraryModel::setAnalysis);
    QObject::connect(&deckB, &jx::DeckController::trackLoaded, &library, &jx::LibraryModel::setAnalysis);
    QObject::connect(&deckA, &jx::DeckController::played, &library, &jx::LibraryModel::onPlayed);
    QObject::connect(&deckB, &jx::DeckController::played, &library, &jx::LibraryModel::onPlayed);

#ifdef Q_OS_ANDROID
    // Permiso para leer la música del teléfono/tableta (Android 13+: READ_MEDIA_AUDIO; antes: almacenamiento)
    {
        bool granted = false;
        for (const QString perm : {QStringLiteral("android.permission.READ_MEDIA_AUDIO"),
                                   QStringLiteral("android.permission.READ_EXTERNAL_STORAGE")}) {
            if (QtAndroidPrivate::checkPermission(perm).result() == QtAndroidPrivate::Authorized) { granted = true; break; }
            if (QtAndroidPrivate::requestPermission(perm).result() == QtAndroidPrivate::Authorized) { granted = true; break; }
        }
        if (granted) library.rescan();
    }
#endif

    QQmlApplicationEngine qml;
    qml.rootContext()->setContextProperty(QStringLiteral("deckA"), &deckA);
    qml.rootContext()->setContextProperty(QStringLiteral("deckB"), &deckB);
    qml.rootContext()->setContextProperty(QStringLiteral("mixer"), &mixer);
    qml.rootContext()->setContextProperty(QStringLiteral("library"), &library);
    qml.rootContext()->setContextProperty(QStringLiteral("midi"), &midi);
    qml.rootContext()->setContextProperty(QStringLiteral("sampler"), &sampler);
    qml.rootContext()->setContextProperty(QStringLiteral("recorder"), &recorder);
    qml.rootContext()->setContextProperty(QStringLiteral("automix"), &automix);
    qml.rootContext()->setContextProperty(QStringLiteral("scheduler"), &scheduler);
    qml.rootContext()->setContextProperty(QStringLiteral("karaoke"), &karaoke);
    qml.rootContext()->setContextProperty(QStringLiteral("startupPanel"), cli.value(optPanel));
    qml.rootContext()->setContextProperty(QStringLiteral("appVersion"), QStringLiteral(JOCKER_VERSION));
    QObject::connect(&qml, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    // El módulo JockerMX (qmldir con el singleton Theme) vive en qrc:/JockerMX;
    // sin esta ruta solo se encontraba al ejecutar desde el directorio de compilación.
    qml.addImportPath(QStringLiteral("qrc:/"));
    qml.load(QUrl(QStringLiteral("qrc:/JockerMX/qml/Main.qml")));
    if (qml.rootObjects().isEmpty()) return 1;

    if (cli.isSet(optA)) deckA.load(cli.value(optA));
    if (cli.isSet(optB)) deckB.load(cli.value(optB));
    if (cli.isSet(optAutomix)) automix.setEnabled(true);
    if (cli.isSet(optPlay)) {
        auto startWhenReady = [](jx::DeckController* d) {
            QObject::connect(d, &jx::DeckController::trackChanged, d, [d]() {
                if (d->loaded() && !d->playing()) d->playPause();
            });
        };
        startWhenReady(&deckA);
        startWhenReady(&deckB);
    }
    if (cli.isSet(optShot)) {
        const QString file = cli.value(optShot);
        QTimer::singleShot(cli.value(optWait).toInt(), &app, [&qml, file]() {
            auto* win = qobject_cast<QQuickWindow*>(qml.rootObjects().first());
            if (win) win->grabWindow().save(file);
            QCoreApplication::quit();
        });
    }
    const int rc = app.exec();
    recorder.stop();   // cierra el WAV y escribe la lista de canciones
    engine.stop();
    return rc;
}
