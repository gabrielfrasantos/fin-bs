#include "source/sim/HistoricalPath.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <vector>

namespace fin::sim
{
    namespace
    {
        constexpr auto convention = core::DayCountConvention::Act365Fixed;

        std::string Padded(std::uint32_t part)
        {
            return part < 10 ? "0" + std::to_string(part) : std::to_string(part);
        }

        std::string TextOf(const core::Date& date)
        {
            return std::to_string(date.Year()) + "-" + Padded(date.Month()) + "-" + Padded(date.Day());
        }
    }

    HistoricalPath::HistoricalPath(const estimation::PriceSeries& history, std::span<const core::Date> schedule)
    {
        really_assert(schedule.size() >= 2);
        really_assert(std::is_sorted(schedule.begin(), schedule.end()));
        really_assert(std::adjacent_find(schedule.begin(), schedule.end()) == schedule.end());

        const auto dates = history.Dates();
        const auto closes = history.Closes();

        if (schedule.front() < dates.front())
        {
            failure = "The history begins on " + TextOf(dates.front()) + ", after the schedule opens on " + TextOf(schedule.front());
            return;
        }

        if (schedule.back() > dates.back())
        {
            failure = "The history ends on " + TextOf(dates.back()) + ", before the schedule reaches " + TextOf(schedule.back());
            return;
        }

        std::vector<core::Money> replayed;
        replayed.reserve(schedule.size());

        for (const auto& on : schedule)
        {
            const auto after = std::upper_bound(dates.begin(), dates.end(), on);
            replayed.push_back(closes[static_cast<std::size_t>(after - dates.begin()) - 1]);
        }

        path = estimation::PriceSeries::From(std::vector<core::Date>(schedule.begin(), schedule.end()), std::move(replayed), convention);
    }

    const std::optional<estimation::PriceSeries>& HistoricalPath::Path() const
    {
        return path;
    }

    const std::string& HistoricalPath::Failure() const
    {
        return failure;
    }
}
