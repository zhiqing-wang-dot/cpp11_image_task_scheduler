#pragma once 

#include <vector>
#include <string>

#include "image_task_scheduler/config.hpp"

class DatasetPipline 
{
public:
    explicit DatasetPipline(const Config& config);
    ~DatasetPipline() = default;
    // 总的执行函数，返回是否成功
    bool run();

private:
    Config config_;

    std::vector<std::string> image_files_;
    std::vector<std::string> process_images_;

private:

    // 准备阶段
    bool prepare();
    // 质检阶段
    bool run_quality_stage();
    // good images 处理阶段
    bool run_process_stage();
};