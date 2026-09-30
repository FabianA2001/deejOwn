#include "AudioManager.h"

#include <windows.h>
#include <psapi.h>

#include <algorithm>

#pragma comment(lib, "psapi.lib")

AudioManager::AudioManager()
{
}

AudioManager::~AudioManager()
{
    Shutdown();
}

bool AudioManager::Initialize()
{
    HRESULT hr;

    // Default Audio Endpoint
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&m_deviceEnumerator));

    if (FAILED(hr))
        return false;

    // Standard-Ausgabegerät holen
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &m_device);

    if (FAILED(hr))
        return false;

    // Audio Session Manager holen
    hr = m_device->Activate(
        __uuidof(IAudioSessionManager2),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void **>(&m_sessionManager));

    if (FAILED(hr))
        return false;

    return true;
}

void AudioManager::Shutdown()
{
    if (m_sessionManager)
    {
        m_sessionManager->Release();
        m_sessionManager = nullptr;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }

    if (m_deviceEnumerator)
    {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }
}

std::wstring AudioManager::GetProcessName(DWORD processId)
{
    if (processId == 0)
        return L"System Sounds";

    HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        processId);

    if (!process)
        return L"Unknown";

    wchar_t buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    std::wstring result = L"Unknown";

    if (QueryFullProcessImageNameW(
            process,
            0,
            buffer,
            &size))
    {
        std::wstring path(buffer, size);

        size_t pos = path.find_last_of(L"\\/");

        if (pos != std::wstring::npos)
            result = path.substr(pos + 1);
        else
            result = path;
    }

    CloseHandle(process);

    return result;
}

std::wstring AudioManager::GetDisplayName(
    IAudioSessionControl *control)
{
    LPWSTR name = nullptr;

    HRESULT hr = control->GetDisplayName(&name);

    if (FAILED(hr) || !name)
        return L"";

    std::wstring result(name);

    CoTaskMemFree(name);

    return result;
}

std::vector<AudioSession> AudioManager::GetSessions()
{
    std::vector<AudioSession> result;

    if (!m_sessionManager)
        return result;

    IAudioSessionEnumerator *enumerator = nullptr;

    HRESULT hr = m_sessionManager->GetSessionEnumerator(&enumerator);

    if (FAILED(hr) || !enumerator)
        return result;

    int count = 0;

    hr = enumerator->GetCount(&count);

    if (FAILED(hr))
    {
        enumerator->Release();
        return result;
    }

    for (int i = 0; i < count; ++i)
    {
        IAudioSessionControl *control = nullptr;

        hr = enumerator->GetSession(i, &control);

        if (FAILED(hr) || !control)
            continue;

        IAudioSessionControl2 *control2 = nullptr;

        hr = control->QueryInterface(
            IID_PPV_ARGS(&control2));

        if (FAILED(hr) || !control2)
        {
            control->Release();
            continue;
        }

        DWORD processId = 0;

        hr = control2->GetProcessId(&processId);

        if (FAILED(hr))
        {
            control2->Release();
            control->Release();
            continue;
        }

        // Lautstärke-Schnittstelle holen
        ISimpleAudioVolume *volume = nullptr;

        hr = control->QueryInterface(
            IID_PPV_ARGS(&volume));

        if (FAILED(hr) || !volume)
        {
            control2->Release();
            control->Release();
            continue;
        }

        AudioSession session;

        session.processId = processId;
        session.processName = GetProcessName(processId);
        session.displayName = GetDisplayName(control);
        session.volume = volume;

        // Falls kein Prozessname vorhanden ist
        // (z.B. bestimmte System-Audio-Sessions)
        if (session.processName.empty())
        {
            session.processName = L"System Audio";
        }

        // Falls kein Anzeigename vorhanden ist
        if (session.displayName.empty())
        {
            session.displayName = session.processName;
        }

        result.push_back(session);

        control2->Release();
        control->Release();
    }

    enumerator->Release();

    return result;
}

bool AudioManager::SetVolume(
    ISimpleAudioVolume *volume,
    float value)
{
    if (!volume)
        return false;

    value = std::clamp(value, 0.0f, 1.0f);

    HRESULT hr = volume->SetMasterVolume(
        value,
        nullptr);

    return SUCCEEDED(hr);
}

bool AudioManager::GetVolume(
    ISimpleAudioVolume *volume,
    float &value)
{
    if (!volume)
        return false;

    HRESULT hr = volume->GetMasterVolume(&value);

    return SUCCEEDED(hr);
}

bool AudioManager::SetMute(
    ISimpleAudioVolume *volume,
    bool mute)
{
    if (!volume)
        return false;

    HRESULT hr = volume->SetMute(
        mute ? TRUE : FALSE,
        nullptr);

    return SUCCEEDED(hr);
}

bool AudioManager::GetMute(
    ISimpleAudioVolume *volume,
    bool &mute)
{
    if (!volume)
        return false;

    BOOL value = FALSE;

    HRESULT hr = volume->GetMute(&value);

    if (FAILED(hr))
        return false;

    mute = value != FALSE;

    return true;
}

void AudioManager::ReleaseSession(AudioSession &session)
{
    if (session.volume)
    {
        session.volume->Release();
        session.volume = nullptr;
    }
}