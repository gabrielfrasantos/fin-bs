#pragma once

#include "source/gui/RunModel.hpp"
#include "source/gui/ScenarioModel.hpp"
#include <QObject>
#include <QString>
#include <QUrl>

namespace fin::gui
{
    // What the window works on: the scenario, the run, where the configuration files live, and
    // the last thing that went wrong at a file boundary.
    class Workspace
        : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(fin::gui::ScenarioModel* scenario READ Scenario CONSTANT)
        Q_PROPERTY(fin::gui::RunModel* run READ Run CONSTANT)
        Q_PROPERTY(QString configurations READ Configurations CONSTANT)
        Q_PROPERTY(QString scenarioPath READ ScenarioPath NOTIFY changed)
        Q_PROPERTY(QString message READ Message NOTIFY changed)

    public:
        explicit Workspace(QString configurations, QObject* parent = nullptr);

        ScenarioModel* Scenario();
        RunModel* Run();
        QString Configurations() const;
        QString ScenarioPath() const;
        QString Message() const;

        bool Open(const QString& path);
        bool Save(const QString& path);
        bool Export(const QString& path);

        Q_INVOKABLE bool open(const QUrl& file);
        Q_INVOKABLE bool save(const QUrl& file);
        Q_INVOKABLE bool exportResults(const QUrl& file);
        Q_INVOKABLE bool estimate(int index, const QUrl& file);
        Q_INVOKABLE QUrl scenariosFolder() const;

    signals:
        void changed();

    private:
        QString configurations;
        ScenarioModel* scenario;
        RunModel* run;
        QString scenarioPath;
        QString message;
    };
}
