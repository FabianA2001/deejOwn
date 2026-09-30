#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <vector>
#include <string>

struct AudioSession
{
    DWORD processId = 0;

    std::wstring processName;
    std::wstring displayName;

    ISimpleAudioVolume *volume = nullptr;

    HWND label = nullptr;
    HWND slider = nullptr;
    HWND muteButton = nullptr;
};

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    bool Initialize();
    void Shutdown();

    std::vector<AudioSession> GetSessions();

    bool SetVolume(ISimpleAudioVolume *volume, float value);
    bool GetVolume(ISimpleAudioVolume *volume, float &value);

    bool SetMute(ISimpleAudioVolume *volume, bool mute);
    bool GetMute(ISimpleAudioVolume *volume, bool &mute);

private:
    IMMDeviceEnumerator *m_deviceEnumerator = nullptr;
    IMMDevice *m_device = nullptr;
    IAudioSessionManager2 *m_sessionManager = nullptr;

    std::wstring GetProcessName(DWORD processId);
    std::wstring GetDisplayName(IAudioSessionControl *control);

    void ReleaseSession(AudioSession &session);
};