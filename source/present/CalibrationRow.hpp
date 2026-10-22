#pragma once

#include "source/sim/Calibration.hpp"
#include <cstdint>
#include <string>

namespace fin::present
{
    struct CalibrationRow
    {
        std::int32_t cycle;
        std::string underlying;
        sim::CalibratedEstimate estimate;

        bool operator==(const CalibrationRow& other) const = default;
    };
}
