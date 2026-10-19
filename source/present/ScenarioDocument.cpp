#include "source/present/ScenarioDocument.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace fin::present
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
        constexpr std::int32_t mostVolatilityBasisPoints = 100000;
        constexpr std::int32_t mostCycles = 1000;
        constexpr std::int32_t mostDaysPerCycle = 365;
        constexpr std::int32_t mostPaths = 1000000;
        constexpr std::int32_t nonLeapYear = 2001;
        constexpr std::int32_t mostCents = std::numeric_limits<std::int32_t>::max();

        double FractionOf(std::int32_t basisPoints)
        {
            return static_cast<double>(basisPoints) / static_cast<double>(basisPointsPerUnit);
        }

        std::int32_t BasisPointsOf(double fraction)
        {
            return static_cast<std::int32_t>(std::llround(fraction * basisPointsPerUnit));
        }

        std::optional<FieldFault> Unnamed(std::string field, const std::string& text)
        {
            if (text.empty())
                return FieldFault{ std::move(field), "names nothing" };

            return std::nullopt;
        }

        std::optional<FieldFault> Outside(std::string field, std::int32_t value, std::int32_t least, std::int32_t most)
        {
            if (value < least || value > most)
                return FieldFault{ std::move(field), "lies outside " + std::to_string(least) + " to " + std::to_string(most) };

            return std::nullopt;
        }

        std::string RowField(std::size_t index, const std::string& field)
        {
            return "underlyings[" + std::to_string(index) + "]." + field;
        }

        ScenarioFields Starting()
        {
            return ScenarioFields{ .name = "New scenario",
                .currency = core::Currency::Eur,
                .capitalCents = 10000000,
                .start = core::Date::FromYearMonthDay(2026, 1, 5),
                .cycles = 4,
                .daysPerCycle = 91,
                .observationDaysBeforeExpiry = 14,
                .targetDeltaBasisPoints = 3000,
                .cashBufferBasisPoints = 500,
                .rollDeltaLimitBasisPoints = 5000,
                .rollDaysToExpiry = 7,
                .rollProfitCaptureBasisPoints = 8000,
                .riskFreeRateBasisPoints = 250,
                .assessment = sim::AnnualReference{ 1, 1 },
                .paths = 256,
                .seed = 1,
                .taxConfiguration = "tax/nl_2025.json",
                .brokerConfiguration = "broker/degiro.json" };
        }
    }

    ScenarioDocument::ScenarioDocument()
        : fields(Starting())
    {}

    ScenarioDocument::ScenarioDocument(const io::Scenario& scenario)
        : fields(ScenarioFields{ .name = scenario.name,
              .currency = io::CurrencyOf(scenario),
              .capitalCents = static_cast<std::int32_t>(scenario.capital.Cents()),
              .start = scenario.terms.start,
              .cycles = scenario.terms.cycles,
              .daysPerCycle = scenario.terms.daysPerCycle,
              .observationDaysBeforeExpiry = scenario.terms.observationDaysBeforeExpiry,
              .targetDeltaBasisPoints = BasisPointsOf(scenario.terms.targetDelta.Value()),
              .cashBufferBasisPoints = scenario.cashBufferBasisPoints,
              .rollDeltaLimitBasisPoints = BasisPointsOf(scenario.triggers.deltaLimit.Value()),
              .rollDaysToExpiry = scenario.triggers.daysToExpiry,
              .rollProfitCaptureBasisPoints = scenario.triggers.profitCaptureBasisPoints,
              .riskFreeRateBasisPoints = BasisPointsOf(scenario.terms.riskFreeRate.Continuous()),
              .assessment = scenario.terms.assessment,
              .paths = scenario.paths,
              .seed = static_cast<std::int32_t>(scenario.seed),
              .taxConfiguration = scenario.taxConfiguration,
              .brokerConfiguration = scenario.brokerConfiguration })
    {
        underlyings.reserve(scenario.terms.underlyings.size());

        for (const auto& underlying : scenario.terms.underlyings)
            underlyings.push_back(UnderlyingRow{ .ticker = underlying.underlying,
                .spotCents = static_cast<std::int32_t>(underlying.initialPrice.Cents()),
                .driftBasisPoints = BasisPointsOf(underlying.drift.Continuous()),
                .volatilityBasisPoints = BasisPointsOf(underlying.volatility.Annualized()),
                .impliedVolatilityBasisPoints = BasisPointsOf(underlying.impliedVolatility.Annualized()),
                .dividendYieldBasisPoints = BasisPointsOf(underlying.dividendYield.Continuous()),
                .dividendWithholdingBasisPoints = underlying.dividendWithholdingBasisPoints,
                .strikeIncrementCents = static_cast<std::int32_t>(underlying.strikeIncrement.Cents()) });
    }

    const ScenarioFields& ScenarioDocument::Fields() const
    {
        return fields;
    }

    void ScenarioDocument::Amend(const ScenarioFields& amended)
    {
        fields = amended;
    }

    std::span<const UnderlyingRow> ScenarioDocument::Underlyings() const
    {
        return underlyings;
    }

    void ScenarioDocument::Append(const UnderlyingRow& row)
    {
        underlyings.push_back(row);
    }

    void ScenarioDocument::Replace(std::size_t index, const UnderlyingRow& row)
    {
        really_assert(index < underlyings.size());
        underlyings[index] = row;
    }

    void ScenarioDocument::Remove(std::size_t index)
    {
        really_assert(index < underlyings.size());
        underlyings.erase(underlyings.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void ScenarioDocument::Move(std::size_t from, std::size_t to)
    {
        really_assert(from < underlyings.size() && to < underlyings.size());

        const auto row = underlyings[from];
        underlyings.erase(underlyings.begin() + static_cast<std::ptrdiff_t>(from));
        underlyings.insert(underlyings.begin() + static_cast<std::ptrdiff_t>(to), row);
    }

    void ScenarioDocument::Adopt(std::size_t index, std::int32_t volatilityBasisPoints, std::int32_t driftBasisPoints)
    {
        really_assert(index < underlyings.size());

        underlyings[index].volatilityBasisPoints = volatilityBasisPoints;
        underlyings[index].driftBasisPoints = driftBasisPoints;
    }

    std::optional<FieldFault> ScenarioDocument::Offending() const
    {
        if (auto fault = Unnamed("scenario", fields.name))
            return fault;

        if (auto fault = Outside("capital_cents", fields.capitalCents, 1, mostCents))
            return fault;

        if (auto fault = Outside("cycles", fields.cycles, 1, mostCycles))
            return fault;

        if (auto fault = Outside("days_per_cycle", fields.daysPerCycle, 1, mostDaysPerCycle))
            return fault;

        if (auto fault = Outside("observation_days_before_expiry", fields.observationDaysBeforeExpiry, 0, fields.daysPerCycle - 1))
            return fault;

        if (auto fault = Outside("target_delta_basis_points", fields.targetDeltaBasisPoints, 1, basisPointsPerUnit - 1))
            return fault;

        if (auto fault = Outside("cash_buffer_basis_points", fields.cashBufferBasisPoints, 0, basisPointsPerUnit))
            return fault;

        if (auto fault = Outside("roll_delta_limit_basis_points", fields.rollDeltaLimitBasisPoints, 1, basisPointsPerUnit))
            return fault;

        if (auto fault = Outside("roll_days_to_expiry", fields.rollDaysToExpiry, 0, mostDaysPerCycle))
            return fault;

        if (auto fault = Outside("roll_profit_capture_basis_points", fields.rollProfitCaptureBasisPoints, 1, basisPointsPerUnit))
            return fault;

        if (auto fault = Outside("risk_free_rate_basis_points", fields.riskFreeRateBasisPoints, -basisPointsPerUnit, basisPointsPerUnit))
            return fault;

        if (fields.assessment.has_value() && !core::Date::IsValid(nonLeapYear, fields.assessment->month, fields.assessment->day))
            return FieldFault{ "assessment", "is not a day that every year holds" };

        if (auto fault = Outside("paths", fields.paths, 1, mostPaths))
            return fault;

        if (auto fault = Outside("seed", fields.seed, 0, std::numeric_limits<std::int32_t>::max()))
            return fault;

        if (auto fault = Unnamed("tax_configuration", fields.taxConfiguration))
            return fault;

        if (auto fault = Unnamed("broker_configuration", fields.brokerConfiguration))
            return fault;

        if (underlyings.empty())
            return FieldFault{ "underlyings", "names no underlying to wheel" };

        for (std::size_t index = 0; index != underlyings.size(); ++index)
            if (auto fault = OffendingRow(index))
                return fault;

        return std::nullopt;
    }

    std::optional<FieldFault> ScenarioDocument::OffendingRow(std::size_t index) const
    {
        const auto& row = underlyings[index];

        if (auto fault = Unnamed(RowField(index, "ticker"), row.ticker))
            return fault;

        if (std::count_if(underlyings.begin(), underlyings.end(), [&row](const UnderlyingRow& other)
                {
                    return other.ticker == row.ticker;
                }) != 1)
            return FieldFault{ RowField(index, "ticker"), "names an underlying the book already holds" };

        if (auto fault = Outside(RowField(index, "spot_cents"), row.spotCents, 1, mostCents))
            return fault;

        if (auto fault = Outside(RowField(index, "drift_basis_points"), row.driftBasisPoints, -basisPointsPerUnit, basisPointsPerUnit))
            return fault;

        if (auto fault = Outside(RowField(index, "volatility_basis_points"), row.volatilityBasisPoints, 1, mostVolatilityBasisPoints))
            return fault;

        if (auto fault = Outside(RowField(index, "implied_volatility_basis_points"), row.impliedVolatilityBasisPoints, 1, mostVolatilityBasisPoints))
            return fault;

        if (auto fault = Outside(RowField(index, "dividend_yield_basis_points"), row.dividendYieldBasisPoints, 0, basisPointsPerUnit))
            return fault;

        if (auto fault = Outside(RowField(index, "dividend_withholding_basis_points"), row.dividendWithholdingBasisPoints, 0, basisPointsPerUnit))
            return fault;

        return Outside(RowField(index, "strike_increment_cents"), row.strikeIncrementCents, 1, mostCents);
    }

    io::Scenario ScenarioDocument::Prepared() const
    {
        really_assert(!Offending().has_value());

        std::vector<sim::UnderlyingTerms> terms;
        terms.reserve(underlyings.size());

        for (const auto& row : underlyings)
            terms.push_back(sim::UnderlyingTerms{ .underlying = row.ticker,
                .initialPrice = core::Money::FromCents(row.spotCents, fields.currency),
                .drift = core::Rate::FromContinuous(FractionOf(row.driftBasisPoints)),
                .volatility = core::Volatility::FromAnnualized(FractionOf(row.volatilityBasisPoints)),
                .impliedVolatility = core::Volatility::FromAnnualized(FractionOf(row.impliedVolatilityBasisPoints)),
                .dividendYield = core::Rate::FromContinuous(FractionOf(row.dividendYieldBasisPoints)),
                .dividendWithholdingBasisPoints = row.dividendWithholdingBasisPoints,
                .strikeIncrement = core::Money::FromCents(row.strikeIncrementCents, fields.currency) });

        return io::Scenario{ .name = fields.name,
            .capital = core::Money::FromCents(fields.capitalCents, fields.currency),
            .cashBufferBasisPoints = fields.cashBufferBasisPoints,
            .triggers = strategy::RollTriggers{ .daysToExpiry = fields.rollDaysToExpiry,
                .profitCaptureBasisPoints = fields.rollProfitCaptureBasisPoints,
                .deltaLimit = core::Delta::FromValue(FractionOf(fields.rollDeltaLimitBasisPoints)) },
            .terms = sim::BookTerms{ .start = fields.start,
                .cycles = fields.cycles,
                .daysPerCycle = fields.daysPerCycle,
                .observationDaysBeforeExpiry = fields.observationDaysBeforeExpiry,
                .targetDelta = core::Delta::FromValue(FractionOf(fields.targetDeltaBasisPoints)),
                .riskFreeRate = core::Rate::FromContinuous(FractionOf(fields.riskFreeRateBasisPoints)),
                .assessment = fields.assessment,
                .underlyings = std::move(terms) },
            .paths = fields.paths,
            .seed = static_cast<std::uint64_t>(fields.seed),
            .taxConfiguration = fields.taxConfiguration,
            .brokerConfiguration = fields.brokerConfiguration };
    }

    bool IsWellFormed(const ScenarioDocument& document)
    {
        return !document.Offending().has_value();
    }

    std::string Describing(const FieldFault& fault)
    {
        return fault.field + " " + fault.reason;
    }
}
