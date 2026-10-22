#pragma once

#include "source/present/RunController.hpp"
#include "source/sim/GbmPathGenerator.hpp"

namespace fin::present::test
{
    inline core::Money Euro(double amount)
    {
        return core::Money::FromAmount(amount, core::Currency::Eur);
    }

    inline costs::FeeSchedule Fees()
    {
        return costs::FeeSchedule{ .optionFeePerContract = Euro(0.75),
            .shareCommissionPerTrade = Euro(2.00),
            .handlingFeePerTrade = Euro(1.00),
            .connectivityFeeCapPerExchangePerYear = Euro(2.50),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25,
            .debitInterestBasisPoints = 690 };
    }

    inline tax::Box3Parameters Taxes()
    {
        return tax::Box3Parameters{ .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 };
    }

    inline sim::UnderlyingTerms Underlying(std::string ticker, double spot, double volatility)
    {
        return sim::UnderlyingTerms{ .underlying = std::move(ticker),
            .initialPrice = Euro(spot),
            .drift = core::Rate::FromContinuous(0.06),
            .volatility = core::Volatility::FromAnnualized(volatility),
            .impliedVolatility = core::Volatility::FromAnnualized(volatility),
            .dividendYield = core::Rate::FromContinuous(0.02),
            .dividendWithholdingBasisPoints = 1500,
            .strikeIncrement = Euro(1.00) };
    }

    inline io::Scenario TwoNames()
    {
        return io::Scenario{ .name = "Two names over two cycles",
            .capital = Euro(200000.00),
            .cashBufferBasisPoints = 500,
            .maxCollateralPerNameBasisPoints = 10000,
            .triggers = strategy::RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = core::Delta::FromValue(0.50) },
            .terms = sim::BookTerms{ .start = core::Date::FromYearMonthDay(2026, 1, 5),
                .cycles = 2,
                .daysPerCycle = 182,
                .observationDaysBeforeExpiry = 14,
                .targetDelta = core::Delta::FromValue(0.30),
                .riskFreeRate = core::Rate::FromContinuous(0.025),
                .assessment = sim::AnnualReference{ 1, 1 },
                .underlyings = { Underlying("ACME", 95.00, 0.22), Underlying("BETA", 32.00, 0.26) } },
            .paths = 8,
            .seed = 20250811,
            .taxConfiguration = "tax/nl_2025.json",
            .brokerConfiguration = "broker/degiro.json" };
    }

    inline RunInputs Inputs()
    {
        const auto scenario = TwoNames();

        return RunInputs{ .scenario = scenario, .fees = Fees(), .taxes = Taxes(), .search = SearchAround(scenario, 4000, 5000, 1), .histories = {} };
    }

    inline std::vector<core::Date> BusinessDays(const core::Date& start, std::size_t observations)
    {
        std::vector<core::Date> dates;
        auto date = start;

        while (dates.size() != observations)
        {
            if (!date.IsWeekend())
                dates.push_back(date);

            date = date.AddDays(1);
        }

        return dates;
    }

    inline estimation::PriceSeries History(const sim::UnderlyingTerms& underlying, std::uint64_t seed)
    {
        sim::Rng rng(seed);
        const sim::GbmPathGenerator generator(sim::GbmParameters{ .initialPrice = underlying.initialPrice,
            .drift = underlying.drift,
            .volatility = underlying.volatility,
            .convention = core::DayCountConvention::Act252 });

        return generator.Along(BusinessDays(core::Date::FromYearMonthDay(2023, 1, 2), 1050), rng);
    }

    inline io::Calibration Rule()
    {
        return io::Calibration{ .priceHistories = "../data/prices",
            .rule = sim::CalibrationRule{ .windowObservations = 252,
                .volatility = sim::VolatilityEstimator::historical,
                .halfLifeObservations = 60,
                .drift = sim::DriftEstimator::leastSquares,
                .impliedPremiumBasisPoints = 200 } };
    }

    inline RunInputs Calibrated()
    {
        auto inputs = Inputs();
        inputs.scenario.calibration = Rule();

        std::uint64_t seed = 1;

        for (const auto& underlying : inputs.scenario.terms.underlyings)
            inputs.histories.push_back(NamedHistory{ underlying.underlying, History(underlying, seed++) });

        return inputs;
    }
}
