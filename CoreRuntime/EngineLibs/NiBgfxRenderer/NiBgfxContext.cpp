#include "NiBgfxContext.h"

#include <NiLog.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace
{
class NiBgfxCallback final : public bgfx::CallbackI
{
public:
    static constexpr const char* SCREENSHOT_TOKEN = "__nibgfx_backbuffer__";

    bool BeginScreenshot()
    {
        bool expected = false;
        if (!m_pending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel))
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pixels.clear();
        m_width = m_height = m_pitch = 0;
        m_yFlip = false;
        m_ready.store(false, std::memory_order_release);
        return true;
    }

    void CancelScreenshot()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pixels.clear();
        m_width = m_height = m_pitch = 0;
        m_yFlip = false;
        m_ready.store(false, std::memory_order_release);
        m_pending.store(false, std::memory_order_release);
    }

    bool ConsumeScreenshot(std::vector<std::uint8_t>& pixels,
        unsigned int& width, unsigned int& height, unsigned int& pitch,
        bool& yFlip)
    {
        if (!m_ready.load(std::memory_order_acquire))
            return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_ready.load(std::memory_order_relaxed))
            return false;

        pixels = m_pixels;
        width = m_width;
        height = m_height;
        pitch = m_pitch;
        yFlip = m_yFlip;
        m_ready.store(false, std::memory_order_release);
        m_pending.store(false, std::memory_order_release);
        return true;
    }

    bool IsScreenshotReady() const
    {
        return m_ready.load(std::memory_order_acquire);
    }

    void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code,
        const char* str) override
    {
        NiLogWriteFormat(NI_LOG_FATAL, "bgfx", filePath,
            static_cast<unsigned int>(line),
            "Fatal error (code=%u): %s", static_cast<unsigned int>(code),
            str ? str : "<no message>");
    }

    void traceVargs(const char* filePath, uint16_t line, const char* format,
        va_list argList) override
    {
        // bgfx can emit a lot of trace output. Always forward it when an
        // application callback is installed; without one, keep the historical
        // debug-build-only debugger output behavior.
#if !defined(_DEBUG)
        if (!NiGetLogCallback())
            return;
#endif
        if (!format)
            return;

        char buffer[4096];
        va_list argsCopy;
        va_copy(argsCopy, argList);
        std::vsnprintf(buffer, sizeof(buffer), format, argsCopy);
        va_end(argsCopy);
        buffer[sizeof(buffer) - 1] = '\0';

        // bgfx trace strings commonly carry their own newline. Logging sinks
        // such as spdlog add one themselves, so normalize the callback payload.
        std::size_t length = std::strlen(buffer);
        while (length > 0 && (buffer[length - 1] == '\n' ||
            buffer[length - 1] == '\r'))
        {
            buffer[--length] = '\0';
        }

        if (length != 0)
        {
            NiLogWrite(NI_LOG_TRACE, "bgfx", buffer, filePath,
                static_cast<unsigned int>(line));
        }
    }

    void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerEnd() override {}
    uint32_t cacheReadSize(uint64_t) override { return 0; }
    bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
    void cacheWrite(uint64_t, const void*, uint32_t) override {}

    void screenShot(const char* filePath, uint32_t width, uint32_t height,
        uint32_t pitch, const void* data, uint32_t size, bool yFlip) override
    {
        if (!m_pending.load(std::memory_order_acquire) || !filePath ||
            std::strcmp(filePath, SCREENSHOT_TOKEN) != 0 || !data || size == 0)
        {
            return;
        }

        // bgfx 1.129 guarantees 4-byte BGRA screenshot pixels. Validate the
        // pitch/size relationship before exposing the capture to Gamebryo.
        if (width == 0 || height == 0 || pitch < width * 4u ||
            size < pitch * height)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "bgfx", __FILE__, __LINE__,
                "Invalid screenshot payload: %ux%u pitch=%u size=%u.",
                width, height, pitch, size);
            CancelScreenshot();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            const auto* bytes = static_cast<const std::uint8_t*>(data);
            m_pixels.assign(bytes, bytes + size);
            m_width = width;
            m_height = height;
            m_pitch = pitch;
            m_yFlip = yFlip;
        }
        m_ready.store(true, std::memory_order_release);
    }

    void captureBegin(uint32_t, uint32_t, uint32_t,
        bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, uint32_t) override {}

private:
    mutable std::mutex m_mutex;
    std::vector<std::uint8_t> m_pixels;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_pitch = 0;
    bool m_yFlip = false;
    std::atomic<bool> m_pending{ false };
    std::atomic<bool> m_ready{ false };
};
}

NiBgfxContext::~NiBgfxContext()
{
    Shutdown();
}

