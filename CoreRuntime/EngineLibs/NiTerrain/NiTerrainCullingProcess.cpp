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
//--------------------------------------------------------------------------------------------------
// Precompiled Header
#include "NiTerrainPCH.h"

#include "NiTerrainCullingProcess.h"
#include "NiTerrainUtils.h"

NiImplementRTTI(NiTerrainCullingProcess, NiMeshCullingProcess);

//--------------------------------------------------------------------------------------------------
NiTerrainCullingProcess::NiTerrainCullingProcess(NiVisibleArray* pkVisibleSet,
    NiSPWorkflowManager* pkWorkflowManager) :
    NiMeshCullingProcess(pkVisibleSet, pkWorkflowManager),
    m_uiMaxLOD(NiTerrainUtils::ms_uiMAX_LOD),
    m_uiMinLOD(0),
    m_bUpdateGeometry(true),
    m_bRenderDecals(true)
{
    /* */
}

//--------------------------------------------------------------------------------------------------
NiCullingProcess* NiTerrainCullingProcess::CreateTerrainCullingProcess()
{
    // Create a new terrain culling process to be used by the shadowing system.
    NiTerrainCullingProcess* pkCullingProcess =
        NiNew NiTerrainCullingProcess(NULL, NULL);

    // Disable rendering decals since this culling process will be used to
    // while rendering to a shadow map.
    pkCullingProcess->SetRenderDecals(false);

    return pkCullingProcess;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCullingProcess::SetMaxTerrainLOD(NiUInt32 uiMaxLOD)
{
    m_uiMaxLOD = uiMaxLOD;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainCullingProcess::GetMaxTerrainLOD() const
{
    return m_uiMaxLOD;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCullingProcess::SetMinTerrainLOD(NiUInt32 uiMinLOD)
{
    m_uiMinLOD = uiMinLOD;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainCullingProcess::GetMinTerrainLOD() const
{
    return m_uiMinLOD;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCullingProcess::SetUpdateGeometry(bool bUpdateGeometry)
{
    m_bUpdateGeometry = bUpdateGeometry;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCullingProcess::GetUpdateGeometry() const
{
    return m_bUpdateGeometry;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCullingProcess::SetRenderDecals(bool bRenderDecals)
{
    m_bRenderDecals = bRenderDecals;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCullingProcess::GetRenderDecals() const
{
    return m_bRenderDecals;
}

//--------------------------------------------------------------------------------------------------