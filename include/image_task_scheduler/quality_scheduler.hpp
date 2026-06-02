#pragma once

#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <future>

#include "image_task_scheduler/image_quality.hpp"
#include "image_task_scheduler/quality_report_writer.hpp"
#include "image_task_scheduler/quality_analyzer.hpp"
#include "image_task_scheduler/thread_pool.hpp"

// 定义调度器
class QualityScheduler 
{
public:
    // 完成创建线程池和报告文件的作用
    QualityScheduler(std::size_t thread_num, const std::string& report_file);
    QualityScheduler(std::size_t thread_num,
                     const std::string& report_file,
                     const QualityThresholds& thresholds);
    ~QualityScheduler() = default;

    // callback 模式
    // woker 线程负责分析图片、写csv、更新统计信息
    bool run(const std::vector<std::string>& image_files);

    // future 模式
    // woker 线程负责分析图片并返回ImageQuality
    // 主线程负责写csv和统计信息
    bool run_future(const std::vector<std::string>& image_files);

    // 函数重载
    bool run_future(const std::vector<std::string>& image_file, 
                    std::vector<ImageQuality>* results);

private:
    // 单张图片分析，可以复用
    ImageQuality analyze_one(int task_id, const std::string& image_path) const;

    // callback 版本使用。
    // 它不仅分析图片，还会写报告、更新计数器。
    void process_one(int task_id,
                     const std::string& image_file,
                     std::atomic<int>* finished_count,
                     std::atomic<int>* success_count,
                     std::atomic<int>* failed_count,
                     std::vector<long long>* costs,
                     std::mutex* costs_mutex);

    // 统一输出性能统计，避免 run 和 run_future 重复写一大段统计代码。
    void print_summary(const std::string& title,
                       std::size_t total_tasks,
                       int success_count,
                       int failed_count,
                       const std::vector<long long>& costs,
                       long long total_wall_cost) const;

private:
    ThreadPool thread_pool_; // 线程池
    QualityAnalyzer quality_analyzer_; // 质量分析器
    QualityReportWriter report_writer_; // 输出报告
};
