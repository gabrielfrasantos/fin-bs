#pragma once

#include "source/core/Money.hpp"
#include <cstdint>

namespace fin::costs
{
    class FxModel
    {
    public:
        static FxModel From(core::Currency foreign, double foreignPerBase, std::int32_t handlingBasisPoints);

        core::Currency Base() const;
        core::Currency Foreign() const;

        core::Money AtSpot(const core::Money& amount) const;
        core::Money HandlingCost(const core::Money& amount) const;
        core::Money Convert(const core::Money& amount) const;

    private:
        FxModel(core::Currency foreign, double foreignPerBase, std::int32_t handlingBasisPoints);

        core::Currency foreign;
        double foreignPerBase;
        std::int32_t handlingBasisPoints;
    };
}
