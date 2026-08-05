#include "pch.h"

#include "App.h"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    winrt::init_apartment();
    winrt::Windows::UI::Xaml::Application::Start(
        [](auto&&)
        {
            winrt::make<winrt::SoftRendererUwp::implementation::App>();
        });
    return 0;
}
