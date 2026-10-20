#include "source/gui/RunModel.hpp"
#include "source/io/BrokerConfig.hpp"
#include "source/io/PriceCsvLoader.hpp"
#include "source/io/ReportWriter.hpp"
#include "source/io/ScenarioLoader.hpp"
#include "source/io/TaxConfig.hpp"
#include "source/present/FanChartSeries.hpp"
#include "source/present/ResultTable.hpp"
#include <QFile>
#include <QLocale>
#include <QTextStream>
#include <QTimer>
#include <QVariantMap>
#include <algorithm>
#include <cstdint>
#include <fstream>

namespace fin::gui
{
    namespace
    {
        constexpr int pollInterval = 40;
        constexpr std::int32_t searchDrawdownBasisPoints = 4000;
        constexpr std::int32_t searchAssignmentBasisPoints = 5000;
        constexpr std::size_t searchIterations = 8;
        constexpr int mostHighlights = 8;

        std::string Contents(const QString& path)
        {
            QFile file(path);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return {};

            return QTextStream(&file).readAll().toStdString();
        }

        std::vector<std::string> Lines(const QString& path)
        {
            QFile file(path);
            std::vector<std::string> lines;

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return lines;

            QTextStream in(&file);

            while (!in.atEnd())
                lines.push_back(in.readLine().toStdString());

            return lines;
        }

        bool Wrote(const QString& path, const std::string& contents)
        {
            std::ofstream file(path.toStdString());
            file << contents;

            return file.good();
        }

        QString Amount(const core::Money& money)
        {
            return QLocale(QLocale::English).toString(static_cast<double>(money.Cents()) / 100.0, 'f', 2);
        }

        QVariantMap Highlight(const QString& label, const QString& value)
        {
            return QVariantMap{ { "label", label }, { "value", value } };
        }
    }

    RunModel::RunModel(ScenarioModel& scenario, QString configurations, QObject* parent)
        : QObject(parent)
        , scenario(scenario)
        , configurations(std::move(configurations))
        , poll(new QTimer(this))
        , cycles(new ResultTableModel(this))
        , ledger(new ResultTableModel(this))
        , summary(new ResultTableModel(this))
        , calibration(new ResultTableModel(this))
        , chart(new ChartSeries(this))
        , status(tr("Ready"))
    {
        poll->setInterval(pollInterval);

        connect(poll, &QTimer::timeout, this, &RunModel::Poll);
        connect(&scenario, &ScenarioModel::changed, this, &RunModel::changed);

        Clear();
    }

    RunModel::~RunModel()
    {
        Stop();
        Await();
    }

    bool RunModel::Running() const
    {
        return worker != nullptr;
    }

    bool RunModel::Runnable() const
    {
        return !Running() && scenario.WellFormed();
    }

    bool RunModel::HasResult() const
    {
        return !Running() && controller.Result().has_value();
    }

    QString RunModel::State() const
    {
        switch (controller.State())
        {
            case present::RunState::idle:
                return QStringLiteral("idle");
            case present::RunState::running:
                return QStringLiteral("running");
            case present::RunState::completed:
                return QStringLiteral("completed");
            case present::RunState::cancelled:
                return QStringLiteral("cancelled");
            case present::RunState::failed:
            default:
                return QStringLiteral("failed");
        }
    }

    QString RunModel::Kind() const
    {
        switch (kind)
        {
            case present::RunKind::projection:
                return QStringLiteral("projection");
            case present::RunKind::simulation:
                return QStringLiteral("simulation");
            case present::RunKind::backtest:
                return QStringLiteral("backtest");
            case present::RunKind::search:
            default:
                return QStringLiteral("search");
        }
    }

    QString RunModel::Status() const
    {
        return status;
    }

    int RunModel::Completed() const
    {
        return controller.Progress().completed;
    }

    int RunModel::Total() const
    {
        return controller.Progress().total;
    }

    QVariantList RunModel::Highlights() const
    {
        return highlights;
    }

    ResultTableModel* RunModel::Cycles()
    {
        return cycles;
    }

    ResultTableModel* RunModel::Ledger()
    {
        return ledger;
    }

    ResultTableModel* RunModel::Summary()
    {
        return summary;
    }

    ResultTableModel* RunModel::Calibration()
    {
        return calibration;
    }

    ChartSeries* RunModel::Chart()
    {
        return chart;
    }

    void RunModel::project()
    {
        Start(present::RunKind::projection);
    }

    void RunModel::simulate()
    {
        Start(present::RunKind::simulation);
    }

    void RunModel::search()
    {
        Start(present::RunKind::search);
    }

    void RunModel::backtest()
    {
        Start(present::RunKind::backtest);
    }

    void RunModel::cancel()
    {
        Stop();
    }

    const present::RunController& RunModel::Controller() const
    {
        return controller;
    }

    QString RunModel::Complaint() const
    {
        return complaint;
    }

