#include "image_task_scheduler/blur_processor.hpp"

#include <opencv2/imgproc.hpp> // For GaussianBlur 
#include <opencv2/imgcodecs.hpp> // For imread and imwrite

#include "image_task_scheduler/timer.hpp"

ImageResult BlurProcessor::process(const ImageTask& task, std::size_t worker_id)
{
    Timer timer;

    ImageResult result;
    result.task_id = task.task_id();
    result.input_path = task.input_path();
    result.output_path = task.output_path();
    result.worker_id = worker_id;

    if (task.image().empty()) 
    {
        result.success = false;
        result.error_message = "input image is empty";
        result.cost_ms = timer.elapsed_ms();
        return result;
    }

    cv::Mat blurred;
    // 高斯核的宽和高通常必须是正奇数，可以根据需要调整
    cv::GaussianBlur(task.image(), blurred, cv::Size(5, 5), 0);

    bool saved = cv::imwrite(task.output_path(), blurred);
    if (!saved) 
    {
        result.success = false;
        result.error_message = "failed to save blurred image";
        result.cost_ms = timer.elapsed_ms();
        return result;
    }

    result.success = true;
    result.cost_ms = timer.elapsed_ms();
    return result;

}

