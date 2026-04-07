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
//--------------------------------------------------------------------------------------------------
#include "NiMeshPCH.h"
#include "NiMeshSDM.h"
#include "NiMeshLib.h"
#include "NiMeshModifier.h"
#include "NiMeshCullingProcess.h"
#include "NiToolDataStream.h"
#include "NiMeshVertexOperators.h"
#include "NiSkinningMeshModifier.h"
#include "NiMorphMeshModifier.h"
#include "NiInstancingMeshModifier.h"
#include "NiMeshHWInstance.h"
#include "NiMeshScreenElements.h"
#include "NiScreenFillingRenderViewImpl.h"
#include "NiMeshUpdateProcess.h"
#include <NiStream.h>
#include <NiShadowManager.h>

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
#include "NiGeometryConverter.h"
#endif

//--------------------------------------------------------------------------------------------------
NiImplementSDMConstructor(NiMesh, "NiFloodgate NiMain");

//--------------------------------------------------------------------------------------------------
#ifdef NIMESH_EXPORT
NiImplementDllMain(NiMesh);
#endif

//--------------------------------------------------------------------------------------------------
void NiMeshSDM::Init()
{
    NiImplementSDMInitCheck();
    NiMeshVertexOperators::_SDMInit();

    // Register implementation classes
    NiRegisterStream(NiMesh);
    NiRegisterStream(NiMeshScreenElements);
    NiRegisterStream(NiMorphMeshModifier);
    NiRegisterStream(NiSkinningMeshModifier);
    NiRegisterStream(NiInstancingMeshModifier);
    NiRegisterStream(NiMeshHWInstance);
    NiRegisterStream(NiDataStream);

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
    NiStream::RegisterPostProcessFunction(
        NiGeometryConverter::ConvertToNiMesh);
#endif

    NiScreenFillingRenderViewImpl::RegisterFactoryMethod();

    // Give NiAVObject a default update process to use for the legacy
    // Update(float) calls
    NiAVObject::RegisterDefaultUpdateProcess(NiNew NiMeshUpdateProcess());

    // Register NiMeshCullingProcess as the culling process to be used
    // by the shadowing system.
    NiShadowManager::SetShadowRenderCullingProcessFactory(
        NiMeshCullingProcess::CreateMeshCullingProcess);
}

//--------------------------------------------------------------------------------------------------
void NiMeshSDM::Shutdown()
{
    NiImplementSDMShutdownCheck();

    NiAVObject::RegisterDefaultUpdateProcess(NULL);

    NiUnregisterStream(NiMesh);
    NiUnregisterStream(NiMeshScreenElements);
    NiUnregisterStream(NiMorphMeshModifier);
    NiUnregisterStream(NiSkinningMeshModifier);
    NiUnregisterStream(NiInstancingMeshModifier);
    NiUnregisterStream(NiMeshHWInstance);
    NiUnregisterStream(NiDataStream);

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING
    NiStream::UnregisterPostProcessFunction(
        NiGeometryConverter::ConvertToNiMesh);
#endif

    NiMeshVertexOperators::_SDMShutdown();
}

//--------------------------------------------------------------------------------------------------
