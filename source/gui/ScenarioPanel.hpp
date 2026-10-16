#pragma once

#include "source/present/ScenarioDocument.hpp"
#include <QString>
#include <QWidget>
#include <cstddef>
#include <string>
#include <vector>

class QCheckBox;
class QDateEdit;
class QLineEdit;
class QSpinBox;
class QTableWidget;

namespace fin::gui
{
    class ScenarioPanel
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ScenarioPanel(QWidget* parent = nullptr);

        const present::ScenarioDocument& Document() const;
        void Present(const present::ScenarioDocument& document);

        void Add();
        void Remove();
        void MoveBy(int step);
        int Selected() const;
        bool Adopt(std::size_t index, const std::vector<std::string>& closes);
        const QString& Complaint() const;

    signals:
        void Edited();

    private:
        void BuildFields();
        void BuildUnderlyings();
        void Gather();
        void GatherRows();
        void ShowRows();

        present::ScenarioDocument document;
        bool filling{ false };
        QString complaint;

        QLineEdit* name{ nullptr };
        QSpinBox* capital{ nullptr };
        QDateEdit* start{ nullptr };
        QSpinBox* cycles{ nullptr };
        QSpinBox* daysPerCycle{ nullptr };
        QSpinBox* observation{ nullptr };
        QSpinBox* targetDelta{ nullptr };
        QSpinBox* cashBuffer{ nullptr };
        QSpinBox* rollDeltaLimit{ nullptr };
        QSpinBox* rollDaysToExpiry{ nullptr };
        QSpinBox* rollProfitCapture{ nullptr };
        QSpinBox* riskFreeRate{ nullptr };
        QCheckBox* assessed{ nullptr };
        QSpinBox* assessmentMonth{ nullptr };
        QSpinBox* assessmentDay{ nullptr };
        QSpinBox* paths{ nullptr };
        QSpinBox* seed{ nullptr };
        QLineEdit* taxConfiguration{ nullptr };
        QLineEdit* brokerConfiguration{ nullptr };
        QTableWidget* underlyings{ nullptr };
    };
}
