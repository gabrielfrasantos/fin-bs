#pragma once

#include "infra/util/ReallyAssert.hpp"
#include "source/core/Date.hpp"
#include <cstdint>

namespace fin::core
{
    enum class DayCountConvention : uint8_t
    {
        Act365Fixed,
        Act252
    };

    constexpr int32_t BusinessDaysBetween(const Date& start, const Date& end)
    {
        really_assert(start <= end);

        const int32_t totalDays = end - start;
        const int32_t wholeWeeks = totalDays / 7;

        int32_t businessDays = wholeWeeks * 5;

        for (Date day = start.AddDays(wholeWeeks * 7); day < end; day = day.AddDays(1))
            if (!day.IsWeekend())
                ++businessDays;

        return businessDays;
    }

    constexpr double YearFraction(const Date& start, const Date& end, DayCountConvention convention)
    {
        constexpr double daysPerYearFixed = 365.0;
        constexpr double businessDaysPerYear = 252.0;

        really_assert(start <= end);

        switch (convention)
        {
            case DayCountConvention::Act365Fixed:
                return static_cast<double>(end - start) / daysPerYearFixed;
            case DayCountConvention::Act252:
                return static_cast<double>(BusinessDaysBetween(start, end)) / businessDaysPerYear;
        }

        really_assert(false);
        return 0.0;
    }
}
