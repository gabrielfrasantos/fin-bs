#pragma once

#include "source/core/Date.hpp"
#include "source/costs/BrokerFees.hpp"
#include "source/strategy/Account.hpp"
#include "source/strategy/Projection.hpp"
#include "source/strategy/ReinvestmentPolicy.hpp"
#include "source/strategy/RollRules.hpp"
#include "source/strategy/WheelEngine.hpp"
#include <span>
#include <string>
#include <vector>

namespace fin::strategy
{
    struct BookPeriod
    {
        std::string underlying;
        ProjectionPeriod period;
    };

    struct BookRow
    {
        std::string underlying;
        ProjectionRow row;

        bool operator==(const BookRow& other) const = default;
    };

    struct BookAssessment
    {
        core::Date on;
        std::vector<SpotPrice> prices;
    };

    class Book
    {
    public:
        Book(std::vector<std::string> underlyings, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees, const tax::Box3TaxModel& taxes);

        std::vector<BookRow> Over(std::span<const BookPeriod> cycle);
        core::Money Assess(const BookAssessment& assessment);

        std::span<const std::string> Underlyings() const;
        WheelState StateOf(const std::string& underlying) const;
        const Portfolio& Holdings() const;
        const CompoundingLedger& Ledger() const;
        const Account& Holds() const;

    private:
        bool Carries(const std::string& underlying) const;

        Account account;
        std::vector<std::string> underlyings;
        ReinvestmentPolicy policy;
        RollRules rules;
        costs::BrokerFees brokerFees;
    };
}
