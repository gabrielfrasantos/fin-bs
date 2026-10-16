#pragma once

#include <cstdint>

namespace fin::sim
{
    class RunObserver
    {
    public:
        RunObserver(const RunObserver& other) = delete;
        RunObserver& operator=(const RunObserver& other) = delete;

        virtual void Completed(std::int32_t path, std::int32_t of) = 0;
        virtual bool Continues() const = 0;

    protected:
        RunObserver() = default;
        ~RunObserver() = default;
    };
}
