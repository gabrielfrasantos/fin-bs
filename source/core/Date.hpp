#pragma once

#include "infra/util/ReallyAssert.hpp"
#include <compare>
#include <cstdint>

namespace fin::core
{
    enum class Weekday : uint8_t
    {
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    };

    namespace detail
    {
        struct YearMonthDay
        {
            int32_t year;
            uint32_t month;
            uint32_t day;
        };

        constexpr int32_t DaysFromCivil(int32_t year, uint32_t month, uint32_t day)
        {
            const int32_t shiftedYear = year - (month <= 2 ? 1 : 0);
            const int32_t era = (shiftedYear >= 0 ? shiftedYear : shiftedYear - 399) / 400;
            const uint32_t yearOfEra = static_cast<uint32_t>(shiftedYear - era * 400);
            const uint32_t shiftedMonth = month > 2 ? month - 3 : month + 9;
            const uint32_t dayOfYear = (153u * shiftedMonth + 2u) / 5u + day - 1u;
            const uint32_t dayOfEra = yearOfEra * 365u + yearOfEra / 4u - yearOfEra / 100u + dayOfYear;

            return era * 146097 + static_cast<int32_t>(dayOfEra) - 719468;
        }

        constexpr YearMonthDay CivilFromDays(int32_t daysSinceEpoch)
        {
            const int32_t shifted = daysSinceEpoch + 719468;
            const int32_t era = (shifted >= 0 ? shifted : shifted - 146096) / 146097;
            const uint32_t dayOfEra = static_cast<uint32_t>(shifted - era * 146097);
            const uint32_t yearOfEra = (dayOfEra - dayOfEra / 1460u + dayOfEra / 36524u - dayOfEra / 146096u) / 365u;
            const uint32_t dayOfYear = dayOfEra - (365u * yearOfEra + yearOfEra / 4u - yearOfEra / 100u);
            const uint32_t shiftedMonth = (5u * dayOfYear + 2u) / 153u;
            const uint32_t day = dayOfYear - (153u * shiftedMonth + 2u) / 5u + 1u;
            const uint32_t month = shiftedMonth < 10u ? shiftedMonth + 3u : shiftedMonth - 9u;

            return { static_cast<int32_t>(yearOfEra) + era * 400 + (month <= 2 ? 1 : 0), month, day };
        }
    }

    class Date
    {
    public:
        static constexpr bool IsLeapYear(int32_t year)
        {
            return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        }

        static constexpr uint32_t DaysInMonth(int32_t year, uint32_t month)
        {
            really_assert(month >= 1 && month <= 12);

            constexpr uint32_t lengths[] = { 31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u };

            return month == 2 && IsLeapYear(year) ? 29u : lengths[month - 1];
        }

        static constexpr bool IsValid(int32_t year, uint32_t month, uint32_t day)
        {
            return month >= 1 && month <= 12 && day >= 1 && day <= DaysInMonth(year, month);
        }

        static constexpr Date FromYearMonthDay(int32_t year, uint32_t month, uint32_t day)
        {
            really_assert(IsValid(year, month, day));
            return Date(detail::DaysFromCivil(year, month, day));
        }

        static constexpr Date FromDaysSinceEpoch(int32_t days)
        {
            return Date(days);
        }

        constexpr int32_t Year() const
        {
            return detail::CivilFromDays(daysSinceEpoch).year;
        }

        constexpr uint32_t Month() const
        {
            return detail::CivilFromDays(daysSinceEpoch).month;
        }

        constexpr uint32_t Day() const
        {
            return detail::CivilFromDays(daysSinceEpoch).day;
        }

        constexpr int32_t DaysSinceEpoch() const
        {
            return daysSinceEpoch;
        }

        constexpr Weekday DayOfWeek() const
        {
            return static_cast<Weekday>(((daysSinceEpoch + 3) % 7 + 7) % 7);
        }

        constexpr bool IsWeekend() const
        {
            return DayOfWeek() >= Weekday::Saturday;
        }

        constexpr Date AddDays(int32_t days) const
        {
            return Date(daysSinceEpoch + days);
        }

        constexpr int32_t operator-(const Date& other) const
        {
            return daysSinceEpoch - other.daysSinceEpoch;
        }

        constexpr bool operator==(const Date& other) const = default;
        constexpr std::strong_ordering operator<=>(const Date& other) const = default;

    private:
        constexpr explicit Date(int32_t daysSinceEpoch)
            : daysSinceEpoch(daysSinceEpoch)
        {}

        int32_t daysSinceEpoch;
    };
}
