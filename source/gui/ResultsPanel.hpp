#pragma once

#include "source/present/ResultTable.hpp"
#include "source/present/RunController.hpp"
#include <QWidget>

class QLabel;
class QTableWidget;

namespace fin::gui
{
    class FanChartWidget;

    class ResultsPanel
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ResultsPanel(QWidget* parent = nullptr);

        void Present(const present::RunOutcome& outcome);
        void Clear();

    private:
        void Fill(QTableWidget* table, const present::ResultTable& shown);

        QTableWidget* cycles{ nullptr };
        QTableWidget* ledger{ nullptr };
        QTableWidget* summary{ nullptr };
        FanChartWidget* chart{ nullptr };
        QLabel* disclaimer{ nullptr };
    };
}
