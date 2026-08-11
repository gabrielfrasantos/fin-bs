#pragma once

#include "source/costs/FeeSchedule.hpp"
#include <optional>
#include <string>
#include <vector>

namespace fin::io
{
    class BrokerConfig
    {
    public:
        explicit BrokerConfig(const std::vector<std::string>& document);

        const std::optional<costs::FeeSchedule>& Schedule() const;
        const std::string& Failure() const;

    private:
        std::optional<costs::FeeSchedule> schedule;
        std::string failure;
    };
}
