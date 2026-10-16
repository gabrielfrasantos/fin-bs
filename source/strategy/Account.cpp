#include "source/strategy/Account.hpp"

namespace fin::strategy
{
    Account::Account(const Portfolio& holdings, const tax::Box3TaxModel& taxes)
        : opening(holdings.Cash())
        , portfolio(holdings)
        , ledger(holdings.GetCurrency())
        , taxes(taxes)
    {}

    core::Currency Account::GetCurrency() const
    {
        return portfolio.GetCurrency();
    }

    const core::Money& Account::Opening() const
    {
        return opening;
    }

    Portfolio& Account::Holdings()
    {
        return portfolio;
    }

    const Portfolio& Account::Holdings() const
    {
        return portfolio;
    }

    CompoundingLedger& Account::Ledger()
    {
        return ledger;
    }

    const CompoundingLedger& Account::Ledger() const
    {
        return ledger;
    }

    const tax::Box3TaxModel& Account::Taxes() const
    {
        return taxes;
    }

    bool Reconciles(const Account& account)
    {
        return account.Opening() + account.Ledger().Net() == account.Holdings().Cash();
    }
}
