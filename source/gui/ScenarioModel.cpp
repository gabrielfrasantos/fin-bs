#include "source/gui/ScenarioModel.hpp"
#include "source/present/PriceHistory.hpp"
#include <QFile>
#include <QTextStream>
#include <cstdint>
#include <optional>

namespace fin::gui
{
    namespace
    {
        QDate DateOf(const core::Date& date)
        {
            return QDate(date.Year(), static_cast<int>(date.Month()), static_cast<int>(date.Day()));
        }

        core::Date DateOf(const QDate& date)
        {
            return core::Date::FromYearMonthDay(date.year(), static_cast<std::uint32_t>(date.month()), static_cast<std::uint32_t>(date.day()));
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
    }

    ScenarioModel::ScenarioModel(QObject* parent)
        : QObject(parent)
        , underlyings(new UnderlyingsModel(document, this))
    {
        connect(underlyings, &UnderlyingsModel::edited, this, &ScenarioModel::changed);
    }

    const present::ScenarioDocument& ScenarioModel::Document() const
    {
        return document;
    }

    void ScenarioModel::Present(const present::ScenarioDocument& shown)
    {
        document = shown;
        priceComplaint.clear();
        underlyings->Reset();

        emit changed();
    }

    template<class Edit>
    void ScenarioModel::Amend(Edit edit)
    {
        auto fields = document.Fields();
        edit(fields);
        document.Amend(fields);

        emit changed();
    }

    QString ScenarioModel::Name() const
    {
        return QString::fromStdString(document.Fields().name);
    }

