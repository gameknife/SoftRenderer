#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <winrt/Windows.UI.Xaml.Controls.h>

#include "SoftRendererRuntime.h"

#include <array>
#include <cstdint>

namespace SoftRendererUwp
{
    class Renderer final
    {
    public:
        Renderer() = default;
        Renderer(Renderer const&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        ~Renderer();

        void Initialize(
            winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& panel);
        void Resize();
        void Render();

    private:
        static constexpr UINT FrameCount = 2;
        static constexpr UINT SoftwareFrameWidth = 854;
        static constexpr UINT SoftwareFrameHeight = 480;

        struct PanelMetrics
        {
            UINT Width;
            UINT Height;
            float ScaleX;
            float ScaleY;
        };

        PanelMetrics MeasurePanel() const;
        void CreateDeviceResources(
            Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory);
        void CreateCommandObjects();
        void CreateSwapChain(
            Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory);
        void CreateRenderTargets();
        void CreatePipelineState();
        void CreateSoftwareFrameResources();
        void UploadSoftwareFrame(SoftRendererRuntime::FrameView const& frame);
        void SetSwapChainMatrix();
        static void Transition(
            ID3D12GraphicsCommandList* commandList,
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after);
        void WaitForFrame(UINT frameIndex);
        void WaitForGpu();

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount>
            m_renderTargets;
        std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount>
            m_commandAllocators;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_frameRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_framePipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_frameTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_frameUpload;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_frameSrvHeap;

        UINT m_renderWidth{};
        UINT m_renderHeight{};
        UINT m_frameUploadRowPitch{};
        float m_compositionScaleX{ 1.0f };
        float m_compositionScaleY{ 1.0f };
        UINT m_rtvDescriptorSize{};
        UINT64 m_fenceValue{};
        std::array<UINT64, FrameCount> m_frameFenceValues{};
        winrt::handle m_fenceEvent;
        D3D12_RESOURCE_STATES m_frameTextureState{
            D3D12_RESOURCE_STATE_COPY_DEST };
        D3D_FEATURE_LEVEL m_featureLevel{ D3D_FEATURE_LEVEL_11_0 };
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel m_panel{ nullptr };
        bool m_initialized{};
        SoftRendererRuntime m_softRuntime;
    };
}
