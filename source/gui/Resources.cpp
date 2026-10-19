#include "source/gui/Resources.hpp"
#include "source/gui/ChartSeries.hpp"
#include "source/gui/FanChartItem.hpp"
#include "source/gui/ResultTableModel.hpp"
#include "source/gui/RunModel.hpp"
#include "source/gui/ScenarioModel.hpp"
#include "source/gui/UnderlyingsModel.hpp"
#include "source/gui/Workspace.hpp"
#include <QDir>
#include <QtQml>

static void InitialiseEmbeddedFiles()
{
    Q_INIT_RESOURCE(fin_gui_qml);
}

namespace fin::gui
{
    void Initialise()
    {
        static const bool once = []
        {
            InitialiseEmbeddedFiles();

            qmlRegisterType<FanChartItem>("Fin", 1, 0, "FanChart");
            qmlRegisterUncreatableType<ChartSeries>("Fin", 1, 0, "ChartSeries", "owned by the run");
            qmlRegisterUncreatableType<ResultTableModel>("Fin", 1, 0, "ResultTableModel", "owned by the run");
            qmlRegisterUncreatableType<RunModel>("Fin", 1, 0, "RunModel", "owned by the workspace");
            qmlRegisterUncreatableType<ScenarioModel>("Fin", 1, 0, "ScenarioModel", "owned by the workspace");
            qmlRegisterUncreatableType<UnderlyingsModel>("Fin", 1, 0, "UnderlyingsModel", "owned by the scenario");
            qmlRegisterUncreatableType<Workspace>("Fin", 1, 0, "Workspace", "created by the application");

            return true;
        }();

        static_cast<void>(once);
    }
}
