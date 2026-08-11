#pragma once

#include "source/core/Money.hpp"
#include "source/sim/BookScript.hpp"
#include "source/strategy/RollRules.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fin::io
{
    struct Scenario
    {
        std::string name;
        core::Money capital;
        std::int32_t cashBufferBasisPoints;
        strategy::RollTriggers triggers;
        sim::BookTerms terms;
        std::int32_t paths;
        std::uint64_t seed;
        std::string taxConfiguration;
        std::string brokerConfiguration;

        bool operator==(const Scenario& other) const = default;
    };

    class ScenarioLoader
    {
    public:
        explicit ScenarioLoader(const std::vector<std::string>& document);

        const std::optional<Scenario>& Loaded() const;
        const std::string& Failure() const;

    private:
        std::optional<Scenario> loaded;
        std::string failure;
    };

    bool IsWellFormed(const Scenario& scenario);
    core::Currency CurrencyOf(const Scenario& scenario);
    std::string Formatted(const Scenario& scenario);
}
