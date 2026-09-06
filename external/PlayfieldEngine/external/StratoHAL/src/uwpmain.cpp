// uwpmain.cpp
//
// Minimal UWP skeleton for Playfield Engine style HAL runtime.
// - Creates a CoreWindow via IFrameworkView
// - Initializes D3D12 by D3D12InitializeForUWP()
// - Dispatches input events to hal_callback_on_event_*()
// - Runs the frame loop and calls D3D12StartFrame()/D3D12EndFrame()
//
// NOTE:
//   This is only a skeleton. The following are intentionally left as TODO:
//   - exact swap chain binding details inside D3D12InitializeForUWP()
//   - viewport / mouse scale calculation
//   - gamepad / video / suspend-resume details
//   - full keycode mapping
//
// Assumptions:
//   - The HAL callback symbols are available from C code.
//   - D3D12InitializeForUWP(IUnknown *nativeWindow, int width, int height)
//     is provided by your renderer side.
//   - D3D12ResizeWindow(), D3D12StartFrame(), D3D12EndFrame(), D3D12Cleanup()
//     are also provided.
//   - This file is compiled as C++/CX for UWP.
//

#include <windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include <ppltasks.h>

#include <agile.h>

#include <collection.h>

#include <string>
#include <memory>

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <windows.ui.core.h>
#include <windows.applicationmodel.core.h>
#include <windows.system.h>
#include <windows.foundation.h>
#include <windows.globalization.h>
#include <windows.ui.viewmanagement.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>

using namespace Windows::Globalization;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;

extern "C" {
#include <strato/strato.h>
};
#include "d3d12.h"

static bool g_videoPlaying = false;
static bool g_fullScreen = false;

static const wchar_t* UWP_LOG_FILE_NAME = L"log.txt";



static int ConvertVirtualKey(VirtualKey key)
{
    // TODO:
    // Replace this with your engine's exact keycode mapping.
    // For now, pass through a subset that is often useful.
    switch (key)
    {
    case VirtualKey::Enter:   return 0x0D;
    case VirtualKey::Escape:  return 0x1B;
    case VirtualKey::Space:   return 0x20;
    case VirtualKey::Left:    return 0x25;
    case VirtualKey::Up:      return 0x26;
    case VirtualKey::Right:   return 0x27;
    case VirtualKey::Down:    return 0x28;
    case VirtualKey::Control: return 0x11;
    case VirtualKey::Shift:   return 0x10;
    case VirtualKey::Tab:     return 0x09;
    case VirtualKey::Back:    return 0x08;
    default:
        break;
    }

    // ASCII-ish A-Z, 0-9 passthrough
    int v = static_cast<int>(key);
    if ((v >= '0' && v <= '9') || (v >= 'A' && v <= 'Z'))
        return v;

    return 0;
}

static int ConvertPointerButton(PointerEventArgs^ args)
{
    auto pt = args->CurrentPoint;
    auto props = pt->Properties;

    if (props->IsRightButtonPressed)
        return HAL_MOUSE_RIGHT;
    if (props->IsMiddleButtonPressed)
        return HAL_MOUSE_MIDDLE;
    return HAL_MOUSE_LEFT;
}

// -----------------------------------------------------------------------------
// App runtime
// -----------------------------------------------------------------------------

ref class UWPFrameworkView sealed : public IFrameworkView
{
internal:
    UWPFrameworkView();

public:
    virtual void Initialize(CoreApplicationView^ applicationView);
    virtual void SetWindow(CoreWindow^ window);
    virtual void Load(String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

private:
    // Application lifecycle handlers
    void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args);
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
    void OnResuming(Platform::Object^ sender, Platform::Object^ args);

    // Window handlers
    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args);
    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
    void OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args);

    // Input handlers
    void OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args);
    void OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args);
    void OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args);
    void OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args);
    void OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args);
    void OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args);

private:
    Agile<CoreWindow> m_window;

    bool m_windowClosed;
    bool m_windowVisible;
    bool m_initialized;
    bool m_started;

    int m_logicalWidth;
    int m_logicalHeight;

    // TODO:
    // Later you can replace these with proper viewport transform values.
    float m_mouseScaleX;
    float m_mouseScaleY;
    int m_mouseOffsetX;
    int m_mouseOffsetY;
};

