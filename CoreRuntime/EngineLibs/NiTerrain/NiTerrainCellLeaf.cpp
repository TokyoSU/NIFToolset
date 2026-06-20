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
#include "NiTerrainCellLeaf.h"
#include "NiTextureRegion.h"
#include "NiTerrain.h"
#include "NiSurface.h"
#include "NiTerrainDataSnapshot.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainCellLeaf, NiTerrainCell, NiTypeMask::NiTerrainCellLeaf);
//--------------------------------------------------------------------------------------------------
NiTerrainCellLeaf::NiTerrainCellLeaf(NiTerrainSector* pkSector, NiUInt32 uiLevel) : 
    NiTerrainCell(pkSector, uiLevel),
    m_uiNumUsedSurfaces(0),
    m_bSurfaceChanged(true),
    m_bSurfaceLayersChanged(true)
{    
    // initialize the surface mask arrays
    for (NiUInt32 ui = 0; ui < MAX_NUM_SURFACES; ++ui)
    {
        m_aiSurfaceIndex[ui] = -1;
    }
}

    //--------------------------------------------------------------------------------------------------
NiTerrainCellLeaf::~NiTerrainCellLeaf()
{
    // Release any allocated resources
    // Only release the blend mask if we are the bottom left cell using this
    // blend mask. (If the region start pixel is 1, 1 then we are the bottom
    // left)
    if (m_kBlendTextureRegion.GetStartPixelIndex() == NiIndex(1,1))
    {
        GetResourceManager()->ReleaseTexture(NiTerrain::TextureType::BLEND_MASK,
            m_kBlendTextureRegion.GetTexture());
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::DoAddSurface(NiUInt32 uiSurfaceIndex, 
    NiUInt32 uiNewPriority)
{
    // Prepare to paint on this leaf.
    PrepareForPainting();

    // Attempt to find out if the surface is already there
    NiUInt32 uiCurrentPriority;
    if (GetSurfacePriority(uiSurfaceIndex, uiCurrentPriority))
    {
        uiNewPriority = uiCurrentPriority;
        return true;
    }
    
    // We didn't find it, check that we aren't full then
    if (m_uiNumUsedSurfaces == MAX_NUM_SURFACES)
        return false;
    
    // Add the surface
    m_aiSurfaceIndex[m_uiNumUsedSurfaces++] = uiSurfaceIndex;

    // Mark the surface masks as changed
    MarkSurfaceMasksChanged(true);
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::PrepareForPainting()
{
    if (!m_kBlendTextureRegion.GetTexture())
    {
        SetTexture(m_pkContainingSector->CreateBlendTexture(), 
            TextureType::BLEND_MASK);  
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::DoRemoveSurface(NiUInt32 uiSurfaceIndex)
{
    // Quit immediately if there are no surfaces
    if (m_uiNumUsedSurfaces == 0)
        return false;

    // Find the entry in the priority list
    NiUInt32 uiPriority;
    if (!GetSurfacePriority(uiSurfaceIndex, uiPriority))
    {
        return false;
    }

    // We remove a surface reference and replace it by the last added 
    // surface. We also need to swap out the channels in the blend mask

    if (uiPriority != m_uiNumUsedSurfaces - 1)
    {
        // Swap the references to the masks in the surface index array
        // This allows GetPixel/SetPixel to deal with borders correctly 
        // in the following Copy/Clear calls
        m_aiSurfaceIndex[uiPriority] = 
            m_aiSurfaceIndex[m_uiNumUsedSurfaces - 1];
        m_aiSurfaceIndex[m_uiNumUsedSurfaces - 1] = uiSurfaceIndex;

        // Copy the values from the last mask and put them in the removed 
        // position    
        CopySurfaceMask(m_uiNumUsedSurfaces - 1, uiPriority);
    }

    // Clear the values in the removed mask and push that data into surrounding
    // blocks.
    ClearSurfaceMask(m_uiNumUsedSurfaces - 1, 0);
    SyncAdjacentSurfaceMasks(m_uiNumUsedSurfaces - 1, true);
    // Set the final mask for use by other surfaces
    m_uiNumUsedSurfaces -= 1;
    m_aiSurfaceIndex[m_uiNumUsedSurfaces] = -1;

    // Flag this cell for a shader update
    MarkSurfaceMasksChanged(true);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::DoReplaceSurface(NiUInt32 uiSurfaceIndex, 
    NiUInt32 uiPriority, bool bForceReplace)
{    
    if (!bForceReplace && !EnsureValidReplace(uiSurfaceIndex, uiPriority))
        return false;

    // first check this surface is not on the leaf already
    NiUInt32 uiReplacerPriority;
    if (!GetSurfacePriority(uiSurfaceIndex, uiReplacerPriority))
    {
        // First clear adjacent borders of the replaced surface to 0
        if (!bForceReplace)
            ClearAdjacentBorders(uiPriority);

        // Replace the surfcae
        m_aiSurfaceIndex[uiPriority] = uiSurfaceIndex;
        
        // Update the borders: Pull the data from the adjacent leaf
        SyncAdjacentSurfaceMasks(uiPriority, false);
        MarkSurfaceMasksChanged();
        return true;
    }
    
    // Obtain access to the blend mask texture's data
    NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture,
        m_kBlendTextureRegion.GetTexture());
    EE_ASSERT(pkTexture);
    NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();

    // Fetch the range over which to modify this texture
    NiIndex kStartPoint = m_kBlendTextureRegion.GetStartPixelIndex();
    NiIndex kEndPoint = m_kBlendTextureRegion.GetEndPixelIndex();

    // Include the border pixels
    kStartPoint -= NiIndex(1,1);
    kEndPoint += NiIndex(1,1);

    // we need to merge values between the two surfaces and remove the one we 
    // are replacing
    for (NiUInt32 uiY = kStartPoint.y; uiY < kEndPoint.y; ++uiY)
    {
        for (NiUInt32 uiX = kStartPoint.x; uiX < kEndPoint.x; ++uiX)
        {
            // Get the current value for this point
            NiUInt16 usReplacerValue = DoGetPixelAt(pkPixelData, 
                NiIndex(uiX, uiY), uiReplacerPriority);
            NiUInt16 usReplaceeValue = DoGetPixelAt(pkPixelData, 
                NiIndex(uiX, uiY), uiPriority);
            NiUInt8 usNewValue = NiUInt8(
                NiMin(usReplacerValue + usReplaceeValue, 255));

            // We can now set the new value
            DoSetPixelAt(pkPixelData, NiIndex(uiX, uiY), uiReplacerPriority, 
                usNewValue);                  
        }
    }

    // We can now remove the replaced surface
    DoRemoveSurface(m_aiSurfaceIndex[uiPriority]);
    GetSurfacePriority(uiSurfaceIndex, uiReplacerPriority);
    SyncAdjacentSurfaceMasks(uiReplacerPriority, true);
    
    pkPixelData->MarkAsChanged();

    // Flag this cell for a shader update
    MarkSurfaceMasksChanged(true);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::EnsureValidReplace(NiUInt32 uiSurfaceIndex, 
    NiUInt32 uiPriority)
{
    // for each borders
    // Setup some basic data to allow simple looping through the cases
    const NiUInt32 uiMaxDirections = 8;
    const NiUInt32 auiDirection[uiMaxDirections] = {
        NiTerrainCell::BORDER_LEFT,
        NiTerrainCell::BORDER_RIGHT,
        NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_BOTTOM,
        };

    for (NiUInt32 uiIndex = 0; uiIndex < uiMaxDirections; ++uiIndex)
    {
        NiUInt32 uiDirection = auiDirection[uiIndex];
        NiTerrainCellLeaf* pkAdjacentLeaf = 
            (NiTerrainCellLeaf*)GetAdjacent(uiDirection);
        
        if (!pkAdjacentLeaf || pkAdjacentLeaf == this)
        {
            // There is no leaf in this direction
            continue;
        }

        // Fetch the location of this surface on the adjacent block
        NiUInt32 uiAdjacentPriority;
        if (!pkAdjacentLeaf->GetSurfacePriority(GetSurfaceIndex(uiPriority), 
            uiAdjacentPriority))
        {
            // The surface to be replaced is not on this leaf
            continue;
        }

        // Now get the border values for that material and work out if it is 
        // present there

        // Obtain access to the blend mask texture's data
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture,
            m_kBlendTextureRegion.GetTexture());
        EE_ASSERT(pkTexture);
        NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();

        NiUInt32 uiMaskWidth = pkTexture->GetWidth();
        NiUInt32 uiStride = pkPixelData->GetPixelStride();

        NiUInt8* pucLocalPixels = pkTexture->GetSourcePixelData()->GetPixels();

        // Calculate the number of values to check.
        NiUInt32 uiNumValues = uiMaskWidth;
        if (!NiIsPowerOf2(uiDirection)) // Processing corners
            uiNumValues = 2;

        // Calculate the increment of the pixel position between values
        NiUInt32 uiIncrement;
        NiUInt32 uiDataFlowOffset;
        if (uiDirection & BORDER_LEFT || uiDirection & BORDER_RIGHT)
        {
            uiIncrement = uiMaskWidth;
            uiDataFlowOffset = 1;
        }
        else
        {
            uiIncrement = 1;
            uiDataFlowOffset = uiMaskWidth;
        }

        // Calculate the local starting position and the stepping direction
        NiIndex kLocalStart(0,0);
        NiPoint2 kCopyDirection(0,0);
        if (uiDirection & BORDER_LEFT)
        {
            kLocalStart.x = 0;
            kCopyDirection.x = 1;
        }
        else if (uiDirection & BORDER_RIGHT)
        {
            kLocalStart.x = uiMaskWidth - 2;
            kCopyDirection.x = -1;
        }
        if (uiDirection & BORDER_BOTTOM)
        {
            // NOTE: y = uiMaskWidth - 2 as an image origin is in the top left 
            // corner and we want to modify the bottom row
            kLocalStart.y = uiMaskWidth - 2;
            kCopyDirection.y = -1;
        }
        else if (uiDirection & BORDER_TOP)
        {
            // NOTE: y = 0 as an image origin is in the top left 
            // corner and we want to modify the top row
            kLocalStart.y = 0;
            kCopyDirection.y = 1;
            
        }

        NiUInt32 uiLocalOffset = 
            kLocalStart.y * uiMaskWidth + kLocalStart.x;
        NiUInt32 uiSum = 0;
        for (NiUInt32 uiValueIndex = 0; uiValueIndex < uiNumValues; 
            ++uiValueIndex)
        {
            NiUInt32 uiLocalIndex = 
                uiValueIndex * uiIncrement + uiLocalOffset;
            uiLocalIndex = uiLocalIndex * uiStride + uiPriority;

            // Border 1
            uiSum += pucLocalPixels[uiLocalIndex];
            // Border 2
            uiSum += pucLocalPixels[uiLocalIndex + uiDataFlowOffset * uiStride];
        }

        if (uiSum == 0)
            continue;
        
        // We are replacing this border so we need to make sure the surface 
        // is on the corresponding adjacent leaf
        NiUInt32 uiTemp;
        if(!pkAdjacentLeaf->GetSurfacePriority(uiSurfaceIndex, uiTemp))
        {
            return false;
        }
  
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::ClearAdjacentBorders(NiUInt32 uiPriority,
    NiUInt8 usValue)
{
    // Setup some basic data to allow simple looping through the cases
    const NiUInt32 uiMaxDirections = 8;
    const NiUInt32 auiDirection[uiMaxDirections] = {
        NiTerrainCell::BORDER_LEFT,
        NiTerrainCell::BORDER_RIGHT,
        NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_BOTTOM,
        };

    for (NiUInt32 uiIndex = 0; uiIndex < uiMaxDirections; ++uiIndex)
    {
        NiUInt32 uiDirection = auiDirection[uiIndex];
        NiTerrainCellLeaf* pkAdjacentLeaf = 
            (NiTerrainCellLeaf*)GetAdjacent(uiDirection);
        
        if (!pkAdjacentLeaf || pkAdjacentLeaf == this)
        {
            // There is no leaf in this direction
            continue;
        }

        // Fetch the location of this surface on the adjacent block
        NiUInt32 uiAdjacentPriority;
        if (!pkAdjacentLeaf->GetSurfacePriority(GetSurfaceIndex(uiPriority), 
            uiAdjacentPriority))
        {
            // The surface to be replaced is not on this leaf
            continue;
        }

            // Fetch the textures to be synchronized.
        NiTextureRegion kAdjacentRegion = 
            pkAdjacentLeaf->GetTextureRegion(TextureType::BLEND_MASK);
        NiSourceTexture* pkAdjacentTex = NiDynamicCast(NiSourceTexture, 
            kAdjacentRegion.GetTexture());

        // This function assumes that the leaves adjacent to it all share the 
        // same mask size. 
        EE_ASSERT(pkAdjacentTex);
        NiUInt32 uiMaskWidth = pkAdjacentTex->GetWidth();
        NiUInt32 uiStride = pkAdjacentTex->GetSourcePixelData()->GetPixelStride();

        // Obtain access to the pixel information
        NiUInt8* pucAdjacentPixels = 
            pkAdjacentTex->GetSourcePixelData()->GetPixels();
        EE_ASSERT(pucAdjacentPixels);

        // Calculate the number of values to copy.
        NiUInt32 uiNumValues = uiMaskWidth;
        if (!NiIsPowerOf2(uiDirection)) // Processing corners
            uiNumValues = 2;

        // Calculate the increment of the pixel position between values
        NiUInt32 uiIncrement;
        NiUInt32 uiDataFlowOffset;
        if (uiDirection & BORDER_LEFT || uiDirection & BORDER_RIGHT)
        {
            uiIncrement = uiMaskWidth;
            uiDataFlowOffset = 1;
        }
        else
        {
            uiIncrement = 1;
            uiDataFlowOffset = uiMaskWidth;
        }

        // Calculate the local starting position and the direction of the copy
        NiIndex kAdjacentStart;
        NiPoint2 kCopyDirection(0,0);
        if (uiDirection & BORDER_LEFT)
        {
            kAdjacentStart.x = 0;
            kCopyDirection.x = 1;
        }
        else if (uiDirection & BORDER_RIGHT)
        {
            kAdjacentStart.x = uiMaskWidth - 2;
            kCopyDirection.x = -1;
        }
        if (uiDirection & BORDER_BOTTOM)
        {
            // NOTE: y = uiMaskWidth - 2 as an image origin is in the top left 
            // corner and we want to modify the bottom row
            kAdjacentStart.y = uiMaskWidth - 2;
            kCopyDirection.y = -1;
        }
        else if (uiDirection & BORDER_TOP)
        {
            // NOTE: y = 0 as an image origin is in the top left 
            // corner and we want to modify the top row
            kAdjacentStart.y = 0;
            kCopyDirection.y = 1;
            
        }

        // Calculate the adjacent starting position        
        kAdjacentStart.x = kAdjacentStart.x + NiInt32(kCopyDirection.x * (
            uiMaskWidth - 2));
        kAdjacentStart.y = kAdjacentStart.y + NiInt32(kCopyDirection.y * 
            (uiMaskWidth - 2));

        // Prepare to update the mask usage data on the adjacent cell
        efd::UInt32 uiCellID = pkAdjacentLeaf->GetCellID();
        efd::UInt32 uiUsage = 
            pkAdjacentLeaf->m_pkContainingSector->GetLeafMaskUsage(uiCellID, uiAdjacentPriority);

        // Loop over the pixels and copy them
        NiUInt32 uiAdjacentOffset = 
            kAdjacentStart.y * uiMaskWidth + kAdjacentStart.x;
        for (NiUInt32 uiValueIndex = 0; uiValueIndex < uiNumValues; 
            ++uiValueIndex)
        {
            NiUInt32 uiAdjacentIndex = 
                uiValueIndex * uiIncrement + uiAdjacentOffset;
            uiAdjacentIndex = uiAdjacentIndex * uiStride + uiAdjacentPriority;

            // Border 1
            // ---------
            // Check if this pixel is no longer used or newly used
            bool bOldUsage = pucAdjacentPixels[uiAdjacentIndex] != 0;
            bool bNewUsage = usValue != 0;
            if (bOldUsage != bNewUsage)
            {
                // Make sure that we don't get underflow on the mask usage
                EE_ASSERT(uiUsage != 0 || bNewUsage);
                uiUsage += (bNewUsage) ? 1 : -1;
            }
            // Set the value
            pucAdjacentPixels[uiAdjacentIndex] = usValue;

            // Border 2
            // ---------
            // Check if this pixel is no longer used or newly used
            bOldUsage = pucAdjacentPixels[uiAdjacentIndex + uiDataFlowOffset * uiStride] != 0;
            bNewUsage = usValue != 0;
            if (bOldUsage != bNewUsage)
            {
                // Make sure that we don't get underflow on the mask usage
                EE_ASSERT(uiUsage != 0 || bNewUsage);
                uiUsage += (bNewUsage) ? 1 : -1;
            }
            // Set the value
            pucAdjacentPixels[uiAdjacentIndex + uiDataFlowOffset * uiStride] = 
                usValue;
        }

        // Update mask usage data on the adjacent cell
        pkAdjacentLeaf->m_pkContainingSector->SetLeafMaskUsage(uiCellID, uiAdjacentPriority, 
            uiUsage);

        pkAdjacentTex->GetSourcePixelData()->MarkAsChanged();
        pkAdjacentLeaf->MarkSurfaceMasksChanged();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::RemoveAllSurfaces()
{
    // Reset the priority arrays 
    for (NiUInt32 ui = 0; ui < MAX_NUM_SURFACES; ++ui)
    {
        ClearSurfaceMask(ui, 0);
        SyncAdjacentSurfaceMasks(ui, true);
        m_aiSurfaceIndex[ui] = -1;
    }

    m_uiNumUsedSurfaces = 0;
    MarkSurfaceMasksChanged(true);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::ReplaceSurface(const NiSurface* pkSurface,
    NiUInt32 uiPriority, bool bForceReplace)
{
    // Make sure the surface exist
    // Add surface returns the index if the surface already exists
    NiInt32 iSurfaceIndex = m_pkContainingSector->AddSurface(pkSurface);
    
    // Now check the we are not replacing a surface with the same surface
    if (iSurfaceIndex == GetSurfaceIndex(uiPriority))
        return false;

    return DoReplaceSurface(iSurfaceIndex, uiPriority, bForceReplace);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::ClearSurfaceMask(NiUInt32 uiPriority, 
    NiUInt8 uiClearValue)
{
    // Obtain access to the blend mask texture's data
    NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture,
        m_kBlendTextureRegion.GetTexture());
    EE_ASSERT(pkTexture);
    NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();

    // Fetch the range over which to modify this texture
    NiIndex kStartPoint = m_kBlendTextureRegion.GetStartPixelIndex();
    NiIndex kEndPoint = m_kBlendTextureRegion.GetEndPixelIndex();

    // Include the border pixels
    kStartPoint -= NiIndex(1,1);
    kEndPoint += NiIndex(1,1);

    // We now need to reinitialize the blend mask values
    for (NiUInt32 uiY = kStartPoint.y; uiY < kEndPoint.y; ++uiY)
    {
        for (NiUInt32 uiX = kStartPoint.x; uiX < kEndPoint.x; ++uiX)
        {
            DoSetPixelAt(pkPixelData, NiIndex(uiX, uiY), uiPriority, uiClearValue);
        }
    }

    // Make sure the mask usage is correct
    EE_ASSERT(m_pkContainingSector->GetLeafMaskUsage(GetCellID(), uiPriority) == 0);

    pkPixelData->MarkAsChanged();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::CopySurfaceMask(NiUInt32 uiPrioritySrc, 
    NiUInt32 uiPriorityDst)
{
    // Obtain access to the blend mask texture's data
    NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture,
        m_kBlendTextureRegion.GetTexture());
    EE_ASSERT(pkTexture);
    NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();

    // Fetch the range to iterate over for this mask
    NiIndex kStartPoint = m_kBlendTextureRegion.GetStartPixelIndex(); 
    NiIndex kEndPoint = m_kBlendTextureRegion.GetEndPixelIndex();

    // Include the border pixels
    kStartPoint -= NiIndex(1,1);
    kEndPoint += NiIndex(1,1);

    for (NiUInt32 uiY = kStartPoint.y; uiY < kEndPoint.y; ++uiY)
    {
        for (NiUInt32 uiX = kStartPoint.x; uiX < kEndPoint.x; ++uiX)
        {
            // Value of the last added surface for this point
            NiUInt8 ucValue = DoGetPixelAt(pkPixelData, NiIndex(uiX, uiY), uiPrioritySrc);

            // Copy that value into the new position
            DoSetPixelAt(pkPixelData, NiIndex(uiX, uiY), uiPriorityDst, ucValue);
        }
    }

    // Update the surface mask's painting data on the sector
//    efd::UInt32 uiCellID = GetCellID();
//    efd::UInt32 uiUsage = m_pkContainingSector->GetLeafMaskUsage(uiCellID, uiPrioritySrc);
//    EE_ASSERT(m_pkContainingSector->GetLeafMaskUsage(uiCellID, uiPriorityDst) == uiUsage);
//    m_pkContainingSector->SetLeafMaskUsage(uiCellID, uiPriorityDst, uiUsage);

    pkPixelData->MarkAsChanged();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::SyncAdjacentSurfaceMasks(NiUInt32 uiPriority, 
    bool bPushValues)
{
    // Setup some basic data to allow simple looping through the cases
    const NiUInt32 uiMaxDirections = 8;
    const NiUInt32 auiDirection[uiMaxDirections] = {
        NiTerrainCell::BORDER_LEFT,
        NiTerrainCell::BORDER_RIGHT,
        NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_LEFT | NiTerrainCell::BORDER_BOTTOM,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_TOP,
        NiTerrainCell::BORDER_RIGHT | NiTerrainCell::BORDER_BOTTOM,
        };

    for (NiUInt32 uiIndex = 0; uiIndex < uiMaxDirections; ++uiIndex)
    {
        NiUInt32 uiDirection = auiDirection[uiIndex];
        SyncAdjacentSurfaceMask(uiPriority, uiDirection, bPushValues);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::SyncAdjacentSurfaceMask(NiUInt32 uiPriority, 
    NiUInt32 uiDirection, bool bPushValues)
{
    EE_ASSERT(uiDirection);
    NiTerrainCellLeaf* pkAdjacentLeaf = (NiTerrainCellLeaf*)
        GetAdjacent(uiDirection);
    if (!pkAdjacentLeaf || pkAdjacentLeaf == this)
        return;

    // Fetch the location of this surface on the adjacent block
    NiUInt32 uiAdjacentPriority;
    if (!pkAdjacentLeaf->GetSurfacePriority(GetSurfaceIndex(uiPriority), 
        uiAdjacentPriority))
    {
        return;
    }

    // If we want to pull the values then ask the opposite side to do the 
    // pushing.
    if (!bPushValues)
    {
        pkAdjacentLeaf->SyncAdjacentSurfaceMask(uiAdjacentPriority, 
            GetOppositeBorder(uiDirection), true);
    }

    // Fetch the textures to be synchronized.
    NiTextureRegion kAdjacentRegion = 
        pkAdjacentLeaf->GetTextureRegion(TextureType::BLEND_MASK);
    NiSourceTexture* pkAdjacentTex = NiDynamicCast(NiSourceTexture, 
        kAdjacentRegion.GetTexture());
    NiSourceTexture* pkLocalTex = NiDynamicCast(NiSourceTexture,
        m_kBlendTextureRegion.GetTexture());

    // This function assumes that the leaves adjacent to it all share the 
    // same mask size. 
    EE_ASSERT(pkAdjacentTex && pkLocalTex);
    EE_ASSERT(pkAdjacentTex->GetWidth() == pkLocalTex->GetWidth());
    NiUInt32 uiMaskWidth = pkLocalTex->GetWidth();
    NiUInt32 uiStride = pkLocalTex->GetSourcePixelData()->GetPixelStride();

    // Obtain access to the pixel information
    NiUInt8* pucLocalPixels = 
        pkLocalTex->GetSourcePixelData()->GetPixels();
    NiUInt8* pucAdjacentPixels = 
        pkAdjacentTex->GetSourcePixelData()->GetPixels();
    EE_ASSERT(pucLocalPixels && pucAdjacentPixels);

    // Calculate the number of values to copy.
    NiUInt32 uiNumValues = uiMaskWidth;
    if (!NiIsPowerOf2(uiDirection)) // Processing corners
        uiNumValues = 2;

    // Calculate the increment of the pixel position between values
    NiUInt32 uiIncrement;
    NiUInt32 uiDataFlowOffset;
    if (uiDirection & BORDER_LEFT || uiDirection & BORDER_RIGHT)
    {
        uiIncrement = uiMaskWidth;
        uiDataFlowOffset = 1;
    }
    else
    {
        uiIncrement = 1;
        uiDataFlowOffset = uiMaskWidth;
    }

    // Calculate the local starting position and the direction of the copy
    NiIndex kLocalStart(0,0);
    NiPoint2 kCopyDirection(0,0);
    if (uiDirection & BORDER_LEFT)
    {
        kLocalStart.x = 0;
        kCopyDirection.x = 1;
    }
    else if (uiDirection & BORDER_RIGHT)
    {
        kLocalStart.x = uiMaskWidth - 2;
        kCopyDirection.x = -1;
    }
    if (uiDirection & BORDER_BOTTOM)
    {
        // NOTE: y = uiMaskWidth - 2 as an image origin is in the top left 
        // corner and we want to modify the bottom row
        kLocalStart.y = uiMaskWidth - 2;
        kCopyDirection.y = -1;
    }
    else if (uiDirection & BORDER_TOP)
    {
        // NOTE: y = 0 as an image origin is in the top left 
        // corner and we want to modify the top row
        kLocalStart.y = 0;
        kCopyDirection.y = 1;
    }

    // Calculate the adjacent starting position
    NiIndex kAdjacentStart;
    kAdjacentStart.x = kLocalStart.x + NiInt32(kCopyDirection.x * (uiMaskWidth - 2));
    kAdjacentStart.y = kLocalStart.y + NiInt32(kCopyDirection.y * (uiMaskWidth - 2));

    // Prepare to update the mask usage data on the adjacent cell
    efd::UInt32 uiCellID = pkAdjacentLeaf->GetCellID();
    efd::UInt32 uiUsage = 
        pkAdjacentLeaf->m_pkContainingSector->GetLeafMaskUsage(uiCellID, uiAdjacentPriority);

    // Loop over the pixels and copy them
    NiUInt32 uiLocalOffset = 
        kLocalStart.y * uiMaskWidth + kLocalStart.x;
    NiUInt32 uiAdjacentOffset = 
        kAdjacentStart.y * uiMaskWidth + kAdjacentStart.x;
    for (NiUInt32 uiValueIndex = 0; uiValueIndex < uiNumValues; 
        ++uiValueIndex)
    {
        NiUInt32 uiLocalIndex = 
            uiValueIndex * uiIncrement + uiLocalOffset;
        NiUInt32 uiAdjacentIndex = 
            uiValueIndex * uiIncrement + uiAdjacentOffset;
        uiLocalIndex = uiLocalIndex * uiStride + uiPriority;
        uiAdjacentIndex = uiAdjacentIndex * uiStride + uiAdjacentPriority;

        // Border 1
        // ---------
        // Check if this pixel is no longer used or newly used
        bool bOldUsage = pucAdjacentPixels[uiAdjacentIndex] != 0;
        bool bNewUsage = pucLocalPixels[uiLocalIndex] != 0;
        if (bOldUsage != bNewUsage)
        {
            // Make sure that we don't get underflow on the mask usage
            EE_ASSERT(uiUsage != 0 || bNewUsage);
            uiUsage += (bNewUsage) ? 1 : -1;
        }
        // Set the value
        pucAdjacentPixels[uiAdjacentIndex] = 
            pucLocalPixels[uiLocalIndex];

        // Border 2
        // ---------
        // Check if this pixel is no longer used or newly used
        bOldUsage = pucAdjacentPixels[uiAdjacentIndex + uiDataFlowOffset * uiStride] != 0;
        bNewUsage = pucLocalPixels[uiLocalIndex + uiDataFlowOffset * uiStride] != 0;
        if (bOldUsage != bNewUsage)
        {
            // Make sure that we don't get underflow on the mask usage
            EE_ASSERT(uiUsage != 0 || bNewUsage);
            uiUsage += (bNewUsage) ? 1 : -1;
        }
        // Set the value
        pucAdjacentPixels[uiAdjacentIndex + uiDataFlowOffset * uiStride] = 
            pucLocalPixels[uiLocalIndex + uiDataFlowOffset * uiStride];
    }

    // Update mask usage data on the adjacent cell
    pkAdjacentLeaf->m_pkContainingSector->SetLeafMaskUsage(uiCellID, uiAdjacentPriority, uiUsage);
    pkAdjacentLeaf->MarkSurfaceMasksChanged(true);

    pkAdjacentTex->GetSourcePixelData()->MarkAsChanged();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::GetSurfacePriority(const NiSurface* pkSurface, 
                                           NiUInt32& uiPriority) const
{
    NiUInt32 uiSurfaceIndex = m_pkContainingSector->GetSurfaceIndex(pkSurface);
    return GetSurfacePriority(uiSurfaceIndex, uiPriority);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::GetSurfacePriority(NiUInt32 uiSurfaceIndex, 
    NiUInt32& uiPriority) const
{
    for (uiPriority = 0; uiPriority < m_uiNumUsedSurfaces; ++uiPriority)
    {
        if (m_aiSurfaceIndex[uiPriority] == (NiInt32)uiSurfaceIndex)
        {
            return true;
        }
    }

    uiPriority = m_uiNumUsedSurfaces;
    return false;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainCellLeaf::GetSurfaceCount() const
{
    return m_uiNumUsedSurfaces;
}

//--------------------------------------------------------------------------------------------------
const NiSurface* NiTerrainCellLeaf::GetSurface(NiUInt32 uiPriority) const
{
    if (uiPriority >= GetSurfaceCount())
        return 0;

    return m_pkContainingSector->GetSurfaceAt(m_aiSurfaceIndex[uiPriority]);
}

//--------------------------------------------------------------------------------------------------
NiInt32 NiTerrainCellLeaf::GetSurfaceIndex(NiUInt32 uiPriority) const
{
    if (uiPriority >= GetSurfaceCount())
        return -1;

    return m_aiSurfaceIndex[uiPriority];
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::MarkSurfaceMasksChanged(bool bCalcUV)
{
    m_bSurfaceChanged = true;

    // Now, children only update if there the child is inheriting our mask.
    if (bCalcUV)
    {
        m_bSurfaceLayersChanged = true;
    }

    // Request an update
    if (!RequiresUpdate())
        RequestUpdate();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainCellLeaf::ProcessLOD()
{
    if (!NiTerrainCell::ProcessLOD())
        return false;

    AddToVisible();
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::Update()
{    
    // Perform a normal update of the cell
    NiTerrainCell::Update();     

    // if we have a mesh, make sure its bound is up to date.
    if (m_spMesh != NULL)
        m_spMesh->SetWorldBound(m_kBound);

    // Update the masks
    if (m_bSurfaceChanged || m_bSurfaceLayersChanged)
    {
        UpdateLeafShaderData();
        m_bSurfaceChanged = false;
        m_bSurfaceLayersChanged = false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::SetTextureRegion(NiPoint2 kOffset, float fScale, 
    NiTerrainCell::TextureType::Value eTexType)
{
    NiTerrainCell::SetTextureRegion(kOffset, fScale, eTexType);

    if (eTexType == TextureType::BLEND_MASK)
    {
        // Adjust the region specified to take into account a 1 pixel
        // border around the outside of the region. 
        // (1 pixel of shared border around entire texture)        
        // Assuming that the texture is square
        NiTexture* pkTexture = m_kBlendTextureRegion.GetTexture();
        EE_ASSERT(pkTexture->GetWidth() == pkTexture->GetHeight());

        float fBorderOffset = 1.0f / float(pkTexture->GetWidth());
        NiPoint2 kBorderOffset(fBorderOffset, fBorderOffset);
        float fBorderScale = 1.0f / ((1.0f / fScale) - (2.0f * fBorderOffset));

        // Apply these adjustments
        kOffset = kOffset + kBorderOffset;
        fScale = fBorderScale;
        m_kBlendTextureRegion.SetRegion(kOffset, fScale);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::SetTexture(NiTexture* pkTexture, 
    NiTerrainCell::TextureType::Value eTexType)
{
    NiTerrainCell::SetTexture(pkTexture, eTexType);

    if (eTexType == TextureType::BLEND_MASK)
    {
        // Release the original mask
        GetResourceManager()->ReleaseTexture(TextureType::BLEND_MASK, 
            m_kBlendTextureRegion.GetTexture());

        // Assign this texture
        m_kBlendTextureRegion.SetTexture(pkTexture);

        // Use the entire blend mask by default
        SetTextureRegion(NiPoint2::ZERO, 1.0f, eTexType);

        MarkSurfaceMasksChanged();
    }
}

//--------------------------------------------------------------------------------------------------
NiTexture* NiTerrainCellLeaf::GetTexture(NiTerrainCell::TextureType::Value eTexType)
{
    if (eTexType == TextureType::BLEND_MASK)
        return m_kBlendTextureRegion.GetTexture();
    else
        return NiTerrainCell::GetTexture(eTexType);
}

//--------------------------------------------------------------------------------------------------
const NiTextureRegion& NiTerrainCellLeaf::GetTextureRegion(
    NiTerrainCell::TextureType::Value eType) const
{
    if (eType == TextureType::BLEND_MASK)
        return m_kBlendTextureRegion;
    else
        return NiTerrainCell::GetTextureRegion(eType);
}

//--------------------------------------------------------------------------------------------------
NiTextureRegion& NiTerrainCellLeaf::GetTextureRegion(
    NiTerrainCell::TextureType::Value eType)
{
    if (eType == TextureType::BLEND_MASK)
        return m_kBlendTextureRegion;
    else
        return NiTerrainCell::GetTextureRegion(eType);
}

//--------------------------------------------------------------------------------------------------
NiUInt8 NiTerrainCellLeaf::GetPixelAt(NiTerrainCell::TextureType::Value eType, 
    NiIndex kCoordinates, NiUInt32 uiComponent) const
{
    NiSourceTexture* pkTexture;
    if (eType == TextureType::BLEND_MASK)
    {
        pkTexture = 
            NiDynamicCast(NiSourceTexture, m_kBlendTextureRegion.GetTexture());
    }
    else
    {
        pkTexture = NiDynamicCast(NiSourceTexture, 
            m_kLowDetailTextureRegion.GetTexture());
    }

    if (!pkTexture)
        return 0;

    NiPixelData* pkTextureData = pkTexture->GetSourcePixelData();
    return DoGetPixelAt(pkTextureData, kCoordinates, uiComponent);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::SetPixelAt(NiTerrainCell::TextureType::Value eType, 
    NiIndex kCoordinates, NiUInt32 uiComponent, NiUInt8 ucNewValue)
{
    // Check if this texture exists
    NiSourceTexture* pkTexture = NULL;
    if (eType == TextureType::BLEND_MASK)
    {
        pkTexture = 
            NiDynamicCast(NiSourceTexture, m_kBlendTextureRegion.GetTexture());
    }
    else
    {
        EE_FAIL("Only blend mask pixels may be changed");
    }

    if (!pkTexture)
        return;

    // Extract the format data from the texture
    NiPixelData* pkTextureData = pkTexture->GetSourcePixelData();
    NiUInt32 uiWidth = pkTextureData->GetWidth();

    // Check to see if bordering blocks need their border pixels adjusted
    // NOTE: This code assumes that all blend masks are the same dimensions.
    NiUInt32 uiMinBorder = 1;
    NiUInt32 uiMaxBorder = uiWidth - 2;
    NiIndex kAdjacentCoords = kCoordinates;
    NiUInt32 kHorizontalBorder = BORDER_NONE;
    NiUInt32 kVerticalBorder = BORDER_NONE;

    // Check the blocks neighboring horizontally
    if (kCoordinates.x == uiMinBorder)
    {
        kHorizontalBorder |= BORDER_LEFT;
        kAdjacentCoords.x = uiMaxBorder + 1;
    }
    else if (kCoordinates.x == uiMaxBorder)
    {
        kHorizontalBorder |= BORDER_RIGHT;
        kAdjacentCoords.x = 0;
    }
    // Check the blocks neighboring Vertically
    if (kCoordinates.y == uiMinBorder)
    {
        kVerticalBorder |= BORDER_BOTTOM;
        kAdjacentCoords.y = uiMaxBorder + 1;
    }
    else if (kCoordinates.y == uiMaxBorder)
    {
        kVerticalBorder |= BORDER_TOP;
        kAdjacentCoords.y = 0;
    }

    if (kVerticalBorder || kHorizontalBorder)
    {
        // Fetch the relevant neighboring cells
        NiTerrainCellLeaf* pkAdjacentHorizontalCell = NULL;
        if (kHorizontalBorder)
            pkAdjacentHorizontalCell = NiDynamicCast(NiTerrainCellLeaf, 
                GetAdjacent(kHorizontalBorder));

        NiTerrainCellLeaf* pkAdjacentVerticalCell = NULL;
        if (kVerticalBorder)
            pkAdjacentVerticalCell = NiDynamicCast(NiTerrainCellLeaf, 
                GetAdjacent(kVerticalBorder));

        NiTerrainCellLeaf* pkAdjacentCornerCell = NULL;
        if (kVerticalBorder && kHorizontalBorder)
            pkAdjacentCornerCell = NiDynamicCast(NiTerrainCellLeaf, 
                GetAdjacent(kVerticalBorder | kHorizontalBorder));

        // Find the channels that this surface exists in on the other blocks
        // and test if painting this pixel is a valid operation
        bool bAllowPainting = true;
        const NiSurface* pkSurface = GetSurface(uiComponent);

        NiUInt32 uiHorizontalCellChannel = 0;
        if (pkAdjacentHorizontalCell && 
            !pkAdjacentHorizontalCell->GetSurfacePriority(pkSurface, 
                uiHorizontalCellChannel))
        {
            bAllowPainting = false;
        }

        NiUInt32 uiVerticalCellChannel = 0;
        if (pkAdjacentVerticalCell && 
            !pkAdjacentVerticalCell->GetSurfacePriority(pkSurface, 
                uiVerticalCellChannel))
        {
            bAllowPainting = false;
        }

        NiUInt32 uiCornerCellChannel = 0;
        if (pkAdjacentCornerCell && 
            !pkAdjacentCornerCell->GetSurfacePriority(pkSurface, 
                uiCornerCellChannel))
        {
            bAllowPainting = false;
        }

        if (bAllowPainting)
        {
            // Paint the relevant blend mask pixels
            if (pkAdjacentHorizontalCell)
            {
                pkAdjacentHorizontalCell->SetPixelAt(eType, 
                    NiIndex(kAdjacentCoords.x, kCoordinates.y), 
                    uiHorizontalCellChannel, ucNewValue);
            }
            if (pkAdjacentVerticalCell)
            {
                pkAdjacentVerticalCell->SetPixelAt(eType,  
                    NiIndex(kCoordinates.x, kAdjacentCoords.y), 
                    uiVerticalCellChannel, ucNewValue);
            }
            if (pkAdjacentCornerCell)
            {
                pkAdjacentCornerCell->SetPixelAt(eType,  
                    NiIndex(kAdjacentCoords.x, kAdjacentCoords.y), 
                    uiCornerCellChannel, ucNewValue);
            }
        }
        else
        {
            ucNewValue = 0;   
        }
    }
    
    // Apply the changes to this cell
    DoSetPixelAt(pkTextureData, kCoordinates, uiComponent, ucNewValue);
    pkTextureData->MarkAsChanged();
    MarkSurfaceMasksChanged();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::DoSetPixelAt(NiPixelData* pkPixelData, 
    NiIndex kCoordinates, NiUInt32 uiComponent, NiUInt8 ucNewValue)
{
    EE_ASSERT(pkPixelData);
    NiUInt32 uiStride = pkPixelData->GetPixelStride();
    NiUInt32 uiWidth = pkPixelData->GetWidth();

    EE_ASSERT(kCoordinates.x < pkPixelData->GetWidth());
    EE_ASSERT(kCoordinates.y < pkPixelData->GetHeight());
    EE_ASSERT(uiComponent < pkPixelData->GetPixelFormat().GetNumComponents());

    // Calculate the pixel index
    NiUInt32 uiPixelIndex = NiUInt32(
        ((kCoordinates.y) * uiWidth + kCoordinates.x) * uiStride + uiComponent);
    unsigned char* pucPixels = pkPixelData->GetPixels();

    // Update the surface mask's painting data on the sector
    efd::UInt32 uiCellID = GetCellID();
    efd::UInt32 uiUsage = m_pkContainingSector->GetLeafMaskUsage(uiCellID, uiComponent);
    
    // Check if this pixel is no longer used or newly used
    bool bOldUsage = pucPixels[uiPixelIndex] != 0;
    bool bNewUsage = ucNewValue != 0;
    if (bOldUsage != bNewUsage)
    {
        // Make sure that we don't get underflow on the mask usage
        EE_ASSERT(uiUsage != 0 || bNewUsage);
        uiUsage += (bNewUsage) ? 1 : -1;
    }
    m_pkContainingSector->SetLeafMaskUsage(uiCellID, uiComponent, uiUsage);

    pucPixels[uiPixelIndex] = ucNewValue;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::GetTerrainSpaceRegion(NiRect<efd::SInt32>& kRegion)
{
    NiTerrain* pkTerrain = m_pkContainingSector->GetTerrain();
    EE_ASSERT(pkTerrain);

    // Work out the region over which this cell lives
    NiIndex kBottomLeft;
    GetBottomLeftIndex(kBottomLeft);

    efd::UInt32 uiCellSize = GetCellSize();
    efd::UInt32 uiSectorSize = pkTerrain->GetCalcSectorSize() - 1;
    efd::SInt16 sSectorX, sSectorY;
    m_pkContainingSector->GetSectorIndex(sSectorX, sSectorY);

    kRegion.m_left = kBottomLeft.x + (efd::SInt32)sSectorX * uiSectorSize;
    kRegion.m_bottom = kBottomLeft.y + (efd::SInt32)sSectorY * uiSectorSize;
    kRegion.m_right = kRegion.m_left + uiCellSize;
    kRegion.m_top = kRegion.m_bottom + uiCellSize;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::TakeSnapshot(NiTerrainDataSnapshot* pkSnapshot, efd::UInt32 eDataSet)
{
    NiTerrain* pkTerrain = m_pkContainingSector->GetTerrain();
    EE_ASSERT(pkTerrain);

    // Work out the region over which this cell lives
    NiRect<efd::SInt32> kWorldSpaceRegion;
    GetTerrainSpaceRegion(kWorldSpaceRegion);
    
    // Fetch the cell's snapshot
    NiTerrainDataSnapshot::CellSnapshot* pkCellShot = pkSnapshot->GetCellSnapshot(
        NiTerrainSector::CellID(m_pkContainingSector->GetSectorID(), GetCellID()));
    EE_ASSERT(pkCellShot);

    // Check if there is already data recorded for this cell
    if (pkCellShot->ContainsData(eDataSet))
        return;

    switch (eDataSet)
    {
    case NiTerrainDataSnapshot::BufferType::SURFACE_MASK:
        {
            for (efd::UInt32 uiSlot = 0; uiSlot < m_uiNumUsedSurfaces; ++uiSlot)
            {
                // Initialize the buffer
                SurfaceMaskBuffer* pkMask = EE_NEW SurfaceMaskBuffer();
                efd::SInt32 iSurfaceIndex = GetSurfaceIndex(uiSlot);
                // Populate the buffer
                pkTerrain->GetSurfaceMask(iSurfaceIndex, kWorldSpaceRegion, pkMask);
                // Store the snapshot
                pkCellShot->AppendData(iSurfaceIndex, pkMask);
            }
        }
        break;
    case NiTerrainDataSnapshot::BufferType::HEIGHTMAP:
        {
            // Initialize the buffer
            HeightMapBuffer* pkHeightmap = EE_NEW HeightMapBuffer();
            // Populate the buffer
            pkTerrain->GetHeightMap(kWorldSpaceRegion, pkHeightmap);
            // Store the snapshot
            pkCellShot->AppendData(pkHeightmap);
        }
        break;
    }
    // Mark the cell shot as containing the data
    pkCellShot->MarkContainsData(eDataSet);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainCellLeaf::UpdateLeafShaderData() 
{
    // We reached here due to a configuration change of some sort. 
    // Flag this leaf as requiring a low detail redraw
    NiTerrainSector* pkSector = GetContainingSector();
    pkSector->QueueCellForLowDetailUpdate(this);

    // Mark the material as requiring an update
    if (m_spMesh)
        m_spMesh->SetMaterialNeedsUpdate(true);

    EE_ASSERT(m_uiNumUsedSurfaces <= MAX_NUM_SURFACES);
    
    if (m_bSurfaceLayersChanged || m_bSurfaceChanged)
    {  
        EE_ASSERT(m_spMesh->GetExtraDataSize());

        // The repetition or scale of a surface has changed
        NiPoint2 kBlendOffset = m_kBlendTextureRegion.GetTextureOffset();
        float fLeafScale = m_kBlendTextureRegion.GetTextureScale();

        m_kShaderData.m_kBlendMapOffset = kBlendOffset;
        m_kShaderData.m_kBlendMapScale = NiPoint2(1.0f / fLeafScale, 1.0f / fLeafScale);

        float afLayerScales[MAX_NUM_SURFACES] = {1.0f, 1.0f, 1.0f, 1.0f };
        float afParallaxStrength[MAX_NUM_SURFACES] = {0.05f, 0.05f, 0.05f, 0.05f};
        float afSpecularIntensity[MAX_NUM_SURFACES] = {0.0f, 0.0f, 0.0f, 0.0f};
        float afSpecularPower[MAX_NUM_SURFACES] = {0.0f, 0.0f, 0.0f, 0.0f};
        float afDetailTextureScale[MAX_NUM_SURFACES] = {1.0f, 1.0f, 1.0f, 1.0f};
        float afDistribStrength[MAX_NUM_SURFACES] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (NiUInt32 ui = 0; ui < m_uiNumUsedSurfaces; ui++)
        {
            const NiSurface* pkSurface = GetSurface(ui);
            EE_ASSERT(pkSurface);
            afLayerScales[ui] = pkSurface->GetTextureTiling();
            afParallaxStrength[ui] = pkSurface->GetParallaxStrength();
            afSpecularIntensity[ui] = pkSurface->GetSpecularIntensity();
            afSpecularPower[ui] = pkSurface->GetSpecularPower();
            afDetailTextureScale[ui] = pkSurface->GetDetailTextureTiling();
            afDistribStrength[ui] = pkSurface->GetDistributionMaskStrength();
        }

        m_kShaderData.m_kLayerScale.SetX(afLayerScales[0]);
        m_kShaderData.m_kLayerScale.SetY(afLayerScales[1]);
        m_kShaderData.m_kLayerScale.SetZ(afLayerScales[2]);
        m_kShaderData.m_kLayerScale.SetW(afLayerScales[3]);

        m_kShaderData.m_kParallaxStrength.SetX(afParallaxStrength[0]);
        m_kShaderData.m_kParallaxStrength.SetY(afParallaxStrength[1]);
        m_kShaderData.m_kParallaxStrength.SetZ(afParallaxStrength[2]);
        m_kShaderData.m_kParallaxStrength.SetW(afParallaxStrength[3]);

        m_kShaderData.m_kSpecularIntensity.SetX(afSpecularIntensity[0]);
        m_kShaderData.m_kSpecularIntensity.SetY(afSpecularIntensity[1]);
        m_kShaderData.m_kSpecularIntensity.SetZ(afSpecularIntensity[2]);
        m_kShaderData.m_kSpecularIntensity.SetW(afSpecularIntensity[3]);

        m_kShaderData.m_kSpecularPower.SetX(afSpecularPower[0]);
        m_kShaderData.m_kSpecularPower.SetY(afSpecularPower[1]);
        m_kShaderData.m_kSpecularPower.SetZ(afSpecularPower[2]);
        m_kShaderData.m_kSpecularPower.SetW(afSpecularPower[3]);

        m_kShaderData.m_kDetailTextureScale.SetX(afDetailTextureScale[0]);
        m_kShaderData.m_kDetailTextureScale.SetY(afDetailTextureScale[1]);
        m_kShaderData.m_kDetailTextureScale.SetZ(afDetailTextureScale[2]);
        m_kShaderData.m_kDetailTextureScale.SetW(afDetailTextureScale[3]);

        m_kShaderData.m_kDistributionRamp.SetX(afDistribStrength[0]);
        m_kShaderData.m_kDistributionRamp.SetY(afDistribStrength[1]);
        m_kShaderData.m_kDistributionRamp.SetZ(afDistribStrength[2]);
        m_kShaderData.m_kDistributionRamp.SetW(afDistribStrength[3]);

        m_kShaderData.UpdateShaderData(m_spMesh);
    }

    if (m_bSurfaceChanged)
    {
        NiTexturingProperty* pkProperty = GetTexturingProperty();
        if (pkProperty)
        {
            // Clean out all the shader maps:
            efd::UInt32 uiNumMaps = pkProperty->GetShaderArrayCount();
            for (efd::UInt32 uiMapID = 0; uiMapID < uiNumMaps; ++uiMapID)
                pkProperty->SetShaderMap(uiMapID, NULL);
        }

        // Blend map is the first shader map entry.
        NiSourceTexture* pkBlendTexture = NiDynamicCast(
            NiSourceTexture, m_kBlendTextureRegion.GetTexture());
        if (!pkBlendTexture)
            return;

        pkProperty->SetShaderMap(0, NiNew NiTexturingProperty::ShaderMap(pkBlendTexture,
            0, NiTexturingProperty::CLAMP_S_CLAMP_T, NiTexturingProperty::FILTER_BILERP,
            NiTerrainMaterial::BLEND_MAP));

        // Add shader maps for each layer. (and accumulate their caps)
        NiInt32 aiLayerCaps[5] = {(NiInt32)m_uiNumUsedSurfaces, 0, 0, 0, 0};
        for (NiUInt32 ui = 0; ui < m_uiNumUsedSurfaces; ui++)
        {
            const NiSurface* pkCurrSurface = GetSurface(ui);
            EE_ASSERT(pkCurrSurface);

            pkProperty = pkCurrSurface->MergeSurfaces(pkProperty, pkCurrSurface);
            aiLayerCaps[ui + 1] = (NiInt32)pkCurrSurface->GetSurfaceCaps();
        }

        NiIntegersExtraData* pkConfigData = (pkProperty->GetExtraDataSize() > 0) ? 
            NiDynamicCast(NiIntegersExtraData, pkProperty->GetExtraDataAt(0)) : NULL;
        if (!pkConfigData)
        {
            pkConfigData = NiNew NiIntegersExtraData(5, aiLayerCaps);
            pkProperty->AddExtraData(pkConfigData);
        }
        else
        {
            pkConfigData->SetArray(5, aiLayerCaps);
        }
        
        m_spMesh->UpdateProperties();
    }
}

//--------------------------------------------------------------------------------------------------
