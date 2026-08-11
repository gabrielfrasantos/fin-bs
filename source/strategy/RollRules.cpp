#include "source/strategy/RollRules.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <cmath>

namespace fin::strategy
{
    namespace
    {
        constexpr double basisPointsPerUnit = 10000.0;
    }

    RollRules::RollRules(const RollTriggers& triggers)
        : triggers(triggers)
    {
        really_assert(IsWellFormed(triggers));
    }

    std::optional<RollReason> RollRules::Triggered(const PositionObservation& observation) const
    {
        really_assert(observation.daysToExpiry >= 0);
        really_assert(observation.premiumReceivedPerShare > 0.0);
        really_assert(observation.valuePerShare >= 0.0);

        if (observation.delta.Magnitude() >= triggers.deltaLimit.Magnitude())
            return RollReason::DeltaBreached;

        if (CapturedBasisPoints(observation) >= triggers.profitCaptureBasisPoints)
            return RollReason::ProfitCaptured;

        if (observation.daysToExpiry <= triggers.daysToExpiry)
            return RollReason::NearExpiry;

        return std::nullopt;
    }

    bool RollRules::Allows(const core::Delta& replacement) const
    {
        return replacement.Magnitude() <= triggers.deltaLimit.Magnitude();
    }

    bool IsWellFormed(const RollTriggers& triggers)
    {
        return triggers.daysToExpiry >= 0 &&
               triggers.profitCaptureBasisPoints > 0 &&
               triggers.profitCaptureBasisPoints <= static_cast<std::int32_t>(basisPointsPerUnit) &&
               triggers.deltaLimit.Magnitude() > 0.0;
    }

    std::int32_t CapturedBasisPoints(const PositionObservation& observation)
    {
        really_assert(observation.premiumReceivedPerShare > 0.0);

        const auto captured = 1.0 - observation.valuePerShare / observation.premiumReceivedPerShare;

        return static_cast<std::int32_t>(std::floor(std::clamp(captured, -1.0, 1.0) * basisPointsPerUnit));
    }
}