    std::optional<present::RunInputs> RunModel::Gathered()
    {
        if (!scenario.WellFormed())
            return std::nullopt;

        const auto prepared = scenario.Document().Prepared();
        const io::TaxConfig taxes({ Contents(configurations + "/" + QString::fromStdString(prepared.taxConfiguration)) });

        if (!taxes.Parameters().has_value())
        {
            complaint = QString::fromStdString(taxes.Failure());
            return std::nullopt;
        }

        const io::BrokerConfig fees({ Contents(configurations + "/" + QString::fromStdString(prepared.brokerConfiguration)) });

        if (!fees.Schedule().has_value())
        {
            complaint = QString::fromStdString(fees.Failure());
            return std::nullopt;
        }

        std::vector<present::NamedHistory> histories;

        if (prepared.calibration.has_value())
            for (const auto& underlying : prepared.terms.underlyings)
            {
                const auto path = configurations + "/" + QString::fromStdString(prepared.calibration->priceHistories) + "/" + QString::fromStdString(underlying.underlying) + ".csv";

                if (!QFile::exists(path))
                {
                    complaint = tr("No price history for %1 at %2").arg(QString::fromStdString(underlying.underlying), path);
                    return std::nullopt;
                }

                const io::PriceCsvLoader loader(Lines(path), io::CurrencyOf(prepared), core::DayCountConvention::Act252);

                if (!loader.Series().has_value())
                {
                    complaint = QString::fromStdString(underlying.underlying + ": " + loader.Failure());
                    return std::nullopt;
                }

                histories.push_back(present::NamedHistory{ underlying.underlying, *loader.Series() });
            }

        complaint.clear();

        return present::RunInputs{ .scenario = prepared,
            .fees = *fees.Schedule(),
            .taxes = *taxes.Parameters(),
            .search = present::SearchAround(prepared, searchDrawdownBasisPoints, searchAssignmentBasisPoints, searchIterations),
            .histories = std::move(histories) };
    }

    void RunModel::Start(present::RunKind started)
    {
        if (Running())
            return;

        const auto inputs = Gathered();

        if (!inputs.has_value())
        {
            status = complaint.isEmpty() ? tr("The scenario cannot be run as it stands") : complaint;
            emit changed();
            return;
        }

        kind = started;
        Clear();
        status = tr("Running");

        worker = std::make_unique<std::thread>([this, held = *inputs, started]
            {
                controller.Start(held, started);
            });

        poll->start();

        emit changed();
    }

    void RunModel::Stop()
    {
        if (Running())
            controller.Cancel();
    }

    void RunModel::Poll()
    {
        if (controller.State() != present::RunState::running)
            Finish();
        else
            emit changed();
    }

    void RunModel::Await()
    {
        if (Running())
            Finish();
    }

    void RunModel::Finish()
    {
        poll->stop();

        if (worker != nullptr)
        {
            worker->join();
            worker.reset();
        }

        if (controller.Result().has_value())
            Present(*controller.Result());
        else
            Clear();

        if (controller.State() == present::RunState::failed)
            status = QString::fromStdString(controller.Failure());
        else if (controller.State() == present::RunState::cancelled)
            status = tr("Cancelled, showing what had been run");
        else
            status = tr("Done");

        emit changed();
        emit finished();
    }

    void RunModel::Clear()
    {
        cycles->Present(present::CycleTable({}));
        ledger->Present(present::ResultTable{ { "date", "kind", "amount" }, {} });
        summary->Present(present::ResultTable{ { "measure", "value" }, {} });
        calibration->Present(present::CalibrationTable({}));
        chart->Present(present::TraceOf({}, {}));
        highlights.clear();
    }

    void RunModel::Present(const present::RunOutcome& outcome)
    {
        cycles->Present(present::CycleTable(outcome.rows));
        ledger->Present(present::LedgerTable(outcome.ledger));
        calibration->Present(present::CalibrationTable(outcome.calibration));
        highlights.clear();

        if (outcome.distribution.has_value())
        {
            summary->Present(present::OutcomeTable(*outcome.distribution));
            chart->Present(present::FanOf(*outcome.distribution, outcome.boundaries));
        }
        else if (outcome.recommendation.has_value())
        {
            summary->Present(present::RecommendationTable(*outcome.recommendation));
            chart->Present(present::TraceOf({}, {}));
        }
        else
        {
            summary->Present(present::ResultTable{ { "measure", "value" }, {} });
            chart->Present(present::TraceOf(outcome.equity, outcome.boundaries));

            if (!outcome.equity.empty())
            {
                highlights << Highlight(tr("Closing equity"), Amount(outcome.equity.back()));
                highlights << Highlight(tr("Lowest equity"), Amount(*std::min_element(outcome.equity.begin(), outcome.equity.end(), [](const auto& a, const auto& b)
                                                                 {
                                                                     return a.Cents() < b.Cents();
                                                                 })));
                highlights << Highlight(tr("Cycle rows"), QString::number(outcome.rows.size()));
            }
        }

        const auto& shown = summary->Shown();

        for (std::size_t row = 0; row != shown.rows.size() && highlights.size() < mostHighlights; ++row)
            if (shown.rows[row].size() >= 2)
                highlights << Highlight(QString::fromStdString(shown.rows[row][0]), QString::fromStdString(shown.rows[row][1]));
    }

    bool RunModel::Export(const QString& path) const
    {
        if (Running() || !controller.Result().has_value())
            return false;

        return Wrote(path, io::CycleReport(controller.Result()->rows) + "\n" + io::LedgerReport(controller.Result()->ledger));
    }
}
