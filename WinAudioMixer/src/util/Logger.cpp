#include "util/Logger.h"

#include <iostream>

namespace winaudiomixer
{

    void Logger::log(LogLevel level, const std::string &message)
    {
        if (level < LogStatus)
            return;
        const char *prefix = "[INFO]";
        switch (level)
        {
        case LogLevel::Info:
            prefix = "[INFO]";
            break;
        case LogLevel::Warning:
            prefix = "[WARNING]";
            break;
        case LogLevel::Error:
            prefix = "[ERROR]";
            break;
        case LogLevel::Debug:
            prefix = "[DEBUG]";
            break;
        }

        std::cout << prefix << " " << message << std::endl;
    }

    void Logger::info(const std::string &message)
    {
        log(LogLevel::Info, message);
    }

    void Logger::warning(const std::string &message)
    {
        log(LogLevel::Warning, message);
    }

    void Logger::error(const std::string &message)
    {
        log(LogLevel::Error, message);
    }
    void Logger::debug(const std::string &message)
    {
        log(LogLevel::Debug, message);
    }

} // namespace winaudiomixer
