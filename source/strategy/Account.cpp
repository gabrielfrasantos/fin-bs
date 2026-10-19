#include "source/strategy/Account.hpp"
#include "infra/util/ReallyAssert.hpp"

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

    void AccrueDebitInterest(Account& account, const costs::BrokerFees& fees, const core::Money& openingCash, const core::Date& from, const core::Date& to)
    {
        really_assert(openingCash.GetCurrency() == account.GetCurrency());
        really_assert(from <= to);

        const auto interest = fees.DebitInterest(openingCash, to - from);

        if (interest.Cents() == 0)
            return;

        account.Holdings().Pay(interest);
        account.Ledger().Post(to, CashflowKind::Interest, -interest);
    }
}
