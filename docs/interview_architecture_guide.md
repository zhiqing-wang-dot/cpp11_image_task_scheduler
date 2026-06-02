# 项目框架与面试讲解指南

## 1. 项目定位

本项目当前版本可以概括为：

```txt
基于 C++11 和 OpenCV 的多线程图像数据集质检、清洗与预处理 Pipeline
```

项目分为两条主线：

```txt
1. 数据集质量检测与清洗主线
   多线程质检 -> report.csv -> bad_images.txt -> good_images

2. 图像预处理主线
   对 good_images 执行 gray / sobel / resize / blur
```

它体现的能力包括：

```txt
C++11 多线程
线程池
任务调度
callback / future
OpenCV 图像处理
OpenCV 图像质量指标计算
CSV 报告输出
数据集清洗
Pipeline 流程编排
模块化工程设计
```

## 2. 总体架构图

```txt
                             main.cpp
                                |
        +-----------------------+-----------------------+
        |                                               |
        v                                               v
   Config / FileUtils                            根据 mode 分流
 参数解析 / 图片扫描                                   |
                                                        |
        +-----------------------------------------------+
        |
        v
 +------+-------------------+
 |                          |
 v                          v
普通图像处理模式             数据集质量检测模式
gray/sobel/resize/blur       quality
 |                          |
 v                          v
FactoryProcessor            QualityScheduler
 |                          |
 v                          +----------------------+
ImageProcessor              |                      |
 |                          v                      v
 +--> GrayProcessor      ThreadPool          QualityAnalyzer
 +--> SobelProcessor        |                      |
 +--> ResizeProcessor       v                      v
 +--> BlurProcessor      worker 线程         ImageQuality
 |                          |                      |
 v                          v                      v
TaskScheduler          process_one()        QualityReportWriter
 |                          |                      |
 v                          +-----------> report.csv
ThreadPool
 |
 v
worker 线程
 |
 v
ImageTask -> processor->process() -> ImageResult
 |
 v
callback / future
 |
 v
Performance Summary
```

## 3. 分层架构

```txt
+--------------------------------------------------+
| Application Layer                                |
| main.cpp                                         |
+--------------------------------------------------+
| Config / File Layer                              |
| Config, FileUtils                                |
+--------------------------------------------------+
| Scheduling Layer                                 |
| ThreadPool, TaskScheduler, QualityScheduler       |
+--------------------------------------------------+
| Task / Result Model Layer                        |
| ImageTask, ImageResult, ImageQuality              |
+--------------------------------------------------+
| Algorithm Layer                                  |
| ImageProcessor, Gray/Sobel/Resize/Blur            |
| QualityAnalyzer                                  |
+--------------------------------------------------+
| Output / Utility Layer                           |
| QualityReportWriter, Logger, Timer                |
+--------------------------------------------------+
| Third-party / STL                                |
| OpenCV, C++11 STL, POSIX dirent/stat              |
+--------------------------------------------------+
```

## 4. main.cpp 的职责

`main.cpp` 不负责具体算法，它只做流程编排：

```txt
1. 解析 Config
2. 检查 input 目录
3. 创建 output 目录
4. 使用 FileUtils 扫描图片
5. 如果 mode == quality，走 QualityScheduler
6. 否则走普通图像处理流程
7. 输出性能统计
```

面试讲法：

```txt
main.cpp 是应用层入口，只负责组织流程，不直接实现图像算法和线程池细节。
普通处理任务交给 TaskScheduler，数据集质检任务交给 QualityScheduler。
```

## 5. 普通图像处理流程

```txt
命令行参数
    |
    v
Config
    |
    v
FileUtils 扫描图片
    |
    v
FactoryProcessor 根据 mode 创建处理器
    |
    v
TaskScheduler
    |
    v
ThreadPool
    |
    v
worker 执行 processor->process()
    |
    v
ImageResult
    |
    v
callback 或 future 返回结果
```

### 关键类

`ImageTask`

```txt
封装一张图片任务：
task_id
cv::Mat image
input_path
output_path
```

`ImageResult`

```txt
封装任务执行结果：
success
error_message
cost_ms
worker_id
input_path
output_path
```

`ImageProcessor`

```txt
图像算法抽象接口。
TaskScheduler 只依赖 ImageProcessor，不依赖具体处理器。
```

`FactoryProcessor`

```txt
根据 mode 字符串创建具体处理器。
```

支持：

```txt
gray
sobel
resize
blur
```

## 6. ThreadPool 线程池设计

线程池是项目核心之一。

### 核心成员

```txt
workers_      保存 worker 线程
tasks_        保存待执行任务
queue_mutex_  保护任务队列
condition_    worker 等待和唤醒
stop_         停止标志
```

