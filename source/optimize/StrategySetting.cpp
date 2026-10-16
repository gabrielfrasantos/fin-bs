#include "source/optimize/StrategySetting.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <cmath>

namespace fin::optimize
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

        double Between(double lowest, double highest, double fraction)
        {
            return lowest + fraction * (highest - lowest);
        }

        double FractionOf(double lowest, double highest, double value)
        {
            return std::clamp((value - lowest) / (highest - lowest), 0.0, 1.0);
        }
    }

    bool IsWellFormed(const StrategySetting& setting)
    {
        return setting.targetDelta.Value() > 0.0 &&
               setting.targetDelta.Value() < 1.0 &&
               setting.cashBufferBasisPoints >= 0 &&
               setting.cashBufferBasisPoints <= basisPointsPerUnit &&
               setting.rollDeltaLimit.Value() > 0.0;
    }

    bool IsWellFormed(const SettingRange& range)
    {
        return IsWellFormed(range.lowest) &&
               IsWellFormed(range.highest) &&
               range.lowest.targetDelta < range.highest.targetDelta &&
               range.lowest.cashBufferBasisPoints < range.highest.cashBufferBasisPoints &&
               range.lowest.rollDeltaLimit < range.highest.rollDeltaLimit;
    }

    StrategySetting SettingAt(const SettingRange& range, const Point& at)
    {
        really_assert(IsWellFormed(range));

        const auto within = ClampedToTheUnitBox(at);

        return { .targetDelta = core::Delta::FromValue(Between(range.lowest.targetDelta.Value(), range.highest.targetDelta.Value(), within[0])),
            .cashBufferBasisPoints = static_cast<std::int32_t>(std::llround(Between(range.lowest.cashBufferBasisPoints, range.highest.cashBufferBasisPoints, within[1]))),
            .rollDeltaLimit = core::Delta::FromValue(Between(range.lowest.rollDeltaLimit.Value(), range.highest.rollDeltaLimit.Value(), within[2])) };
    }

    Point PointOf(const SettingRange& range, const StrategySetting& setting)
    {
        really_assert(IsWellFormed(range));
        really_assert(IsWellFormed(setting));

        return { FractionOf(range.lowest.targetDelta.Value(), range.highest.targetDelta.Value(), setting.targetDelta.Value()),
            FractionOf(range.lowest.cashBufferBasisPoints, range.highest.cashBufferBasisPoints, setting.cashBufferBasisPoints),
            FractionOf(range.lowest.rollDeltaLimit.Value(), range.highest.rollDeltaLimit.Value(), setting.rollDeltaLimit.Value()) };
    }
}
