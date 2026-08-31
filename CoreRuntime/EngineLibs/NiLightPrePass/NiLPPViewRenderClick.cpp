// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

#include "NiLightPrePassPCH.h"


#include "NiLPPViewRenderClick.h"
#include "NiLPPDepthNormalMaterial.h"
#include "NiLPPFinalMaterial.h"
#include "NiLPPTerrainDepthNormalMaterial.h"
#include "NiLPPTerrainFinalMaterial.h"
#include "NiLPPDecorationDepthNormalMaterial.h"
#include "NiLPPDecorationFinalMaterial.h"
#include "NiLPPMaterialSwapProcessor.h"

#include <NiShaderFactory.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>
#include <NiTextureEffect.h>
#include <NiStencilProperty.h>
#include <NiLightManager.h>
#include <efd/Profiler.h>

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
#include <XenonNiRenderer.h>
#endif

//--------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT(LightPrePass);
NiImplementRTTI(NiLPPViewRenderClick, NiViewRenderClick, NiTypeMask::NiLPPViewRenderClick);
//-------------------------------------------------------------------------------------------------
NiLPPViewRenderClick::NiLPPViewRenderClick(bool bInitializeSwaps) :
    m_iDraws(0),
    m_fCullTime(0.0f),
    m_fRenderTime(0.0f),
    m_eDebug(DEBUG_NONE),
	m_eRenderMode(FORWARD_INTERLEAVED),
    m_fDepthRange(256.0f),
    m_fDepthBias(0.0f),
    m_bFirstRender(true)
{
    SetName("NiLPPViewRenderClick");

    InitializeShaderConstants();

    // TODO move this stuff to an init step so we can do something
    //      besides assert on failure, and also allow customization
    //      of the various pieces.

    m_spDepthNormalMaterial = NiLPPDepthNormalMaterial::Create();
    m_spFinalMaterial = NiLPPFinalMaterial::Create();
    EE_ASSERT(m_spDepthNormalMaterial);
    EE_ASSERT(m_spFinalMaterial);

	m_spSwapG = EE_NEW NiLPPMaterialSwapProcessorG();
	m_spSwapF = EE_NEW NiLPPMaterialSwapProcessorF();
	EE_ASSERT(m_spSwapG);
	EE_ASSERT(m_spSwapF);

	if (bInitializeSwaps)
	{
		// Configure the swap materials for the NiStandardMaterial
		NiMaterial* pkStandardNiMaterial = NiMaterial::GetMaterial("NiStandardMaterial");
		EE_ASSERT(pkStandardNiMaterial);
		pkStandardNiMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_G, m_spDepthNormalMaterial);
		pkStandardNiMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_F, m_spFinalMaterial);


		// Configure the swap materials for the NiTerrainMaterial
		NiTerrainMaterial* pkTerrainMaterial = NiTerrainMaterial::Create();
		if (pkTerrainMaterial)
		{
			pkTerrainMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_G, NiLPPTerrainDepthNormalMaterial::Create());
			pkTerrainMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_F, NiLPPTerrainFinalMaterial::Create());
		}

		// Configure the swap materials for the NiDecorationMaterial
		NiDecorationMaterial* pkDecorationMaterial = NiDecorationMaterial::Create();
		if (pkDecorationMaterial)
		{
			pkDecorationMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_G, NiLPPDecorationDepthNormalMaterial::Create());
			pkDecorationMaterial->SetSwapMaterial(NiMaterial::SWAP_LPP_F, NiLPPDecorationFinalMaterial::Create());
		}
	}
}

