#include "source/gui/MainWindow.hpp"
#include <QApplication>
#include <QString>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    const QString configurations = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString(FIN_BS_CONFIG_DIRECTORY);

    fin::gui::MainWindow window(configurations);
    window.Open(configurations + "/scenarios/example.json");
    window.resize(1440, 900);
    window.show();

    return QApplication::exec();
}
