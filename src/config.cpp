#include "image_task_scheduler/config.hpp"

#include <cstdlib>
#include <iostream>

namespace 
{
bool para_int (const std::string& text, int* value)
{
    char* end = nullptr;
    // std::strtol 函数将字符串转换为长整数，参数 10 表示使用十进制
    long result = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') // 如果一个数字都没有被转换，或者转换后还有剩余字符
    {
        return false; // 转换失败，或者结果不是正整数
    }
    *value = static_cast<int>(result);
    return true;
}

bool para_bool (const std::string& text, bool* value)
{
    if (text == "true")
    {
        *value = true;
        return true;
    }
    if (text == "false")
    {
        *value = false;
        return true;
    }
    return false;
}

bool para_double(const std::string& text, double* value)
{
    char* end = nullptr;
    double result = std::strtod(text.c_str(), &end);

    if (end == text.c_str() || *end != '\0')
    {
        return false;
    }

    *value = result;
    return true;
}
} // namespace 

Config::Config()
: input_dir("./images")
, output_dir("./output")
, thread_count(4)
, mode("sobel")
, use_future(false) 
, enable_quality(true)
, quality_thresholds()
{ }

bool Config::parse(int argc, char** argv, Config* config)
{
    if (config == nullptr)
    {
        return false;
    }

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i]; // 将参数转换为 std::string 以便比较
        if (i + 1 >= argc) // i 是下标， argc 是参数个数
        {
            print_usage(); 
            return false;
        }

        std::string value = argv[++i];
        if (arg == "--input")
        {
            config->input_dir = value;
        }
        else if (arg == "--output")
        {
            config->output_dir = value;
        }
        else if (arg == "--threads")
        {
            if (!para_int(value, &config->thread_count))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--mode")
        {
            config->mode = value;
        }
        else if (arg == "--use_future")
        {
            if (!para_bool(value, &config->use_future))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--enable_quality")
        {
            if (!para_bool(value, &config->enable_quality))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--sharpness_threshold")
        {
            if (!para_double(value, &config->quality_thresholds.sharpness_threshold))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--dark_threshold")
        {
            if (!para_double(value, &config->quality_thresholds.dark_threshold))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--bright_threshold")
        {
            if (!para_double(value, &config->quality_thresholds.bright_threshold))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--contrast_threshold")
        {
            if (!para_double(value, &config->quality_thresholds.contrast_threshold))
            {
                print_usage();
                return false;
            }
        }
        else if (arg == "--edge_density_threshold")
        {
            if (!para_double(value, &config->quality_thresholds.edge_density_threshold))
            {
                print_usage();
                return false;
            }
        }
        else
        {
            print_usage();
            return false;
        }
    }

    // 验证模式和线程数是否合法
    if (!config->is_valid_mode() || config->thread_count <= 0)
    {
        print_usage();
        return false;
    }
    
    return true;
}

void Config::print_usage() // 打印使用说明
{
    std::cout << "Usage:\n"
              << "  image_task_scheduler "
              << "--input ./images "
              << "--output ./output "
              << "--threads 4 "
              << "--mode sobel "
              << "--use_future false "
              << "--enable_quality true "
              << "--sharpness_threshold 100 "
              << "--dark_threshold 40 "
              << "--bright_threshold 220 "
              << "--contrast_threshold 20 "
              << "--edge_density_threshold 0.01\n"
              << "\n"
              << "Modes: gray, sobel, resize, blur\n";
}

bool Config::is_valid_mode() const
{
    return mode == "sobel" || 
            mode == "gray" || 
            mode == "blur" || 
            mode == "resize";
}
