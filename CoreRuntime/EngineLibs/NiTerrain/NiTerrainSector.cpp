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

#include "NiTerrainPCH.h"

#include <algorithm>
#include <NiMath.h>
#include "NiTerrainStreamLocks.h"
#include "NiTerrainSector.h"
#include "NiDynamicStreamCache.h"
#include "NiTerrain.h"
#include "NiTerrainRandomAccessIterator.h"
#include "NiTerrainCullingProcess.h"
#include "NiTerrainStreamingManager.h"
#include "NiTerrainDataSnapshot.h"

NiImplementRTTI(NiTerrainSector, NiNode, NiTypeMask::NiTerrainSector);

//--------------------------------------------------------------------------------------------------
NiTerrainSector::NiTerrainSector(bool) :
    m_bRecycleDynamicStreams(true),
    m_bUsingShortIndexBuffer(true),
    m_bSurfacesInFile(false),
    m_bLightingOutdated(false),
    m_bCalculateData(true),
    m_pkSectorPagingData(NULL),
    m_pkTerrain(NULL),
    m_pkSectorData(NULL),
    m_puiCellOffsets(NULL),
    m_pkLocalCamera(NULL),
    m_pkQuadCell(NULL),
    m_pkLowDetailDiffuseTexture(NULL),
    m_pkLowDetailNormalTexture(NULL),
    m_bHasShapeChangedLastUpdate(false),
    m_uiLowDetailTextureSize(512),
    m_pkPaintingData(NULL),
    m_uiSectorID(0)
{
    m_pkSectorData = NiNew NiTerrainSectorData();
    m_pkLocalCamera = NiNew NiCamera();
}

