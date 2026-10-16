#include "source/gui/ResultsPanel.hpp"
#include "source/gui/FanChartWidget.hpp"
#include "source/present/FanChartSeries.hpp"
#include "source/present/ResultTable.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace fin::gui
{
    namespace
    {
        QTableWidget* Table(const char* named)
        {
            auto* table = new QTableWidget(0, 0);
            table->setObjectName(named);
            table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            table->horizontalHeader()->setStretchLastSection(true);

            return table;
        }
    }

    ResultsPanel::ResultsPanel(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        auto* tabs = new QTabWidget;
        tabs->setObjectName("results");

        cycles = Table("cycles");
        ledger = Table("ledger");
        summary = Table("summary");
        chart = new FanChartWidget;
        chart->setObjectName("chart");

        tabs->addTab(cycles, tr("Cycles"));
        tabs->addTab(ledger, tr("Ledger"));
        tabs->addTab(summary, tr("Summary"));
        tabs->addTab(chart, tr("Fan"));

        disclaimer = new QLabel(tr("These are estimates, not forecasts. The underlyings are drawn independently of one another, so every drawdown and assignment rate shown here is optimistic; prices are drawn from a lognormal model that understates what a market does at its worst."));
        disclaimer->setObjectName("disclaimer");
        disclaimer->setWordWrap(true);

        layout->addWidget(tabs);
        layout->addWidget(disclaimer);
    }

    void ResultsPanel::Fill(QTableWidget* table, const present::ResultTable& shown)
    {
        QStringList headings;

        for (const auto& heading : shown.headings)
            headings << QString::fromStdString(heading);

        table->clear();
        table->setColumnCount(static_cast<int>(shown.headings.size()));
        table->setRowCount(static_cast<int>(shown.rows.size()));
        table->setHorizontalHeaderLabels(headings);

        for (int row = 0; row != table->rowCount(); ++row)
            for (int column = 0; column != table->columnCount(); ++column)
                table->setItem(row, column, new QTableWidgetItem(QString::fromStdString(shown.rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)])));
    }

    void ResultsPanel::Present(const present::RunOutcome& outcome)
    {
        Fill(cycles, present::CycleTable(outcome.rows));
        Fill(ledger, present::LedgerTable(outcome.ledger));

        if (outcome.distribution.has_value())
        {
            Fill(summary, present::OutcomeTable(*outcome.distribution));
            chart->Present(present::FanOf(*outcome.distribution, outcome.boundaries));
        }
        else if (outcome.recommendation.has_value())
        {
            Fill(summary, present::RecommendationTable(*outcome.recommendation));
            chart->Present(present::TraceOf({}, {}));
        }
        else
        {
            Fill(summary, present::ResultTable{ { "measure", "value" }, {} });
            chart->Present(present::TraceOf(outcome.equity, outcome.boundaries));
        }
    }

    void ResultsPanel::Clear()
    {
        Fill(cycles, present::CycleTable({}));
        Fill(ledger, present::ResultTable{ { "date", "kind", "amount" }, {} });
        Fill(summary, present::ResultTable{ { "measure", "value" }, {} });
        chart->Present(present::TraceOf({}, {}));
    }
}
