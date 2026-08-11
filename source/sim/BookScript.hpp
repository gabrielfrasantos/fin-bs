#pragma once

#include "source/core/Date.hpp"
#include "source/core/Delta.hpp"
#include "source/core/Money.hpp"
#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/sim/WheelScript.hpp"
#include "source/strategy/Book.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fin::sim
{
    struct UnderlyingTerms
    {
        std::string underlying;
        core::Money initialPrice;
        core::Rate drift;
        core::Volatility volatility;
        core::Rate dividendYield;
        core::Money strikeIncrement;

        bool operator==(const UnderlyingTerms& other) const = default;
    };

    struct BookTerms
    {
        core::Date start;
        std::int32_t cycles;
        std::int32_t daysPerCycle;
        std::int32_t observationDaysBeforeExpiry;
        core::Delta targetDelta;
        core::Rate riskFreeRate;
        std::optional<AnnualReference> assessment;
        std::vector<UnderlyingTerms> underlyings;

        bool operator==(const BookTerms& other) const = default;
    };

    class BookScript
    {
    public:
        explicit BookScript(const BookTerms& terms);

        std::int32_t Cycles() const;
        double Horizon() const;
        std::vector<core::Date> Schedule() const;
        std::vector<std::string> Underlyings() const;

        std::vector<strategy::BookPeriod> CycleFor(std::int32_t cycle, const strategy::Book& book, std::span<const estimation::PriceSeries> paths) const;
        std::optional<strategy::BookAssessment> AssessmentBefore(std::int32_t cycle, std::span<const estimation::PriceSeries> paths) const;

    private:
        core::Date Opened(std::int32_t cycle) const;
        std::optional<core::Date> ReferenceDateWithin(std::int32_t cycle) const;

        BookTerms terms;
        std::vector<WheelScript> wheels;
    };

    bool IsWellFormed(const BookTerms& terms);
}