//--------------------------------------------------------------------------------------------------
NiTerrainSector::~NiTerrainSector()
{
    // We should have had all our data unloaded by now
    EE_ASSERT(m_pkQuadCell == NULL);
    RemoveAllChildren();
    
    // Destroy painting data
    NiDelete m_pkPaintingData;

    DestroySupportingData();

    NiFree(m_puiCellOffsets);
    m_puiCellOffsets = NULL;

    NiDelete m_pkSectorData;
    NiDelete m_pkLocalCamera;

    NiDelete m_pkSectorPagingData;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::DoUpdate(NiUpdateProcess& kUpdate)
{
    EE_UNUSED_ARG(kUpdate);

    // Update transforms?
    NiTransform kWorldTransform = GetWorldTransform();
    if (m_pkQuadCell && 
        kWorldTransform != m_pkSectorData->GetWorldTransform())
    {
        LockSectorUpdating();
        m_pkSectorData->SetWorldTransform(kWorldTransform);
        UnlockSectorUpdating();

        NiUInt32 uiMaxCell;
        if (GetMaxLoadedCellIndex(uiMaxCell))
        {
            NiTerrainCell* pkCell;
            for (NiUInt32 ui = 0; ui <= uiMaxCell; ++ui)
            {
                EE_ASSERT(m_pkSectorData->IsLODLoaded());
                pkCell = m_kCellArray[ui];            
                EE_ASSERT(pkCell);

                pkCell->SetWorldTranslate(kWorldTransform.m_Translate);
                pkCell->SetWorldScale(kWorldTransform.m_fScale);
                pkCell->SetWorldRotate(kWorldTransform.m_Rotate);

                pkCell->GetMesh().SetWorldTranslate(kWorldTransform.m_Translate);
                pkCell->GetMesh().SetWorldScale(kWorldTransform.m_fScale);
                pkCell->GetMesh().SetWorldRotate(kWorldTransform.m_Rotate);
            }

            SetShapeChangedLastUpdate(true);
        }
    }

    // Update the Meshes?
    if (m_pkSectorData->IsLODLoaded(0) && m_pkQuadCell->RequiresUpdate())
    {
        m_pkQuadCell->Update();

        // Re-assign the root node bound
        if (m_pkQuadCell)
        {
            NiBound kBound = m_pkQuadCell->GetLocalBound();
            kBound.SetRadius(kBound.GetRadius() * GetWorldScale());
            kBound.SetCenter(GetWorldTransform() * kBound.GetCenter());
            m_pkQuadCell->SetWorldBound(kBound);
        }
    }

    // Check if the shape has changed recently
    if (HasShapeChangedLastUpdate() && m_pkTerrain)
    {
        m_pkTerrain->SetShapeChangedLastUpdate(true);
        SetShapeChangedLastUpdate(false);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::ProcessLOD(NiCullingProcess &kCuller)
{    
    bool bResult = false;
    if (m_pkQuadCell && m_pkSectorData->IsLODLoaded()) 
    {   
        // Reset the LOD information:
        m_pkQuadCell->ResetCullingResults();

        // Need to convert the camera to terrain coordinate space
        // create the transformation matrix to go from world space to terrain
        // model space:
        const NiCamera* pkCamera = kCuller.GetCamera();
        m_pkLocalCamera->SetTranslate(GetWorldRotate().Inverse() * (
            (pkCamera->GetWorldTranslate() - GetWorldTranslate())
            / GetWorldScale()));
        m_pkLocalCamera->SetRotate(GetWorldRotate().Inverse() *
            pkCamera->GetWorldRotate());
        m_pkLocalCamera->SetScale(pkCamera->GetWorldScale());

        NiFrustum kFrustum = pkCamera->GetViewFrustum();
        kFrustum.m_fFar /= GetWorldScale();
        kFrustum.m_fNear /= GetWorldScale();
        m_pkLocalCamera->SetViewFrustum(kFrustum);

        m_pkLocalCamera->Update(0.0f);

        // Set this sectors culling and camera data - shared throughout our
        // block tree during rendering
        m_pkSectorData->SetCullingProcess(&kCuller);
        m_pkSectorData->SetLODCamera(m_pkLocalCamera);
        bResult = m_pkQuadCell->ProcessLOD();
    }

    // Render any children that have been attached by third parties
    NiUInt32 uiChildCount = GetChildCount();
    for (NiUInt32 ui = 0; ui < uiChildCount; ++ui)
    {
        NiAVObject* pkScene = GetAt(ui);
        if (pkScene == m_pkQuadCell)
            continue;

        pkScene->OnVisible(kCuller);
    }

    return bResult;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::ProcessBorders()
{
    if (!m_pkSectorData->IsLODLoaded())
        return;
    
    // Update the stitching information
    if (m_pkQuadCell)
        m_pkQuadCell->ProcessBorders();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::OnVisible(NiCullingProcess &kCuller)
{
    ProcessLOD(kCuller);
}

//---------------------------------------------------------------------------
void NiTerrainSector::SetIsDeformable(bool bIsDeformable) 
{
    bIsDeformable |= NiTerrain::InToolMode();

    if (m_pkSectorData->GetDeformable() != bIsDeformable)
    {
        m_pkSectorData->SetDeformable(bIsDeformable);

        // If no dynamic stream cache exists, build one.
        if (!m_pkSectorData->GetDynamicStreamCache() && bIsDeformable &&
            m_pkQuadCell)
        {
            EE_ASSERT(m_pkSectorData->GetStaticPositionStream(0));
            EE_ASSERT(m_pkSectorData->GetStaticNormalTangentStream(0));

            if (!m_pkSectorData->GetStaticPositionStream(0) ||
                !m_pkSectorData->GetStaticNormalTangentStream(0))
            {
                m_pkSectorData->SetDeformable(false);
                return;
            }

            NiUInt32 uiNumDesc = 0;
            NiDataStream* pkStream;
            NiDataStreamElementSet kElementSetP;
            NiDataStreamElementSet kElementSetNT;

            pkStream = m_pkSectorData->GetStaticPositionStream(0);
            uiNumDesc = pkStream->GetElementDescCount();
            for (NiUInt32 ui = 0; ui < uiNumDesc; ++ui)
                kElementSetP.Add(pkStream->GetElementDescAt(ui));
            kElementSetP.m_uiStride = pkStream->GetStride();

            pkStream = m_pkSectorData->GetStaticNormalTangentStream(0);
            uiNumDesc = pkStream->GetElementDescCount();
            for (NiUInt32 ui = 0; ui < uiNumDesc; ++ui)
                kElementSetNT.Add(pkStream->GetElementDescAt(ui));
            kElementSetNT.m_uiStride = pkStream->GetStride();

            NiUInt32 uiSize = GetNumCells() / 4;

            // If we failed to create the cache, we can't be deformable.
            bool bRes = CreateDeformationCache(uiSize);
            if (!bRes)
            {
                m_pkSectorData->SetDeformable(false);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::SetTerrain(NiTerrain* pkTerrain)
{
    if (m_pkTerrain == pkTerrain)
        return;

    if (m_pkTerrain)
    {
        m_pkTerrain->RemoveSector(this);
    }

    m_pkTerrain = pkTerrain;

    if (m_pkTerrain)
    {
        m_pkTerrain->AddSector(this);
        m_pkSectorData->SetConfiguration(m_pkTerrain->GetConfiguration());
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::SetSectorIndex(
    NiInt16 sXIndex, NiInt16 sYIndex)
{
    if (m_pkSectorData->GetSectorIndexX() != sXIndex ||
        m_pkSectorData->GetSectorIndexY() != sYIndex)
    {
        m_pkSectorData->SetSectorIndexX(sXIndex);
        m_pkSectorData->SetSectorIndexY(sYIndex);

        // Update our stored sector ID
        GenerateSectorID(sXIndex, sYIndex, m_uiSectorID);
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSector* NiTerrainSector::GetAdjacentSector(NiUInt32 uiBorder)
    const
{
    EE_ASSERT(m_pkTerrain);
    NiInt16 sIndexX;
    NiInt16 sIndexY;
    GetSectorIndex(sIndexX, sIndexY);

    if (uiBorder & NiTerrainCell::BORDER_LEFT)
        sIndexX --;
    if (uiBorder & NiTerrainCell::BORDER_RIGHT)
        sIndexX ++;
    if (uiBorder & NiTerrainCell::BORDER_TOP)
        sIndexY ++;
    if (uiBorder & NiTerrainCell::BORDER_BOTTOM)
        sIndexY --;

    NiTerrainSector* pkSector = m_pkTerrain->GetSector(sIndexX, sIndexY);

    // A sector is only valid if it has any data loaded; otherwise it is simply waiting to be
    // deleted
    if (pkSector && pkSector->GetSectorData()->GetHighestLoadedLOD() >= 0)
    {
        return pkSector;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::PrepareForPainting(const NiSurface* pkSurface, NiUInt32* puiErrorCode)
{
    NiUInt32 uiDummyErrorCode;
    if (!puiErrorCode)
        puiErrorCode = &uiDummyErrorCode;

    // Make sure painting is supported by having a renderable texture
    if (!IsReadyToPaint())
    {
        // Create the relevant painting environment
        CreatePaintingData(puiErrorCode);
    }

    // Add the surface to the sector data
    AddSurface(pkSurface);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::NotifySurfaceChanged(NiUInt32 uiSurfaceIndex)
{
    NiTerrainCellLeaf* pkLeaf;
    NiUInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (NiUInt32 ui = 0; ui <= uiMaxCell; ++ui)
    {
        pkLeaf = NiDynamicCast(NiTerrainCellLeaf, m_kCellArray[ui]);
        if (!pkLeaf)
            continue;

        NiUInt32 uiPriority;
        if (pkLeaf->GetSurfacePriority(uiSurfaceIndex, uiPriority))
            pkLeaf->MarkSurfaceMasksChanged(true);
    }
}

//---------------------------------------------------------------------------
void NiTerrainSector::RemoveSurface(NiUInt32 uiSurfaceIndex)
{
    NiTerrainCellLeaf* pkLeaf;
    NiUInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (NiUInt32 ui = 0; ui <= uiMaxCell; ++ui)
    {
        pkLeaf = NiDynamicCast(NiTerrainCellLeaf, m_kCellArray[ui]);
        
        // We can only remove surfaces from Leaves and not from nodes
        if (!pkLeaf)
            continue;

        pkLeaf->RemoveSurface(uiSurfaceIndex);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::RemoveAllSurfaces()
{
    NiTerrainCellLeaf* pkLeaf;
    NiUInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (NiUInt32 ui = 0; ui <= uiMaxCell; ++ui)
    {
        pkLeaf = NiDynamicCast(NiTerrainCellLeaf, m_kCellArray[ui]);
        
        // We can only remove surfaces from Leaves and not from nodes
        if (!pkLeaf)
            continue;

        pkLeaf->RemoveAllSurfaces();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::GetSurfacesUsed(efd::set<efd::UInt32>& kSurfaceIndex)
{
    NiTerrainCellLeaf* pkLeaf;
    efd::UInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (efd::UInt32 ui = 0; ui <= uiMaxCell; ++ui)
    {
        pkLeaf = NiDynamicCast(NiTerrainCellLeaf, m_kCellArray[ui]);

        // We can only remove surfaces from Leaves and not from nodes
        if (!pkLeaf)
            continue;

        for (efd::UInt32 uiSlot = 0; uiSlot < NiTerrainCellLeaf::MAX_NUM_SURFACES; ++uiSlot)
        {
            efd::SInt32 iSurfaceIndex = pkLeaf->GetSurfaceIndex(uiSlot);
            if (iSurfaceIndex != -1)
            {
                kSurfaceIndex.insert(efd::UInt32(iSurfaceIndex));
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiInt32 NiTerrainSector::AddSurface(const NiSurface* pkSurface)
{
    EE_ASSERT(m_pkTerrain);
    return m_pkTerrain->AddSurface(pkSurface);
}

//--------------------------------------------------------------------------------------------------
const NiSurface* NiTerrainSector::GetSurfaceAt(NiUInt32 uiIndex)
{
    EE_ASSERT(m_pkTerrain);
    return m_pkTerrain->GetSurfaceAt(uiIndex);
}

//--------------------------------------------------------------------------------------------------
NiInt32 NiTerrainSector::GetSurfaceIndex(const NiSurface* pkSurface)
{
    EE_ASSERT(m_pkTerrain);
    return m_pkTerrain->GetSurfaceIndex(pkSurface);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::SetFormat(NiUInt32 uiBlockSize, NiUInt32 uiNumLOD)
{
    // We should not have any loaded data
    EE_ASSERT(!m_pkQuadCell);

    DestroySupportingData();

    m_pkSectorData->SetCellSize(uiBlockSize);
    m_pkSectorData->SetNumLOD(uiNumLOD);

    // Make sure the cell offset information is accurate with the 
    // change in the number of LODs
    m_puiCellOffsets = (NiUInt32*)NiRealloc(m_puiCellOffsets, 
        sizeof(NiUInt32) * (m_pkSectorData->GetNumLOD() + 2));
    m_puiCellOffsets[0] = 0;

    for (NiUInt32 ui = 1; ui < m_pkSectorData->GetNumLOD() + 2; ++ui)
    {
        NiUInt32 uiBlocksPerRow = 1 << (ui - 1);
        NiUInt32 uiVal = m_puiCellOffsets[ui - 1] + 
            uiBlocksPerRow * uiBlocksPerRow;
        m_puiCellOffsets[ui] = uiVal;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreateIndexStream()
{
    if (m_pkSectorData->GetStaticIndexStream() != NULL)
        return true;

    NiUInt32 uiBlockSize = m_pkSectorData->GetCellSize();

    // NOTE: Each block will only ever have a maximum of 2 sides which need
    // stitching and those two sides must be adjoining. This allows us to use
    // up to 65x65 blocks with a 16 bit index buffer

    // We will support both 16 and 32bit index buffers, using smallest where
    // possible

    // [indices per tri] * [tri's per square] * [num squares]
    NiUInt32 auiNumIndicesPerNumBorders[3];
    auiNumIndicesPerNumBorders[0] = 3 * 2 * uiBlockSize * uiBlockSize;
    auiNumIndicesPerNumBorders[1] = 3 * 2 * uiBlockSize * (uiBlockSize-1) +
        3 * (3 * (uiBlockSize / 2));
    auiNumIndicesPerNumBorders[2] = 3 * 2 * (uiBlockSize-1) * (uiBlockSize-1) +
        3 * (6 * (uiBlockSize / 2) - 2);

    // 1 set of full LOD
    NiUInt32 uiNumIndices = 1 * auiNumIndicesPerNumBorders[0];
    // 4 sets of One Border
    uiNumIndices += 4 * auiNumIndicesPerNumBorders[1];
    // 4 sets of Two Borders
    uiNumIndices += 4 * auiNumIndicesPerNumBorders[2];

    // If we can, support 16 bit index buffer
    if (uiNumIndices > USHRT_MAX)
    {
        m_bUsingShortIndexBuffer = false;
    }
    else
    {
        m_bUsingShortIndexBuffer = true;
    }

    NiDataStream* pkIndexStream = GetResourceManager()->CreateStream(
        NiTerrain::StreamType::INDEX, uiNumIndices);
    if (pkIndexStream->GetCount(0) != uiNumIndices)
    {
        EE_FAIL("Failed to create valid index stream");

        NiDelete pkIndexStream;
        return false;
    }

    // Create the regions.
    pkIndexStream->RemoveAllRegions();
    NiDataStream::Region* pkRegion;
    NiUInt32 uiRegionStart = 0;
    for (NiUInt8 i = 0; i < NiTerrainSectorData::NUM_INDEX_REGIONS; ++i)
    {
        pkRegion = m_pkSectorData->GetIndexRegion(i);
        if (!pkRegion)
        {
            pkRegion = NiNew NiDataStream::Region();
            m_pkSectorData->SetIndexRegion(i, pkRegion);
        }

        NiUInt32 uiRegionSize;
        if (i > 4)
            uiRegionSize = auiNumIndicesPerNumBorders[2];
        else if (i > 0)
            uiRegionSize = auiNumIndicesPerNumBorders[1];
        else
            uiRegionSize = auiNumIndicesPerNumBorders[0];

        pkRegion->SetRange(uiRegionSize);

        pkRegion->SetStartIndex(uiRegionStart);
        uiRegionStart += uiRegionSize;

        pkIndexStream->AddRegion(*pkRegion);
    }

    m_pkSectorData->SetStaticIndexStream(pkIndexStream);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreateUVStream()
{
    if (m_pkSectorData->GetStaticUVStream() != NULL)
        return true;
    
    NiDataStream* pkUVStream = GetResourceManager()->CreateStream(
        NiTerrain::StreamType::TEXTURE_COORD, 0);
    m_pkSectorData->SetStaticUVStream(pkUVStream);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreatePNTStream(NiUInt32 uiMaxDetailLevel)
{
    // Initialize the detail level to load to if required:
    if (uiMaxDetailLevel == NiTerrainUtils::ms_uiMAX_LOD)
        uiMaxDetailLevel = GetSectorData()->GetNumLOD();

    // Figure out what LODs we have already created:
    NiUInt32 uiNextLevel = GetSectorData()->GetHighestLoadedLOD() + 1;

    // Allocate the streams
    NiUInt32 uiNumBlocks = GetNumCells();
    if (NiTerrain::InToolMode())
    {
        // Check if an old stream cache exists:
        if (m_pkSectorData->GetDynamicStreamCache())
        {
            return true;
        }

        // Create a new stream cache
        bool bRes = CreateDeformationCache(uiNumBlocks);

        if (!bRes)
        {
            EE_FAIL("Failed to create deformation cache");
            return false;
        }
    }
    else
    {
        NiTerrainResourceManager* pkManager = GetResourceManager();
        // Create a static stream for each LOD Level
        for (NiUInt32 uiLevel = uiNextLevel; uiLevel < uiMaxDetailLevel + 1;
            ++uiLevel)
        {
            // Create the streams
            NiDataStream* pkVertexStream = 
                pkManager->CreateStream(NiTerrain::StreamType::POSITION, uiLevel);
            NiDataStream* pkNormalTangentStream = 
                pkManager->CreateStream(NiTerrain::StreamType::NORMAL_TANGENT, uiLevel);;

            if (!pkVertexStream)
            {
                EE_ASSERT(!"Failed to create streams");
                return false;
            }

            m_pkSectorData->SetStaticVertexStream(uiLevel, pkVertexStream);
            m_pkSectorData->SetStaticNormalTangentStream(uiLevel, pkNormalTangentStream);
        }

        //=======================//
        // Dynamic Block Streams //
        //=======================//

        if (m_pkSectorData->GetDeformable())
        {
            // Check if an old stream cache exists:
            if (m_pkSectorData->GetDynamicStreamCache())
                return true;
            CreateDeformationCache(uiNumBlocks / 5);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreateStreams(NiUInt32 uiMaxDetailLevel)
{
    //==============//
    // Index Stream //
    //==============//
    if(!CreateIndexStream())
        return false;

    //=====================================//
    // Position and Normal/Tangent streams //
    //=====================================//
    if (!CreatePNTStream(uiMaxDetailLevel))
        return false;

    //==================//
    // Shared UV Stream //
    //==================//
    if (!CreateUVStream())
        return false;
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreateDeformationCache(NiUInt32 uiCacheSize)
{
    // Should not recreate the stream cache if it already exists, since blocks
    // may already be using the streams. Similarly, we can't change the element
    // sets in the existing cache to match the new sets.
    EE_ASSERT(!m_pkSectorData->GetDynamicStreamCache());
    if (m_pkSectorData->GetDynamicStreamCache())
        return false;

    bool bRes = true;

    NiDynamicStreamCache* pkDynamicStreamsCache = NiNew NiDynamicStreamCache(GetResourceManager());

    bRes &= pkDynamicStreamsCache->InitializeStreamCache(
        NiTerrain::StreamType::POSITION,
        uiCacheSize,
        50);

    bRes &= pkDynamicStreamsCache->InitializeStreamCache(
        NiTerrain::StreamType::NORMAL_TANGENT,
        uiCacheSize,
        50);

    if (bRes)
    {
        m_pkSectorData->SetDynamicStreamCache(pkDynamicStreamsCache);
    }
    else
    {
        NiDelete pkDynamicStreamsCache;
        pkDynamicStreamsCache = NULL;
    }

    return bRes;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::BuildData(NiUInt32 uiMaxDetailLevel)
{
    bool bCalculateIndices = false;
    // Initialize the Default Detail level if required:
    if (uiMaxDetailLevel == NiTerrainUtils::ms_uiMAX_LOD)
    {
        uiMaxDetailLevel = GetSectorData()->GetNumLOD();
        bCalculateIndices = true;
    }

    // Make sure we only do this once for a sector
    if (m_bCalculateData)
    {
        m_bCalculateData = false;
        // Create the shared index buffer
        CalculateIndices();

        // Create some shared UV coordinates
        CalculateUV();
    }

    // Update the world transforms for these leaves that we just loaded:
    NiUInt32 uiBeginIndex = GetCellOffset(m_pkSectorData->GetHighestLoadedLOD() + 1);

    // It is safe to access the cell array size here, since this function is called from within
    // the thread that is responsible for modifying it (the loading thread)
    NiUInt32 uiEndIndex = m_kCellArray.GetEffectiveSize();

    LockSectorUpdating();
    m_pkTerrain->LockSurface();
    {
        NiTerrainCell* pkCell;
        NiTransform kWorld = m_pkSectorData->GetWorldTransform();

        for (NiUInt32 ui = uiBeginIndex; ui < uiEndIndex; ++ui)
        {
            pkCell = GetCell(ui);
            pkCell->SetWorldTranslate(kWorld.m_Translate);
            pkCell->SetWorldScale(kWorld.m_fScale);
            pkCell->SetWorldRotate(kWorld.m_Rotate);

            // We need to update the meshes transforms, since the cell doesn't.
            pkCell->GetMesh().SetWorldTranslate(kWorld.m_Translate);
            pkCell->GetMesh().SetWorldScale(kWorld.m_fScale);
            pkCell->GetMesh().SetWorldRotate(kWorld.m_Rotate);            

            if (pkCell->IsJustLoaded())
            {
                pkCell->RequestBoundsUpdate();
                pkCell->RequestUpdate();
            }

            pkCell->Update();
        }

        m_pkSectorData->SetHighestLoadedLOD(uiMaxDetailLevel);
    }
    m_pkTerrain->UnlockSurface();
    UnlockSectorUpdating();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::BuildBounds()
{
    if (!m_pkQuadCell)
        return;

    NiTerrainCell* pkCell;
    NiUInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (NiInt32 uiCellID = uiMaxCell; uiCellID >= 0; --uiCellID)
    {
        // Force each cell to update it's bounds
        // (starting from the leaves and working up)
        pkCell = m_kCellArray[uiCellID];
        pkCell->UpdateBounds();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    // Update any children attached by third parties
    NiUInt32 uiChildCount = GetChildCount();
    for (NiUInt32 ui = 0; ui < uiChildCount; ++ui)
    {
        NiAVObject* pkScene = GetAt(ui);
        if (pkScene == m_pkQuadCell)
            continue;

        pkScene->UpdateDownwardPass(kUpdate);
    }

    // Perform terrain related updates
    SyncWithPaging();

    if (!m_pkSectorData->IsLODLoaded())
        return;
    
    if (!m_pkQuadCell)
        return;
    
    DoUpdate(kUpdate);
    // Use the AV Object version of the function so that bounds are
    // not calculated based on the children of this node.
    NiAVObject::UpdateDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    // Update any children attached by third parties
    NiUInt32 uiChildCount = GetChildCount();
    for (NiUInt32 ui = 0; ui < uiChildCount; ++ui)
    {
        NiAVObject* pkScene = GetAt(ui);
        if (pkScene == m_pkQuadCell)
            continue;

        pkScene->UpdateSelectedDownwardPass(kUpdate);
    }

    // Perform terrain related updates
    SyncWithPaging();

    if (!m_pkSectorData->IsLODLoaded())
        return;

    if (!m_pkQuadCell)
        return;
    
    if (GetSelectiveUpdateTransforms())
        DoUpdate(kUpdate);

    // Use the AV Object version of the function so that bounds are
    // not calculated based on the children of this node.
    NiAVObject::UpdateSelectedDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    SyncWithPaging();

    if (!m_pkSectorData->IsLODLoaded())
        return;

    if (!m_pkQuadCell)
        return;
    
    if (GetSelectiveUpdateTransforms())
        DoUpdate(kUpdate);

    // Use the AV Object version of the function so that bounds are
    // not calculated based on the children of this node.
    NiAVObject::UpdateRigidDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::UpdateNodeBound()
{
    if (m_pkQuadCell)
    {
        m_kBound = m_pkQuadCell->GetLocalBound();
    }
    else
    {
        // Give the sector some bounds as it probably hasn't been loaded yet
        // setting to zero would flag it as a non-visible object and we don't want that
        m_kBound.SetCenterAndRadius(NiPoint3::ZERO, 1.0f);
    }

    m_kWorldBound.Update(m_kBound, m_kWorld);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::UpdateWorldBound()
{
    UpdateNodeBound();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::CalculateIndices()
{
    NiTerrainStreamLocks kLocks = NiTerrainStreamLocks();
    NiUInt8 usLockMask = NiDataStream::LOCK_WRITE |
        NiDataStream::LOCK_TOOL_WRITE;

    NiTerrainCell* pkCell = GetCell(0);

    NiTStridedRandomAccessIterator<NiUInt16> kIndicesIter16;
    NiTStridedRandomAccessIterator<NiUInt32> kIndicesIter32;

    // Need to get a lock appropriate to the type of stream we are using
    // Now create the triangles for each region
    for (NiUInt8 uc = 0; uc < NiTerrainSectorData::NUM_INDEX_REGIONS; ++uc)
    {
        if (m_bUsingShortIndexBuffer)
            kLocks.GetIndexIterator(pkCell, usLockMask, kIndicesIter16, uc);
        else
            kLocks.GetIndexIterator(pkCell, usLockMask, kIndicesIter32, uc);

        NiUInt32 uiBlockSize = m_pkSectorData->GetCellSize();
        bool bFlipped = false;
        bool bDrewBorder = false;

        // Block size minus one
        NiUInt32 uiBkSzMinOne = uiBlockSize - 1;

        for (NiUInt32 y = 0; y < uiBlockSize; ++y)
        {
            for (NiUInt32 x = 0; x < uiBlockSize; ++x)
            {
                bDrewBorder = false;

                // Bottom border
                if (y == 0 && (uc == 1 || uc == 8 || uc == 5))
                {
                    bDrewBorder = true;
                    if (x % 2 == 1)
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // Stitching triangle
                            // (x-1, y) (x+1, y) (x, y+1)
                            AddTri<NiUInt16>(x-1, y, x+1, y, x, y+1,
                                kIndicesIter16);

                            // do not draw if left is being stitched
                            if (x > 1 || uc != 8)
                            {
                                // (x-1, y) (x, y+1) (x-1, y+1)
                                AddTri<NiUInt16>(x-1, y, x, y+1, x-1, y+1,
                                    kIndicesIter16);
                            }

                            // do not draw if right is being stitched
                            if (x < uiBkSzMinOne || uc != 5)
                            {
                                // (x+1, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt16>(x+1, y, x+1, y+1, x, y+1,
                                    kIndicesIter16);
                            }
                        }
                        else
                        {
                            // Stitching triangle
                            // (x-1, y) (x+1, y) (x, y+1)
                            AddTri<NiUInt32>(x-1, y, x+1, y, x, y+1,
                                kIndicesIter32);

                            // do not draw if left is being stitched
                            if (x > 1 || uc != 8)
                            {
                                // (x-1, y) (x, y+1) (x-1, y+1)
                                AddTri<NiUInt32>(x-1, y, x, y+1, x-1, y+1,
                                    kIndicesIter32);
                            }

                            // do not draw if right is being stitched
                            if (x < uiBkSzMinOne || uc != 5)
                            {
                                // (x+1, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt32>(x+1, y, x+1, y+1, x, y+1,
                                    kIndicesIter32);
                            }
                        }
                    }
                }
                // Top border
                else if (y == uiBkSzMinOne && (uc == 3 || uc == 6 || uc == 7))
                {
                    bDrewBorder = true;
                    if (x % 2 == 1)
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // Stitching triangle
                            // (x-1, y+1) (x, y) (x+1, y+1)
                            AddTri<NiUInt16>(x-1, y+1, x, y, x+1, y+1,
                                kIndicesIter16);

                            // do not draw if left is being stitched
                            if (x > 1 || uc != 7)
                            {
                                // (x-1, y) (x, y) (x-1, y+1)
                                AddTri<NiUInt16>(x-1, y, x, y, x-1, y+1,
                                    kIndicesIter16);
                            }

                            // do not draw if right is being stitched
                            if (x < uiBkSzMinOne || uc != 6)
                            {
                                // (x, y) (x+1, y) (x+1, y+1)
                                AddTri<NiUInt16>(x, y, x+1, y, x+1, y+1,
                                    kIndicesIter16);
                            }
                        }
                        else
                        {
                            // Stitching triangle
                            // (x-1, y+1) (x, y) (x+1, y+1)
                            AddTri<NiUInt32>(x-1, y+1, x, y, x+1, y+1,
                                kIndicesIter32);

                            // do not draw if left is being stitched
                            if (x > 1 || uc != 7)
                            {
                                // (x-1, y) (x, y) (x-1, y+1)
                                AddTri<NiUInt32>(x-1, y, x, y, x-1, y+1,
                                    kIndicesIter32);
                            }

                            // do not draw if right is being stitched
                            if (x < uiBkSzMinOne || uc != 6)
                            {
                                // (x, y) (x+1, y) (x+1, y+1)
                                AddTri<NiUInt32>(x, y, x+1, y, x+1, y+1,
                                    kIndicesIter32);
                            }
                        }
                    }
                }

                // Right border
                if (x == uiBkSzMinOne && (uc == 2 || uc == 5 || uc == 6))
                {
                    bDrewBorder = true;
                    if (y % 2 == 1)
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // Stitching triangle
                            // (x+1, y-1) (x+1, y+1) (x, y)
                            AddTri<NiUInt16>(x+1, y-1, x+1, y+1, x, y,
                                kIndicesIter16);

                            // do not draw if top is being stitched
                            if (y < uiBkSzMinOne || uc != 6)
                            {
                                // (x, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt16>(x, y, x+1, y+1, x, y+1,
                                    kIndicesIter16);
                            }

                            // do not draw if bottom is being stitched
                            if (y > 1 || uc != 5)
                            {
                                // (x, y) (x+1, y) (x, y+1)
                                AddTri<NiUInt16>(x, y, x, y-1, x+1, y-1,
                                    kIndicesIter16);
                            }
                        }
                        else
                        {
                            // Stitching triangle
                            // (x+1, y-1) (x+1, y+1) (x, y)
                            AddTri<NiUInt32>(x+1, y-1, x+1, y+1, x, y,
                                kIndicesIter32);

                            // do not draw if top is being stitched
                            if (y < uiBkSzMinOne || uc != 6)
                            {
                                // (x, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt32>(x, y, x+1, y+1, x, y+1,
                                    kIndicesIter32);
                            }

                            // do not draw if bottom is being stitched
                            if (y > 1 || uc != 5)
                            {
                                // (x, y) (x+1, y) (x, y+1)
                                AddTri<NiUInt32>(x, y, x, y-1, x+1, y-1,
                                    kIndicesIter32);
                            }
                        }
                    }
                }
                // Left border
                else if (x == 0 && (uc == 4 || uc == 7 || uc == 8))
                {
                    bDrewBorder = true;
                    if (y % 2 == 1)
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // Stitching triangle
                            // (x, y-1) (x+1, y) (x, y+1)
                            AddTri<NiUInt16>(x, y-1, x+1, y, x, y+1,
                                kIndicesIter16);

                            // do not draw if top is being stitched
                            if (y < uiBkSzMinOne || uc != 7)
                            {
                                // (x+1, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt16>(x+1, y, x+1, y+1, x, y+1,
                                    kIndicesIter16);
                            }

                            // do not draw if bottom is being stitched
                            if (y > 1 || uc != 8)
                            {
                                // (x, y) (x+1, y) (x+1, y+1)
                                AddTri<NiUInt16>(x, y-1, x+1, y-1, x+1, y,
                                    kIndicesIter16);
                            }
                        }
                        else
                        {
                            // Stitching triangle
                            // (x, y-1) (x+1, y) (x, y+1)
                            AddTri<NiUInt32>(x, y-1, x+1, y, x, y+1,
                                kIndicesIter32);

                            // do not draw if top is being stitched
                            if (y < uiBkSzMinOne || uc != 7)
                            {
                                // (x+1, y) (x+1, y+1) (x, y+1)
                                AddTri<NiUInt32>(x+1, y, x+1, y+1, x, y+1,
                                    kIndicesIter32);
                            }

                            // do not draw if bottom is being stitched
                            if (y > 1 || uc != 8)
                            {
                                // (x, y) (x+1, y) (x+1, y+1)
                                AddTri<NiUInt32>(x, y-1, x+1, y-1, x+1, y,
                                    kIndicesIter32);
                            }
                        }
                    }
                }

                if (!bDrewBorder)
                {
                    // Just a normal triangle
                    if (bFlipped)
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // (x, y) (x+1, y) (x, y+1)
                            AddTri<NiUInt16>(x, y, x+1, y, x, y+1,
                                kIndicesIter16);

                            // (x+1, y) (x+1, y+1) (x, y+1)
                            AddTri<NiUInt16>(x+1, y, x+1, y+1, x, y+1,
                                kIndicesIter16);
                        }
                        else
                        {
                            // (x, y) (x+1, y) (x, y+1)
                            AddTri<NiUInt32>(x, y, x+1, y, x, y+1,
                                kIndicesIter32);

                            // (x+1, y) (x+1, y+1) (x, y+1)
                            AddTri<NiUInt32>(x+1, y, x+1, y+1, x, y+1,
                                kIndicesIter32);
                        }
                    }
                    else
                    {
                        if (m_bUsingShortIndexBuffer)
                        {
                            // (x, y) (x+1, y) (x+1, y+1)
                            AddTri<NiUInt16>(x, y, x+1, y, x+1, y+1,
                                kIndicesIter16);

                            // (x, y) (x+1, y+1) (x, y+1)
                            AddTri<NiUInt16>(x, y, x+1, y+1, x, y+1,
                                kIndicesIter16);
                        }
                        else
                        {
                            // (x, y) (x+1, y) (x+1, y+1)
                            AddTri<NiUInt32>(x, y, x+1, y, x+1, y+1,
                                kIndicesIter32);

                            // (x, y) (x+1, y+1) (x, y+1)
                            AddTri<NiUInt32>(x, y, x+1, y+1, x, y+1,
                                kIndicesIter32);
                        }
                    }
                }

                bFlipped = !bFlipped;
            }

            bFlipped = !bFlipped;
        }
    }   

    kLocks.ReleaseLocks();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::SetLODScale(float fScale, float fShift)
{
    m_pkSectorData->SetLODScale(fScale, fShift);

    NiTerrainCell* pkCell;
    NiUInt32 uiMaxCell;
    if (!GetMaxLoadedCellIndex(uiMaxCell))
        return;

    for (NiUInt32 uiIndex = 0; uiIndex <= uiMaxCell; ++uiIndex)
    {
        pkCell = GetCell(uiIndex);
        
        pkCell->UpdateMorphConstants();
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::SetLODMode(NiUInt32 uiMode)
{
    if (uiMode == GetLODMode())
        return true;

    if (m_pkSectorData->SetLODMode(uiMode))
    {
        NiTerrainCell* pkCell;
        NiUInt32 uiMaxCell;
        if (!GetMaxLoadedCellIndex(uiMaxCell))
            return true;

        for (NiUInt32 uiIndex = 0; uiIndex <= uiMaxCell; ++uiIndex)
        {
            pkCell = GetCell(uiIndex);
            
            pkCell->UpdateMorphConstants();
            pkCell->GetMesh().SetMaterialNeedsUpdate(true);            
        }
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::GetMaxLoadedCellIndex(NiUInt32& uiIndex) const
{
    NiInt32 iHighestLOD = GetSectorData()->GetHighestLoadedLOD();

    // Return -1 if no cells have been loaded.
    if (iHighestLOD < 0)
        return false;

    // Return the offset of this level, plus the number of cells in this level
    NiUInt32 uiNumCellsPerSide = (1 << iHighestLOD);
    uiIndex = GetCellOffset(iHighestLOD) + uiNumCellsPerSide * uiNumCellsPerSide - 1;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::CalculateUV()
{
    // We need to store UV in a small -0.5 -> 0.5 range, to avoid floating
    // point error within the shader
    NiTerrainStreamLocks kLocks = NiTerrainStreamLocks();
    NiUInt8 ucLockMask = NiDataStream::LOCK_WRITE;

    NiTerrainCell* pkBlock = GetCell(0);
    
    NiTStridedRandomAccessIterator<NiPoint2> kUVs;
    kLocks.GetUVIterator(pkBlock, ucLockMask, kUVs);    

    NiUInt32 uiWidthInVerts = m_pkSectorData->GetCellWidthInVerts();
    float fInvWidthInVerts = 1.0f / float(m_pkSectorData->GetCellSize());
    for (NiUInt32 y = 0; y < uiWidthInVerts; ++y)
    {
        for (NiUInt32 x = 0; x < uiWidthInVerts; ++x)
        {
            NiPoint2& kPoint = kUVs[x + (uiWidthInVerts - 1 - y) * uiWidthInVerts];
            kPoint.x = float(x) * fInvWidthInVerts;
            kPoint.y = float(y) * fInvWidthInVerts;
        }
    }

    kLocks.ReleaseLocks();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::BuildQuadTree(NiUInt32 uiMaxDetailLevel)
{
    NiTerrainCell* pkCell = 0;
    NiTerrainCell* pkChild = 0;

    // Initialise the Default Detail level if required:
    if (uiMaxDetailLevel == NiTerrainUtils::ms_uiMAX_LOD)
    {
        uiMaxDetailLevel = GetSectorData()->GetNumLOD();
    }
    NiInt32 iLastLevel = GetSectorData()->GetHighestLoadedLOD();

    // Figure out the amount by which the tree needs resizing:
    NiUInt32 uiNumBlocks = 0;
    for (NiUInt32 ui = 0; ui <= m_pkSectorData->GetNumLOD(); ++ui)
        uiNumBlocks += (1 << ui) * (1 << ui);

    m_kCellArray.SetSize(uiNumBlocks);
    m_kCellRegionArray.SetSize(uiNumBlocks);    

    // Iterate over the leaves that need creating and attaching to the tree
    NiUInt32 uiBeginIndex = GetCellOffset(iLastLevel + 1);
    
    NiUInt32 uiNumNewBlocks = 0;
    for (NiInt32 i = iLastLevel + 1; i <= (NiInt32)uiMaxDetailLevel; ++i)
        uiNumNewBlocks += (1 << i) * (1 << i);
    NiUInt32 uiEndIndex = uiBeginIndex + uiNumNewBlocks;
    const NiUInt32 auiOffsetMultiplyX[4] = {0, 1, 1, 0};
    const NiUInt32 auiOffsetMultiplyY[4] = {0, 0, 1, 1};

    for (NiUInt32 uiRegionID = uiBeginIndex; uiRegionID < uiEndIndex; ++uiRegionID)
    {
        NiIndex kBottomLeft;
        NiIndex kSectorIndex;

        if (uiRegionID == 0)
        {
            // Create the base level of detail:
            pkCell = NiNew NiTerrainCellNode(this, 0);
            pkCell->SetRegionID(0);
            m_pkQuadCell = pkCell;
            AttachChild(m_pkQuadCell);
        }
        else
        {
            // Fetch the parent of this cell (It will already exist)
            pkCell = GetCellByRegion((uiRegionID - 1) / 4);

            // The parent must be a node
            NiTerrainCellNode* pkNode = 
                NiDynamicCast(NiTerrainCellNode, pkCell);
            if (!pkNode)
                continue;

            NiUInt32 uiChildID = (uiRegionID - 1) % 4;

            NiUInt32 uiBlockIndexOffset = (
                m_pkSectorData->GetCellSize() << pkNode->GetNumSubDivisions()) 
                / 2;

            // Create this new cell:
            if (pkNode->GetLevel() + 1 == m_pkSectorData->GetNumLOD())
            {
                pkChild = NiNew NiTerrainCellLeaf(this, pkNode->GetLevel() + 1);
            }
            else
            {
                pkChild = NiNew NiTerrainCellNode(this, pkNode->GetLevel() + 1);
            }
            // Figure out the region index of this cell:
            pkChild->SetRegionID(uiRegionID);

            pkNode->SetChildAt(uiChildID, pkChild);

            // Setup the indexing information for this cell:
            pkNode->GetBottomLeftIndex(kBottomLeft);
            kBottomLeft.x += auiOffsetMultiplyX[uiChildID] * uiBlockIndexOffset;
            kBottomLeft.y += auiOffsetMultiplyY[uiChildID] * uiBlockIndexOffset;
            pkChild->SetBottomLeftIndex(kBottomLeft);

            pkCell = pkChild;
        }

        pkCell->GetBottomLeftIndex(kBottomLeft);
        NiUInt32 uiBlocksPerSectorSide = 1 << pkCell->GetLevel();

        // Data
        kSectorIndex = kBottomLeft /
            (m_pkSectorData->GetCellSize() << pkCell->GetNumSubDivisions());
        NiUInt32 uiCellID = kSectorIndex.x +
            kSectorIndex.y * uiBlocksPerSectorSide;

        // The blockID used to store in the array is in LOD space, this means
        // siblings are NOT stored sequentially in the array, instead the whole
        // LOD level is stored sequentially together. In order to parse this
        // information into a proper quad-tree layout, math must be done on this
        // block ID, an example of this can be found in the Save/Load functions
        // It is done this way to speed the lookup of vertices based on their
        // index into the sector as a whole.

        // Add this cell to the correct place in the block array      
        NiUInt32 uiBlockID = GetCellOffset(pkCell->GetLevel()) + uiCellID;
        pkCell->SetCellID(uiBlockID);

        // Sanity checks
        EE_ASSERT(uiBlockID < uiNumBlocks);
        EE_ASSERT(uiRegionID < uiNumBlocks);
        EE_ASSERT(m_kCellArray[uiBlockID] == 0);
        EE_ASSERT(m_kCellRegionArray[uiRegionID] == 0);

        m_kCellArray.SetAt(uiBlockID, pkCell);
        m_kCellRegionArray.SetAt(uiRegionID, pkCell);

        // Tell the leaf only it's offset from the beginning of it's static stream
        pkCell->SetRegionID(uiRegionID - GetCellOffset(pkCell->GetLevel()));

        // Build the cell's mesh
        pkCell->CreateMesh();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::DestroyMesh()
{
    DetachChild(m_pkQuadCell);
    m_pkQuadCell = 0;

    // Reset the dynamic stream block cache
    m_kDynamicCellQueue.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::DestroySupportingData()
{
    if (m_pkQuadCell)
        DestroyMesh();
    
    NiTerrainResourceManager* pkManager = GetResourceManager();

    // Let go of the low detail textures
    SetTexture(NiTerrain::TextureType::LOWDETAIL_DIFFUSE, NULL);
    SetTexture(NiTerrain::TextureType::LOWDETAIL_NORMAL, NULL);

    // Remove stream references
    pkManager->ReleaseStream(NiTerrain::StreamType::INDEX, 
        m_pkSectorData->GetStaticIndexStream());
    pkManager->ReleaseStream(NiTerrain::StreamType::TEXTURE_COORD, 
        m_pkSectorData->GetStaticUVStream());
    m_pkSectorData->SetStaticIndexStream(NULL);
    m_pkSectorData->SetStaticUVStream(NULL);
    m_pkSectorData->SetDynamicStreamCache(NULL);
    for (NiUInt32 uiLevel = 0; uiLevel < m_pkSectorData->GetNumLOD() + 1;
        ++uiLevel)
    {
        NiDataStream* pkPositionStream = m_pkSectorData->GetStaticPositionStream(uiLevel);
        NiDataStream* pkLightingStream = m_pkSectorData->GetStaticNormalTangentStream(uiLevel);

        pkManager->ReleaseStream(NiTerrain::StreamType::POSITION, pkPositionStream);
        pkManager->ReleaseStream(NiTerrain::StreamType::NORMAL_TANGENT, pkLightingStream);

        if (pkPositionStream)
            m_pkSectorData->SetStaticVertexStream(uiLevel, 0);

        if (pkLightingStream)
            m_pkSectorData->SetStaticNormalTangentStream(uiLevel, 0);
    }

    // Delete the quad data tree
    for (NiUInt32 ui = 0; ui < m_kCellArray.GetSize(); ++ui)
        NiDelete(m_kCellArray.RemoveAt(ui));

    m_kCellRegionArray.RemoveAll();

    m_pkQuadCell = 0;

    // Delete the shared index regions
    for (NiUInt32 uiLevel = 0; uiLevel < m_pkSectorData->GetNumLOD();
        ++uiLevel)
    {
        NiUInt8 uc;
        for (uc = 0; uc < NiTerrainSectorData::NUM_INDEX_REGIONS; ++uc)
        {
            NiDelete m_pkSectorData->GetIndexRegion(uc);
            m_pkSectorData->SetIndexRegion(uc, NULL);
        }
    }

    // Surfaces
    RemoveAllSurfaces();
}

//---------------------------------------------------------------------------
bool NiTerrainSector::CollideWithRay2D(NiRay& kRay, NiUInt32 uiDetailLOD) const
{
    if (!m_pkQuadCell || !m_pkSectorData->IsLODLoaded())
        return false;

    // Initialise the Default Detail level if required:
    if (uiDetailLOD == NiTerrainUtils::ms_uiMAX_LOD || 
        (NiInt32)uiDetailLOD > m_pkSectorData->GetHighestLoadedLOD())
    {
        uiDetailLOD = m_pkSectorData->GetHighestLoadedLOD();
    }

    // First convert the ray to sector space
    NiTransform kTerrainTransform = GetWorldTransform();
    kRay.TransformToSpace(kTerrainTransform);

    // Now find the leaf index we are colliding with
    NiUInt32 uiNumCell = 1 << uiDetailLOD;
    NiUInt32 uiStartIndex = GetCellOffset(uiDetailLOD);
    NiUInt32 uiSectorSize = m_pkSectorData->GetSectorSize();

    NiUInt32 uiCellX = NiUInt32(((efd::Floor(kRay.GetOrigin().x) / 
        uiSectorSize) + 0.5f) * uiNumCell);
    NiUInt32 uiCellY = NiUInt32(((efd::Floor(kRay.GetOrigin().y) / 
        uiSectorSize) + 0.5f) * uiNumCell);

    NiUInt32 uiCellID = uiStartIndex + uiCellX + uiCellY * uiNumCell;
    NiTerrainCell* pkCell = GetCell(uiCellID);

    EE_ASSERT (pkCell);
    bool bRes = NiTerrainUtils::TestRay2D(kRay, pkCell);

    kRay.TransformToWorldSpace();

    return bRes;
}

//-------------------------------------------------------------------------------------------------
bool NiTerrainSector::CollideWithRay(NiRay& kRay, NiUInt32 uiDetailLOD) const
{
    if (!m_pkQuadCell || !m_pkSectorData->IsLODLoaded())
        return false;

    // First convert the ray to sector space
    NiTransform kTerrainTransform = GetWorldTransform();
    kRay.TransformToSpace(kTerrainTransform);

    bool bRes = NiTerrainUtils::TestRay(kRay, m_pkQuadCell, uiDetailLOD);

    kRay.TransformToWorldSpace();

    return bRes;
}

//---------------------------------------------------------------------------
void NiTerrainSector::UpdateEffectsDownward(
    NiDynamicEffectState* pkParentState)
{    
    if (!m_pkQuadCell ||!m_pkSectorData->IsLODLoaded())
    {
        return;
    }
    
    LockSectorUpdating();
    NiNode::UpdateEffectsDownward(pkParentState);
    UnlockSectorUpdating();
}

//---------------------------------------------------------------------------
void NiTerrainSector::UpdatePropertiesDownward(NiPropertyState* pkParentState)
{    
    if (!m_pkQuadCell || !m_pkSectorData->IsLODLoaded())
    {
        return;
    }
    LockSectorUpdating();
    NiNode::UpdatePropertiesDownward(pkParentState);
    UnlockSectorUpdating();
}

//--------------------------------------------------------------------------------------------------
NiBool NiTerrainSector::GetUsingShortIndexBuffer()
{
    return m_bUsingShortIndexBuffer;
}

//--------------------------------------------------------------------------------------------------
NiBool NiTerrainSector::GetUsingShortIndexBuffer() const
{
    return m_bUsingShortIndexBuffer;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::SetTexture(NiTerrainResourceManager::TextureType::Value eTexType, 
    NiTexture* pkTexture)
{
    NiTerrainResourceManager* pkManager = GetResourceManager();

    if (eTexType == NiTerrain::TextureType::LOWDETAIL_DIFFUSE)
    {
        if (pkTexture != m_pkLowDetailDiffuseTexture)
        {
            pkManager->ReleaseTexture(NiTerrain::TextureType::LOWDETAIL_DIFFUSE,
                m_pkLowDetailDiffuseTexture);
            m_pkLowDetailDiffuseTexture = pkTexture;
        }
    }
    if (eTexType == NiTerrain::TextureType::LOWDETAIL_NORMAL)
    {
        if (pkTexture != m_pkLowDetailNormalTexture)
        {
            pkManager->ReleaseTexture(NiTerrain::TextureType::LOWDETAIL_NORMAL,
                m_pkLowDetailNormalTexture);
            m_pkLowDetailNormalTexture = pkTexture;
        }
    }

    if (m_pkQuadCell)
    {
        m_pkQuadCell->SetTexture(pkTexture, eTexType);
        m_pkQuadCell->SetTextureRegion(NiPoint2::ZERO, 1.0f, eTexType);
    }
}

//---------------------------------------------------------------------------
NiTexture* NiTerrainSector::GetTexture(NiTerrainCell::TextureType::Value eTexType)
{
    if (eTexType == NiTerrain::TextureType::LOWDETAIL_DIFFUSE)
    {
        return m_pkLowDetailDiffuseTexture;
    }
    else if (eTexType == NiTerrain::TextureType::LOWDETAIL_NORMAL)
    {
        return m_pkLowDetailNormalTexture;
    }
    else if (m_pkQuadCell)
    {
        return m_pkQuadCell->GetTexture(eTexType);
    }
    return NULL;
}

//---------------------------------------------------------------------------
void NiTerrainSector::GenerateSectorID(NiInt16 sIndexX, NiInt16 sIndexY, SectorID& kSectorID)
{
    kSectorID = 0;
    kSectorID |= (NiUInt16)sIndexX;
    kSectorID <<= 16;
    kSectorID |= (NiUInt16)sIndexY;
}

//---------------------------------------------------------------------------
void NiTerrainSector::GenerateSectorIndex(const SectorID& kSectorID, NiInt16& sIndexX, 
    NiInt16& sIndexY)
{
    sIndexX = (NiInt16)(kSectorID >> 16);
    sIndexY = (NiInt16)(kSectorID & (NiUInt32)USHRT_MAX);
}

//---------------------------------------------------------------------------
void NiTerrainSector::QueueCellForLowDetailUpdate(NiTerrainCell* pkCell)
{
    if (m_pkPaintingData)
        m_pkPaintingData->m_kLowDetailRenderQueue.AddTail(pkCell);
}

//---------------------------------------------------------------------------
void NiTerrainSector::QueueAllCellsForLowDetailUpdate()
{
    NiUInt32 uiBeginIndex = GetCellOffset(m_pkSectorData->GetNumLOD());
    NiUInt32 uiMaxCell;
    if (GetMaxLoadedCellIndex(uiMaxCell))
    {
        for (NiUInt32 uiIndex = uiBeginIndex; uiIndex <= uiMaxCell; ++uiIndex)
        {
            NiTerrainCell* pkCell = m_kCellRegionArray[uiIndex];
            EE_ASSERT(pkCell);
            QueueCellForLowDetailUpdate(pkCell);
        }
    }
}

//---------------------------------------------------------------------------
void NiTerrainSector::InitializeRenderedTexture(
    NiRenderedTexture* pkTexture, NiTexture* pkInitialTexture, 
    NiRenderClick* pkRenderClick)
{
    NiViewRenderClick* pkClick = NiDynamicCast(NiViewRenderClick, 
        pkRenderClick);
    EE_ASSERT(pkClick);

    // Setup a render target group for this texture:
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    NiRenderTargetGroup* pkTarget = NiRenderTargetGroup::Create(
        pkTexture->GetBuffer(), pkRenderer, false, true);
    pkClick->SetRenderTargetGroup(pkTarget);

    // Clear the texture to white
    NiColorA kBackgroundColor(
        NiTerrainMaterial::ms_kDefaultColor.r,
        NiTerrainMaterial::ms_kDefaultColor.g,
        NiTerrainMaterial::ms_kDefaultColor.b,
        0.0f);
    pkClick->SetBackgroundColor(kBackgroundColor);
    pkClick->SetUseRendererBackgroundColor(false);
    pkClick->SetClearAllBuffers(true);

    NiMesh2DRenderView* pkView = NiNew NiMesh2DRenderView();
    if (pkInitialTexture)
    {
        // Add a screen quad renderview to initialise the texture
        NiMeshScreenElements* pkScreenQuad = 
            NiNew NiMeshScreenElements(false, false, 1);
        pkScreenQuad->Insert(4);
        pkScreenQuad->SetRectangle(0, 0.0f, 0.0f, 1.0f, 1.0f);
        pkScreenQuad->UpdateBound();
        pkScreenQuad->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);

        // Setup how to render this quad
        NiTexturingProperty* pkTexProp = NiNew NiTexturingProperty();      
        pkTexProp->SetBaseTexture(pkInitialTexture);
        pkTexProp->SetApplyMode(NiTexturingProperty::APPLY_REPLACE);
        pkScreenQuad->AttachProperty(pkTexProp);
        NiZBufferProperty* pkZBufferProp = NiNew NiZBufferProperty();
        pkZBufferProp->SetZBufferTest(false);
        pkZBufferProp->SetZBufferWrite(false);
        pkScreenQuad->AttachProperty(pkZBufferProp);
        NiAlphaProperty* pkAlphaProp = NiNew NiAlphaProperty();
        pkAlphaProp->SetAlphaTesting(false);
        pkAlphaProp->SetAlphaBlending(false);
        pkScreenQuad->AttachProperty(pkAlphaProp);

        pkScreenQuad->UpdateProperties();
        pkScreenQuad->Update(0);

        // Attach the quad to the view
        pkView->AppendScreenElement(pkScreenQuad);
    }
    pkClick->PrependRenderView(pkView);

    pkClick->SetPostProcessingCallbackFunc(
        &Callback_RemoveTextureInitialization);
}

//---------------------------------------------------------------------------
bool NiTerrainSector::Callback_RemoveTextureInitialization(
    NiRenderClick* pkCurrentRenderClick, void* pvCallbackData)
{
    EE_UNUSED_ARG(pvCallbackData);
    // This function is designed to be called after the first time a 
    // low detail texture render target has been used. This function will
    // remove any initialisation rendering from the click.

    NiViewRenderClick* pkViewClick = NiDynamicCast(NiViewRenderClick, 
        pkCurrentRenderClick);
    EE_ASSERT(pkViewClick);

    // Stop clearing the background
    pkViewClick->SetUseRendererBackgroundColor(true);
    pkViewClick->SetClearAllBuffers(false);

    // Remove all the views ahead of the actual cell rendering view 
    // (the cell rendering view is the last one)
    NiTPointerList<NiRenderViewPtr> &kRenderViews = 
        pkViewClick->GetRenderViews();
    while (kRenderViews.GetSize() > 1)
    {
        kRenderViews.RemoveHead();   
    }

    // Remove this function callback
    pkViewClick->SetPostProcessingCallbackFunc(NULL);

    return true;
}

//---------------------------------------------------------------------------
NiRenderedTexture* NiTerrainSector::CreateLowDetailTexture(
    NiTerrainCell::TextureType::Value eTexType, NiRenderClick* pkClick, 
    NiUInt32 uiTextureSize, NiUInt32* puiErrorCode)
{
    // Cases where the texture is not valid
    if (uiTextureSize < 4)
        uiTextureSize = 4;

    NiRenderedTexture* pkTexture = GetResourceManager()->CreateRenderedTexture(
        NiTerrain::TextureType::LOWDETAIL_DIFFUSE);

    if (!pkTexture)
    {
        *puiErrorCode |= NiTerrain::EC_LRT_TEXTURE_CREATION_FAILED;
        *puiErrorCode |= NiTerrain::EC_LRT_INSUFICIENT_VRAM;
        return NULL;
    }

    // Bind this texture to the render click 
    // (performs any initialisation of the texture that may be required)
    NiTexture* pkInitialTexture = GetTexture(eTexType);
    if (pkClick)
    {
        InitializeRenderedTexture(pkTexture, pkInitialTexture, 
            pkClick);
    }
    else
    {
        // If this assert is triggered, then the texture that is 
        // currently being created will not be initialised with the data
        // from the previous one.
        EE_ASSERT(pkInitialTexture == NULL);
    }

    // Apply this new texture to the cells
    SetTexture(eTexType, pkTexture);

    // Loop through the high detail cells and cause an update
    // If the size has changed, then it needs rerendering because
    // the overlapping pixels on the borders will be different sizes now.
    bool bRerenderCells = (pkInitialTexture == NULL || 
        pkInitialTexture->GetWidth() != uiTextureSize);
    if (bRerenderCells)
    {
        QueueAllCellsForLowDetailUpdate();
    }

    return pkTexture;
}

//---------------------------------------------------------------------------
NiNode* NiTerrainSector::CreateSectorLowDetailScene(NiUInt32* puiErrorCode)
{
    // Do not create this render click if the sector has not been fully loaded
    if (!m_pkSectorData->IsLODLoaded(m_pkSectorData->GetNumLOD()))
    {
        *puiErrorCode |= NiTerrain::EC_LRT_SCENE_CREATION_FAILED;
        return NULL;
    }

    // Create a scene to use during rendering of the low detail texture
    // Common variable initialisation
    NiUInt32 uiNumLOD = m_pkSectorData->GetNumLOD();
    NiUInt32 uiNumBlocksPerSide = 1 << uiNumLOD;
    NiUInt32 uiNumBlocks = (NiUInt32)NiSqr((float)uiNumBlocksPerSide);

    // Divide the available pixels between the blocks
    float fBlockSize = 1.0f / float(uiNumBlocksPerSide);

    // Setup a scene to render with this click:
    NiNode* pkScene = NiNew NiNode();

    // Generate some streams to facilitate rendering
    NiDataStream* pkPositionStream = 
        NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_3, 4, 
        NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX, 0, true, false);
    NiDataStreamLock kPositions(pkPositionStream);

    NiDataStream* pkColorStream = 
        NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_4, 4, 
        NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX, 0, true, false);
    NiDataStreamLock kColors(pkColorStream);

    NiDataStream* pkNormalStream = 
        NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_3, 4, 
        NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX, 0, true, false);
    NiDataStreamLock kNormals(pkNormalStream);

    NiDataStream* pkTexCoordStream = 
        NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_2, 4, 
        NiDataStream::ACCESS_GPU_READ | 
        NiDataStream::ACCESS_CPU_WRITE_STATIC, 
        NiDataStream::USAGE_VERTEX, 0, true, false);
    NiDataStreamLock kTexCoords(pkTexCoordStream);

    NiTStridedRandomAccessIterator<NiPoint3> kPosIter = 
        kPositions.begin<NiPoint3>();
    NiTStridedRandomAccessIterator<NiPoint3> kNormIter = 
        kNormals.begin<NiPoint3>();
    NiTStridedRandomAccessIterator<NiColorA> kColIter = 
        kColors.begin<NiColorA>();
    NiTStridedRandomAccessIterator<NiPoint2> kTexIter = 
        kTexCoords.begin<NiPoint2>();

    kPosIter[0] = NiPoint3(0, 0, 0);
    kPosIter[2] = NiPoint3(0, fBlockSize, 0);
    kPosIter[1] = NiPoint3(0, 0, fBlockSize);    
    kPosIter[3] = NiPoint3(0, fBlockSize, fBlockSize);

    kColIter[0] = NiColorA(1,1,1,1);
    kColIter[1] = NiColorA(1,1,1,1);
    kColIter[2] = NiColorA(1,1,1,1);
    kColIter[3] = NiColorA(1,1,1,1);

    NiPoint3 kNormal = NiPoint3::UNIT_Z;
    kNormIter[0] = kNormal;
    kNormIter[1] = kNormal;
    kNormIter[2] = kNormal;
    kNormIter[3] = kNormal;

    kTexIter[0] = NiPoint2(0, 1);
    kTexIter[2] = NiPoint2(0, 0);
    kTexIter[1] = NiPoint2(1, 1);
    kTexIter[3] = NiPoint2(1, 0);

    NiBound kMeshBound;
    NiPoint3 kMeshPoints[4] = {
        NiPoint3(0,0,0),
        NiPoint3(0,fBlockSize, 0),
        NiPoint3(0,0, fBlockSize),
        NiPoint3(0,fBlockSize, fBlockSize)};
    kMeshBound.ComputeFromData(4, kMeshPoints);

    NiMaterialPtr spMaterial = m_pkTerrain->GetMaterial();
    EE_ASSERT(spMaterial);

    // Create a mesh for each cell to render flat
    for (NiUInt32 uiIndex = 0; uiIndex < uiNumBlocks; ++uiIndex)
    {
        NiUInt32 uiRealIndex = GetCellOffset(uiNumLOD) + uiIndex;
        NiTerrainCell* pkCell = m_kCellRegionArray[uiRealIndex];
        EE_ASSERT(pkCell);
        
        QueueCellForLowDetailUpdate(pkCell);

        NiTextureRegion& kRegion = 
            pkCell->GetTextureRegion(NiTerrain::TextureType::LOWDETAIL_DIFFUSE);
        NiPoint2 kTextureOffset = kRegion.GetTextureOffset();

        NiMesh* pkMesh = NiNew NiMesh();
        pkMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRISTRIPS);
        pkMesh->SetSubmeshCount(1);        

        pkMesh->AddStreamRef(pkPositionStream, NiCommonSemantics::POSITION());
        pkMesh->AddStreamRef(pkColorStream, NiCommonSemantics::COLOR());
        pkMesh->AddStreamRef(pkNormalStream, NiCommonSemantics::NORMAL());
        pkMesh->AddStreamRef(pkTexCoordStream, NiCommonSemantics::TEXCOORD());

        kTextureOffset.y = 1.0f - kTextureOffset.y - fBlockSize;
        pkMesh->SetTranslate(10.0f, kTextureOffset.y, kTextureOffset.x);
        
        NiString kName;
        kName.Format("LowDetailTerrainCell %d", uiIndex);
        pkMesh->SetName((const char*)kName);
        pkMesh->SetModelBound(kMeshBound);

        pkMesh->ApplyAndSetActiveMaterial(spMaterial);
        pkScene->AttachChild(pkMesh);
    }

    NiVertexColorProperty* pkVertProp = NiNew NiVertexColorProperty();
    pkVertProp->SetSourceMode(NiVertexColorProperty::SOURCE_EMISSIVE);
    pkVertProp->SetLightingMode(NiVertexColorProperty::LIGHTING_E);
    pkScene->AttachProperty(pkVertProp);

    NiZBufferProperty* pkZBufferProp = NiNew NiZBufferProperty();
    pkZBufferProp->SetZBufferTest(false);
    pkZBufferProp->SetZBufferWrite(false);
    pkScene->AttachProperty(pkZBufferProp);

    NiAlphaProperty* pkAlphaProp = NiNew NiAlphaProperty();
    pkAlphaProp->SetAlphaTesting(false);
    pkAlphaProp->SetAlphaBlending(false);
    pkScene->AttachProperty(pkAlphaProp);

    pkScene->Update(0);
    pkScene->UpdateProperties();
    pkScene->UpdateEffects();

    return pkScene;
}

//---------------------------------------------------------------------------
NiViewRenderClick* NiTerrainSector::CreateLowDetailRenderClick(
    NiTerrainCell::TextureType::Value eTexType, NiNode* pkScene, NiCamera* pkCamera, 
    NiUInt32* puiErrorCode)
{
    if (pkScene == NULL)
    {
        *puiErrorCode |= NiTerrain::EC_LRT_CLICK_CREATION_FAILED;
        return NULL;
    }

    // Setup a render click to render to this target:
    NiViewRenderClick* pkClick = NiNew NiViewRenderClick();
    pkClick->SetClearAllBuffers(false);

    // Create an appropriate render view
    Ni3DRenderView* pkView = NiNew Ni3DRenderView(pkCamera,
        NiNew NiMeshCullingProcess(NULL, NULL));
    pkView->AppendScene(pkScene);
    pkClick->AppendRenderView(pkView);

    //Setup the texture required for a render target
    NiTexture* pkInitialTexture = GetTexture(eTexType);
    NiRenderedTexture* pkTexture = NiDynamicCast(NiRenderedTexture, 
        pkInitialTexture);
    if (!pkTexture)
    {
        pkTexture = CreateLowDetailTexture(eTexType,
            pkClick, m_uiLowDetailTextureSize, puiErrorCode);

        if (!pkTexture)
        {
            NiDelete pkClick;
            *puiErrorCode |= NiTerrain::EC_LRT_CLICK_CREATION_FAILED;
            return NULL;
        }
    }
    else if (pkTexture)
    {
        InitializeRenderedTexture(pkTexture, NULL, pkClick);
    }

    return pkClick;
}

//---------------------------------------------------------------------------
NiUInt32 NiTerrainSector::UpdateLowDetailScene(
    RuntimePaintingData* pkPaintingData, NiInt32& iMaxNumCells)
{
    EE_ASSERT(pkPaintingData);

    NiTPointerList<NiTerrainCell*>& kUpdateQueue = 
        pkPaintingData->m_kLowDetailRenderQueue;
    NiNode* pkScene = pkPaintingData->m_spLowDetailScene;
    NiCamera* pkCamera = pkPaintingData->m_spLowDetailCamera;

    // Skip this function is nothing is going to happen.
    if (kUpdateQueue.GetSize() == 0)
        return 0;

    // Common variable initialisation   
    NiUInt32 uiNumLOD = m_pkSectorData->GetNumLOD();
    NiUInt32 uiNumBlocksPerSide = 1 << uiNumLOD;

    // Divide the available pixels between the blocks.
    float fBlockSize = 1.0f / float(uiNumBlocksPerSide);

    // Adjust the camera's range to render only the relevant area.
    NiRect<float> kRegenView;
    // Calculate the region that requires redrawing:
    NiUInt32 uiBeginIndex = 0;
    NiUInt32 uiEndIndex = iMaxNumCells;

    if (uiEndIndex > kUpdateQueue.GetSize())
        uiEndIndex = kUpdateQueue.GetSize();

    for (NiUInt32 uiIndex = uiBeginIndex; uiIndex < uiEndIndex; ++uiIndex)
    {
        NiTerrainCell* pkCell;
        pkCell = kUpdateQueue.RemoveHead();
        NiUInt32 uiLODIndex = pkCell->GetRegionID();
        
        NiTextureRegion& kRegion = 
            pkCell->GetTextureRegion(NiTerrain::TextureType::LOWDETAIL_DIFFUSE);
        NiPoint2 kTextureOffset = kRegion.GetTextureOffset();

        NiMesh* pkMesh = NiDynamicCast(NiMesh, pkScene->GetAt(uiLODIndex));
        EE_ASSERT(pkMesh);
        pkMesh->SetMaterialNeedsUpdate(true);

        kTextureOffset.y = 1.0f - kTextureOffset.y - fBlockSize;
        if (kRegenView.GetHeight() == 0 && kRegenView.GetWidth() == 0)
        {
            kRegenView.m_left = kTextureOffset.x;
            kRegenView.m_bottom = kTextureOffset.y;
            kRegenView.m_right = kTextureOffset.x + fBlockSize;
            kRegenView.m_top = kTextureOffset.y + fBlockSize;
        }
        else
        {
            kRegenView.m_left = NiMin(kRegenView.m_left,
                kTextureOffset.x);
            kRegenView.m_bottom = NiMin(kRegenView.m_bottom,
                kTextureOffset.y);

            kRegenView.m_right = NiMax(kRegenView.m_right,
                kTextureOffset.x + fBlockSize);
            kRegenView.m_top = NiMax(kRegenView.m_top,
                kTextureOffset.y + fBlockSize);
        }

        pkMesh->SetTranslate(10.0f,kTextureOffset.y, kTextureOffset.x);
        pkMesh->RemoveProperty(NiTexturingProperty::GetType());
        NiTexturingProperty* pkTexProp = NiDynamicCast(NiTexturingProperty, 
            pkCell->GetMesh().GetProperty(NiTexturingProperty::GetType())->Clone());        
        // Filter out all the normals and paralax maps
        pkTexProp->SetBaseMap(0);
        pkTexProp->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
        pkMesh->AttachProperty(pkTexProp);

        // Update the material property
        pkMesh->RemoveProperty(NiMaterialProperty::GetType());
        pkMesh->RemoveProperty(NiVertexColorProperty::GetType());
        NiMesh& kRealMesh = pkCell->GetMesh();
        NiProperty* pkMatProp = kRealMesh.GetProperty(NiMaterialProperty::GetType());
        if (pkMatProp)
        {
            pkMesh->AttachProperty(pkMatProp);
            NiVertexColorProperty* pkVertProp = NiNew NiVertexColorProperty();
            pkVertProp->SetSourceMode(NiVertexColorProperty::SOURCE_IGNORE);
            pkVertProp->SetLightingMode(NiVertexColorProperty::LIGHTING_E);
            pkMesh->AttachProperty(pkVertProp);
        }

        // Steal copies of all the extradata
        pkMesh->RemoveAllExtraData();
        for (NiUInt32 uiDataIndex = 0; uiDataIndex < 
            (NiUInt32)kRealMesh.GetExtraDataSize(); ++ uiDataIndex)
        {
            NiExtraData* pkData = kRealMesh.GetExtraDataAt((unsigned short)uiDataIndex);
            // Ignore the render mode constant
            if (pkData->GetName() == NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT)
                continue;
            // Ignore cached shader constant data
            if (NiIsKindOf(NiSCMExtraData, pkData))
                continue;

            // Add the extra data to the mesh
            pkMesh->AddExtraData(pkData->GetName(), pkData);
        }
        pkMesh->AddExtraData(NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT,
            pkPaintingData->m_spLowDetailRenderMode);
        pkMesh->UpdateProperties();
    }

    // Ensure the viewport is within limits
    EE_ASSERT(kRegenView.m_bottom <= kRegenView.m_top);
    EE_ASSERT(kRegenView.m_left <= kRegenView.m_right);

    // Shrink the viewport and offset it so that there is a border of pixels 
    // for adjacent sectors
    float fPixelWidth = 1.0f / float(m_uiLowDetailTextureSize);
    float fBorderScale = 1.0f - 2.0f * fPixelWidth;
    kRegenView.m_left = kRegenView.m_left * fBorderScale;
    kRegenView.m_bottom = kRegenView.m_bottom * fBorderScale;
    kRegenView.m_right = kRegenView.m_right * fBorderScale + 2.0f * fPixelWidth;
    kRegenView.m_top = kRegenView.m_top * fBorderScale + 2.0f * fPixelWidth;

    // Lock the viewport to the pixel grid (the rasterizer will do this anyway, 
    // but we need to calculate the frustum from this viewport, so we need to 
    // be sure of what pixels will be rendered)
    kRegenView.m_left = 
        NiFloor(kRegenView.m_left / fPixelWidth) * fPixelWidth;
    kRegenView.m_bottom = 
        NiFloor(kRegenView.m_bottom / fPixelWidth) * fPixelWidth;
    kRegenView.m_right = 
        (NiFloor(kRegenView.m_right / fPixelWidth) + 1.0f) * fPixelWidth;
    kRegenView.m_top = 
        (NiFloor(kRegenView.m_top / fPixelWidth) + 1.0f) * fPixelWidth;

    // Clamp to the max viewport
    // (This is necessary because of the absence of an appropriate NiCeil 
    // function to use in the last two calculations above, so 1 is simply
    // added blindly, and so, it may cause the region to go outside the 
    // valid range of a viewport)
    kRegenView.m_left   = NiClamp(kRegenView.m_left, 0.0f, 1.0f);
    kRegenView.m_right  = NiClamp(kRegenView.m_right, 0.0f, 1.0f);
    kRegenView.m_top    = NiClamp(kRegenView.m_top, 0.0f, 1.0f);
    kRegenView.m_bottom = NiClamp(kRegenView.m_bottom, 0.0f, 1.0f);
    
    // Generate the camera frustum from this render view
    NiFrustum kFrustum(kRegenView.m_left, kRegenView.m_right,
        kRegenView.m_top, kRegenView.m_bottom, 0, 100, true);

    // Add pixels to the border of the frustum 
    // NOTE: we are working in world space now (scaling the world into the
    // texture)
    float fWorldPixelWidth = 1.0f / float(m_uiLowDetailTextureSize - 2);
    float fFrustumMin = -fWorldPixelWidth;
    float fFrustumMax = 1.0f + fWorldPixelWidth;
    kFrustum.m_fLeft   = NiLerp(kFrustum.m_fLeft, fFrustumMin, fFrustumMax);
    kFrustum.m_fRight  = NiLerp(kFrustum.m_fRight, fFrustumMin, fFrustumMax);
    kFrustum.m_fTop    = NiLerp(kFrustum.m_fTop, fFrustumMin, fFrustumMax);
    kFrustum.m_fBottom = NiLerp(kFrustum.m_fBottom, fFrustumMin, fFrustumMax);

    // Configure the camera
    pkCamera->SetViewFrustum(kFrustum);
    pkCamera->SetViewPort(kRegenView);
    pkCamera->SetTranslate(NiPoint3(-10.0f, 0.0f, 0.0f));

    // Return the number of cells updated
    return uiEndIndex - uiBeginIndex;
}

//---------------------------------------------------------------------------
void NiTerrainSector::GenerateLeafMaskUsageData(
    efd::vector<RuntimePaintingData::LeafMaskUsage>& kLeafUsage)
{
    // Loop over all the cells
    // Create a scene to use during rendering of the low detail texture
    // Common variable initialisation
    NiUInt32 uiNumLOD = m_pkSectorData->GetNumLOD();
    NiUInt32 uiNumBlocksPerSide = 1 << uiNumLOD;
    NiUInt32 uiNumBlocks = (NiUInt32)NiSqr((float)uiNumBlocksPerSide);

    // Loop through each leaf
    for (NiUInt32 uiIndex = 0; uiIndex < uiNumBlocks; ++uiIndex)
    {
        // Blank out a usage structure for this leaf
        RuntimePaintingData::LeafMaskUsage kUsage;
        for (efd::UInt32 uiSurfaceIndex = 0; uiSurfaceIndex < NiTerrainCellLeaf::MAX_NUM_SURFACES; 
            ++uiSurfaceIndex)
        {
            kUsage.m_uiSurfaceUsage[uiSurfaceIndex] = 0;
        }

        // Fetch the leaf:
        NiUInt32 uiRealIndex = GetCellOffset(uiNumLOD) + uiIndex;
        NiTerrainCell* pkCell = m_kCellArray[uiRealIndex];
        NiTerrainCellLeaf* pkLeaf = NiDynamicCast(NiTerrainCellLeaf, pkCell);
        EE_ASSERT(pkLeaf);

        // Obtain access to the blend mask texture's data
        NiTextureRegion kBlendTextureRegion = 
            pkLeaf->GetTextureRegion(NiTerrainCell::TextureType::BLEND_MASK);
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture,
            kBlendTextureRegion.GetTexture());
        EE_ASSERT(pkTexture);
        NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();
        efd::UInt8* pucPixels = pkPixelData->GetPixels();

        // Fetch the range over which to modify this texture
        NiIndex kStartPoint = kBlendTextureRegion.GetStartPixelIndex();
        NiIndex kEndPoint = kBlendTextureRegion.GetEndPixelIndex();

        // Include the border pixels
        kStartPoint -= NiIndex(1,1);
        kEndPoint += NiIndex(1,1);

        // Iterate over the mask's data
        for (NiUInt32 uiY = kStartPoint.y; uiY < kEndPoint.y; ++uiY)
        {
            for (NiUInt32 uiX = kStartPoint.x; uiX < kEndPoint.x; ++uiX)
            {
                efd::UInt32 uiPixelIndex = uiX + uiY * pkPixelData->GetWidth();
                // Iterate over each channel
                for (efd::UInt32 uiSurfaceIndex = 0; 
                    uiSurfaceIndex < pkPixelData->GetPixelStride(); ++uiSurfaceIndex)
                {
                    efd::UInt8 uiSample = 
                        pucPixels[uiPixelIndex * pkPixelData->GetPixelStride() + uiSurfaceIndex];
                    kUsage.m_uiSurfaceUsage[uiSurfaceIndex] += ((uiSample == 0) ? 0 : 1);
                }
            }
        }

        // Iterate over the data for the unused surfaces and blank those counts out
        for (efd::UInt32 uiSurfaceIndex = pkLeaf->GetSurfaceCount(); 
            uiSurfaceIndex < NiTerrainCellLeaf::MAX_NUM_SURFACES; ++uiSurfaceIndex)
        {
            kUsage.m_uiSurfaceUsage[uiSurfaceIndex] = 0;
        }

        // Add this coverage data.
        EE_ASSERT((pkCell->GetCellID() - GetCellOffset(uiNumLOD)) == uiIndex);
        EE_ASSERT(kLeafUsage.size() == uiIndex);
        kLeafUsage.push_back(kUsage);
    }
}

//---------------------------------------------------------------------------
efd::UInt32 NiTerrainSector::GetLeafMaskUsage(efd::UInt32 uiCellID, efd::UInt32 uiChannel)
{
    EE_ASSERT(m_pkPaintingData);
    efd::UInt32 uiLeafID = uiCellID - GetCellOffset(m_pkSectorData->GetNumLOD());
    return m_pkPaintingData->m_kLeafMaskUsage[uiLeafID].m_uiSurfaceUsage[uiChannel];
}

//---------------------------------------------------------------------------
void NiTerrainSector::SetLeafMaskUsage(efd::UInt32 uiCellID, efd::UInt32 uiChannel, 
    efd::UInt32 uiUsage)
{
    EE_ASSERT(m_pkPaintingData);
    efd::UInt32 uiLeafID = uiCellID - GetCellOffset(m_pkSectorData->GetNumLOD());
    m_pkPaintingData->m_kLeafMaskUsage[uiLeafID].m_uiSurfaceUsage[uiChannel] = uiUsage;
}

//---------------------------------------------------------------------------
bool NiTerrainSector::CreatePaintingData(NiUInt32* puiErrorCode)
{
    // Don't create more than one painting data object
    if (m_pkPaintingData)
        return true;

    // Make sure the terrain has a painting data object
    if (!m_pkTerrain->CreatePaintingData())
    {
        *puiErrorCode |= NiTerrain::EC_LRT_RENDERING_FAILED;
        return false;
    }
    NiNode* pkTerrainLowDetailScene = m_pkTerrain->GetLowDetailScene();

    // Create new painting data
    RuntimePaintingData* pkData = NiNew RuntimePaintingData();
    m_pkPaintingData = pkData;

    // Generate the appropriate clicks and textures
    pkData->m_spLowDetailScene = CreateSectorLowDetailScene(puiErrorCode);
    pkData->m_spLowDetailCamera = NiNew NiCamera();
    pkData->m_spLowDetailRenderMode = NiNew NiIntegerExtraData(0);
    pkData->m_spLowDetailDiffuseRenderClick = CreateLowDetailRenderClick(
        NiTerrain::TextureType::LOWDETAIL_DIFFUSE,
        pkTerrainLowDetailScene, pkData->m_spLowDetailCamera, puiErrorCode);

    // Check that the data is complete
    bool bComplete = true;
    bComplete &= pkData->m_spLowDetailScene != NULL;
    bComplete &= pkData->m_spLowDetailCamera != NULL;
    bComplete &= pkData->m_spLowDetailRenderMode != NULL;
    bComplete &= pkData->m_spLowDetailDiffuseRenderClick != NULL;

    // Free the data if it was a failure
    if (!bComplete)
    {
        m_pkPaintingData = NULL;
        NiDelete pkData;
        *puiErrorCode |= NiTerrain::EC_LRT_RENDERING_FAILED;
        return false;
    }

    // Configure the render steps
    pkData->m_spLowDetailRenderMode->SetValue(NiTerrainCellShaderData::BAKE_DIFFUSE);

    // Register the sector's scene with the parent terrain.
    NiInt16 sSectorX, sSectorY;
    GetSectorIndex(sSectorX, sSectorY);
    pkData->m_spLowDetailScene->SetTranslate(0, 
        (float)(sSectorY), (float)sSectorX);

    pkTerrainLowDetailScene->AttachChild(pkData->m_spLowDetailScene);

    // Attach the camera to this sector's scene
    pkData->m_spLowDetailScene->AttachChild(pkData->m_spLowDetailCamera);

    // Do a pass over all the leaf blend masks and count the leaf mask usage
    GenerateLeafMaskUsageData(pkData->m_kLeafMaskUsage);

    return bComplete;
}

//---------------------------------------------------------------------------
bool NiTerrainSector::UpdatePaintingData(NiInt32& iMaxNumCells, NiUInt32* puiErrorCode)
{
    bool bAppendRenderClicks = false;
    
    // Make sure the painting context is ready to update
    RuntimePaintingData*& pkData = m_pkPaintingData;
    if (!IsReadyToPaint())
    {
        if (!CreatePaintingData(puiErrorCode))
            return false;
    }
    else
    {
        // Check that the texture sizes are correct
        NiRenderTargetGroup* pkTarget = 
            pkData->m_spLowDetailDiffuseRenderClick->GetRenderTargetGroup();
        EE_ASSERT(pkTarget);
        Ni2DBuffer* pkBuffer = pkTarget->GetBuffer(0);

        // Recreate the textures if required
        if (m_uiLowDetailTextureSize != pkBuffer->GetWidth())
        {
            NiTexture* pkTexture = CreateLowDetailTexture(NiTerrain::TextureType::LOWDETAIL_DIFFUSE,
                pkData->m_spLowDetailDiffuseRenderClick, m_uiLowDetailTextureSize, puiErrorCode);

            if (!pkTexture)
                return false;
        }
    }

    // Check for any rendering initialization that may be required
    NiUInt32 uiExtraViews = 
        (pkData->m_spLowDetailDiffuseRenderClick->GetRenderViews().GetSize() - 
        1);
    bAppendRenderClicks |= uiExtraViews > 0;

    // Update the render's scene
    NiUInt32 uiRenderedCells = UpdateLowDetailScene(pkData, iMaxNumCells);
    if (uiRenderedCells)
    {
        iMaxNumCells -= uiRenderedCells;
        bAppendRenderClicks |= true;

        // We now need to clear all the cached geometry from the render views
        // Not doing so would render the same geometry when this function is
        // called several times in the same frame (happens in scene designer)
        NiTPointerList<NiRenderViewPtr>& kViewList = 
            pkData->m_spLowDetailDiffuseRenderClick->GetRenderViews();
        NiTListIterator kIter = kViewList.GetHeadPos();
        while (kIter)
        {
            NiRenderViewPtr spRenderView = kViewList.GetNext(kIter);
            spRenderView->ClearCachedPVGeometry();
        }
    }

    return bAppendRenderClicks;
}

//---------------------------------------------------------------------------
bool NiTerrainSector::AppendRenderClicks(NiDefaultClickRenderStep* pkStep, 
    NiInt32& iMaxNumCells, NiUInt32* puiErrorCode)
{
    // Are we even fully loaded?
    if (!m_pkSectorData->IsLODLoaded(m_pkSectorData->GetNumLOD()))
        return true;

    // Update painting data
    if (UpdatePaintingData(iMaxNumCells, puiErrorCode))
    {
        pkStep->AppendRenderClick(m_pkPaintingData->m_spLowDetailDiffuseRenderClick);
    }
    else
    {
        if (*puiErrorCode & NiTerrain::EC_LRT_RENDERING_FAILED)
            return false;
    }

    return true;
}

//---------------------------------------------------------------------------
NiNode* NiTerrainSector::GetLowDetailScene()
{
    if (IsReadyToPaint())
        return m_pkPaintingData->m_spLowDetailScene;
    return NULL;
}

//---------------------------------------------------------------------------
NiTerrainSector::NiTerrainSectorPagingData::NiTerrainSectorPagingData():
    m_bWaitingForUpdate(false)
{

}

//---------------------------------------------------------------------------
NiTerrainSector::NiTerrainSectorPagingData::~NiTerrainSectorPagingData()
{

}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::PauseUpdateThread()
{
    // Wait untill we are ready to perform the operation
    m_bWaitingForUpdate = true;
    m_kUpdatePaused.Wait();
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::SyncWithPaging()
{
    if (m_bWaitingForUpdate)
    {
        // Signal we can now remove the sector
        m_bWaitingForUpdate = false;
        m_kUpdatePaused.Signal();
        
        // Wait for the operation to be completed
        m_kResumeUpdate.Wait();
    }
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::ResumeUpdateThread()
{
    // Signal we have finished the operation
    m_kResumeUpdate.Signal();
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::LockSectorPaging()
{
    m_kSectorPages.Lock();
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::UnlockSectorPaging()
{
    m_kSectorPages.Unlock();
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::LockSectorUpdating()
{
    m_kSectorUpdates.Lock();
}

//---------------------------------------------------------------------------
void NiTerrainSector::NiTerrainSectorPagingData::UnlockSectorUpdating()
{
    m_kSectorUpdates.Unlock();
}

//---------------------------------------------------------------------------
NiTerrainResourceManager* NiTerrainSector::GetResourceManager()
{
    return m_pkTerrain->GetResourceManager();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSector::NotifyHeightMapRegionChanged(NiRect<NiInt32> kRegion)
{
    NiTerrainStreamingManager* pkStreaming = GetTerrain()->GetStreamingManager();
    pkStreaming->RequestRebuildSector(this, kRegion);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::GetSurfaceMask(efd::UInt32 uiSurfaceIndex, SurfaceMaskBuffer* pkMaskBuffer, 
    SurfaceMaskBuffer* pkMaskSumBuffer, NiTerrainDataSnapshot* pkSnapshot)
{
    EE_ASSERT(pkMaskBuffer);

    // Precondition: Sector is completely loaded
    if (!m_pkSectorData->IsLODLoaded(m_pkTerrain->GetNumLOD()))
        return true;

    // Fetch the mask region
    NiRect<efd::SInt32> kMaskRegion = pkMaskBuffer->GetDataRegion();

    // The given mask region identifies pixels in other sectors as well, extend it in those 
    // directions. 
    efd::SInt16 sSectX, sSectY;
    GetSectorIndex(sSectX, sSectY);
    NiRect<efd::SInt32> kAffectedSectors = pkMaskBuffer->GetAffectedSectorRange();
    efd::SInt32 iSectorPixels = m_pkTerrain->GetMaskSize() - (2 << m_pkTerrain->GetNumLOD());
    kMaskRegion.m_left += iSectorPixels * (kAffectedSectors.m_left - sSectX); 
    kMaskRegion.m_right += iSectorPixels * (kAffectedSectors.m_right - sSectX);
    kMaskRegion.m_top += iSectorPixels * (kAffectedSectors.m_top - sSectY);
    kMaskRegion.m_bottom += iSectorPixels * (kAffectedSectors.m_bottom - sSectY);
    EE_ASSERT(kMaskRegion.GetWidth() + 1 == efd::SInt32(pkMaskBuffer->GetWidth()));
    EE_ASSERT(kMaskRegion.GetHeight() + 1 == efd::SInt32(pkMaskBuffer->GetHeight()));

    // Figure out which cells this region will affect:
    efd::SInt32 iPixelsPerCell = (efd::SInt32(m_pkTerrain->GetMaskSize()) / 
        (1 << m_pkTerrain->GetNumLOD())) - 2;
    NiRect<efd::SInt32> kCellRegion;
    kCellRegion.m_left = kMaskRegion.m_left / iPixelsPerCell;
    kCellRegion.m_right = kMaskRegion.m_right / iPixelsPerCell;
    kCellRegion.m_top = kMaskRegion.m_top / iPixelsPerCell;
    kCellRegion.m_bottom = kMaskRegion.m_bottom / iPixelsPerCell;

    // Clamp the cell range to the legal range for this sector
    NiUInt32 uiNumBlocksAcross = 1 << m_pkTerrain->GetNumLOD();
    kCellRegion.m_left = NiClamp(kCellRegion.m_left, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_right = NiClamp(kCellRegion.m_right, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_top = NiClamp(kCellRegion.m_top, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_bottom = NiClamp(kCellRegion.m_bottom, 0, uiNumBlocksAcross - 1);

    // Fetch the buffers we are populating
    efd::UInt8* pucMask = pkMaskBuffer->GetBuffer();
    EE_ASSERT(pucMask);
    efd::UInt8* pucMaskSum = NULL;
    if (pkMaskSumBuffer)
    {
        pucMaskSum = pkMaskSumBuffer->GetBuffer();
        EE_ASSERT(pucMaskSum);
    }
    
    // Update every relevant cell's blend mask data
    for (efd::SInt32 iCellX = kCellRegion.m_left; 
        iCellX <= kCellRegion.m_right; ++iCellX)
    {
        for (efd::SInt32 iCellY = kCellRegion.m_bottom; 
            iCellY <= kCellRegion.m_top; ++iCellY)
        {
            // Calculate the ID of the cell
            NiUInt32 uiCellID = iCellX + iCellY * uiNumBlocksAcross;
            uiCellID += GetCellOffset(m_pkTerrain->GetNumLOD());
            // Fetch this cell, its blend mask region and texture
            NiTerrainCellLeaf* pkCell = NiDynamicCast(NiTerrainCellLeaf, GetCell(uiCellID));
            EE_ASSERT(pkCell);

            // Grab a snapshot of this cell's data
            if (pkSnapshot && !pkSnapshot->ContainsData(CellID(GetSectorID(), uiCellID), 
                NiTerrainDataSnapshot::BufferType::SURFACE_MASK))
            {
                // Add this cell's current data buffers in to the snapshot
                pkCell->TakeSnapshot(pkSnapshot, NiTerrainDataSnapshot::BufferType::SURFACE_MASK);
            }

            // Find the channel of this surface
            efd::UInt32 uiMaskChannel = 0;
            pkCell->GetSurfacePriority(uiSurfaceIndex, uiMaskChannel);

            // Fetch the texture and the region this cell uses
            NiTextureRegion kTexRegion = pkCell->GetTextureRegion(
                NiTerrainCell::TextureType::BLEND_MASK);
            NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, 
                pkCell->GetTexture(NiTerrainCell::TextureType::BLEND_MASK));
            EE_ASSERT(pkTexture);
            NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();
            EE_ASSERT(pkPixelData);
            efd::UInt8* pucPixels = pkPixelData->GetPixels();

            // Figure out the region of the texture dedicated to this cell
            NiIndex kCellPixelOffset = kTexRegion.GetStartPixelIndex();

            // Figure out the region of the cell to copy into the buffer
            NiRect<efd::SInt32> kPixelRegion;
            kPixelRegion.m_left = kMaskRegion.m_left - iPixelsPerCell * iCellX;
            kPixelRegion.m_right = kMaskRegion.m_right -  iPixelsPerCell * iCellX;
            kPixelRegion.m_top = kMaskRegion.m_top - iPixelsPerCell * iCellY;
            kPixelRegion.m_bottom = kMaskRegion.m_bottom - iPixelsPerCell * iCellY;
            
            // Figure out the offset of this cell's region to the overall buffer
            efd::SInt32 kCellBufferOffsetX, kCellBufferOffsetY;
            kCellBufferOffsetX = -kPixelRegion.m_left;
            kCellBufferOffsetY = -kPixelRegion.m_bottom;

            // Clamp the region to allowed range
            kPixelRegion.m_left = NiClamp(kPixelRegion.m_left, 0, iPixelsPerCell - 1);
            kPixelRegion.m_right = NiClamp(kPixelRegion.m_right, 0, iPixelsPerCell - 1);
            kPixelRegion.m_top = NiClamp(kPixelRegion.m_top, 0, iPixelsPerCell - 1);
            kPixelRegion.m_bottom = NiClamp(kPixelRegion.m_bottom, 0, iPixelsPerCell - 1);

            // Copy the relevant pixels into the buffer 
            for (efd::SInt32 iY = kPixelRegion.m_bottom; 
                iY <= kPixelRegion.m_top; ++iY)
            {
                for (efd::SInt32 iX = kPixelRegion.m_left; 
                    iX <= kPixelRegion.m_right; ++iX)
                {
                    // Calculate the index of the pixel
                    efd::SInt32 iSampleIndex = (iX + kCellPixelOffset.x) + 
                        (pkPixelData->GetHeight() - 1 - (iY + kCellPixelOffset.y)) * 
                        pkPixelData->GetWidth();
                    iSampleIndex *= pkPixelData->GetPixelStride();

                    // Calculate the sum of all the samples
                    efd::UInt8 ucSample = 0;
                    efd::UInt32 uiSampleSum = 0;
                    for (efd::UInt32 uiC = 0; uiC < pkCell->GetSurfaceCount(); ++uiC)
                    {
                        // Sample the relevant channel
                        if (uiC == uiMaskChannel)
                            ucSample = pucPixels[iSampleIndex + uiC];

                        // Update the sum
                        uiSampleSum += pucPixels[iSampleIndex + uiC];
                    }   

                    // Buffer the values
                    efd::SInt32 iBufferIndex = (iX + kCellBufferOffsetX) +
                        ((iY + kCellBufferOffsetY) * pkMaskBuffer->GetWidth());
                    if (pucMask)
                        pucMask[iBufferIndex] = ucSample;
                    if (pucMaskSum)
                        pucMaskSum[iBufferIndex] = efd::UInt8(efd::Clamp(uiSampleSum, 0, 255));
                }
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::SetSurfaceMask(efd::UInt32 uiSurfaceIndex, SurfaceMaskBuffer* pkMaskBuffer,
    SurfaceMaskBuffer* pkMaskSumBuffer)
{   
    EE_ASSERT(pkMaskBuffer);

    // Precondition: Sector is completely loaded
    if (!m_pkSectorData->IsLODLoaded(m_pkTerrain->GetNumLOD()))
        return true;

    // Make sure that we are prepared for painting
    if (!IsReadyToPaint())
    {
        efd::UInt32 uiErrorCode = 0;
        if (!CreatePaintingData(&uiErrorCode))
            return false;
    }

    // Fetch the mask region
    NiRect<efd::SInt32> kMaskRegion = pkMaskBuffer->GetDataRegion();

    // The given mask region identifies pixels in other sectors as well, extend it in those 
    // directions. 
    efd::SInt16 sSectX, sSectY;
    GetSectorIndex(sSectX, sSectY);
    NiRect<efd::SInt32> kAffectedSectors = pkMaskBuffer->GetAffectedSectorRange();
    efd::SInt32 iSectorPixels = m_pkTerrain->GetMaskSize() - (2 << m_pkTerrain->GetNumLOD());
    kMaskRegion.m_left += iSectorPixels * (kAffectedSectors.m_left - sSectX); 
    kMaskRegion.m_right += iSectorPixels * (kAffectedSectors.m_right - sSectX);
    kMaskRegion.m_top += iSectorPixels * (kAffectedSectors.m_top - sSectY);
    kMaskRegion.m_bottom += iSectorPixels * (kAffectedSectors.m_bottom - sSectY);
    EE_ASSERT(kMaskRegion.m_right - kMaskRegion.m_left + 1 == efd::SInt32(pkMaskBuffer->GetWidth()));
    EE_ASSERT(kMaskRegion.m_top - kMaskRegion.m_bottom + 1 == efd::SInt32(pkMaskBuffer->GetHeight()));

    // Figure out which cells this region will affect:
    efd::SInt32 iPixelsPerCell = (efd::SInt32(m_pkTerrain->GetMaskSize()) / 
        (1 << m_pkTerrain->GetNumLOD())) - 2;
    NiRect<efd::SInt32> kCellRegion;
    kCellRegion.m_left = kMaskRegion.m_left / iPixelsPerCell;
    kCellRegion.m_right = kMaskRegion.m_right / iPixelsPerCell;
    kCellRegion.m_top = kMaskRegion.m_top / iPixelsPerCell;
    kCellRegion.m_bottom = kMaskRegion.m_bottom / iPixelsPerCell;

    // Detect if the cell range needs expanding to include duplicated pixels
    if (kMaskRegion.m_left == kCellRegion.m_left * iPixelsPerCell)
        kCellRegion.m_left -= 1;
    if (kMaskRegion.m_bottom == kCellRegion.m_bottom * iPixelsPerCell)
        kCellRegion.m_bottom -= 1;
    if (kMaskRegion.m_right == (kCellRegion.m_right + 1) * iPixelsPerCell - 1)
        kCellRegion.m_right += 1;
    if (kMaskRegion.m_top == (kCellRegion.m_top + 1) * iPixelsPerCell - 1)
        kCellRegion.m_top += 1;

    // Clamp the cell range to the legal range for this sector
    NiUInt32 uiNumBlocksAcross = 1 << m_pkTerrain->GetNumLOD();
    kCellRegion.m_left = NiClamp(kCellRegion.m_left, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_right = NiClamp(kCellRegion.m_right, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_top = NiClamp(kCellRegion.m_top, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_bottom = NiClamp(kCellRegion.m_bottom, 0, uiNumBlocksAcross - 1);

    // Fetch the buffers we are populating from
    efd::UInt8* pucMask = pkMaskBuffer->GetBuffer();
    EE_ASSERT(pucMask);

    efd::UInt8* pucMaskSum = NULL;
    if (pkMaskSumBuffer)
        pucMaskSum = pkMaskSumBuffer->GetBuffer();

    // Update every relevant cell's blend mask data
    for (efd::SInt32 iCellX = kCellRegion.m_left; 
        iCellX <= kCellRegion.m_right; ++iCellX)
    {
        for (efd::SInt32 iCellY = kCellRegion.m_bottom; 
            iCellY <= kCellRegion.m_top; ++iCellY)
        {
            // Calculate the ID of the cell
            NiUInt32 uiLeafID = iCellX + iCellY * uiNumBlocksAcross;
            NiUInt32 uiCellID = uiLeafID + GetCellOffset(m_pkTerrain->GetNumLOD());
            // Fetch this cell, its blend mask region and texture
            NiTerrainCellLeaf* pkCell = NiDynamicCast(NiTerrainCellLeaf, GetCell(uiCellID));
            EE_ASSERT(pkCell);

            // Find the channel of this surface
            efd::UInt32 uiMaskChannel = 0;
            if (!pkCell->GetSurfacePriority(uiSurfaceIndex, uiMaskChannel))
            {
                // This cell does not have this surface - skip it
                continue;
            }

            // Fetch the texture and the region this cell uses
            NiTextureRegion kTextureRegion = pkCell->GetTextureRegion(
                NiTerrainCell::TextureType::BLEND_MASK);
            NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, 
                pkCell->GetTexture(NiTerrainCell::TextureType::BLEND_MASK));
            EE_ASSERT(pkTexture);
            NiPixelData* pkPixelData = pkTexture->GetSourcePixelData();
            EE_ASSERT(pkPixelData);
            efd::UInt8* pucPixels = pkPixelData->GetPixels();

            // Figure out the region of the texture dedicated to this cell
            NiIndex kCellPixelOffset = kTextureRegion.GetStartPixelIndex();

            // Figure out the region of the cell to copy into the buffer
            NiRect<efd::SInt32> kPixelRegion;
            kPixelRegion.m_left = kMaskRegion.m_left - iPixelsPerCell * iCellX;
            kPixelRegion.m_right = kMaskRegion.m_right -  iPixelsPerCell * iCellX;
            kPixelRegion.m_top = kMaskRegion.m_top - iPixelsPerCell * iCellY;
            kPixelRegion.m_bottom = kMaskRegion.m_bottom - iPixelsPerCell * iCellY;

            // Figure out the offset of this cell's region to the overall buffer
            efd::SInt32 kCellBufferOffsetX, kCellBufferOffsetY;
            kCellBufferOffsetX = -kPixelRegion.m_left;
            kCellBufferOffsetY = -kPixelRegion.m_bottom;

            // Clamp the region to allowed range
            kPixelRegion.m_left = NiClamp(kPixelRegion.m_left, -1, iPixelsPerCell);
            kPixelRegion.m_right = NiClamp(kPixelRegion.m_right, -1, iPixelsPerCell);
            kPixelRegion.m_top = NiClamp(kPixelRegion.m_top, -1, iPixelsPerCell);
            kPixelRegion.m_bottom = NiClamp(kPixelRegion.m_bottom, -1, iPixelsPerCell);

            // Detect which borders of this cell require pixel duplication 
            // Query all the neighboring cells to see if they have the surface on them
            PixelPaintMode::VALUE aeAreaPaintMode[3][3] = {
                {PixelPaintMode::NORMAL, PixelPaintMode::NORMAL, PixelPaintMode::NORMAL},
                {PixelPaintMode::NORMAL, PixelPaintMode::NORMAL, PixelPaintMode::NORMAL},
                {PixelPaintMode::NORMAL, PixelPaintMode::NORMAL, PixelPaintMode::NORMAL}};
            NiUInt32 uiHorizontalBorder;
            NiUInt32 uiVerticalBorder;
            for (NiUInt32 uiBorderY = 0; uiBorderY < 3; ++uiBorderY)
            {
                // Figure out which border to look at
                uiVerticalBorder = NiTerrainCell::BORDER_NONE;
                if (uiBorderY == 0)
                    uiVerticalBorder = NiTerrainCell::BORDER_BOTTOM;
                if (uiBorderY == 2)
                    uiVerticalBorder = NiTerrainCell::BORDER_TOP;
                for (NiUInt32 uiBorderX = 0; uiBorderX < 3; ++uiBorderX)
                {
                    // Figure out which border to look at
                    uiHorizontalBorder = NiTerrainCell::BORDER_NONE;
                    if (uiBorderX == 0)
                        uiHorizontalBorder = NiTerrainCell::BORDER_LEFT;
                    if (uiBorderX == 2)
                        uiHorizontalBorder = NiTerrainCell::BORDER_RIGHT;
                    
                    // Allow the leaf to be NULL (allows sector borders to be painted when 
                    // adjacent sector does not exist)
                    NiUInt32 uiAdjChannel;
                    NiTerrainCellLeaf* pkAdjLeaf = NiDynamicCast(NiTerrainCellLeaf, 
                        pkCell->GetAdjacent(uiVerticalBorder | uiHorizontalBorder));
                    PixelPaintMode::VALUE ePaintMode;
                    if (!pkAdjLeaf)
                    {// The adjacent leaf doesn't exist so copy our data over that area
                        ePaintMode = PixelPaintMode::DUPLICATE_LOCAL;
                    }
                    else if (pkAdjLeaf && 
                        pkAdjLeaf->GetSurfacePriority(uiSurfaceIndex, uiAdjChannel))
                    {// The adjacent leaf exists and has our surface
                        ePaintMode = PixelPaintMode::NORMAL;
                    }
                    else 
                    {// The adjacent leaf exists and doesn't have our surface - don't paint here
                        ePaintMode = PixelPaintMode::NONE;
                    }

                    if (uiBorderY == 1 && uiBorderX != 1)
                    {// We just tested a cell that was shared with the corners
                        aeAreaPaintMode[uiBorderX][0] = (std::min)(
                            aeAreaPaintMode[uiBorderX][0], ePaintMode);
                        aeAreaPaintMode[uiBorderX][2] = (std::min)(
                            aeAreaPaintMode[uiBorderX][2], ePaintMode);
                    }
                    else if (uiBorderY != 1 && uiBorderX == 1)
                    {// We just tested a cell that was shared with the corners
                        aeAreaPaintMode[0][uiBorderY] = (std::min)(
                            aeAreaPaintMode[0][uiBorderY], ePaintMode);
                        aeAreaPaintMode[2][uiBorderY] = (std::min)(
                            aeAreaPaintMode[2][uiBorderY], ePaintMode);
                    }
                    aeAreaPaintMode[uiBorderX][uiBorderY] = (std::min)(
                        aeAreaPaintMode[uiBorderX][uiBorderY], ePaintMode);
                }
            }

            // Fetch the mask usage stats for this leaf
            RuntimePaintingData::LeafMaskUsage& kMaskUsage = 
                m_pkPaintingData->m_kLeafMaskUsage[uiLeafID];

            // Copy the relevant pixels into the buffer
            for (efd::SInt32 iY = kPixelRegion.m_bottom; 
                iY <= kPixelRegion.m_top; ++iY)
            {
                // Detect if we can duplicate this row of pixels
                efd::SInt32 iBorderIndexY(
                    efd::SInt32(NiFloor(float(iY - 1) / float(iPixelsPerCell - 2))) + 1);
                if (aeAreaPaintMode[1][iBorderIndexY] == PixelPaintMode::NONE)
                    continue;

                for (efd::SInt32 iX = kPixelRegion.m_left; 
                    iX <= kPixelRegion.m_right; ++iX)
                {
                    // Detect if we can duplicate this pixel in this row
                    efd::SInt32 iBorderIndexX(
                        efd::SInt32(NiFloor(float(iX - 1) / float(iPixelsPerCell - 2))) + 1);
                    PixelPaintMode::VALUE ePaintMode = 
                        aeAreaPaintMode[iBorderIndexX][iBorderIndexY];
                    if (ePaintMode == PixelPaintMode::NONE)
                        continue;
                        
                    // Select which pixel to sample.
                    efd::SInt32 iCopyX = iX;
                    efd::SInt32 iCopyY = iY;

                    // If any of the borders for this pixel say to duplicate local then clamp
                    // appropriately
                    if (aeAreaPaintMode[iBorderIndexX][1] != PixelPaintMode::NORMAL &&
                        ePaintMode == PixelPaintMode::DUPLICATE_LOCAL)
                        iCopyX = NiClamp(iCopyX, 0, iPixelsPerCell - 1);
                    if (aeAreaPaintMode[1][iBorderIndexY] != PixelPaintMode::NORMAL &&
                        ePaintMode == PixelPaintMode::DUPLICATE_LOCAL)
                        iCopyY = NiClamp(iCopyY, 0, iPixelsPerCell - 1);

                    // Transform these coordinates to the buffer's coordinates
                    iCopyX += kCellBufferOffsetX;
                    iCopyY += kCellBufferOffsetY;

                    // If the value we selected is outside the buffer's range
                    // then that value hasn't changed anyway. So don't paint this pixel
                    if (iCopyX != NiClamp(iCopyX, 0, pkMaskBuffer->GetWidth() - 1) ||
                        iCopyY != NiClamp(iCopyY, 0, pkMaskBuffer->GetHeight() - 1))
                        continue;

                    // Figure out where these values will be put
                    efd::SInt32 iTextureIndex = (iX + kCellPixelOffset.x) + 
                        (pkPixelData->GetHeight() - 1 - (iY + kCellPixelOffset.y)) * 
                        pkPixelData->GetWidth();
                    iTextureIndex *= pkPixelData->GetPixelStride();

                    // Figure out the current sum of the pixels
                    efd::UInt32 uiCurrentSampleSum = 0;
                    efd::UInt8 ucCurrentSample = 0;
                    for (efd::UInt32 uiC = 0; uiC < pkCell->GetSurfaceCount(); ++uiC)
                    {
                        // Sample the relevant channel
                        if (uiC == uiMaskChannel)
                            ucCurrentSample = pucPixels[iTextureIndex + uiC];
                        // Update the sum
                        uiCurrentSampleSum += pucPixels[iTextureIndex + uiC];
                    }

                    // Sample the selected pixel
                    efd::SInt32 iBufferIndex = iCopyX + (iCopyY * pkMaskBuffer->GetWidth());
                    efd::UInt8 ucSample = pucMask[iBufferIndex];

                    // Figure out how much to change the existing masks by
                    float fUnpaintRatio = 1.0f;
                    if (pucMaskSum && (uiCurrentSampleSum - ucCurrentSample) != 0)
                    {
                        efd::UInt32 uiSampleSum = pucMaskSum[iBufferIndex];

                        fUnpaintRatio = 
                            float(uiSampleSum - ucSample) / 
                            float(uiCurrentSampleSum - ucCurrentSample);
                    }
                    fUnpaintRatio = efd::Clamp(fUnpaintRatio, 0.0f, 1.0f);

                    // Apply the new values
                    for (efd::UInt32 uiC = 0; uiC < pkCell->GetSurfaceCount(); ++uiC)
                    {
                        efd::UInt8 ucNewValue = ucSample;
                        if (uiC != uiMaskChannel)
                        {// The other masks
                            ucNewValue = efd::UInt8(efd::Clamp(
                                float(pucPixels[iTextureIndex + uiC]) * fUnpaintRatio, 
                                0.0f, 255.0f));
                        }

                        // Check if this pixel is no longer used or newly used
                        bool bOldUsage = pucPixels[iTextureIndex + uiC] != 0;
                        bool bNewUsage = ucNewValue != 0;
                        if (bOldUsage != bNewUsage)
                        {
                            // Make sure that we don't get underflow on the mask usage
                            EE_ASSERT(kMaskUsage.m_uiSurfaceUsage[uiC] != 0 || bNewUsage);
                            kMaskUsage.m_uiSurfaceUsage[uiC] += (bNewUsage) ? 1 : -1;
                        }

                        // Apply the value
                        pucPixels[iTextureIndex + uiC] = ucNewValue;
                    }
                }
            }

            // Check if any of there are any unused surfaces on this cell
            for (efd::SInt32 iC = 0; iC < efd::SInt32(pkCell->GetSurfaceCount()); ++iC)
            {
                if (kMaskUsage.m_uiSurfaceUsage[iC] == 0)
                {
                    // Remove this surface from the leaf
                    pkCell->RemoveSurface(pkCell->GetSurfaceIndex(iC));

                    // go backwards on the uiC value because if the surface was removed then we 
                    // should stay checking the current surface index (as it will now contain
                    // a different surface's data (or won't be used anymore)
                    iC--;
                }
            }
            
            // Make sure the texture updates itself
            pkPixelData->MarkAsChanged();
            // Mark this cell as changed so the low detail renderer will pick it up
            pkCell->MarkSurfaceMasksChanged(true);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::AddSurface(efd::UInt32 uiSurfaceIndex, NiRect<efd::SInt32> kAffectedSectors, 
    NiRect<efd::SInt32> kMaskRegion)
{
    // Precondition: Sector is completely loaded
    if (!m_pkSectorData->IsLODLoaded(m_pkTerrain->GetNumLOD()))
        return true;

    // The given mask region identifies pixels in other sectors as well, extend it in those 
    // directions. 
    efd::SInt16 sSectX, sSectY;
    GetSectorIndex(sSectX, sSectY);
    efd::SInt32 iSectorPixels = m_pkTerrain->GetMaskSize() - (2 << m_pkTerrain->GetNumLOD());
    kMaskRegion.m_left += iSectorPixels * (kAffectedSectors.m_left - sSectX); 
    kMaskRegion.m_right += iSectorPixels * (kAffectedSectors.m_right - sSectX);
    kMaskRegion.m_top += iSectorPixels * (kAffectedSectors.m_top - sSectY);
    kMaskRegion.m_bottom += iSectorPixels * (kAffectedSectors.m_bottom - sSectY);

    // Figure out which cells this region will affect:
    efd::SInt32 iPixelsPerCell = (efd::SInt32(m_pkTerrain->GetMaskSize()) / 
        (1 << m_pkTerrain->GetNumLOD())) - 2;
    NiRect<efd::SInt32> kCellRegion;
    kCellRegion.m_left = efd::SInt32(NiFloor(float(kMaskRegion.m_left) / float(iPixelsPerCell)));
    kCellRegion.m_right = efd::SInt32(NiFloor(float(kMaskRegion.m_right) / float(iPixelsPerCell)));
    kCellRegion.m_top = efd::SInt32(NiFloor(float(kMaskRegion.m_top) / float(iPixelsPerCell)));
    kCellRegion.m_bottom = efd::SInt32(NiFloor(float(kMaskRegion.m_bottom) / float(iPixelsPerCell)));

    // Detect if the cell range needs expanding to include duplicated pixels
    if (kMaskRegion.m_left == kCellRegion.m_left * iPixelsPerCell)
        kCellRegion.m_left -= 1;
    if (kMaskRegion.m_bottom == kCellRegion.m_bottom * iPixelsPerCell)
        kCellRegion.m_bottom -= 1;
    if (kMaskRegion.m_right == (kCellRegion.m_right + 1) * iPixelsPerCell - 1)
        kCellRegion.m_right += 1;
    if (kMaskRegion.m_top == (kCellRegion.m_top + 1) * iPixelsPerCell - 1)
        kCellRegion.m_top += 1;

    // Clamp the cell range to the legal range for this sector
    NiUInt32 uiNumBlocksAcross = 1 << m_pkTerrain->GetNumLOD();
    kCellRegion.m_left = NiClamp(kCellRegion.m_left, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_right = NiClamp(kCellRegion.m_right, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_top = NiClamp(kCellRegion.m_top, 0, uiNumBlocksAcross - 1);
    kCellRegion.m_bottom = NiClamp(kCellRegion.m_bottom, 0, uiNumBlocksAcross - 1);

    // Update every relevant cell's blend mask data
    for (efd::SInt32 iCellX = kCellRegion.m_left; iCellX <= kCellRegion.m_right; ++iCellX)
    {
        for (efd::SInt32 iCellY = kCellRegion.m_bottom; iCellY <= kCellRegion.m_top; ++iCellY)
        {
            // Calculate the ID of the cell
            NiUInt32 uiCellID = iCellX + iCellY * uiNumBlocksAcross;
            uiCellID += GetCellOffset(m_pkTerrain->GetNumLOD());
            // Fetch this cell, its blend mask region and texture
            NiTerrainCellLeaf* pkCell = NiDynamicCast(NiTerrainCellLeaf, GetCell(uiCellID));
            EE_ASSERT(pkCell);

            // Find the channel of this surface
            efd::UInt32 uiMaskChannel = 0;
            if (!pkCell->GetSurfacePriority(uiSurfaceIndex, uiMaskChannel))
            {
                // This cell does not have this surface - attempt to add it
                uiMaskChannel = pkCell->GetSurfaceCount();
                pkCell->AddSurface(uiSurfaceIndex, uiMaskChannel);
            }
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------
void NiTerrainSector::GetHeightMap(HeightMapBuffer* pkBuffer, NiTerrainDataSnapshot* pkSnapshot)
{
    HeightMap* pkHeightMap = GetHeightMap();
    // If this sector hasn't completed loading then the heightmap will not be available
    if (!pkHeightMap)
        return;

    // Calculate the verts that are relevant to this sector
    efd::SInt32 iSectorWidth = pkHeightMap->GetWidth() - 1;
    NiRect<efd::SInt32> kHeightMapRegion = pkBuffer->GetDataRegion();

    // The given mask region identifies verts in other sectors as well, extend it in those 
    // directions. 
    efd::SInt16 sSectX, sSectY;
    GetSectorIndex(sSectX, sSectY);
    NiRect<efd::SInt32> kAffectedSectors = pkBuffer->GetAffectedSectorRange();
    kHeightMapRegion.m_left += iSectorWidth * (kAffectedSectors.m_left - sSectX); 
    kHeightMapRegion.m_right += iSectorWidth * (kAffectedSectors.m_right - sSectX);
    kHeightMapRegion.m_top += iSectorWidth * (kAffectedSectors.m_top - sSectY);
    kHeightMapRegion.m_bottom += iSectorWidth * (kAffectedSectors.m_bottom - sSectY);
    EE_ASSERT(kHeightMapRegion.m_right - kHeightMapRegion.m_left + 1 == 
        efd::SInt32(pkBuffer->GetWidth()));
    EE_ASSERT(kHeightMapRegion.m_top - kHeightMapRegion.m_bottom + 1 == 
        efd::SInt32(pkBuffer->GetHeight()));

    // Figure out the offset of this data in the buffer
    efd::SInt32 iBufferOffsetX = -kHeightMapRegion.m_left;
    efd::SInt32 iBufferOffsetY = -kHeightMapRegion.m_bottom;

    // Clamp the region to that relevant to this sector
    kHeightMapRegion.m_left = NiClamp(kHeightMapRegion.m_left, 0, iSectorWidth);
    kHeightMapRegion.m_right = NiClamp(kHeightMapRegion.m_right, 0, iSectorWidth);
    kHeightMapRegion.m_top = NiClamp(kHeightMapRegion.m_top, 0, iSectorWidth);
    kHeightMapRegion.m_bottom = NiClamp(kHeightMapRegion.m_bottom, 0, iSectorWidth);

    // Copy the data across
    efd::UInt16* pusData = pkBuffer->GetBuffer();
    efd::UInt16* pusMap = pkHeightMap->Lock(HeightMap::LockType::READ);
    for (efd::SInt32 iY = kHeightMapRegion.m_bottom; iY <= kHeightMapRegion.m_top; ++iY)
    {
        for (efd::SInt32 iX = kHeightMapRegion.m_left; iX <= kHeightMapRegion.m_right; ++iX)
        {
            efd::SInt32 iMapIndex = iX + iY * pkHeightMap->GetWidth();
            efd::SInt32 iBufferIndex = (iX + iBufferOffsetX) + 
                (iY + iBufferOffsetY) * pkBuffer->GetWidth();
            pusData[iBufferIndex] = pusMap[iMapIndex];
        }
    }
    pkHeightMap->Unlock(HeightMap::LockType::READ);

    // Loop over the cells involved in this area and take snapshots of the data there
    if (pkSnapshot)
    {
        NiRect<efd::SInt32> kCellRegion;
        efd::UInt32 uiCellWidth = iSectorWidth >> m_pkTerrain->GetNumLOD();
        kCellRegion.m_left = kHeightMapRegion.m_left / uiCellWidth;
        kCellRegion.m_right = kHeightMapRegion.m_right / uiCellWidth;
        kCellRegion.m_top = kHeightMapRegion.m_top / uiCellWidth;
        kCellRegion.m_bottom = kHeightMapRegion.m_bottom / uiCellWidth;

        // Clamp the cell range to the legal range for this sector
        NiUInt32 uiNumBlocksAcross = 1 << m_pkTerrain->GetNumLOD();
        kCellRegion.m_left = NiClamp(kCellRegion.m_left, 0, uiNumBlocksAcross - 1);
        kCellRegion.m_right = NiClamp(kCellRegion.m_right, 0, uiNumBlocksAcross - 1);
        kCellRegion.m_top = NiClamp(kCellRegion.m_top, 0, uiNumBlocksAcross - 1);
        kCellRegion.m_bottom = NiClamp(kCellRegion.m_bottom, 0, uiNumBlocksAcross - 1);

        for (efd::SInt32 iCellX = kCellRegion.m_left; iCellX <= kCellRegion.m_right; ++iCellX)
        {
            for (efd::SInt32 iCellY = kCellRegion.m_bottom; iCellY <= kCellRegion.m_top; ++iCellY)
            {
                // Calculate the ID of the cell
                NiUInt32 uiCellID = iCellX + iCellY * uiNumBlocksAcross;
                uiCellID += GetCellOffset(m_pkTerrain->GetNumLOD());
                // Fetch this cell, its blend mask region and texture
                NiTerrainCellLeaf* pkCell = NiDynamicCast(NiTerrainCellLeaf, GetCell(uiCellID));
                EE_ASSERT(pkCell);

                // Grab a snapshot of this cell's data
                if (pkSnapshot && !pkSnapshot->ContainsData(CellID(GetSectorID(), uiCellID), 
                    NiTerrainDataSnapshot::BufferType::HEIGHTMAP))
                {
                    // Add this cell's current data buffers in to the snapshot
                    pkCell->TakeSnapshot(pkSnapshot, NiTerrainDataSnapshot::BufferType::HEIGHTMAP);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiRect<efd::SInt32> NiTerrainSector::SetHeightMap(HeightMapBuffer* pkBuffer)
{
    HeightMap* pkHeightMap = GetHeightMap();
    EE_ASSERT(pkHeightMap);

    // Calculate the verts that are relevant to this sector
    efd::SInt32 iSectorWidth = pkHeightMap->GetWidth() - 1;
    NiRect<efd::SInt32> kHeightMapRegion = pkBuffer->GetDataRegion();
    
    // The given mask region identifies pixels in other sectors as well, extend it in those 
    // directions. 
    efd::SInt16 sSectX, sSectY;
    GetSectorIndex(sSectX, sSectY);
    NiRect<efd::SInt32> kAffectedSectors = pkBuffer->GetAffectedSectorRange();
    kHeightMapRegion.m_left += iSectorWidth * (kAffectedSectors.m_left - sSectX); 
    kHeightMapRegion.m_right += iSectorWidth * (kAffectedSectors.m_right - sSectX);
    kHeightMapRegion.m_top += iSectorWidth * (kAffectedSectors.m_top - sSectY);
    kHeightMapRegion.m_bottom += iSectorWidth * (kAffectedSectors.m_bottom - sSectY);
    EE_ASSERT(kHeightMapRegion.m_right - kHeightMapRegion.m_left + 1 == 
        efd::SInt32(pkBuffer->GetWidth()));
    EE_ASSERT(kHeightMapRegion.m_top - kHeightMapRegion.m_bottom + 1 == 
        efd::SInt32(pkBuffer->GetHeight()));

    // Figure out the offset of this data in the buffer
    efd::SInt32 iBufferOffsetX = -kHeightMapRegion.m_left;
    efd::SInt32 iBufferOffsetY = -kHeightMapRegion.m_bottom;

    // Clamp the region to that relevant to this sector
    kHeightMapRegion.m_left = NiClamp(kHeightMapRegion.m_left, 0, iSectorWidth);
    kHeightMapRegion.m_right = NiClamp(kHeightMapRegion.m_right, 0, iSectorWidth);
    kHeightMapRegion.m_top = NiClamp(kHeightMapRegion.m_top, 0, iSectorWidth);
    kHeightMapRegion.m_bottom = NiClamp(kHeightMapRegion.m_bottom, 0, iSectorWidth);

    // Copy the data across
    efd::UInt16* pusData = pkBuffer->GetBuffer();
    efd::UInt16* pusMap = pkHeightMap->Lock(HeightMap::LockType::WRITE);
    for (efd::SInt32 iY = kHeightMapRegion.m_bottom; iY <= kHeightMapRegion.m_top; ++iY)
    {
        for (efd::SInt32 iX = kHeightMapRegion.m_left; iX <= kHeightMapRegion.m_right; ++iX)
        {
            efd::SInt32 iMapIndex = iX + iY * pkHeightMap->GetWidth();
            efd::SInt32 iBufferIndex = (iX + iBufferOffsetX) + 
                (iY + iBufferOffsetY) * pkBuffer->GetWidth();
            pusMap[iMapIndex] = pusData[iBufferIndex];
        }
    }
    
    pkHeightMap->Unlock(HeightMap::LockType::WRITE);
    return kHeightMapRegion;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
NiTerrainSector::CellID::CellID(SectorID kSectorId, efd::UInt32 uiCellId):
    m_kSectorId(kSectorId),
    m_uiCellId(uiCellId)
{
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CellID::operator<(const CellID& other) const
{
    if (m_kSectorId != other.m_kSectorId)
        return m_kSectorId < other.m_kSectorId;
    else
        return m_uiCellId < other.m_uiCellId;
}

//--------------------------------------------------------------------------------------------------