/* ImageProcessor 抽象基类。后面 GrayProcessor / SobelProcessor / ResizeProcessor / BlurProcessor 的统一接口*/
#pragma once

#include "image_task_scheduler/image_task.hpp"
#include "image_task_scheduler/image_result.hpp"

class ImageProcessor
{
public:
    ImageProcessor() = default;
    virtual ~ImageProcessor() = default;

    // 纯虚函数，子类必须实现
    virtual ImageResult process(const ImageTask& task, std::size_t worker_id) = 0;
};