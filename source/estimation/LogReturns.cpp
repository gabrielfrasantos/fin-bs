#include "source/estimation/LogReturns.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cmath>

namespace fin::estimation
{
    namespace
    {
        void VerifyPricesAreQuotable(std::span<const core::Money> prices)
        {
            really_assert(!prices.empty());

            for (const auto& price : prices)
            {
                really_assert(price.GetCurrency() == prices.front().GetCurrency());
                really_assert(price.Cents() > 0);
            }
        }
    }

    std::vector<double> LogPrices(std::span<const core::Money> prices)
    {
        VerifyPricesAreQuotable(prices);

        std::vector<double> result;
        result.reserve(prices.size());

        for (const auto& price : prices)
            result.push_back(std::log(price.ToDouble()));

        return result;
    }

    std::vector<double> LogReturns(std::span<const core::Money> prices)
    {
        VerifyPricesAreQuotable(prices);

        std::vector<double> result;
        result.reserve(prices.size() - 1);

        for (std::size_t i = 1; i != prices.size(); ++i)
            result.push_back(std::log(prices[i].ToDouble() / prices[i - 1].ToDouble()));

        return result;
    }
}
