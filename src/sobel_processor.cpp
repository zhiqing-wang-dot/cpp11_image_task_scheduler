#include "image_task_scheduler/sobel_processor.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "image_task_scheduler/timer.hpp"

ImageResult SobelProcessor::process(const ImageTask& task, std::size_t worker_id)
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

    // gray        灰度图
    // grad_x      x 方向梯度，也就是横向边缘变化
    // grad_y      y 方向梯度，也就是纵向边缘变化
    // abs_grad_x  x 方向梯度的绝对值图
    // abs_grad_y  y 方向梯度的绝对值图
    // grad        最终合成的边缘图
    cv::Mat gray, grad_x, grad_y, abs_grad_x, abs_grad_y, grad;
    cv::cvtColor(task.image(), gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(gray, grad_x, CV_16S, 1, 0);
    cv::Sobel(gray, grad_y, CV_16S, 0, 1);
    cv::convertScaleAbs(grad_x, abs_grad_x);
    cv::convertScaleAbs(grad_y, abs_grad_y);
    // grad = abs_grad_x * 0.5 + abs_grad_y * 0.5 + 0
    cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

    bool saved = cv::imwrite(task.output_path(), grad);
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