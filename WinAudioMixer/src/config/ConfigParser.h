#pragma once

#include "config/Configuration.h"

#include <string>

namespace winaudiomixer {

class ConfigParser {
public:
    static Configuration parseFile(const std::string& path);
    static Configuration parseString(const std::string& content);

private:
    static std::string trim(const std::string& value);
    static std::string toLower(const std::string& value);
    static std::vector<std::string> splitCsv(const std::string& value);
};

} // namespace winaudiomixer
