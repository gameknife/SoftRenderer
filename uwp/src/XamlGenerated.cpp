#include "pch.h"

#include "MainPage.h"

#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>

namespace winrt::SoftRendererUwp::implementation
{
    using Application = ::winrt::Windows::UI::Xaml::Application;
    using ComponentResourceLocation =
        ::winrt::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation;
    using DependencyObject = ::winrt::Windows::UI::Xaml::DependencyObject;
    using IComponentConnector =
        ::winrt::Windows::UI::Xaml::Markup::IComponentConnector;
    using IInspectable = ::winrt::Windows::Foundation::IInspectable;
    using Uri = ::winrt::Windows::Foundation::Uri;

    template <typename D, typename... I>
    void MainPageT<D, I...>::InitializeComponent()
    {
        if (!_contentLoaded)
        {
            _contentLoaded = true;
            Uri resourceLocator{ L"ms-appx:///MainPage.xaml" };
            Application::LoadComponent(
                *this,
                resourceLocator,
                ComponentResourceLocation::Application);
        }
    }

    template <typename D, typename... I>
    void MainPageT<D, I...>::Connect(
        int32_t connectionId,
        IInspectable const& target)
    {
        switch (connectionId)
        {
        case 2:
            this->SwapChainPanel(
                target.as<::winrt::Windows::UI::Xaml::Controls::SwapChainPanel>());
            break;
        }
        _contentLoaded = true;
    }

    template <typename D, typename... I>
    void MainPageT<D, I...>::DisconnectUnloadedObject(int32_t)
    {
        throw ::winrt::hresult_invalid_argument{
            L"No unloadable objects to disconnect." };
    }

    template <typename D, typename... I>
    void MainPageT<D, I...>::UnloadObject(DependencyObject const&)
    {
        throw ::winrt::hresult_invalid_argument{ L"No unloadable objects." };
    }

    template <typename D, typename... I>
    IComponentConnector MainPageT<D, I...>::GetBindingConnector(
        int32_t,
        IInspectable const&)
    {
        return nullptr;
    }

    template struct MainPageT<struct MainPage>;
}
