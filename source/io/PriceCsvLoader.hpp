#pragma once

#include "source/core/DayCount.hpp"
#include "source/core/Money.hpp"
#include "source/estimation/PriceSeries.hpp"
#include <optional>
#include <string>
#include <vector>

namespace fin::io
{
    class PriceCsvLoader
    {
    public:
        PriceCsvLoader(const std::vector<std::string>& document, core::Currency currency, core::DayCountConvention convention);

        const std::optional<estimation::PriceSeries>& Series() const;
        const std::string& Failure() const;

    private:
        std::optional<estimation::PriceSeries> series;
        std::string failure;
    };
}
