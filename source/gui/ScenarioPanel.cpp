#include "source/gui/ScenarioPanel.hpp"
#include "source/present/PriceHistory.hpp"
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <limits>

namespace fin::gui
{
    namespace
    {
        constexpr int columns = 8;
        constexpr int mostBasisPoints = 1000000;

        QSpinBox* Counter(const char* named, int least, int most)
        {
            auto* box = new QSpinBox;
            box->setObjectName(named);
            box->setRange(least, most);
            box->setKeyboardTracking(false);

            return box;
        }

        QLineEdit* Text(const char* named)
        {
            auto* edit = new QLineEdit;
            edit->setObjectName(named);

            return edit;
        }

        QPushButton* Button(const char* named, const QString& caption)
        {
            auto* button = new QPushButton(caption);
            button->setObjectName(named);

            return button;
        }

        QDate DateOf(const core::Date& date)
        {
            return QDate(date.Year(), static_cast<int>(date.Month()), static_cast<int>(date.Day()));
        }

        core::Date DateOf(const QDate& date)
        {
            return core::Date::FromYearMonthDay(date.year(), static_cast<std::uint32_t>(date.month()), static_cast<std::uint32_t>(date.day()));
        }
    }

    ScenarioPanel::ScenarioPanel(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);

        BuildFields();
        BuildUnderlyings();

        auto* fields = new QGroupBox(tr("Scenario"));
        auto* form = new QFormLayout(fields);

        form->addRow(tr("Name"), name);
        form->addRow(tr("Capital (cents)"), capital);
        form->addRow(tr("Start"), start);
        form->addRow(tr("Cycles"), cycles);
        form->addRow(tr("Days a cycle"), daysPerCycle);
        form->addRow(tr("Observation days before expiry"), observation);
        form->addRow(tr("Target delta (basis points)"), targetDelta);
        form->addRow(tr("Cash buffer (basis points)"), cashBuffer);
        form->addRow(tr("Roll delta limit (basis points)"), rollDeltaLimit);
        form->addRow(tr("Roll days to expiry"), rollDaysToExpiry);
        form->addRow(tr("Roll profit capture (basis points)"), rollProfitCapture);
        form->addRow(tr("Risk free rate (basis points)"), riskFreeRate);
        form->addRow(assessed, new QLabel(tr("Assessed every year on")));
        form->addRow(tr("Assessment month"), assessmentMonth);
        form->addRow(tr("Assessment day"), assessmentDay);
        form->addRow(tr("Paths"), paths);
        form->addRow(tr("Seed"), seed);
        form->addRow(tr("Tax configuration"), taxConfiguration);
        form->addRow(tr("Broker configuration"), brokerConfiguration);

        auto* book = new QGroupBox(tr("Underlyings, in the order collateral is offered"));
        auto* stacked = new QVBoxLayout(book);
        auto* buttons = new QHBoxLayout;

        auto* add = Button("add", tr("Add"));
        auto* remove = Button("remove", tr("Remove"));
        auto* up = Button("up", tr("Up"));
        auto* down = Button("down", tr("Down"));

        buttons->addWidget(add);
        buttons->addWidget(remove);
        buttons->addWidget(up);
        buttons->addWidget(down);
        buttons->addStretch();

        stacked->addWidget(underlyings);
        stacked->addLayout(buttons);

        layout->addWidget(fields);
        layout->addWidget(book);
        layout->addStretch();

        connect(add, &QPushButton::clicked, this, &ScenarioPanel::Add);
        connect(remove, &QPushButton::clicked, this, &ScenarioPanel::Remove);
        connect(up, &QPushButton::clicked, this, [this]
            {
                MoveBy(-1);
            });
        connect(down, &QPushButton::clicked, this, [this]
            {
                MoveBy(1);
            });

