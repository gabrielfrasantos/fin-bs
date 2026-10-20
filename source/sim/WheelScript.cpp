#include "source/sim/WheelScript.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/Greeks.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace fin::sim
{
    namespace
    {
        constexpr auto convention = core::DayCountConvention::Act365Fixed;
        constexpr std::int32_t nonLeapYear = 2001;
        constexpr std::int64_t deepStrikeMultiple = 8;
        constexpr std::int32_t basisPointsPerUnit = 10000;

        pricing::OptionParameters Written(const core::Money& spot, const core::Money& strike, const WheelTerms& terms, core::OptionType type, double timeToExpiry)
        {
            return { .spot = spot, .strike = strike, .riskFreeRate = terms.riskFreeRate, .dividendYield = terms.dividendYield, .timeToExpiry = timeToExpiry, .type = type };
        }

        core::Money PriceOn(const estimation::PriceSeries& path, const core::Date& on)
        {
            const auto dates = path.Dates();
            const auto found = std::lower_bound(dates.begin(), dates.end(), on);

            really_assert(found != dates.end() && *found == on);

            return path.Closes()[static_cast<std::size_t>(found - dates.begin())];
        }
    }

    WheelScript::WheelScript(const WheelTerms& terms)
        : terms(terms)
    {
        really_assert(IsWellFormed(terms));
    }

    std::int32_t WheelScript::Cycles() const
    {
        return terms.cycles;
    }

    double WheelScript::Horizon() const
    {
        return core::YearFraction(terms.start, Expiry(terms.cycles - 1), convention);
    }

    std::vector<core::Date> WheelScript::Schedule() const
    {
        std::vector<core::Date> dates{ terms.start };

        for (std::int32_t cycle = 0; cycle != terms.cycles; ++cycle)
        {
            if (const auto observed = Observation(cycle))
                dates.push_back(*observed);

            if (const auto assessed = Assessment(cycle))
                dates.push_back(*assessed);

            dates.push_back(Expiry(cycle));
        }

        std::sort(dates.begin(), dates.end());
        dates.erase(std::unique(dates.begin(), dates.end()), dates.end());

        return dates;
    }

    strategy::ProjectionPeriod WheelScript::PeriodFor(std::int32_t cycle, strategy::WheelState state, const estimation::PriceSeries& path) const
    {
        really_assert(cycle >= 0 && cycle < terms.cycles);
        really_assert(state == strategy::WheelState::Cash || state == strategy::WheelState::HoldingShares);

        const auto opened = Opened(cycle);
        const auto expiry = Expiry(cycle);
        const auto type = state == strategy::WheelState::Cash ? core::OptionType::Put : core::OptionType::Call;
        const auto spot = PriceOn(path, opened);
        const auto timeToExpiry = core::YearFraction(opened, expiry, convention);
        const auto strike = StrikeFor(spot, type, timeToExpiry);

        strategy::ProjectionPeriod period{ .opened = opened,
            .expiry = expiry,
            .spotAtOpen = spot,
            .strike = strike,
            .premiumPerShare = pricing::BlackScholes(Written(spot, strike, terms, type, timeToExpiry), terms.impliedVolatility).Price(),
            .spotAtExpiry = PriceOn(path, expiry),
            .observed = std::nullopt,
            .assessed = std::nullopt,
            .dividendPerShare = spot.ToDouble() * std::expm1(terms.dividendYield.Continuous() * timeToExpiry),
            .dividendWithholdingBasisPoints = terms.dividendWithholdingBasisPoints };

        if (const auto on = type == core::OptionType::Put ? Observation(cycle) : std::nullopt)
        {
            const auto remaining = Written(PriceOn(path, *on), strike, terms, type, core::YearFraction(*on, expiry, convention));

            period.observed = strategy::RollObservation{ .on = *on,
                .valuePerShare = pricing::BlackScholes(remaining, terms.impliedVolatility).Price(),
                .delta = pricing::Greeks(remaining, terms.impliedVolatility).Delta() };
        }

        if (const auto on = Assessment(cycle))
            period.assessed = strategy::TaxObservation{ .on = *on, .spot = PriceOn(path, *on) };

        return period;
    }

    core::Date WheelScript::Opened(std::int32_t cycle) const
    {
        return terms.start.AddDays(cycle * terms.daysPerCycle);
    }

    core::Date WheelScript::Expiry(std::int32_t cycle) const
    {
        return Opened(cycle).AddDays(terms.daysPerCycle);
    }

    std::optional<core::Date> WheelScript::Observation(std::int32_t cycle) const
    {
        if (terms.observationDaysBeforeExpiry == 0)
            return std::nullopt;

        return Expiry(cycle).AddDays(-terms.observationDaysBeforeExpiry);
    }

    std::optional<core::Date> WheelScript::Assessment(std::int32_t cycle) const
    {
        if (!terms.assessment.has_value())
            return std::nullopt;

        const auto opened = Opened(cycle);
        const auto expiry = Expiry(cycle);

        for (auto year = opened.Year(); year <= expiry.Year(); ++year)
        {
            const auto on = core::Date::FromYearMonthDay(year, terms.assessment->month, terms.assessment->day);

            if (on > opened && on <= expiry)
                return on;
        }

        return std::nullopt;
    }

    core::Money WheelScript::StrikeFor(const core::Money& spot, core::OptionType type, double timeToExpiry) const
    {
        const auto increment = terms.strikeIncrement.Cents();
        const auto atSpot = spot.Cents() / increment;
        really_assert(atSpot >= 1);

        const auto withinTarget = [&](std::int64_t strikes)
        {
            const auto strike = core::Money::FromCents(strikes * increment, spot.GetCurrency());
            return pricing::Greeks(Written(spot, strike, terms, type, timeToExpiry), terms.impliedVolatility).Delta().Magnitude() <= terms.targetDelta.Magnitude();
        };

        const auto furthest = atSpot * deepStrikeMultiple;
        const auto put = type == core::OptionType::Put;

        really_assert(withinTarget(put ? 1 : furthest));
        really_assert(!withinTarget(put ? furthest : 1));

        auto inside = put ? 1 : furthest;
        auto outside = put ? furthest : 1;

        while (std::abs(inside - outside) > 1)
        {
            const auto middle = inside + (outside - inside) / 2;

            if (withinTarget(middle))
                inside = middle;
            else
                outside = middle;
        }

        return core::Money::FromCents(inside * increment, spot.GetCurrency());
    }

    bool IsWellFormed(const WheelTerms& terms)
    {
        return terms.cycles > 0 &&
               terms.daysPerCycle > 0 &&
               terms.daysPerCycle <= 365 &&
               terms.observationDaysBeforeExpiry >= 0 &&
               terms.observationDaysBeforeExpiry < terms.daysPerCycle &&
               terms.targetDelta.Magnitude() > 0.0 &&
               terms.targetDelta.Magnitude() < 1.0 &&
               terms.strikeIncrement.Cents() > 0 &&
               terms.dividendYield.Continuous() >= 0.0 &&
               terms.volatility.Annualized() > 0.0 &&
               terms.impliedVolatility.Annualized() > 0.0 &&
               terms.dividendWithholdingBasisPoints >= 0 &&
               terms.dividendWithholdingBasisPoints <= basisPointsPerUnit &&
               (!terms.assessment.has_value() || core::Date::IsValid(nonLeapYear, terms.assessment->month, terms.assessment->day));
    }
}
