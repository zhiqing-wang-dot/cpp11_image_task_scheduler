# 简历项目描述

## 简历项目名称

基于 C++11 的多线程图像处理任务调度框架

## 项目描述

基于 C++11 实现面向批量图像处理任务的异步调度框架，支持目录图片扫描、线程池并发处理、任务结果回调、future 异步结果获取、算法模块扩展和性能统计。项目使用 OpenCV 实现 Gray、Sobel、Resize、GaussianBlur 等基础图像处理能力，并通过 CMake 进行模块化构建。

## 技术栈

C++11、OpenCV、CMake、STL、Linux、std::thread、std::mutex、std::condition_variable、std::future、std::promise、std::atomic、std::unique_ptr、std::shared_ptr、lambda、RAII

## 项目亮点

1. 基于生产者-消费者模型实现线程池，使用 `std::thread` 管理 worker 线程，使用 `std::mutex` 和 `std::condition_variable` 保证任务队列线程安全。
2. 使用 RAII 管理线程池生命周期，在析构阶段自动设置停止标志、唤醒 worker 并 join 所有线程，避免线程泄漏。
3. 将图像处理算法抽象为 `ImageProcessor` 接口，通过工厂模式支持 Gray、Sobel、Resize、Blur 等算法模块扩展。
4. 使用 `std::unique_ptr` 管理处理器对象，使用 `std::shared_ptr` 管理跨线程传递的任务对象，降低生命周期管理风险。
5. 支持 callback 和 `std::future<ImageResult>` 两种异步结果获取方式。
6. 使用 `std::chrono` 统计单任务耗时、总耗时、平均耗时和吞吐量，便于评估并发处理效果。

## 面试可讲点

线程池：
可以讲 worker 如何等待任务、如何避免空转、为什么执行任务前要释放锁、stop 时如何安全退出。

任务生命周期：
可以讲为什么 `ImageTask` 用 `shared_ptr`，为什么 `ImageProcessor` 用 `unique_ptr`。

callback 和 future：
可以对比两种异步结果获取方式。callback 更适合任务完成立即处理，future 更适合主线程集中等待结果。

算法解耦：
可以讲 `ImageProcessor` 的多态设计，以及如何新增一个处理器。

性能统计：
可以讲总耗时、单任务耗时和吞吐量的区别。

## 和 realtime_image_service 的衔接说法

本项目是实时图像服务的离线任务调度版本。它先解决 C++11 多线程调度、任务封装、算法解耦和性能统计问题。后续扩展到 `realtime_image_service` 时，可以把目录扫描替换为摄像头输入、ROS2 topic 或网络流输入，把 `ImageTask` 从静态图片任务扩展为实时帧任务，并继续复用线程池、处理器接口和性能统计模块。

## 简历写法

```txt
基于 C++11 实现多线程图像处理任务调度框架，支持目录图片扫描、线程池并发处理、callback/future 异步结果获取和性能统计。使用 OpenCV 实现 Gray、Sobel、Resize、Blur 等图像处理算法，并通过 ImageProcessor 抽象接口和工厂模式支持算法扩展。

- 基于生产者-消费者模型实现线程池，使用 std::thread、std::mutex、std::condition_variable 完成任务队列同步和 worker 唤醒。
- 使用 RAII 管理线程池生命周期，析构阶段自动 stop、notify_all、join，避免线程泄漏。
- 使用 unique_ptr 管理处理器对象，shared_ptr 管理跨线程任务对象，降低多线程生命周期风险。
- 支持 callback 和 future 两种异步结果获取方式，并统计总耗时、平均耗时、最大/最小耗时和吞吐量。
```
