#pragma once

#include "source/optimize/StrategyObjective.hpp"

namespace fin::optimize::test
{
    inline core::Money Euro(double amount)
    {
        return core::Money::FromAmount(amount, core::Currency::Eur);
    }

    inline StrategySetting Setting(double targetDelta, std::int32_t cashBufferBasisPoints, double rollDeltaLimit)
    {
        return { .targetDelta = core::Delta::FromValue(targetDelta), .cashBufferBasisPoints = cashBufferBasisPoints, .rollDeltaLimit = core::Delta::FromValue(rollDeltaLimit) };
    }

    inline SettingRange SearchedRange()
    {
        return { .lowest = Setting(0.10, 200, 0.40), .highest = Setting(0.50, 4200, 0.80) };
    }

    inline sim::UnderlyingTerms Market(std::string underlying, double initialPrice)
    {
        return { .underlying = std::move(underlying),
            .initialPrice = Euro(initialPrice),
            .drift = core::Rate::FromContinuous(0.07),
            .volatility = core::Volatility::FromAnnualized(0.25),
            .impliedVolatility = core::Volatility::FromAnnualized(0.25),
            .dividendYield = core::Rate::Zero(),
            .dividendWithholdingBasisPoints = 1500,
            .strikeIncrement = Euro(1.00) };
    }

    inline BookExperiment Experiment(std::int32_t paths, std::vector<sim::UnderlyingTerms> underlyings)
    {
        return { .start = strategy::Portfolio::Funded(Euro(200000.00)),
            .triggers = { .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = core::Delta::FromValue(0.60) },
            .maxCollateralPerNameBasisPoints = 10000,
            .fees = costs::BrokerFees(costs::FeeSchedule{ .optionFeePerContract = Euro(0.75),
                .shareCommissionPerTrade = Euro(2.00),
                .handlingFeePerTrade = Euro(1.00),
                .connectivityFeeCapPerExchangePerYear = Euro(2.50),
                .connectivityBasisPoints = 25,
                .currencyHandlingBasisPoints = 25,
                .debitInterestBasisPoints = 690 }),
            .taxes = tax::Box3TaxModel(tax::Box3Parameters{ .taxYear = 2025,
                .taxFreeAllowance = Euro(57684.00),
                .depositReturnBasisPoints = 137,
                .investmentReturnBasisPoints = 588,
                .taxBasisPoints = 3600,
                .dividendWithholdingBasisPoints = 1500 }),
            .terms = { .start = core::Date::FromYearMonthDay(2025, 1, 2),
                .cycles = 6,
                .daysPerCycle = 182,
                .observationDaysBeforeExpiry = 30,
                .targetDelta = core::Delta::FromValue(0.30),
                .riskFreeRate = core::Rate::FromContinuous(0.02),
                .assessment = sim::AnnualReference{ .month = 1, .day = 1 },
                .underlyings = std::move(underlyings) },
            .paths = paths,
            .seed = 20250811u };
    }

    inline BookExperiment Experiment(std::int32_t paths)
    {
        return Experiment(paths, { Market("ACME", 40.00) });
    }

    inline RiskLimits UnreachableLimits()
    {
        return { .maximumDrawdown = 1.0, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 1.0 };
    }
}
