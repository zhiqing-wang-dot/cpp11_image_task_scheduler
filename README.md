# cpp11_image_task_scheduler

## 项目简介

项目名称：基于 C++11 的多线程图像处理任务调度框架

本项目使用 C++11、OpenCV 和 CMake 实现一个批量图像处理任务调度框架。程序可以扫描指定目录下的图片，将每张图片封装为 `ImageTask`，通过线程池并发执行图像处理任务，并支持 callback 和 `std::future` 两种结果获取方式。

项目重点不是复杂图像算法，而是 C++11 工程能力：线程池、任务队列、RAII、智能指针、多态、工厂模式、异步结果和性能统计。

## 项目功能

- 批量扫描输入目录中的 `.jpg`、`.jpeg`、`.png`、`.bmp` 图片。
- 支持灰度转换、Sobel 边缘检测、Resize、GaussianBlur。
- 使用 `ImageProcessor` 抽象基类解耦算法实现。
- 使用 `FactoryProcessor` 根据 `mode` 创建不同处理器。
- 使用 `ThreadPool` 并发执行任务。
- 使用 `TaskScheduler` 连接任务、线程池和处理器。
- 支持 callback 模式和 `std::future<ImageResult>` 模式。
- 输出总任务数、成功数、失败数、总耗时、平均耗时、最大/最小耗时和吞吐量。

## 技术栈

C++11、OpenCV、CMake、STL、Linux POSIX 文件接口、`std::thread`、`std::mutex`、`std::condition_variable`、`std::future`、`std::promise`、`std::unique_ptr`、`std::shared_ptr`、lambda、RAII。

## 目录结构

```txt
cpp11_image_task_scheduler/
├── CMakeLists.txt
├── README.md
├── include/image_task_scheduler/
│   ├── config.hpp
│   ├── image_task.hpp
│   ├── image_result.hpp
│   ├── image_processor.hpp
│   ├── gray_processor.hpp
│   ├── sobel_processor.hpp
│   ├── resize_processor.hpp
│   ├── blur_processor.hpp
│   ├── factory_processor.hpp
│   ├── thread_pool.hpp
│   ├── task_scheduler.hpp
│   ├── logger.hpp
│   ├── timer.hpp
│   ├── file_utils.hpp
│   └── make_unique.hpp
├── src/
│   ├── main.cpp
│   ├── config.cpp
│   ├── image_task.cpp
│   ├── gray_processor.cpp
│   ├── sobel_processor.cpp
│   ├── resize_processor.cpp
│   ├── blur_processor.cpp
│   ├── factory_processor.cpp
│   ├── thread_pool.cpp
│   ├── task_scheduler.cpp
│   ├── logger.cpp
│   ├── timer.cpp
│   └── file_utils.cpp
├── images/
├── output/
└── docs/
    ├── architecture.md
    ├── cpp11_knowledge.md
    └── resume_description.md
```

## 编译方法

需要先安装 OpenCV 开发包：

```bash
sudo apt install libopencv-dev
```

编译：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 运行方法

callback 模式：

```bash
./build/image_task_scheduler \
  --input ./images \
  --output ./output \
  --threads 4 \
  --mode gray \
  --use_future false
```

future 模式：

```bash
./build/image_task_scheduler \
  --input ./images \
  --output ./output \
  --threads 4 \
  --mode sobel \
  --use_future true
```

如果 `images/` 目录没有图片，请将测试图片放到：

```txt
./images/
```

处理结果会输出到：

```txt
./output/
```

## 参数说明

```txt
--input       输入图片目录，默认 ./images
--output      输出图片目录，默认 ./output
--threads     工作线程数，默认 4
--mode        图像处理模式：gray / sobel / resize / blur
--use_future  是否使用 future 模式：true / false
```

## 示例输出

```txt
========== Performance Summary ==========
total tasks      : 185
success tasks    : 185
failed tasks     : 0
total wall cost  : 32146 ms
avg task cost    : 62.3243 ms
max task cost    : 130 ms
min task cost    : 48 ms
throughput       : 5.75499 images/s
=========================================
```

## 如何扩展新算法

1. 新建处理器类，例如 `SharpenProcessor`。
2. 继承 `ImageProcessor`。
3. 实现：

```cpp
ImageResult process(const ImageTask& task, std::size_t worker_id) override;
```

4. 在工厂中增加新的 mode 分支。
5. 在 CMake 中加入新的 `.cpp` 文件。

## C++11 知识点

项目中使用了线程、锁、条件变量、智能指针、lambda、`std::function`、`std::future`、`std::promise`、`std::atomic`、`std::chrono`、RAII、多态和工厂模式。详细说明见 [docs/cpp11_knowledge.md](docs/cpp11_knowledge.md)。

## 和 realtime_image_service 的关系

本项目可以作为 `realtime_image_service` 前置训练项目：它先在离线批处理场景中练习 C++11 多线程调度、任务抽象、算法解耦和性能统计。后续接入实时图像服务时，可以将 `ImageTask` 替换为实时帧任务，将目录扫描替换为摄像头、ROS2 topic 或网络输入。

## 简历项目描述

项目名称：基于 C++11 的多线程图像处理任务调度框架

项目描述：
基于 C++11 实现面向图像处理任务的异步调度框架，支持批量图像读取、线程池并发处理、任务结果回调、算法模块插件化扩展和处理耗时统计。项目使用 OpenCV 完成灰度、Sobel、Resize、Blur 等基础图像处理任务，并通过 CMake 进行模块化构建。

技术栈：
C++11、OpenCV、CMake、STL、std::thread、std::mutex、std::condition_variable、std::future、std::unique_ptr、std::shared_ptr、lambda、RAII

项目亮点：
1. 基于生产者-消费者模型实现线程池，使用 std::thread 管理 worker 线程，使用 std::mutex 和 std::condition_variable 保证任务队列线程安全。
2. 使用 std::unique_ptr 管理算法处理器对象，使用 std::shared_ptr 管理跨线程传递的任务对象，避免裸指针导致的生命周期问题。
3. 通过 std::function 和 lambda 实现任务完成回调，使任务执行逻辑与结果处理逻辑解耦。
4. 将图像处理算法抽象为 ImageProcessor 接口，通过工厂模式支持 Sobel、Gray、Resize、Blur 等算法模块扩展。
5. 使用 RAII 思想管理线程池生命周期，在析构阶段自动通知 worker 线程退出并完成 join，保证资源安全释放。
6. 使用 std::chrono 统计单任务耗时、平均耗时和整体吞吐量，为后续接入实时图像处理链路提供性能参考。
