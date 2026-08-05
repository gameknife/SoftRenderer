#pragma once

#include <cstdint>

struct GlobalEnvironment;
struct SrRendContext;
class SrSoftRenderer;
class SrInputManager;
class SrTimer;
struct IProfiler;
class SrResourceManager;
class SrLogger;
class SrAppFramework;

namespace SoftRendererUwp
{
    class SoftRendererRuntime final
    {
    public:
        struct FrameView
        {
            std::uint32_t const* Pixels{};
            std::uint32_t Width{};
            std::uint32_t Height{};
            std::uint32_t PitchBytes{};
        };

        SoftRendererRuntime() = default;
        SoftRendererRuntime(SoftRendererRuntime const&) = delete;
        SoftRendererRuntime& operator=(SoftRendererRuntime const&) = delete;
        ~SoftRendererRuntime();

        void Initialize(std::uint32_t width, std::uint32_t height);
        FrameView Render();
        bool IsInitialized() const noexcept { return m_initialized; }

    private:
        bool m_initialized{};
        std::uint32_t m_width{};
        std::uint32_t m_height{};
        ::GlobalEnvironment* m_environment{};
        ::SrRendContext* m_context{};
        ::SrSoftRenderer* m_renderer{};
        ::SrInputManager* m_input{};
        ::SrTimer* m_timer{};
        ::IProfiler* m_profiler{};
        ::SrResourceManager* m_resources{};
        ::SrLogger* m_logger{};
        ::SrAppFramework* m_task{};
    };
}
