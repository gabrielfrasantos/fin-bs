#pragma once

#include <cstdint>
#include <random>

namespace fin::sim
{
    class Rng
    {
    public:
        explicit Rng(std::uint64_t seed);

        double NextUniform();
        double NextStandardNormal();

    private:
        std::mt19937_64 engine;
        double spare{};
        bool spareAvailable{ false };
    };
}
