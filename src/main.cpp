#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "image_task_scheduler/config.hpp"
#include "image_task_scheduler/factory_processor.hpp"
#include "image_task_scheduler/file_utils.hpp"
#include "image_task_scheduler/image_processor.hpp"
#include "image_task_scheduler/image_result.hpp"
#include "image_task_scheduler/image_task.hpp"
#include "image_task_scheduler/logger.hpp"
#include "image_task_scheduler/task_scheduler.hpp"
#include "image_task_scheduler/timer.hpp"

namespace
{

void print_performance_summary(int total_tasks,
                               int success_tasks,
                               int failed_tasks,
                               long long total_wall_cost,
                               const std::vector<long long>& costs)
{
    long long total_task_cost = 0;
    long long max_task_cost = 0;
    long long min_task_cost = 0;

    if (!costs.empty())
    {
        min_task_cost = costs[0];

        for (std::size_t i = 0; i < costs.size(); ++i)
        {
            total_task_cost += costs[i];

            if (costs[i] > max_task_cost)
            {
                max_task_cost = costs[i];
            }

            if (costs[i] < min_task_cost)
            {
                min_task_cost = costs[i];
            }
        }
    }

    double avg_task_cost = 0.0;
    if (!costs.empty())
    {
        avg_task_cost =
            static_cast<double>(total_task_cost) /
            static_cast<double>(costs.size());
    }

    double throughput = 0.0;
    if (total_wall_cost > 0)
    {
        throughput =
            static_cast<double>(total_tasks) /
            (static_cast<double>(total_wall_cost) / 1000.0);
    }

    std::cout << "========== Performance Summary ==========" << std::endl;
    std::cout << "total tasks      : " << total_tasks << std::endl;
    std::cout << "success tasks    : " << success_tasks << std::endl;
    std::cout << "failed tasks     : " << failed_tasks << std::endl;
    std::cout << "total wall cost  : " << total_wall_cost << " ms" << std::endl;
    std::cout << "avg task cost    : " << avg_task_cost << " ms" << std::endl;
    std::cout << "max task cost    : " << max_task_cost << " ms" << std::endl;
    std::cout << "min task cost    : " << min_task_cost << " ms" << std::endl;
    std::cout << "throughput       : " << throughput << " images/s" << std::endl;
    std::cout << "=========================================" << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
    Config config;
    if (!config.parse(argc, argv, &config))
    {
        return 1;
    }

    if (!FileUtils::is_directory(config.input_dir))
    {
        LOG_ERROR("input directory does not exist");
        return 1;
    }

    if (!FileUtils::create_directory_if_not_exists(config.output_dir))
    {
        LOG_ERROR("failed to create output directory");
        return 1;
    }

    std::vector<std::string> image_files =
        FileUtils::list_image_files(config.input_dir);

    if (image_files.empty())
    {
        LOG_WARN("no image files found");
        return 0;
    }

    std::unique_ptr<ImageProcessor> processor =
        FactoryProcessor::create(config.mode);

    if (!processor)
    {
        LOG_ERROR("failed to create processor");
        return 1;
    }

    TaskScheduler scheduler(config.thread_count, std::move(processor));

    const int total_tasks = static_cast<int>(image_files.size());
    Timer wall_timer;

    if (config.use_future)
    {
        int success_count = 0;
        int failed_count = 0;
        std::vector<long long> costs;
        std::vector<std::future<ImageResult> > futures;

        for (std::size_t i = 0; i < image_files.size(); ++i)
        {
            cv::Mat image = cv::imread(image_files[i]);
            if (image.empty())
            {
                ++failed_count;
                costs.push_back(0);
                continue;
            }

            std::string output_path =
                FileUtils::build_output_path(config.output_dir,
                                             image_files[i],
                                             config.mode);

            std::shared_ptr<ImageTask> task(
                new ImageTask(static_cast<int>(i),
                              image,
                              image_files[i],
                              output_path)
            );

            futures.push_back(scheduler.submit_future(task));
        }

        for (std::size_t i = 0; i < futures.size(); ++i)
        {
            ImageResult result = futures[i].get();
            costs.push_back(result.cost_ms);

            if (result.success)
            {
                ++success_count;
            }
            else
            {
                ++failed_count;
                std::cout << "failed task " << result.task_id
                          << ": " << result.error_message << std::endl;
            }
        }

        print_performance_summary(total_tasks,
                                  success_count,
                                  failed_count,
                                  wall_timer.elapsed_ms(),
                                  costs);
        return 0;
    }

    std::atomic<int> finished_count(0);
    std::atomic<int> success_count(0);
    std::atomic<int> failed_count(0);
    std::vector<long long> costs;
    std::mutex costs_mutex;

    TaskScheduler::Callback callback =
        [&finished_count,
         &success_count,
         &failed_count,
         &costs,
         &costs_mutex](const ImageResult& result) {
            {
                std::lock_guard<std::mutex> lock(costs_mutex);
                costs.push_back(result.cost_ms);
            }

            if (result.success)
            {
                success_count.fetch_add(1);
            }
            else
            {
                failed_count.fetch_add(1);
                std::cout << "failed task " << result.task_id
                          << ": " << result.error_message << std::endl;
            }

            finished_count.fetch_add(1);
        };

    for (std::size_t i = 0; i < image_files.size(); ++i)
    {
        cv::Mat image = cv::imread(image_files[i]);
        if (image.empty())
        {
            {
                std::lock_guard<std::mutex> lock(costs_mutex);
                costs.push_back(0);
            }

            failed_count.fetch_add(1);
            finished_count.fetch_add(1);
            continue;
        }

        std::string output_path =
            FileUtils::build_output_path(config.output_dir,
                                         image_files[i],
                                         config.mode);

        std::shared_ptr<ImageTask> task(
            new ImageTask(static_cast<int>(i),
                          image,
                          image_files[i],
                          output_path)
        );

        scheduler.submit(task, callback);
    }

    while (finished_count.load() < total_tasks)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    print_performance_summary(total_tasks,
                              success_count.load(),
                              failed_count.load(),
                              wall_timer.elapsed_ms(),
                              costs);

    return 0;
}
