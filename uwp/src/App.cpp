#include "pch.h"
#include "App.h"
#include "MainPage.h"

namespace winrt::SoftRendererUwp::implementation
{
    using winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs;
    using winrt::Windows::ApplicationModel::SuspendingEventArgs;
    using winrt::Windows::Foundation::IInspectable;
    using winrt::Windows::System::Profile::AnalyticsInfo;
    using winrt::Windows::UI::Xaml::ApplicationRequiresPointerMode;
    using winrt::Windows::UI::Xaml::Controls::Frame;
    using winrt::Windows::UI::Xaml::Navigation::NavigationFailedEventArgs;
    using winrt::Windows::UI::Xaml::Window;
    using winrt::Windows::UI::ViewManagement::ApplicationView;
    using winrt::Windows::UI::ViewManagement::ApplicationViewBoundsMode;
    using winrt::Windows::UI::ViewManagement::ApplicationViewWindowingMode;
    using winrt::box_value;
    using winrt::hresult_error;
    using winrt::hstring;
    using winrt::xaml_typename;

    namespace
    {
        bool IsXbox()
        {
            static const bool isXbox =
                AnalyticsInfo::VersionInfo().DeviceFamily() == L"Windows.Xbox";
            return isXbox;
        }
    }

    App::App()
    {
        if (IsXbox())
        {
            RequiresPointerMode(ApplicationRequiresPointerMode::WhenRequested);
            ApplicationView::PreferredLaunchWindowingMode(
                ApplicationViewWindowingMode::FullScreen);
        }
        Suspending({ this, &App::OnSuspending });
    }

    void App::OnLaunched(LaunchActivatedEventArgs const& args)
    {
        if (args.PrelaunchActivated())
            return;

        Frame rootFrame{ nullptr };
        if (auto const content = Window::Current().Content())
            rootFrame = content.try_as<Frame>();

        if (rootFrame == nullptr)
        {
            rootFrame = Frame();
            rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });
            Window::Current().Content(rootFrame);
        }

        if (rootFrame.Content() == nullptr)
        {
            rootFrame.Navigate(
                xaml_typename<SoftRendererUwp::MainPage>(),
                box_value(args.Arguments()));
        }

        Window::Current().Activate();
        ConfigureFullScreen();
    }

    void App::ConfigureFullScreen()
    {
        if (!IsXbox())
            return;

        auto view = ApplicationView::GetForCurrentView();
        view.SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);
        if (!view.IsFullScreenMode() && !view.TryEnterFullScreenMode())
            ::OutputDebugStringW(L"Unable to enter full-screen app view.\n");
    }

    void App::OnSuspending(IInspectable const&, SuspendingEventArgs const&)
    {
    }

    void App::OnNavigationFailed(
        IInspectable const&,
        NavigationFailedEventArgs const& args)
    {
        throw hresult_error(
            E_FAIL,
            hstring(L"Failed to load Page ") + args.SourcePageType().Name);
    }
}
