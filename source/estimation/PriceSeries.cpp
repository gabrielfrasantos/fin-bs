#include "source/estimation/PriceSeries.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/estimation/LogReturns.hpp"

namespace fin::estimation
{
    PriceSeries PriceSeries::From(std::vector<core::Date> dates, std::vector<core::Money> closes, core::DayCountConvention convention)
    {
        really_assert(dates.size() == closes.size());
        really_assert(dates.size() >= 2);

        for (std::size_t i = 1; i != dates.size(); ++i)
            really_assert(dates[i - 1] < dates[i]);

        for (const auto& close : closes)
        {
            really_assert(close.GetCurrency() == closes.front().GetCurrency());
            really_assert(close.Cents() > 0);
        }

        really_assert(core::YearFraction(dates.front(), dates.back(), convention) > 0.0);

        return PriceSeries(std::move(dates), std::move(closes), convention);
    }

    std::size_t PriceSeries::Size() const
    {
        return closes.size();
    }

    core::DayCountConvention PriceSeries::Convention() const
    {
        return convention;
    }

    core::Currency PriceSeries::GetCurrency() const
    {
        return closes.front().GetCurrency();
    }

    std::span<const core::Date> PriceSeries::Dates() const
    {
        return dates;
    }

    std::span<const core::Money> PriceSeries::Closes() const
    {
        return closes;
    }

    double PriceSeries::Horizon() const
    {
        return core::YearFraction(dates.front(), dates.back(), convention);
    }

    double PriceSeries::PeriodsPerYear() const
    {
        return static_cast<double>(Size() - 1) / Horizon();
    }

    std::vector<double> PriceSeries::Times() const
    {
        std::vector<double> result;
        result.reserve(dates.size());

        for (const auto& date : dates)
            result.push_back(core::YearFraction(dates.front(), date, convention));

        return result;
    }

    std::vector<double> PriceSeries::LogPrices() const
    {
        return estimation::LogPrices(closes);
    }

    std::vector<double> PriceSeries::LogReturns() const
    {
        return estimation::LogReturns(closes);
    }

    PriceSeries::PriceSeries(std::vector<core::Date>&& dates, std::vector<core::Money>&& closes, core::DayCountConvention convention)
        : dates(std::move(dates))
        , closes(std::move(closes))
        , convention(convention)
    {}
}
