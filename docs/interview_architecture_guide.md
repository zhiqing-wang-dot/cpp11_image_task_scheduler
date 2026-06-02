# 项目框架与面试讲解指南

## 1. 项目定位

本项目可以概括为：

```txt
基于 C++11 和 OpenCV 的多线程图像处理与数据集质量检测框架
```

项目分为两条主线：

```txt
1. 图像处理主线
   gray / sobel / resize / blur

2. 数据集质量检测主线
   quality 模式，输出 report.csv
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