    void ScenarioModel::SetName(const QString& name)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.name = name.toStdString();
            });
    }

    QString ScenarioModel::Currency() const
    {
        return document.Fields().currency == core::Currency::Eur ? QStringLiteral("EUR") : QStringLiteral("USD");
    }

    int ScenarioModel::CapitalCents() const
    {
        return document.Fields().capitalCents;
    }

    void ScenarioModel::SetCapitalCents(int cents)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.capitalCents = cents;
            });
    }

    QDate ScenarioModel::Start() const
    {
        return DateOf(document.Fields().start);
    }

    void ScenarioModel::SetStart(const QDate& date)
    {
        if (!date.isValid())
            return;

        Amend([&](present::ScenarioFields& fields)
            {
                fields.start = DateOf(date);
            });
    }

    int ScenarioModel::Cycles() const
    {
        return document.Fields().cycles;
    }

    void ScenarioModel::SetCycles(int count)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.cycles = count;
            });
    }

    int ScenarioModel::DaysPerCycle() const
    {
        return document.Fields().daysPerCycle;
    }

    void ScenarioModel::SetDaysPerCycle(int days)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.daysPerCycle = days;
            });
    }

    int ScenarioModel::ObservationDaysBeforeExpiry() const
    {
        return document.Fields().observationDaysBeforeExpiry;
    }

    void ScenarioModel::SetObservationDaysBeforeExpiry(int days)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.observationDaysBeforeExpiry = days;
            });
    }

    int ScenarioModel::TargetDeltaBasisPoints() const
    {
        return document.Fields().targetDeltaBasisPoints;
    }

    void ScenarioModel::SetTargetDeltaBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.targetDeltaBasisPoints = basisPoints;
            });
    }

    int ScenarioModel::CashBufferBasisPoints() const
    {
        return document.Fields().cashBufferBasisPoints;
    }

    void ScenarioModel::SetCashBufferBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.cashBufferBasisPoints = basisPoints;
            });
    }

    int ScenarioModel::MaxCollateralPerNameBasisPoints() const
    {
        return document.Fields().maxCollateralPerNameBasisPoints;
    }

    void ScenarioModel::SetMaxCollateralPerNameBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.maxCollateralPerNameBasisPoints = basisPoints;
            });
    }

    int ScenarioModel::RollDeltaLimitBasisPoints() const
    {
        return document.Fields().rollDeltaLimitBasisPoints;
    }

    void ScenarioModel::SetRollDeltaLimitBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.rollDeltaLimitBasisPoints = basisPoints;
            });
    }

    int ScenarioModel::RollDaysToExpiry() const
    {
        return document.Fields().rollDaysToExpiry;
    }

    void ScenarioModel::SetRollDaysToExpiry(int days)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.rollDaysToExpiry = days;
            });
    }

    int ScenarioModel::RollProfitCaptureBasisPoints() const
    {
        return document.Fields().rollProfitCaptureBasisPoints;
    }

    void ScenarioModel::SetRollProfitCaptureBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.rollProfitCaptureBasisPoints = basisPoints;
            });
    }

    int ScenarioModel::RiskFreeRateBasisPoints() const
    {
        return document.Fields().riskFreeRateBasisPoints;
    }

    void ScenarioModel::SetRiskFreeRateBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.riskFreeRateBasisPoints = basisPoints;
            });
    }

    bool ScenarioModel::Assessed() const
    {
        return document.Fields().assessment.has_value();
    }

    void ScenarioModel::SetAssessed(bool assessed)
    {
        if (assessed == Assessed())
            return;

        Amend([&](present::ScenarioFields& fields)
            {
                fields.assessment = assessed ? std::optional<sim::AnnualReference>(sim::AnnualReference{ 1, 1 }) : std::nullopt;
            });
    }

    int ScenarioModel::AssessmentMonth() const
    {
        return Assessed() ? static_cast<int>(document.Fields().assessment->month) : 1;
    }

    void ScenarioModel::SetAssessmentMonth(int month)
    {
        if (!Assessed())
            return;

        Amend([&](present::ScenarioFields& fields)
            {
                fields.assessment->month = static_cast<std::uint32_t>(month);
            });
    }

    int ScenarioModel::AssessmentDay() const
    {
        return Assessed() ? static_cast<int>(document.Fields().assessment->day) : 1;
    }

    void ScenarioModel::SetAssessmentDay(int day)
    {
        if (!Assessed())
            return;

        Amend([&](present::ScenarioFields& fields)
            {
                fields.assessment->day = static_cast<std::uint32_t>(day);
            });
    }

    int ScenarioModel::Paths() const
    {
        return document.Fields().paths;
    }

    void ScenarioModel::SetPaths(int count)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.paths = count;
            });
    }

    int ScenarioModel::Seed() const
    {
        return document.Fields().seed;
    }

    void ScenarioModel::SetSeed(int seed)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.seed = seed;
            });
    }

    QString ScenarioModel::TaxConfiguration() const
    {
        return QString::fromStdString(document.Fields().taxConfiguration);
    }

    void ScenarioModel::SetTaxConfiguration(const QString& path)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.taxConfiguration = path.toStdString();
            });
    }

    QString ScenarioModel::BrokerConfiguration() const
    {
        return QString::fromStdString(document.Fields().brokerConfiguration);
    }

    void ScenarioModel::SetBrokerConfiguration(const QString& path)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.brokerConfiguration = path.toStdString();
            });
    }

    bool ScenarioModel::Calibrated() const
    {
        return document.Fields().calibrated;
    }

    void ScenarioModel::SetCalibrated(bool calibrated)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.calibrated = calibrated;
            });
    }

    QString ScenarioModel::PriceHistories() const
    {
        return QString::fromStdString(document.Fields().priceHistories);
    }

    void ScenarioModel::SetPriceHistories(const QString& folder)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.priceHistories = folder.toStdString();
            });
    }

    int ScenarioModel::WindowObservations() const
    {
        return document.Fields().windowObservations;
    }

    void ScenarioModel::SetWindowObservations(int observations)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.windowObservations = observations;
            });
    }

    int ScenarioModel::VolatilityEstimator() const
    {
        return document.Fields().volatilityEstimator == sim::VolatilityEstimator::historical ? 0 : 1;
    }

    void ScenarioModel::SetVolatilityEstimator(int estimator)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.volatilityEstimator = estimator == 0 ? sim::VolatilityEstimator::historical : sim::VolatilityEstimator::exponentiallyWeighted;
            });
    }

    int ScenarioModel::HalfLifeObservations() const
    {
        return document.Fields().halfLifeObservations;
    }

    void ScenarioModel::SetHalfLifeObservations(int observations)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.halfLifeObservations = observations;
            });
    }

    int ScenarioModel::DriftEstimator() const
    {
        return document.Fields().driftEstimator == sim::DriftEstimator::leastSquares ? 0 : 1;
    }

    void ScenarioModel::SetDriftEstimator(int estimator)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.driftEstimator = estimator == 0 ? sim::DriftEstimator::leastSquares : sim::DriftEstimator::stated;
            });
    }

    int ScenarioModel::ImpliedPremiumBasisPoints() const
    {
        return document.Fields().impliedPremiumBasisPoints;
    }

    void ScenarioModel::SetImpliedPremiumBasisPoints(int basisPoints)
    {
        Amend([&](present::ScenarioFields& fields)
            {
                fields.impliedPremiumBasisPoints = basisPoints;
            });
    }

    bool ScenarioModel::WellFormed() const
    {
        return present::IsWellFormed(document);
    }

    QString ScenarioModel::OffendingField() const
    {
        const auto offending = document.Offending();

        return offending.has_value() ? QString::fromStdString(offending->field) : QString();
    }

    QString ScenarioModel::Complaint() const
    {
        const auto offending = document.Offending();

        return offending.has_value() ? QString::fromStdString(present::Describing(*offending)) : QString();
    }

    QString ScenarioModel::PriceComplaint() const
    {
        return priceComplaint;
    }

    UnderlyingsModel* ScenarioModel::Underlyings()
    {
        return underlyings;
    }

    void ScenarioModel::add()
    {
        document.Append(present::UnderlyingRow{ .ticker = "NEW", .spotCents = 10000, .driftBasisPoints = 0, .volatilityBasisPoints = 2500, .impliedVolatilityBasisPoints = 2500, .dividendYieldBasisPoints = 0, .dividendWithholdingBasisPoints = 1500, .strikeIncrementCents = 100 });
        underlyings->Reset();

        emit changed();
    }

    void ScenarioModel::remove(int index)
    {
        if (index < 0 || index >= underlyings->Count())
            return;

        document.Remove(static_cast<std::size_t>(index));
        underlyings->Reset();

        emit changed();
    }

    void ScenarioModel::move(int index, int step)
    {
        const auto moved = index + step;

        if (index < 0 || index >= underlyings->Count() || moved < 0 || moved >= underlyings->Count())
            return;

        document.Move(static_cast<std::size_t>(index), static_cast<std::size_t>(moved));
        underlyings->Reset();

        emit changed();
    }

    bool ScenarioModel::estimate(int index, const QString& path)
    {
        if (index < 0)
            return false;

        return Adopt(static_cast<std::size_t>(index), Lines(path));
    }

    bool ScenarioModel::Adopt(std::size_t index, const std::vector<std::string>& closes)
    {
        if (index >= document.Underlyings().size())
            return false;

        const present::PriceHistory history(closes, document.Fields().currency, document.Underlyings()[index].dividendYieldBasisPoints);

        if (!history.Estimate().has_value())
        {
            priceComplaint = QString::fromStdString(history.Failure());
            emit changed();
            return false;
        }

        priceComplaint.clear();
        document.Adopt(index, history.Estimate()->volatilityBasisPoints, history.Estimate()->driftBasisPoints);
        underlyings->Reset();

        emit changed();

        return true;
    }
}
