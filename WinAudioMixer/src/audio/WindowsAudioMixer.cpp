#include "audio/WindowsAudioMixer.h"

#include <algorithm>
#include <map>

namespace winaudiomixer {
namespace {

std::map<std::string, float>& volumeStorage()
{
    static std::map<std::string, float> storage;
    return storage;
}

} // namespace

bool WindowsAudioMixer::setApplicationVolume(const std::string& application, float volume)
{
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    volumeStorage()[application] = clamped;
    return true;
}

bool WindowsAudioMixer::getApplicationVolume(const std::string& application, float& outVolume) const
{
    const auto it = volumeStorage().find(application);
    if (it == volumeStorage().end()) {
        return false;
    }

    outVolume = it->second;
    return true;
}

std::vector<std::string> WindowsAudioMixer::getAvailableApplications() const
{
    std::vector<std::string> applications;
    const auto& storage = volumeStorage();
    applications.reserve(storage.size());
    for (const auto& [application, volume] : storage) {
        (void)volume;
        applications.push_back(application);
    }

    return applications;
}

} // namespace winaudiomixer
