#pragma once

#include "audio/IAudioMixer.h"

#include <cstddef>
#include <string>
#include <vector>

namespace winaudiomixer {

struct ApplicationVolumeChange {
    std::string application;
    float volume = 0.0f;
};

class SliderController {
public:
    explicit SliderController(std::size_t sliderCount);

    void setDeadZone(int deadZone);
    void setMapping(int sliderIndex, const std::vector<std::string>& programs);
    void setLastAppliedValue(int sliderIndex, int value);
    bool shouldApplyChange(int sliderIndex, int newValue) const;

    std::vector<ApplicationVolumeChange> process(const std::vector<int>& filteredValues, const IAudioMixer& audioMixer);

    static float sliderValueToVolume(int sliderValue);

private:
    std::size_t sliderCount_;
    int deadZone_;
    std::vector<int> lastAppliedValues_;
    std::vector<std::vector<std::string>> sliderAssignments_;
};

} // namespace winaudiomixer
