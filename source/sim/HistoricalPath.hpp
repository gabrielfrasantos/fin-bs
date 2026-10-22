#pragma once

#include "source/core/Date.hpp"
#include "source/estimation/PriceSeries.hpp"
#include <optional>
#include <span>
#include <string>

namespace fin::sim
{
    class HistoricalPath
    {
    public:
        HistoricalPath(const estimation::PriceSeries& history, std::span<const core::Date> schedule);

        const std::optional<estimation::PriceSeries>& Path() const;
        const std::string& Failure() const;

    private:
        std::optional<estimation::PriceSeries> path;
        std::string failure;
    };
}
