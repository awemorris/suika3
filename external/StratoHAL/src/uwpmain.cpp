// main.cpp (UWP / C++/WinRT minimal skeleton)

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

struct GameView : winrt::implements<GameView, winrt::Windows::ApplicationModel::Core::IFrameworkView>
{
    CoreWindow m_window{ nullptr };
    bool m_running = true;

    // Called when the app object is created
    void Initialize(CoreApplicationView const& /*appView*/)
    {
        // You can hook lifecycle events here (Activated, Suspending, Resuming, etc.)
    }

    // Called when the window is created
    void SetWindow(CoreWindow const& window)
    {
        m_window = window;

        // Exit on window close (Xbox Dev Mode may behave differently, but safe)
        m_window.Closed([this](auto const&, CoreWindowEventArgs const&)
        {
            m_running = false;
        });

        // Optional: handle key input (for quick exit testing on PC)
        m_window.KeyDown([this](auto const&, KeyEventArgs const& args)
        {
            // VirtualKey::Escape is in Windows::System, not included here to keep it minimal.
            // If you want: include <winrt/Windows.System.h> and check args.VirtualKey().
        });
    }

    // Called before Run
    void Load(hstring const& /*entryPoint*/) {}

    // The main loop
    void Run()
    {
        // Activate window (important!)
        m_window.Activate();

        while (m_running)
        {
            // Process queued events; don't block if none
            m_window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            // Your "tick" goes here:
            // Update();
            // Render();
        }
    }

    // Called when the app is shutting down
    void Uninitialize() {}
};

struct App : winrt::implements<App, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
    Windows::ApplicationModel::Core::IFrameworkView CreateView()
    {
        return winrt::make<GameView>();
    }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // UWP entrypoint (VS templates often use this signature)
    winrt::init_apartment();

    CoreApplication::Run(winrt::make<App>());

    return 0;
}
