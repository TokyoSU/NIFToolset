#include "NIF.Native.Renderer.Utility.h"
#include "NIF.Native.Internal.h"

#include <NiAlphaAccumulator.h>
#include <NiCamera.h>
#include <NiParticleSDM.h>
#include <NiPortalSDM.h>
#include <NiRenderer.h>
#include <NiShadowManager.h>
#include <Windows.h>
#include <ecrD3D11Renderer/D3D11Renderer.h>

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

void NIF_DX11Renderer_SetDefaultViewport(NIF_RendererHandle renderer, unsigned int width, unsigned int height)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer)
	{
		return;
	}

	ecr::D3D11Renderer* pkD3D11Renderer = NiDynamicCast(ecr::D3D11Renderer, pkRenderer);
	if (!pkD3D11Renderer)
	{
		return;
	}

	UINT uiViewportCount = 0;
	ID3D11DeviceContext* pDeviceContext = pkD3D11Renderer->GetImmediateD3D11DeviceContext();
	if (!pDeviceContext)
	{
		return;
	}

	pDeviceContext->RSGetViewports(&uiViewportCount, nullptr);
	if (uiViewportCount == 0)
	{
		D3D11_VIEWPORT kViewport = {};
		kViewport.TopLeftX = 0.0f;
		kViewport.TopLeftY = 0.0f;
		kViewport.Width = static_cast<float>(width);
		kViewport.Height = static_cast<float>(height);
		kViewport.MinDepth = 0.0f;
		kViewport.MaxDepth = 1.0f;
		pDeviceContext->RSSetViewports(1, &kViewport);
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
