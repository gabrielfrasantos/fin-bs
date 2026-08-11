#include "source/sim/BookScript.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::sim
{
    namespace
    {
        constexpr std::int32_t nonLeapYear = 2001;

        WheelTerms WheelFor(const BookTerms& terms, const UnderlyingTerms& underlying)
        {
            return { .start = terms.start,
                .cycles = terms.cycles,
                .daysPerCycle = terms.daysPerCycle,
                .observationDaysBeforeExpiry = terms.observationDaysBeforeExpiry,
                .targetDelta = terms.targetDelta,
                .strikeIncrement = underlying.strikeIncrement,
                .riskFreeRate = terms.riskFreeRate,
                .dividendYield = underlying.dividendYield,
                .volatility = underlying.volatility,
                .assessment = std::nullopt };
        }

        std::vector<WheelScript> WheelsFor(const BookTerms& terms)
        {
            std::vector<WheelScript> wheels;
            wheels.reserve(terms.underlyings.size());

            for (const auto& underlying : terms.underlyings)
                wheels.emplace_back(WheelFor(terms, underlying));

            return wheels;
        }

        core::Money PriceOn(const estimation::PriceSeries& path, const core::Date& on)
        {
            const auto dates = path.Dates();
            const auto found = std::lower_bound(dates.begin(), dates.end(), on);

            really_assert(found != dates.end() && *found == on);

            return path.Closes()[static_cast<std::size_t>(found - dates.begin())];
        }
    }

    BookScript::BookScript(const BookTerms& terms)
        : terms(terms)
        , wheels(WheelsFor(terms))
    {
        really_assert(IsWellFormed(terms));
    }

    std::int32_t BookScript::Cycles() const
    {
        return terms.cycles;
    }

    double BookScript::Horizon() const
    {
        return wheels.front().Horizon();
    }

    std::vector<core::Date> BookScript::Schedule() const
    {
        auto dates = wheels.front().Schedule();

        for (std::int32_t cycle = 0; cycle != terms.cycles; ++cycle)
            if (const auto on = ReferenceDateWithin(cycle))
                dates.push_back(*on);

        std::sort(dates.begin(), dates.end());
        dates.erase(std::unique(dates.begin(), dates.end()), dates.end());

        return dates;
    }

    std::vector<std::string> BookScript::Underlyings() const
    {
        std::vector<std::string> names;
        names.reserve(terms.underlyings.size());

        for (const auto& underlying : terms.underlyings)
            names.push_back(underlying.underlying);

        return names;
    }

    std::vector<strategy::BookPeriod> BookScript::CycleFor(std::int32_t cycle, const strategy::Book& book, std::span<const estimation::PriceSeries> paths) const
    {
        really_assert(paths.size() == wheels.size());

        std::vector<strategy::BookPeriod> periods;
        periods.reserve(wheels.size());

        for (std::size_t wheel = 0; wheel != wheels.size(); ++wheel)
        {
            const auto& underlying = terms.underlyings[wheel].underlying;

            periods.push_back({ underlying, wheels[wheel].PeriodFor(cycle, book.StateOf(underlying), paths[wheel]) });
        }

        return periods;
    }

    std::optional<strategy::BookAssessment> BookScript::AssessmentBefore(std::int32_t cycle, std::span<const estimation::PriceSeries> paths) const
    {
        really_assert(paths.size() == wheels.size());

        const auto on = ReferenceDateWithin(cycle);

        if (!on.has_value())
            return std::nullopt;

        std::vector<strategy::SpotPrice> prices;
        prices.reserve(wheels.size());

        for (std::size_t wheel = 0; wheel != wheels.size(); ++wheel)
            prices.push_back({ terms.underlyings[wheel].underlying, PriceOn(paths[wheel], *on) });

        return strategy::BookAssessment{ *on, std::move(prices) };
    }

    core::Date BookScript::Opened(std::int32_t cycle) const
    {
        return terms.start.AddDays(cycle * terms.daysPerCycle);
    }

    std::optional<core::Date> BookScript::ReferenceDateWithin(std::int32_t cycle) const
    {
        if (!terms.assessment.has_value())
            return std::nullopt;

        const auto opened = Opened(cycle);
        const auto expiry = Opened(cycle + 1);

        for (auto year = opened.Year(); year <= expiry.Year(); ++year)
        {
            const auto on = core::Date::FromYearMonthDay(year, terms.assessment->month, terms.assessment->day);

            if (on > opened && on <= expiry)
                return on;
        }

        return std::nullopt;
    }

    bool IsWellFormed(const BookTerms& terms)
    {
        if (terms.underlyings.empty())
            return false;

        for (const auto& underlying : terms.underlyings)
        {
            if (underlying.underlying.empty() || underlying.initialPrice.Cents() <= 0)
                return false;

            if (std::count_if(terms.underlyings.begin(), terms.underlyings.end(), [&underlying](const UnderlyingTerms& other)
                    {
                        return other.underlying == underlying.underlying;
                    }) != 1)
                return false;

            if (!IsWellFormed(WheelFor(terms, underlying)))
                return false;
        }

        return !terms.assessment.has_value() || core::Date::IsValid(nonLeapYear, terms.assessment->month, terms.assessment->day);
    }
}
