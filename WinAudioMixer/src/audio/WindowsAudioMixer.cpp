#ifdef _WIN32

#include "WindowsAudioMixer.h"

#include <psapi.h>

#include <algorithm>

#pragma comment(lib, "psapi.lib")

namespace winaudiomixer
{

    WindowsAudioMixer::WindowsAudioMixer()
    {
        initialize();
    }

    WindowsAudioMixer::~WindowsAudioMixer()
    {
        shutdown();
    }

    bool WindowsAudioMixer::initialize()
    {
        HRESULT hr;

        // COM initialisieren
        hr = CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            return false;

        // Device Enumerator
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&m_deviceEnumerator));

        if (FAILED(hr))
            return false;

        // Standard-Ausgabegerät
        hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
            eRender,
            eConsole,
            &m_device);

        if (FAILED(hr))
            return false;

        // Audio Session Manager
        hr = m_device->Activate(
            __uuidof(IAudioSessionManager2),
            CLSCTX_ALL,
            nullptr,
            reinterpret_cast<void **>(&m_sessionManager));

        if (FAILED(hr))
            return false;

        return true;
    }

    void WindowsAudioMixer::shutdown()
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

        CoUninitialize();
    }

    bool WindowsAudioMixer::setApplicationVolume(
        const std::string &application,
        float volume)
    {
        if (!m_sessionManager)
            return false;

        volume = std::clamp(volume, 0.0f, 1.0f);

        const std::wstring targetApplication =
            stringToWString(application);

        IAudioSessionEnumerator *enumerator = nullptr;

        HRESULT hr = m_sessionManager->GetSessionEnumerator(
            &enumerator);

        if (FAILED(hr) || !enumerator)
            return false;

        int count = 0;

        hr = enumerator->GetCount(&count);

        if (FAILED(hr))
        {
            enumerator->Release();
            return false;
        }

        bool changed = false;

        for (int i = 0; i < count; ++i)
        {
            IAudioSessionControl *control = nullptr;

            hr = enumerator->GetSession(
                i,
                &control);

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

            const std::string processName =
                getProcessName(processId);

            const std::wstring processNameW =
                stringToWString(processName);

            if (processNameW == targetApplication)
            {
                ISimpleAudioVolume *simpleVolume = nullptr;

                hr = control->QueryInterface(
                    IID_PPV_ARGS(&simpleVolume));

                if (SUCCEEDED(hr) && simpleVolume)
                {
                    hr = simpleVolume->SetMasterVolume(
                        volume,
                        nullptr);

                    if (SUCCEEDED(hr))
                        changed = true;

                    simpleVolume->Release();
                }
            }

            control2->Release();
            control->Release();
        }

        enumerator->Release();

        return changed;
    }

    std::vector<std::string>
    WindowsAudioMixer::getAvailableApplications() const
    {
        std::vector<std::string> result;

        if (!m_sessionManager)
            return result;

        IAudioSessionEnumerator *enumerator = nullptr;

        HRESULT hr = m_sessionManager->GetSessionEnumerator(
            &enumerator);

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

            hr = enumerator->GetSession(
                i,
                &control);

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

            if (SUCCEEDED(hr) && processId != 0)
            {
                std::string processName =
                    getProcessName(processId);

                if (!processName.empty())
                {
                    if (std::find(
                            result.begin(),
                            result.end(),
                            processName) == result.end())
                    {
                        result.push_back(processName);
                    }
                }
            }

            control2->Release();
            control->Release();
        }

        enumerator->Release();

        return result;
    }

    std::string WindowsAudioMixer::getProcessName(
        DWORD processId) const
    {
        if (processId == 0)
            return {};

        HANDLE process = OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            processId);

        if (!process)
            return {};

        wchar_t buffer[MAX_PATH];
        DWORD size = MAX_PATH;

        std::string result;

        if (QueryFullProcessImageNameW(
                process,
                0,
                buffer,
                &size))
        {
            std::wstring path(
                buffer,
                size);

            const size_t position =
                path.find_last_of(L"\\/");

            if (position != std::wstring::npos)
                path = path.substr(position + 1);

            result = wStringToString(path);
        }

        CloseHandle(process);

        return result;
    }

    std::wstring WindowsAudioMixer::stringToWString(
        const std::string &value)
    {
        if (value.empty())
            return {};

        int size = MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0);

        if (size <= 0)
            return {};

        std::wstring result(size, L'\0');

        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            size);

        return result;
    }

    std::string WindowsAudioMixer::wStringToString(
        const std::wstring &value)
    {
        if (value.empty())
            return {};

        int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (size <= 0)
            return {};

        std::string result(size, '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            size,
            nullptr,
            nullptr);

        return result;
    }

} // namespace winaudiomixer

#endif