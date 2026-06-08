#include "NIF.Native.RenderTarget.h"
#include "NIF.Native.Internal.h"

namespace
{
	NiRenderer* NIF_GetRenderer(NIF_RendererHandle renderer)
	{
		NIF_RendererHandle_t* pHandle = static_cast<NIF_RendererHandle_t*>(renderer);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRenderTargetGroup* NIF_GetRenderTargetGroup(NIF_RenderTargetGroupHandle renderTargetGroup)
	{
		NIF_RenderTargetGroupHandle_t* pHandle = static_cast<NIF_RenderTargetGroupHandle_t*>(renderTargetGroup);
		return pHandle ? pHandle->spObject : nullptr;
	}

	Ni2DBuffer* NIF_GetRenderBuffer(NIF_RenderBufferHandle buffer)
	{
		NIF_RenderBufferHandle_t* pHandle = static_cast<NIF_RenderBufferHandle_t*>(buffer);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiDepthStencilBuffer* NIF_GetDepthStencilBuffer(NIF_DepthStencilBufferHandle depthStencilBuffer)
	{
		NIF_DepthStencilBufferHandle_t* pHandle = static_cast<NIF_DepthStencilBufferHandle_t*>(depthStencilBuffer);
		return pHandle ? pHandle->spObject : nullptr;
	}
}

extern "C"
{

void NIF_RenderTargetGroup_Destroy(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	delete static_cast<NIF_RenderTargetGroupHandle_t*>(renderTargetGroup);
}

NIF_RenderTargetGroupHandle NIF_RenderTargetGroup_Create(unsigned int bufferCount, NIF_RendererHandle renderer)
{
	NiRenderer* pkRenderer = NIF_GetRenderer(renderer);
	return NIF_CreateRenderTargetGroupHandle(pkRenderer ? NiRenderTargetGroup::Create(bufferCount, pkRenderer) : nullptr);
}

unsigned int NIF_RenderTargetGroup_GetBufferCount(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return pkRenderTargetGroup ? pkRenderTargetGroup->GetBufferCount() : 0;
}

unsigned int NIF_RenderTargetGroup_GetWidth(NIF_RenderTargetGroupHandle renderTargetGroup, unsigned int index)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return pkRenderTargetGroup ? pkRenderTargetGroup->GetWidth(index) : 0;
}

unsigned int NIF_RenderTargetGroup_GetHeight(NIF_RenderTargetGroupHandle renderTargetGroup, unsigned int index)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return pkRenderTargetGroup ? pkRenderTargetGroup->GetHeight(index) : 0;
}

unsigned int NIF_RenderTargetGroup_GetDepthStencilWidth(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return pkRenderTargetGroup ? pkRenderTargetGroup->GetDepthStencilWidth() : 0;
}

unsigned int NIF_RenderTargetGroup_GetDepthStencilHeight(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return pkRenderTargetGroup ? pkRenderTargetGroup->GetDepthStencilHeight() : 0;
}

int NIF_RenderTargetGroup_HasDepthStencil(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return (pkRenderTargetGroup && pkRenderTargetGroup->HasDepthStencil()) ? 1 : 0;
}

int NIF_RenderTargetGroup_IsValid(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return (pkRenderTargetGroup && pkRenderTargetGroup->IsValid()) ? 1 : 0;
}

int NIF_RenderTargetGroup_CheckMSAAPrefConsistency(NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return (pkRenderTargetGroup && pkRenderTargetGroup->CheckMSAAPrefConsistency()) ? 1 : 0;
}

int NIF_RenderTargetGroup_AttachBuffer(NIF_RenderTargetGroupHandle renderTargetGroup, NIF_RenderBufferHandle buffer, unsigned int index)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	Ni2DBuffer* pkBuffer = NIF_GetRenderBuffer(buffer);
	return (pkRenderTargetGroup && pkBuffer && pkRenderTargetGroup->AttachBuffer(pkBuffer, index)) ? 1 : 0;
}

int NIF_RenderTargetGroup_AttachDepthStencilBuffer(NIF_RenderTargetGroupHandle renderTargetGroup, NIF_DepthStencilBufferHandle depthStencilBuffer)
{
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	NiDepthStencilBuffer* pkDepthStencilBuffer = NIF_GetDepthStencilBuffer(depthStencilBuffer);
	return (pkRenderTargetGroup && pkDepthStencilBuffer && pkRenderTargetGroup->AttachDepthStencilBuffer(pkDepthStencilBuffer)) ? 1 : 0;
}

int NIF_RenderTargetGroup_GetBuffer(NIF_RenderTargetGroupHandle renderTargetGroup, unsigned int index, NIF_RenderBufferHandle* outBuffer)
{
	if (outBuffer)
	{
		*outBuffer = nullptr;
	}

	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	if (!pkRenderTargetGroup || !outBuffer)
	{
		return 0;
	}

	*outBuffer = NIF_CreateRenderBufferHandle(pkRenderTargetGroup->GetBuffer(index));
	return *outBuffer ? 1 : 0;
}

int NIF_RenderTargetGroup_GetDepthStencilBuffer(NIF_RenderTargetGroupHandle renderTargetGroup, NIF_DepthStencilBufferHandle* outDepthStencilBuffer)
{
	if (outDepthStencilBuffer)
	{
		*outDepthStencilBuffer = nullptr;
	}

	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	if (!pkRenderTargetGroup || !outDepthStencilBuffer)
	{
		return 0;
	}

	*outDepthStencilBuffer = NIF_CreateDepthStencilBufferHandle(pkRenderTargetGroup->GetDepthStencilBuffer());
	return *outDepthStencilBuffer ? 1 : 0;
}

}
