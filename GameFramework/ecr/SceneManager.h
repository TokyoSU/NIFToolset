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

#pragma once
#ifndef EE_SCENEMANAGER_H
#define EE_SCENEMANAGER_H

#include "ecrLibType.h"

#include <efd/ISystemService.h>
#include <ecr/ecrSystemServiceIDs.h>

#include <NiNode.h>
#include <NiCamera.h>
#include <NiStream.h>
#include <NiActorManager.h>
#include <Ni3DRenderView.h>
#include <NiMeshCullingProcess.h>

#include <unordered_map>

namespace ecr
{

class RenderService;
class RenderSurface;

/**
    SceneManager is a lightweight ISystemService that manages manually loaded
    scenes (NIF) and animated actors (KFM/KF) independently of the entity system.

    It owns a single Ni3DRenderView that is attached to the active RenderSurface's
    NiViewRenderClick on initialisation. All loaded roots are automatically registered
    with that view and updated every tick.

    Priority 1600 — ticks before SceneGraphService (1500) and RenderService (1000).

    Typical usage (from a GameApplication override):
    @code
        void MyApp::OnInitComplete()
        {
            auto* pSM = GetSceneManager();

            // Static NIF
            auto hScene = pSM->LoadNIF("Data/World/Room.nif");

            // Animated character
            auto hActor = pSM->LoadKFM("Data/Characters/Hero.kfm");
            if (auto* pActor = pSM->GetActor(hActor))
                pActor->SetTargetAnimation(0);

            // Camera
            auto* pCam = pSM->GetCamera();
            pCam->SetTranslate(NiPoint3(0.f, -200.f, 80.f));
            pCam->LookAtWorldPoint(NiPoint3::ZERO, NiPoint3::UNIT_Z);
        }
    @endcode
*/
class EE_ECR_ENTRY SceneManager : public efd::ISystemService
{
    /// @cond EMERGENT_INTERNAL
    EE_DECLARE_CLASS1(SceneManager, efd::kCLASSID_SceneManager, efd::ISystemService);
    EE_DECLARE_CONCRETE_REFCOUNT;
    /// @endcond

public:

    /// Opaque handle to a loaded NIF scene root.
    using SceneHandle = efd::UInt32;
    /// Opaque handle to a loaded KFM actor.
    using ActorHandle = efd::UInt32;

    static const SceneHandle kInvalidSceneHandle = 0u;
    static const ActorHandle kInvalidActorHandle = 0u;

    SceneManager();
    virtual ~SceneManager();

    // ------------------------------------------------------------------
    //  Static NIF loading
    // ------------------------------------------------------------------

    /**
        Load a NIF from disk and register it for rendering and per-tick update.

        @param pcPath  Path to the .nif file.
        @return        A handle used to address the scene later, or kInvalidSceneHandle on error.
    */
    SceneHandle LoadNIF(const char* pcPath);

    /**
        Remove a previously loaded NIF from the view and release it.

        @param hScene  Handle returned by LoadNIF(). No-op if kInvalidSceneHandle.
    */
    void UnloadNIF(SceneHandle hScene);

    /**
        Return the NiNode root for an active scene, or nullptr.
    */
    NiNode* GetSceneRoot(SceneHandle hScene) const;

    // ------------------------------------------------------------------
    //  Animated actor loading (KFM + KF + NIF)
    // ------------------------------------------------------------------

    /**
        Load a KFM file and all referenced KF / NIF assets, then register for rendering.

        @param pcPath         Path to the .kfm file.
        @param bCumulative    Pass true for cumulative (additive) animation blending.
        @return               A handle, or kInvalidActorHandle on error.
    */
    ActorHandle LoadKFM(const char* pcPath, bool bCumulative = false);

    /**
        Remove a previously loaded actor from the view and release it.

        @param hActor  Handle returned by LoadKFM(). No-op if kInvalidActorHandle.
    */
    void UnloadKFM(ActorHandle hActor);

    /**
        Return the NiActorManager for an active actor, or nullptr.
    */
    NiActorManager* GetActor(ActorHandle hActor) const;

    // ------------------------------------------------------------------
    //  Camera
    // ------------------------------------------------------------------

    /**
        Replace the render view camera.

        Ownership is shared (smart pointer); the caller may continue to hold a reference.
        Passing nullptr resets to the default surface camera.
    */
    void SetCamera(NiCamera* pCamera);

    /// Return the current render view camera.
    NiCamera* GetCamera() const;

    // ------------------------------------------------------------------
    //  ISystemService
    // ------------------------------------------------------------------
    const char* GetDisplayName() const override;

protected:
    efd::SyncResult  OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar) override;
    efd::AsyncResult OnInit()     override;
    efd::AsyncResult OnTick()     override;
    efd::AsyncResult OnShutdown() override;

private:
    /// Attach pRoot to the managed Ni3DRenderView.
    void AttachToView(NiAVObject* pRoot);
    /// Detach pRoot from the managed Ni3DRenderView.
    void DetachFromView(NiAVObject* pRoot);

    // Services
	NiVisibleArray m_visibleArray; // Reused every tick to avoid reallocations.
    ecr::RenderService*  m_pRenderService = nullptr;

    // Render objects
    NiPointer<Ni3DRenderView>          m_spRenderView;
    NiPointer<NiMeshCullingProcess>    m_spCullingProcess;  // was NiFrustumCullingProcess

    // Scene storage
    efd::UInt32 m_uiNextSceneHandle = 1u;
    efd::UInt32 m_uiNextActorHandle = 1u;

    std::unordered_map<SceneHandle, NiNodePtr>                 m_scenes;
    std::unordered_map<ActorHandle, NiPointer<NiActorManager>> m_actors;

    // Frame time accumulator
    float m_fTime = 0.0f;
};

} // namespace ecr

#endif // EE_SCENEMANAGER_H