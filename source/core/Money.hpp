#pragma once

#include "infra/util/ReallyAssert.hpp"
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>

namespace fin::core
{
    enum class Currency : uint8_t
    {
        Eur,
        Usd
    };

    namespace detail
    {
        constexpr uint64_t Magnitude(int64_t value)
        {
            return value < 0 ? ~static_cast<uint64_t>(value) + 1u : static_cast<uint64_t>(value);
        }

        constexpr int64_t CheckedAdd(int64_t lhs, int64_t rhs)
        {
            really_assert(rhs <= 0 || lhs <= std::numeric_limits<int64_t>::max() - rhs);
            really_assert(rhs >= 0 || lhs >= std::numeric_limits<int64_t>::min() - rhs);
            return lhs + rhs;
        }

        constexpr int64_t CheckedSubtract(int64_t lhs, int64_t rhs)
        {
            really_assert(rhs >= 0 || lhs <= std::numeric_limits<int64_t>::max() + rhs);
            really_assert(rhs <= 0 || lhs >= std::numeric_limits<int64_t>::min() + rhs);
            return lhs - rhs;
        }

        constexpr int64_t CheckedMultiply(int64_t lhs, int64_t rhs)
        {
            really_assert(rhs == 0 || Magnitude(lhs) <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) / Magnitude(rhs));
            return lhs * rhs;
        }

        constexpr int64_t DivideRoundHalfToEven(int64_t numerator, int64_t denominator)
        {
            really_assert(denominator != 0);
            really_assert(numerator != std::numeric_limits<int64_t>::min() || denominator != -1);

            const int64_t quotient = numerator / denominator;
            const int64_t remainder = numerator % denominator;

            if (remainder == 0)
                return quotient;

            const uint64_t twiceRemainder = Magnitude(remainder) * 2u;
            const uint64_t absoluteDenominator = Magnitude(denominator);
            const int64_t step = (numerator < 0) != (denominator < 0) ? -1 : 1;

            if (twiceRemainder > absoluteDenominator)
                return quotient + step;
            if (twiceRemainder < absoluteDenominator)
                return quotient;

            return quotient % 2 == 0 ? quotient : quotient + step;
        }

        inline int64_t RoundHalfToEven(double value)
        {
            constexpr double int64Bound = 9223372036854775808.0;

            really_assert(std::isfinite(value));

            const double lower = std::floor(value);
            really_assert(lower >= -int64Bound && lower < int64Bound);

            const double fraction = value - lower;
            int64_t result = static_cast<int64_t>(lower);

            if (fraction > 0.5 || (fraction == 0.5 && result % 2 != 0))
                ++result;

            return result;
        }
    }

    class Money
    {
    public:
        static constexpr int64_t minorUnitsPerUnit = 100;

        static constexpr Money FromCents(int64_t cents, Currency currency)
        {
            return Money(cents, currency);
        }

        static Money FromAmount(double amount, Currency currency)
        {
            return Money(detail::RoundHalfToEven(amount * static_cast<double>(minorUnitsPerUnit)), currency);
        }

        static constexpr Money Zero(Currency currency)
        {
            return Money(0, currency);
        }

        constexpr int64_t Cents() const
        {
            return cents;
        }

        constexpr Currency GetCurrency() const
        {
            return currency;
        }

        constexpr double ToDouble() const
        {
            return static_cast<double>(cents) / static_cast<double>(minorUnitsPerUnit);
        }

        constexpr Money operator+(const Money& other) const
        {
            really_assert(currency == other.currency);
            return Money(detail::CheckedAdd(cents, other.cents), currency);
        }

        constexpr Money operator-(const Money& other) const
        {
            really_assert(currency == other.currency);
            return Money(detail::CheckedSubtract(cents, other.cents), currency);
        }

        constexpr Money operator-() const
        {
            really_assert(cents != std::numeric_limits<int64_t>::min());
            return Money(-cents, currency);
        }

        constexpr Money& operator+=(const Money& other)
        {
            *this = *this + other;
            return *this;
        }

        constexpr Money& operator-=(const Money& other)
        {
            *this = *this - other;
            return *this;
        }

        template<std::integral T>
        constexpr Money operator*(T quantity) const
        {
            return Money(detail::CheckedMultiply(cents, static_cast<int64_t>(quantity)), currency);
        }

        template<std::floating_point T>
        Money operator*(T factor) const
        {
            return Money(detail::RoundHalfToEven(static_cast<double>(cents) * static_cast<double>(factor)), currency);
        }

        template<std::integral T>
        constexpr Money operator/(T divisor) const
        {
            return Money(detail::DivideRoundHalfToEven(cents, static_cast<int64_t>(divisor)), currency);
        }

        constexpr bool operator==(const Money& other) const
        {
            really_assert(currency == other.currency);
            return cents == other.cents;
        }

        constexpr std::strong_ordering operator<=>(const Money& other) const
        {
            really_assert(currency == other.currency);
            return cents <=> other.cents;
        }

    private:
        constexpr Money(int64_t cents, Currency currency)
            : cents(cents)
            , currency(currency)
        {}

        int64_t cents;
        Currency currency;
    };
}
