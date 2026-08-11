#pragma once

#include "source/core/Money.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/tax/Box3TaxModel.hpp"

namespace fin::strategy
{
    class Account
    {
    public:
        Account(const Portfolio& holdings, const tax::Box3TaxModel& taxes);

        core::Currency GetCurrency() const;
        const core::Money& Opening() const;

        Portfolio& Holdings();
        const Portfolio& Holdings() const;

        CompoundingLedger& Ledger();
        const CompoundingLedger& Ledger() const;

        const tax::Box3TaxModel& Taxes() const;

    private:
        core::Money opening;
        Portfolio portfolio;
        CompoundingLedger ledger;
        tax::Box3TaxModel taxes;
    };

    bool Reconciles(const Account& account);
}
