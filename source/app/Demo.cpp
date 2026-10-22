#include "source/app/Demo.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <algorithm>

namespace fin::app
{
    namespace
    {
        constexpr int settle = 250;
        constexpr int cyclesTab = 2;
        constexpr int ledgerTab = 3;
        constexpr int estimatesTab = 4;
        constexpr int fanTab = 1;
        constexpr int summaryTab = 0;
        constexpr int demoPaths = 400;
    }

    Demo::Demo(QQmlApplicationEngine& engine, gui::Workspace& workspace, QString folder)
        : engine(engine)
        , workspace(workspace)
        , folder(std::move(folder))
    {
        QDir().mkpath(this->folder);

        const auto scenario = qEnvironmentVariableIsSet("FIN_BS_DEMO_SCENARIO") ? qEnvironmentVariable("FIN_BS_DEMO_SCENARIO") : workspace.Configurations() + "/scenarios/benelux.json";
        const auto backtest = qEnvironmentVariableIsSet("FIN_BS_DEMO_BACKTEST") ? qEnvironmentVariable("FIN_BS_DEMO_BACKTEST") : workspace.Configurations() + "/scenarios/benelux-backtest.json";

        steps = {
            { "scenario", [this, scenario]
                {
                    this->workspace.Open(scenario);
                },
                false },
            { "projection-running", [this]
                {
                    this->workspace.Run()->project();
                },
                true },
            { "projection-fan", [this]
                {
                    ShowTab(fanTab);
                },
                false },
            { "projection-cycles", [this]
                {
                    ShowTab(cyclesTab);
                },
                false },
            { "projection-ledger", [this]
                {
                    ShowTab(ledgerTab);
                },
                false },
            { "simulation-done", [this]
                {
                    this->workspace.Scenario()->SetPaths(demoPaths);
                    ShowTab(summaryTab);
                    this->workspace.Run()->simulate();
                },
                true },
            { "simulation-fan", [this]
                {
                    ShowTab(fanTab);
                },
                false },
            { "backtest-done", [this, backtest]
                {
                    this->workspace.Open(backtest);
                    ShowTab(cyclesTab);
                    this->workspace.Run()->backtest();
                },
                true },
            { "backtest-estimates", [this]
                {
                    ShowTab(estimatesTab);
                },
                false },
            { "underlyings", [this, scenario]
                {
                    this->workspace.Open(scenario);
                    Scroll(1.0);
                },
                false },
            { "validation", [this]
                {
                    Scroll(0.0);
                    this->workspace.Scenario()->SetTargetDeltaBasisPoints(15000);
                },
                false },
            { "dark-theme", [this]
                {
                    this->workspace.Scenario()->SetTargetDeltaBasisPoints(3000);
                    if (auto* window = Window())
                        window->setProperty("dark", true);
                },
                false },
        };

        QTimer::singleShot(settle * 2, this, &Demo::Next);
    }

    QQuickWindow* Demo::Window() const
    {
        return engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    }

    void Demo::Scroll(double fraction)
    {
        if (auto* window = Window())
            if (auto* pane = window->findChild<QObject*>("scenarioPane"))
                if (auto* flickable = pane->property("contentItem").value<QObject*>())
                {
                    const auto contentHeight = flickable->property("contentHeight").toDouble();
                    const auto height = flickable->property("height").toDouble();
                    flickable->setProperty("contentY", std::max(0.0, (contentHeight - height) * fraction));
                }
    }

    void Demo::ShowTab(int index)
    {
        if (auto* window = Window())
            if (auto* tabs = window->findChild<QObject*>("resultsTabs"))
                tabs->setProperty("currentIndex", index);
    }

    void Demo::Capture(const QString& name)
    {
        if (auto* window = Window())
        {
            const QImage image = window->grabWindow();
            image.save(QString("%1/%2-%3.png").arg(folder).arg(++taken, 2, 10, QChar('0')).arg(name));
        }
    }

    void Demo::Next()
    {
        if (at >= steps.size())
        {
            QCoreApplication::quit();
            return;
        }

        const auto& step = steps[at++];
        step.act();

        if (!step.awaitsRun)
        {
            QTimer::singleShot(settle, this, [this, name = step.name]
                {
                    Capture(name);
                    Next();
                });
            return;
        }

        QTimer::singleShot(settle, this, [this, name = step.name]
            {
                Capture(name + "-underway");
            });

        auto* done = new QObject(this);

        connect(workspace.Run(), &gui::RunModel::finished, done, [this, done, name = step.name]
            {
                done->deleteLater();
                QTimer::singleShot(settle, this, [this, name]
                    {
                        Capture(name);
                        Next();
                    });
            });
    }
}
