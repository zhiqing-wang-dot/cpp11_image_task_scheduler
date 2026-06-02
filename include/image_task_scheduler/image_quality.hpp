#pragma once
#include <string>

struct QualityThresholds
{
    double sharpness_threshold;
    double dark_threshold;
    double bright_threshold;
    double contrast_threshold;
    double edge_density_threshold;

    QualityThresholds()
    : sharpness_threshold(100.0)
    , dark_threshold(40.0)
    , bright_threshold(220.0)
    , contrast_threshold(20.0)
    , edge_density_threshold(0.01)
    { }
};

struct ImageQuality 
{
    // 任务基础信息
    int task_id; 
    std::string input_path;

    // 图片是否可读
    bool readable;

    // 图片基础信息
    int width;
    int height;
    int channels;

    // 图片质量指标
    double brightness; // 亮度
    double contrast;   // 对比度
    double sharpness;  // 锐度
    double edge_density; // 边缘密度

    // 质量判断结果
    bool blurry; // 是否模糊
    bool too_dark; // 是否过暗
    bool too_bright; // 是否过亮
    bool low_contrast; // 对比度过低
    bool low_texture; // 纹理过少

    // 耗时指标
    long long cost_ms; // 处理时间（毫秒）

    ImageQuality()
    // 因为 C++ 里成员变量的真实初始化顺序，不是看你初始化列表怎么写，
    // 而是看它们在类/结构体里的声明顺序
    : task_id(-1)
    , input_path("")
    , readable(false)
    , width(0)
    , height(0)
    , channels(0)
    , brightness(0.0)
    , contrast(0.0)
    , sharpness(0.0)
    , edge_density(0.0)
    , blurry(false)
    , too_dark(false)
    , too_bright(false)
    , low_contrast(false)
    , low_texture(false)
    , cost_ms(0) {}  
};
