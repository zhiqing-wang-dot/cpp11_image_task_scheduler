#pragma once  

#include <string>
#include <fstream>
#include <mutex>

#include "image_task_scheduler/image_quality.hpp"

class QualityReportWriter 
{
public:
    explicit QualityReportWriter(const std::string& csv_path);
    ~QualityReportWriter() = default;

    bool open(); // 打开文件
    void write_header(); // 写入表头
    void write_row(const ImageQuality& quality); // 写入一行数据
    void close();  // 关闭文件

private:
    std::string csv_path_;
    std::ofstream file_;
    std::mutex mutex_;
};
