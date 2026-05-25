# 项目架构

## 整体架构

```txt
命令行参数
    |
    v
Config
    |
    v
FileUtils 扫描图片目录
    |
    v
FactoryProcessor 创建 ImageProcessor
    |
    v
TaskScheduler
    |
    +--> ThreadPool
            |
            +--> worker 0 -> ImageProcessor::process(ImageTask)
            +--> worker 1 -> ImageProcessor::process(ImageTask)
            +--> worker 2 -> ImageProcessor::process(ImageTask)
            +--> worker 3 -> ImageProcessor::process(ImageTask)
```

## 模块职责

`Config`：
解析命令行参数，保存输入目录、输出目录、线程数、处理模式和是否使用 future。

`FileUtils`：
使用 `dirent.h` 和 `sys/stat.h` 完成目录判断、目录创建、图片扫描和输出路径生成。

`ImageTask`：
封装单张图片任务，包括任务 id、图像数据、输入路径和输出路径。

`ImageResult`：
描述任务处理结果，包括是否成功、错误信息、耗时和 worker id。

`ImageProcessor`：
图像处理算法抽象接口，所有算法处理器都实现 `process()`。

`GrayProcessor / SobelProcessor / ResizeProcessor / BlurProcessor`：
具体图像处理算法实现。

`FactoryProcessor`：
根据 `mode` 字符串创建不同的处理器对象。

`ThreadPool`：
管理 worker 线程和任务队列，负责并发执行任务。

`TaskScheduler`：
连接 `ImageTask`、`ImageProcessor` 和 `ThreadPool`，提供 callback 和 future 两种提交方式。

`Logger`：
使用互斥锁保证多线程日志输出不交叉。

`Timer`：
基于 `std::chrono::steady_clock` 统计耗时。

## 任务执行流程

```txt
main 创建 ImageTask
    |
    v
TaskScheduler::submit
    |
    v
ThreadPool::submit
    |
    v
任务进入 queue
    |
    v
worker 被 condition_variable 唤醒
    |
    v
worker 调用 processor_->process(*task, worker_id)
    |
    v
返回 ImageResult
```

## callback 模式流程

```txt
main 提交任务
    |
    v
worker 执行图像处理
    |
    v
生成 ImageResult
    |
    v
调用 callback(result)
    |
    v
callback 更新 success / failed / finished / costs
    |
    v
main 等待 finished_count == total_tasks
```

callback 模式中，统计变量由多个 worker 线程修改，因此：

- `finished_count`、`success_count`、`failed_count` 使用 `std::atomic<int>`。
- `costs` 使用 `std::mutex` 保护。

## future 模式流程

```txt
main 调用 submit_future
    |
    v
TaskScheduler 创建 promise / future
    |
    v
worker 执行图像处理
    |
    v
promise->set_value(result)
    |
    v
main 通过 future.get() 获取 ImageResult
```

future 模式中，结果汇总发生在主线程，因此成功数、失败数可以使用普通 `int`。

## 线程池工作流程

```txt
ThreadPool 构造
    |
    v
创建 N 个 worker 线程
    |
    v
worker_loop 等待任务
    |
    v
submit 加锁写入任务队列
    |
    v
condition_variable notify_one
    |
    v
worker 取出任务
    |
    v
释放锁
    |
    v
执行任务
```

停止流程：

```txt
ThreadPool::stop
    |
    v
加锁设置 stop_ = true
    |
    v
notify_all
    |
    v
worker 发现 stop_ 且队列为空
    |
    v
退出循环
    |
    v
主线程 join 所有 worker
```

## 新增算法扩展流程

1. 新建头文件和源文件，例如 `sharpen_processor.hpp/.cpp`。
2. 继承 `ImageProcessor`。
3. 实现 `process()`。
4. 使用 `Timer` 统计耗时。
5. 失败时填写 `ImageResult::error_message`。
6. 在 `FactoryProcessor::create()` 中增加 `mode` 分支。
7. 在 `CMakeLists.txt` 中加入新的源文件。