### 工作流程

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
submit 加锁压入任务队列
    |
    v
condition_.notify_one()
    |
    v
worker 被唤醒
    |
    v
取出任务
    |
    v
释放锁
    |
    v
执行任务
```

### 为什么执行任务前释放锁

```txt
锁只用于保护任务队列。
图像处理任务可能耗时较长，如果执行任务时仍然持有锁，
其他 worker 就无法取任务，线程池会失去并发效果。
```

### 安全退出

```txt
stop()
    |
    v
设置 stop_ = true
    |
    v
notify_all 唤醒所有 worker
    |
    v
worker 发现 stop_ 且任务队列为空
    |
    v
退出循环
    |
    v
主线程 join 所有 worker
```

面试重点：

```txt
std::thread 析构前如果仍然 joinable 会触发 std::terminate，
所以 ThreadPool 析构时必须 stop 并 join 所有线程。
```

## 7. TaskScheduler 设计

`TaskScheduler` 是普通图像处理模式的调度器。

它连接：

```txt
ThreadPool
ImageTask
ImageProcessor
callback / future
```

### callback 模式

```txt
submit(task, callback)
    |
    v
任务进入 ThreadPool
    |
    v
worker 调用 processor_->process(*task, worker_id)
    |
    v
得到 ImageResult
    |
    v
callback(result)
```

callback 模式中多个 worker 会同时更新统计变量，因此：

```txt
finished_count / success_count / failed_count 使用 std::atomic<int>
costs vector 使用 std::mutex 保护
```

### future 模式

```txt
submit_future(task)
    |
    v
创建 promise / future
    |
    v
worker 处理图片
    |
    v
promise->set_value(result)
    |
    v
main 通过 future.get() 获取结果
```

面试讲法：

```txt
callback 更适合任务完成时立即处理结果；
future 更适合主线程集中等待和汇总结果。
```

## 8. 数据集质量检测扩展

质量检测模式对应：

```txt
--mode quality
```

目标是解决视觉模型训练前的数据清洗问题：

```txt
损坏图
模糊图
过暗图
过曝图
低对比度图
低纹理图
```

## 9. QualityScheduler 设计

`QualityScheduler` 是数据集质检流程的组织者。

它连接：

```txt
ThreadPool
QualityAnalyzer
QualityReportWriter
```

流程：

```txt
QualityScheduler::run(image_files)
    |
    v
打开 report.csv
    |
    v
写 CSV 表头
    |
    v
提交每张图片到 ThreadPool
    |
    v
worker 执行 process_one()
    |
    +--> cv::imread
    +--> QualityAnalyzer::analyze
    +--> QualityReportWriter::write_row
    +--> 更新统计
    |
    v
等待全部完成
    |
    v
关闭 report.csv
    |
    v
