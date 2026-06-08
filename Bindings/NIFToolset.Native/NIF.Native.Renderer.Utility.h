#pragma once
#ifndef NIF_NATIVE_RENDERER_UTILITY_H
#define NIF_NATIVE_RENDERER_UTILITY_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_BeginDefaultScene(NIF_RendererHandle renderer, unsigned int clearFlags);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_EndScene(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetSorter(NIF_RendererHandle renderer, NIF_AlphaAccumulatorHandle accumulator);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_GetSorter(NIF_RendererHandle renderer, NIF_AlphaAccumulatorHandle* outAccumulator);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetCameraData(NIF_RendererHandle renderer, NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_GetCameraData(NIF_RendererHandle renderer, NIF_Vec3* worldLocation, NIF_Vec3* worldDirection, NIF_Vec3* worldUp, NIF_Vec3* worldRight, NIF_Frustum* frustum, NIF_Rect* viewport);
NIFTOOLSET_NATIVE_ENTRY void NIF_DX11Renderer_SetDefaultViewport(NIF_RendererHandle renderer, unsigned int width, unsigned int height);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_InitParticle(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_ShutdownParticle(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_InitPortal(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_ShutdownPortal(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_RenderSubsystems_InitShadowManager(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_ShutdownShadowManager(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RenderSubsystems_SetShadowManagerActive(int active);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_RENDERER_UTILITY_H
