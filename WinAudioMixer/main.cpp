#include <windows.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <memory>

#include "AudioManager.h"

#pragma comment(lib, "comctl32.lib")

// ------------------------------------------------------------
// IDs
// ------------------------------------------------------------

constexpr int IDC_REFRESH = 1001;
constexpr int IDC_SCROLL = 1002;

constexpr int IDC_SLIDER_BASE = 2000;
constexpr int IDC_MUTE_BASE = 3000;

constexpr UINT TIMER_REFRESH = 1;

// ------------------------------------------------------------
// Globale Variablen
// ------------------------------------------------------------

AudioManager g_audioManager;

std::vector<AudioSession> g_sessions;

HWND g_mainWindow = nullptr;
HWND g_refreshButton = nullptr;

bool g_updatingUI = false;

// ------------------------------------------------------------
// Hilfsfunktionen
// ------------------------------------------------------------

std::wstring BuildSessionName(
    const AudioSession &session)
{
    if (!session.displayName.empty())
    {
        if (session.displayName != session.processName)
        {
            return session.displayName +
                   L" (" +
                   session.processName +
                   L")";
        }

        return session.displayName;
    }

    return session.processName;
}

void DestroySessionControls()
{
    for (auto &session : g_sessions)
    {
        if (session.label)
            DestroyWindow(session.label);

        if (session.slider)
            DestroyWindow(session.slider);

        if (session.muteButton)
            DestroyWindow(session.muteButton);

        if (session.volume)
        {
            session.volume->Release();
            session.volume = nullptr;
        }
    }

    g_sessions.clear();
}

// ------------------------------------------------------------
// UI aktualisieren
// ------------------------------------------------------------

void RefreshSessions()
{
    if (!g_mainWindow)
        return;

    g_updatingUI = true;

    DestroySessionControls();

    g_sessions = g_audioManager.GetSessions();

    int y = 70;

    const int labelWidth = 240;
    const int sliderWidth = 280;
    const int buttonWidth = 90;

    for (size_t i = 0; i < g_sessions.size(); ++i)
    {
        auto &session = g_sessions[i];

        int sliderId =
            IDC_SLIDER_BASE + static_cast<int>(i);

        int muteId =
            IDC_MUTE_BASE + static_cast<int>(i);

        // ----------------------------------------------------
        // Name
        // ----------------------------------------------------

        std::wstring name = BuildSessionName(session);

        session.label = CreateWindowExW(
            0,
            L"STATIC",
            name.c_str(),
            WS_CHILD | WS_VISIBLE,
            20,
            y,
            labelWidth,
            30,
            g_mainWindow,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);

        // ----------------------------------------------------
        // Slider
        // ----------------------------------------------------

        session.slider = CreateWindowExW(
            0,
            TRACKBAR_CLASSW,
            L"",
            WS_CHILD |
                WS_VISIBLE |
                TBS_AUTOTICKS |
                TBS_HORZ,
            260,
            y - 5,
            sliderWidth,
            40,
            g_mainWindow,
            reinterpret_cast<HMENU>(
                static_cast<INT_PTR>(sliderId)),
            GetModuleHandleW(nullptr),
            nullptr);

        SendMessageW(
            session.slider,
            TBM_SETRANGE,
            TRUE,
            MAKELONG(0, 100));

        SendMessageW(
            session.slider,
            TBM_SETTICFREQ,
            10,
            0);

        float volume = 0.0f;

        if (g_audioManager.GetVolume(
                session.volume,
                volume))
        {
            int percent =
                static_cast<int>(volume * 100.0f + 0.5f);

            SendMessageW(
                session.slider,
                TBM_SETPOS,
                TRUE,
                percent);
        }

        // ----------------------------------------------------
        // Mute Button
        // ----------------------------------------------------

        session.muteButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"Mute",
            WS_CHILD |
                WS_VISIBLE |
                BS_PUSHBUTTON,
            560,
            y,
            buttonWidth,
            28,
            g_mainWindow,
            reinterpret_cast<HMENU>(
                static_cast<INT_PTR>(muteId)),
            GetModuleHandleW(nullptr),
            nullptr);

        bool muted = false;

        if (g_audioManager.GetMute(
                session.volume,
                muted))
        {
            SetWindowTextW(
                session.muteButton,
                muted ? L"Unmute" : L"Mute");
        }

        y += 55;
    }

    g_updatingUI = false;

    InvalidateRect(
        g_mainWindow,
        nullptr,
        TRUE);
}

// ------------------------------------------------------------
// Fenstergröße
// ------------------------------------------------------------

