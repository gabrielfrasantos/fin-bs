#pragma once

#include "source/core/OptionContract.hpp"
#include "source/core/Volatility.hpp"
#include "source/pricing/OptionParameters.hpp"
#include <cstdint>

namespace fin::pricing
{
    enum class AssignmentOutcome : uint8_t
    {
        NotAssigned,
        Assigned
    };

    class AssignmentModel
    {
    public:
        static AssignmentOutcome AtExpiry(const core::OptionContract& contract, const core::Money& spot);
        static AssignmentOutcome BeforeExDividend(const OptionParameters& parameters, const core::Volatility& volatility, const core::Money& dividendPerShare);
    };
}