//-------------------------------------------------------------------------------------------------
NiLPPViewRenderClick::~NiLPPViewRenderClick()
{
    ShutdownShaderConstants();

    for (efd::vector<NiVisibleArray*>::iterator iter = m_kViewForwardArrays.begin();
        iter != m_kViewForwardArrays.end();
        ++iter)
    {
        NiVisibleArray* pViewForwardList = (*iter);
        NiDelete pViewForwardList;
    }
    m_kViewForwardArrays.clear();

	for (VisibleArrayVector::iterator iter = m_kViewLPPArrays.begin();
		iter != m_kViewLPPArrays.end();
		++iter)
	{
		NiVisibleArray* pViewLPPList = (*iter);
		NiDelete pViewLPPList;
	}
	m_kViewLPPArrays.clear();
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::InitializeShaderConstants()
{
    float tempArray[16] = {0};

    // Set the shader constant strings
    m_kSCDepthScale = "g_fDepthScale";

    // Initialize the shader constants
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCDepthScale, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4, 4 * sizeof(float), &tempArray[0]);
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::ShutdownShaderConstants()
{
    // Release the shader constants
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCDepthScale);

    // reset the fixed strings
    m_kSCDepthScale = 0;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::SetDebugMode(DebugMode mode)
{
    m_eDebug = mode;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::SetRenderMode(RenderMode mode)
{
	m_eRenderMode = mode;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::SetGBuffer(NiRenderTargetGroup* pkTarget, NiRenderedTexture* pkTexture)
{
    m_spTargetGBuffer = pkTarget;
    m_spTextureGBuffer = pkTexture;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::SetLightingBuffer(NiRenderTargetGroup* pkTarget, NiRenderedTexture* pkTexture)
{
    m_spTargetLighting = pkTarget;
    m_spTextureLighting = pkTexture;
}

//-------------------------------------------------------------------------------------------------
#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
void NiLPPViewRenderClick::SetGBufferDepth(ResolvableDepthStencilBuffer* pkBuffer)
{
    m_spGBufferDepth = pkBuffer;
}
#endif

//-------------------------------------------------------------------------------------------------
// Derived from NiViewRenderClick
//-------------------------------------------------------------------------------------------------
efd::UInt32 NiLPPViewRenderClick::GetNumObjectsDrawn() const
{
    return m_iDraws;
}

//-------------------------------------------------------------------------------------------------
efd::Float32 NiLPPViewRenderClick::GetCullTime() const
{
    return m_fCullTime;
}

//-------------------------------------------------------------------------------------------------
efd::Float32 NiLPPViewRenderClick::GetRenderTime() const
{
    return m_fRenderTime;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::PerformRendering(unsigned int uiFrameID)
{
    EE_PROFILER_ZONE(LightPrePass, 0, "LPP:PerformRendering");
#if defined(NI_RENDERER_BGFX)
    // The original LPP path relies on D3D-era generated G-buffer/final-light
    // materials and light-volume shaders. bgfx already implements the same
    // NiStandardMaterial/NiTerrainMaterial lighting inputs in the forward
    // renderer, so preserve visual correctness by rendering the original
    // materials directly instead of swapping to an unsupported deferred
    // material graph. This also keeps transparent/special materials in their
    // normal ordering and avoids allocating unused G/L buffers.
    NiViewRenderClick::PerformRendering(uiFrameID);
    m_iDraws = NiViewRenderClick::GetNumObjectsDrawn();
    m_fCullTime = NiViewRenderClick::GetCullTime();
    m_fRenderTime = NiViewRenderClick::GetRenderTime();
    return;
#endif
    if (m_bFirstRender)
    {
        CreateBuffers();
        m_bFirstRender = false;
    }

    m_pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(m_pkRenderer);

    EE_PUSH_GPU_MARKER((const char*)GetName());
    efd::TimeType fStartTime = efd::GetCurrentTimeInSec();

    // Separate the visible geometry into LPP and Forward-only lists
    GenerateLists(uiFrameID);
    // Generate the light list
    GenerateLights();
    // Record this as culling time
    m_fCullTime = (float)(efd::GetCurrentTimeInSec() - fStartTime);

    // Perform the four render passes
    RenderPasses();
    m_iDraws = GetLPPDraws() * 2; // two passes for LPP
    m_iDraws += GetForwardDraws(); // one pass for Forward-only
    m_iDraws += m_kLights.GetSize();

    if (m_eDebug == DEBUG_LIGHTS)
    {
        DebugLighting();
    }

    m_fRenderTime = (float)(efd::GetCurrentTimeInSec() - fStartTime);
    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
// Internal Rendering
//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::GenerateLists(unsigned int uiFrameID)
{
    EE_PROFILER_ZONE(LightPrePass, 0, "LPP::GenerateLists");

    // Separate objects into a set that can use LPP (e.g. opaque)
    // and ones that can't (e.g. transparent, special, etc.)
	//m_kListLPP.RemoveAll();
	//m_kListForward.RemoveAll();

	// ensure these arrays are synchronized
	EE_ASSERT(m_kViewForwardArrays.size() == m_kViewLPPArrays.size());

	// Remove the existing items
	for (efd::UInt32 index = 0; index  < m_kViewLPPArrays.size(); ++index)
	{
		m_kViewLPPArrays[index]->RemoveAll();
		m_kViewForwardArrays[index]->RemoveAll();
	}

	// Allocate space for the views
	while (m_kViewLPPArrays.size() < m_kRenderViews.GetSize())
	{
		m_kViewForwardArrays.push_back(NiNew NiVisibleArray());
		m_kViewLPPArrays.push_back(NiNew NiVisibleArray());
	}

	NiTListIterator kIter = m_kRenderViews.GetHeadPos();
	efd::UInt32 index = 0;	
	while (kIter)
	{
		NiVisibleArray* pViewForwardList = m_kViewForwardArrays[index];
		NiVisibleArray* pViewLPPList = m_kViewLPPArrays[index];
		++index;

		Ni3DRenderView* pkRenderView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetNext(kIter));
		if (!pkRenderView)
			continue;

		EE_ASSERT(pkRenderView);

		const NiVisibleArray& kListFull = pkRenderView->GetPVGeometry(uiFrameID);
		const efd::UInt32 count = kListFull.GetCount();
		for (efd::UInt32 ui = 0; ui < count; ++ui)
		{
			NiRenderObject& mesh = kListFull.GetAt(ui);
			NiAlphaProperty* pAlpha = mesh.GetPropertyState()->GetAlpha();
			NiZBufferProperty* pZBuffer = mesh.GetPropertyState()->GetZBuffer();
			const NiMaterial* pkNiMaterial = mesh.GetActiveMaterial();

			// TODO: Allow user to specify specific NiMaterials to not use LPP
			// ExtraData* pForward = mesh.GetExtraData(m_kForwardToken);
			
			if (!pkNiMaterial ||
				(!pkNiMaterial->GetSwapMaterial(NiMaterial::SWAP_LPP_G)) || // NiMaterial does not have an LPP swap
				(pAlpha != NULL && pAlpha->GetAlphaBlending()) || // no alpha blending in LPP pass
				(pZBuffer != NULL && !pZBuffer->GetZBufferWrite()) // only depth writing objects allowed
			)
			{
				// object cannot use LPP rendering
				//m_kListForward.Add(mesh);
				pViewForwardList->Add(mesh);
			}
			else
			{
				EE_ASSERT(pkNiMaterial->GetSwapMaterial(NiMaterial::SWAP_LPP_G) && pkNiMaterial->GetSwapMaterial(NiMaterial::SWAP_LPP_F));

				// object qualifies for LPP pass
				pViewLPPList->Add(mesh);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::GenerateLights()
{
    EE_PROFILER_ZONE(LightPrePass, 0, "LPP::GenerateLights");
    m_kLights.RemoveAll();
    if (NiLightManager::GetLightManager())
    {
        NiLightManager::GetLightManager()->GetAllLights(m_kLights);
    }
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::UseTarget(NiRenderTargetGroup* pkTarget, efd::UInt32 uiClearMode)
{
    if (m_pkRenderer->IsRenderTargetGroupActive())
    {
        m_pkRenderer->EndUsingRenderTargetGroup();
    }

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
    efd::UInt32 uiOriginalClearMode = uiClearMode;
    uiClearMode |= (NiRenderer::CLEAR_STENCIL | NiRenderer::CLEAR_DEPTH);
#endif

    EE_ASSERT(pkTarget);
    m_pkRenderer->BeginUsingRenderTargetGroup(pkTarget, uiClearMode);

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
    // tiling requires a restoration of the depth buffer
    if (0 == (uiOriginalClearMode & (NiRenderer::CLEAR_STENCIL | NiRenderer::CLEAR_DEPTH)))
    {
        XenonNiRenderer* pkXenon = (XenonNiRenderer*)m_pkRenderer;
        pkXenon->RestoreDepthBuffer(m_spGBufferDepth, true);
    }
#endif
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::RenderPasses()
{
    EE_PROFILER_ZONE(LightPrePass, 0, "LPP::RenderPasses");
    NiTextureEffect::SetLPPGBuffer(m_spTextureGBuffer);
    NiTextureEffect::SetLPPLBuffer(m_spTextureLighting);

    // TODO: Could this be replaced by information available in the click
    const Ni3DRenderView* pView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetHead());
    if (pView)
    {
        // Calculate the depth range
        NiCamera* pkCamera = pView->GetCamera();
        m_fDepthRange = pkCamera->GetViewFrustum().m_fFar;
    }

    // set depth scale for writing GBuffer
    NiPoint4 kDepthScale = NiPoint4(
        m_fDepthRange / 255.0f,
        m_fDepthBias,
        0.5f / efd::Float32(m_spTextureGBuffer->GetWidth()), // half pixel
        0.5f / efd::Float32(m_spTextureGBuffer->GetHeight())); // half pixel
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCDepthScale,  sizeof(kDepthScale),  &kDepthScale); 

    // 1. Render G-Buffer (screen space normal and depth information)
    {
        EE_PROFILER_ZONE(LightPrePass, 0, "LPP::RenderDepthNormal");
        UseTarget(
            (m_eDebug == DEBUG_GBUFFER) ? m_spRenderTargetGroup : m_spTargetGBuffer,
            NiRenderer::CLEAR_ALL);
        RenderGBuffer();
    }
    if (m_eDebug == DEBUG_GBUFFER) return;

    // 2. Render NiLight buffer using G-Buffer and special light meshes
    {
        EE_PROFILER_ZONE(LightPrePass, 0, "LPP::RenderLights");
        m_pkRenderer->SetBackgroundColor(efd::ColorA(0,0,0,0));
        UseTarget(
            (m_eDebug == DEBUG_LIGHTING) ? m_spRenderTargetGroup : m_spTargetLighting,
            NiRenderer::CLEAR_BACKBUFFER);
        RenderLighting();
    }
    if (m_eDebug == DEBUG_LIGHTING) return;

    // TODO we should actually render the sky click before LPP instead of
    //      having the m_bClearBackbuffer flag for a couple of reasons
    //      1. if tiling on the 360, we'd have to restore the color buffer otherwise
    //      2. potentially we could optimize the skybox render with a depth test,
    //         if we force the depth clear value in the shader, we can depth test
    //         to only draw where depth has not yet been written.

	// 3. Render Final and forward
	{
		unsigned int uiEffectiveClearMode = m_uiClearMode | m_uiSingleFrameClearMode;
		m_uiSingleFrameClearMode = 0;

        // Never clear the ZBuffer as it has been cleared in the GBuffer pass
        uiEffectiveClearMode &= ~NiRenderer::CLEAR_ZBUFFER;

		m_pkRenderer->SetBackgroundColor(m_kBackgroudColor);
		UseTarget(m_spRenderTargetGroup, uiEffectiveClearMode);

		m_kVisibleOutput.RemoveAll();

		bool bForwardLast = (m_eRenderMode  == FORWARD_LAST);
		{
			EE_PROFILER_ZONE(LightPrePass, 0, 
				bForwardLast ? "LPP::RenderFinalPass" : "LPP::RenderFinalAndForward");
			NiTListIterator kIter = m_kRenderViews.GetHeadPos();
			for (efd::UInt32 index = 0; kIter; ++index)
			{
				Ni3DRenderView* pView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetNext(kIter));
				if (!pView)
					continue;

				RenderLPP(pView, index);

				if (!bForwardLast && m_eDebug != DEBUG_NOFORWARD)
					RenderForward(pView, index);
			}
		}

		if (m_eDebug == DEBUG_NOFORWARD) 
			return;

		if (bForwardLast)
		{
			EE_PROFILER_ZONE(LightPrePass, 0, "LPP::RenderForwardPass");
			NiTListIterator kIter = m_kRenderViews.GetHeadPos();
			for (efd::UInt32 index = 0; kIter; ++index)
			{
				Ni3DRenderView* pView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetNext(kIter));
				if (!pView)
					continue;

				RenderForward(pView, index);
			}
		}
	}
	
    NiTextureEffect::SetLPPGBuffer(NULL);
    NiTextureEffect::SetLPPLBuffer(NULL);
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::RenderGBuffer()
{
    EE_PUSH_GPU_MARKER("RenderGBuffer");

	// TODO: Figure out why it's setting the camera data at this stage anyway
	NiTListIterator kIter = m_kRenderViews.GetHeadPos();
	while (kIter)
	{
		NiRenderView* pkRenderView = m_kRenderViews.GetNext(kIter);
		EE_ASSERT(pkRenderView);
		pkRenderView->SetCameraData(m_kViewport);
	}

    m_kVisibleOutput.RemoveAll();
	for (VisibleArrayVector::iterator iter = m_kViewLPPArrays.begin();
		iter != m_kViewLPPArrays.end();
		++iter)
	{
		m_spSwapG->PreRenderProcessList(*iter, m_kVisibleOutput, NULL);
	}
    EE_ASSERT(m_kVisibleOutput.GetCount() == 0);

    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::RenderLighting()
{
    EE_PUSH_GPU_MARKER("RenderLighting");

	// TODO: Could this be replaced by information available in the click
	const Ni3DRenderView* pView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetHead());
	if (!pView)
		return;

    NiCamera* pkCamera = pView->GetCamera();
    m_kLightRenderer.SetCamera(pkCamera);

    NiTListIterator kIter = m_kLights.GetHeadPos();
    while (kIter)
    {
        NiLight* pkLight = m_kLights.GetNext(kIter);
        m_kLightRenderer.RenderLight(pkLight, m_spTextureGBuffer, m_pkRenderer);
    }

    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::DebugLighting()
{
    EE_PUSH_GPU_MARKER("DebugLighting");

	// TODO: Could this be replaced by information available in the click
	Ni3DRenderView* pView = NiDynamicCast(Ni3DRenderView, m_kRenderViews.GetHead());
	if (!pView)
		return;

    NiCamera* pkCamera = pView->GetCamera();
    m_kLightRenderer.SetCamera(pkCamera);

    NiTListIterator kIter = m_kLights.GetHeadPos();
    while (kIter)
    {
        NiLight* pkLight = m_kLights.GetNext(kIter);
        m_kLightRenderer.DebugLight(pkLight, m_pkRenderer);
    }

    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::RenderLPP(Ni3DRenderView* pView, efd::UInt32 viewIndex)
{
    EE_PUSH_GPU_MARKER("RenderLPP");

	// TODO: Could this be replaced by information available in the click
	EE_ASSERT(pView);

    pView->SetCameraData(m_kViewport);
    
	m_spSwapF->PreRenderProcessList(m_kViewLPPArrays[viewIndex], m_kVisibleOutput, NULL);
    
    EE_ASSERT(m_kVisibleOutput.GetCount() == 0);

    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::RenderForward(Ni3DRenderView* pView, efd::UInt32 viewIndex)
{
    EE_PUSH_GPU_MARKER("RenderForward");

	EE_ASSERT(pView);

	pView->SetCameraData(m_kViewport);
	NiCamera* pkCamera = pView->GetCamera();

	m_kAccumulator.StartAccumulating(pkCamera);
	m_kAccumulator.RegisterObjectArray(*(m_kViewForwardArrays[viewIndex]));
	m_kAccumulator.FinishAccumulating();

    EE_POP_GPU_MARKER();
}

//-------------------------------------------------------------------------------------------------
efd::UInt32 NiLPPViewRenderClick::GetLPPDraws() const
{
	efd::UInt32 count = 0;
	for (VisibleArrayVector::const_iterator iter = m_kViewLPPArrays.begin();
		iter != m_kViewLPPArrays.end();
		++iter)
	{
		count += (*iter)->GetCount();
	}
	return count;
}

//-------------------------------------------------------------------------------------------------
efd::UInt32 NiLPPViewRenderClick::GetForwardDraws() const
{
	efd::UInt32 count = 0;
	for (VisibleArrayVector::const_iterator iter = m_kViewForwardArrays.begin();
		iter != m_kViewForwardArrays.end();
		++iter)
	{
		count += (*iter)->GetCount();
	}
	return count;
}

//-------------------------------------------------------------------------------------------------
void NiLPPViewRenderClick::CreateBuffers()
{
    // Make sure the buffers don't exist:
    if (m_spTextureGBuffer || m_spTextureLighting || m_spTargetGBuffer || m_spTargetLighting)
        return;

    // create fullscreen render targets
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);
    Ni2DBuffer* pkBackBuffer = pkRenderer->GetDefaultBackBuffer();
    NiDepthStencilBuffer* pkDepthStencilBuffer = pkRenderer->GetDefaultDepthStencilBuffer();
    if (m_spRenderTargetGroup)
    {
        pkBackBuffer = m_spRenderTargetGroup->GetBuffer(0);
        pkDepthStencilBuffer = m_spRenderTargetGroup->GetDepthStencilBuffer();
    }
    const efd::UInt32 iW = pkBackBuffer->GetWidth();
    const efd::UInt32 iH = pkBackBuffer->GetHeight();
    Ni2DBuffer::MultiSamplePreference eMSAAPrefs = pkBackBuffer->GetMSAAPref();

    NiTexture::FormatPrefs kPrefsG;
    kPrefsG.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    kPrefsG.m_eMipMapped = NiTexture::FormatPrefs::NO;
    kPrefsG.m_ePixelLayout = NiTexture::FormatPrefs::FLOAT_COLOR_64;

    NiTexture::FormatPrefs kPrefsL;
    kPrefsL.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    kPrefsL.m_eMipMapped = NiTexture::FormatPrefs::NO;

#if !defined(EE_PLATFORM_XBOX360)
    kPrefsL.m_ePixelLayout = NiTexture::FormatPrefs::FLOAT_COLOR_64;
#else
    // TODO XBox360 can't do alpha add properly on floating points.
    //      Could use a TRUE_COLOR_64 but this isn't exposed via Gamebryo
    kPrefsL.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
#endif

    NiRenderedTexture* pkTextureGBuffer  = 
        NiRenderedTexture::Create(iW, iH, pkRenderer, kPrefsG, eMSAAPrefs);
    NiRenderedTexture* pkTextureLighting = 
        NiRenderedTexture::Create(iW, iH, pkRenderer, kPrefsL, eMSAAPrefs);
    EE_ASSERT(pkTextureGBuffer);
    EE_ASSERT(pkTextureLighting);

#if !defined(EE_PLATFORM_XBOX360) ||  !defined(XBOX360_LPP_TILING)
    NiDepthStencilBuffer* pkDepth = pkDepthStencilBuffer;
#else
    NiDepthStencilBuffer* pkDepth = 
        NiDepthStencilBuffer::CreateCompatible(pkTextureGBuffer->GetBuffer(), pkRenderer);
#endif
    EE_ASSERT(pkDepth);

    NiRenderTargetGroup* pkTargetGBuffer  = 
        NiRenderTargetGroup::Create(pkTextureGBuffer->GetBuffer(),  pkRenderer, pkDepth);
    NiRenderTargetGroup* pkTargetLighting = 
        NiRenderTargetGroup::Create(pkTextureLighting->GetBuffer(), pkRenderer, pkDepth);
    EE_ASSERT(pkTargetGBuffer);
    EE_ASSERT(pkTargetLighting);

    // assign to light prepass step
    SetGBuffer(pkTargetGBuffer, pkTextureGBuffer);
    SetLightingBuffer(pkTargetLighting, pkTextureLighting);

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
    SetGBufferDepth(pkDepth);
#endif
}

//------------------------------------------------------------------------------------------------
