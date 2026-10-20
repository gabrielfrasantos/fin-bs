#pragma once

#include "source/core/Date.hpp"
#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/sim/BookScript.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace fin::sim
{
    enum class VolatilityEstimator : std::uint8_t
    {
        historical,
        exponentiallyWeighted
    };

    enum class DriftEstimator : std::uint8_t
    {
        leastSquares,
        stated
    };

    struct CalibrationRule
    {
        std::int32_t windowObservations;
        VolatilityEstimator volatility;
        std::int32_t halfLifeObservations;
        DriftEstimator drift;
        std::int32_t impliedPremiumBasisPoints;

        bool operator==(const CalibrationRule& other) const = default;
    };

    struct CalibratedEstimate
    {
        core::Date windowEnd;
        std::int32_t observations;
        core::Rate drift;
        core::Volatility volatility;
        core::Volatility impliedVolatility;

        bool operator==(const CalibratedEstimate& other) const = default;
    };

    class Calibrator
    {
    public:
        Calibrator(const CalibrationRule& rule, const estimation::PriceSeries& history, const core::Date& on, const UnderlyingTerms& stated);

        const std::optional<CalibratedEstimate>& Estimate() const;
        const std::string& Failure() const;

    private:
        std::optional<CalibratedEstimate> estimate;
        std::string failure;
    };

    bool IsWellFormed(const CalibrationRule& rule);
    UnderlyingTerms Calibrated(const UnderlyingTerms& stated, const CalibratedEstimate& estimate);

    constexpr std::int32_t leastWindowObservations = 20;
    constexpr std::int32_t mostWindowObservations = 5000;
}
