// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"
#include "NiTextureRegion.h"
#include "NiTerrainCellNode.h"

//---------------------------------------------------------------------------
NiImplementRTTI(NiTerrainCellNode, NiTerrainCell);
//---------------------------------------------------------------------------
NiTerrainCellNode::NiTerrainCellNode(NiTerrainSector* pkSector, NiUInt32 uiLevel) : 
    NiTerrainCell(pkSector, uiLevel)
{
    // Initialize child array
    for (NiUInt32 ui = 0; ui < NUM_CHILDREN; ++ui)
        m_apkChildren[ui] = NULL;
}
//---------------------------------------------------------------------------
NiTerrainCellNode::~NiTerrainCellNode()
{

}
//---------------------------------------------------------------------------
void NiTerrainCellNode::Update()
{
    // Freeze updating the bounds until after children are updated
    bool bUpdateBounds = GetUpdateFlag(UPDATE_CELL_BOUNDS);
    SetUpdateFlag(false, UPDATE_CELL_BOUNDS);

    // Update the cell as per normal (without bounds)
    NiTerrainCell::Update();

    // Make sure we can descend the tree
    if (m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1))
    {
        // Check if our children require an update 
        NiTerrainCell* pkChild;
        for (NiUInt32 uiIndex = 0; uiIndex < NUM_CHILDREN; ++uiIndex)
        {
            pkChild = GetChildAt(uiIndex);
            if (!pkChild) 
                break;

            if (pkChild->RequiresUpdate())
                pkChild->Update();
        }
    }

    // Complete updating the bounds
    if (bUpdateBounds)
        UpdateBounds();
}
//---------------------------------------------------------------------------
void NiTerrainCellNode::UpdateBounds()
{
    // Generate the bounds based on this mesh
    NiTerrainCell::UpdateBounds();

    // Merge the child bounds
    if (m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1))
    {
        NiTerrainCell* pkChild = NULL;
        NiBound kChildBound;
        NiBoxBV kChildBoundBox;
        float fLocalZExtent = m_kBoxBound.GetExtent(2);
        NiPoint3 kLocalCenter = m_kBoxBound.GetCenter();
        float fLocalZMax = kLocalCenter.z + fLocalZExtent;
        float fLocalZMin = kLocalCenter.z - fLocalZExtent;
        for (NiUInt32 uiIndex = 0; uiIndex < NUM_CHILDREN; ++uiIndex)
        {
            pkChild = GetChildAt(uiIndex);
            if (!pkChild) 
                break;

            // Merge the bounding sphere
            kChildBound = pkChild->GetLocalBound();
            m_kBound.Merge(&kChildBound);

            // Merge the bounding box
            kChildBoundBox = pkChild->GetLocalBoxBound();
            fLocalZMax = NiMax(fLocalZMax,
                kChildBoundBox.GetCenter().z + kChildBoundBox.GetExtent(2)); 
            fLocalZMin = NiMin(fLocalZMin,
                kChildBoundBox.GetCenter().z - kChildBoundBox.GetExtent(2));
        }
        kLocalCenter.z = (fLocalZMax + fLocalZMin) / 2.0f;
        fLocalZExtent = fLocalZMax - kLocalCenter.z;
        m_kBoxBound.SetExtent(2, fLocalZExtent);
        m_kBoxBound.SetCenter(kLocalCenter);
    }

    // if we have a mesh, make sure its bound is up to date.
    if (m_spMesh != NULL)
        m_spMesh->SetWorldBound(m_kBound);

    // Flag the bounds as built
    SetUpdateFlag(false, UPDATE_CELL_BOUNDS);
}
//---------------------------------------------------------------------------
bool NiTerrainCellNode::ProcessLOD()
{
    if (!NiTerrainCell::ProcessLOD())
        return false;

    NiTerrainCell* pkChild = GetChildAt(0);

    // Can we add children?
    bool bAllowed = (m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1)) &&
        (m_pkSectorData->GetHighestVisibleLOD() > GetLevel());

    if (!pkChild || !bAllowed)
    {
        // We don't have any children; add our low detail mesh instead.
        AddToVisible();
    }
    else
    {
        bool bVisibleFlag = false;
        
        // Do we need to force a higher LOD?
        if (GetLevel() < m_pkSectorData->GetLowestVisibleLOD())
        {
            // Force the higher LOD by recursing to children.
            bVisibleFlag = true;
        }
        else
        {        
            for (NiUInt32 ui = 0; ui < 4; ui++)
            {
                pkChild = GetChildAt(ui);
                EE_ASSERT(pkChild);

                if (pkChild->IsInRange())
                {
                    bVisibleFlag = true;
                    break;
                }
            }
        }

        // If any child is "InRange" then this implies the need to further 
        // subdivide the terrain by continuing down the hierarchy.
        if (bVisibleFlag)
        {
            SetCullingResult(CHILDREN_VISIBLE);

            GetChildAt(0)->ProcessLOD();
            GetChildAt(1)->ProcessLOD();
            GetChildAt(2)->ProcessLOD();
            GetChildAt(3)->ProcessLOD();
        }
        else
        {
            // We have subdivided the terrain block as far as we need to based 
            // on its distance from the camera so just go ahead and add 
            // the block.
            AddToVisible();
        }
    }

    return true;
}
//---------------------------------------------------------------------------
void NiTerrainCellNode::ProcessBorders()
{
    NiTerrainCell::ProcessBorders();

    if (GetCullingResult() == CHILDREN_VISIBLE)
    {
        NiTerrainCell* pkChild = GetChildAt(0);

        // We either have 0 or 4 children
        if (pkChild && m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel + 1))) 
        {     
            if (!NiTerrainCell::IsCellVisible(pkChild))
                pkChild->ProcessBorders();

            pkChild = GetChildAt(1);
            if (!NiTerrainCell::IsCellVisible(pkChild))
                pkChild->ProcessBorders();

            pkChild = GetChildAt(2);
            if (!NiTerrainCell::IsCellVisible(pkChild))
                pkChild->ProcessBorders();

            pkChild = GetChildAt(3);
            if (!NiTerrainCell::IsCellVisible(pkChild))
                pkChild->ProcessBorders();
        }
    }
}
//---------------------------------------------------------------------------
void NiTerrainCellNode::SetTextureRegion(NiPoint2 kOffset, float fScale, 
    NiTerrainCell::TextureType::Value eTexType)
{
    NiTerrainCell::SetTextureRegion(kOffset, fScale, eTexType);

    // Adjust the scale and offset for the children
    fScale *= 2.0f;
    const NiPoint2 kOffsetMultipliers[4] = {
        NiPoint2(0.0f, 1.0f),
        NiPoint2(1.0f, 1.0f),
        NiPoint2(1.0f, 0.0f),
        NiPoint2(0.0f, 0.0f)};
    float fOffsetScale  = 1.0f / (fScale);

    if (!m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1))
        return;

    // Iterate over the children
    for (NiUInt32 ui = 0; ui < 4; ui++)
    {
        // Fetch the child
        NiTerrainCell* pkChild = GetChildAt(ui);
        if (!pkChild)
        {
            return;
        }

        // Calculate this child's offset values
        NiPoint2 kChildOffset = kOffset;
        kChildOffset += kOffsetMultipliers[ui] * fOffsetScale;
        
        // Update the child's region        
        pkChild->SetTextureRegion(kChildOffset, fScale, eTexType);        
    }
}
//---------------------------------------------------------------------------
void NiTerrainCellNode::SetTexture(NiTexture* pkTexture, 
    NiTerrainCell::TextureType::Value eTexType)
{
    NiTerrainCell::SetTexture(pkTexture, eTexType);

    if (!m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1))
        return;

    for (NiUInt32 ui = 0; ui < 4; ui++)
    {
        // Find the children and apply the new texture
        NiTerrainCell* pkCell = GetChildAt(ui);
        if (!pkCell)
        {
            return;
        }
        
        pkCell->SetTexture(pkTexture, eTexType);        
    }
}
//---------------------------------------------------------------------------
bool NiTerrainCellNode::DoAddSurface(NiUInt32 uiSurfaceIndex, 
    NiUInt32 uiNewPriority)
{
    NiTerrainCell* pkChild = GetChildAt(0);

        // We either have 0 or 4 children
    if (pkChild && m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1)) 
    {     
        pkChild->AddSurface(uiSurfaceIndex, uiNewPriority);

        pkChild = GetChildAt(1);
        pkChild->AddSurface(uiSurfaceIndex, uiNewPriority);

        pkChild = GetChildAt(2);
        pkChild->AddSurface(uiSurfaceIndex, uiNewPriority);

        pkChild = GetChildAt(3);
        pkChild->AddSurface(uiSurfaceIndex, uiNewPriority);
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainCellNode::DoRemoveSurface(NiUInt32 uiSurfaceIndex)
{
    NiTerrainCell* pkChild = GetChildAt(0);

    // We either have 0 or 4 children
    if (pkChild && m_pkSectorData->IsLODLoaded(NiInt32(m_uiLevel) + 1)) 
    {     
        pkChild->RemoveSurface(uiSurfaceIndex);

        pkChild = GetChildAt(1);
        pkChild->RemoveSurface(uiSurfaceIndex);

        pkChild = GetChildAt(2);
        pkChild->RemoveSurface(uiSurfaceIndex);

        pkChild = GetChildAt(3);
        pkChild->RemoveSurface(uiSurfaceIndex);
    }

    return true;
}

