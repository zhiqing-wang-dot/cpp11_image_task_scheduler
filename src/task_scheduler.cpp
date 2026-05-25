#include "image_task_scheduler/task_scheduler.hpp"

#include <exception>
#include <utility>

TaskScheduler::TaskScheduler(std::size_t thread_num, 
                             std::unique_ptr<ImageProcessor> processor)
    : processor_(std::move(processor))
    , thread_pool_(thread_num) {}

void TaskScheduler::submit(std::shared_ptr<ImageTask> task, Callback callback)
{
    bool submitted = thread_pool_.submit([this, task, callback](std::size_t worker_id) {
        ImageResult result;

        if (processor_ == nullptr)
        {
            result = make_failed_result(task, "processor is nullptr", worker_id);
        }
        else
        {
            result = processor_->process(*task, worker_id);
        }

        if (callback)
        {
            try
            {
                callback(result);
            }
            catch (...) // ... 表示捕获所有异常，防止回调函数抛出异常导致线程池崩溃
            {
                std::cout << "捕获到异常" << std::endl;
            } 
        }
    });

    if (!submitted && callback) // 如果提交失败并且传入了回调函数，调用回调函数返回失败结果
    {
        callback(make_failed_result(task, "thread pool stopped", static_cast<std::size_t>(-1)));
    }
}

std::future<ImageResult> TaskScheduler::submit_future(std::shared_ptr<ImageTask> task)
{
    // promise：负责存结果
    // future：负责取结果
    std::shared_ptr<std::promise<ImageResult>> promise (new std::promise<ImageResult>());

    // get_future()：从 promise 获取 future 对象，future 可以在其他线程中等待结果或者获取结果
    std::future<ImageResult> future = promise->get_future();

    bool submitted = thread_pool_.submit([this, task, promise](std::size_t worker_id) {
        try
        {
            if (!processor_)
            {
                promise->set_value(make_failed_result(task, "processor is null", worker_id));
                return;
            }

            promise->set_value(processor_->process(*task, worker_id));
        }
        catch (const std::exception& e)
        {
            promise->set_value(make_failed_result(task, e.what(), worker_id));
        }
        catch (...)
        {
            promise->set_value(make_failed_result(task, "unknown exception", worker_id));
        }
    });

    if (!submitted) // 如果提交失败，直接设置 promise 的值为失败结果，这样 future 在等待时就会得到这个失败结果，而不是一直等待
    {
        promise->set_value(make_failed_result(task, "thread pool stopped", 0));
    }

    return future;
}

ImageResult TaskScheduler::make_failed_result(std::shared_ptr<ImageTask> task, 
                                              const std::string& error_message,
                                              std::size_t worker_id) const
{
    ImageResult result;
    result.task_id = task->task_id();
    result.success = false;
    result.input_path = task->input_path();
    result.output_path = task->output_path();
    result.error_message = error_message;
    result.worker_id = worker_id;
    return result;
}
