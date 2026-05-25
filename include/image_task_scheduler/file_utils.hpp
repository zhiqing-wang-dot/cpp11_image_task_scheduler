#pragma once 

#include <string>
#include <vector>

namespace FileUtils
{

bool exists(const std::string& path);
bool is_directory(const std::string& path);
bool create_directory_if_not_exists(const std::string& path);
std::vector<std::string> list_image_files(const std::string& input_dir);
std::string get_file_name_without_extension(const std::string& file_path);
std::string build_output_path(const std::string& output_dir, const std::string& input_dir, const std::string& mode); 

} // namespace FileUtils