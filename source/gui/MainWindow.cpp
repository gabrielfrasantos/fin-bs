#include "source/gui/MainWindow.hpp"
#include "source/gui/ResultsPanel.hpp"
#include "source/gui/ScenarioPanel.hpp"
#include "source/io/BrokerConfig.hpp"
#include "source/io/ReportWriter.hpp"
#include "source/io/ScenarioLoader.hpp"
#include "source/io/TaxConfig.hpp"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace fin::gui
{
    namespace
    {
        constexpr int pollInterval = 40;
        constexpr std::int32_t searchDrawdownBasisPoints = 4000;
        constexpr std::int32_t searchAssignmentBasisPoints = 5000;
        constexpr std::size_t searchIterations = 8;

        QPushButton* Button(const char* named, const QString& caption)
        {
            auto* button = new QPushButton(caption);
            button->setObjectName(named);

            return button;
        }

        bool Wrote(const QString& path, const std::string& contents)
        {
            std::ofstream file(path.toStdString());
            file << contents;

            return file.good();
        }

        std::string Contents(const QString& path)
        {
            std::ifstream file(path.toStdString());
            std::ostringstream contents;
            contents << file.rdbuf();

            return contents.str();
        }
    }

    MainWindow::MainWindow(QString configurations, QWidget* parent)
        : QMainWindow(parent)
        , configurations(std::move(configurations))
    {
        setObjectName("fin.bs");
        setWindowTitle(tr("Options income and compounding estimator"));

        scenario = new ScenarioPanel;
        results = new ResultsPanel;

        auto* scrolled = new QScrollArea;
        scrolled->setWidget(scenario);
        scrolled->setWidgetResizable(true);

        auto* right = new QWidget;
        auto* stacked = new QVBoxLayout(right);
        auto* buttons = new QHBoxLayout;

        project = Button("project", tr("Project"));
        simulate = Button("simulate", tr("Simulate"));
        search = Button("search", tr("Search"));
        cancel = Button("cancel", tr("Cancel"));

        progress = new QProgressBar;
        progress->setObjectName("progress");
        progress->setRange(0, 1);
        progress->setValue(0);

        status = new QLabel;
        status->setObjectName("status");
        status->setWordWrap(true);

        buttons->addWidget(project);
        buttons->addWidget(simulate);
        buttons->addWidget(search);
        buttons->addWidget(cancel);
        buttons->addWidget(progress);

        stacked->addLayout(buttons);
        stacked->addWidget(status);
        stacked->addWidget(results);

        auto* split = new QSplitter;
        split->addWidget(scrolled);
        split->addWidget(right);
        split->setStretchFactor(1, 1);

        setCentralWidget(split);

        poll = new QTimer(this);
        poll->setInterval(pollInterval);

        BuildActions();

        connect(poll, &QTimer::timeout, this, &MainWindow::Poll);
        connect(scenario, &ScenarioPanel::Edited, this, &MainWindow::Judge);
        connect(project, &QPushButton::clicked, this, [this]
            {
                Start(present::RunKind::projection);
            });
        connect(simulate, &QPushButton::clicked, this, [this]
            {
                Start(present::RunKind::simulation);
            });
        connect(search, &QPushButton::clicked, this, [this]
            {
                Start(present::RunKind::search);
            });
        connect(cancel, &QPushButton::clicked, this, &MainWindow::Stop);

        cancel->setEnabled(false);
        Judge();
    }

    MainWindow::~MainWindow()
    {
        Stop();
        Await();
    }

    void MainWindow::BuildActions()
    {
        auto* file = menuBar()->addMenu(tr("&Scenario"));

        connect(file->addAction(tr("&Open...")), &QAction::triggered, this, [this]
            {
                const auto path = QFileDialog::getOpenFileName(this, tr("Open a scenario"), configurations, tr("Scenarios (*.json)"));

                if (!path.isEmpty())
                    Open(path);
            });

        connect(file->addAction(tr("&Save as...")), &QAction::triggered, this, [this]
            {
                const auto path = QFileDialog::getSaveFileName(this, tr("Save the scenario"), configurations, tr("Scenarios (*.json)"));

                if (!path.isEmpty())
                    Save(path);
            });

        connect(file->addAction(tr("&Export results...")), &QAction::triggered, this, [this]
            {
                const auto path = QFileDialog::getSaveFileName(this, tr("Export the results"), QString(), tr("Comma separated values (*.csv)"));

                if (!path.isEmpty())
                    Export(path);
            });

        auto* prices = menuBar()->addMenu(tr("&Prices"));

        connect(prices->addAction(tr("&Estimate the selected underlying from a price history...")), &QAction::triggered, this, [this]
            {
                const auto path = QFileDialog::getOpenFileName(this, tr("Open a price history"), QString(), tr("Comma separated values (*.csv)"));

                if (path.isEmpty() || scenario->Selected() < 0)
                    return;

                if (!scenario->Adopt(static_cast<std::size_t>(scenario->Selected()), Read(path)))
                    status->setText(scenario->Complaint());
            });
    }

    ScenarioPanel& MainWindow::Scenario()
    {
        return *scenario;
    }

    const present::RunController& MainWindow::Controller() const
    {
        return controller;
    }

    QString MainWindow::Complaint() const
    {
        return complaint;
    }

    std::vector<std::string> MainWindow::Read(const QString& path) const
    {
        std::ifstream file(path.toStdString());
        std::vector<std::string> lines;

        for (std::string line; std::getline(file, line);)
            lines.push_back(line);

        return lines;
    }

    bool MainWindow::Open(const QString& path)
    {
        const io::ScenarioLoader loader({ Contents(path) });

        if (!loader.Loaded().has_value())
        {
            complaint = QString::fromStdString(loader.Failure());
            status->setText(complaint);
            return false;
        }

        complaint.clear();
        results->Clear();
        scenario->Present(present::ScenarioDocument(*loader.Loaded()));

        return true;
    }

    bool MainWindow::Save(const QString& path) const
    {
        if (!present::IsWellFormed(scenario->Document()))
            return false;

        return Wrote(path, io::Formatted(scenario->Document().Prepared()));
    }

    bool MainWindow::Export(const QString& path) const
    {
        if (!controller.Result().has_value())
            return false;

        return Wrote(path, io::CycleReport(controller.Result()->rows) + "\n" + io::LedgerReport(controller.Result()->ledger));
    }

    void MainWindow::Judge()
    {
        const auto offending = scenario->Document().Offending();
        const auto runnable = !offending.has_value() && worker == nullptr;

        project->setEnabled(runnable);
        simulate->setEnabled(runnable);
        search->setEnabled(runnable);

        if (offending.has_value())
            status->setText(QString::fromStdString(present::Describing(*offending)));
        else if (worker == nullptr)
            status->setText(tr("Ready"));
    }

    std::optional<present::RunInputs> MainWindow::Gathered()
    {
        if (!present::IsWellFormed(scenario->Document()))
            return std::nullopt;

        const auto prepared = scenario->Document().Prepared();
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

        complaint.clear();

        return present::RunInputs{ .scenario = prepared,
            .fees = *fees.Schedule(),
            .taxes = *taxes.Parameters(),
            .search = present::SearchAround(prepared, searchDrawdownBasisPoints, searchAssignmentBasisPoints, searchIterations) };
    }

    void MainWindow::Start(present::RunKind kind)
    {
        if (worker != nullptr)
            return;

        const auto inputs = Gathered();

        if (!inputs.has_value())
        {
            status->setText(complaint.isEmpty() ? tr("The scenario cannot be run as it stands") : complaint);
            return;
        }

        results->Clear();
        progress->setRange(0, 0);
        status->setText(tr("Running"));

        project->setEnabled(false);
        simulate->setEnabled(false);
        search->setEnabled(false);
        cancel->setEnabled(true);

        worker = std::make_unique<std::thread>([this, held = *inputs, kind]
            {
                controller.Start(held, kind);
            });

        poll->start();
    }

    void MainWindow::Stop()
    {
        if (worker != nullptr)
            controller.Cancel();
    }

    void MainWindow::Poll()
    {
        const auto walked = controller.Progress();

        if (walked.total > 0)
        {
            progress->setRange(0, walked.total);
            progress->setValue(walked.completed);
        }

        if (controller.State() != present::RunState::running)
            Finish();
    }

    void MainWindow::Await()
    {
        if (worker != nullptr)
            Finish();
    }

    void MainWindow::Finish()
    {
        poll->stop();

        if (worker != nullptr)
        {
            worker->join();
            worker.reset();
        }

        const auto walked = controller.Progress();
        progress->setRange(0, std::max(1, walked.total));
        progress->setValue(walked.completed);
        cancel->setEnabled(false);

        if (controller.Result().has_value())
            results->Present(*controller.Result());
        else
            results->Clear();

        Judge();

        if (controller.State() == present::RunState::failed)
            status->setText(QString::fromStdString(controller.Failure()));
        else if (controller.State() == present::RunState::cancelled)
            status->setText(tr("Cancelled, showing what had been run"));
        else
            status->setText(tr("Done"));
    }
}
