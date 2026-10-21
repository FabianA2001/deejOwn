#pragma once

#include "audio/IAudioMixer.h"

#include <map>
#include <string>
#include <vector>

namespace winaudiomixer
{

    class NullAudioMixer : public IAudioMixer
    {
    public:
        bool setApplicationVolume(const std::string &application, float volume) override;
        std::vector<std::string> getAvailableApplications() const override;
        void setNewApplicationCallback(NewApplicationCallback callback) override;
        void printVolumes() const;

        const std::map<std::string, float> &volumes() const noexcept;

    private:
        std::map<std::string, float> volumes_;
    };

} // namespace winaudiomixer
