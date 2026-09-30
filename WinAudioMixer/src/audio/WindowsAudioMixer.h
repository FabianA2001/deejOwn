#pragma once

#include "audio/IAudioMixer.h"

namespace winaudiomixer {

class WindowsAudioMixer : public IAudioMixer {
public:
    bool setApplicationVolume(const std::string& application, float volume) override;
    bool getApplicationVolume(const std::string& application, float& outVolume) const override;
    std::vector<std::string> getAvailableApplications() const override;
};

} // namespace winaudiomixer
