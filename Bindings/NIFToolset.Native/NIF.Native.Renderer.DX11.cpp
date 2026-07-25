#include "NIF.Native.Renderer.DX11.h"
#include "NIF.Native.Internal.h"

#include <cstddef>

static_assert(offsetof(NIF_DX11RendererDesc, outputWindow) == 72u, "Unexpected DX11 renderer descriptor layout");
static_assert(sizeof(NIF_DX11RendererDesc) == (sizeof(void*) == 8 ? 96u : 88u), "Unexpected DX11 renderer descriptor size");

#include <ecrD3D11Renderer/D3D11Renderer.h>

namespace
{
	NiRenderer* NIF_GetRenderer(NIF_RendererHandle renderer)
	{
		NIF_RendererHandle_t* pHandle = static_cast<NIF_RendererHandle_t*>(renderer);
		return pHandle ? pHandle->spObject : nullptr;
	}

	ecr::D3D11Renderer* NIF_GetD3D11Renderer(NIF_RendererHandle renderer)
	{
		return NiDynamicCast(ecr::D3D11Renderer, NIF_GetRenderer(renderer));
	}

	NIF_ColorA NIF_MakeZeroColorA()
	{
		return NIF_ColorA{ 0.0f, 0.0f, 0.0f, 0.0f };
	}

	void NIF_FillCreateParams(const NIF_DX11RendererDesc& desc, ecr::D3D11Renderer::CreationParameters& createParams)
	{
		createParams.m_adapterIndex = desc.adapterIndex;
		createParams.m_outputIndex = desc.outputIndex;
		createParams.m_driverType = static_cast<ecr::D3D11Renderer::DriverType>(desc.driverType);
		createParams.m_createFlags = desc.createFlags;
		createParams.m_createSwapChain = desc.createSwapChain != 0;
		createParams.m_createDepthStencilBuffer = desc.createDepthStencilBuffer != 0;
		createParams.m_associateWithWindow = desc.associateWithWindow != 0;
		createParams.m_windowAssociationFlags = desc.windowAssociationFlags;
		createParams.m_depthStencilFormat = static_cast<DXGI_FORMAT>(desc.depthStencilFormat);
		createParams.m_swapChain.BufferDesc.Width = desc.backBufferWidth;
		createParams.m_swapChain.BufferDesc.Height = desc.backBufferHeight;
		createParams.m_swapChain.BufferDesc.Format = static_cast<DXGI_FORMAT>(desc.backBufferFormat);
		createParams.m_swapChain.BufferDesc.RefreshRate.Numerator = desc.refreshRateNumerator;
		createParams.m_swapChain.BufferDesc.RefreshRate.Denominator = desc.refreshRateDenominator;
		createParams.m_swapChain.SampleDesc.Count = desc.sampleCount;
		createParams.m_swapChain.SampleDesc.Quality = desc.sampleQuality;
		createParams.m_swapChain.BufferUsage = desc.bufferUsage;
		createParams.m_swapChain.BufferCount = desc.bufferCount;
		createParams.m_swapChain.OutputWindow = static_cast<HWND>(desc.outputWindow);
		createParams.m_swapChain.Windowed = desc.windowed != 0;
		createParams.m_swapChain.SwapEffect = static_cast<DXGI_SWAP_EFFECT>(desc.swapEffect);
		createParams.m_swapChain.Flags = desc.swapChainFlags;
	}

	NIF_DX11RendererDesc NIF_MakeDesc(const ecr::D3D11Renderer::CreationParameters& createParams)
	{
		NIF_DX11RendererDesc desc = {};
		desc.adapterIndex = createParams.m_adapterIndex;
		desc.outputIndex = createParams.m_outputIndex;
		desc.driverType = static_cast<int>(createParams.m_driverType);
		desc.createFlags = createParams.m_createFlags;
		desc.createSwapChain = createParams.m_createSwapChain ? 1 : 0;
		desc.createDepthStencilBuffer = createParams.m_createDepthStencilBuffer ? 1 : 0;
		desc.associateWithWindow = createParams.m_associateWithWindow ? 1 : 0;
		desc.windowAssociationFlags = createParams.m_windowAssociationFlags;
		desc.depthStencilFormat = static_cast<unsigned int>(createParams.m_depthStencilFormat);
		desc.backBufferWidth = createParams.m_swapChain.BufferDesc.Width;
		desc.backBufferHeight = createParams.m_swapChain.BufferDesc.Height;
		desc.backBufferFormat = static_cast<unsigned int>(createParams.m_swapChain.BufferDesc.Format);
		desc.refreshRateNumerator = createParams.m_swapChain.BufferDesc.RefreshRate.Numerator;
		desc.refreshRateDenominator = createParams.m_swapChain.BufferDesc.RefreshRate.Denominator;
		desc.sampleCount = createParams.m_swapChain.SampleDesc.Count;
		desc.sampleQuality = createParams.m_swapChain.SampleDesc.Quality;
		desc.bufferUsage = createParams.m_swapChain.BufferUsage;
		desc.bufferCount = createParams.m_swapChain.BufferCount;
		desc.outputWindow = createParams.m_swapChain.OutputWindow;
		desc.windowed = createParams.m_swapChain.Windowed ? 1 : 0;
		desc.swapEffect = static_cast<unsigned int>(createParams.m_swapChain.SwapEffect);
		desc.swapChainFlags = createParams.m_swapChain.Flags;
		return desc;
	}
}

