#include "source/app/Demo.hpp"
#include "source/gui/Resources.hpp"
#include "source/gui/Workspace.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QString>
#include <QUrl>
#include <memory>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);
    application.setApplicationName("fin-bs");
    application.setOrganizationName("fin-bs");

    QQuickStyle::setStyle("Material");
    fin::gui::Initialise();

    const QString configurations = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString(FIN_BS_CONFIG_DIRECTORY);

    fin::gui::Workspace workspace(configurations);
    workspace.Open(configurations + "/scenarios/example.json");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("workspace", &workspace);
    engine.load(QUrl("qrc:/fin/qml/Main.qml"));

    if (engine.rootObjects().isEmpty())
        return 1;

    std::unique_ptr<fin::app::Demo> demo;

    if (qEnvironmentVariableIsSet("FIN_BS_DEMO"))
        demo = std::make_unique<fin::app::Demo>(engine, workspace, qEnvironmentVariable("FIN_BS_DEMO"));

    return QGuiApplication::exec();
}
