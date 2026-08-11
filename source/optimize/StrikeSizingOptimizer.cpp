#include "source/optimize/StrikeSizingOptimizer.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "numerical/optimization/GradientDescent.hpp"
#include "numerical/regularization/L1.hpp"
#include "numerical/regularization/L2.hpp"

namespace fin::optimize
{
    namespace
    {
        using Displacement = math::Vector<double, dimensions>;

        Point Displaced(const Point& start, const Displacement& displacement)
        {
            Point moved{};

            for (std::size_t coordinate = 0; coordinate != dimensions; ++coordinate)
                moved[coordinate] = start[coordinate] + displacement[coordinate];

            return ClampedToTheUnitBox(moved);
        }

        class HeldNearTheStart
            : public optimization::ObjectiveFunction<double, dimensions>
        {
        public:
            HeldNearTheStart(const Point& start, const Objective& objective, const DescentTerms& terms)
                : start(start)
                , objective(objective)
                , sparsity(terms.sparsityWeight)
                , shrinkage(terms.shrinkageWeight)
            {}

            double Cost(const Vector& displacement) override
            {
                return objective.Cost(Displaced(start, displacement)) + sparsity.Calculate(displacement) + shrinkage.Calculate(displacement);
            }

            Vector Gradient(const Vector& displacement) override
            {
                const auto steepest = objective.Gradient(Displaced(start, displacement));
                auto gradient = sparsity.Gradient(displacement) + shrinkage.Gradient(displacement);

                for (std::size_t coordinate = 0; coordinate != dimensions; ++coordinate)
                    gradient[coordinate] += steepest[coordinate];

                return gradient;
            }

        private:
            Point start;
            const Objective& objective;
            regularization::L1<double, dimensions> sparsity;
            regularization::L2<double, dimensions> shrinkage;
        };
    }

    StrikeSizingOptimizer::StrikeSizingOptimizer(const DescentTerms& terms)
        : terms(terms)
    {
        really_assert(IsWellFormed(terms));
    }

    Descent StrikeSizingOptimizer::From(const Point& start, const Objective& objective) const
    {
        really_assert(IsWithinTheUnitBox(start));

        HeldNearTheStart held(start, objective, terms);
        optimization::GradientDescent<double, dimensions> descent({ .learningRate = terms.learningRate, .maxIterations = terms.iterations });

        const auto& outcome = descent.Minimize(Displacement{}, held);
        const auto at = Displaced(start, outcome.parameters);

        return { .at = at, .cost = objective.Cost(at), .iterations = outcome.iterations };
    }

    bool IsWellFormed(const DescentTerms& terms)
    {
        return terms.learningRate > 0.0 &&
               terms.iterations > 0 &&
               terms.sparsityWeight >= 0.0 &&
               terms.shrinkageWeight >= 0.0;
    }
}
