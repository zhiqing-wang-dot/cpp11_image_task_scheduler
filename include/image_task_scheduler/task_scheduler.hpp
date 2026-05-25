# pragma once   

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "image_task_scheduler/thread_pool.hpp"
#include "image_task_scheduler/image_task.hpp"
#include "image_task_scheduler/image_processor.hpp"
#include "image_task_scheduler/image_result.hpp"

class TaskScheduler 
{
public:
    typedef std::function<void(const ImageResult&)> Callback;
    // processor 用 unique_ptr：
    // 表示 TaskScheduler 独占这个图像处理器，别人不能共享它
    // 构造 TaskScheduler 时，把处理器的所有权交给调度器管理
    TaskScheduler(std::size_t thread_num, 
                  std::unique_ptr<ImageProcessor> processor);

    ~TaskScheduler() = default;

    // task 用 shared_ptr：
    // 表示这个任务可能会被多个地方同时使用，
    // 比如提交者、任务队列、工作线程
    // 只有所有地方都不用了，任务才会自动释放
    void submit(std::shared_ptr<ImageTask> task, Callback callback);

    std::future<ImageResult> submit_future(std::shared_ptr<ImageTask> task);

private:
    ImageResult make_failed_result(std::shared_ptr<ImageTask> task, 
                                   const std::string& error_message,
                                   std::size_t worker_id) const;

private:
    std::unique_ptr<ImageProcessor> processor_;
    ThreadPool thread_pool_;
};
