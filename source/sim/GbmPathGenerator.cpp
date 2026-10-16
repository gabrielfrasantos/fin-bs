#include "source/sim/GbmPathGenerator.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "numerical/solvers/RungeKuttaIntegrators.hpp"
#include <cmath>
#include <vector>

namespace fin::sim
{
    namespace
    {
        class LogDriftSystem
            : public solvers::OdeSystem<double, 1, 0>
        {
        public:
            explicit LogDriftSystem(double perYear)
                : perYear(perYear)
            {}

            math::Vector<double, 1> Derivative(const math::Vector<double, 1>&, double) override
            {
                return math::Vector<double, 1>{ { perYear } };
            }

        private:
            double perYear;
        };
    }

    GbmPathGenerator::GbmPathGenerator(const GbmParameters& parameters)
        : parameters(parameters)
    {
        really_assert(parameters.initialPrice.Cents() > 0);
    }

    core::Rate GbmPathGenerator::LogDrift() const
    {
        return core::Rate::FromContinuous(parameters.drift.Continuous() - 0.5 * parameters.volatility.Variance());
    }

    estimation::PriceSeries GbmPathGenerator::Along(std::span<const core::Date> schedule, Rng& rng) const
    {
        really_assert(schedule.size() >= 2);

        LogDriftSystem drift(LogDrift().Continuous());

        std::vector<core::Date> dates(schedule.begin(), schedule.end());
        std::vector<core::Money> closes;
        closes.reserve(schedule.size());
        closes.push_back(parameters.initialPrice);

        math::Vector<double, 1> logPrice{ { std::log(parameters.initialPrice.ToDouble()) } };
        auto elapsed = 0.0;

        for (std::size_t step = 1; step != schedule.size(); ++step)
        {
            const auto interval = core::YearFraction(schedule[step - 1], schedule[step], parameters.convention);
            really_assert(interval > 0.0);

            solvers::RungeKutta4<double, 1, 0> integrator(drift, interval);
            logPrice = integrator.Step(logPrice, elapsed);
            logPrice.at(0, 0) += parameters.volatility.OverHorizon(interval) * rng.NextStandardNormal();
            elapsed += interval;

            closes.push_back(core::Money::FromAmount(std::exp(logPrice.at(0, 0)), parameters.initialPrice.GetCurrency()));
        }

        return estimation::PriceSeries::From(std::move(dates), std::move(closes), parameters.convention);
    }
}
