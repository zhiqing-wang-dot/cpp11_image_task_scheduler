#pragma once 

#include "image_task_scheduler/image_processor.hpp"

class FactoryProcessor 
: public ImageProcessor 
{
public:
    static std::unique_ptr<ImageProcessor> create(const std::string& mode);
};