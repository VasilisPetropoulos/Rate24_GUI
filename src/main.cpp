#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "Rate_24_Backend.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    Rate_24_Backend backend;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("Rate_24_Backend", &backend);

    const QUrl url(u"qrc:/qt/qml/App/qml/main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
        
    engine.load(url);

    return app.exec();
}