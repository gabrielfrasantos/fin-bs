#include "source/costs/Trade.hpp"
#include "infra/util/ReallyAssert.hpp"

namespace fin::costs
{
    Trade Trade::Shares(std::int64_t quantity)
    {
        return Trade(Instrument::Share, quantity);
    }

    Trade Trade::Options(std::int64_t contracts)
    {
        return Trade(Instrument::Option, contracts);
    }

    Instrument Trade::Traded() const
    {
        return instrument;
    }

    std::int64_t Trade::Quantity() const
    {
        return quantity;
    }

    Trade::Trade(Instrument instrument, std::int64_t quantity)
        : instrument(instrument)
        , quantity(quantity)
    {
        really_assert(quantity > 0);
    }
}
