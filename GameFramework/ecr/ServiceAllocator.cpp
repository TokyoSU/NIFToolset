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


// Precompiled Header
#include "ecrPCH.h"

#include <ecr/RenderService.h>
#include <ecr/SceneGraphService.h>
#include <ecr/LightService.h>
#include <ecr/CameraService.h>
#include <ecr/PickService.h>
#include <ecr/ServiceAllocator.h>

#if !defined(EE_CONFIG_SHIPPING)
#include <ecr/TextureRapidIterationService.h>
#endif

using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
Bool ecr::CreateRuntimeServices(ServiceManager* pServiceManager, UInt32 flags)
{
    // The RenderService renders the scene every tick.
    RenderServicePtr spRenderService = EE_NEW RenderService();
    pServiceManager->RegisterSystemService(spRenderService);

    // The SceneGraphService manages all the nodes that form the graph of renderable objects.
    SceneGraphServicePtr spSceneGraphService = EE_NEW SceneGraphService();
    pServiceManager->RegisterSystemService(spSceneGraphService);

    // The LightService creates and updates NiLight objects based on Light entities.
    LightServicePtr spLightService = EE_NEW LightService();
    pServiceManager->RegisterSystemService(spLightService);

    // The CameraService creates and updates NiCamera objects based on their Entity
    // representations.
    ecr::CameraServicePtr spCameraService = EE_NEW ecr::CameraService();
    pServiceManager->RegisterSystemService(spCameraService);

    // The PickService can be used to compute ray intersections with the scene graph.
    if (!(flags & rsaf_NO_PICK_SERVICE))
    {
        PickServicePtr spPickService = EE_NEW PickService();
        pServiceManager->RegisterSystemService(spPickService);
    }

#if !defined(EE_CONFIG_SHIPPING)
    // The TextureRapidIterationService handles rapid iteration of external textures.
    if ((flags & rsaf_NO_RAPID_ITERATION) == 0)
    {
        TextureRapidIterationServicePtr spTextureService = EE_NEW TextureRapidIterationService();
        pServiceManager->RegisterSystemService(spTextureService);
    }
#endif

    return true;
}
