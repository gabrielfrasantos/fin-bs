#pragma once

#include "source/core/Money.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fin::present
{
    struct PriceEstimate
    {
        std::int32_t volatilityBasisPoints;
        std::int32_t driftBasisPoints;

        bool operator==(const PriceEstimate& other) const = default;
    };

    class PriceHistory
    {
    public:
        PriceHistory(const std::vector<std::string>& document, core::Currency currency, std::int32_t dividendYieldBasisPoints);

        const std::optional<PriceEstimate>& Estimate() const;
        const std::string& Failure() const;

    private:
        std::optional<PriceEstimate> estimate;
        std::string failure;
    };
}
