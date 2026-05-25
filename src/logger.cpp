#include <iostream>
#include "image_task_scheduler/logger.hpp"

Logger& Logger::instance() 
{
    static Logger instance;
    return instance;
}

void Logger::log(Level level, const std::string& message) 
{
    mutex_.lock();
    switch (level)
    {
    case Level::INFO:
        std::cout << "[INFO] " << message << std::endl;
        break;
    case Level::WARN:
        std::cout << "[WARN] " << message << std::endl;
        break;
    case Level::ERROR:
        std::cout << "[ERROR] " << message << std::endl;
        break;
    case Level::DEBUG:
        std::cout << "[DEBUG] " << message << std::endl;
        break;
    default:
        break;
    }
    mutex_.unlock();
}