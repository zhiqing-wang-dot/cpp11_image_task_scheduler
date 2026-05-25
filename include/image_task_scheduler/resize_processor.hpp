#pragma once

#include "image_task_scheduler/image_processor.hpp"

class ResizeProcessor 
: public ImageProcessor
{
public:
    ImageResult process(const ImageTask& task, std::size_t worker_id) override;
};