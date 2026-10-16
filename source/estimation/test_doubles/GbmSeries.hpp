#pragma once

#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include <cstddef>
#include <cstdint>
#include <random>

namespace fin::estimation::test_doubles
{
    class GbmSeries
    {
    public:
        struct Parameters
        {
            core::Date start;
            core::Money initialPrice;
            core::Rate drift;
            core::Volatility volatility;
        };

        static constexpr double periodsPerYear = 252.0;

        GbmSeries(const Parameters& parameters, uint64_t seed);

        PriceSeries Generate(std::size_t observations);
        PriceSeries GenerateWithRegimeSwitch(std::size_t observations, const core::Volatility& switched);

    private:
        double NextStandardNormal();
        core::Date NextBusinessDay(const core::Date& date) const;

        Parameters parameters;
        std::mt19937_64 engine;
        double spare{};
        bool spareAvailable{ false };
    };
}
