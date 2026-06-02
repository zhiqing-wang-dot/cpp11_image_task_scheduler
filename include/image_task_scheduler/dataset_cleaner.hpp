#pragma once 

#include <vector>

#include "image_task_scheduler/image_quality.hpp"

class DatasetCleaner 
{
public:
    DatasetCleaner() = default;
    ~DatasetCleaner() = default;

    // 判断是否合格的图片
    bool is_good(const ImageQuality& quality) const;

    // 将所有照片分开
    // good_images 只存路径，后面要继续做预处理。
    // bad_images 存完整 ImageQuality，因为要知道坏在哪里。
    void split(const std::vector<ImageQuality>& results,
               std::vector<std::string>* good_images,
               std::vector<ImageQuality>* bad_images) const;

    // 写入不合格的图以及原因
    bool write_bad_images(
        const std::string& file_path,
        const std::vector<ImageQuality>& bad_images) const;

    // 获取一张 bad 图片的原因
    std::string get_bad_image_reason(const ImageQuality& quality) const;

};