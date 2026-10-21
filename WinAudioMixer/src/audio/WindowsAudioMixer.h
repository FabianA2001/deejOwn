#pragma once

#ifdef _WIN32

#include "IAudioMixer.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace winaudiomixer
{

    class WindowsAudioMixer;

    // Meldet Aenderungen des Standard-Ausgabegeraets (Callback laeuft auf fremdem Thread,
    // deshalb wird nur ein atomares Flag gesetzt).
    class DeviceNotificationClient : public IMMNotificationClient
    {
    public:
        explicit DeviceNotificationClient(std::atomic<bool> &flag) : m_flag(flag) {}

        // IUnknown
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
        ULONG STDMETHODCALLTYPE Release() override
        {
            ULONG r = InterlockedDecrement(&m_ref);
            if (r == 0)
                delete this;
            return r;
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
        {
            if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient))
            {
                *ppv = static_cast<IMMNotificationClient *>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        // IMMNotificationClient
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
        {
            if (flow == eRender && role == eConsole)
                m_flag = true;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
        {
            m_flag = true;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

    private:
        LONG m_ref = 1;
        std::atomic<bool> &m_flag;
    };

    // Meldet neue Audio-Sessions (z. B. wenn Spotify anfaengt, Ton auszugeben).
    class SessionNotificationClient : public IAudioSessionNotification
    {
    public:
        explicit SessionNotificationClient(WindowsAudioMixer *owner) : m_owner(owner) {}

        // IUnknown
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
        ULONG STDMETHODCALLTYPE Release() override
        {
            ULONG r = InterlockedDecrement(&m_ref);
            if (r == 0)
                delete this;
            return r;
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
        {
            if (riid == __uuidof(IUnknown) || riid == __uuidof(IAudioSessionNotification))
            {
                *ppv = static_cast<IAudioSessionNotification *>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        // In der .cpp definiert (braucht die volle WindowsAudioMixer-Klasse)
        HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *newSession) override;

    private:
        LONG m_ref = 1;
        WindowsAudioMixer *m_owner;
    };

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

        void setNewApplicationCallback(NewApplicationCallback callback) override;

        // Wird vom SessionNotificationClient aufgerufen (fremder Thread)
        void onSessionCreated(IAudioSessionControl *control) const;

    private:
        // COM-Objekte (mutable, damit auch const-Methoden das Geraet neu holen koennen)
        IMMDeviceEnumerator *m_deviceEnumerator = nullptr;
        mutable IMMDevice *m_device = nullptr;
        mutable IAudioSessionManager2 *m_sessionManager = nullptr;

        bool m_comInitialized = false;

        // Geraetewechsel
        mutable std::atomic<bool> m_deviceChanged{false};
        DeviceNotificationClient *m_notificationClient = nullptr;

        // Neue Sessions
        SessionNotificationClient *m_sessionNotification = nullptr;

        mutable std::mutex m_callbackMutex;
        NewApplicationCallback m_newApplicationCallback;

        // Schuetzt Geraet + SessionManager
        mutable std::mutex m_deviceMutex;

        bool initialize();
        void shutdown();

        bool acquireDevice() const;  // Geraet + SessionManager holen, Registrierung
        void releaseDevice() const;  // Abmelden + beides freigeben
        bool ensureDevice() const;   // bei Bedarf neu holen (m_deviceMutex muss gehalten werden)

        std::string getProcessName(DWORD processId) const;

        static std::string normalizeName(const std::string &value);

        static std::wstring stringToWString(
            const std::string &value);

        static std::string wStringToString(
            const std::wstring &value);
    };

} // namespace winaudiomixer

#endif // _WIN32
