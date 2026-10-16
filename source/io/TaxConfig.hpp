#pragma once

#include "source/tax/Box3Parameters.hpp"
#include <optional>
#include <string>
#include <vector>

namespace fin::io
{
    class TaxConfig
    {
    public:
        explicit TaxConfig(const std::vector<std::string>& document);

        const std::optional<tax::Box3Parameters>& Parameters() const;
        const std::string& Failure() const;

    private:
        std::optional<tax::Box3Parameters> parameters;
        std::string failure;
    };
}
