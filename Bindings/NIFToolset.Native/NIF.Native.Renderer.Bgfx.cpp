#include "NIF.Native.Renderer.Bgfx.h"
#include "NIF.Native.Internal.h"

#include <BgfxRenderer.h>

namespace
{
    BgfxRenderer* NIF_GetBgfxRenderer(NIF_RendererHandle renderer)
    {
        NIF_RendererHandle_t* handle = static_cast<NIF_RendererHandle_t*>(renderer);
        NiRenderer* base = handle ? handle->spObject : nullptr;
        if (!base || base->GetRendererID() != NiSystemDesc::RENDERER_BGFX)
            return nullptr;
        return static_cast<BgfxRenderer*>(base);
    }
}

extern "C"
{

void NIF_BgfxRenderer_FillDefaultDesc(NIF_BgfxRendererDesc* desc)
{
    if (!desc)
        return;

    desc->nativeWindowHandle = nullptr;
    desc->width = 1280;
    desc->height = 720;
    desc->vsync = 1;
}

void NIF_BgfxRenderer_FillWindowedDesc(NIF_BgfxRendererDesc* desc,
    void* nativeWindowHandle)
{
    NIF_BgfxRenderer_FillDefaultDesc(desc);
    if (desc)
        desc->nativeWindowHandle = nativeWindowHandle;
}

NIF_RendererHandle NIF_BgfxRenderer_Create(const NIF_BgfxRendererDesc* desc)
{
    if (!desc || !desc->nativeWindowHandle || !desc->width || !desc->height)
    {
        NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT,
            "Bgfx renderer creation requires a native window handle and non-zero dimensions");
        return nullptr;
    }

    NiRendererPtr renderer = BgfxRenderer::Create(desc->nativeWindowHandle,
        desc->width, desc->height, desc->vsync != 0);
    if (!renderer)
    {
        NIF_SetLastError(NIF_RESULT_ENGINE_ERROR,
            "BgfxRenderer::Create failed");
        return nullptr;
    }

    return NIF_CreateRendererHandle(renderer);
}

int NIF_BgfxRenderer_Resize(NIF_RendererHandle renderer, unsigned int width,
    unsigned int height, int vsync)
{
    BgfxRenderer* bgfxRenderer = NIF_GetBgfxRenderer(renderer);
    if (!bgfxRenderer)
    {
        NIF_SetLastError(NIF_RESULT_INVALID_HANDLE,
            "Renderer handle is null or is not a bgfx renderer");
        return 0;
    }
    if (!width || !height)
    {
        NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT,
            "Bgfx renderer dimensions must be non-zero");
        return 0;
    }

    if (!bgfxRenderer->Resize(width, height, vsync != 0))
    {
        NIF_SetLastError(NIF_RESULT_ENGINE_ERROR,
            "BgfxRenderer::Resize failed");
        return 0;
    }
    return 1;
}

int NIF_BgfxRenderer_GetVSync(NIF_RendererHandle renderer)
{
    BgfxRenderer* bgfxRenderer = NIF_GetBgfxRenderer(renderer);
    if (!bgfxRenderer)
    {
        NIF_SetLastError(NIF_RESULT_INVALID_HANDLE,
            "Renderer handle is null or is not a bgfx renderer");
        return 0;
    }
    return bgfxRenderer->GetVSync() ? 1 : 0;
}

}
