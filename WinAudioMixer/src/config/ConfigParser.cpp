#include "config/ConfigParser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "util/Logger.h"

namespace winaudiomixer {
    namespace {

        std::string makeLower(const std::string& value)
        {
            std::string copy = value;
            std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
                });
            return copy;
        }

        bool startsWith(const std::string& text, const std::string& prefix)
        {
            return text.rfind(prefix, 0) == 0;
        }

        int parseIntOrThrow(const std::string& value)
        {
            std::size_t pos = 0;
            const int parsed = std::stoi(value, &pos);
            if (pos != value.size()) {
                throw std::runtime_error("Invalid numeric value: " + value);
            }
            return parsed;
        }

    } // namespace

    std::string ConfigParser::trim(const std::string& value)
    {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            return "";
        }

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1);
    }

    std::string ConfigParser::toLower(const std::string& value)
    {
        return makeLower(value);
    }

    std::vector<std::string> ConfigParser::splitCsv(const std::string& value)
    {
        std::vector<std::string> items;
        std::stringstream stream(value);
        std::string item;
        while (std::getline(stream, item, ',')) {
            const std::string trimmed = trim(item);
            if (!trimmed.empty()) {
                items.push_back(trimmed);
            }
        }
        return items;
    }

    Configuration ConfigParser::parseFile(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            throw std::runtime_error("Configuration file not found: " + path.u8string());
        }

        std::stringstream stream;
        stream << input.rdbuf();
        std::string content = stream.str();

        // UTF-8-BOM entfernen (Windows-Editoren schreiben ihn manchmal)
        if (content.size() >= 3 &&
            static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            content.erase(0, 3);
        }

        return parseString(content);
    }

    Configuration ConfigParser::parseString(const std::string& content)
    {
        Configuration config;
        std::string currentSection;

        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
                continue;
            }

            if (trimmed.front() == '[' && trimmed.back() == ']') {
                currentSection = toLower(trimmed.substr(1, trimmed.size() - 2));
                continue;
            }

            const auto equalsPos = trimmed.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }

            const std::string key = toLower(trim(trimmed.substr(0, equalsPos)));
            const std::string value = trim(trimmed.substr(equalsPos + 1));

            if (currentSection == "serial") {
                if (key == "port") {
                    config.serialPort = value;
                }
                else if (key == "baudrate") {
                    config.baudRate = parseIntOrThrow(value);
                }
                else if (key == "slider_count") {
                    config.sliderCount = static_cast<std::size_t>(parseIntOrThrow(value));
                }
            }
            else if (currentSection == "filter") {
                if (key == "window_size") {
                    config.filterWindowSize = parseIntOrThrow(value);
                }
            }
            else if (currentSection == "deadzone") {
                if (key == "value") {
                    config.deadZone = parseIntOrThrow(value);
                }
            }
            else if (currentSection == "mapping") {
                if (!startsWith(key, "slider_")) {
                    continue;
                }

                const std::string sliderKey = key.substr(7);
                const int sliderIndex = parseIntOrThrow(sliderKey);
                const std::vector<std::string> items = splitCsv(value);
                config.sliderAssignments[sliderIndex] = items;
            }
        }

        if (config.sliderCount == 0) {
            throw std::runtime_error("Invalid slider count in configuration.");
        }

        if (config.filterWindowSize <= 0) {
            throw std::runtime_error("Filter window size must be greater than zero.");
        }

        if (config.deadZone < 0) {
            throw std::runtime_error("Dead zone must not be negative.");
        }

        return config;
    }

} // namespace winaudiomixer