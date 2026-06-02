#include "image_task_scheduler/dataset_cleaner.hpp"

#include <fstream>
#include <string>

bool DatasetCleaner::is_good(const ImageQuality& quality) const
{
    return quality.readable &&
           !quality.blurry &&
           !quality.too_dark &&
           !quality.too_bright &&
           !quality.low_contrast &&
           !quality.low_texture;
}

// 指针版本适合这种场景：
// 这个输出参数是可选的, 不想要的可以传 nullptr
// 有时候我只想要 good_images
// 有时候我只想要 bad_images
// 有时候两个都要
void DatasetCleaner::split(const std::vector<ImageQuality>& results,
                           std::vector<std::string>* good_images,
                           std::vector<ImageQuality>* bad_images) const
{
    if (good_images != nullptr)
    {
        good_images->clear();
    }

    if (bad_images != nullptr)
    {
        bad_images->clear();
    }

    for (const auto& quality : results)
    {
        if (is_good(quality))
        {
            if (good_images != nullptr)
            {
                good_images->push_back(quality.input_path);
            }
        }
        else
        {
            if (bad_images != nullptr)
            {
                bad_images->push_back(quality);
            }
        }
    }
}

std::string DatasetCleaner::get_bad_image_reason(const ImageQuality& quality) const
{

    if (!quality.readable)
    {
        return "not_readable";
    }
    else if (quality.blurry)
    {
        return "blurry";
    }
    else if (quality.too_dark)
    {
        return "too_dark";
    }
    else if (quality.too_bright)
    {
        return "too_bright";
    }
    else if (quality.low_contrast)
    {
        return "low_contrast";
    }
    else if (quality.low_texture)
    {
        return "low_texture";
    }
    else
    {
        return "unknown";
    } 
}

// 将bad_images 写入文件，并指出原因
bool DatasetCleaner::write_bad_images(
    const std::string& file_path,
    const std::vector<ImageQuality>& bad_images) const
{
    std::ofstream file(file_path);

    if (!file.is_open())
    {
        return false;
    }

    file << "image_path,reason" << std::endl; 

    for (const auto& quality : bad_images)
    {
        file << quality.input_path << "," << get_bad_image_reason(quality) << "\n";
    }

    return true;
}
