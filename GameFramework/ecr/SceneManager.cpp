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

#include <ecr/SceneManager.h>
#include <ecr/RenderService.h>
#include <ecr/RenderSurface.h>

#include <efd/ServiceManager.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

#include <NiStream.h>
#include <NiActorManager.h>
#include <NiMeshCullingProcess.h>
#include <NiViewRenderClick.h>

using namespace efd;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SceneManager);

//--------------------------------------------------------------------------------------------------
SceneManager::SceneManager() : m_visibleArray(1024, 1024)
{
    // Ticks before SceneGraphService (1500) and RenderService (1000).
    m_defaultPriority = 1600;
}

//--------------------------------------------------------------------------------------------------
SceneManager::~SceneManager() = default;

//--------------------------------------------------------------------------------------------------
const char* SceneManager::GetDisplayName() const
{
    return "SceneManager";
}

//--------------------------------------------------------------------------------------------------
SyncResult SceneManager::OnPreInit(IDependencyRegistrar* pDependencyRegistrar)
{
    // RenderService must be fully initialised before we can attach to the surface.
    pDependencyRegistrar->AddDependency<RenderService>();
    return SyncResult_Success;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SceneManager::OnInit()
{
    m_pRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();
    if (!m_pRenderService)
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR0,
            ("SceneManager::OnInit — RenderService not found."));
        return AsyncResult_Failure;
    }

    RenderSurface* pSurface = m_pRenderService->GetActiveRenderSurface();
    if (!pSurface)
    {
        // Surface not ready yet — retry next tick instead of failing permanently.
        return AsyncResult_Pending;
    }

    NiCamera* pCamera = pSurface->GetCamera();
    if (!pCamera)
    {
        // Camera not yet initialised (SetupDefaultCamera may not have run).
        // Ask the surface to configure itself with a sane default, then retry.
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kLVL1,
            ("SceneManager::OnInit — surface camera is null, retrying."));
        return AsyncResult_Pending;
    }

    // Use SceneGraphService's workflow manager if it is already initialised;
    // this matches what RenderContext does and avoids skinning/morph issues.
    NiSPWorkflowManager* pWorkflowManager = nullptr;
    auto* pSGS = m_pServiceManager->GetSystemServiceAs<SceneGraphService>();
    if (pSGS)
        pWorkflowManager = pSGS->GetWorkflowManager();

    m_spCullingProcess = NiNew NiMeshCullingProcess(&m_visibleArray, pWorkflowManager);
    m_spRenderView     = NiNew Ni3DRenderView(pCamera, m_spCullingProcess);
    m_spRenderView->SetName("SceneManagerView");

    pSurface->GetSceneRenderClick()->AppendRenderView(m_spRenderView);

    // Retroactively attach any scenes / actors that were loaded before OnInit ran
    // (e.g. when LoadNIF / LoadKFM is called between Initialize() and the first Tick()).
    for (auto& [handle, spNode] : m_scenes)
        AttachToView(spNode.data());
    for (auto& [handle, spActor] : m_actors)
        AttachToView(spActor->GetNIFRoot());

    return AsyncResult_Complete;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SceneManager::OnTick()
{
    // Advance time by the service manager's delta.
    // GetTime returns absolute time; we derive delta from it each tick.
    const float fAbsTime = static_cast<float>(
        m_pServiceManager->GetTime(kCLASSID_GameTimeClock));

    // Update static scenes.
    for (auto& [handle, spNode] : m_scenes)
    {
        spNode->Update(fAbsTime);
        spNode->UpdateProperties();
        spNode->UpdateEffects();
    }

    // Update animated actors (NiActorManager drives its own scene graph update internally).
    for (auto& [handle, spActor] : m_actors)
    {
        spActor->Update(fAbsTime);
    }

    return AsyncResult_Pending;
}

//--------------------------------------------------------------------------------------------------
AsyncResult SceneManager::OnShutdown()
{
    // Detach all roots from the view before releasing them.
    for (auto& [handle, spNode] : m_scenes)
        DetachFromView(spNode);

    for (auto& [handle, spActor] : m_actors)
        DetachFromView(spActor->GetNIFRoot());

    m_scenes.clear();
    m_actors.clear();

    // Remove the view from the surface if both are still alive.
    if (m_pRenderService)
    {
        RenderSurface* pSurface = m_pRenderService->GetActiveRenderSurface();
        if (pSurface && m_spRenderView)
            pSurface->GetSceneRenderClick()->RemoveRenderView(m_spRenderView);
    }

    m_spRenderView     = nullptr;
    m_spCullingProcess = nullptr;
    m_pRenderService   = nullptr;

    return AsyncResult_Complete;
}

