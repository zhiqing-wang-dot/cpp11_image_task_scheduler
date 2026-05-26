#include "image_task_scheduler/file_utils.hpp"

#include <algorithm> // for std::transform
#include <sys/stat.h> // for stat
#include <dirent.h> // for opendir, readdir, closedir

namespace
{

// 将字符串转换为小写    
std::string to_lower(std::string str)
{
    // ::tolower 是一个 C 标准库函数，用于将单个字符转换为小写。
    // std::transform 是一个算法函数，可以对字符串中的每个字符应用这个转换。
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// 判断文件是否是图片文件
bool is_image_file(const std::string& path)
{
    std::string lower_path = to_lower(path);
    // rfind(".jpg") 从字符串右边开始查找 .jpg 出现的位置
    return lower_path.size() > 4 &&
           (lower_path.rfind(".jpg") == lower_path.size() - 4 ||
            lower_path.rfind(".png") == lower_path.size() - 4 ||
            lower_path.rfind(".bmp") == lower_path.size() - 4 ||
            (lower_path.size() > 5 && lower_path.rfind(".jpeg") == lower_path.size() - 5));
            
}

} // namespace

namespace FileUtils
{

bool exists(const std::string& path)
{
    // struct stat 
    //{
    // dev_t     st_dev;     // 文件所在设备
    // ino_t     st_ino;     // inode 编号
    // mode_t    st_mode;    // 文件类型和权限
    // nlink_t   st_nlink;   // 硬链接数量
    // uid_t     st_uid;     // 所有者用户 ID
    // gid_t     st_gid;     // 所属组 ID
    // off_t     st_size;    // 文件大小
    // time_t    st_mtime;   // 最后修改时间
    // ...
    struct stat info;
    // stat 函数获取文件信息，存入结构体
    return stat(path.c_str(), &info) == 0;
}

bool is_directory(const std::string& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    // S_ISDIR(info.st_mode) 是一个宏，用于判断文件类型是否为目录。
    return S_ISDIR(info.st_mode);
}

bool create_directory_if_not_exists(const std::string& path)
{
    if (is_directory(path))
    {
        return true; // 目录已存在
    }

    if (exists(path))
    {
        return false; // 存在但不是目录
    }

    if (mkdir(path.c_str(), 0755) != 0) 
    {
        return false; // 创建目录失败
    }
    
    return true;
}

std::vector<std::string> list_image_files(const std::string& input_dir)
{
    std::vector<std::string> image_files; // 默认构造一个空数组
    DIR*dir = opendir(input_dir.c_str());
    if (dir == nullptr)
    {
        return image_files;
    }

    // struct dirent
    // {
    //     ino_t          d_ino;       // inode 编号
    //     off_t          d_off;       // 目录项偏移，一般很少直接用
    //     unsigned short d_reclen;    // 当前目录项记录长度
    //     unsigned char  d_type;      // 文件类型
    //     char           d_name[];    // 文件名
    // };
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string name(entry->d_name);
        
        if (name == "." || name == "..")
        {
            continue; // 跳过当前目录和父目录
        }

        std::string full_path = input_dir + "/" + name;
        // 判断是否是文件且是图片文件
        if (!is_directory(full_path) && is_image_file(full_path))
        {
            image_files.push_back(full_path);
        }
    }

    closedir(dir); // 关闭目录流
    std::sort(image_files.begin(), image_files.end()); // 按照字节序排序文件路径
    return image_files;
}

// 从文件路径中提取文件名（不带扩展名）
std::string get_file_name_without_extension(const std::string& file_path)
{
    // \ 在 C++ 字符串里需要转义，所以要写成 "/\\"，实际上就是在查找 / 或 \ 这两种路径分隔符
    std::size_t slash_pos = file_path.find_last_of("/\\");
    std::string file_name = slash_pos == std::string::npos ? file_path : file_path.substr(slash_pos + 1);

    std::size_t dot_pos = file_name.find_last_of(".");
    if (dot_pos == std::string::npos)
    {
        return file_name;
    }

    return file_name.substr(0, dot_pos);
}

std::string build_output_path(const std::string& output_dir, const std::string& input_dir, const std::string& mode)
{
    return output_dir + "/" + get_file_name_without_extension(input_dir) + "_" + mode + ".jpg";
}

}
