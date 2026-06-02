#include "image_task_scheduler/quality_report_writer.hpp"

QualityReportWriter::QualityReportWriter(const std::string& csv_path)
    : csv_path_(csv_path)
    { }

bool QualityReportWriter::open()
{
    // 这个类后面会被多个 worker 线程使用，所以文件操作统一加锁保护。
    std::lock_guard<std::mutex> lock(mutex_);

    // trunc 表示如果 report.csv 已经存在，就先清空旧内容再写入本次结果。
    file_.open(csv_path_.c_str(), std::ios::out | std::ios::trunc);
    return file_.is_open();
}

void QualityReportWriter::write_header()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 如果文件没有成功打开，直接返回，避免向无效文件流写数据。
    if (!file_.is_open())
    {
        return;
    }

    // CSV 第一行是表头，后续每一行都按这个字段顺序写入 ImageQuality。
    file_ << "task_id,input_path,readable,width,height,channels,"
          << "brightness,contrast,sharpness,edge_density,"
          << "blurry,too_dark,too_bright,low_contrast,low_texture,cost_ms\n";
}

void QualityReportWriter::write_row(const ImageQuality& quality)
{
    // 多线程同时写同一个 ofstream 会导致内容交叉，所以每次写一整行时加锁。
    std::lock_guard<std::mutex> lock(mutex_);

    if (!file_.is_open())
    {
        return;
    }

    // 注意字段顺序要和 write_header() 中的表头保持一致。
    file_ << quality.task_id << ","
          << quality.input_path << ","
          << quality.readable << ","
          << quality.width << ","
          << quality.height << ","
          << quality.channels << ","
          << quality.brightness << ","
          << quality.contrast << ","
          << quality.sharpness << ","
          << quality.edge_density << ","
          << quality.blurry << ","
          << quality.too_dark << ","
          << quality.too_bright << ","
          << quality.low_contrast << ","
          << quality.low_texture << ","
          << quality.cost_ms << "\n";
}

void QualityReportWriter::close()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // close 可以重复调用；只有文件处于打开状态时才真正关闭。
    if (file_.is_open())
    {
        file_.close();
    }
}