UWPFrameworkView::UWPFrameworkView()
    : m_windowClosed(false)
    , m_windowVisible(true)
    , m_initialized(false)
    , m_started(false)
    , m_logicalWidth(1280)
    , m_logicalHeight(720)
    , m_mouseScaleX(1.0f)
    , m_mouseScaleY(1.0f)
    , m_mouseOffsetX(0)
    , m_mouseOffsetY(0)
{
}

void UWPFrameworkView::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(
            this, &UWPFrameworkView::OnActivated);

    Windows::ApplicationModel::Core::CoreApplication::Suspending +=
        ref new EventHandler<Windows::ApplicationModel::SuspendingEventArgs^>(
            this, &UWPFrameworkView::OnSuspending);

    Windows::ApplicationModel::Core::CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(
            this, &UWPFrameworkView::OnResuming);

    // Ask the engine what logical screen size it wants.
    const char* windowTitle = nullptr;
    hal_callback_on_event_boot(&windowTitle, &m_logicalWidth, &m_logicalHeight);

    m_initialized = true;
}

void UWPFrameworkView::SetWindow(CoreWindow^ window)
{
    m_window = window;

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
            this, &UWPFrameworkView::OnVisibilityChanged);

    window->Closed +=
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(
            this, &UWPFrameworkView::OnWindowClosed);

    window->SizeChanged +=
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
            this, &UWPFrameworkView::OnSizeChanged);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
            this, &UWPFrameworkView::OnPointerMoved);

    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
            this, &UWPFrameworkView::OnPointerPressed);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
            this, &UWPFrameworkView::OnPointerReleased);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
            this, &UWPFrameworkView::OnPointerWheelChanged);

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
            this, &UWPFrameworkView::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(
            this, &UWPFrameworkView::OnKeyUp);

    // Initialize D3D12 renderer for UWP.
    // The renderer side can internally use CreateSwapChainForCoreWindow()
    // or any later replacement you choose.
    IUnknown* nativeWindow = reinterpret_cast<IUnknown*>(window);

    if (!D3D12InitializeForUWP(nativeWindow, m_logicalWidth, m_logicalHeight))
    {
        // In a fuller implementation, log and terminate cleanly.
        ::OutputDebugStringW(L"D3D12InitializeForUWP() failed.\n");
    }
}

void UWPFrameworkView::Load(String^)
{
    // Nothing needed for now.
}

void UWPFrameworkView::Run()
{
    if (!m_initialized)
        return;

    auto window = m_window.Get();
    if (window == nullptr)
        return;

    if (!m_started)
    {
        hal_callback_on_event_start();
        m_started = true;
    }

    while (!m_windowClosed)
    {
        if (m_windowVisible)
        {
            window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            D3D12StartFrame();
            hal_callback.on_updaet();
            hal_callback.on_render();
            D3D12EndFrame();
        }
        else
        {
            window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }

    if (m_started)
    {
        hal_callback_on_event_stop();
        m_started = false;
    }
}

void UWPFrameworkView::Uninitialize()
{
    D3D12Cleanup();
}

void UWPFrameworkView::OnActivated(CoreApplicationView^, IActivatedEventArgs^)
{
    auto window = m_window.Get();
    if (window != nullptr)
        window->Activate();
}

void UWPFrameworkView::OnSuspending(
    Platform::Object^,
    Windows::ApplicationModel::SuspendingEventArgs^)
{
    // TODO:
    // Save state, flush resources, suspend audio/video, etc.
    hal_callback_on_event_suspend();
}

void UWPFrameworkView::OnResuming(Platform::Object^, Platform::Object^)
{
    // TODO:
    // Restore transient resources if needed.
    hal_callback_on_event_resume();
}

void UWPFrameworkView::OnWindowClosed(CoreWindow^, CoreWindowEventArgs^)
{
    m_windowClosed = true;
}

void UWPFrameworkView::OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args)
{
    m_windowVisible = args->Visible;
}

void UWPFrameworkView::OnSizeChanged(CoreWindow^, WindowSizeChangedEventArgs^ args)
{
    // TODO:
    // Later compute proper viewport, letterboxing, scale, offsets, etc.
    int newWidth = static_cast<int>(args->Size.Width);
    int newHeight = static_cast<int>(args->Size.Height);

    if (newWidth < 1)
        newWidth = 1;
    if (newHeight < 1)
        newHeight = 1;

    D3D12ResizeWindow(newWidth, newHeight);

    // Placeholder identity mapping for pointer coordinates.
    m_mouseScaleX = 1.0f;
    m_mouseScaleY = 1.0f;
    m_mouseOffsetX = 0;
    m_mouseOffsetY = 0;
}

