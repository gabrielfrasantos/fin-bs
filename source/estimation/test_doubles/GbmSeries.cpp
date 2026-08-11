#include "source/estimation/test_doubles/GbmSeries.hpp"
#include <cmath>
#include <numbers>
#include <vector>

namespace fin::estimation::test_doubles
{
    namespace
    {
        constexpr double timeStep = 1.0 / GbmSeries::periodsPerYear;
    }

    GbmSeries::GbmSeries(const Parameters& parameters, uint64_t seed)
        : parameters(parameters)
        , engine(seed)
    {}

    PriceSeries GbmSeries::Generate(std::size_t observations)
    {
        return GenerateWithRegimeSwitch(observations, parameters.volatility);
    }

    PriceSeries GbmSeries::GenerateWithRegimeSwitch(std::size_t observations, const core::Volatility& switched)
    {
        std::vector<core::Date> dates;
        std::vector<core::Money> closes;
        dates.reserve(observations);
        closes.reserve(observations);

        auto date = parameters.start;
        while (date.IsWeekend())
            date = date.AddDays(1);

        auto logPrice = std::log(parameters.initialPrice.ToDouble());

        for (std::size_t i = 0; i != observations; ++i)
        {
            if (i != 0)
            {
                const auto& volatility = i <= observations / 2 ? parameters.volatility : switched;
                const auto deviation = volatility.OverHorizon(timeStep);

                logPrice += (parameters.drift.Continuous() - 0.5 * volatility.Variance()) * timeStep + deviation * NextStandardNormal();
                date = NextBusinessDay(date);
            }

            dates.push_back(date);
            closes.push_back(core::Money::FromAmount(std::exp(logPrice), parameters.initialPrice.GetCurrency()));
        }

        return PriceSeries::From(std::move(dates), std::move(closes), core::DayCountConvention::Act252);
    }

    double GbmSeries::NextStandardNormal()
    {
        if (spareAvailable)
        {
            spareAvailable = false;
            return spare;
        }

        constexpr double scale = 0x1.0p-53;

        const auto uniform = static_cast<double>((engine() >> 11) + 1u) * scale;
        const auto angle = static_cast<double>(engine() >> 11) * scale * 2.0 * std::numbers::pi;
        const auto radius = std::sqrt(-2.0 * std::log(uniform));

        spare = radius * std::sin(angle);
        spareAvailable = true;

        return radius * std::cos(angle);
    }

    core::Date GbmSeries::NextBusinessDay(const core::Date& date) const
    {
        auto next = date.AddDays(1);
        while (next.IsWeekend())
            next = next.AddDays(1);

        return next;
    }
}
