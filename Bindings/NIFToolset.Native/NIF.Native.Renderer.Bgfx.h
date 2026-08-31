#pragma once
#ifndef NIF_NATIVE_RENDERER_BGFX_H
#define NIF_NATIVE_RENDERER_BGFX_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct NIF_BgfxRendererDesc
{
    void* nativeWindowHandle;
    unsigned int width;
    unsigned int height;
    int vsync;
} NIF_BgfxRendererDesc;

NIFTOOLSET_NATIVE_ENTRY void NIF_BgfxRenderer_FillDefaultDesc(NIF_BgfxRendererDesc* desc);
NIFTOOLSET_NATIVE_ENTRY void NIF_BgfxRenderer_FillWindowedDesc(NIF_BgfxRendererDesc* desc, void* nativeWindowHandle);
NIFTOOLSET_NATIVE_ENTRY NIF_RendererHandle NIF_BgfxRenderer_Create(const NIF_BgfxRendererDesc* desc);
NIFTOOLSET_NATIVE_ENTRY int NIF_BgfxRenderer_Resize(NIF_RendererHandle renderer, unsigned int width, unsigned int height, int vsync);
NIFTOOLSET_NATIVE_ENTRY int NIF_BgfxRenderer_GetVSync(NIF_RendererHandle renderer);

#ifdef __cplusplus
}
#endif

#endif
