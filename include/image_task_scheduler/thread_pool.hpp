#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <functional> // for std::function<>
#include <condition_variable> // for std::condition_variable

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads); // 显示构造函数，接受线程数量作为参数
    ~ThreadPool();

    // std::function<void(std::size_t)> 是一个可调用对象，可以是函数、lambda表达式、函数对象等
    bool submit(std::function<void(std::size_t)> task); // 提交任务到线程池
    void stop(); 

    ThreadPool(const ThreadPool&) = delete; // 禁止拷贝构造函数
    ThreadPool& operator=(const ThreadPool&) = delete; // 禁止拷贝赋值运算符

private:
    void worker_loop(std::size_t worker_id); // 工作线程的循环函数

private:
    std::vector<std::thread> workers_; // 工作线程列表
    std::queue<std::function<void(std::size_t)>> tasks_; // 任务队列
    std::mutex queue_mutex_; // 保护任务队列的互斥锁
    std::condition_variable condition_; // 条件变量，用于通知工作线程有新任务到来
    bool stop_; // 停止标志

};
