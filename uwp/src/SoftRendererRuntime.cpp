#include "pch.h"
#include "SoftRendererRuntime.h"

#include "prerequisite.h"
#include "SrAppFramework.h"
#include "SrLogger.h"
#include "SrProfiler.h"
#include "SrResourceManager.h"
#include "SrRenderContext.h"
#include "SrSponzaApp.h"
#include "SrSoftRenderer.h"
#include "InputManager.h"
#include "BasicShaders.h"
#include "AdvanceShaders.h"

#include <cstdlib>

GlobalEnvironment* gEnv = nullptr;
SrRendContext* g_context = nullptr;

namespace SoftRendererUwp
{
    namespace
    {
        bool PresentToUwp(HWND, const void*)
        {
            // The D3D12 host uploads getBuffer() after EndFrame(). The
            // callback only completes the renderer's frame hand-off.
            return true;
        }

        void SetDefaultEnvironment()
        {
            if (!std::getenv("SR_RENDERER_BACKEND"))
                _putenv_s("SR_RENDERER_BACKEND", "tile");
            if (!std::getenv("SR_DISABLE_SSAO"))
                _putenv_s("SR_DISABLE_SSAO", "1");
        }

        void AddShaderList(SrResourceManager& resources)
        {
            resources.AddShader(new SrShader("flat", eVd_F4F4F4));
            resources.AddShader(new SrShader("gourand", eVd_F4F4));
            resources.AddShader(new SrShader("default", eVd_F4F4F4));
            resources.AddShader(new SrShader("skin", eVd_F4F4F4F4U4));
            resources.AddShader(new SrShader("hair", eVd_F4F4F4));
            resources.AddShader(new SrShader("fresnel", eVd_F4F4F4));
            resources.AddShader(new SrShader("default_normal", eVd_F4F4F4));
        }

        void RegisterStaticShaders(SrSoftRenderer& renderer)
        {
            renderer.RegisterStaticShader(&g_FlatShadingShader,
                gEnv->resourceMgr->GetShader("flat"));
            renderer.RegisterStaticShader(&g_GourandShadingShader,
                gEnv->resourceMgr->GetShader("gourand"));
            renderer.RegisterStaticShader(&g_PhongShadingShader,
                gEnv->resourceMgr->GetShader("default"));
            renderer.RegisterStaticShader(&g_PhongShadingWithNormalShader,
                gEnv->resourceMgr->GetShader("default_normal"));
            renderer.RegisterStaticShader(&g_SkinSimShader,
                gEnv->resourceMgr->GetShader("skin"));
            renderer.RegisterStaticShader(&g_FresnelNormalShader,
                gEnv->resourceMgr->GetShader("fresnel"));
            renderer.RegisterStaticShader(&g_HairShader,
                gEnv->resourceMgr->GetShader("hair"));
        }
    }

    SoftRendererRuntime::~SoftRendererRuntime()
    {
        if (m_task)
        {
            m_task->OnDestroy();
            delete m_task;
            m_task = nullptr;
        }

        if (m_renderer)
        {
            m_renderer->ShutdownRenderer();
            delete m_renderer;
            m_renderer = nullptr;
        }

        if (m_input)
        {
            m_input->Destroy();
            delete m_input;
            m_input = nullptr;
        }

        delete m_profiler;
        delete m_timer;
        delete m_resources;
        delete m_logger;
        delete m_context;

        m_environment = nullptr;
        g_context = nullptr;
        gEnv = nullptr;
    }

    void SoftRendererRuntime::Initialize(
        std::uint32_t width,
        std::uint32_t height)
    {
        if (m_initialized)
            return;
        if (width == 0 || height == 0)
            throw winrt::hresult_invalid_argument{
                L"Software renderer size is zero." };

        SetDefaultEnvironment();
        m_width = width;
        m_height = height;
        m_environment = new GlobalEnvironment{};
        gEnv = m_environment;
        gEnv->present = PresentToUwp;

        m_logger = new SrLogger();
        gEnv->logger = m_logger;
        m_resources = new SrResourceManager();
        gEnv->resourceMgr = m_resources;
        AddShaderList(*m_resources);

        m_context = new SrRendContext(static_cast<int>(width), static_cast<int>(height), 32);
        g_context = m_context;
        gEnv->context = m_context;
        m_resources->InitDefaultMedia();

        m_renderer = new SrSoftRenderer();
        gEnv->renderer = m_renderer;
        if (!m_renderer->InitRenderer(nullptr, static_cast<int>(width), static_cast<int>(height), 32))
            throw winrt::hresult_error(E_FAIL,
                L"CPU software renderer initialization failed.");
        RegisterStaticShaders(*m_renderer);

        m_timer = new SrTimer();
        m_timer->Init();
        gEnv->timer = m_timer;
        m_input = new SrInputManager();
        m_input->Init(nullptr);
        gEnv->inputSys = m_input;
        m_profiler = new SrProfiler();
        gEnv->profiler = m_profiler;

        m_task = new SrSponzaApp();
        m_task->OnInit();
        m_initialized = true;
    }

    SoftRendererRuntime::FrameView SoftRendererRuntime::Render()
    {
        if (!m_initialized || !m_renderer || !m_task)
            return {};

        m_timer->Update();
        m_input->Update();
        m_renderer->BeginFrame();
        m_renderer->HwClear();
        m_task->OnUpdate();
        m_renderer->EndFrame();

        return
        {
            static_cast<std::uint32_t const*>(m_renderer->getBuffer()),
            m_width,
            m_height,
            m_width * sizeof(std::uint32_t),
        };
    }
}
