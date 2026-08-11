#pragma once

#include "infra/util/ReallyAssert.hpp"
#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include <compare>
#include <cstdint>
#include <string>
#include <utility>

namespace fin::core
{
    enum class OptionType : uint8_t
    {
        Call,
        Put
    };

    class OptionContract
    {
    public:
        static constexpr uint32_t standardMultiplier = 100;

        OptionContract(std::string underlying, const Date& expiry, const Money& strike, OptionType type, uint32_t multiplier = standardMultiplier)
            : underlying(std::move(underlying))
            , expiry(expiry)
            , strike(strike)
            , type(type)
            , multiplier(multiplier)
        {
            really_assert(!this->underlying.empty());
            really_assert(strike.Cents() > 0);
            really_assert(multiplier > 0);
        }

        const std::string& Underlying() const
        {
            return underlying;
        }

        const Date& Expiry() const
        {
            return expiry;
        }

        const Money& Strike() const
        {
            return strike;
        }

        OptionType Type() const
        {
            return type;
        }

        uint32_t Multiplier() const
        {
            return multiplier;
        }

        Money Notional() const
        {
            return strike * static_cast<int64_t>(multiplier);
        }

        bool operator==(const OptionContract& other) const = default;
        std::strong_ordering operator<=>(const OptionContract& other) const = default;

    private:
        std::string underlying;
        Date expiry;
        Money strike;
        OptionType type;
        uint32_t multiplier;
    };
}
