#pragma once

#include <string>

#include "image_task_scheduler/image_quality.hpp"

class Config
{
public:
    std::string input_dir;
    std::string output_dir;
    std::string mode;
    int thread_count;
    bool use_future; // 是否使用 std::future 来获取结果
    
    // 如果 enable_quality=false：
    // 直接处理全部图片
    // 如果 enable_quality=true：
    // 先质检
    // 只处理 good_images
    bool enable_quality;
    QualityThresholds quality_thresholds;

    Config();

    bool parse(int argc, char** argv, Config* config); 
    void print_usage();
    bool is_valid_mode() const;
};
