#include "audio/NullAudioMixer.h"
#include "util/Logger.h"
#include <algorithm>

namespace winaudiomixer
{

    bool NullAudioMixer::setApplicationVolume(const std::string &application, float volume)
    {
        const float clamped = std::clamp(volume, 0.0f, 1.0f);
        volumes_[application] = clamped;
        this->printVolumes();
        return true;
    }

    std::vector<std::string> NullAudioMixer::getAvailableApplications() const
    {
        return {
            "Spotify",
            "Chrome",
            "Discord",
            "Teams"};
    }

    void NullAudioMixer::setNewApplicationCallback(IAudioMixer::NewApplicationCallback)
    {
        // Dummy: Der NullAudioMixer hat keine echten Audioquellen, die neu auftauchen koennten.
    }

    const std::map<std::string, float> &NullAudioMixer::volumes() const noexcept
    {
        return volumes_;
    }

    void NullAudioMixer::printVolumes() const
    {
        for (const auto &[application, volume] : volumes_)
        {
            winaudiomixer::Logger::debug("Application: " + application + ", Volume: " + std::to_string(volume));
        }
    }
} // namespace winaudiomixer
