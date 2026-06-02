#pragma once

#include <chrono>

class Timer 
{
public:
    Timer(); // 初始化并设置开始时间
    void reset();
    long long elapsed_ms() const; // 记录耗时

private:
    std::chrono::steady_clock::time_point start_;
};