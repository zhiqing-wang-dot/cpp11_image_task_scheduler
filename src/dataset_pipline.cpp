#include "image_task_scheduler/dataset_pipline.hpp"

#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "image_task_scheduler/dataset_cleaner.hpp"
#include "image_task_scheduler/factory_processor.hpp"
#include "image_task_scheduler/file_utils.hpp"
#include "image_task_scheduler/image_processor.hpp"
#include "image_task_scheduler/image_result.hpp"
#include "image_task_scheduler/image_task.hpp"
#include "image_task_scheduler/logger.hpp"
#include "image_task_scheduler/quality_scheduler.hpp"
#include "image_task_scheduler/task_scheduler.hpp"
#include "image_task_scheduler/timer.hpp"

DatasetPipline::DatasetPipline(const Config& config)
: config_(config)
{ }

bool DatasetPipline::prepare()
{
   if (!FileUtils::is_directory(config_.input_dir))
   {
       LOG_ERROR("输入路径不是一个目录: " + config_.input_dir);
       return false;
   }

   if (!FileUtils::create_directory_if_not_exists(config_.output_dir))
   {
       LOG_ERROR("创建输出目录失败: " + config_.output_dir);
       return false;
   }

   image_files_ = FileUtils::list_image_files(config_.input_dir);
    if (image_files_.empty())
    {
         LOG_ERROR("输入目录中没有找到图片文件: " + config_.input_dir);
         return false;
    }

    process_images_ = image_files_; // 默认处理全部图片，后续质检阶段会根据配置筛选
    return true;

}

bool DatasetPipline::run_quality_stage()
{
    std::string report_file = config_.output_dir + "/report.csv";

    QualityScheduler quality_scheduler(
        static_cast<std::size_t>(config_.thread_count),
        report_file,
        config_.quality_thresholds
    );

    std::vector<ImageQuality> quality_results; // 存储质检结果的容器

    if (!quality_scheduler.run_future(image_files_, &quality_results))
    {
        return false;
    }

    DatasetCleaner cleaner; // 创建 DatasetCleaner 实例

    std::vector<ImageQuality> bad_images; // 存储不合格图片信息的容器
    cleaner.split(quality_results, &process_images_, &bad_images);

    std::string bad_images_file = config_.output_dir + "/bad_images.txt";

    if (!cleaner.write_bad_images(bad_images_file, bad_images))
    {
        LOG_ERROR("failed to write bad_images.txt");
        return false;
    }

    std::cout << "========== Dataset Clean Summary ==========" << std::endl;
    std::cout << "raw images : " << image_files_.size() << std::endl;
    std::cout << "good images: " << process_images_.size() << std::endl;
    std::cout << "bad images : " << bad_images.size() << std::endl;
    std::cout << "report file: " << report_file << std::endl;
    std::cout << "bad list   : " << bad_images_file << std::endl;
    std::cout << "===========================================" << std::endl;

    return true;
}

bool DatasetPipline::run_process_stage()
{
    std::unique_ptr<ImageProcessor> processor =
        FactoryProcessor::create(config_.mode);

    if (!processor)
    {
        LOG_ERROR("failed to create processor");
        return false;
    }

    TaskScheduler scheduler(config_.thread_count, std::move(processor));

    int success_count = 0;
    int failed_count = 0;

    std::vector<long long> costs;
    Timer wall_timer;

    if (config_.use_future)
    {
        std::vector<std::future<ImageResult> > futures;
        futures.reserve(process_images_.size());

        for (std::size_t i = 0; i < process_images_.size(); ++i)
        {
            cv::Mat image = cv::imread(process_images_[i]);

            if (image.empty())
            {
                ++failed_count;
                costs.push_back(0);
                continue;
            }

            std::string output_path =
                FileUtils::build_output_path(config_.output_dir,
                                             process_images_[i],
                                             config_.mode);

            std::shared_ptr<ImageTask> task(
                new ImageTask(static_cast<int>(i),
                              image,
                              process_images_[i],
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
    }
    else // 使用 callback
    {
        std::atomic<int> atomic_success_count(0);
        std::atomic<int> atomic_failed_count(0);

        int finished_count = 0;

        std::mutex costs_mutex;
        std::mutex finished_mutex;
        std::condition_variable finish_cv;

        TaskScheduler::Callback callback =
            [&atomic_success_count,
             &atomic_failed_count,
             &finished_count,
             &costs,
             &costs_mutex,
             &finished_mutex,
             &finish_cv](const ImageResult& result) {
                {
                    std::lock_guard<std::mutex> lock(costs_mutex);
                    costs.push_back(result.cost_ms);
                }

                if (result.success)
                {
                    atomic_success_count.fetch_add(1);
                }
                else
                {
                    atomic_failed_count.fetch_add(1);
                    std::cout << "failed task " << result.task_id
                              << ": " << result.error_message << std::endl;
                }

                {
                    std::lock_guard<std::mutex> lock(finished_mutex);
                    ++finished_count;
                }

                finish_cv.notify_one();
            };

        for (std::size_t i = 0; i < process_images_.size(); ++i)
        {
            cv::Mat image = cv::imread(process_images_[i]);

            if (image.empty())
            {
                {
                    std::lock_guard<std::mutex> lock(costs_mutex);
                    costs.push_back(0);
                }

                atomic_failed_count.fetch_add(1);

                {
                    std::lock_guard<std::mutex> lock(finished_mutex);
                    ++finished_count;
                }

                finish_cv.notify_one();
                continue;
            }

            std::string output_path =
                FileUtils::build_output_path(config_.output_dir,
                                             process_images_[i],
                                             config_.mode);

            std::shared_ptr<ImageTask> task(
                new ImageTask(static_cast<int>(i),
                              image,
                              process_images_[i],
                              output_path)
            );

            scheduler.submit(task, callback);
        }

        {
            std::unique_lock<std::mutex> lock(finished_mutex);
            finish_cv.wait(lock, [&finished_count, this]() {
                return finished_count >= static_cast<int>(process_images_.size());
            });
        }

        success_count = atomic_success_count.load();
        failed_count = atomic_failed_count.load();
    }

    long long total_cost_ms = wall_timer.elapsed_ms();

    std::cout << "========== Preprocess Summary ==========" << std::endl;
    std::cout << "process images  : " << process_images_.size() << std::endl;
    std::cout << "success tasks    : " << success_count << std::endl;
    std::cout << "failed tasks     : " << failed_count << std::endl;
    std::cout << "total wall cost  : " << total_cost_ms << " ms" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed_count == 0;

}


bool DatasetPipline::run()
{
    if (!prepare())
    {
        return false;
    }

    // enable_quality 决定是否先做数据集质量检测。
    // mode 只表示后续预处理算法：gray / sobel / resize / blur。
    if (config_.enable_quality)
    {
        if (!run_quality_stage())
        {
            return false;
        }

        if (process_images_.empty())
        {
            LOG_WARN("no good images after quality check");
            return true;
        }
    }
    else
    {
        process_images_ = image_files_; // 如果不启用质检，直接处理全部图片
    }

    return run_process_stage();
}
