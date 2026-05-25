# C++11 知识点

## auto

本项目中可以用 `auto` 接收迭代器或临时结果，但当前代码多数地方显式写出类型，便于新手理解。

## nullptr

用于判断指针是否为空，例如 `processor_ == nullptr` 或 `dir == nullptr`。相比 `NULL`，`nullptr` 是 C++11 的空指针字面量，类型更安全。

## enum class

`Logger::Level` 使用强类型枚举表示日志级别，避免普通 enum 名称污染和隐式转换问题。

## override

各处理器类实现：

```cpp
ImageResult process(const ImageTask& task, std::size_t worker_id) override;
```

`override` 可以让编译器检查函数是否真的重写了父类虚函数。

## default

`ImageProcessor` 使用默认虚析构：

```cpp
virtual ~ImageProcessor() = default;
```

表示使用编译器生成的默认析构逻辑。

## delete

`ThreadPool`、`Logger` 禁止拷贝：

```cpp
ThreadPool(const ThreadPool&) = delete;
ThreadPool& operator=(const ThreadPool&) = delete;
```

线程池和日志单例不应该被复制。

## unique_ptr

`TaskScheduler` 使用 `std::unique_ptr<ImageProcessor>` 独占处理器对象。处理器对象的生命周期由调度器管理，避免手动 `delete`。

## shared_ptr

`ImageTask` 使用 `std::shared_ptr` 跨线程传递。任务被提交到线程池后，主线程和 worker 线程都可能持有它，引用计数可以保证任务对象在使用期间不会提前释放。

## std::move

构造 `TaskScheduler` 时：

```cpp
TaskScheduler scheduler(config.thread_count, std::move(processor));
```

把 `unique_ptr` 的所有权移动给调度器。

## 右值引用

自定义 `make_unique` 中使用右值引用和完美转发：

```cpp
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
```

它把构造参数原样转发给目标对象构造函数。

## lambda

线程池任务、callback 和 future 任务都使用 lambda：

```cpp
thread_pool_.submit([this, task, callback](std::size_t worker_id) {
    ...
});
```

lambda 让任务逻辑可以直接捕获上下文变量。

## std::function

线程池任务类型：

```cpp
std::function<void(std::size_t)>
```

callback 类型：

```cpp
std::function<void(const ImageResult&)>
```

`std::function` 可以保存 lambda、普通函数或函数对象。

## std::bind

项目当前主要使用 lambda 完成绑定。`std::bind` 也可以用于绑定成员函数和参数，但 lambda 在这里更直观。

## std::thread

`ThreadPool` 中保存 worker：

```cpp
std::vector<std::thread> workers_;
```

每个 worker 独立执行 `worker_loop()`。

## std::mutex

保护共享数据，例如任务队列、日志输出和性能耗时数组。

## std::lock_guard

用于简单加锁场景：

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

离开作用域自动解锁，符合 RAII。

## std::unique_lock

`condition_variable::wait()` 需要 `std::unique_lock`：

```cpp
std::unique_lock<std::mutex> lock(queue_mutex_);
condition_.wait(lock, ...);
```

它支持 wait 内部临时释放锁和被唤醒后重新加锁。

## std::condition_variable

线程池中 worker 没有任务时进入等待：

```cpp
condition_.wait(lock, [this] {
    return stop_ || !tasks_.empty();
});
```

避免 worker 空转浪费 CPU。

## std::future

future 模式中，主线程通过：

```cpp
future.get();
```

等待 worker 返回 `ImageResult`。

## std::promise

worker 线程通过：

```cpp
promise->set_value(result);
```

把结果传给主线程持有的 `future`。

## std::atomic

callback 模式中多个 worker 会同时更新计数：

```cpp
std::atomic<int> finished_count;
```

使用 atomic 可以避免数据竞争。

## std::chrono

`Timer` 使用 `std::chrono::steady_clock` 统计任务耗时和总耗时。`steady_clock` 不受系统时间调整影响，适合测量时间间隔。

## RAII

线程池析构时调用 `stop()`，自动通知 worker 退出并 join 线程。锁对象、智能指针也都体现 RAII 思想。

## 工厂模式

`FactoryProcessor::create(mode)` 根据字符串创建不同处理器，避免 `main.cpp` 直接依赖每个具体算法类。

## 多态

主流程只持有：

```cpp
std::unique_ptr<ImageProcessor>
```

实际对象可以是 `GrayProcessor`、`SobelProcessor`、`ResizeProcessor` 或 `BlurProcessor`。调用 `process()` 时通过虚函数分发到具体实现。
