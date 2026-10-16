#pragma once

#include <cstdint>

namespace fin::costs
{
    enum class Instrument : std::uint8_t
    {
        Share,
        Option
    };

    class Trade
    {
    public:
        static Trade Shares(std::int64_t quantity);
        static Trade Options(std::int64_t contracts);

        Instrument Traded() const;
        std::int64_t Quantity() const;

        bool operator==(const Trade& other) const = default;

    private:
        Trade(Instrument instrument, std::int64_t quantity);

        Instrument instrument;
        std::int64_t quantity;
    };
}
