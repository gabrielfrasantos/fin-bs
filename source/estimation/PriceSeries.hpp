#pragma once

#include "source/core/Date.hpp"
#include "source/core/DayCount.hpp"
#include "source/core/Money.hpp"
#include <cstddef>
#include <span>
#include <vector>

namespace fin::estimation
{
    class PriceSeries
    {
    public:
        static PriceSeries From(std::vector<core::Date> dates, std::vector<core::Money> closes, core::DayCountConvention convention);

        std::size_t Size() const;
        core::DayCountConvention Convention() const;
        core::Currency GetCurrency() const;
        std::span<const core::Date> Dates() const;
        std::span<const core::Money> Closes() const;

        double Horizon() const;
        double PeriodsPerYear() const;
        std::vector<double> Times() const;
        std::vector<double> LogPrices() const;
        std::vector<double> LogReturns() const;

    private:
        PriceSeries(std::vector<core::Date>&& dates, std::vector<core::Money>&& closes, core::DayCountConvention convention);

        std::vector<core::Date> dates;
        std::vector<core::Money> closes;
        core::DayCountConvention convention;
    };
}
