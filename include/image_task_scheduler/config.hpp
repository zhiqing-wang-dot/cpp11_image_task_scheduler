#pragma once

#include <string>

class Config
{
public:
    std::string input_dir;
    std::string output_dir;
    std::string mode;
    int thread_count;
    bool use_future; // 是否使用 std::future 来获取结果

    Config();

    bool parse(int argc, char** argv, Config* config); 
    void print_usage();
    bool is_valid_mode() const;
};