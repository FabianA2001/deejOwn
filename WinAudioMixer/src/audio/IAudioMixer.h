#pragma once

#include <functional>
#include <string>
#include <vector>

namespace winaudiomixer
{

    class IAudioMixer
    {
    public:
        // Wird mit dem Prozessnamen (z. B. "Spotify.exe") aufgerufen, sobald eine neue
        // Audioquelle auftaucht. ACHTUNG: kann auf einem anderen Thread laufen als der
        // Hauptthread. Im Callback also nur kurz arbeiten (z. B. in eine Queue schreiben).
        using NewApplicationCallback = std::function<void(const std::string &)>;

        virtual ~IAudioMixer() = default;

        virtual bool setApplicationVolume(const std::string &application, float volume) = 0;
        virtual std::vector<std::string> getAvailableApplications() const = 0;
        virtual void setNewApplicationCallback(NewApplicationCallback callback) = 0;
    };

} // namespace winaudiomixer
