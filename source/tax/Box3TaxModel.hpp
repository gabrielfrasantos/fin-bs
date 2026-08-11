#pragma once

#include "source/core/Money.hpp"
#include "source/tax/Box3Parameters.hpp"
#include <cstdint>

namespace fin::tax
{
    struct Box3Assets
    {
        core::Money deposits;
        core::Money investments;
    };

    core::Money TotalValue(const Box3Assets& assets);

    class Box3TaxModel
    {
    public:
        explicit Box3TaxModel(const Box3Parameters& parameters);

        std::int32_t Year() const;
        core::Money DeemedReturn(const Box3Assets& assets) const;
        core::Money TaxableBase(const Box3Assets& assets) const;
        core::Money TaxableReturn(const Box3Assets& assets) const;
        core::Money Charge(const Box3Assets& assets) const;

    private:
        Box3Parameters parameters;
    };
}
