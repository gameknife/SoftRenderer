#pragma once

#include "App.xaml.g.h"

namespace winrt::SoftRendererUwp::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(
            Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const& args);

    private:
        void ConfigureFullScreen();
        void OnSuspending(
            winrt::Windows::Foundation::IInspectable const& sender,
            Windows::ApplicationModel::SuspendingEventArgs const& args);
        void OnNavigationFailed(
            winrt::Windows::Foundation::IInspectable const& sender,
            Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const& args);
    };
}
