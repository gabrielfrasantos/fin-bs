#pragma once

#include "source/optimize/Objective.hpp"
#include <cstddef>

namespace fin::optimize
{
    struct DescentTerms
    {
        double learningRate;
        std::size_t iterations;
        double sparsityWeight;
        double shrinkageWeight;
    };

    struct Descent
    {
        Point at;
        double cost;
        std::size_t iterations;
    };

    class StrikeSizingOptimizer
    {
    public:
        explicit StrikeSizingOptimizer(const DescentTerms& terms);

        Descent From(const Point& start, const Objective& objective) const;

    private:
        DescentTerms terms;
    };

    bool IsWellFormed(const DescentTerms& terms);
}
