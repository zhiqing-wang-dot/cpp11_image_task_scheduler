#include "image_task_scheduler/timer.hpp"

Timer::Timer() 
: start_(std::chrono::steady_clock::now()) {}

void Timer::reset() 
{
    start_ = std::chrono::steady_clock::now();
}

long long Timer::elapsed_ms() const 
{
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
}