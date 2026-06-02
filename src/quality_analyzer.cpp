#include "image_task_scheduler/quality_analyzer.hpp"

#include <chrono>

QualityAnalyzer::QualityAnalyzer()
: thresholds_()
{ }

QualityAnalyzer::QualityAnalyzer(const QualityThresholds& thresholds)
: thresholds_(thresholds)
{ }

ImageQuality QualityAnalyzer::analyze(int task_id, 
                                      const std::string& image_path, 
                                      const cv::Mat& image) const
{
    auto time = std::chrono::steady_clock::now();
    ImageQuality quality;
    if (image.empty())
    {
        quality.task_id = task_id;
        quality.input_path = image_path;
        quality.readable = false;
        quality.cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - time).count();
        
        return quality;
    }

    quality.task_id = task_id;
    quality.input_path = image_path;
    quality.readable = true;
    quality.width = image.cols;
    quality.height = image.rows;
    quality.channels = image.channels();

    // 计算亮度、对比度、锐度、边缘密度等指标
    // 这是简单算法，且都基于灰度图的，所以先转成灰度图
    cv::Mat gray;
    if (quality.channels == 3)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }
    else if (quality.channels == 4)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    }
    else
    {
        gray = image;
    }

    // 亮度：灰度图像的平均像素值
    quality.brightness = cv::mean(gray)[0];

    // 对比度：灰度图像的标准差
    cv::Scalar mean; // 
    cv::Scalar stddev;

    cv::meanStdDev(gray, mean, stddev);
    quality.contrast = stddev[0];

    // 锐度（清晰度）：灰度图像的方差
   cv::Mat laplacian;
   // 计算拉普拉斯算子，得到图像的二阶导数， CV_64F 是为了避免溢出, 输出图的类型是 64 位浮点数
   cv::Laplacian(gray, laplacian, CV_64F);
    
   cv::Scalar laplacian_mean;
   cv::Scalar laplacian_stddev;
   cv::meanStdDev(laplacian, laplacian_mean, laplacian_stddev);
   quality.sharpness = laplacian_stddev[0] * laplacian_stddev[0]; // 锐度可以用拉普拉斯图像的方差来衡量

   // 边缘密度：边缘像素的百分比
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150); 

    quality.edge_density = static_cast<double>(cv::countNonZero(edges)) 
                            / (gray.rows * gray.cols);

    quality.blurry = quality.sharpness < thresholds_.sharpness_threshold;
    quality.too_dark = quality.brightness < thresholds_.dark_threshold;
    quality.too_bright = quality.brightness > thresholds_.bright_threshold;
    quality.low_contrast = quality.contrast < thresholds_.contrast_threshold;
    quality.low_texture = quality.edge_density < thresholds_.edge_density_threshold;

    quality.cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - time).count();
    return quality;
}
