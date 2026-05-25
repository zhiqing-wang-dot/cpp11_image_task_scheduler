#pragma once

#include <string>
#include <opencv2/opencv.hpp>

class ImageTask
{
public:
    ImageTask(int id, cv::Mat image, std::string input_path, std::string output_path);

    int task_id() const;
    const cv::Mat& image() const;
    const std::string& input_path() const;
    const std::string& output_path() const;

private:
    int id_;
    cv::Mat image_;
    std::string input_path_;
    std::string output_path_;
};

