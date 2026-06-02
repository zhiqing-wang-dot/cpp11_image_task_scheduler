// QualityAnalyzer 的职责是只用 opencv 做指标计算
#pragma once

#include <string>
#include <opencv2/opencv.hpp>

#include "image_task_scheduler/image_quality.hpp"

class QualityAnalyzer 
{
public:
    QualityAnalyzer();
    explicit QualityAnalyzer(const QualityThresholds& thresholds);
    ~QualityAnalyzer() = default;
    // 分析图片的各类特征并根据阈值给出结论
    ImageQuality analyze(int task_id, const std::string& image_path, const cv::Mat& image) const;

private:
    QualityThresholds thresholds_;
};
