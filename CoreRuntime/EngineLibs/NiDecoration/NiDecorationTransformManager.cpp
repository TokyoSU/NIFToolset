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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiDecorationPCH.h"

#include "NiDecorationTransformManager.h"
#include <algorithm>

#include <NiAVObject.h>
#include "NiDecorationGenerator.h"

// Included for debugging purposes
#include "NiDecorationField.h"

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::NiDecorationTransformManager()
    : m_uiMaxCellCount(0)
    , m_uiInstancesPerCell(0)
    , m_uiTransformCount(0)
    , m_pkTransforms(NULL)
    , m_pkInstanceStream(NULL)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::NiDecorationTransformManager(
    NiDataStream* pkInstanceStream, NiUInt32 uiMaxCellCount, NiUInt32 uiInstancesPerCell)
    : m_uiMaxCellCount(0)
    , m_uiInstancesPerCell(0)
    , m_uiTransformCount(0)
    , m_pkTransforms(NULL)
    , m_pkInstanceStream(NULL)
{
    SetInstanceStream(pkInstanceStream, uiMaxCellCount, uiInstancesPerCell);
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::SetInstanceStream(NiDataStream* pkInstanceStream, 
    NiUInt32 uiMaxCellCount, NiUInt32 uiInstancesPerCell)
{
    // Make sure all the regions are empty or all cells within them are released
    for (RegionIdToStreamRegionMap::iterator poolIter = m_kStreamRegions.begin();
        poolIter != m_kStreamRegions.end();
        poolIter++)
    {
        Region& kRegion = poolIter->second;

        EE_ASSERT(kRegion.m_range == kRegion.m_kReleasedCells.size());

        // Free any remaining released cells
        for (CellList::iterator cellIter = kRegion.m_kReleasedCells.begin();
            cellIter != kRegion.m_kReleasedCells.end();
            cellIter++)
        {
            NiDecorationCell* pkReleasedCell = *cellIter;
            EE_ASSERT(pkReleasedCell->m_uiIndexInRegion < kRegion.m_range);
            m_kCells.at(pkReleasedCell->m_uiIndexInRegion + kRegion.m_start) = NULL;

            // Invalidate the freed cell
            pkReleasedCell->m_uiIndexInRegion = UINT_MAX;
            pkReleasedCell->m_pkRegionID = NULL;
        }

        // Empty the region
        kRegion.m_start = 0;
        kRegion.m_range = 0;

        // Add the freed cells to the unused array
        m_kUnusedCells.insert(m_kUnusedCells.end(), 
            kRegion.m_kReleasedCells.begin(), kRegion.m_kReleasedCells.end());
        kRegion.m_kReleasedCells.clear();
    }

    m_pkInstanceStream = pkInstanceStream;
    m_uiTransformCount = uiMaxCellCount * uiInstancesPerCell;
    m_uiMaxCellCount = uiMaxCellCount;
    m_uiInstancesPerCell = uiInstancesPerCell;

    // Release old data
    EE_FREE(m_pkTransforms);
    m_pkTransforms = NULL;

    // Initialize only if we have a valid stream
    if (pkInstanceStream != NULL)
    {
        // Allocate new data
        if (m_uiTransformCount > 0)
            m_pkTransforms = EE_ALLOC(NiTransform, m_uiTransformCount);
        else
            m_pkTransforms = NULL;

        // Make sure that the instance stream is large enough to cope with all of our instances.
        EE_ASSERT(m_pkInstanceStream->GetTotalCount() >= m_uiTransformCount);

        // Avoid any nasty allocation surprises
        m_kCells.resize(uiMaxCellCount);

        // Assign to our regions meshes
        pkInstanceStream->RemoveAllRegions();
        for (RegionIDList::iterator poolIter = m_kRegions.begin();
            poolIter != m_kRegions.end();
            poolIter++)
        {
            Region& kRegion = GetRegion(*poolIter);
            NiDecorationMeshInfo* pkMeshInfo = GetMeshInfo(*poolIter);
            NiMesh* pkBaseMesh = pkMeshInfo->GetMesh();

            // Add a data stream region
            NiDataStream::Region kStreamRegion(kRegion.m_start * GetInstancesPerCell(), 0);
            pkInstanceStream->AddRegion(kStreamRegion);
            kRegion.m_uiDataStreamRegionIndex = pkInstanceStream->GetRegionCount() - 1;

            // Assign this data stream to the mesh
            NiDataStreamRef* pkTransStreamRef = pkBaseMesh->GetBaseInstanceStream();
            pkTransStreamRef->SetDataStream(m_pkInstanceStream);
            pkTransStreamRef->BindRegionToSubmesh(0, kRegion.m_uiDataStreamRegionIndex);
        }
    }
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::~NiDecorationTransformManager()
{
    for (CellVector::iterator iter = m_kCells.begin(); iter != m_kCells.end(); iter++)
        EE_DELETE(*iter);

    for (CellList::iterator iter = m_kUnusedCells.begin(); iter != m_kUnusedCells.end(); iter++)
        EE_DELETE(*iter);

    EE_FREE(m_pkTransforms);
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::CreateInstanceRegion(
    RegionID kRegionID, 
    NiDecorationTransformProcessor* pkProcessor)
{
    EE_ASSERT(m_kStreamRegions.find(kRegionID) == m_kStreamRegions.end());

    // Find the start of the region
    NiUInt32 uiRegionStart = 0;
    if (m_kRegions.size())
    {
        RegionID kLastID = m_kRegions.back();
        Region& lastRegion = GetRegion(kLastID);
        uiRegionStart = lastRegion.m_start + lastRegion.m_range;
    }

    // Create a new region at the end
    Region& region = m_kStreamRegions[kRegionID];
    region.m_pkProcessor = pkProcessor;
    region.m_start = uiRegionStart;
    region.m_range = 0;

    // Add knowledge of this new pool
    m_kRegions.push_back(kRegionID);

    NiMesh* pkBaseMesh = GetMeshInfo(kRegionID)->GetMesh();
    if (pkBaseMesh->GetInstanced())
    {
        // Create a new instance stream region
        NiDataStream::Region kRegion(region.m_start * GetInstancesPerCell(), 0);
        m_pkInstanceStream->AddRegion(kRegion);

        region.m_uiDataStreamRegionIndex = m_pkInstanceStream->GetRegionCount() - 1;

        // Create stream reference
        NiDataStreamRef* pkTransStreamRef = pkBaseMesh->GetBaseInstanceStream();

        // TODO: Make sure that the semantics and element refs are the same
        pkTransStreamRef->SetDataStream(m_pkInstanceStream);

        // Bind the region and the mesh
        pkTransStreamRef->BindRegionToSubmesh(0, region.m_uiDataStreamRegionIndex);
    }
    else
    {
        // TODO: support non instanced meshes...
        EE_FAIL("Non-GPU instanced meshes are currently unsupported");
    }

    AssertCellConsistency();
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::ReleaseInstanceRegion(RegionID kRegionID)
{
    // Make sure the region has no allocated cells
    Region& releasedRegion = GetRegion(kRegionID);
    EE_ASSERT(releasedRegion.m_range == 0);

    {
        NiMesh* pkBaseMesh = GetMeshInfo(kRegionID)->GetMesh();
        if (pkBaseMesh->GetInstanced())
        {
            m_pkInstanceStream->RemoveRegion(releasedRegion.m_uiDataStreamRegionIndex);
        }
        else
        {
            // TODO: support non instanced meshes...
            EE_FAIL("Non-GPU instanced meshes are currently unsupported");
        }
    }

    // Decrement the region ID's of pools that are after this one in the stream
    if (kRegionID != m_kRegions.back())
    {
        RegionIDList::iterator poolIter = m_kRegions.find(kRegionID);
        NIASSERT(poolIter != m_kRegions.end());
        for (++poolIter;
            poolIter != m_kRegions.end();
            poolIter++)
        {
            Region& kNextRegion = GetRegion(*poolIter);
            kNextRegion.m_uiDataStreamRegionIndex--;

            NiMesh* pkNextBaseMesh = GetMeshInfo(*poolIter)->GetMesh();
            NiDataStreamRef* pkTransStreamRef = pkNextBaseMesh->GetBaseInstanceStream();
            pkTransStreamRef->BindRegionToSubmesh(0, kNextRegion.m_uiDataStreamRegionIndex);

#if defined(EE_ASSERTS_ARE_ENABLED)
            const NiDataStreamRef* pkInstanceStreamRef = 
                pkNextBaseMesh->FindStreamRef(NiCommonSemantics::INSTANCETRANSFORMS());

            EE_ASSERT(pkInstanceStreamRef == pkTransStreamRef);
#endif
        }
    }

    m_kStreamRegions.erase(kRegionID);
    m_kRegions.remove(kRegionID);

    AssertCellConsistency();
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::RequestCells(
    RegionID kRegionID, 
    NiUInt32 uiCellIndexX, 
    NiUInt32 uiCellIndexY)
{
    Region& kRegion = GetRegion(kRegionID);
    kRegion.m_kRequestedCells.push_back(CellIndex(uiCellIndexX, uiCellIndexY));
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::ReleaseCell(
    RegionID kRegionID, 
    NiDecorationCell* pkCell)
{
    Region& kRegion = GetRegion(kRegionID);

    // Make sure this cell has not been released
    EE_ASSERT(kRegion.m_kReleasedCells.find(pkCell) == kRegion.m_kReleasedCells.end());

    kRegion.m_kReleasedCells.push_back(pkCell);
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::ClearRegion(RegionID kRegionID)
{
    Region& kRegion = GetRegion(kRegionID);
    
    // Clear the released cells list, and re-add all of this regions cells so we don't have to
    // worry about duplication
    kRegion.m_kReleasedCells.clear();

    for (NiUInt32 ui = 0; ui < kRegion.m_range; ui++)
    {
        kRegion.m_kReleasedCells.push_back(m_kCells[kRegion.m_start + ui]);
    }

    // Cancel any cell requests
    kRegion.m_kRequestedCells.clear();

    // Cancel any uploads
    CellList kCellsToRemove;
    for (CellSet::iterator cellIter = m_kCellsToUpload.begin();
        cellIter != m_kCellsToUpload.end();
        cellIter++)
    {
        if ((*cellIter)->m_pkRegionID == kRegionID)
        {
            kCellsToRemove.push_back(*cellIter);
        }
    }

    for (CellList::iterator cellIter = kCellsToRemove.begin();
        cellIter != kCellsToRemove.end();
        cellIter++)
    {
        m_kCellsToUpload.erase(*cellIter);
    }
}

//------------------------------------------------------------------------------------------------
struct CellSortCompare
{
    bool operator()(NiDecorationCell* pkLeft, NiDecorationCell* pkRight)
    {
        return pkLeft->m_uiIndexInRegion < pkRight->m_uiIndexInRegion;
    }
};

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::ProcessRequests()
{
    /*
        1) Create new cells using 'released' cells
        2) Compact regions, putting released cells at the end
        3) Remove gaps between the regions
        4) Process new cell requests
        5) Trim the cell array, removing any leftover freed cells
        6) Update the mesh region sizes to match the actual region sizes
     */

    // Can only process requests if we have a transform stream
    if (GetTransformStream() == NULL)
        return;

    // Variables used within the loops
    NiUInt32 uiInstancesPerCell = GetInstancesPerCell();
    const size_t stTransformSize = sizeof(NiTransform);

    AssertCellConsistency();

    // Starting with the first region, attempt to create new cells.
    for (RegionIDList::iterator poolIter = m_kRegions.begin();
        poolIter != m_kRegions.end();
        poolIter++)
    {
        Region& kRegion = GetRegion(*poolIter);

        // Sort the freed cells according to position in the region
        kRegion.m_kReleasedCells.sort(CellSortCompare());

        // Lock the data stream region
        
        NiTransform* pkRegionStream = LockRegion(kRegion, NiDataStream::LOCK_WRITE);
        
        // New requests
        for (CellIndexList::iterator newCellIter = kRegion.m_kRequestedCells.begin();
            newCellIter != kRegion.m_kRequestedCells.end();)
        {
            // Can we use a released cell?
            if (kRegion.m_kReleasedCells.size() == 0)
                break;

            NiDecorationCell* pkCell = kRegion.m_kReleasedCells.front();

            // Initialize the cell
            NIASSERT(pkCell->m_pkRegionID == *poolIter);
            pkCell->m_uiIndexX = newCellIter->m_uiX;
            pkCell->m_uiIndexY = newCellIter->m_uiY;

            // Generate the transforms
            NIASSERT(pkCell->m_uiIndexInRegion < kRegion.m_range);
            CellRequestGenerationResult eGenerationResult = 
                kRegion.m_pkProcessor->ProcessCellTransforms(pkCell,
                pkRegionStream + uiInstancesPerCell * pkCell->m_uiIndexInRegion,
                m_uiInstancesPerCell,
                false);

            if (eGenerationResult == GR_GENERATED_VALID_TRANSFORMS)
            {
                AssertCellDebugTransforms(pkCell);

                kRegion.m_kReleasedCells.pop_front();
            }
            else
            {
                // Acquired from the freed cells array, do nothing.
            }

            // Do we need to upload transforms?
            if (NiDecorationTransformProcessor::RequiresUpload(eGenerationResult))
            {
                m_kCellsToUpload.insert(pkCell);
            }

            // The cell has now been processed, so it can be forgotten about
            newCellIter = kRegion.m_kRequestedCells.erase(newCellIter);
        }

        Unlock(NiDataStream::LOCK_WRITE);
    }

    AssertCellConsistency();

    // Now compact each region, so that the free cells are at the end
    for (RegionIDList::iterator poolIter = m_kRegions.begin();
        poolIter != m_kRegions.end();
        poolIter++)
    {
        Region& kRegion = GetRegion(*poolIter);

        // Lock the data stream region
        NiTransform* pkRegionStream = LockRegion(kRegion, NiDataStream::LOCK_WRITE);
        NIASSERT(pkRegionStream);

        NIASSERT(kRegion.m_range >= kRegion.m_kReleasedCells.size());

        // Start at the last cell, working our way back. This list must be sorted according to
        // position in the cell array!
        for (CellList::reverse_iterator cellIter = kRegion.m_kReleasedCells.rbegin();
            cellIter != kRegion.m_kReleasedCells.rend();
            cellIter++)
        {
            NiUInt32 uiLastCellIndexInRegion = kRegion.m_range - 1;
            NiUInt32 uiLastCellIndex = kRegion.m_start + uiLastCellIndexInRegion;
            NiDecorationCell* pkLastCell = m_kCells[uiLastCellIndex];
            NiDecorationCell* pkReleasedCell = *cellIter;

            NIASSERT(pkLastCell->m_pkRegionID == pkReleasedCell->m_pkRegionID);
            NIASSERT(pkLastCell->m_pkRegionID == *poolIter);

            // Are we already at the end?
            if (pkLastCell != pkReleasedCell)
            {
                // We need to swap with the end.
                NiUInt32 uiFreedCellIndexInRegion = pkReleasedCell->m_uiIndexInRegion;
                NIASSERT(uiFreedCellIndexInRegion < kRegion.m_range);
                size_t stRemainingStream = 
                    (kRegion.m_range - uiFreedCellIndexInRegion) * uiInstancesPerCell * stTransformSize;

                NiTransform* pkDest = 
                    pkRegionStream + uiFreedCellIndexInRegion * uiInstancesPerCell;
                NiTransform* pkSource = 
                    pkRegionStream + uiLastCellIndexInRegion * uiInstancesPerCell;

                Memmove(pkDest, 
                    stRemainingStream, 
                    pkSource, 
                    uiInstancesPerCell * stTransformSize);

                // Change the cell details
                pkLastCell->m_uiIndexInRegion = uiFreedCellIndexInRegion;
                m_kCells.at(kRegion.m_start + uiFreedCellIndexInRegion) = pkLastCell;
                m_kCells.at(uiLastCellIndex) = pkReleasedCell;

                AssertCellDebugTransforms(pkLastCell);

                // Mark the cell for upload.
                m_kCellsToUpload.insert(pkLastCell);
            }

            // Invalidate the freed cell
            pkReleasedCell->m_uiIndexInRegion = UINT_MAX;
            pkReleasedCell->m_pkRegionID = NULL;

            // Reduce the size of the region
            kRegion.m_range--;
        }

        // Unlock the stream
        Unlock(NiDataStream::LOCK_WRITE);
    }

    AssertCellConsistency();

    // Remove any gaps between the regions

    // Lock the entire data stream region
    NiTransform* pkTransformStream = LockStream(NiDataStream::LOCK_WRITE);

    NiUInt32 uiCount = 0;
    NiUInt32 uiMoveOffset;
    NiUInt32 uiMoveCount;
    for (RegionIDList::iterator poolIter = m_kRegions.begin();
        poolIter != m_kRegions.end();
        poolIter++)
    {
        Region& kRegion = GetRegion(*poolIter);

        // Can't shift beyond the start of the stream
        NIASSERT(uiCount <= kRegion.m_start);

        if (uiCount > kRegion.m_range)
        {
            uiMoveOffset = 0;
            uiMoveCount = kRegion.m_range;
        }
        else
        {
            uiMoveOffset = kRegion.m_range - uiCount;
            uiMoveCount = uiCount;
        }

        // Perform the memmove
        NiUInt32 uiStartOffset = (kRegion.m_start - uiCount) * uiInstancesPerCell; 
        NiTransform* pkDest = pkTransformStream + uiStartOffset;
        NiTransform* pkSource = 
            pkTransformStream + (kRegion.m_start + uiMoveOffset) * uiInstancesPerCell;

        if (uiCount > 0)
        {
            for (NiUInt32 ui = 0; ui < uiMoveCount; ++ui)
            {
                NiUInt32 uiOldIndex = kRegion.m_start + uiMoveOffset + ui;

                NiDecorationCell* pkCell = m_kCells[uiOldIndex];
                AssertCellDebugTransforms(pkCell);
            }

            Memmove(
                pkDest, 
                (GetTransformCount() - uiStartOffset) * stTransformSize, 
                pkSource, 
                uiMoveCount * uiInstancesPerCell * stTransformSize);

            // Artificially move the beginning of the region backwards
            for (NiUInt32 ui = 0; ui < kRegion.m_range; ui++)
            {
                m_kCells[ui + kRegion.m_start]->m_uiIndexInRegion += uiCount;
            }

            // Update the region position
            NiUInt32 uiOldIndex = kRegion.m_start + uiMoveOffset;
            kRegion.m_start -= uiCount;

            // Update all the moved cells? (including telling them that they need to upload)
            for (NiUInt32 ui = 0; ui < uiMoveCount; ++ui)
            {
                NiUInt32 uiShiftDistance = uiCount + uiMoveOffset;

                NiDecorationCell* pkCell = m_kCells[uiOldIndex + ui];

                NIASSERT(pkCell->m_pkRegionID == *poolIter);
                NIASSERT(pkCell->m_uiIndexInRegion >= uiShiftDistance);

                // Move the cell
                pkCell->m_uiIndexInRegion -= uiShiftDistance;
                m_kCells.at(kRegion.m_start + ui) = pkCell;
                
                // TODO: Remove this debugging statement
                m_kCells.at(uiOldIndex + ui) = NULL;

                AssertCellDebugTransforms(pkCell);

                // We now need to upload this cell.
                m_kCellsToUpload.insert(pkCell);
            }
        }

        // TODO: Remove debugging statement, unless CheckConsistency is enabled
        for (CellList::iterator cellIter = kRegion.m_kReleasedCells.begin();
            cellIter != kRegion.m_kReleasedCells.end();
            cellIter++)
        {
            (*cellIter)->m_pkRegionID = NULL;
        }

        // How far left does the next region need to shift?
        uiCount += kRegion.m_kReleasedCells.size();

        // TODO: Remove debugging statement, unless CheckConsistency is enabled
        for (NiUInt32 ui = 0; ui < uiCount; ui++)
        {
            NiUInt32 uiFreedCellIndex = kRegion.m_start + kRegion.m_range + ui;
            NIASSERT(m_kCells.at(uiFreedCellIndex) == NULL ||
                m_kCells.at(uiFreedCellIndex)->m_pkRegionID == NULL);

            m_kCells.at(uiFreedCellIndex) = NULL;
        }

        // Forget any released cells
        m_kUnusedCells.insert(m_kUnusedCells.end(), 
            kRegion.m_kReleasedCells.begin(), kRegion.m_kReleasedCells.end());
        kRegion.m_kReleasedCells.clear();
    }

    AssertCellConsistency();

    // Keep the stream lock, since it is needed in the next section

    // Process any remaining new cell requests
    for (RegionIDList::iterator poolIter = m_kRegions.begin();
        poolIter != m_kRegions.end();
        poolIter++)
    {
        Region& kRegion = GetRegion(*poolIter);
        Region& kLastRegion = GetRegion(m_kRegions.back());

        // New requests
        while (!kRegion.m_kRequestedCells.empty())
        {
            // Have we run out of room for new requests? Don't assert this; it is quite possible 
            // that there are leftover 'free' requests. Just wait until the next run.
            if (kLastRegion.m_start + kLastRegion.m_range + 1 >= GetNumMaxCells())
                break;

            // Make room for a new cell
            for (RegionIDList::reverse_iterator shiftPoolIter = m_kRegions.rbegin();
                shiftPoolIter != m_kRegions.rend();
                shiftPoolIter++)
            {
                // Have we got to the inserter?
                if (*shiftPoolIter == *poolIter)
                    break;

                Region& kShiftRegion = GetRegion(*shiftPoolIter);

                if (kShiftRegion.m_range > 0)
                {
                    NiDecorationCell* pkFirstCell = m_kCells[kShiftRegion.m_start];

                    NIASSERT(pkFirstCell->m_pkRegionID == *shiftPoolIter);

                    // Copy first cell to last
                    NiUInt32 uiDestOffset_inst = 
                        (kShiftRegion.m_start + kShiftRegion.m_range) * uiInstancesPerCell;
                    NIASSERT(uiDestOffset_inst <= GetTransformCount());
                    size_t stDestSize = (GetTransformCount() - uiDestOffset_inst) * 
                        stTransformSize;
                    NiTransform* pkDestStream = pkTransformStream + uiDestOffset_inst;
                    NiTransform* pkSourceStream = pkTransformStream + 
                        (kShiftRegion.m_start + pkFirstCell->m_uiIndexInRegion) * 
                        uiInstancesPerCell;

                    Memmove(
                        pkDestStream, 
                        stDestSize, 
                        pkSourceStream, 
                        uiInstancesPerCell * stTransformSize);

                    // Update the cells transform stream pointer?
                    NiUInt32 uiNewIndex = kShiftRegion.m_start + kShiftRegion.m_range;
                    m_kCells.at(uiNewIndex) = pkFirstCell;
                    pkFirstCell->m_uiIndexInRegion = kShiftRegion.m_range - 1;

                    // TODO: Find an efficient way to decrement the index in region of all the other
                    // cells?
                    for (NiUInt32 ui = kShiftRegion.m_start + 1; ui < uiNewIndex; ui++)
                    {
                        NIASSERT(m_kCells[ui]->m_uiIndexInRegion > 0);
                        m_kCells[ui]->m_uiIndexInRegion--;
                    }

                    AssertCellDebugTransforms(pkFirstCell);

                    // Mark the destination cell as requiring an upload
                    m_kCellsToUpload.insert(pkFirstCell);
                }

                // Update the region
                kShiftRegion.m_start++;
            }

            // Create the new cell
            NiDecorationCell* pkCell;
            if (m_kUnusedCells.size() != 0)
            {
                EE_VERIFY(m_kUnusedCells.pop_front(pkCell));
            }
            else
            {
                pkCell = EE_NEW NiDecorationCell();
            }

            m_kCells.at(kRegion.m_start + kRegion.m_range) = pkCell;

            // Initialize the cell
            CellIndex& newCell = kRegion.m_kRequestedCells.front();
            pkCell->m_uiIndexInRegion = kRegion.m_range;
            pkCell->m_pkRegionID = *poolIter;
            pkCell->m_uiIndexX = newCell.m_uiX;
            pkCell->m_uiIndexY = newCell.m_uiY;

            // Update the region
            kRegion.m_range++;

            // Generate the transforms
            NIASSERT(pkCell->m_uiIndexInRegion < kRegion.m_range);
            NIASSERT((kRegion.m_start + kRegion.m_range) <= m_uiMaxCellCount);

            CellRequestGenerationResult eGenerationResult = 
                kRegion.m_pkProcessor->ProcessCellTransforms(
                    pkCell,
                    pkTransformStream + 
                        (kRegion.m_start + pkCell->m_uiIndexInRegion) * uiInstancesPerCell,
                    m_uiInstancesPerCell,
                    true);

            if (eGenerationResult == GR_GENERATED_VALID_TRANSFORMS)
            {
                AssertCellDebugTransforms(pkCell);
            }
            else
            {
                // Free this cell next frame
                kRegion.m_kReleasedCells.push_back(pkCell);
            }

            // Do we need to upload transforms?
            if (NiDecorationTransformProcessor::RequiresUpload(eGenerationResult))
            {
                // Mark cell for upload
                m_kCellsToUpload.insert(pkCell);
            }

            // The cell has now been processed, so it can be forgotten about
            kRegion.m_kRequestedCells.pop_front();
        }
    }

    // Might not be able to check consistency here, because the leftover free'd cells 
    AssertCellConsistency();

    Unlock(NiDataStream::LOCK_WRITE);

    // Remove any leftover cell pointers in the cells array
    {
        if (m_kRegions.size() > 0)
        {
            Region& kLastRegion = GetRegion(m_kRegions.back());

            for (NiUInt32 uiCellIndex = kLastRegion.m_start + kLastRegion.m_range;
                uiCellIndex < m_kCells.size();
                ++uiCellIndex)
            {
                NiDecorationCell*& pkCell = m_kCells.at(uiCellIndex);
                if (pkCell == NULL)
                    continue;
                else
                    pkCell = NULL;
            }
        }
    }

    AssertCellConsistency();

    // Update the region sizes
    for (RegionIDList::iterator poolIter = m_kRegions.begin();
        poolIter != m_kRegions.end();
        poolIter++)
    {
        Region& kRegion = GetRegion(*poolIter);

        NiMesh* pkBaseMesh = GetMeshInfo(*poolIter)->GetMesh();
        if (pkBaseMesh->GetInstanced())
        {
            NiDataStream::Region& kStreamRegion = m_pkInstanceStream->GetRegion(
                kRegion.m_uiDataStreamRegionIndex);

            kStreamRegion.SetStartIndex(kRegion.m_start * uiInstancesPerCell);
            kStreamRegion.SetRange(kRegion.m_range * uiInstancesPerCell);

            pkBaseMesh->UpdateCachedPrimitiveCount();
        }
    }
}

//------------------------------------------------------------------------------------------------
NiTransform* NiDecorationTransformManager::LockRegion(Region& kRegion, 
    NiDataStream::LockType lockType)
{
    EE_UNUSED_ARG(lockType);

    EE_ASSERT((kRegion.m_start + kRegion.m_range) * GetInstancesPerCell() <= 
        m_uiTransformCount);

    NiTransform* pkRegion = m_pkTransforms + kRegion.m_start * GetInstancesPerCell();

    return pkRegion;
}

//------------------------------------------------------------------------------------------------
NiTransform* NiDecorationTransformManager::LockStream(NiDataStream::LockType lockType)
{
    EE_UNUSED_ARG(lockType);

    return m_pkTransforms;
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::Unlock(NiDataStream::LockType lockType)
{
    EE_UNUSED_ARG(lockType);
    // Do nothing
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::DoAssertCellConsistency()
{
#if defined(EE_ASSERTS_ARE_ENABLED)
    for (NiUInt32 uiIndex = 0; uiIndex < m_kCells.size(); ++uiIndex)
    {
        NiDecorationCell* pkCell = m_kCells[uiIndex];
        if (pkCell == NULL || pkCell->m_pkRegionID == NULL)
            continue;

        Region& kRegion = GetRegion(pkCell->m_pkRegionID);
        NiUInt32 uiRegionOffset = kRegion.m_start;

        // Is this cell within the regions bounds?
        EE_ASSERT(uiIndex >= kRegion.m_start && uiIndex < kRegion.m_start + kRegion.m_range);

        // Is the cells stored index in region correct?
        EE_ASSERT(pkCell->m_uiIndexInRegion + uiRegionOffset == uiIndex);
    }
#endif
}

//------------------------------------------------------------------------------------------------
void NiDecorationTransformManager::DoAssertCellDebugTransforms(NiDecorationCell* pkCell)
{
    Region& kRegion = GetRegion(pkCell->m_pkRegionID);

    NiDecorationField* pkField = 
        (NiDecorationField*)(GetMeshInfo(pkCell->m_pkRegionID)->GetMesh()->GetParent()->GetParent());

    NIASSERT(pkCell->m_uiIndexInRegion < kRegion.m_range);
    NiTransform* pkTransforms = LockRegion(kRegion, NiDataStream::LOCK_READ) + 
        pkCell->m_uiIndexInRegion * m_uiInstancesPerCell;


    if (pkField->GetFieldIndex() == 0)
    {
        // Expect < 0
        float fHeight;
        for (NiUInt32 ui = 0; ui < m_uiInstancesPerCell; ++ui)
        {
            fHeight = pkTransforms[ui].m_Translate.z;
            NIASSERT(fHeight < 0.0f);
        }
    }
    else
    {
        // Expect > 0 or negative infinity
        float fHeight;
        for (NiUInt32 ui = 0; ui < m_uiInstancesPerCell; ++ui)
        {
            fHeight = pkTransforms[ui].m_Translate.z;
            NIASSERT(fHeight > 0.0f || fHeight < 10000.0f);
        }
    }
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::Region& NiDecorationTransformManager::GetRegion(RegionID kRegionID)
{
    EE_ASSERT(m_kStreamRegions.find(kRegionID) != m_kStreamRegions.end());
    return m_kStreamRegions[kRegionID];
}

//------------------------------------------------------------------------------------------------
NiDecorationMeshInfo* NiDecorationTransformManager::GetMeshInfo(RegionID kRegion)
{
    return kRegion;
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::Region::Region()
    : m_pkProcessor(NULL)
{
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformManager::Region::~Region()
{
    NiDelete(m_pkProcessor);
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformProcessor::NiDecorationTransformProcessor()
{
}

//------------------------------------------------------------------------------------------------
NiDecorationTransformProcessor::~NiDecorationTransformProcessor()
{
}

//------------------------------------------------------------------------------------------------
bool NiDecorationTransformProcessor::RequiresUpload(CellRequestGenerationResult eGenerationResult)
{
    switch (eGenerationResult)
    {
    case GR_GENERATED_INVALID_TRANSFORMS:
    case GR_GENERATED_VALID_TRANSFORMS:
        return true;

    case GR_EXISTING_INVALID_TRANSFORMS:
    case GR_EXISTING_UNKNOWN_TRANSFORMS:
    default:
        return false;
    }
}

//------------------------------------------------------------------------------------------------
