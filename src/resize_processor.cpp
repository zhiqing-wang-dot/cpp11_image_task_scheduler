#include "image_task_scheduler/resize_processor.hpp"

#include <opencv2/imgproc.hpp> // cv::resize
#include <opencv2/imgcodecs.hpp> // cv::imwrite

#include "image_task_scheduler/timer.hpp"
ImageResult ResizeProcessor::process(const ImageTask& task, std::size_t worker_id)
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

    cv::Mat resized;
    cv::resize(task.image(), resized, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR);
    result.success = true;
    result.cost_ms = timer.elapsed_ms();

    bool saved = cv::imwrite(task.output_path(), resized);
    if (!saved)
    {
        result.success = false;
        result.error_message = "failed to save resized image";
        result.cost_ms = timer.elapsed_ms();
        return result;
    }

    result.success = true;
    result.cost_ms = timer.elapsed_ms();
    return result;
}