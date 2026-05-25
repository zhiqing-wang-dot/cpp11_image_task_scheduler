#include "image_task_scheduler/gray_processor.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "image_task_scheduler/timer.hpp"

ImageResult GrayProcessor::process(const ImageTask& task, std::size_t worker_id)
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

    cv::Mat gray;
    // cv::cvtColor 的第四个参数 dstCn 是目标图像的通道数，默认为 0，表示自动推断。对于灰度图像，通道数应该是 1，所以这里显式指定为 0。
    cv::cvtColor(task.image(), gray, cv::COLOR_BGR2GRAY);

    bool saved = cv::imwrite(task.output_path(), gray);
    if (!saved)
    {
        result.success = false;
        result.error_message = "failed to save output image";
        result.cost_ms = timer.elapsed_ms();
        return result;
    }

    result.success = true;
    result.cost_ms = timer.elapsed_ms();
    return result;
}