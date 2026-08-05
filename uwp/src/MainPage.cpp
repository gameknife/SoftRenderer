#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

#include <winrt/Windows.UI.Xaml.Media.h>

namespace winrt::SoftRendererUwp::implementation
{
    using winrt::Windows::UI::Xaml::Media::CompositionTarget;

    MainPage::~MainPage()
    {
        if (m_renderingSubscribed)
            CompositionTarget::Rendering(m_renderingToken);
    }

    MainPage::MainPage()
    {
        InitializeComponent();
        Loaded({ this, &MainPage::OnLoaded });
        SwapChainPanel().SizeChanged({ this, &MainPage::OnSizeChanged });
        SwapChainPanel().CompositionScaleChanged(
            { this, &MainPage::OnCompositionScaleChanged });
    }

    void MainPage::OnLoaded(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Windows::UI::Xaml::RoutedEventArgs const&)
    {
        try
        {
            m_renderer.Initialize(SwapChainPanel());
            if (!m_renderingSubscribed)
            {
                m_renderingToken =
                    CompositionTarget::Rendering({ this, &MainPage::OnRendering });
                m_renderingSubscribed = true;
            }
        }
        catch (winrt::hresult_error const& error)
        {
            ::OutputDebugStringW(error.message().c_str());
        }
    }

    void MainPage::ResizeSwapChain()
    {
        try
        {
            m_renderer.Resize();
        }
        catch (winrt::hresult_error const& error)
        {
            ::OutputDebugStringW(error.message().c_str());
        }
    }

    void MainPage::OnSizeChanged(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Windows::UI::Xaml::SizeChangedEventArgs const&)
    {
        ResizeSwapChain();
    }

    void MainPage::OnCompositionScaleChanged(
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&,
        winrt::Windows::Foundation::IInspectable const&)
    {
        ResizeSwapChain();
    }

    void MainPage::OnRendering(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Windows::Foundation::IInspectable const&)
    {
        try
        {
            m_renderer.Render();
        }
        catch (winrt::hresult_error const& error)
        {
            ::OutputDebugStringW(error.message().c_str());
            if (m_renderingSubscribed)
            {
                CompositionTarget::Rendering(m_renderingToken);
                m_renderingSubscribed = false;
            }
        }
    }
}
