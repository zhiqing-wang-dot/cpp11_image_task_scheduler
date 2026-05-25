#include "image_task_scheduler/image_task.hpp"


ImageTask::ImageTask(int id, cv::Mat image, std::string input_path, std::string output_path)
: id_(id)
, image_(std::move(image))
, input_path_(std::move(input_path))
, output_path_(std::move(output_path))
{}

int ImageTask::task_id() const
{
    return id_;
}

const cv::Mat& ImageTask::image() const
{
    return image_;
}

const std::string& ImageTask::input_path() const
{
    return input_path_;
}

const std::string& ImageTask::output_path() const
{
    return output_path_;
}