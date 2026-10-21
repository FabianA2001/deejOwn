#ifdef _WIN32

#include "WindowsAudioMixer.h"

#include <psapi.h>

#include <algorithm>
#include <cwctype>

#pragma comment(lib, "psapi.lib")

namespace winaudiomixer
{

    // ------------------------------------------------------------
    // SessionNotificationClient
    // ------------------------------------------------------------

    HRESULT SessionNotificationClient::OnSessionCreated(IAudioSessionControl *newSession)
    {
        if (newSession && m_owner)
            m_owner->onSessionCreated(newSession);
        return S_OK;
    }

    // ------------------------------------------------------------
    // WindowsAudioMixer
    // ------------------------------------------------------------

    WindowsAudioMixer::WindowsAudioMixer()
    {
        initialize();
    }

    WindowsAudioMixer::~WindowsAudioMixer()
    {
        shutdown();
    }

    void WindowsAudioMixer::setNewApplicationCallback(NewApplicationCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_newApplicationCallback = std::move(callback);
    }

    bool WindowsAudioMixer::initialize()
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);

        // COM initialisieren
        HRESULT hr = CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

        if (SUCCEEDED(hr))
            m_comInitialized = true;
        else if (hr != RPC_E_CHANGED_MODE)
            return false;

        // Device Enumerator
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&m_deviceEnumerator));

        if (FAILED(hr))
            return false;

        // Benachrichtigung bei Geraetewechsel
        m_notificationClient = new DeviceNotificationClient(m_deviceChanged);
        m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient);

        // Benachrichtigung bei neuen Sessions (einmal anlegen, wird pro Geraet registriert)
        m_sessionNotification = new SessionNotificationClient(this);

        return acquireDevice();
    }

    void WindowsAudioMixer::shutdown()
    {
        std::lock_guard<std::mutex> lock(m_deviceMutex);

        releaseDevice();

        if (m_deviceEnumerator)
        {
            if (m_notificationClient)
                m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_notificationClient);

            m_deviceEnumerator->Release();
            m_deviceEnumerator = nullptr;
        }

        if (m_notificationClient)
        {
            m_notificationClient->Release();
            m_notificationClient = nullptr;
        }

        if (m_sessionNotification)
        {
            m_sessionNotification->Release();
            m_sessionNotification = nullptr;
        }

        if (m_comInitialized)
        {
            CoUninitialize();
            m_comInitialized = false;
        }
    }

    bool WindowsAudioMixer::acquireDevice() const
    {
        if (!m_deviceEnumerator)
            return false;

        // Standard-Ausgabegeraet
        HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(
            eRender,
            eConsole,
            &m_device);

        if (FAILED(hr))
        {
            m_device = nullptr;
            return false;
        }

        // Audio Session Manager
        hr = m_device->Activate(
            __uuidof(IAudioSessionManager2),
            CLSCTX_ALL,
            nullptr,
            reinterpret_cast<void **>(&m_sessionManager));

        if (FAILED(hr))
        {
            m_sessionManager = nullptr;
            m_device->Release();
            m_device = nullptr;
            return false;
        }

        // Fuer neue Sessions registrieren
        if (m_sessionNotification)
            m_sessionManager->RegisterSessionNotification(m_sessionNotification);

        // Bereits laufende Sessions des (neuen) Geraets ebenfalls als "neue Anwendung" melden.
        // Der Aufruf von GetSessionEnumerator ist ausserdem noetig, damit die
        // Benachrichtigungen zuverlaessig ankommen.
        IAudioSessionEnumerator *enumerator = nullptr;

        if (SUCCEEDED(m_sessionManager->GetSessionEnumerator(&enumerator)) && enumerator)
        {
            int count = 0;

            if (SUCCEEDED(enumerator->GetCount(&count)))
            {
                for (int i = 0; i < count; ++i)
                {
                    IAudioSessionControl *control = nullptr;

                    if (SUCCEEDED(enumerator->GetSession(i, &control)) && control)
                    {
                        onSessionCreated(control);
                        control->Release();
                    }
                }
            }

            enumerator->Release();
        }

        return true;
    }

    void WindowsAudioMixer::releaseDevice() const
    {
        if (m_sessionManager)
        {
            if (m_sessionNotification)
                m_sessionManager->UnregisterSessionNotification(m_sessionNotification);

            m_sessionManager->Release();
            m_sessionManager = nullptr;
        }

        if (m_device)
        {
            m_device->Release();
            m_device = nullptr;
        }
    }

    // m_deviceMutex muss vom Aufrufer gehalten werden
    bool WindowsAudioMixer::ensureDevice() const
    {
        if (m_deviceChanged.exchange(false))
        {
            releaseDevice();
            return acquireDevice();
        }

        if (!m_sessionManager)
        {
            // Frueherer Versuch fehlgeschlagen (z. B. kein Geraet vorhanden): erneut versuchen
            releaseDevice();
            return acquireDevice();
        }

        return true;
    }

    void WindowsAudioMixer::onSessionCreated(IAudioSessionControl *control) const
    {
        if (!control)
            return;

        // Callback kopieren und ausserhalb des Locks aufrufen
        NewApplicationCallback callback;

        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_newApplicationCallback;
        }

        if (!callback)
            return;

        IAudioSessionControl2 *control2 = nullptr;

        if (FAILED(control->QueryInterface(IID_PPV_ARGS(&control2))) || !control2)
            return;

        DWORD processId = 0;

        if (SUCCEEDED(control2->GetProcessId(&processId)) && processId != 0)
        {
            const std::string name = getProcessName(processId);

            if (!name.empty())
                callback(name);
        }

        control2->Release();
    }

    bool WindowsAudioMixer::setApplicationVolume(
        const std::string &application,
        float volume)
    {
        volume = std::clamp(volume, 0.0f, 1.0f);

        const std::string target = normalizeName(application);

        std::lock_guard<std::mutex> deviceLock(m_deviceMutex);

        if (!ensureDevice())
            return false;

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

            if (normalizeName(getProcessName(processId)) == target)
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

        std::lock_guard<std::mutex> deviceLock(m_deviceMutex);

        if (!ensureDevice())
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

    // Prozessnamen vergleichbar machen (Gross-/Kleinschreibung egal)
    std::string WindowsAudioMixer::normalizeName(const std::string &value)
    {
        std::wstring wide = stringToWString(value);

        std::transform(
            wide.begin(),
            wide.end(),
            wide.begin(),
            [](wchar_t c)
            { return static_cast<wchar_t>(std::towlower(c)); });

        return wStringToString(wide);
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

#endif // _WIN32
