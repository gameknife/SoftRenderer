#include "pch.h"
#include "Renderer.h"

#include "SoftRendererUwpShaders.h"

#include <windows.ui.xaml.media.dxinterop.h>

#include <cstring>
#include <limits>

using Microsoft::WRL::ComPtr;
using winrt::check_hresult;
using winrt::throw_last_error;

namespace SoftRendererUwp
{
    namespace
    {
        constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    void Renderer::Transition(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    Renderer::~Renderer()
    {
        if (m_initialized)
        {
            try
            {
                WaitForGpu();
            }
            catch (winrt::hresult_error const&)
            {
            }
        }
    }

    Renderer::PanelMetrics Renderer::MeasurePanel() const
    {
        const float scaleX = static_cast<float>(m_panel.CompositionScaleX());
        const float scaleY = static_cast<float>(m_panel.CompositionScaleY());
        return
        {
            static_cast<UINT>(m_panel.ActualWidth() * scaleX + 0.5),
            static_cast<UINT>(m_panel.ActualHeight() * scaleY + 0.5),
            scaleX,
            scaleY,
        };
    }

    void Renderer::Initialize(
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& panel)
    {
        if (m_initialized)
            return;

        m_panel = panel;
        const PanelMetrics metrics = MeasurePanel();
        if (metrics.Width == 0 || metrics.Height == 0)
            throw winrt::hresult_invalid_argument{
                L"SwapChainPanel has no physical layout size." };

        m_renderWidth = metrics.Width;
        m_renderHeight = metrics.Height;
        m_compositionScaleX = metrics.ScaleX;
        m_compositionScaleY = metrics.ScaleY;

        const wchar_t* stage = L"CreateDXGIFactory2";
        try
        {
            ComPtr<IDXGIFactory4> factory;
            check_hresult(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

            stage = L"CreateDeviceResources";
            CreateDeviceResources(factory);
            stage = L"CreateCommandObjects";
            CreateCommandObjects();
            stage = L"CreateSwapChain";
            CreateSwapChain(factory);
            stage = L"CreateRenderTargets";
            CreateRenderTargets();
            stage = L"CreatePipelineState";
            CreatePipelineState();
            stage = L"CreateSoftwareFrameResources";
            CreateSoftwareFrameResources();
            stage = L"Initialize software renderer";
            m_softRuntime.Initialize(SoftwareFrameWidth, SoftwareFrameHeight);
            m_initialized = true;
        }
        catch (winrt::hresult_error const& error)
        {
            throw winrt::hresult_error(
                error.code(),
                winrt::hstring(stage) + L": " + error.message());
        }
    }

    void Renderer::Resize()
    {
        if (!m_initialized)
            return;

        const PanelMetrics metrics = MeasurePanel();
        if (metrics.Width == 0 || metrics.Height == 0)
            return;

        const bool sizeChanged =
            metrics.Width != m_renderWidth || metrics.Height != m_renderHeight;
        const bool scaleChanged =
            metrics.ScaleX != m_compositionScaleX ||
            metrics.ScaleY != m_compositionScaleY;
        if (!sizeChanged && !scaleChanged)
            return;

        WaitForGpu();
        if (sizeChanged)
        {
            for (auto& renderTarget : m_renderTargets)
                renderTarget.Reset();
            m_rtvHeap.Reset();
            check_hresult(m_swapChain->ResizeBuffers(
                FrameCount,
                metrics.Width,
                metrics.Height,
                BackBufferFormat,
                0));
            m_renderWidth = metrics.Width;
            m_renderHeight = metrics.Height;
            CreateRenderTargets();
        }

        m_compositionScaleX = metrics.ScaleX;
        m_compositionScaleY = metrics.ScaleY;
        m_frameFenceValues.fill(0);
        SetSwapChainMatrix();
    }

    void Renderer::CreateDeviceResources(ComPtr<IDXGIFactory4> const& factory)
    {
        for (UINT adapterIndex = 0;; ++adapterIndex)
        {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT result = factory->EnumAdapters1(adapterIndex, &adapter);
            if (result == DXGI_ERROR_NOT_FOUND)
                break;
            check_hresult(result);

            DXGI_ADAPTER_DESC1 description{};
            check_hresult(adapter->GetDesc1(&description));
            if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
                continue;

            constexpr D3D_FEATURE_LEVEL levels[] =
            {
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_0,
            };
            for (const auto level : levels)
            {
                if (SUCCEEDED(D3D12CreateDevice(
                    adapter.Get(), level, IID_PPV_ARGS(&m_device))))
                {
                    m_featureLevel = level;
                    return;
                }
            }
        }

        check_hresult(DXGI_ERROR_UNSUPPORTED);
    }

    void Renderer::CreateCommandObjects()
    {
        D3D12_COMMAND_QUEUE_DESC queueDescription{};
        queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        check_hresult(m_device->CreateCommandQueue(
            &queueDescription, IID_PPV_ARGS(&m_commandQueue)));

        for (auto& allocator : m_commandAllocators)
        {
            check_hresult(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
        }

        check_hresult(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(&m_commandList)));
        check_hresult(m_commandList->Close());

        check_hresult(m_device->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;
        m_fenceEvent = winrt::handle{ CreateEvent(nullptr, FALSE, FALSE, nullptr) };
        if (!m_fenceEvent)
            throw_last_error();
    }

    void Renderer::CreateSwapChain(ComPtr<IDXGIFactory4> const& factory)
    {
        DXGI_SWAP_CHAIN_DESC1 description{};
        description.Width = m_renderWidth;
        description.Height = m_renderHeight;
        description.Format = BackBufferFormat;
        description.SampleDesc.Count = 1;
        description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        description.BufferCount = FrameCount;
        description.Scaling = DXGI_SCALING_STRETCH;
        description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ComPtr<IDXGISwapChain1> swapChain;
        check_hresult(factory->CreateSwapChainForComposition(
            m_commandQueue.Get(), &description, nullptr, &swapChain));
        check_hresult(swapChain.As(&m_swapChain));
        SetSwapChainMatrix();

        auto panelNative = m_panel.as<ISwapChainPanelNative>();
        check_hresult(panelNative->SetSwapChain(m_swapChain.Get()));
    }

    void Renderer::SetSwapChainMatrix()
    {
        ComPtr<IDXGISwapChain2> swapChain;
        check_hresult(m_swapChain.As(&swapChain));
        DXGI_MATRIX_3X2_F matrix{};
        matrix._11 = 1.0f / m_compositionScaleX;
        matrix._22 = 1.0f / m_compositionScaleY;
        check_hresult(swapChain->SetMatrixTransform(&matrix));
    }

    void Renderer::CreateRenderTargets()
    {
        D3D12_DESCRIPTOR_HEAP_DESC description{};
        description.NumDescriptors = FrameCount;
        description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        check_hresult(m_device->CreateDescriptorHeap(
            &description, IID_PPV_ARGS(&m_rtvHeap)));
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        auto descriptor = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT index = 0; index < FrameCount; ++index)
        {
            check_hresult(m_swapChain->GetBuffer(
                index, IID_PPV_ARGS(&m_renderTargets[index])));
            m_device->CreateRenderTargetView(
                m_renderTargets[index].Get(), nullptr, descriptor);
            descriptor.ptr += m_rtvDescriptorSize;
        }
    }

    void Renderer::CreatePipelineState()
    {
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.BaseShaderRegister = 0;
        range.OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER parameter{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable.NumDescriptorRanges = 1;
        parameter.DescriptorTable.pDescriptorRanges = &range;
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootDescription{};
        rootDescription.NumParameters = 1;
        rootDescription.pParameters = &parameter;
        rootDescription.NumStaticSamplers = 1;
        rootDescription.pStaticSamplers = &sampler;

        ComPtr<ID3DBlob> serialized;
        ComPtr<ID3DBlob> error;
        check_hresult(D3D12SerializeRootSignature(
            &rootDescription,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &serialized,
            &error));
        check_hresult(m_device->CreateRootSignature(
            0,
            serialized->GetBufferPointer(),
            serialized->GetBufferSize(),
            IID_PPV_ARGS(&m_frameRootSignature)));

        D3D12_RASTERIZER_DESC rasterizer{};
        rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizer.CullMode = D3D12_CULL_MODE_NONE;
        rasterizer.DepthClipEnable = TRUE;
        D3D12_BLEND_DESC blend{};
        blend.RenderTarget[0].RenderTargetWriteMask =
            D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
        pipeline.pRootSignature = m_frameRootSignature.Get();
        pipeline.VS =
        {
            SoftRendererUwp::shaders::SoftwareFrameVertexShader,
            SoftRendererUwp::shaders::SoftwareFrameVertexShaderSize
        };
        pipeline.PS =
        {
            SoftRendererUwp::shaders::SoftwareFramePixelShader,
            SoftRendererUwp::shaders::SoftwareFramePixelShaderSize
        };
        pipeline.RasterizerState = rasterizer;
        pipeline.BlendState = blend;
        pipeline.DepthStencilState.DepthEnable = FALSE;
        pipeline.DepthStencilState.StencilEnable = FALSE;
        pipeline.SampleMask = (std::numeric_limits<UINT>::max)();
        pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipeline.NumRenderTargets = 1;
        pipeline.RTVFormats[0] = BackBufferFormat;
        pipeline.SampleDesc.Count = 1;
        check_hresult(m_device->CreateGraphicsPipelineState(
            &pipeline, IID_PPV_ARGS(&m_framePipelineState)));
    }

    void Renderer::CreateSoftwareFrameResources()
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDescription{};
        heapDescription.NumDescriptors = 1;
        heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        check_hresult(m_device->CreateDescriptorHeap(
            &heapDescription, IID_PPV_ARGS(&m_frameSrvHeap)));

        D3D12_HEAP_PROPERTIES defaultHeap{};
        defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC textureDescription{};
        textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDescription.Width = SoftwareFrameWidth;
        textureDescription.Height = SoftwareFrameHeight;
        textureDescription.DepthOrArraySize = 1;
        textureDescription.MipLevels = 1;
        textureDescription.Format = BackBufferFormat;
        textureDescription.SampleDesc.Count = 1;
        textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        check_hresult(m_device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &textureDescription,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_frameTexture)));

        m_frameUploadRowPitch =
            (SoftwareFrameWidth * sizeof(std::uint32_t) + 255u) & ~255u;
        D3D12_HEAP_PROPERTIES uploadHeap{};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC uploadDescription{};
        uploadDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDescription.Width =
            static_cast<UINT64>(m_frameUploadRowPitch) * SoftwareFrameHeight;
        uploadDescription.Height = 1;
        uploadDescription.DepthOrArraySize = 1;
        uploadDescription.MipLevels = 1;
        uploadDescription.SampleDesc.Count = 1;
        uploadDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        check_hresult(m_device->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &uploadDescription,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_frameUpload)));

        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format = BackBufferFormat;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(
            m_frameTexture.Get(),
            &view,
            m_frameSrvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    void Renderer::UploadSoftwareFrame(SoftRendererRuntime::FrameView const& frame)
    {
        if (!frame.Pixels ||
            frame.Width != SoftwareFrameWidth ||
            frame.Height != SoftwareFrameHeight)
        {
            return;
        }

        D3D12_RANGE readRange{ 0, 0 };
        void* mappedData = nullptr;
        check_hresult(m_frameUpload->Map(0, &readRange, &mappedData));
        auto const* source = reinterpret_cast<std::uint8_t const*>(frame.Pixels);
        auto* destination = reinterpret_cast<std::uint8_t*>(mappedData);
        for (UINT row = 0; row < SoftwareFrameHeight; ++row)
        {
            std::memcpy(
                destination + static_cast<size_t>(row) * m_frameUploadRowPitch,
                source + static_cast<size_t>(row) * frame.PitchBytes,
                SoftwareFrameWidth * sizeof(std::uint32_t));
        }
        m_frameUpload->Unmap(0, nullptr);
    }

    void Renderer::Render()
    {
        if (!m_initialized)
            return;

        UploadSoftwareFrame(m_softRuntime.Render());
        const UINT frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        WaitForFrame(frameIndex);
        check_hresult(m_commandAllocators[frameIndex]->Reset());
        check_hresult(m_commandList->Reset(
            m_commandAllocators[frameIndex].Get(), m_framePipelineState.Get()));

        D3D12_VIEWPORT viewport{
            0.0f, 0.0f,
            static_cast<float>(m_renderWidth),
            static_cast<float>(m_renderHeight),
            0.0f, 1.0f };
        D3D12_RECT scissor{
            0, 0,
            static_cast<LONG>(m_renderWidth),
            static_cast<LONG>(m_renderHeight) };
        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissor);

        if (m_frameTextureState != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            Transition(
                m_commandList.Get(),
                m_frameTexture.Get(),
                m_frameTextureState,
                D3D12_RESOURCE_STATE_COPY_DEST);
            m_frameTextureState = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        D3D12_TEXTURE_COPY_LOCATION source{};
        source.pResource = m_frameUpload.Get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint.Footprint.Format = BackBufferFormat;
        source.PlacedFootprint.Footprint.Width = SoftwareFrameWidth;
        source.PlacedFootprint.Footprint.Height = SoftwareFrameHeight;
        source.PlacedFootprint.Footprint.Depth = 1;
        source.PlacedFootprint.Footprint.RowPitch = m_frameUploadRowPitch;

        D3D12_TEXTURE_COPY_LOCATION destination{};
        destination.pResource = m_frameTexture.Get();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destination.SubresourceIndex = 0;
        m_commandList->CopyTextureRegion(
            &destination, 0, 0, 0, &source, nullptr);
        Transition(
            m_commandList.Get(),
            m_frameTexture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_frameTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        Transition(
            m_commandList.Get(),
            m_renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto descriptor = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        descriptor.ptr += frameIndex * m_rtvDescriptorSize;
        m_commandList->OMSetRenderTargets(1, &descriptor, FALSE, nullptr);
        constexpr float clearColor[] = { 0.125f, 0.125f, 0.125f, 1.0f };
        m_commandList->ClearRenderTargetView(descriptor, clearColor, 0, nullptr);

        m_commandList->SetPipelineState(m_framePipelineState.Get());
        m_commandList->SetGraphicsRootSignature(m_frameRootSignature.Get());
        ID3D12DescriptorHeap* heaps[] = { m_frameSrvHeap.Get() };
        m_commandList->SetDescriptorHeaps(1, heaps);
        m_commandList->SetGraphicsRootDescriptorTable(
            0, m_frameSrvHeap->GetGPUDescriptorHandleForHeapStart());
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->DrawInstanced(3, 1, 0, 0);

        Transition(
            m_commandList.Get(),
            m_renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
        check_hresult(m_commandList->Close());
        ID3D12CommandList* commandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, commandLists);
        check_hresult(m_swapChain->Present(1, 0));

        const UINT64 fenceToSignal = m_fenceValue++;
        check_hresult(m_commandQueue->Signal(m_fence.Get(), fenceToSignal));
        m_frameFenceValues[frameIndex] = fenceToSignal;
    }

    void Renderer::WaitForFrame(UINT frameIndex)
    {
        const UINT64 fenceToWaitFor = m_frameFenceValues[frameIndex];
        if (fenceToWaitFor == 0 ||
            m_fence->GetCompletedValue() >= fenceToWaitFor)
        {
            return;
        }

        check_hresult(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent.get()));
        WaitForSingleObjectEx(m_fenceEvent.get(), INFINITE, FALSE);
    }

    void Renderer::WaitForGpu()
    {
        const UINT64 fenceToWaitFor = m_fenceValue++;
        check_hresult(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
        if (m_fence->GetCompletedValue() < fenceToWaitFor)
        {
            check_hresult(m_fence->SetEventOnCompletion(
                fenceToWaitFor, m_fenceEvent.get()));
            WaitForSingleObjectEx(m_fenceEvent.get(), INFINITE, FALSE);
        }
    }
}
