#pragma once

#include <array>
#include <cstddef>

namespace fin::optimize
{
    inline constexpr std::size_t dimensions = 3;

    using Point = std::array<double, dimensions>;

    class Objective
    {
    protected:
        Objective() = default;
        Objective(const Objective& other) = delete;
        Objective& operator=(const Objective& other) = delete;
        ~Objective() = default;

    public:
        virtual double Cost(const Point& at) const = 0;
        virtual Point Gradient(const Point& at) const = 0;
    };

    bool IsWithinTheUnitBox(const Point& at);
    Point ClampedToTheUnitBox(const Point& at);
}