        Present(present::ScenarioDocument());
    }

    void ScenarioPanel::BuildFields()
    {
        name = Text("scenario");
        capital = Counter("capital_cents", 0, std::numeric_limits<int>::max());
        cycles = Counter("cycles", 0, 10000);
        daysPerCycle = Counter("days_per_cycle", 0, 10000);
        observation = Counter("observation_days_before_expiry", 0, 10000);
        targetDelta = Counter("target_delta_basis_points", 0, mostBasisPoints);
        cashBuffer = Counter("cash_buffer_basis_points", 0, mostBasisPoints);
        rollDeltaLimit = Counter("roll_delta_limit_basis_points", 0, mostBasisPoints);
        rollDaysToExpiry = Counter("roll_days_to_expiry", 0, 10000);
        rollProfitCapture = Counter("roll_profit_capture_basis_points", 0, mostBasisPoints);
        riskFreeRate = Counter("risk_free_rate_basis_points", -mostBasisPoints, mostBasisPoints);
        assessmentMonth = Counter("assessment_month", 1, 12);
        assessmentDay = Counter("assessment_day", 1, 31);
        paths = Counter("paths", 0, 1000000);
        seed = Counter("seed", 0, std::numeric_limits<int>::max());
        taxConfiguration = Text("tax_configuration");
        brokerConfiguration = Text("broker_configuration");

        start = new QDateEdit;
        start->setObjectName("start_date");
        start->setDisplayFormat("yyyy-MM-dd");
        start->setCalendarPopup(true);

        assessed = new QCheckBox;
        assessed->setObjectName("assessment");

        for (auto* box : { capital, cycles, daysPerCycle, observation, targetDelta, cashBuffer, rollDeltaLimit, rollDaysToExpiry, rollProfitCapture, riskFreeRate, assessmentMonth, assessmentDay, paths, seed })
            connect(box, &QSpinBox::valueChanged, this, &ScenarioPanel::Gather);

        for (auto* edit : { name, taxConfiguration, brokerConfiguration })
            connect(edit, &QLineEdit::textChanged, this, &ScenarioPanel::Gather);

        connect(start, &QDateEdit::dateChanged, this, &ScenarioPanel::Gather);
        connect(assessed, &QCheckBox::toggled, this, &ScenarioPanel::Gather);
    }

    void ScenarioPanel::BuildUnderlyings()
    {
        underlyings = new QTableWidget(0, columns);
        underlyings->setObjectName("underlyings");
        underlyings->setHorizontalHeaderLabels(QStringList{ tr("Ticker"), tr("Spot (cents)"), tr("Drift (bp)"), tr("Volatility (bp)"), tr("Implied volatility (bp)"), tr("Dividend yield (bp)"), tr("Withholding (bp)"), tr("Strike increment (cents)") });
        underlyings->horizontalHeader()->setStretchLastSection(true);
        underlyings->setSelectionBehavior(QAbstractItemView::SelectRows);
        underlyings->setSelectionMode(QAbstractItemView::SingleSelection);

        connect(underlyings, &QTableWidget::itemChanged, this, &ScenarioPanel::GatherRows);
    }

    const present::ScenarioDocument& ScenarioPanel::Document() const
    {
        return document;
    }

    void ScenarioPanel::Present(const present::ScenarioDocument& shown)
    {
        document = shown;

        const auto fields = document.Fields();
        const auto guard = filling;
        filling = true;

        name->setText(QString::fromStdString(fields.name));
        capital->setValue(fields.capitalCents);
        start->setDate(DateOf(fields.start));
        cycles->setValue(fields.cycles);
        daysPerCycle->setValue(fields.daysPerCycle);
        observation->setValue(fields.observationDaysBeforeExpiry);
        targetDelta->setValue(fields.targetDeltaBasisPoints);
        cashBuffer->setValue(fields.cashBufferBasisPoints);
        rollDeltaLimit->setValue(fields.rollDeltaLimitBasisPoints);
        rollDaysToExpiry->setValue(fields.rollDaysToExpiry);
        rollProfitCapture->setValue(fields.rollProfitCaptureBasisPoints);
        riskFreeRate->setValue(fields.riskFreeRateBasisPoints);
        assessed->setChecked(fields.assessment.has_value());
        assessmentMonth->setValue(fields.assessment.has_value() ? static_cast<int>(fields.assessment->month) : 1);
        assessmentDay->setValue(fields.assessment.has_value() ? static_cast<int>(fields.assessment->day) : 1);
        paths->setValue(fields.paths);
        seed->setValue(fields.seed);
        taxConfiguration->setText(QString::fromStdString(fields.taxConfiguration));
        brokerConfiguration->setText(QString::fromStdString(fields.brokerConfiguration));

        ShowRows();

        filling = guard;

        emit Edited();
    }

    void ScenarioPanel::ShowRows()
    {
        const auto guard = filling;
        filling = true;

        underlyings->setRowCount(static_cast<int>(document.Underlyings().size()));

        for (int row = 0; row != underlyings->rowCount(); ++row)
        {
            const auto& held = document.Underlyings()[static_cast<std::size_t>(row)];
            const QStringList cells{ QString::fromStdString(held.ticker),
                QString::number(held.spotCents),
                QString::number(held.driftBasisPoints),
                QString::number(held.volatilityBasisPoints),
                QString::number(held.impliedVolatilityBasisPoints),
                QString::number(held.dividendYieldBasisPoints),
                QString::number(held.dividendWithholdingBasisPoints),
                QString::number(held.strikeIncrementCents) };

            for (int column = 0; column != columns; ++column)
                underlyings->setItem(row, column, new QTableWidgetItem(cells.at(column)));
        }

        filling = guard;
    }

    void ScenarioPanel::Gather()
    {
        if (filling)
            return;

        auto fields = document.Fields();

        fields.name = name->text().toStdString();
        fields.capitalCents = capital->value();
        fields.start = DateOf(start->date());
        fields.cycles = cycles->value();
        fields.daysPerCycle = daysPerCycle->value();
        fields.observationDaysBeforeExpiry = observation->value();
        fields.targetDeltaBasisPoints = targetDelta->value();
        fields.cashBufferBasisPoints = cashBuffer->value();
        fields.rollDeltaLimitBasisPoints = rollDeltaLimit->value();
        fields.rollDaysToExpiry = rollDaysToExpiry->value();
        fields.rollProfitCaptureBasisPoints = rollProfitCapture->value();
        fields.riskFreeRateBasisPoints = riskFreeRate->value();
        fields.assessment = assessed->isChecked() ? std::optional<sim::AnnualReference>(sim::AnnualReference{ static_cast<std::uint32_t>(assessmentMonth->value()), static_cast<std::uint32_t>(assessmentDay->value()) }) : std::nullopt;
        fields.paths = paths->value();
        fields.seed = seed->value();
        fields.taxConfiguration = taxConfiguration->text().toStdString();
        fields.brokerConfiguration = brokerConfiguration->text().toStdString();

        document.Amend(fields);

        emit Edited();
    }

    void ScenarioPanel::GatherRows()
    {
        if (filling)
            return;

        std::vector<present::UnderlyingRow> rows;

        for (int row = 0; row != underlyings->rowCount(); ++row)
        {
            const auto cell = [this, row](int column)
            {
                const auto* item = underlyings->item(row, column);
                return item == nullptr ? QString() : item->text();
            };

            rows.push_back(present::UnderlyingRow{ .ticker = cell(0).toStdString(),
                .spotCents = cell(1).toInt(),
                .driftBasisPoints = cell(2).toInt(),
                .volatilityBasisPoints = cell(3).toInt(),
                .impliedVolatilityBasisPoints = cell(4).toInt(),
                .dividendYieldBasisPoints = cell(5).toInt(),
                .dividendWithholdingBasisPoints = cell(6).toInt(),
                .strikeIncrementCents = cell(7).toInt() });
        }

        while (document.Underlyings().size() > rows.size())
            document.Remove(document.Underlyings().size() - 1u);

        for (std::size_t row = 0; row != rows.size(); ++row)
            if (row < document.Underlyings().size())
                document.Replace(row, rows[row]);
            else
                document.Append(rows[row]);

        emit Edited();
    }

    void ScenarioPanel::Add()
    {
        document.Append(present::UnderlyingRow{ .ticker = "NEW", .spotCents = 10000, .driftBasisPoints = 0, .volatilityBasisPoints = 2500, .impliedVolatilityBasisPoints = 2500, .dividendYieldBasisPoints = 0, .dividendWithholdingBasisPoints = 1500, .strikeIncrementCents = 100 });

        ShowRows();
        emit Edited();
    }

    void ScenarioPanel::Remove()
    {
        const auto selected = Selected();

        if (selected < 0)
            return;

        document.Remove(static_cast<std::size_t>(selected));

        ShowRows();
        emit Edited();
    }

    void ScenarioPanel::MoveBy(int step)
    {
        const auto selected = Selected();
        const auto moved = selected + step;

        if (selected < 0 || moved < 0 || moved >= static_cast<int>(document.Underlyings().size()))
            return;

        document.Move(static_cast<std::size_t>(selected), static_cast<std::size_t>(moved));

        ShowRows();
        underlyings->selectRow(moved);
        emit Edited();
    }

    int ScenarioPanel::Selected() const
    {
        return underlyings->currentRow();
    }

    bool ScenarioPanel::Adopt(std::size_t index, const std::vector<std::string>& closes)
    {
        if (index >= document.Underlyings().size())
            return false;

        const present::PriceHistory history(closes, document.Fields().currency, document.Underlyings()[index].dividendYieldBasisPoints);

        if (!history.Estimate().has_value())
        {
            complaint = QString::fromStdString(history.Failure());
            return false;
        }

        complaint.clear();
        document.Adopt(index, history.Estimate()->volatilityBasisPoints, history.Estimate()->driftBasisPoints);

        ShowRows();
        emit Edited();

        return true;
    }

    const QString& ScenarioPanel::Complaint() const
    {
        return complaint;
    }
}