bool NiBgfxContext::Initialize(void* nativeWindowHandle, unsigned int width,
    unsigned int height, bool vsync)
{
    if (m_initialized)
    {
        NiLogWrite(NI_LOG_WARNING, "bgfx", "Initialize called while bgfx is already initialized.");
        return false;
    }
    if (!nativeWindowHandle || !width || !height)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "bgfx", __FILE__, __LINE__,
            "Invalid initialization parameters: window=%p size=%ux%u.",
            nativeWindowHandle, width, height);
        return false;
    }

    NiLogWriteFormat(NI_LOG_INFO, "bgfx", __FILE__, __LINE__,
        "Initializing bgfx: window=%p size=%ux%u vsync=%s.",
        nativeWindowHandle, width, height, vsync ? "true" : "false");

    m_callback = std::make_unique<NiBgfxCallback>();

    bgfx::Init init{};
    init.type = bgfx::RendererType::Count;
    init.platformData.nwh = nativeWindowHandle;
    init.callback = m_callback.get();
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;

    m_initialized = bgfx::init(init);
    if (!m_initialized)
    {
        NiLogWrite(NI_LOG_ERROR, "bgfx",
            "bgfx::init failed. Check the preceding bgfx fatal/trace messages for the backend/device error.",
            __FILE__, __LINE__);
        m_callback.reset();
        return false;
    }

    const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    const bgfx::Caps* caps = bgfx::getCaps();
    NiLogWriteFormat(NI_LOG_INFO, "bgfx", __FILE__, __LINE__,
        "Initialized renderer '%s' (vendor=0x%04x device=0x%04x, homogeneousDepth=%s, originBottomLeft=%s).",
        bgfx::getRendererName(rendererType),
        caps ? static_cast<unsigned int>(caps->vendorId) : 0u,
        caps ? static_cast<unsigned int>(caps->deviceId) : 0u,
        caps && caps->homogeneousDepth ? "true" : "false",
        caps && caps->originBottomLeft ? "true" : "false");

    if (caps)
    {
        NiLogWriteFormat(NI_LOG_INFO, "bgfx", __FILE__, __LINE__,
            "Capabilities: mask=0x%016llx instancing=%s textureBlit=%s textureReadBack=%s maxFBAttachments=%u.",
            static_cast<unsigned long long>(caps->supported),
            (caps->supported & BGFX_CAPS_INSTANCING) != 0 ? "yes" : "no",
            (caps->supported & BGFX_CAPS_TEXTURE_BLIT) != 0 ? "yes" : "no",
            (caps->supported & BGFX_CAPS_TEXTURE_READ_BACK) != 0 ? "yes" : "no",
            static_cast<unsigned int>(caps->limits.maxFBAttachments));
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x000000ff, 1.0f, 0);
    bgfx::touch(0);
    bgfx::frame();
    return true;
}

void NiBgfxContext::Shutdown()
{
    if (!m_initialized)
    {
        m_callback.reset();
        return;
    }

    NiLogWrite(NI_LOG_INFO, "bgfx", "Shutting down bgfx.");
    bgfx::shutdown();
    m_initialized = false;
    m_callback.reset();
}

void NiBgfxContext::Reset(unsigned int width, unsigned int height, bool vsync)
{
    if (!m_initialized)
    {
        NiLogWrite(NI_LOG_WARNING, "bgfx", "Reset ignored because bgfx is not initialized.");
        return;
    }
    if (width == 0 || height == 0)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "bgfx", __FILE__, __LINE__,
            "Reset rejected invalid size %ux%u.", width, height);
        return;
    }

    NiLogWriteFormat(NI_LOG_DEBUG, "bgfx", __FILE__, __LINE__,
        "Resetting back buffer to %ux%u, vsync=%s.", width, height,
        vsync ? "true" : "false");
    bgfx::reset(width, height, vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
}

unsigned int NiBgfxContext::Frame()
{
    if (!m_initialized)
        return 0;

    return bgfx::frame();
}

bool NiBgfxContext::CaptureFrameBuffer(bgfx::FrameBufferHandle frameBuffer,
    std::vector<std::uint8_t>& pixels, unsigned int& width,
    unsigned int& height, unsigned int& pitch, bool& yFlip)
{
    if (!m_initialized || !m_callback)
    {
        NiLogWrite(NI_LOG_ERROR, "bgfx", "Framebuffer capture requested before bgfx callback initialization.");
        return false;
    }

    auto* callback = static_cast<NiBgfxCallback*>(m_callback.get());
    if (!callback->BeginScreenshot())
    {
        NiLogWrite(NI_LOG_WARNING, "bgfx", "Framebuffer capture rejected because another capture is pending.");
        return false;
    }

    bgfx::requestScreenShot(frameBuffer, NiBgfxCallback::SCREENSHOT_TOKEN);

    // Screenshot delivery is asynchronous on the render thread. Bound the
    // pump to avoid turning a backend/device failure into an application hang.
    constexpr unsigned int MAX_SCREENSHOT_FRAMES = 16;
    for (unsigned int i = 0; i < MAX_SCREENSHOT_FRAMES; ++i)
    {
        Frame();
        if (callback->IsScreenshotReady())
            return callback->ConsumeScreenshot(pixels, width, height, pitch, yFlip);
    }

    callback->CancelScreenshot();
    NiLogWriteFormat(NI_LOG_ERROR, "bgfx", __FILE__, __LINE__,
        "Framebuffer screenshot timed out after %u frames.", MAX_SCREENSHOT_FRAMES);
    return false;
}

bool NiBgfxContext::IsInitialized() const
{
    return m_initialized;
}