extern "C"
{

void NIF_DX11Renderer_FillDefaultDesc(NIF_DX11RendererDesc* desc)
{
	if (!desc)
	{
		return;
	}

	const ecr::D3D11Renderer::CreationParameters createParams;
	*desc = NIF_MakeDesc(createParams);
}

void NIF_DX11Renderer_FillWindowedDesc(NIF_DX11RendererDesc* desc, void* hwnd)
{
	if (!desc)
	{
		return;
	}

	ecr::D3D11Renderer::CreationParameters createParams(static_cast<HWND>(hwnd));
	*desc = NIF_MakeDesc(createParams);
}

NIF_RendererHandle NIF_DX11Renderer_Create(const NIF_DX11RendererDesc* desc)
{
	if (!desc)
	{
		return nullptr;
	}

	ecr::D3D11Renderer::CreationParameters createParams;
	NIF_FillCreateParams(*desc, createParams);
	ecr::D3D11RendererPtr spRenderer;
	return ecr::D3D11Renderer::Create(createParams, spRenderer) ? NIF_CreateRendererHandle(spRenderer) : nullptr;
}

void NIF_Renderer_Destroy(NIF_RendererHandle renderer)
{
	delete static_cast<NIF_RendererHandle_t*>(renderer);
}

int NIF_Renderer_AsObject(NIF_RendererHandle renderer, NIF_ObjectHandle* outObject)
{
	if (outObject)
	{
		*outObject = nullptr;
	}

	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer || !outObject)
	{
		return 0;
	}

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
	return pkRenderer ? static_cast<unsigned int>(pkRenderer->GetRendererID()) : 0;
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
	return pkRenderer ? pkRenderer->GetFrameID() : 0;
}

unsigned int NIF_Renderer_GetFrameState(NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	return pkRenderer ? static_cast<unsigned int>(pkRenderer->GetFrameState()) : 0;
}

unsigned int NIF_Renderer_GetDefaultClearMode(void)
{
	return static_cast<unsigned int>(NiRenderer::CLEAR_ALL);
}

void NIF_Renderer_SetBackgroundColor(NIF_RendererHandle renderer, NIF_ColorA color)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (pkRenderer)
	{
		pkRenderer->SetBackgroundColor(NIF_MakeColorA(color));
	}
}

NIF_ColorA NIF_Renderer_GetBackgroundColor(NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (!pkRenderer)
	{
		return NIF_MakeZeroColorA();
	}

	NiColorA color;
	pkRenderer->GetBackgroundColor(color);
	return NIF_MakeColorA(color);
}

void NIF_Renderer_SetDepthClear(NIF_RendererHandle renderer, float depthClear)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (pkRenderer)
	{
		pkRenderer->SetDepthClear(depthClear);
	}
}

float NIF_Renderer_GetDepthClear(NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	return pkRenderer ? pkRenderer->GetDepthClear() : 0.0f;
}

void NIF_Renderer_SetStencilClear(NIF_RendererHandle renderer, unsigned int stencilClear)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	if (pkRenderer)
	{
		pkRenderer->SetStencilClear(stencilClear);
	}
}

unsigned int NIF_Renderer_GetStencilClear(NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	return pkRenderer ? pkRenderer->GetStencilClear() : 0;
}

unsigned int NIF_Renderer_GetSyncInterval(NIF_RendererHandle renderer)
{
	ecr::D3D11Renderer* pkRenderer = NIF_GetD3D11Renderer(renderer);
	return pkRenderer ? pkRenderer->GetSyncInterval() : 0;
}

void NIF_Renderer_SetSyncInterval(NIF_RendererHandle renderer, unsigned int syncInterval)
{
	ecr::D3D11Renderer* pkRenderer = NIF_GetD3D11Renderer(renderer);
	if (pkRenderer)
	{
		pkRenderer->SetSyncInterval(syncInterval);
	}
}

int NIF_DX11Renderer_ResizeBuffers(NIF_RendererHandle renderer, unsigned int width, unsigned int height, void* hwnd)
{
	ecr::D3D11Renderer* pkRenderer = NIF_GetD3D11Renderer(renderer);
	return (pkRenderer && pkRenderer->ResizeBuffers(width, height, static_cast<HWND>(hwnd))) ? 1 : 0;
}

}
