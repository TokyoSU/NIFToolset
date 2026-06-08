#pragma once
#ifndef NIF_NATIVE_RENDERER_DX11_H
#define NIF_NATIVE_RENDERER_DX11_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct NIF_DX11RendererDesc
{
	unsigned int adapterIndex;
	unsigned int outputIndex;
	int driverType;
	unsigned int createFlags;
	int createSwapChain;
	int createDepthStencilBuffer;
	int associateWithWindow;
	unsigned int windowAssociationFlags;
	unsigned int depthStencilFormat;
	unsigned int backBufferWidth;
	unsigned int backBufferHeight;
	unsigned int backBufferFormat;
	unsigned int refreshRateNumerator;
	unsigned int refreshRateDenominator;
	unsigned int sampleCount;
	unsigned int sampleQuality;
	unsigned int bufferUsage;
	unsigned int bufferCount;
	void* outputWindow;
	int windowed;
	unsigned int swapEffect;
	unsigned int swapChainFlags;
} NIF_DX11RendererDesc;

NIFTOOLSET_NATIVE_ENTRY void NIF_DX11Renderer_FillDefaultDesc(NIF_DX11RendererDesc* desc);
NIFTOOLSET_NATIVE_ENTRY void NIF_DX11Renderer_FillWindowedDesc(NIF_DX11RendererDesc* desc, void* hwnd);
NIFTOOLSET_NATIVE_ENTRY NIF_RendererHandle NIF_DX11Renderer_Create(const NIF_DX11RendererDesc* desc);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_Destroy(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_AsObject(NIF_RendererHandle renderer, NIF_ObjectHandle* outObject);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Renderer_GetDriverInfo(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetRendererID(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_BeginFrame(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_EndFrame(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY int NIF_Renderer_DisplayFrame(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetFrameID(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetFrameState(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetDefaultClearMode(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetBackgroundColor(NIF_RendererHandle renderer, NIF_ColorA color);
NIFTOOLSET_NATIVE_ENTRY NIF_ColorA NIF_Renderer_GetBackgroundColor(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetDepthClear(NIF_RendererHandle renderer, float depthClear);
NIFTOOLSET_NATIVE_ENTRY float NIF_Renderer_GetDepthClear(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetStencilClear(NIF_RendererHandle renderer, unsigned int stencilClear);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetStencilClear(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Renderer_GetSyncInterval(NIF_RendererHandle renderer);
NIFTOOLSET_NATIVE_ENTRY void NIF_Renderer_SetSyncInterval(NIF_RendererHandle renderer, unsigned int syncInterval);
NIFTOOLSET_NATIVE_ENTRY int NIF_DX11Renderer_ResizeBuffers(NIF_RendererHandle renderer, unsigned int width, unsigned int height, void* hwnd);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_RENDERER_DX11_H
