#include "NIF.Native.Renderer.Utility.h"
#include "NIF.Native.Internal.h"

#include <NiAlphaAccumulator.h>
#include <NiCamera.h>
#include <NiParticleSDM.h>
#include <NiPortalSDM.h>
#include <NiRenderer.h>
#include <NiShadowManager.h>

namespace
{
	NiRenderer* NIF_GetRenderer(NIF_RendererHandle renderer)
	{
		NIF_RendererHandle_t* pHandle = static_cast<NIF_RendererHandle_t*>(renderer);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiAlphaAccumulator* NIF_GetAlphaAccumulator(NIF_AlphaAccumulatorHandle accumulator)
	{
		NIF_AlphaAccumulatorHandle_t* pHandle = static_cast<NIF_AlphaAccumulatorHandle_t*>(accumulator);
		return pHandle ? pHandle->spObject : nullptr;
	}
}

extern "C"
{

void NIF_Renderer_Destroy(NIF_RendererHandle renderer)
{
    delete static_cast<NIF_RendererHandle_t*>(renderer);
}

int NIF_Renderer_AsObject(NIF_RendererHandle renderer, NIF_ObjectHandle* outObject)
{
    if (outObject)
        *outObject = nullptr;

    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    if (!pkRenderer || !outObject)
        return 0;

    *outObject = NIF_CreateObjectHandle(pkRenderer);
    return *outObject ? 1 : 0;
}

const char* NIF_Renderer_GetDriverInfo(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? pkRenderer->GetDriverInfo() : nullptr;
}

unsigned int NIF_Renderer_GetRendererID(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? static_cast<unsigned int>(pkRenderer->GetRendererID()) : 0u;
}

int NIF_Renderer_BeginFrame(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return (pkRenderer && pkRenderer->BeginFrame()) ? 1 : 0;
}

int NIF_Renderer_EndFrame(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return (pkRenderer && pkRenderer->EndFrame()) ? 1 : 0;
}

int NIF_Renderer_DisplayFrame(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return (pkRenderer && pkRenderer->DisplayFrame()) ? 1 : 0;
}

unsigned int NIF_Renderer_GetFrameID(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? pkRenderer->GetFrameID() : 0u;
}

unsigned int NIF_Renderer_GetFrameState(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? static_cast<unsigned int>(pkRenderer->GetFrameState()) : 0u;
}

unsigned int NIF_Renderer_GetDefaultClearMode(void)
{
    return NiRenderer::CLEAR_ALL;
}

void NIF_Renderer_SetBackgroundColor(NIF_RendererHandle renderer, NIF_ColorA color)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    if (pkRenderer)
        pkRenderer->SetBackgroundColor(NIF_MakeColorA(color));
}

NIF_ColorA NIF_Renderer_GetBackgroundColor(NIF_RendererHandle renderer)
{
    NiColorA color(0.0f, 0.0f, 0.0f, 1.0f);
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    if (pkRenderer)
        pkRenderer->GetBackgroundColor(color);
    return NIF_MakeColorA(color);
}

void NIF_Renderer_SetDepthClear(NIF_RendererHandle renderer, float depthClear)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    if (pkRenderer)
        pkRenderer->SetDepthClear(depthClear);
}

float NIF_Renderer_GetDepthClear(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? pkRenderer->GetDepthClear() : 1.0f;
}

void NIF_Renderer_SetStencilClear(NIF_RendererHandle renderer,
    unsigned int stencilClear)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    if (pkRenderer)
        pkRenderer->SetStencilClear(stencilClear);
}

unsigned int NIF_Renderer_GetStencilClear(NIF_RendererHandle renderer)
{
    NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
    return pkRenderer ? pkRenderer->GetStencilClear() : 0u;
}

int NIF_Renderer_BeginDefaultScene(NIF_RendererHandle renderer, unsigned int clearFlags)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	return (pkRenderer && pkRenderer->BeginUsingDefaultRenderTargetGroup(clearFlags)) ? 1 : 0;
}

int NIF_Renderer_EndScene(NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer || !pkRenderer->IsRenderTargetGroupActive())
	{
		return 0;
	}

	pkRenderer->EndUsingRenderTargetGroup();
	return 1;
}

void NIF_Renderer_SetSorter(NIF_RendererHandle renderer, NIF_AlphaAccumulatorHandle accumulator)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	NiAlphaAccumulator* pkAccumulator = NIF_GetAlphaAccumulator(accumulator);
	if (pkRenderer)
	{
		pkRenderer->SetSorter(pkAccumulator);
	}
}

int NIF_Renderer_GetSorter(NIF_RendererHandle renderer, NIF_AlphaAccumulatorHandle* outAccumulator)
{
	if (outAccumulator)
	{
		*outAccumulator = nullptr;
	}

	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer || !outAccumulator)
	{
		return 0;
	}

	*outAccumulator = NIF_CreateAlphaAccumulatorHandle(NiDynamicCast(NiAlphaAccumulator, pkRenderer->GetSorter()));
	return *outAccumulator ? 1 : 0;
}

void NIF_Renderer_SetCameraData(NIF_RendererHandle renderer, NIF_CameraHandle camera)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (pkRenderer && pCameraHandle && pCameraHandle->spObject)
	{
		pkRenderer->SetCameraData(pCameraHandle->spObject);
	}
}

void NIF_Renderer_GetCameraData(NIF_RendererHandle renderer, NIF_Vec3* worldLocation, NIF_Vec3* worldDirection, NIF_Vec3* worldUp, NIF_Vec3* worldRight, NIF_Frustum* frustum, NIF_Rect* viewport)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer)
	{
		return;
	}

	NiPoint3 kWorldLocation;
	NiPoint3 kWorldDirection;
	NiPoint3 kWorldUp;
	NiPoint3 kWorldRight;
	NiFrustum kFrustum;
	NiRect<float> kViewport;
	pkRenderer->GetCameraData(kWorldLocation, kWorldDirection, kWorldUp, kWorldRight, kFrustum, kViewport);

	if (worldLocation)
	{
		*worldLocation = NIF_MakeVec3(kWorldLocation);
	}
	if (worldDirection)
	{
		*worldDirection = NIF_MakeVec3(kWorldDirection);
	}
	if (worldUp)
	{
		*worldUp = NIF_MakeVec3(kWorldUp);
	}
	if (worldRight)
	{
		*worldRight = NIF_MakeVec3(kWorldRight);
	}
	if (frustum)
	{
		*frustum = NIF_MakeFrustum(kFrustum);
	}
	if (viewport)
	{
		*viewport = NIF_MakeRect(kViewport);
	}
}

void NIF_RenderSubsystems_InitParticle(void)
{
	NiParticleSDM::Init();
}

void NIF_RenderSubsystems_ShutdownParticle(void)
{
	NiParticleSDM::Shutdown();
}

void NIF_RenderSubsystems_InitPortal(void)
{
	NiPortalSDM::Init();
}

void NIF_RenderSubsystems_ShutdownPortal(void)
{
	NiPortalSDM::Shutdown();
}

int NIF_RenderSubsystems_InitShadowManager(void)
{
	NiShadowManager::Initialize();
	return NiShadowManager::GetShadowManager() ? 1 : 0;
}

void NIF_RenderSubsystems_ShutdownShadowManager(void)
{
	NiShadowManager::Shutdown();
}

void NIF_RenderSubsystems_SetShadowManagerActive(int active)
{
	NiShadowManager::SetActive(active != 0);
}

}