void UWPFrameworkView::OnPointerMoved(CoreWindow^, PointerEventArgs^ args)
{
    auto pt = args->CurrentPoint->Position;

    int x = static_cast<int>((pt.X - m_mouseOffsetX) / m_mouseScaleX);
    int y = static_cast<int>((pt.Y - m_mouseOffsetY) / m_mouseScaleY);

    hal_callback_on_event_mouse_move(x, y);
}

void UWPFrameworkView::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    sender->SetPointerCapture();

    auto pt = args->CurrentPoint->Position;
    int x = static_cast<int>((pt.X - m_mouseOffsetX) / m_mouseScaleX);
    int y = static_cast<int>((pt.Y - m_mouseOffsetY) / m_mouseScaleY);
    int button = ConvertPointerButton(args);

    hal_callback_on_event_mouse_press(button, x, y);
}

void UWPFrameworkView::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    sender->ReleasePointerCapture(args->CurrentPoint->PointerId);

    auto pt = args->CurrentPoint->Position;
    int x = static_cast<int>((pt.X - m_mouseOffsetX) / m_mouseScaleX);
    int y = static_cast<int>((pt.Y - m_mouseOffsetY) / m_mouseScaleY);
    int button = ConvertPointerButton(args);

    hal_callback_on_event_mouse_release(button, x, y);
}

void UWPFrameworkView::OnPointerWheelChanged(CoreWindow^, PointerEventArgs^ args)
{
    auto pt = args->CurrentPoint->Position;
    int x = static_cast<int>((pt.X - m_mouseOffsetX) / m_mouseScaleX);
    int y = static_cast<int>((pt.Y - m_mouseOffsetY) / m_mouseScaleY);

    int delta = args->CurrentPoint->Properties->MouseWheelDelta;
    hal_callback_on_event_mouse_wheel(delta, x, y);
}

void UWPFrameworkView::OnKeyDown(CoreWindow^, KeyEventArgs^ args)
{
    int kc = ConvertVirtualKey(args->VirtualKey);
    if (kc != 0)
        hal_callback_on_event_key_press(kc);
}

void UWPFrameworkView::OnKeyUp(CoreWindow^, KeyEventArgs^ args)
{
    int kc = ConvertVirtualKey(args->VirtualKey);
    if (kc != 0)
        hal_callback_on_event_key_release(kc);
}

// -----------------------------------------------------------------------------
// View source
// -----------------------------------------------------------------------------

ref class UWPFrameworkViewSource sealed : public IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
    {
        return ref new UWPFrameworkView();
    }
};

// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto viewSource = ref new UWPFrameworkViewSource();
    CoreApplication::Run(viewSource);
    return 0;
}


// -----------------------------------------------------------------------------
// UTF helpers
// -----------------------------------------------------------------------------

static std::wstring UwpUtf8ToUtf16(const char* s)
{
    if (s == nullptr)
        return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0)
        return L"";

    std::wstring out;
    out.resize((size_t)len - 1);
    if (len > 1)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len);

    return out;
}

static std::string UwpUtf16ToUtf8(const wchar_t* s)
{
    if (s == nullptr)
        return "";

    int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return "";

    std::string out;
    out.resize((size_t)len - 1);
    if (len > 1)
        WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], len, nullptr, nullptr);

    return out;
}

// -----------------------------------------------------------------------------
// Timer
// -----------------------------------------------------------------------------

extern "C" void
hal_reset_lap_timer(uint64_t* t)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER now;

    if (t == nullptr)
        return;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);

    *t = (uint64_t)((now.QuadPart * 1000ULL) / (uint64_t)freq.QuadPart);
}

extern "C" uint64_t
hal_get_lap_timer_millisec(uint64_t* t)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER now;
    uint64_t cur;

    if (t == nullptr)
        return 0;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);

    cur = (uint64_t)((now.QuadPart * 1000ULL) / (uint64_t)freq.QuadPart);
    return cur - *t;
}

// -----------------------------------------------------------------------------
// Logging
// -----------------------------------------------------------------------------