//--------------------------------------------------------------------------------------------------
SceneManager::SceneHandle SceneManager::LoadNIF(const char* pcPath)
{
    EE_ASSERT(pcPath);

    NiStream kStream;
    if (!kStream.Load(pcPath))
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR1,
            ("SceneManager::LoadNIF — failed to load '%s'.", pcPath));
        return kInvalidSceneHandle;
    }

    NiNode* pRoot = NiDynamicCast(NiNode, kStream.GetObjectAt(0));
    if (!pRoot)
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR1,
            ("SceneManager::LoadNIF — root object in '%s' is not an NiNode.", pcPath));
        return kInvalidSceneHandle;
    }

    // Force an initial scene-graph update.
    pRoot->Update(0.0f);
    pRoot->UpdateProperties();
    pRoot->UpdateEffects();

    const SceneHandle hScene = m_uiNextSceneHandle++;
    m_scenes.emplace(hScene, pRoot);
    AttachToView(pRoot);

    return hScene;
}

//--------------------------------------------------------------------------------------------------
void SceneManager::UnloadNIF(SceneHandle hScene)
{
    auto it = m_scenes.find(hScene);
    if (it == m_scenes.end())
        return;

    DetachFromView(it->second);
    m_scenes.erase(it);
}

//--------------------------------------------------------------------------------------------------
NiNode* SceneManager::GetSceneRoot(SceneHandle hScene) const
{
    auto it = m_scenes.find(hScene);
    return it != m_scenes.end() ? it->second.data() : nullptr;
}

//--------------------------------------------------------------------------------------------------
SceneManager::ActorHandle SceneManager::LoadKFM(const char* pcPath, bool bCumulative)
{
    EE_ASSERT(pcPath);

    NiActorManager* pActor = NiActorManager::Create(
        pcPath,
        bCumulative,
        /*bLoadFilesFromDisk=*/ true);

    if (!pActor)
    {
        EE_LOG(efd::kGamebryoGeneral0, ILogger::kERR1,
            ("SceneManager::LoadKFM — failed to load '%s'.", pcPath));
        return kInvalidActorHandle;
    }

    pActor->Update(0.0f);

    const ActorHandle hActor = m_uiNextActorHandle++;
    m_actors.emplace(hActor, pActor);
    AttachToView(pActor->GetNIFRoot());

    return hActor;
}

//--------------------------------------------------------------------------------------------------
void SceneManager::UnloadKFM(ActorHandle hActor)
{
    auto it = m_actors.find(hActor);
    if (it == m_actors.end())
        return;

    DetachFromView(it->second->GetNIFRoot());
    m_actors.erase(it);
}

//--------------------------------------------------------------------------------------------------
NiActorManager* SceneManager::GetActor(ActorHandle hActor) const
{
    auto it = m_actors.find(hActor);
    return it != m_actors.end() ? it->second.data() : nullptr;
}

//--------------------------------------------------------------------------------------------------
void SceneManager::SetCamera(NiCamera* pCamera)
{
    if (!m_spRenderView)
        return;

    // Fall back to surface camera when nullptr is passed.
    if (!pCamera && m_pRenderService)
    {
        RenderSurface* pSurface = m_pRenderService->GetActiveRenderSurface();
        if (pSurface)
            pCamera = pSurface->GetCamera();
    }

    m_spRenderView->SetCamera(pCamera);
}

//--------------------------------------------------------------------------------------------------
NiCamera* SceneManager::GetCamera() const
{
    return m_spRenderView ? m_spRenderView->GetCamera() : nullptr;
}

//--------------------------------------------------------------------------------------------------
void SceneManager::AttachToView(NiAVObject* pRoot)
{
    if (m_spRenderView && pRoot)
        m_spRenderView->AppendScene(pRoot);
}

//--------------------------------------------------------------------------------------------------
void SceneManager::DetachFromView(NiAVObject* pRoot)
{
    if (m_spRenderView && pRoot)
        m_spRenderView->RemoveScene(pRoot);
}