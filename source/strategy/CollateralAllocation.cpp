#include "source/strategy/CollateralAllocation.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <iterator>

namespace fin::strategy
{
    double PremiumPerUnitOfCollateral(const CollateralClaim& claim)
    {
        really_assert(claim.strike.Cents() > 0);
        really_assert(claim.premiumPerShare >= 0.0);

        return claim.premiumPerShare / claim.strike.ToDouble();
    }

    std::vector<std::size_t> ServiceOrder(std::span<const CollateralClaim> claims)
    {
        std::vector<std::size_t> order;
        order.reserve(claims.size());

        for (std::size_t claim = 0; claim != claims.size(); ++claim)
            if (!claims[claim].fromCash)
                order.push_back(claim);

        const auto served = order.size();

        for (std::size_t claim = 0; claim != claims.size(); ++claim)
            if (claims[claim].fromCash)
                order.push_back(claim);

        std::stable_sort(std::next(order.begin(), static_cast<std::ptrdiff_t>(served)), order.end(), [claims](std::size_t left, std::size_t right)
            {
                return PremiumPerUnitOfCollateral(claims[left]) > PremiumPerUnitOfCollateral(claims[right]);
            });

        return order;
    }
}
