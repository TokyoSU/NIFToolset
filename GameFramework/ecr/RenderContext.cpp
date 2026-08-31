// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "ecrPCH.h"

#include "RenderContext.h"
#include "RenderService.h"
#include <NiMeshCullingProcess.h>
#include <NiShaderSortProcessor.h>
#include <NiShadowManager.h>
#include <efd/ServiceManager.h>

using namespace efd;
using namespace egf;
using namespace ecr;

//--------------------------------------------------------------------------------------------------
RenderContext::RenderContext(RenderService* pRenderService, Ni3DRenderView* pRenderView) :
    m_pRenderService(pRenderService),
    m_visibleArray(1024, 1024),
    m_backgroundColor(0.5f, 0.5f, 0.5f, 1.0f),
    m_invalid(true),
    m_validate(false),
	m_kRenderQueueInUseExtraData("g_RenderQueueInUsePointer")
{
    NiSPWorkflowManager* pWorkflowManager = NULL;
    SceneGraphService* pSceneGraphService =
        m_pRenderService->GetServiceManager()->GetSystemServiceAs<ecr::SceneGraphService>();
    if (pSceneGraphService)
        pWorkflowManager = pSceneGraphService->GetWorkflowManager();

    m_spCuller = EE_NEW NiMeshCullingProcess(&m_visibleArray, pWorkflowManager);

	// Setup main render view
	if (!pRenderView)
	{
		pRenderView = EE_NEW Ni3DRenderView();
	}

	pRenderView->SetCullingProcess(m_spCuller);

	AddRenderQueue(pRenderService->GetDefaultRenderQueue(), pRenderView);
}
//--------------------------------------------------------------------------------------------------
RenderContext::~RenderContext()
{
    efd::UInt32 count = static_cast<efd::UInt32>(m_renderSurfaces.size());
    for (efd::UInt32 ui = 0; ui < count; ++ui)
    {
        m_renderSurfaces[ui] = 0;
    }

    m_spRenderListProcessor = 0;
    m_spCuller = 0;
}
//--------------------------------------------------------------------------------------------------
void RenderContext::Draw(efd::Float32 currentTime)
{
    if (m_validate && !m_invalid)
        return;

    // Get renderer pointer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Manually open the rendering frame. Since each render surface has a unique
    // NiRenderFrame, each render frame has the usage of BeginFrame(), EndFrame(),
    // and DisplayFrame() disabled. This prevents us from calling
    // DisplayFrame() multiple times for one frame in the case of multiple
    // RenderSurfaces (split screen). Because of this we have to manually
    // call BeginFrame(), EndFrame(), or DisplayFrame()
    // If the render frame can't begin because of, say, a lost device, don't
    // attempt to render.
    if (!pkRenderer->BeginFrame())
        return;

    m_invalid = false;

    efd::UInt32 count = static_cast<efd::UInt32>(m_renderSurfaces.size());

    for (efd::UInt32 ui = 0; ui < count; ++ui)
    {
        RenderSurface* pSurface = m_renderSurfaces[ui];

        if (pSurface->m_spCamera == NULL)
            continue;

        pSurface->m_spCamera->Update(currentTime);

        EE_ASSERT(pSurface->m_spRenderFrame);

        NiRenderStep* pRenderStep = pSurface->GetRenderStep();
        EE_ASSERT(pRenderStep && NiIsKindOf(NiDefaultClickRenderStep, pRenderStep));
        NiDefaultClickRenderStep* pClickRenderStep = (NiDefaultClickRenderStep*)pRenderStep;
        EE_ASSERT(pClickRenderStep->GetOutputRenderTargetGroup());

        // Setup all the clicks to draw with appropriate viewport and camera information.
        const NiTPointerList<NiRenderClickPtr>& kRenderClicks =
            pClickRenderStep->GetRenderClickList();

        NiCamera* pCamera = pSurface->m_spCamera;
        const NiRect<float>& kViewport = pCamera->GetViewPort();

        // Inform the shadowing system of the active camera for Render Surface we are
        // about to draw.
        NiShadowManager::SetSceneCamera(pCamera);

        NiTListIterator kClickIter = kRenderClicks.GetHeadPos();
        while (kClickIter)
        {
            const NiRenderClickPtr& kClick = kRenderClicks.GetNext(kClickIter);
            kClick->SetViewport(kViewport);

            NiViewRenderClickPtr kViewClick = NiDynamicCast(NiViewRenderClick, kClick);
            if (kViewClick != NULL)
            {
                const NiTPointerList<NiRenderViewPtr>& kRenderViews =
                    kViewClick->GetRenderViews();

                NiTListIterator kViewIter = kRenderViews.GetHeadPos();
                while (kViewIter)
                {
                    Ni3DRenderViewPtr kView =
                        NiDynamicCast(Ni3DRenderView, kRenderViews.GetNext(kViewIter));

                    if (kView)
                        kView->SetCamera(pCamera);
                }
            }
        }

        RaisePreDrawEvent(pSurface);
        pSurface->Draw(currentTime);
        RaisePostDrawEvent(pSurface);
    }

    // Manually close the rendering frame.
    pkRenderer->EndFrame();

    // Manually display the frame.
    pkRenderer->DisplayFrame();
}
//--------------------------------------------------------------------------------------------------
void RenderContext::AddRenderSurface(RenderSurfacePtr spSurface, 
    NiRenderListProcessor* pCustomRenderListProcessor /* = NULL */)
{
    // Set up the scene render click for rendering the scene graphs in the render view.
    NiViewRenderClick* pRenderClick = spSurface->GetSceneRenderClick();

	pRenderClick->RemoveAllRenderViews();
	AddRenderQueues(pRenderClick);
    pRenderClick->SetUseRendererBackgroundColor(false);
    pRenderClick->SetPersistBackgroundColorToRenderer(true);
    pRenderClick->SetBackgroundColor(m_backgroundColor);
    pRenderClick->SetClearAllBuffers(true);

    if (pCustomRenderListProcessor != NULL)
    {
        m_spRenderListProcessor = pCustomRenderListProcessor;
    }
    else
    {
        // Has list processor been previous initialized?
        if (m_spRenderListProcessor == NULL)
        {
            // BgfxRenderer now exposes lightweight NiShader cache objects for
            // NiFragmentMaterial, so use the normal Gamebryo shader sorter.
            // This preserves material/effect bucketing for opaque geometry while
            // NiShaderSortProcessor still depth-sorts transparent objects.
            m_spRenderListProcessor = EE_NEW NiShaderSortProcessor();
        }
    }
    pRenderClick->SetProcessor(m_spRenderListProcessor);

    SetSurfaceContext(spSurface, this);

    m_renderSurfaces.push_back(spSurface);

    RaiseSurfaceAdded(spSurface);
}