void UpdateWindowSize()
{
    RECT rect;

    GetClientRect(
        g_mainWindow,
        &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (height < 100)
        height = 100;

    // Keine spezielle Größenberechnung notwendig.
    // Die Controls werden bei Refresh neu positioniert.

    (void)width;
    (void)height;
}

// ------------------------------------------------------------
// Window Procedure
// ------------------------------------------------------------

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_mainWindow = hwnd;

        // ----------------------------------------------------
        // Refresh Button
        // ----------------------------------------------------

        g_refreshButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"Refresh",
            WS_CHILD |
                WS_VISIBLE |
                BS_PUSHBUTTON,
            20,
            20,
            100,
            30,
            hwnd,
            reinterpret_cast<HMENU>(IDC_REFRESH),
            GetModuleHandleW(nullptr),
            nullptr);

        // ----------------------------------------------------
        // Timer
        // ----------------------------------------------------

        SetTimer(
            hwnd,
            TIMER_REFRESH,
            1500,
            nullptr);

        RefreshSessions();

        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        // Refresh
        if (id == IDC_REFRESH)
        {
            RefreshSessions();
            return 0;
        }

        // Mute Buttons
        if (id >= IDC_MUTE_BASE &&
            id < IDC_MUTE_BASE + 1000)
        {
            int index =
                id - IDC_MUTE_BASE;

            if (index >= 0 &&
                index < static_cast<int>(g_sessions.size()))
            {
                auto &session =
                    g_sessions[index];

                bool muted = false;

                if (g_audioManager.GetMute(
                        session.volume,
                        muted))
                {
                    g_audioManager.SetMute(
                        session.volume,
                        !muted);

                    SetWindowTextW(
                        session.muteButton,
                        !muted
                            ? L"Unmute"
                            : L"Mute");
                }
            }

            return 0;
        }

        break;
    }

    case WM_HSCROLL:
    {
        HWND slider =
            reinterpret_cast<HWND>(lParam);

        if (!slider || g_updatingUI)
            break;

        // Herausfinden, welcher Slider verändert wurde
        for (size_t i = 0;
             i < g_sessions.size();
             ++i)
        {
            if (g_sessions[i].slider == slider)
            {
                int position =
                    static_cast<int>(
                        SendMessageW(
                            slider,
                            TBM_GETPOS,
                            0,
                            0));

                float volume =
                    static_cast<float>(position) / 100.0f;

                g_audioManager.SetVolume(
                    g_sessions[i].volume,
                    volume);

                break;
            }
        }

        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == TIMER_REFRESH)
        {
            // Neue Programme können Audio-Sessions
            // geöffnet haben.
            //
            // Für die erste Version aktualisieren
            // wir einfach regelmäßig die Liste.

            RefreshSessions();
        }

        return 0;
    }

    case WM_SIZE:
    {
        UpdateWindowSize();
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(
            hwnd,
            TIMER_REFRESH);

        DestroySessionControls();

        PostQuitMessage(0);

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam);
}

// ------------------------------------------------------------
// WinMain
// ------------------------------------------------------------

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    // --------------------------------------------------------
    // COM initialisieren
    // --------------------------------------------------------

    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        MessageBoxW(
            nullptr,
            L"COM konnte nicht initialisiert werden.",
            L"Fehler",
            MB_ICONERROR);

        return 1;
    }

    // --------------------------------------------------------
    // Common Controls
    // --------------------------------------------------------

    INITCOMMONCONTROLSEX icc{};

    icc.dwSize =
        sizeof(INITCOMMONCONTROLSEX);

    icc.dwICC =
        ICC_BAR_CLASSES;

    InitCommonControlsEx(&icc);

    // --------------------------------------------------------
    // Audio Manager
    // --------------------------------------------------------

    if (!g_audioManager.Initialize())
    {
        MessageBoxW(
            nullptr,
            L"Die Windows Audio API konnte nicht initialisiert werden.",
            L"Fehler",
            MB_ICONERROR);

        CoUninitialize();

        return 1;
    }

    // --------------------------------------------------------
    // Window Class
    // --------------------------------------------------------

    const wchar_t CLASS_NAME[] =
        L"WinAudioMixerWindow";

    WNDCLASSW wc{};

    wc.lpfnWndProc =
        WindowProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        CLASS_NAME;

    wc.hCursor =
        LoadCursorW(
            nullptr,
            IDC_ARROW);

    wc.hbrBackground =
        reinterpret_cast<HBRUSH>(
            COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(
            nullptr,
            L"Fensterklasse konnte nicht registriert werden.",
            L"Fehler",
            MB_ICONERROR);

        g_audioManager.Shutdown();
        CoUninitialize();

        return 1;
    }

    // --------------------------------------------------------
    // Fenster
    // --------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Windows Audio Mixer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        720,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd)
    {
        MessageBoxW(
            nullptr,
            L"Fenster konnte nicht erstellt werden.",
            L"Fehler",
            MB_ICONERROR);

        g_audioManager.Shutdown();
        CoUninitialize();

        return 1;
    }

    ShowWindow(
        hwnd,
        nCmdShow);

    UpdateWindow(hwnd);

    // --------------------------------------------------------
    // Message Loop
    // --------------------------------------------------------

    MSG msg{};

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    g_audioManager.Shutdown();

    CoUninitialize();

    return static_cast<int>(msg.wParam);
}