输出 Quality Summary
```

为什么单独设计 `QualityScheduler`：

```txt
数据集质检不是普通 ImageProcessor 任务。
它除了分析图片，还需要写 CSV、统计异常和生成报告。
单独拆出来能让 main.cpp 更干净，也让职责边界更清晰。
```

## 10. QualityAnalyzer 指标原理

`QualityAnalyzer` 只负责 OpenCV 指标计算。

### brightness 亮度

```txt
brightness = 灰度图所有像素的平均值
```

OpenCV：

```cpp
cv::mean(gray)[0]
```

含义：

```txt
数值越低，图像越暗。
数值越高，图像越亮。
```

### contrast 对比度

```txt
contrast = 灰度图像素标准差
```

OpenCV：

```cpp
cv::meanStdDev(gray, mean, stddev);
contrast = stddev[0];
```

含义：

```txt
标准差越大，明暗变化越明显，对比度越高。
标准差越小，图像越灰、越平。
```

### sharpness 清晰度

```txt
sharpness = Laplacian 响应方差
```

OpenCV：

```cpp
cv::Laplacian(gray, laplacian, CV_64F);
cv::meanStdDev(laplacian, mean, stddev);
sharpness = stddev[0] * stddev[0];
```

原理：

```txt
清晰图像边缘变化明显，Laplacian 响应变化大，方差大。
模糊图像边缘被平滑，Laplacian 响应变化小，方差小。
```

### edge_density 边缘密度

```txt
edge_density = Canny 边缘像素数量 / 图像总像素数量
```

OpenCV：

```cpp
cv::Canny(gray, edges, 50, 150);
edge_density = countNonZero(edges) / total_pixels;
```

含义：

```txt
边缘密度低，说明图像纹理和结构较少，可能是纯色、过曝、过暗或严重模糊。
```

## 11. QualityReportWriter 设计

职责：

```txt
线程安全写 report.csv
```

为什么需要 mutex：

```txt
多个 worker 可能同时写 CSV。
如果不加锁，不同线程写出的内容可能交叉，导致 CSV 文件损坏。
```

CSV 字段：

```txt
task_id,input_path,readable,width,height,channels,
brightness,contrast,sharpness,edge_density,
blurry,too_dark,too_bright,low_contrast,low_texture,cost_ms
```

## 12. 常见面试问题

### 为什么 ImageTask 用 shared_ptr？

```txt
ImageTask 会跨线程传递。
主线程提交任务后，worker 线程异步执行。
shared_ptr 可以保证任务对象在 worker 使用期间不会被提前释放。
```

### 为什么 ImageProcessor 用 unique_ptr？

```txt
处理器对象由 TaskScheduler 独占管理，不需要共享所有权。
unique_ptr 能表达清晰的生命周期。
```

### 为什么线程池任务类型是 std::function<void(std::size_t)>？

```txt
因为 worker 执行任务时会传入 worker_id，
这样任务结果或日志可以知道由哪个 worker 执行。
```

### 为什么 condition_variable 使用谓词版本？

```txt
可以处理虚假唤醒。
worker 只有在 stop_ 为 true 或任务队列非空时才继续执行。
```

### 为什么 QualityAnalyzer 的 image 参数是 const cv::Mat&？

```txt
质量分析只读取图片，不应该修改原图。
const cv::Mat& 既避免拷贝，又表达只读语义。
```

### 为什么 QualityScheduler 不直接写在 main.cpp？

```txt
main.cpp 应该只做流程分发。
QualityScheduler 封装数据集质检调度逻辑，使 main 更干净，也方便后续扩展 bad_images.txt、阈值配置和更多质量指标。
```

## 13. 面试讲解顺序

推荐按这个顺序讲：

```txt
1. 项目背景：批量图片处理和数据集质检
2. 总体架构：main 分流普通处理和 quality 模式
3. ThreadPool：生产者消费者模型、condition_variable、安全退出
4. TaskScheduler：连接任务、处理器、线程池，支持 callback/future
5. ImageProcessor：多态和工厂模式扩展算法
6. QualityScheduler：数据集质检调度器
7. QualityAnalyzer：亮度、对比度、清晰度、边缘密度原理
8. QualityReportWriter：线程安全写 CSV
9. Performance Summary：耗时、平均耗时、吞吐量
```

## 14. 一句话总结

```txt
这个项目底层用 C++11 线程池实现批量任务并发调度，上层通过 ImageProcessor 多态扩展 OpenCV 图像处理算法，并进一步扩展 QualityScheduler 实现多线程图像数据集质量检测和 CSV 报告输出。
```

## 15. 当前版本简历写法

下面这版更贴合现在的 `image_task_scheduler` 项目：它已经从“批量图像处理 demo”升级成了“数据集质检 + 清洗 + 预处理 Pipeline”。

### 15.1 简历项目名称

推荐写法：

```txt
基于 C++11 的多线程图像数据集质检与预处理 Pipeline
```

如果岗位偏 C++ 后端 / Linux C++：

```txt
C++11 多线程图像任务调度与数据集清洗框架
```

如果岗位偏视觉 / 算法部署：

```txt
基于 OpenCV 的图像数据集质量检测与并发预处理系统
```

### 15.2 简历一句话描述

```txt
基于 C++11、OpenCV 和 CMake 实现图像数据集质检与预处理 Pipeline，支持目录图片扫描、多线程质量评估、坏图筛选、CSV 报告生成，并对合格图片并发执行 Gray/Sobel/Resize/Blur 等预处理任务。
```

### 15.3 技术栈

```txt
C++11、OpenCV、CMake、Linux、STL、std::thread、std::mutex、std::condition_variable、std::future、std::promise、std::atomic、std::unique_ptr、std::shared_ptr、lambda、RAII
```

### 15.4 简历项目描述：精简版

适合简历空间有限时使用：

```txt
基于 C++11 实现多线程图像数据集质检与预处理 Pipeline，支持批量扫描图片、并发计算亮度/对比度/清晰度/边缘密度等质量指标，输出 report.csv 和 bad_images.txt，并对合格图片执行 Gray、Sobel、Resize、Blur 等 OpenCV 预处理。项目通过线程池、任务队列、callback/future、工厂模式和 RAII 实现模块化调度与资源安全管理。
```

### 15.5 简历项目描述：详细版

适合放在项目经历中：

```txt
基于 C++11 和 OpenCV 设计并实现图像数据集质检与预处理 Pipeline。系统首先扫描输入目录图片，通过 QualityScheduler 将质量检测任务提交到线程池，并由 QualityAnalyzer 计算亮度、对比度、Laplacian 清晰度、Canny 边缘密度等指标，生成 report.csv；随后使用 DatasetCleaner 根据阈值筛选 bad images，输出 bad_images.txt，并仅对合格图片执行后续预处理。预处理阶段通过 TaskScheduler 调度 ImageTask，基于 ImageProcessor 抽象接口和 FactoryProcessor 工厂模式支持 Gray、Sobel、Resize、Blur 等算法扩展，同时支持 callback 和 std::future 两种异步结果获取方式，并统计任务耗时和吞吐量。
```

### 15.6 简历亮点 Bullet

推荐放 4 到 6 条，不要全部堆上去：

```txt
- 基于生产者-消费者模型实现 C++11 线程池，使用 std::mutex 和 std::condition_variable 完成任务队列同步、worker 唤醒和安全退出。
- 设计 DatasetPipline 串联数据准备、质量检测、坏图清洗和图像预处理阶段，将 main.cpp 简化为参数解析和流程启动。
- 使用 QualityAnalyzer 计算 brightness、contrast、sharpness、edge_density 等质量指标，识别模糊、过暗、过曝、低对比度和低纹理图片。
- 使用 QualityReportWriter 线程安全写入 report.csv，并通过 DatasetCleaner 输出 bad_images.txt，便于训练数据集清洗和问题图片追踪。
- 通过 ImageProcessor 抽象接口和 FactoryProcessor 工厂模式解耦调度框架与具体算法，支持 Gray、Sobel、Resize、Blur 等处理器扩展。
- 支持 callback 和 std::future 两种异步结果获取方式，使用 std::atomic 和 mutex 保证多线程统计结果正确。
- 使用 RAII 管理线程池生命周期，在析构阶段 stop、notify_all 并 join worker，避免线程泄漏和 std::terminate。
- 使用 std::chrono 统计总耗时、单任务耗时和吞吐量，为不同线程数下的性能对比提供依据。
```

### 15.7 更像企业项目的写法

如果你想突出工程化，而不是只说“用了 OpenCV”，可以这样写：

```txt
项目重点不是单个图像算法，而是围绕数据集处理流程做工程化拆分：Config 负责参数和阈值配置，FileUtils 负责目录扫描和输出路径构造，QualityScheduler 负责并发质检调度，DatasetCleaner 负责合格/不合格样本拆分，TaskScheduler 负责预处理任务调度，ImageProcessor 负责算法扩展。各模块职责清晰，便于后续扩展更多质量指标、清洗策略和预处理算子。
```

### 15.8 面试时的项目介绍

```txt
这个项目是一个离线图像数据集处理 Pipeline。我把它分成两个阶段：第一阶段做数据集质检，线程池并发读取图片并计算亮度、对比度、清晰度和边缘密度，输出 report.csv；然后 DatasetCleaner 根据阈值筛出坏图，生成 bad_images.txt。第二阶段只对合格图片做预处理，TaskScheduler 把每张图片封装成 ImageTask，交给线程池执行具体 ImageProcessor，比如 Gray、Sobel、Resize、Blur。这样既能体现 C++11 多线程调度，也能解决视觉训练前的数据清洗问题。
```

### 15.9 和 VisionReactorServer 的衔接写法

如果你的简历里同时放 `VisionReactorServer`，可以这样区分两个项目：

```txt
image_task_scheduler 是离线图像数据集处理项目，重点是 C++11 多线程任务调度、数据集质检和预处理 Pipeline；
VisionReactorServer 是在线视觉推理服务项目，重点是 Linux 网络编程、epoll Reactor、自定义协议和 ONNX Runtime YOLO 推理。
两个项目可以形成从离线数据处理到在线模型推理服务的完整视觉工程链路。
```

### 15.10 最推荐放到简历里的最终版本

```txt
基于 C++11 的多线程图像数据集质检与预处理 Pipeline
技术栈：C++11、OpenCV、CMake、Linux、STL、std::thread、std::future、RAII

- 设计并实现图像数据集处理 Pipeline，支持目录图片扫描、质量检测、坏图筛选和合格图片预处理，输出 report.csv 与 bad_images.txt。
- 基于生产者-消费者模型实现线程池，使用 mutex、condition_variable 和任务队列完成并发调度，并在析构阶段安全 stop/join worker。
- 使用 QualityAnalyzer 计算亮度、对比度、Laplacian 清晰度、Canny 边缘密度等指标，识别模糊、过暗、过曝、低对比度和低纹理图片。
- 通过 ImageProcessor 抽象接口和 FactoryProcessor 工厂模式解耦算法模块，支持 Gray、Sobel、Resize、Blur 等 OpenCV 预处理算子扩展。
- 支持 callback 和 std::future 两种异步结果获取方式，并统计总耗时、平均耗时、最大/最小耗时和吞吐量。
```
