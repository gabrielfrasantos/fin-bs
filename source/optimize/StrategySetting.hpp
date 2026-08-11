#pragma once

#include "source/core/Delta.hpp"
#include "source/optimize/Objective.hpp"
#include <cstdint>

namespace fin::optimize
{
    struct StrategySetting
    {
        core::Delta targetDelta;
        std::int32_t cashBufferBasisPoints;
        core::Delta rollDeltaLimit;
    };

    struct SettingRange
    {
        StrategySetting lowest;
        StrategySetting highest;
    };

    bool IsWellFormed(const StrategySetting& setting);
    bool IsWellFormed(const SettingRange& range);
    StrategySetting SettingAt(const SettingRange& range, const Point& at);
    Point PointOf(const SettingRange& range, const StrategySetting& setting);
}
