#include "source/gui/Workspace.hpp"
#include "source/io/ScenarioLoader.hpp"
#include <QFile>
#include <QTextStream>
#include <fstream>

namespace fin::gui
{
    namespace
    {
        std::string Contents(const QString& path)
        {
            QFile file(path);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return {};

            return QTextStream(&file).readAll().toStdString();
        }

        bool Wrote(const QString& path, const std::string& contents)
        {
            std::ofstream file(path.toStdString());
            file << contents;

            return file.good();
        }

        QString LocalPath(const QUrl& file)
        {
            return file.isLocalFile() ? file.toLocalFile() : file.toString();
        }
    }

    Workspace::Workspace(QString configurations, QObject* parent)
        : QObject(parent)
        , configurations(std::move(configurations))
        , scenario(new ScenarioModel(this))
        , run(new RunModel(*scenario, this->configurations, this))
    {}

    ScenarioModel* Workspace::Scenario()
    {
        return scenario;
    }

    RunModel* Workspace::Run()
    {
        return run;
    }

    QString Workspace::Configurations() const
    {
        return configurations;
    }

    QString Workspace::ScenarioPath() const
    {
        return scenarioPath;
    }

    QString Workspace::Message() const
    {
        return message;
    }

    bool Workspace::Open(const QString& path)
    {
        const io::ScenarioLoader loader({ Contents(path) });

        if (!loader.Loaded().has_value())
        {
            message = QString::fromStdString(loader.Failure());
            emit changed();
            return false;
        }

        message.clear();
        scenarioPath = path;
        scenario->Present(present::ScenarioDocument(*loader.Loaded()));

        emit changed();

        return true;
    }

    bool Workspace::Save(const QString& path)
    {
        if (!present::IsWellFormed(scenario->Document()))
        {
            message = scenario->Complaint();
            emit changed();
            return false;
        }

        const auto written = Wrote(path, io::Formatted(scenario->Document().Prepared()));

        message = written ? QString() : tr("The scenario could not be written to %1").arg(path);

        if (written)
            scenarioPath = path;

        emit changed();

        return written;
    }

    bool Workspace::Export(const QString& path)
    {
        const auto written = run->Export(path);

        message = written ? QString() : tr("There is no completed run to export");
        emit changed();

        return written;
    }

    bool Workspace::open(const QUrl& file)
    {
        return Open(LocalPath(file));
    }

    bool Workspace::save(const QUrl& file)
    {
        return Save(LocalPath(file));
    }

    bool Workspace::exportResults(const QUrl& file)
    {
        return Export(LocalPath(file));
    }

    bool Workspace::estimate(int index, const QUrl& file)
    {
        const auto adopted = scenario->estimate(index, LocalPath(file));

        message = adopted ? QString() : scenario->PriceComplaint();
        emit changed();

        return adopted;
    }

    QUrl Workspace::scenariosFolder() const
    {
        return QUrl::fromLocalFile(configurations + "/scenarios");
    }
}
