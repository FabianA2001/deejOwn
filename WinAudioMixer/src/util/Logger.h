#pragma once

#include <string>

namespace winaudiomixer
{

    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    };

    const LogLevel LogStatus = LogLevel::Debug;

    class Logger
    {
    public:
        static void log(LogLevel level, const std::string &message);
        static void info(const std::string &message);
        static void warning(const std::string &message);
        static void error(const std::string &message);
        static void debug(const std::string &message);
    };

} // namespace winaudiomixer
