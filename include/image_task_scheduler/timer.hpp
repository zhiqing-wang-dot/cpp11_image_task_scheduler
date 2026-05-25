#pragma once

#include <chrono>

class Timer 
{
public:
    Timer();
    void reset();
    long long elapsed_ms() const;

private:
    std::chrono::steady_clock::time_point start_;
};