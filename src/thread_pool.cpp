#include "image_task_scheduler/thread_pool.hpp"

// 创建线程池，启动指定数量的工作线程
ThreadPool::ThreadPool(size_t num_threads) 
: stop_(false)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers_.emplace_back(std::thread(&ThreadPool::worker_loop, this, i));
    }
}


// 析构函数 
ThreadPool::~ThreadPool()
{
    stop(); 
}

bool ThreadPool::submit(std::function<void(std::size_t)> task)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (stop_)
        {
            return false;
        }

        tasks_.push(task);
    }

    condition_.notify_one(); // 通知一个工作线程有新任务到来
    return true;
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all(); // 通知所有工作线程

    for (auto& worker : workers_)
    {
        if (worker.joinable()) 
        {
            worker.join(); // 等待线程完成
        }
    }
}

// 线程池工作线程的核心循环：每个工作线程都会一直执行这个函数，
// 没任务就睡觉，有任务就取出来执行，收到停止信号后退出。
void ThreadPool::worker_loop(size_t thread_id)
{
    while (true)
    {
        // std::function 是一个函数对象，可以保存一个函数
        std::function<void(std::size_t)> task; // 创建一个任务对象
        // 进入加锁作用域
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); }); 

            if (stop_ && tasks_.empty())
            {
                return; // 停止线程
            }

            task = std::move(tasks_.front()); // 取出任务
            tasks_.pop();
        }

        task(thread_id); // 执行任务，传入线程 ID
    }
}