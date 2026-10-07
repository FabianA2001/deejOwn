#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace winaudiomixer {

struct Configuration {
    std::string serialPort;
    int baudRate;
    std::size_t sliderCount;
    int filterWindowSize;
    int deadZone;
    std::map<int, std::vector<std::string>> sliderAssignments;
    std::vector<std::string> allApplications;
};

} // namespace winaudiomixer
