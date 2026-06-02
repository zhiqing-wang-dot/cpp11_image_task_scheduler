#include "image_task_scheduler/quality_scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <thread>
#include <memory>

#include <opencv2/imgcodecs.hpp>

#include "image_task_scheduler/timer.hpp"
#include "image_task_scheduler/image_quality.hpp"

QualityScheduler::QualityScheduler(std::size_t thread_num, const std::string& report_file)
: thread_pool_(thread_num) // 注意成员变量中类的初始化, 直接传入构造函数的参数即可
, quality_analyzer_() 
, report_writer_(report_file)
{ }

QualityScheduler::QualityScheduler(std::size_t thread_num,
                                   const std::string& report_file,
                                   const QualityThresholds& thresholds)
: thread_pool_(thread_num)
, quality_analyzer_(thresholds)
, report_writer_(report_file)
{ }

ImageQuality QualityScheduler::analyze_one(int task_id, 
                                           const std::string& image_file) const
{
    cv::Mat image = cv::imread(image_file);
    return quality_analyzer_.analyze(task_id, image_file, image);
}

void QualityScheduler::print_summary(const std::string& title,
                                     std::size_t total_tasks,
                                     int success_count,
                                     int failed_count,
                                     const std::vector<long long>& costs,
                                     long long total_wall_cost) const
{
    long long total_cost_time = 0;
    long long max_cost_time = 0;
    long long min_cost_time = 0;

    if (!costs.empty())
    {
        min_cost_time = costs[0];

        for (const auto& cost : costs)
        {
            total_cost_time += cost;
            max_cost_time = std::max(max_cost_time, cost);
            min_cost_time = std::min(min_cost_time, cost);
        }
    }

    double avg_task_cost = 0.0;
    if (!costs.empty())
    {
        avg_task_cost =
            static_cast<double>(total_cost_time) /
            static_cast<double>(costs.size());
    }

    double throughput = 0.0;
    if (total_wall_cost > 0)
    {
        throughput =
            static_cast<double>(total_tasks) /
            (static_cast<double>(total_wall_cost) / 1000.0);
    }

    std::cout << "========== " << title << " ==========" << std::endl;
    std::cout << "total tasks      : " << total_tasks << std::endl;
    std::cout << "readable images  : " << success_count << std::endl;
    std::cout << "failed images    : " << failed_count << std::endl;
    std::cout << "total wall cost  : " << total_wall_cost << " ms" << std::endl;
    std::cout << "avg task cost    : " << avg_task_cost << " ms" << std::endl;
    std::cout << "max task cost    : " << max_cost_time << " ms" << std::endl;
    std::cout << "min task cost    : " << min_cost_time << " ms" << std::endl;
    std::cout << "throughput       : " << throughput << " images/s" << std::endl;
    std::cout << "=====================================" << std::endl;
}

void QualityScheduler::process_one(int task_id,
                                   const std::string& image_file,
                                   std::atomic<int>* finished_count,
                                   std::atomic<int>* success_count,
                                   std::atomic<int>* failed_count,
                                   std::vector<long long> *costs,
                                   std::mutex *costs_mutex)
{
    // 分析一张图片
    ImageQuality quality = analyze_one(task_id, image_file);
    // 写入报告
    report_writer_.write_row(quality);

    if (quality.readable) 
    { 
        success_count->fetch_add(1);
    }
    else 
    {
        failed_count->fetch_add(1);
    }

    {
        std::lock_guard<std::mutex> lock(*costs_mutex);
        costs->push_back(quality.cost_ms);
    }
    finished_count->fetch_add(1);
}

bool QualityScheduler::run_future(const std::vector<std::string>& image_files,
                                  std::vector<ImageQuality>* results) 
{ 
    if (!report_writer_.open())
        {
            std::cerr << "Failed to open report file" << std::endl;
            return false;
        }

        report_writer_.write_header();

        Timer wall_timer;

        std::vector<std::future<ImageQuality>> futures;
        futures.reserve(image_files.size());

        // 每一个 future 最终都会拿到一个 ImageQuality。
        for (int i = 0; i < image_files.size(); ++i)
        {
            std::shared_ptr<std::promise<ImageQuality>> promise(new std::promise<ImageQuality>);
            futures.push_back(promise->get_future());

            bool submitted = thread_pool_.submit(
                [this,
                i,
                &image_files,
                promise](std::size_t worker_id){
                    (void)worker_id;
                    ImageQuality quality = analyze_one(i, image_files[i]);
                    promise->set_value(quality);
                });

            if (!submitted)
            {
                ImageQuality quality;
                quality.readable = false;
                quality.input_path = image_files[i];
                quality.task_id = i;
                promise->set_value(quality);
            }
        }

        // 处理成功和失败的数量
        int success_count = 0;
        int failed_count = 0;
        
        std::vector<long long> costs;
        costs.reserve(image_files.size());


        // 主线程获取结果
        for (std::size_t i = 0; i < futures.size(); ++i)
        { 
            ImageQuality quality = futures[i].get();
            if(results != nullptr)
            {
                results->push_back(quality);
            }
            if (quality.readable)
            {
                success_count++;
            }
            else
            {
                failed_count++;
            }
            report_writer_.write_row(quality);
            costs.push_back(quality.cost_ms);
        }

        // 关闭文件
        report_writer_.close();

        print_summary("Summary",
                        image_files.size(),
                        success_count,
                        failed_count,
                        costs,
                        wall_timer.elapsed_ms());
            
        return true;
}

bool QualityScheduler::run(const std::vector<std::string>& image_files)
{ 
    if (!report_writer_.open()) 
    {
        std::cerr << "Failed to open report file" << std::endl;
        return false;
    }
    report_writer_.write_header(); // 写入表头
    std::atomic<int> finished_count(0);
    std::atomic<int> success_count(0);
    std::atomic<int> failed_count(0);

    std::vector<long long> costs;
    std::mutex costs_mutex;
    Timer wall_timer;
    const int task_num = static_cast<int>(image_files.size());
    for (std::size_t i = 0; i < image_files.size(); ++i)
    {
        bool submitted = thread_pool_.submit(
            [this,
            i,
            &image_files,
            &finished_count,
            &success_count,
            &failed_count,
            &costs,
            &costs_mutex](std::size_t worker_id){
                (void)worker_id;
                process_one(i,
                            image_files[i],
                            &finished_count,
                            &success_count,
                            &failed_count,
                            &costs,
                            &costs_mutex);
            });
        
        if (!submitted) // 未提交成功，要处理数量问题
        {
            failed_count.fetch_add(1);
            finished_count.fetch_add(1);
        }
    } // for
    while (finished_count.load() < task_num)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    report_writer_.close();

    print_summary("Summary",
                  task_num,
                  success_count.load(), // load() 出具体的 int 值
                  failed_count.load(),
                  costs,
                  wall_timer.elapsed_ms());
    return true;
}

bool QualityScheduler::run_future(const std::vector<std::string>& image_files)
{
    return run_future(image_files, nullptr);
}
