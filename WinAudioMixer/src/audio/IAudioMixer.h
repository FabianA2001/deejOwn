#pragma once

#include <string>
#include <vector>

namespace winaudiomixer {

class IAudioMixer {
public:
    virtual ~IAudioMixer() = default;

    virtual bool setApplicationVolume(const std::string& application, float volume) = 0;
    virtual bool getApplicationVolume(const std::string& application, float& outVolume) const = 0;
    virtual std::vector<std::string> getAvailableApplications() const = 0;
};

} // namespace winaudiomixer
