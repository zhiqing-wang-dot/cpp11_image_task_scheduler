#pragma once

#include <mutex>
#include <string>

class Logger 
{
public:
    enum class Level
    {
        INFO,
        WARN,
        ERROR,
        DEBUG
    };

    static Logger& instance();
    void log(Level level, const std::string& message);

private:
    Logger() {};
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&);

    std::mutex mutex_;
};

#define LOG_INFO(message) \
    Logger::instance().log(Logger::Level::INFO, message)

#define LOG_WARN(message) \
    Logger::instance().log(Logger::Level::WARN, message)

#define LOG_ERROR(message) \
    Logger::instance().log(Logger::Level::ERROR, message)

#define LOG_DEBUG(message) \
    Logger::instance().log(Logger::Level::DEBUG, message)
    