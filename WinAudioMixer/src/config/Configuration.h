#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace winaudiomixer {

struct Configuration {
    std::string serialPort = "COM5";
    int baudRate = 9600;
    std::size_t sliderCount = 5;
    int filterWindowSize = 5;
    int deadZone = 5;
    std::map<int, std::vector<std::string>> sliderAssignments;
    std::vector<std::string> allApplications;
};

} // namespace winaudiomixer
