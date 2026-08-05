#pragma once

#include "MainPage.g.h"
#include "Renderer.h"

namespace winrt::SoftRendererUwp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        ~MainPage();

    private:
        void OnLoaded(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Xaml::RoutedEventArgs const& args);
        void OnSizeChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Xaml::SizeChangedEventArgs const& args);
        void OnCompositionScaleChanged(
            winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& sender,
            winrt::Windows::Foundation::IInspectable const& args);
        void OnRendering(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::Foundation::IInspectable const& args);

        void ResizeSwapChain();

        ::SoftRendererUwp::Renderer m_renderer;
        winrt::event_token m_renderingToken{};
        bool m_renderingSubscribed{};
    };
}

namespace winrt::SoftRendererUwp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