static bool UwpAppendLogLine(const char* line)
{
    if (line == nullptr)
        return false;

    // Always send to debugger.
    std::wstring wline = UwpUtf8ToUtf16(line);
    std::wstring wlineCrLf = wline + L"\r\n";
    ::OutputDebugStringW(wlineCrLf.c_str());

    // Best-effort append to LocalFolder\log.txt
    try {
        StorageFolder^ local = ApplicationData::Current->LocalFolder;
        StorageFile^ file =
            concurrency::create_task(local->CreateFileAsync(
                ref new Platform::String(UWP_LOG_FILE_NAME),
                CreationCollisionOption::OpenIfExists)).get();

        Platform::String^ text = ref new Platform::String(wlineCrLf.c_str());
        concurrency::create_task(FileIO::AppendTextAsync(file, text)).get();
    }
    catch (...) {
        // Debug output already happened; keep this best-effort.
    }

    return true;
}

static bool UwpLogV(const char* prefix, const char* fmt, va_list ap)
{
    char body[2048];
    char line[2200];

#if defined(_MSC_VER)
    vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);
#else
    vsnprintf(body, sizeof(body), fmt, ap);
#endif

    if (prefix != nullptr && prefix[0] != '\0') {
#if defined(_MSC_VER)
        snprintf(line, sizeof(line), "[%s] %s", prefix, body);
#else
        snprintf(line, sizeof(line), "[%s] %s", prefix, body);
#endif
        return UwpAppendLogLine(line);
    }

    return UwpAppendLogLine(body);
}

extern "C" bool
hal_log_info(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);
    bool ok = UwpLogV("INFO", s, ap);
    va_end(ap);
    return ok;
}

extern "C" bool
hal_log_warn(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);
    bool ok = UwpLogV("WARN", s, ap);
    va_end(ap);
    return ok;
}

extern "C" bool
hal_log_error(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);
    bool ok = UwpLogV("ERROR", s, ap);
    va_end(ap);
    return ok;
}

extern "C" bool
hal_log_out_of_memory(void)
{
    return hal_log_error("Out of memory.");
}

// -----------------------------------------------------------------------------
// System language
// -----------------------------------------------------------------------------

extern "C" const char*
hal_get_system_language(void)
{
    static char lang[16];

    try {
        auto langs = ApplicationLanguages::Languages;
        if (langs != nullptr && langs->Size > 0) {
            std::wstring w = langs->GetAt(0)->Data();
            std::string utf8 = UwpUtf16ToUtf8(w.c_str());

            // Normalize to the engine's short language style.
            if (utf8.size() >= 2) {
                lang[0] = (char)tolower((unsigned char)utf8[0]);
                lang[1] = (char)tolower((unsigned char)utf8[1]);
                lang[2] = '\0';
                return lang;
            }
        }
    }
    catch (...) {
    }

    strcpy_s(lang, "en");
    return lang;
}

// -----------------------------------------------------------------------------
// Continuous swipe
// -----------------------------------------------------------------------------

extern "C" void
hal_set_continuous_swipe_enabled(bool is_enabled)
{
    // TODO:
    // Hook this to a real gesture recognizer if needed.
    (void)is_enabled;
}

// -----------------------------------------------------------------------------
// Full screen
// -----------------------------------------------------------------------------

extern "C" bool
hal_is_full_screen_supported(void)
{
    return true;
}

extern "C" bool
hal_is_full_screen_mode(void)
{
    return g_fullScreen;
}

extern "C" void
hal_enter_full_screen_mode(void)
{
    try {
        auto view = ApplicationView::GetForCurrentView();
        if (view != nullptr && view->TryEnterFullScreenMode())
            g_fullScreen = true;
    }
    catch (...) {
    }
}

extern "C" void
hal_leave_full_screen_mode(void)
{
    try {
        auto view = ApplicationView::GetForCurrentView();
        if (view != nullptr) {
            view->ExitFullScreenMode();
            g_fullScreen = false;
        }
    }
    catch (...) {
    }
}

// -----------------------------------------------------------------------------
// Video
// -----------------------------------------------------------------------------

extern "C" bool
hal_play_video(const char* fname, bool is_skippable)
{
    // TODO:
    // Later replace with MediaPlayer / MediaPlayerElement path.
    (void)fname;
    (void)is_skippable;

    g_videoPlaying = false;
    return true;
}

extern "C" void
hal_stop_video(void)
{
    g_videoPlaying = false;
}

extern "C" bool
hal_is_video_playing(void)
{
    return g_videoPlaying;
}
