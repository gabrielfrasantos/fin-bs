#pragma once

#include "source/strategy/Book.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include <span>
#include <string>

namespace fin::io
{
    std::string CycleReport(std::span<const strategy::BookRow> rows);
    std::string LedgerReport(const strategy::CompoundingLedger& ledger);
}
