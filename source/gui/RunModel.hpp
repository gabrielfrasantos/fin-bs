#pragma once

#include "source/gui/ChartSeries.hpp"
#include "source/gui/ResultTableModel.hpp"
#include "source/gui/ScenarioModel.hpp"
#include "source/present/RunController.hpp"
#include <QObject>
#include <QString>
#include <QVariantList>
#include <memory>
#include <optional>
#include <thread>

class QTimer;

namespace fin::gui
{
    // Starts a projection, a simulation or a search on a worker thread, reports its progress to the
    // window at a steady beat, and presents the outcome as tables, a chart and a few figures.
    class RunModel
        : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool running READ Running NOTIFY changed)
        Q_PROPERTY(bool runnable READ Runnable NOTIFY changed)
        Q_PROPERTY(bool hasResult READ HasResult NOTIFY changed)
        Q_PROPERTY(QString state READ State NOTIFY changed)
        Q_PROPERTY(QString kind READ Kind NOTIFY changed)
        Q_PROPERTY(QString status READ Status NOTIFY changed)
        Q_PROPERTY(int completed READ Completed NOTIFY changed)
        Q_PROPERTY(int total READ Total NOTIFY changed)
        Q_PROPERTY(QVariantList highlights READ Highlights NOTIFY changed)
        Q_PROPERTY(fin::gui::ResultTableModel* cycles READ Cycles CONSTANT)
        Q_PROPERTY(fin::gui::ResultTableModel* ledger READ Ledger CONSTANT)
        Q_PROPERTY(fin::gui::ResultTableModel* summary READ Summary CONSTANT)
        Q_PROPERTY(fin::gui::ChartSeries* chart READ Chart CONSTANT)

    public:
        RunModel(ScenarioModel& scenario, QString configurations, QObject* parent = nullptr);
        ~RunModel() override;

        bool Running() const;
        bool Runnable() const;
        bool HasResult() const;
        QString State() const;
        QString Kind() const;
        QString Status() const;
        int Completed() const;
        int Total() const;
        QVariantList Highlights() const;
        ResultTableModel* Cycles();
        ResultTableModel* Ledger();
        ResultTableModel* Summary();
        ChartSeries* Chart();

        Q_INVOKABLE void project();
        Q_INVOKABLE void simulate();
        Q_INVOKABLE void search();
        Q_INVOKABLE void cancel();

        void Start(present::RunKind kind);
        void Stop();
        void Await();
        bool Export(const QString& path) const;

        const present::RunController& Controller() const;
        QString Complaint() const;

    signals:
        void changed();
        void finished();

    private:
        void Poll();
        void Finish();
        void Clear();
        void Present(const present::RunOutcome& outcome);
        std::optional<present::RunInputs> Gathered();

        ScenarioModel& scenario;
        QString configurations;
        present::RunController controller;
        std::unique_ptr<std::thread> worker;
        QTimer* poll;
        ResultTableModel* cycles;
        ResultTableModel* ledger;
        ResultTableModel* summary;
        ChartSeries* chart;
        QVariantList highlights;
        QString complaint;
        QString status;
        present::RunKind kind{ present::RunKind::projection };
    };
}
