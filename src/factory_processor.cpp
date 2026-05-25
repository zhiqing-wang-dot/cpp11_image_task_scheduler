#include "image_task_scheduler/factory_processor.hpp"

#include "image_task_scheduler/gray_processor.hpp"
#include "image_task_scheduler/sobel_processor.hpp"
#include "image_task_scheduler/blur_processor.hpp"
#include "image_task_scheduler/resize_processor.hpp"
#include "image_task_scheduler/make_unique.hpp"

std::unique_ptr<ImageProcessor> FactoryProcessor::create(const std::string& mode)
{
    if (mode == "gray") 
    {
        return make_unique<GrayProcessor>();
    } 
    if (mode == "sobel") 
    {
        return make_unique<SobelProcessor>();
    } 
    if (mode == "resize") 
    {
        return make_unique<ResizeProcessor>();
    }
    if (mode == "blur")
    {
        return make_unique<BlurProcessor>();
    }
    
    return nullptr;
}