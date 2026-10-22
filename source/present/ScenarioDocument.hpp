#pragma once

#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include "source/io/ScenarioLoader.hpp"
#include "source/sim/Calibration.hpp"
#include "source/sim/WheelScript.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fin::present
{
    struct UnderlyingRow
    {
        std::string ticker;
        std::int32_t spotCents;
        std::int32_t driftBasisPoints;
        std::int32_t volatilityBasisPoints;
        std::int32_t impliedVolatilityBasisPoints;
        std::int32_t dividendYieldBasisPoints;
        std::int32_t dividendWithholdingBasisPoints;
        std::int32_t strikeIncrementCents;

        bool operator==(const UnderlyingRow& other) const = default;
    };

    struct ScenarioFields
    {
        std::string name;
        core::Currency currency;
        std::int32_t capitalCents;
        core::Date start;
        std::int32_t cycles;
        std::int32_t daysPerCycle;
        std::int32_t observationDaysBeforeExpiry;
        std::int32_t targetDeltaBasisPoints;
        std::int32_t cashBufferBasisPoints;
        std::int32_t maxCollateralPerNameBasisPoints;
        std::int32_t rollDeltaLimitBasisPoints;
        std::int32_t rollDaysToExpiry;
        std::int32_t rollProfitCaptureBasisPoints;
        std::int32_t riskFreeRateBasisPoints;
        std::optional<sim::AnnualReference> assessment;
        std::int32_t paths;
        std::int32_t seed;
        std::string taxConfiguration;
        std::string brokerConfiguration;
        bool calibrated;
        std::string priceHistories;
        std::int32_t windowObservations;
        sim::VolatilityEstimator volatilityEstimator;
        std::int32_t halfLifeObservations;
        sim::DriftEstimator driftEstimator;
        std::int32_t impliedPremiumBasisPoints;

        bool operator==(const ScenarioFields& other) const = default;
    };

    struct FieldFault
    {
        std::string field;
        std::string reason;

        bool operator==(const FieldFault& other) const = default;
    };

    class ScenarioDocument
    {
    public:
        ScenarioDocument();
        explicit ScenarioDocument(const io::Scenario& scenario);

        const ScenarioFields& Fields() const;
        void Amend(const ScenarioFields& fields);

        std::span<const UnderlyingRow> Underlyings() const;
        void Append(const UnderlyingRow& row);
        void Replace(std::size_t index, const UnderlyingRow& row);
        void Remove(std::size_t index);
        void Move(std::size_t from, std::size_t to);
        void Adopt(std::size_t index, std::int32_t volatilityBasisPoints, std::int32_t driftBasisPoints);

        std::optional<FieldFault> Offending() const;
        io::Scenario Prepared() const;

        bool operator==(const ScenarioDocument& other) const = default;

    private:
        std::optional<FieldFault> OffendingRow(std::size_t index) const;

        ScenarioFields fields;
        std::vector<UnderlyingRow> underlyings;
    };

    bool IsWellFormed(const ScenarioDocument& document);
    std::string Describing(const FieldFault& fault);
}
