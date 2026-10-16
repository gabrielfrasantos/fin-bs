#include "source/optimize/Objective.hpp"
#include <algorithm>

namespace fin::optimize
{
    bool IsWithinTheUnitBox(const Point& at)
    {
        return std::all_of(at.begin(), at.end(), [](double coordinate)
            {
                return coordinate >= 0.0 && coordinate <= 1.0;
            });
    }

    Point ClampedToTheUnitBox(const Point& at)
    {
        Point clamped{};

        std::transform(at.begin(), at.end(), clamped.begin(), [](double coordinate)
            {
                return std::clamp(coordinate, 0.0, 1.0);
            });

        return clamped;
    }
}
