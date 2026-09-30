#ifdef _WIN32

#pragma once

#include "IAudioMixer.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include <string>
#include <vector>

namespace winaudiomixer
{

    class WindowsAudioMixer : public IAudioMixer
    {
    public:
        WindowsAudioMixer();
        ~WindowsAudioMixer() override;

        bool setApplicationVolume(
            const std::string &application,
            float volume) override;

        std::vector<std::string>
        getAvailableApplications() const override;

    private:
        IMMDeviceEnumerator *m_deviceEnumerator = nullptr;
        IMMDevice *m_device = nullptr;
        IAudioSessionManager2 *m_sessionManager = nullptr;

        bool initialize();
        void shutdown();

        std::string getProcessName(DWORD processId) const;

        static std::wstring stringToWString(
            const std::string &value);

        static std::string wStringToString(
            const std::wstring &value);
    };

} // namespace winaudiomixer
#endif