//--------------------------------------------------------------------------------------------------
void RenderContext::SetSurfaceContext(RenderSurfacePtr spSurface, RenderContext* pContext)
{
    spSurface->m_pRenderContext = pContext;
}

//--------------------------------------------------------------------------------------------------
void RenderContext::RemoveRenderSurface(efd::WindowRef window)
{
    RemoveRenderSurface(GetRenderSurface(window));
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RemoveRenderSurface(RenderSurface* pSurface)
{
    RenderSurfacePtr spSurface = pSurface;

    m_renderSurfaces.erase(m_renderSurfaces.find(spSurface));

    spSurface->m_pRenderContext = NULL;

    RaiseSurfaceRemoved(spSurface);
}
//--------------------------------------------------------------------------------------------------
RenderSurface* RenderContext::GetRenderSurface(efd::WindowRef window) const
{
    efd::UInt32 count = static_cast<efd::UInt32>(m_renderSurfaces.size());
    for (efd::UInt32 i = 0; i < count; i++)
    {
        if (m_renderSurfaces[i]->GetWindowRef() == window)
            return m_renderSurfaces[i];
    }

    return NULL;
}
//--------------------------------------------------------------------------------------------------
RenderSurface* RenderContext::GetRenderSurfaceAt(efd::UInt32 index) const
{
    EE_ASSERT(index < static_cast<efd::UInt32>(m_renderSurfaces.size()));

    return m_renderSurfaces[index];
}
//--------------------------------------------------------------------------------------------------
void RenderContext::SetBackgroundColor(const NiColorA& kColor)
{
    m_backgroundColor = kColor;

    efd::UInt32 count = static_cast<efd::UInt32>(m_renderSurfaces.size());
    for (efd::UInt32 ui = 0; ui < count; ++ui)
    {
        NiViewRenderClick* pRenderClick = m_renderSurfaces[ui]->GetSceneRenderClick();
        pRenderClick->SetBackgroundColor(m_backgroundColor);
    }
}
//--------------------------------------------------------------------------------------------------
void RenderContext::AddRenderedEntity(egf::Entity* pEntity, NiAVObject* pAVObject)
{
	efd::UInt32 queueID = m_pRenderService->GetRenderQueueID(pEntity, pAVObject);
	AddSceneToQueue(queueID, pAVObject);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RemoveRenderedEntity(egf::Entity* pEntity, NiAVObject* pAVObject)
{
	EE_UNUSED_ARG(pEntity);
	RemoveSceneFromQueue(pAVObject);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::AddRenderedObject(SceneGraphService::SceneGraphHandle handle,
    NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(handle);
    efd::UInt32 queueID = m_pRenderService->GetRenderQueueID(NULL, pAVObject);
    AddSceneToQueue(queueID, pAVObject);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RemoveRenderedObject(SceneGraphService::SceneGraphHandle handle,
    NiAVObject* pAVObject)
{
    EE_UNUSED_ARG(handle);

    RemoveSceneFromQueue(pAVObject);
}

//--------------------------------------------------------------------------------------------------
void RenderContext::RaiseSurfaceAdded(RenderSurface* pSurface)
{
    m_pRenderService->RaiseSurfaceAdded(pSurface);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RaiseSurfaceRemoved(RenderSurface* pSurface)
{
    m_pRenderService->RaiseSurfaceRemoved(pSurface);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RaisePreDrawEvent(RenderSurface* pSurface)
{
    m_pRenderService->RaisePreDrawEvent(pSurface);
}
//--------------------------------------------------------------------------------------------------
void RenderContext::RaisePostDrawEvent(RenderSurface* pSurface)
{
    m_pRenderService->RaisePostDrawEvent(pSurface);
}

//--------------------------------------------------------------------------------------------------
void RenderContext::AddSceneToQueue(efd::UInt32 queueID, NiAVObject* pkScene)
{
	// Get the view associated with the queueID
	Ni3DRenderView* pView = GetQueue(queueID);
	EE_ASSERT(pView);

	// Add the extra data for the queue onto the pkScene
	pkScene->AddExtraData(m_queueDataMap[queueID]);

	// Append the scene to the view
	pView->AppendScene(pkScene);
}


//--------------------------------------------------------------------------------------------------
void RenderContext::RemoveSceneFromQueue(NiAVObject* pkScene)
{
	// Get the queueID extra data from the scene
	NiIntegerExtraData* pExtraData = NiDynamicCast(NiIntegerExtraData,
		pkScene->GetExtraData(m_kRenderQueueInUseExtraData));

	EE_ASSERT(pExtraData);

	// Get the queueID from the extra data
	efd::UInt32 queueID = pExtraData->GetValue();

	// Get the RenderView associated with this queueID
	Ni3DRenderView* pView = GetQueue(queueID);
	EE_ASSERT(pView);

	// Remove the extra data from the pkScene
	pkScene->RemoveExtraData(m_kRenderQueueInUseExtraData);

	// Remove the pkScene from the RenderView
	pView->RemoveScene(pkScene);
}

//--------------------------------------------------------------------------------------------------
void RenderContext::AddRenderQueue(efd::UInt32 queueID, Ni3DRenderView* pRenderView)
{
	//ensure there isnt already a view for this queueID
	EE_VERIFY(m_queueIndexMap.find(queueID) == m_queueIndexMap.end());
	
	// Insert an entry into the index map
	m_queueIndexMap[queueID] = 0;

	// Find the location of the entry
	QueueIndexMap::iterator queueMapIter = m_queueIndexMap.find(queueID);
	EE_ASSERT(queueMapIter != m_queueIndexMap.end());

	// Find the insertion index for the queue map
	efd::UInt32 insertionIndex = m_renderQueue.size();
	if ((++queueMapIter) != m_queueIndexMap.end())
	{
		insertionIndex = queueMapIter->second;
	}

	// Set the inserion index properly
	m_queueIndexMap[queueID] = insertionIndex;

	// Increment all the other indices to ensure they are synchronized
	for (;queueMapIter != m_queueIndexMap.end(); ++queueMapIter)
	{
		queueMapIter->second += 1;
	}

	// Create an extra data for this queue
	NiIntegerExtraDataPtr spData = EE_NEW NiIntegerExtraData(queueID);
	spData->SetName(m_kRenderQueueInUseExtraData);
	m_queueDataMap[queueID] = spData;

	// Create the new render view for this queueID and insert into its correct location
	Ni3DRenderViewPtr spRenderView = pRenderView;
	m_renderQueue.insert((m_renderQueue.begin() + insertionIndex), spRenderView);

	// Update the clicks with the new render queue
	for (efd::UInt32 i = 0; i < m_renderSurfaces.size(); ++i)
	{
		RenderSurface* pSurface = m_renderSurfaces[i];
		NiViewRenderClick* pRenderClick = pSurface->GetSceneRenderClick();
		pRenderClick->RemoveAllRenderViews();
		AddRenderQueues(pRenderClick);
	}
}

//--------------------------------------------------------------------------------------------------
void RenderContext::AddRenderQueues(NiViewRenderClick* pRenderClick)
{
	// Iterate over the render queue, appending to the click in sequence/
	for (efd::UInt32 index = 0; index < m_renderQueue.size(); ++index)
	{
		pRenderClick->AppendRenderView(m_renderQueue[index]);
	}
}

//--------------------------------------------------------------------------------------------------
Ni3DRenderView* RenderContext::GetQueue(efd::UInt32 queueID)
{
	if (m_queueIndexMap.find(queueID) == m_queueIndexMap.end())
	{
		Ni3DRenderView* pRenderView = EE_NEW Ni3DRenderView();
		pRenderView->SetCullingProcess(m_spCuller);

		AddRenderQueue(queueID, pRenderView);

		// could return the view here directly
		// but better to ensure that it goes through smoothly
	}
	
	efd::UInt32 queueIndex;
	EE_VERIFY(m_queueIndexMap.find(queueID, queueIndex));
	EE_ASSERT(queueIndex < m_renderQueue.size());

	return m_renderQueue[queueIndex];
}

//--------------------------------------------------------------------------------------------------

