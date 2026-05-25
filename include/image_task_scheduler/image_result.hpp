#pragma once

#include <cstddef> 
#include <string>

struct ImageResult 
{
    int task_id;
    bool success;
    std::string input_path;
    std::string output_path;
    std::string error_message;
    long long cost_ms;
    std::size_t worker_id;

    ImageResult()
    : task_id (-1),
      success (false),
      cost_ms (0),
      worker_id (0) 
    { }
};
