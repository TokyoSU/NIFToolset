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
#include "NiTerrain.h"
#include "NiTerrainSector.h"
#include "NiTerrainSectorFile.h"
#include "NiTerrainSectorFileDeveloper.h"
#include "NiTerrainStreamingTask.h"

#include <NiImageConverter.h>
#include <NiFilename.h>
#include <efd/SmartCriticalSection.h>

NiImplementRootRTTI(NiTerrainStreamingTask);

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::Cancel()
{
    bool bSuccess = false;
    m_kMutex.Lock();

    if (!m_bBegun)
    {
        // Flag this task as cancelled!
        SetStatus(NiTerrainStreamingTask::Status::CANCELLED);
        CompleteTask(NiTerrain::EC_SECTOR_STREAMING_CANCELLED);
        bSuccess = true;

        if (m_kTaskType != TaskType::UNINITIALIZED)
        {
            // Recover the levels of detail that we locked for this task's execution 
            // (relevant in unload/rebuild tasks)
            EE_ASSERT(m_iOriginalLOD >= -1);
            m_pkSector->GetSectorData()->SetHighestLoadedLOD(m_iOriginalLOD);
        }
    }

#ifdef NITERRAIN_THREADING_DEBUG
    NiString kString;
    ToString((m_bBegun ? "Attempt Cancel":"Cancelled"), kString);
    NiOutputDebugString(kString);
#endif

    // If the task was cancelled, perform a cleanup of our objects
    if (bSuccess)
        Cleanup();

    m_kMutex.Unlock();
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::Initialize()
{
    m_kMutex.Lock();
    // Determine the task type
    m_iOriginalLOD = m_pkSector->GetSectorData()->GetHighestLoadedLOD();
    if (m_iTargetLOD == SpecialLevelOfDetail::UNCHANGED)
    {
        m_iTargetLOD = m_iOriginalLOD;
        m_kTaskType = TaskType::REBUILDING;
    }
    else if (m_iTargetLOD < m_iOriginalLOD ||
        m_iTargetLOD == SpecialLevelOfDetail::UNLOAD)
    {
        m_kTaskType = TaskType::UNLOADING;
        m_pkSector->GetSectorData()->SetHighestLoadedLOD(m_iTargetLOD);
    }
    else
    {
        EE_ASSERT(m_iTargetLOD > m_iOriginalLOD);
        m_kTaskType = TaskType::LOADING;
    }

    // Extract the configuration and figure out what needs building
    NiTerrainConfiguration kConfig = m_pkTerrain->GetConfiguration();
    bool bHighLOD = (NiInt32)m_pkTerrain->GetNumLOD() == m_iTargetLOD;

    // Determine what needs to be output to the cells in this task
    m_bOutputNormals = kConfig.GetNumNormalComponents(bHighLOD) != 0;
    m_bOutputTangents = kConfig.GetNumTangentComponents(bHighLOD) != 0;
    m_bOutputMorphing = kConfig.IsMorphingDataEnabled();
    m_bOutputLowDetailNormalMap = kConfig.IsLowDetailNormalMappingEnabled() && 
        (m_iOriginalLOD == -1 || m_kTaskType == TaskType::REBUILDING);
    m_bOutputLowDetailDiffuseMap = m_iOriginalLOD == -1;
    m_bOutputBlendMasks = bHighLOD && !m_bRebuildFromHeightmap;

    // Calculate the number of blend masks that will be required:
    efd::UInt32 uiMaximumBlendMaskSize = 2048;
    if (NiTerrain::InToolMode())
        uiMaximumBlendMaskSize = m_pkTerrain->GetMaskSize() / (1 << m_pkTerrain->GetNumLOD());
    efd::UInt32 uiBlendMaskSize = 
        (std::min)(m_pkTerrain->GetMaskSize(), uiMaximumBlendMaskSize);
    EE_ASSERT(uiBlendMaskSize <= 2048);
    m_uiNumBlendMasksPerSide = m_pkTerrain->GetMaskSize() / uiBlendMaskSize;

    // Calculate the various build regions.
    // Stream width takes into account the 1 pixel duplication along each edge of the heightmap
    NiUInt32 uiStreamWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    CalculateBuildHeightsRegion(uiStreamWidth + 2);
    CalculateBuildLightingRegion(uiStreamWidth);

    SetWaiting(true);
    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::CalculateNumWorkUnits()
{
    // Make sure that we finished our previous job
    EE_ASSERT(m_uiCompletedWorkUnits == m_uiNumWorkUnits);

    // Define the minimum number of operations that should be involved in a work unit. 
    // The following definition is derived from specifying that for a 1025 * 1025 terrain, 
    // we should let a maximum of four threads process it at once.
    const efd::UInt32 uiMinimumNumOperationsPerUnit = (1025 * 1025 / 4);

    // If we are a building job, we have as many work units as there are threads. The maximum number
    // of work units is the number of cells along a sector side at the target LOD. All other jobs 
    // are non parallelizable so can only have 1 work unit that encompasses all work.
    Status::VALUE eStatus = GetStatus();
    if (eStatus == Status::BUILDING_HEIGHTS)
    {
        efd::UInt32 uiNumRows = m_kBuildHeightsRegion.GetHeight() + 1;
        efd::UInt32 uiOperationsPerRow = m_kBuildHeightsRegion.GetWidth() + 1;
        efd::UInt32 uiMinimumRowsPerUnit = uiMinimumNumOperationsPerUnit / uiOperationsPerRow;

        m_uiNumWorkUnits = uiNumRows / uiMinimumRowsPerUnit;
        if (m_uiNumWorkUnits == 0)
            m_uiNumWorkUnits = 1;
    }
    else if (eStatus == Status::BUILDING_LIGHTING)
    {
        efd::UInt32 uiNumRows = m_kBuildLightingRegion.GetHeight() + 1;
        efd::UInt32 uiOperationsPerRow = m_kBuildLightingRegion.GetWidth() + 1;
        efd::UInt32 uiMinimumRowsPerUnit = uiMinimumNumOperationsPerUnit / uiOperationsPerRow;

        m_uiNumWorkUnits = uiNumRows / uiMinimumRowsPerUnit;
        if (m_uiNumWorkUnits == 0)
            m_uiNumWorkUnits = 1;
    }
    else
    {
        m_uiNumWorkUnits = 1;
    }
    
    if (m_uiNumWorkUnits > m_uiMaxWorkUnits)
        m_uiNumWorkUnits = m_uiMaxWorkUnits;

    EE_ASSERT(m_uiNumWorkUnits > 0);
    // Since we are calculating the number of work units, reset the number of completed.
    m_uiCompletedWorkUnits = 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::Preload()
{
    if (BeginThreading())
        return;

    // Check what data is already loaded
    {
        // Fetch the original low detail normal map
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, m_pkSector->GetTexture(
            NiTerrainCell::TextureType::LOWDETAIL_NORMAL));
        if (pkTexture)
        {
            m_spNormalMap = pkTexture->GetSourcePixelData();

            // If this assert is hit, then the texture has somehow lost the pixel data of the 
            // normal map. Was it created correctly?
            EE_ASSERT(m_spNormalMap);
        }
    }

    // Preload data from this sector and nearby sectors (anything on disk)
    PreloadSectorFiles();

    // Check that this sector matches the terrain's configuration
    NiTerrainSectorFile* pkMainFile = m_aspSectorFiles[1][1];
    if (pkMainFile)
    {
        if (pkMainFile->GetInterfaceVersion() != pkMainFile->GetFileVersion())
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_OUTOFDATE;
        }
        
        if (pkMainFile->IsDataReady(NiTerrainSectorFile::DataField::CONFIG))
        {
            efd::UInt32 uiSectorWidthInVerts;
            efd::UInt32 uiNumLOD;
            if (pkMainFile->ReadSectorConfig(uiSectorWidthInVerts, uiNumLOD))
            {
                if (uiSectorWidthInVerts != m_pkTerrain->GetCalcSectorSize() ||
                    uiNumLOD != m_pkTerrain->GetNumLOD())
                    CompleteTask(NiTerrain::EC_SECTOR_INVALID_SECTOR_SIZE);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::LoadBuffers()
{
    NiTerrainResourceManager* pkResources = m_pkTerrain->GetResourceManager();
        
    // Prepare the buffers that will be used in this task
    PrepareBuffers();

    // Load all the heightmaps for the relevant sectors
    LoadSectorHeights();

    // The following data streams are prebuilt based on a height map. If we are rebuilding then
    // don't load the prebaked versions.
    NiTerrainSectorFile* pkMainFile = m_aspSectorFiles[1][1];
    bool bLoadedNormals = false;
    bool bLoadedTangents = false;
    bool bLoadedLowDetailNormalMap = false;
    if (pkMainFile && !m_bRebuildFromHeightmap)
    {
        NiInt32 iSectorWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
        NiUInt32 uiNumDataPoints = iSectorWidth * iSectorWidth;
        NiTerrainSectorFile* pkFile = m_aspSectorFiles[1][1];

        // --- GEOMETRY ---
        // Load Normals from file
        if (m_bOutputNormals)
            bLoadedNormals = pkFile->ReadNormals(m_pkNormals, uiNumDataPoints);
        // Load Tangents from file
        if (m_bOutputTangents)
            bLoadedTangents = pkFile->ReadTangents(m_pkTangents, uiNumDataPoints);
        
        // --- CELL DATA ---
        if (m_pkBounds)
        {
            efd::UInt32 uiEndCell = m_pkSector->GetCellOffset(m_iTargetLOD + 1);
            efd::UInt32 uiBeginCell = m_pkSector->GetCellOffset(m_iOriginalLOD + 1);
            if (!pkFile->ReadCellBoundData(uiBeginCell, uiEndCell - uiBeginCell, m_pkBounds))
            {
                pkResources->ReleaseBuffer(m_pkBounds);
                m_pkBounds = NULL;
            }
        }

        // --- LEAF SURFACE INDEX DATA ---
        if (m_pkSurfaceIndexes)
        {
            // Read the sector's surface references
            if (!pkFile->ReadSurfaceReferences(m_kSurfaceReferences))
            {
                pkResources->ReleaseBuffer(m_pkSurfaceIndexes);
                m_pkSurfaceIndexes = NULL;
            }
        }

        if (m_pkSurfaceIndexes)
        {
            // Read all the data for each leaf
            efd::UInt32 uiNumLeaves = 1 << m_pkTerrain->GetNumLOD();
            uiNumLeaves *= uiNumLeaves;
            if (!pkFile->ReadCellSurfaceIndex(0, uiNumLeaves, m_pkSurfaceIndexes))
            {
                pkResources->ReleaseBuffer(m_pkSurfaceIndexes);
                m_pkSurfaceIndexes = NULL;
            }
        }

        // --- TEXTURES ---
        // Load the low detail normal texture
        if (!m_spNormalMap && m_bOutputLowDetailNormalMap)
        {
            NiPixelData* pkPixelData = NULL;
            if (pkFile->ReadLowDetailNormalMap(pkPixelData))
            {
                bLoadedLowDetailNormalMap = true;
                m_spNormalMap = pkPixelData;
            }
        }
        // Load the low detail diffuse texture
        if (!m_spDiffuseMap && m_bOutputLowDetailDiffuseMap)
        {
            NiPixelData* pkPixelData = NULL;
            if (pkFile->ReadLowDetailDiffuseMap(pkPixelData))
                m_spDiffuseMap = pkPixelData;
        }
        // Load the blend masks if they exist
        if (!m_spBlendMask && m_bOutputBlendMasks)
        {
            NiPixelData* pkPixelData = NULL;
            if (pkFile->ReadBlendMask(pkPixelData))
                m_spBlendMask = pkPixelData;
        }
    }

    // Did we end up finding a height map?
    if (!m_aspHeightMaps[1][1])
    {
        CompleteTask(NiTerrain::EC_SECTOR_MISSING_DATA);
        return;
    }

    // Build the blend masks now
    BuildBlendMasks();

    // Calculate what needs building (in the next step
    m_bBuildLowDetailNormalMap = m_bOutputLowDetailNormalMap && !bLoadedLowDetailNormalMap;
    m_bBuildNormals = m_bOutputNormals && !bLoadedNormals || m_bBuildLowDetailNormalMap;
    m_bBuildTangents = m_bOutputTangents && !bLoadedTangents;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::AcquireLocks()
{
    if (m_kTaskType == TaskType::UNLOADING)
    {
        AcquireLocks_Unloading();
    }
    else
    {
        AcquireLocks_Loading();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::AcquireLocks_Loading()
{
    // This function is here to process lock requests on a sector that must be processed in the
    // render thread. At this stage, there is no ability to perform these operations directly
    // through gamebryo, but in the future when that ability is available we should use it here

    // Register any sector surface references with the terrain
    // Note: because this deals with the terrain, this must happen via the update thread.
    NiTerrainSectorFile::SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaceReferences.begin(); kIter != m_kSurfaceReferences.end(); ++kIter)
    {
        NiTerrainSectorFile::SurfaceReference& kReference = kIter->second;
        efd::SInt32 iSurfaceIndex = m_pkTerrain->AddSurface(kReference.m_spPackageReference, 
            kReference.m_uiPackageIteration, 
            kReference.m_kSurfaceName);
        EE_ASSERT(iSurfaceIndex != -1);
        kReference.m_iSurfaceIndex = iSurfaceIndex;
    }

    // Lock any custom data the terrain may have
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->BeginLoadCustomData(m_pkSector, m_iTargetLOD, 
            m_aspSectorFiles[1][1]))
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_LOAD_CUSTOM_DATA_FAILED;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::AcquireLocks_Unloading()
{
    // Unlock any custom data the terrain may have
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->BeginUnloadCustomData(m_pkSector, m_iTargetLOD))
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_LOAD_CUSTOM_DATA_FAILED;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::PopulateStreams()
{
    if (!PrepareSector())
    {
        // Error code was already reported by the function, just return.
        return;
    }

    // Upload the streams
    PopulateCells();

    // Apply the textures
    ApplyTextures();

    // Only can be done if the rest was done
    FinilizeSectorLoading();

    // Load any custom data the terrain may have
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->LoadCustomData(m_pkSector, m_iTargetLOD, m_aspSectorFiles[1][1]))
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_LOAD_CUSTOM_DATA_FAILED;
        }
    }
    
    return;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::ReleaseLocks()
{
    if (m_kTaskType == TaskType::UNLOADING)
    {
        ReleaseLocks_Unloading();
    }
    else
    {
        ReleaseLocks_Loading();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::ReleaseLocks_Loading()
{
    // Release all the locks that we aquired in the execution of this task and 
    // complete the upload of any data that we need to do.

    // Upload the low detail normal map
    if (m_bOutputLowDetailNormalMap && m_spNormalMap)
    {
        // Upload the low detail normal map texture
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, m_pkSector->GetTexture(
            NiTerrainCell::TextureType::LOWDETAIL_NORMAL));
        EE_ASSERT(pkTexture);

        // Attempt to upload only the relevant portion of the texture
        NiTerrainUtils::UpdateTextureRegion(pkTexture, m_kNormalMapRegion);
    }

    // Unlock any custom data the terrain may have
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->EndLoadCustomData(m_pkSector, m_iTargetLOD,
            m_aspSectorFiles[1][1]))
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_LOAD_CUSTOM_DATA_FAILED;
        }
    }

    // Signal the task has completed
    CompleteTask(NiTerrain::EC_SECTOR_LOADED);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::ReleaseLocks_Unloading()
{
    // Unlock any custom data the terrain may have
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->EndUnloadCustomData(m_pkSector, m_iTargetLOD))
        {
            m_uiErrorCode |= NiTerrain::EC_SECTOR_LOAD_CUSTOM_DATA_FAILED;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::UnloadStreams()
{
    if (BeginThreading())
        return;

    NiUInt32 uiNumLOD = m_pkTerrain->GetNumLOD();
    EE_ASSERT(m_iTargetLOD >= -1);

    // Unload the custom data
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
        pkCustomDataPolicy->UnloadCustomData(m_pkSector, m_iTargetLOD);

    // Delete the cell objects
    NiUInt32 uiRemoveChildrenFrom = 0;
    if (m_iTargetLOD >= 0)
        uiRemoveChildrenFrom = m_iTargetLOD;

    // Is there actually anything to unload?
    if (m_pkSector->m_pkQuadCell == NULL)
    {
        if (m_iTargetLOD != SpecialLevelOfDetail::UNLOAD)
            CompleteTask(NiTerrain::EC_SECTOR_UNLOADED);

        return;
    }

    NiInt32 iLowestUnloadIndex = m_pkSector->GetCellOffset((NiUInt32)(m_iTargetLOD + 1));
    NiInt32 iLowestRemoveChildrenIndex = m_pkSector->GetCellOffset(uiRemoveChildrenFrom);

    EE_ASSERT(iLowestUnloadIndex >= iLowestRemoveChildrenIndex);

    NiInt32 iHighestIndex = m_pkSector->m_kCellArray.GetSize() - 1;

    for (NiInt32 iIndex = iHighestIndex; iIndex >= iLowestRemoveChildrenIndex; --iIndex)
    {
        NiTerrainCell* pkCell = m_pkSector->m_kCellArray.GetAt(iIndex);

        if (iIndex >= iLowestUnloadIndex)
        {
            m_pkSector->m_kCellArray.SetAt(iIndex, NULL);
            m_pkSector->m_kCellRegionArray.SetAt(iIndex, NULL);
        }

        // Only attempt to remove children if we are a NiTerrainCellNode
        NiTerrainCellNode* pkCellNode = NiDynamicCast(NiTerrainCellNode, pkCell);
        if (!pkCellNode)
            continue;

        // Remove the NiTerrainCellNode defined child references
        pkCellNode->SetChildAt(0, NULL);
        pkCellNode->SetChildAt(1, NULL);
        pkCellNode->SetChildAt(2, NULL);
        pkCellNode->SetChildAt(3, NULL);

        // Remove the NiNode defined children pointers
        pkCellNode->RemoveAllChildren();
    }

    // Unload the streams
    NiUInt32 uiUnloadFrom = (NiUInt32)(m_iTargetLOD + 1);
    for (NiUInt32 uiLevel = uiUnloadFrom; uiLevel <= uiNumLOD; ++uiLevel)
    {
        NiDataStream* pkPositionStream = 
            m_pkSector->GetSectorData()->GetStaticPositionStream(uiLevel);
        NiDataStream* pkLightingStream = 
            m_pkSector->GetSectorData()->GetStaticNormalTangentStream(uiLevel);

        m_pkSector->GetResourceManager()->ReleaseStream(
            NiTerrain::StreamType::POSITION, pkPositionStream);
        m_pkSector->GetResourceManager()->ReleaseStream(
            NiTerrain::StreamType::NORMAL_TANGENT, pkLightingStream);

        m_pkSector->GetSectorData()->SetStaticVertexStream(uiLevel, NULL);
        m_pkSector->GetSectorData()->SetStaticNormalTangentStream(uiLevel, NULL);
    }

    // Special case for head of quadtree
    if (m_iTargetLOD == SpecialLevelOfDetail::UNLOAD)
    {
        if (m_pkSector->m_pkQuadCell)
        {
            EE_ASSERT(m_pkSector->m_pkQuadCell->GetChildCount() == 0);
        }

        m_pkSector->DetachChild(m_pkSector->m_pkQuadCell);
        m_pkSector->m_pkQuadCell = NULL;
    }
    else
    {
        // We don't need to remove the sector from the terrain, so we are finished unloading.
        CompleteTask(NiTerrain::EC_SECTOR_UNLOADED);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::RemoveFromTerrain()
{
    // Remove the sector
    EE_ASSERT(m_pkSector->GetSectorData()->GetHighestLoadedLOD() == SpecialLevelOfDetail::UNLOAD);
    m_pkTerrain->RemoveSector(m_pkSector);

    // Signal the task has completed
    CompleteTask(NiTerrain::EC_SECTOR_UNLOADED);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::PreloadSectorFiles()
{
    typedef NiTerrainSectorFile::DataField DataField;

    // Figure out which sector is being loaded
    NiInt16 sSectorX;
    NiInt16 sSectorY;
    m_pkSector->GetSectorIndex(sSectorX, sSectorY);

    // Load the data for the main sector first (determine if we need to load the other sectors)
    NiTerrainSector::HeightMap* pkMainHeightMap = NULL;
    NiTerrainSectorFile* pkMainFile = NULL;
    // Are we trying to create a new blank sector?
    if (m_bCreateBlankHeightData)
        pkMainHeightMap = CreateBlankSectorHeightmap();
    else
        pkMainHeightMap = FetchSectorHeightmap(sSectorX, sSectorY);

    // Figure out what information will require building
    if (m_bRebuildFromHeightmap || m_bCreateBlankHeightData)
    {
        m_bBuildNormals = m_bOutputNormals || m_bOutputLowDetailNormalMap;
        m_bBuildTangents = m_bOutputTangents;
    }
    else
    {
        // Figure out what we want the main sector to load
        bool bHighestLOD = m_iTargetLOD == (NiInt32)m_pkTerrain->GetNumLOD();
        NiUInt32 eMainDataRequired = DataField::BOUNDS | DataField::CONFIG;
        eMainDataRequired |= (!pkMainHeightMap) ? DataField::HEIGHTS : 0;
        eMainDataRequired |= m_bOutputNormals ? DataField::NORMALS : 0;
        eMainDataRequired |= m_bOutputTangents ? DataField::TANGENTS : 0;
        eMainDataRequired |= m_bOutputLowDetailNormalMap ? DataField::LOWDETAIL_NORMALS : 0;
        eMainDataRequired |= bHighestLOD ? DataField::BLEND_MASK : 0;
        eMainDataRequired |= bHighestLOD ? DataField::SURFACE_INDEXES : 0;
        eMainDataRequired |= m_bOutputLowDetailDiffuseMap ? DataField::LOWDETAIL_DIFFUSE : 0;
        
        NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
        if (pkCustomDataPolicy)
        {
            eMainDataRequired |= pkCustomDataPolicy->GetLoadPrecacheFields();
        }

        // Load the main sector file
        pkMainFile = m_pkTerrain->OpenSectorFile(sSectorX, sSectorY, false);
        if (pkMainFile)
        {
            pkMainFile->Precache(efd::Int32ToUInt32(m_iOriginalLOD + 1), 
                efd::Int32ToUInt32(m_iTargetLOD), eMainDataRequired);

            m_bBuildNormals = 
                m_bOutputNormals && !pkMainFile->IsDataReady(DataField::NORMALS) || 
                m_bOutputLowDetailNormalMap && 
                !pkMainFile->IsDataReady(DataField::LOWDETAIL_NORMALS);
            m_bBuildTangents = 
                m_bOutputTangents && !pkMainFile->IsDataReady(DataField::TANGENTS);
        }
        else
        {
            CompleteTask(NiTerrain::EC_SECTOR_INVALID_FILE);
            return false;
        }
    }

    // Figure out if we need to load our neighbors or not:
    // Basically we need neighbors if we are going to need to calculate any normal information.
    NiUInt32 uiSectorWidthInVerts = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    bool bLoadNeighbors = m_bOutputNormals && m_pkNormals == NULL || 
        m_bOutputLowDetailNormalMap && m_spNormalMap == NULL;

    // Calculate the border pixels to test against
    NiInt32 iMinBorder = ms_iBuildNormalKernel - 1;
    NiInt32 iMaxBorder = uiSectorWidthInVerts - ms_iBuildNormalKernel;
    // Load the required height maps
    for (NiInt32 iX = -1; iX < 2; ++iX)
    {
        for (NiInt32 iY = -1; iY < 2; ++iY)
        {
            bool bMainSector = (iX == 0 && iY == 0);
            bool bLoadHeightMap = 
                iX == -1 && m_kBuildRegion.m_left <= iMinBorder ||
                iX == 1 && m_kBuildRegion.m_right >= iMaxBorder ||
                iY == 1 && m_kBuildRegion.m_top >= iMaxBorder ||
                iY == -1 && m_kBuildRegion.m_bottom <= iMinBorder;

            bLoadHeightMap = bLoadHeightMap && bLoadNeighbors && !bMainSector;
            if (bLoadHeightMap)
            {
                // Attempt to fetch the sector height map from preloaded sectors
                NiTerrainSector::HeightMap* pkHeightmap = 
                    FetchSectorHeightmap(sSectorX + iX, sSectorY + iY);
                m_aspHeightMaps[iX + 1][iY + 1] = pkHeightmap;
                
                // If we weren't able to get the height map we'll need to load it
                if (!pkHeightmap)
                {
                    NiTerrainSectorFile* pkFile = 
                        m_pkTerrain->OpenSectorFile(sSectorX + iX, sSectorY + iY, false);
                    if (pkFile)
                    {
                        pkFile->Precache(efd::Int32ToUInt32(m_iOriginalLOD), 
                            efd::Int32ToUInt32(m_iTargetLOD), 
                            DataField::HEIGHTS | DataField::CONFIG);
                    }
                    m_aspSectorFiles[iX + 1][iY + 1] = pkFile;
                }
            }
        }
    }

    // Assign the main sector's data
    m_aspHeightMaps[1][1] = pkMainHeightMap;
    m_aspSectorFiles[1][1] = pkMainFile;

    // Make sure that all the sector files loaded match the parameters of the terrain
    bool bResult = true;
    for (NiInt32 iX = -1; iX < 2; ++iX)
    {
        for (NiInt32 iY = -1; iY < 2; ++iY)
        {
            NiTerrainSectorFile* pkFile = m_aspSectorFiles[iX + 1][iY + 1];
            bool bValidFile = true;
            if (pkFile)
            {
                efd::UInt32 uiNumLOD;
                if (!pkFile->ReadSectorConfig(uiSectorWidthInVerts, uiNumLOD))
                    bValidFile = false;

                if (uiSectorWidthInVerts != m_pkTerrain->GetCalcSectorSize() ||
                    uiNumLOD != m_pkTerrain->GetNumLOD())
                    bValidFile = false;
            }

            if (!bValidFile)
            {
                pkFile->Close();
                m_aspSectorFiles[iX + 1][iY + 1] = NULL;

                if (iX == 0 && iY == 0)
                {
                    CompleteTask(NiTerrain::EC_SECTOR_INVALID_FILE);
                    bResult = false;
                }
            }
        }
    }

    return bResult;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::PrepareBuffers()
{
    NiTerrainResourceManager* pkResources = m_pkTerrain->GetResourceManager();

    // Allocate buffers for each of these streams
    // Calculate how many data points there are
    NiInt32 iSectorWidthInVerts = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    NiUInt32 uiStreamWidth = iSectorWidthInVerts;
    NiUInt32 uiNumHeightDataPoints = (uiStreamWidth + 2) * (uiStreamWidth + 2);
    NiUInt32 uiNumNTDataPoints = uiStreamWidth * uiStreamWidth;

    // Setup the temporary stream to collect height information in
    NIASSERT(m_pfHeights == NULL);
    m_pfHeights = pkResources->CreateBuffer<efd::Float32>(uiNumHeightDataPoints);
    m_kHeightIterator = NiTStridedRandomAccessIterator<efd::Float32>(m_pfHeights, 
        sizeof(efd::Float32));

    // Initialize the required streams
    // Initialize normal stream
    NIASSERT(m_pkNormals == NULL);
    m_pkNormals = pkResources->CreateBuffer<efd::Point2>(uiNumNTDataPoints);
    m_kNormalIterator = NiTStridedRandomAccessIterator<efd::Point2>(m_pkNormals, 
        sizeof(efd::Point2));

    // Initialize tangent stream
    NIASSERT(m_pkTangents == NULL);
    m_pkTangents = pkResources->CreateBuffer<efd::Point2>(uiNumNTDataPoints);
    m_kTangentIterator = NiTStridedRandomAccessIterator<efd::Point2>(m_pkTangents, 
        sizeof(efd::Point2));

    // Cell data buffers (if we are loading from file)
    NiTerrainSectorFile* pkMainFile = m_aspSectorFiles[1][1];
    if (pkMainFile)
    {
        efd::UInt32 uiNumLeaves = 1 << m_pkTerrain->GetNumLOD();
        uiNumLeaves *= uiNumLeaves;
        efd::UInt32 uiNumCells = m_pkSector->GetCellOffset(m_iTargetLOD + 1) - 
            m_pkSector->GetCellOffset(m_iOriginalLOD + 1);
        
        if (pkMainFile->IsDataReady(NiTerrainSectorFile::DataField::BOUNDS))
        {
            m_pkBounds = pkResources->CreateBuffer<NiTerrainSectorFile::CellData>(uiNumCells);
        }
        if (m_iTargetLOD == efd::SInt32(m_pkTerrain->GetNumLOD()) &&
            pkMainFile->IsDataReady(NiTerrainSectorFile::DataField::SURFACE_INDEXES))
        {
            m_pkSurfaceIndexes = 
                pkResources->CreateBuffer<NiTerrainSectorFile::LeafData>(uiNumLeaves);
        }
    }

    // Blend mask buffers
    if (m_bOutputBlendMasks)
    {
        efd::UInt32 uiTotalBlendMasks = m_uiNumBlendMasksPerSide * m_uiNumBlendMasksPerSide;
        efd::UInt32 uiDividedBlendMaskWidth = m_pkTerrain->GetMaskSize() / m_uiNumBlendMasksPerSide;
        m_pkDividedBlendMasks = EE_ALLOC(NiPixelData*, uiTotalBlendMasks);
        for (efd::UInt32 uiIndex = 0; uiIndex < uiTotalBlendMasks; ++uiIndex)
        {
            m_pkDividedBlendMasks[uiIndex] = EE_NEW NiPixelData(
                uiDividedBlendMaskWidth, uiDividedBlendMaskWidth, NiPixelFormat::RGBA32);
        }   
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::LoadSectorHeights()
{
    // Reusable variables
    efd::UInt32 uiSectorWidth = m_pkTerrain->GetCalcSectorSize();
    efd::UInt32 uiStreamLength = uiSectorWidth * uiSectorWidth;

    // Load the required height maps from file
    for (NiInt32 iX = -1; iX < 2; ++iX)
    {
        for (NiInt32 iY = -1; iY < 2; ++iY)
        {
            // If there is a file loaded, and there is no height map, then we are expected
            // to load from it
            if (m_aspSectorFiles[iX + 1][iY + 1] && !m_aspHeightMaps[iX + 1][iY + 1])
            {
                // Read the heights from the file
                NiTerrainSectorFile* pkFile = m_aspSectorFiles[iX + 1][iY + 1];
                efd::UInt16* pusHeights = EE_ALLOC(efd::UInt16, uiStreamLength);
                if (pkFile->ReadHeights(pusHeights, uiStreamLength))
                {
                    // Create a height map
                    m_aspHeightMaps[iX + 1][iY + 1] = 
                        EE_NEW NiTerrainSector::HeightMap(uiSectorWidth, uiSectorWidth, pusHeights);
                }
                else
                {
                    EE_FREE(pusHeights);
                }

                // Close and delete the file if it is an adjacent file
                if (iX != 0 || iY != 0)
                {
                    pkFile->Close();
                    m_aspSectorFiles[iX + 1][iY + 1] = NULL;
                }
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSector::HeightMap* NiTerrainStreamingTask::CreateBlankSectorHeightmap()
{
    // Create the pixel data
    efd::UInt32 uiSectorWidth = m_pkTerrain->GetCalcSectorSize();
    NiUInt32 uiNumPixels = uiSectorWidth * uiSectorWidth;
    NiUInt16* pusPixelData = EE_ALLOC(efd::UInt16, uiNumPixels);

    // Work out the middle value:
    const efd::UInt16 usMidPoint = efd::UInt16(-1) / 2;
    
    // Set all heights to 0 (must be done in a loop as memset only takes chars
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumPixels; ++uiIndex)
        pusPixelData[uiIndex] = usMidPoint;

    // Create the heightmap
    return EE_NEW NiTerrainSector::HeightMap(uiSectorWidth, uiSectorWidth, pusPixelData); 
}

//--------------------------------------------------------------------------------------------------
NiTerrainSector::HeightMap* NiTerrainStreamingTask::FetchSectorHeightmap(NiInt32 iSectorX, 
    NiInt32 iSectorY)
{
    NiTerrainSector::HeightMap* pkHeightMap = NULL;

    // Attempt to fetch the data from the sector if it was already loaded
    NiTerrainSector* pkSector = m_pkTerrain->GetSector(
        efd::Int32ToInt16(iSectorX), efd::Int32ToInt16(iSectorY));
    if (pkSector)
    {
        pkHeightMap = pkSector->GetHeightMap();
    }

    // Attempt to fetch the data from the height map cache in the loading manager
    // No heightmap cache implemented yet...

    return pkHeightMap;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::BuildBlendMasks()
{
    // Check if we are building blend masks or not
    if (!m_bOutputBlendMasks)
        return;
    EE_ASSERT(m_pkDividedBlendMasks);

    // Perform some calculations
    efd::UInt32 uiBlendMaskWidth = m_pkTerrain->GetMaskSize() / m_uiNumBlendMasksPerSide;
        
    // For each blend mask, fetch the values that are relevant
    for (efd::UInt32 uiMaskY = 0; uiMaskY < m_uiNumBlendMasksPerSide; ++uiMaskY)
    {
        for (efd::UInt32 uiMaskX = 0; uiMaskX < m_uiNumBlendMasksPerSide; ++uiMaskX)
        {
            // Fetch the pixel data for this mask
            NiPixelData* pkPixelData = 
                m_pkDividedBlendMasks[uiMaskY * m_uiNumBlendMasksPerSide + uiMaskX];

            // Figure out the position of the blend mask on the sector wide blend mask
            NiIndex kBufferOffset;
            kBufferOffset.x = uiMaskX * uiBlendMaskWidth;
            kBufferOffset.y = (m_uiNumBlendMasksPerSide - uiMaskY - 1) * uiBlendMaskWidth;

            // Figure out the segment of the blend mask that the leaf is using
            NiIndex kTextureStart(0,0);

            // Figure out the length in bytes of each scanline
            efd::UInt32 uiStride = pkPixelData->GetPixelStride();
            efd::UInt32 uiScanlineLength = uiBlendMaskWidth * uiStride;

            // Copy the segment of this texture that the leaf is using into the overall buffer
            for (efd::UInt32 uiY = 0; uiY < uiBlendMaskWidth; ++uiY)
            {
                // Figure out the beginning of the texture scanline
                efd::UInt32 uiTextureBeginIndex = 
                    ((uiY + kTextureStart.y) * pkPixelData->GetWidth()) + kTextureStart.x;
                void* pvDest = &pkPixelData->GetPixels()[uiTextureBeginIndex * uiStride];

                if (m_spBlendMask)
                {
                    // Figure out the beginning of the buffer scanline
                    efd::UInt32 uiBufferBeginIndex = 
                        ((uiY + kBufferOffset.y) * m_spBlendMask->GetWidth()) + kBufferOffset.x;
                    void* pvSrc = &m_spBlendMask->GetPixels()[uiBufferBeginIndex * uiStride];
            
                    // Execute scanline copy
                    NiMemcpy(pvDest, pvSrc, uiScanlineLength);
                }
                else
                {
                    memset(pvDest, 0, uiScanlineLength);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::BuildHeights(NiUInt32 uiWorkUnit)
{
    if (!m_aspHeightMaps[1][1])
    {
        CompleteTask(true);
        return;
    }

    NIASSERT(m_pfHeights != NULL);

    //Fetch pointers to all the height map pixels (Lock the heightmaps):
    NiUInt16* pusHeights[3][3];
    for (NiUInt32 uiIndex = 0; uiIndex < 9; ++uiIndex)
    {
        NiUInt32 uiX = uiIndex / 3;
        NiUInt32 uiY = uiIndex % 3;
        NiTerrainSector::HeightMap* pkHeightmap = m_aspHeightMaps[uiX][uiY];
        if (pkHeightmap)
            pusHeights[uiX][uiY] = pkHeightmap->Lock(NiTerrainSector::HeightMap::LockType::READ);
        else
            pusHeights[uiX][uiY] = NULL;
    }

    NiRect<NiInt32> kBuildRegion;
    GetWorkUnitRegion(uiWorkUnit, m_kBuildHeightsRegion, kBuildRegion);

    // Load the maximum and minimum heights and convert all the relevant height points
    efd::Float32 fMinHeight = m_pkTerrain->GetMinHeight();
    efd::Float32 fMaxHeight = m_pkTerrain->GetMaxHeight();
    NiInt32 iSectorWidthInVerts = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    NiUInt32 uiStreamWidth = (NiUInt32)iSectorWidthInVerts + 2;
    for (NiInt32 iY = kBuildRegion.m_bottom; iY <= kBuildRegion.m_top; ++iY)
    {
        for (NiInt32 iX = kBuildRegion.m_left; iX <= kBuildRegion.m_right; ++iX)    
        {
            NiInt32 iSectorX = iX - 1;
            NiInt32 iSectorY = iY - 1;

            // Decide if bordering sector height maps need sampling
            NiUInt32 uiMapX = 1;
            NiUInt32 uiMapY = 1;
            {
                if (iSectorX < 0)
                {
                    if (pusHeights[uiMapX - 1][uiMapY])
                    {
                        iSectorX = iSectorWidthInVerts - 2;
                        uiMapX--;
                    }
                    else
                    {
                        iSectorX++;
                    }
                }
                if (iSectorX >= iSectorWidthInVerts)
                {
                    if (pusHeights[uiMapX + 1][uiMapY])
                    {
                        iSectorX = 1;
                        uiMapX++;
                    }
                    else
                    {
                        iSectorX--;
                    }
                }
                if (iSectorY < 0)
                {
                    if (pusHeights[uiMapX][uiMapY - 1])
                    {
                        iSectorY = iSectorWidthInVerts - 2;
                        uiMapY--;
                    }
                    else
                    {
                        iSectorY++;
                    }
                }
                if (iSectorY >= iSectorWidthInVerts)
                {
                    if (pusHeights[uiMapX][uiMapY + 1])
                    {
                        iSectorY = 1;
                        uiMapY++;
                    }
                    else
                    {
                        iSectorY--;
                    }
                }
            }
            EE_ASSERT(pusHeights[uiMapX][uiMapY]);

            // Flip the Y index of the pixel
            NiUInt32 uiPixelIndex = (iSectorWidthInVerts - iSectorY - 1) * iSectorWidthInVerts + 
                iSectorX;
            uiPixelIndex = iSectorY * iSectorWidthInVerts + iSectorX;
            EE_ASSERT(uiPixelIndex < uiStreamWidth * uiStreamWidth);

            // Sample the height from the map and convert it to a float using the designated range
            float fSample = float(pusHeights[uiMapX][uiMapY][uiPixelIndex]) / float(NiUInt16(-1));
            fSample = fSample * (fMaxHeight - fMinHeight) + fMinHeight;

            // Store the sample in the data streams
            NiUInt32 uiDataIndex = iY * uiStreamWidth + iX;
            m_kHeightIterator[uiDataIndex] = fSample;
        }
    }

    //Unlock all the height maps
    for (NiUInt32 uiIndex = 0; uiIndex < 9; ++uiIndex)
    {
        NiUInt32 uiX = uiIndex / 3;
        NiUInt32 uiY = uiIndex % 3;
        NiTerrainSector::HeightMap* pkHeightmap = m_aspHeightMaps[uiX][uiY];
        if (pkHeightmap)
            pkHeightmap->Unlock(NiTerrainSector::HeightMap::LockType::READ);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::BuildLighting(NiUInt32 uiWorkUnit)
{
    if (!m_pfHeights)
        return;

    NiTerrainConfiguration kConfig = m_pkTerrain->GetConfiguration();

    // Do we need to continue?
    if (!m_bBuildNormals && !m_bBuildTangents)
        return;
    
    // Calculate how many data points there are
    NiInt32 iSectorWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts();

    // Generate the data
    // Figure out what values require building
    NiRect<NiInt32> kBuildRegion;
    GetWorkUnitRegion(uiWorkUnit, m_kBuildLightingRegion, kBuildRegion);

    for (NiInt32 iY = kBuildRegion.m_bottom; iY <= kBuildRegion.m_top; ++iY)
    {
        for (NiInt32 iX = kBuildRegion.m_left; iX <= kBuildRegion.m_right; ++iX)    
        {
            efd::Point2 kDiff(2.0f, 2.0f);

            // Sample the 4 points around this position
            float fHeightLeft = SampleHeightMap(iX - 1, iY);
            float fHeightRight = SampleHeightMap(iX + 1, iY);
            float fHeightTop = SampleHeightMap(iX, iY - 1);
            float fHeightBottom = SampleHeightMap(iX, iY + 1);

            NiUInt32 uiStreamPos = iY * iSectorWidth + iX;
            EE_ASSERT((NiInt32)uiStreamPos < iSectorWidth * iSectorWidth);

            // Calculate the normal at this position
            efd::Point3 kNormal;
            if (m_bBuildNormals)
            {
                kNormal =
                    efd::Point3(kDiff.x, 0.0f, fHeightRight - fHeightLeft).Cross(
                    efd::Point3(0.0f ,kDiff.y, fHeightBottom - fHeightTop));
                efd::Point3::UnitizeVector(kNormal);

                // Insert into stream
                efd::Point2& kStream = m_kNormalIterator[uiStreamPos];
                kStream.x = kNormal.x / kNormal.z;
                kStream.y = kNormal.y / kNormal.z;
            }

            // Calculate the tangent at this position
            if (m_bBuildTangents)
            {
                efd::Point3 kTangent = 
                    efd::Point3(2.0f, 0.0f, fHeightRight - fHeightLeft);
                efd::Point3::UnitizeVector(kTangent);

                // Insert into stream
                efd::Point2& kStream = m_kTangentIterator[uiStreamPos];
                kStream.x = kTangent.x;
                kStream.y = kTangent.z;
            }
        }   
    }

    return;
}

//--------------------------------------------------------------------------------------------------
float NiTerrainStreamingTask::SampleHeightMap(NiInt32 iX, NiInt32 iY)
{
    iX += 1;
    iY += 1;

    // Clamp to the dimensions of the map
    NiUInt32 uiWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts() + 2;
    NiInt32 iSampleX = NiClamp(iX, 0, uiWidth - 1);
    NiInt32 iSampleY = NiClamp(iY, 0, uiWidth - 1);
    EE_ASSERT(iSampleX == iX && iSampleY == iY);

    // Sample the map
    return m_kHeightIterator[iSampleY * uiWidth + iSampleX];
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::BuildLowDetailNormalMap()
{
    NiTerrainResourceManager* pkResources = m_pkTerrain->GetResourceManager();

    // Don't do anything if it isn't required:
    if (!m_bBuildLowDetailNormalMap)
        return;

    // Create a image to hold the size of the terrain.
    NiUInt32 iImageWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    NiUInt32 uiNumPixels = iImageWidth * iImageWidth;
    NiUInt8* pucPixels = pkResources->CreateBuffer<NiUInt8>(uiNumPixels * 3);

    NiTerrainNormalRandomAccessIterator kNormalIter;
    efd::Point2 kCompNormal;
    efd::Point3 kNormal;

    for (NiInt32 iY = m_kBuildLightingRegion.m_bottom; iY <= m_kBuildLightingRegion.m_top; ++iY)
    {
        for (NiInt32 iX = m_kBuildLightingRegion.m_left; iX <= m_kBuildLightingRegion.m_right; ++iX)    
        {
            efd::SInt32 iIndex = iY * iImageWidth + iX;

            kCompNormal = m_kNormalIterator[iIndex];
            kNormal = efd::Point3(kCompNormal.x, kCompNormal.y, 1.0f);
            efd::Point3::UnitizeVector(kNormal);

            NiUInt32 uiPixelIndex = (iImageWidth - iY - 1) * iImageWidth + (iX);

            uiPixelIndex = uiPixelIndex * 3;
            pucPixels[uiPixelIndex + 0] = NiUInt8(
                ((kNormal.x + 1.0f) / 2.0f) * 255.0f);
            pucPixels[uiPixelIndex + 1] = NiUInt8(
                ((kNormal.y + 1.0f) / 2.0f) * 255.0f);
            pucPixels[uiPixelIndex + 2] = NiUInt8(
                ((kNormal.z) * 255.0f));
            EE_ASSERT(kNormal.z > 0);
        }
    }

    // Fetch the pixel data to update the normal data inside
    NiInt32 iTextureWidth = efd::Min<NiInt32>(iImageWidth - 1, 2048);
    EE_ASSERT(iImageWidth - 1 <= 2048);
    if (!m_spNormalMap)
        m_spNormalMap = NiNew NiPixelData(iTextureWidth, iTextureWidth, NiPixelFormat::RGB24);

    // Resize this image
    NiRect<float> kCropRegion;
    kCropRegion.m_top = 0.0f;
    kCropRegion.m_bottom = 1.0f;
    kCropRegion.m_left = 0.0f;
    kCropRegion.m_right = 1.0f;
    NiRect<efd::SInt32> kSampleRegion;

    // Expand the area of the build region to include the band of normals that have changed too
    kSampleRegion.m_left   = NiClamp(m_kBuildRegion.m_left - 1  , 0, iImageWidth - 1);
    kSampleRegion.m_right  = NiClamp(m_kBuildRegion.m_right + 1 , 0, iImageWidth - 1);
    kSampleRegion.m_top    = NiClamp(m_kBuildRegion.m_top + 1   , 0, iImageWidth - 1);
    kSampleRegion.m_bottom = NiClamp(m_kBuildRegion.m_bottom - 1, 0, iImageWidth - 1);

    // Convert this region to the min/max sample points in the texture
    float fSampleRate = float(iTextureWidth - 1) / float(iImageWidth - 1);
    kSampleRegion.m_left   = NiInt32(NiFloor(float(kSampleRegion.m_left)   * fSampleRate));
    kSampleRegion.m_right  = NiInt32(ceil(   float(kSampleRegion.m_right)  * fSampleRate));
    kSampleRegion.m_top    = NiInt32(ceil(   float(kSampleRegion.m_top)    * fSampleRate));
    kSampleRegion.m_bottom = NiInt32(NiFloor(float(kSampleRegion.m_bottom) * fSampleRate));
 
    // Perform some sanity clamping
    kSampleRegion.m_left   = NiClamp(kSampleRegion.m_left  , 0, iTextureWidth - 1);
    kSampleRegion.m_right  = NiClamp(kSampleRegion.m_right , 0, iTextureWidth - 1);
    kSampleRegion.m_top    = NiClamp(kSampleRegion.m_top   , 0, iTextureWidth - 1);
    kSampleRegion.m_bottom = NiClamp(kSampleRegion.m_bottom, 0, iTextureWidth - 1);

    // Flip the vertex sampling to pixel sampling (Flip Y)
    kSampleRegion.m_top = iTextureWidth - kSampleRegion.m_top - 1;
    kSampleRegion.m_bottom = iTextureWidth - kSampleRegion.m_bottom - 1;

    m_kNormalMapRegion = kSampleRegion;
    NiTerrainUtils::ResampleImage(kCropRegion, pucPixels, iImageWidth, 
        kSampleRegion, m_spNormalMap->GetPixels(), iTextureWidth, 3);
    pkResources->ReleaseBuffer(pucPixels);

    return;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::PrepareSector()
{
    if (m_bRebuildFromHeightmap)
    {
        // No preparations required.
        return true;
    }

    // Allocate the streams for the new cells
    if (!m_pkSector->CreateStreams(m_iTargetLOD))
    {
        CompleteTask(NiTerrain::EC_SECTOR_STREAM_CREATION_ERROR);
        return false;
    }

    // Build the NiTerrainDataLeaf tree
    m_pkSector->BuildQuadTree(m_iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::PopulateCells()
{
    // Variable declarations
    NiTerrainStreamLocks kLock;
    NiTerrainPositionRandomAccessIterator kPositions;
    NiTerrainNormalRandomAccessIterator kNormals;
    NiTerrainTangentRandomAccessIterator kTangents;
    NiUInt8 ucMask = NiDataStream::LOCK_READ | NiDataStream::LOCK_WRITE;

    // Prepare to populate the position stream
    NiUInt32 uiCellSize = m_pkSector->GetSectorData()->GetCellSize();
    NiUInt32 iNumLOD = m_pkTerrain->GetNumLOD();
    NiUInt32 uiSectorWidth = m_pkSector->GetSectorData()->GetSectorWidthInVerts();
    
    // Jump to the LOD level that we need to begin loading from:
    NiInt32 iBeginLevel;
    if (!m_bRebuildFromHeightmap)
        iBeginLevel = m_iOriginalLOD + 1;
    else
        iBeginLevel = 0;

    // Increase the range of the build region to make sure that the required normals are 
    // updated (1 normals around the area of the affected area).
    m_kBuildRegion.m_left = NiClamp(m_kBuildRegion.m_left - 1, 0, uiSectorWidth - 1);
    m_kBuildRegion.m_right = NiClamp(m_kBuildRegion.m_right + 1, 0, uiSectorWidth - 1);
    m_kBuildRegion.m_top = NiClamp(m_kBuildRegion.m_top + 1, 0, uiSectorWidth - 1);
    m_kBuildRegion.m_bottom = NiClamp(m_kBuildRegion.m_bottom - 1, 0, uiSectorWidth - 1);

    // For each level being updated
    for (NiInt32 iLevel = iBeginLevel; iLevel <= m_iTargetLOD; ++iLevel)
    {
        EE_ASSERT(iLevel >= 0);
        NiUInt32 uiLevel = NiUInt32(iLevel);
        NiInt32 iStride = 1 << (iNumLOD - uiLevel);

        // Calculate which cells are within the build region
        NiUInt32 uiMinX = m_kBuildRegion.m_left / iStride + 
            ((m_kBuildRegion.m_left % iStride)?1:0);
        NiUInt32 uiMaxX = m_kBuildRegion.m_right / iStride;
        NiUInt32 uiMinY = m_kBuildRegion.m_bottom / iStride + 
            ((m_kBuildRegion.m_bottom % iStride)?1:0);
        NiUInt32 uiMaxY = m_kBuildRegion.m_top / iStride;

        uiMinX = uiMinX / uiCellSize - ((uiMinX == 0 || uiMinX % uiCellSize)?0:1);
        uiMaxX = uiMaxX / uiCellSize;
        uiMinY = uiMinY / uiCellSize - ((uiMinY == 0 || uiMinY % uiCellSize)?0:1);
        uiMaxY = uiMaxY / uiCellSize;

        uiMaxX = efd::Min<NiUInt32>(uiMaxX, (1 << uiLevel) - 1);
        uiMaxY = efd::Min<NiUInt32>(uiMaxY, (1 << uiLevel) - 1);

        // Clamp the range
        EE_ASSERT(uiMinX == NiUInt32(NiClamp(uiMinX, 0, 1 << uiLevel)));
        EE_ASSERT(uiMaxX == NiUInt32(NiClamp(uiMaxX, 0, 1 << uiLevel)));
        EE_ASSERT(uiMinY == NiUInt32(NiClamp(uiMinY, 0, 1 << uiLevel)));
        EE_ASSERT(uiMaxY == NiUInt32(NiClamp(uiMaxY, 0, 1 << uiLevel)));

        // Calculate some values for this level
        NiUInt32 uiNumBlocksAcross = 1 << uiLevel;

        // Iterate over every cell within the build region
        for (NiUInt32 uiX = uiMinX; uiX <= uiMaxX; ++uiX)
        {
            for (NiUInt32 uiY = uiMinY; uiY <= uiMaxY; ++uiY)
            {
                // Calculate the ID of the cell
                NiUInt32 uiCellID = uiX + uiY * uiNumBlocksAcross;
                uiCellID += m_pkSector->GetCellOffset(uiLevel);

                // Fetch this cell and populate it's streams
                NiTerrainCell* pkCell = m_pkSector->GetCell(uiCellID);

                // Obtain locks for this cell
                kLock.GetPositionIterator(pkCell, ucMask, kPositions);
                kLock.GetNormalIterator(pkCell, ucMask, kNormals);
                kLock.GetTangentIterator(pkCell, ucMask, kTangents);

                // Copy the data into this cell
                PopulateCellStreams(pkCell, kPositions, kNormals, kTangents);
            }
        }
    }   
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::PopulateCellStreams(NiTerrainCell* pkCell, 
    NiTerrainPositionRandomAccessIterator& kPositions,
    NiTerrainNormalRandomAccessIterator& kNormals,
    NiTerrainTangentRandomAccessIterator& kTangents)
{
    // Check that the required streams are here:
    if (m_pfHeights == NULL)
        return false;

    // Get the terrain configuration
    NiTerrainConfiguration kConfig = m_pkSector->GetConfiguration();

    // Calculate some geometry offsets
    NiTerrainSectorData* pkSectorData = m_pkSector->GetSectorData();
    NiUInt32 uiSectorSize = pkSectorData->GetSectorWidthInVerts();
    NiInt32 iCellSize = pkSectorData->GetCellWidthInVerts();
    NiIndex kStartIndex;
    NiInt32 iStride = 1 << pkCell->GetNumSubDivisions();
    pkCell->GetBottomLeftIndex(kStartIndex);
    float fPositionOffset = float(pkSectorData->GetSectorSize()) * -0.5f;

    // Calculate which pixels are within the build region
    NiInt32 iMinX = m_kBuildRegion.m_left / iStride + ((m_kBuildRegion.m_left % iStride)?1:0);
    NiInt32 iMaxX = m_kBuildRegion.m_right / iStride;
    NiInt32 iMinY = m_kBuildRegion.m_bottom / iStride + ((m_kBuildRegion.m_bottom % iStride)?1:0);
    NiInt32 iMaxY = m_kBuildRegion.m_top / iStride;

    iMinX = NiClamp(iMinX - NiInt32(kStartIndex.x / iStride), 0, iCellSize - 1);
    iMaxX = NiClamp(iMaxX - NiInt32(kStartIndex.x / iStride), 0, iCellSize - 1);
    iMinY = NiClamp(iMinY - NiInt32(kStartIndex.y / iStride), 0, iCellSize - 1);
    iMaxY = NiClamp(iMaxY - NiInt32(kStartIndex.y / iStride), 0, iCellSize - 1);

    // Loop through all vertexes stored in the streams and populate new heights/lighting
    for (NiInt32 iY = iMinY; iY <= iMaxY; ++iY)
    {
        NiInt32 iSectorIndexY = kStartIndex.y + iY * iStride;
        for (NiInt32 iX = iMinX; iX <= iMaxX; ++iX)
        {
            NiInt32 iSectorIndexX = kStartIndex.x + iX * iStride;
            
            // Calculate the index of the vert to modify:
            NiUInt32 uiStreamIndex = iX + iY * iCellSize;
            NiUInt32 uiSectorIndex = iSectorIndexX + iSectorIndexY * uiSectorSize;

            // Set the position of this vertex:
            kPositions.SetComponent(uiStreamIndex, NiTerrainPositionRandomAccessIterator::X,
                float(iSectorIndexX) + fPositionOffset);
            kPositions.SetComponent(uiStreamIndex, NiTerrainPositionRandomAccessIterator::Y,
                float(iSectorIndexY) + fPositionOffset);
            kPositions.SetComponent(uiStreamIndex, NiTerrainPositionRandomAccessIterator::Z,
                SampleHeightMap(iSectorIndexX, iSectorIndexY));

            // Set the normal of this vertex:
            if (kNormals.Exists() && m_kNormalIterator.Exists())
            {
                NiPoint2 kNormal = NiPoint2::ZERO;
                kNormal = m_kNormalIterator[uiSectorIndex];
                kNormals.SetHighDetail(uiStreamIndex, kNormal);
            }

            // Set the tangent of this vertex:
            if (kTangents.Exists() && m_kTangentIterator.Exists())
            {
                NiPoint2 kTangent = m_kTangentIterator[uiSectorIndex];
                kTangents.SetHighDetail(uiStreamIndex, kTangent);
            }
        }
    }

    // Populate the morphing now that the heights are up to date
    if (m_bOutputMorphing)
    {
        // Expand the region to update morphing for as those bordering values may have been
        // calculated by sampling old values
        iMinX = NiClamp(iMinX - 1, 0, iCellSize);
        iMaxX = NiClamp(iMaxX + 1, 0, iCellSize);
        iMinY = NiClamp(iMinY - 1, 0, iCellSize);
        iMaxY = NiClamp(iMaxY + 1, 0, iCellSize);

        for (NiInt32 iY = iMinY; iY < iMaxY; ++iY)
        {
            for (NiInt32 iX = iMinX; iX < iMaxX; ++iX)
            {
                // Calculate the index of the vert to modify:
                NiUInt32 uiStreamIndex = iX + iY * iCellSize;
                NiTerrainPositionRandomAccessIterator::COMPONENT eHeightComponent = 
                    NiTerrainPositionRandomAccessIterator::Z;

                // Calculate the morphing values for this cell
                bool bXOdd = iX % 2 != 0;
                bool bYOdd = iY % 2 != 0;
                bool bStraightCopy = 
                    (pkCell->GetLevel() == 0) || // Lowest level of detail
                    (!bXOdd && !bYOdd); // Verts were on the previous LOD as well.

                // Set the morph height data of this vertex: 
                float fMorphHeight = 0.0f;
                if (bStraightCopy) // Vert was on previous LOD as well
                {
                    fMorphHeight = 2.0f * kPositions.GetComponent(uiStreamIndex, eHeightComponent);
                }
                else if (bXOdd && bYOdd) // On a diagonal?
                {
                    // Which face is it?
                    if ((iX % 4) == (iY % 4))
                    {
                        // bottom left
                        fMorphHeight += kPositions.GetComponent((iX - 1) + (iY - 1) * iCellSize, 
                            eHeightComponent);
                        // top right
                        fMorphHeight += kPositions.GetComponent((iX + 1) + (iY + 1) * iCellSize, 
                            eHeightComponent);    
                    }
                    else
                    {
                        // bottom right
                        fMorphHeight += kPositions.GetComponent((iX + 1) + (iY - 1) * iCellSize, 
                            eHeightComponent);
                        // top left
                        fMorphHeight += kPositions.GetComponent((iX - 1) + (iY + 1) * iCellSize, 
                            eHeightComponent);
                    }
                }
                // Horizontal?
                else if (bXOdd)
                {
                    // Left
                    fMorphHeight += kPositions.GetComponent((iX - 1) + (iY + 0) * iCellSize, 
                        eHeightComponent);
                    // Right
                    fMorphHeight += kPositions.GetComponent((iX + 1) + (iY + 0) * iCellSize, 
                        eHeightComponent);
                }
                // Vertical
                else if (bYOdd)
                {
                    // Bottom
                    fMorphHeight += kPositions.GetComponent((iX + 0) + (iY - 1) * iCellSize, 
                        eHeightComponent);
                    // Top
                    fMorphHeight += kPositions.GetComponent((iX + 0) + (iY + 1) * iCellSize, 
                        eHeightComponent);
                }
                fMorphHeight /= 2.0f;

                kPositions.SetComponent(uiStreamIndex, NiTerrainPositionRandomAccessIterator::W, 
                    fMorphHeight);
            }
        }
    }

    // Apply bounding information
    if (m_pkBounds)
    {
        efd::UInt32 uiBeginIndex = m_pkSector->GetCellOffset(m_iOriginalLOD + 1);
        efd::UInt32 uiIndex = pkCell->GetRegionID() + m_pkSector->GetCellOffset(pkCell->GetLevel());
        uiIndex -= uiBeginIndex;
        pkCell->SetBoundData(m_pkBounds[uiIndex].m_kBound.GetCenter(), 
            m_pkBounds[uiIndex].m_kBound.GetRadius());
        pkCell->SetBoundVolumeBox(m_pkBounds[uiIndex].m_kBox);
    }
    else
    {
        pkCell->RequestBoundsUpdate();
    }

    // Apply surface information
    if (m_pkSurfaceIndexes && pkCell->GetLevel() == m_pkTerrain->GetNumLOD())
    {
        efd::UInt32 uiIndex = pkCell->GetRegionID();
        for (efd::UInt32 uiSlot = 0; uiSlot < m_pkSurfaceIndexes[uiIndex].m_uiNumSurfaces; 
            ++uiSlot)
        {
            // Transform the sector's surface reference into a terrain wide surface index
            efd::SInt32 iSectorSurfaceIndex = 
                m_pkSurfaceIndexes[uiIndex].m_auiSurfaceIndex[uiSlot];
            NiTerrainSectorFile::SurfaceReference kReference;
            EE_VERIFY(m_kSurfaceReferences.find(iSectorSurfaceIndex, kReference));
            efd::SInt32 iTerrainSurfaceIndex = kReference.m_iSurfaceIndex;
            EE_ASSERT(iTerrainSurfaceIndex != -1);
            
            // Add the surface to the cell
            pkCell->AddSurface(iTerrainSurfaceIndex);
        }
    }

    pkCell->MarkCellLoaded();

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::ApplyTextures()
{
    // Low detail normal map
    if (m_bOutputLowDetailNormalMap && m_spNormalMap)
    {
        // Upload the low detail normal map texture
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, m_pkSector->GetTexture(
            NiTerrainCell::TextureType::LOWDETAIL_NORMAL));
        if (!pkTexture)
        {
            pkTexture = m_pkTerrain->GetResourceManager()->CreateTexture(
                NiTerrain::TextureType::LOWDETAIL_NORMAL, m_spNormalMap);
            m_pkSector->SetTexture(NiTerrain::TextureType::LOWDETAIL_NORMAL, pkTexture);
            EE_ASSERT(pkTexture->GetSourcePixelData() && !pkTexture->GetStatic());
        }
    }

    // Low detail diffuse map
    if (m_bOutputLowDetailDiffuseMap && m_spDiffuseMap)
    {
        // Upload the low detail diffuse map texture
        NiSourceTexture* pkTexture = m_pkTerrain->GetResourceManager()->CreateTexture(
            NiTerrain::TextureType::LOWDETAIL_DIFFUSE, m_spDiffuseMap);
        m_pkSector->SetTexture(NiTerrain::TextureType::LOWDETAIL_DIFFUSE, pkTexture);
    }

    // Blend masks
    if (m_pkDividedBlendMasks)
    {
        efd::UInt32 uiNumLOD = m_pkTerrain->GetNumLOD();
        efd::UInt32 uiNumCellsAccross = 1 << uiNumLOD;
        efd::UInt32 uiCellsPerMaskSide = uiNumCellsAccross / m_uiNumBlendMasksPerSide;

        // Iterate over all the blend masks upload them, and apply them to their cells
        for (efd::UInt32 uiMaskX = 0; uiMaskX < m_uiNumBlendMasksPerSide; ++uiMaskX)
        {
            for (efd::UInt32 uiMaskY = 0; uiMaskY < m_uiNumBlendMasksPerSide; ++uiMaskY)
            {           
                // Upload the texture
                NiPixelData* pkPixelData = 
                    m_pkDividedBlendMasks[uiMaskY * m_uiNumBlendMasksPerSide + uiMaskX];
                m_pkDividedBlendMasks[uiMaskY * m_uiNumBlendMasksPerSide + uiMaskX] = NULL;
                NiTexture* pkTexture = m_pkTerrain->GetResourceManager()->CreateTexture(
                    NiTerrain::TextureType::BLEND_MASK, pkPixelData);
                EE_ASSERT(pkTexture);

                // Figure out which cells are relevant
                NiIndex kCellOffset;
                kCellOffset.x = uiMaskX * uiCellsPerMaskSide;
                kCellOffset.y = uiMaskY * uiCellsPerMaskSide;

                // Iterate over all these cells
                for (efd::UInt32 uiCellX = 0; uiCellX < uiCellsPerMaskSide; ++uiCellX)
                {
                    for (efd::UInt32 uiCellY = 0; uiCellY < uiCellsPerMaskSide; ++uiCellY)
                    {
                        // Calculate the ID of the cell at this position on this level
                        NiUInt32 uiCellID = (uiCellX + kCellOffset.x) + 
                            (uiCellY  + kCellOffset.y) * uiNumCellsAccross;
                        uiCellID += m_pkSector->GetCellOffset(uiNumLOD);

                        // Fetch this cell
                        NiTerrainCell* pkCell = m_pkSector->GetCell(uiCellID);
                        EE_ASSERT(pkCell);

                        // Apply the texture
                        pkCell->SetTexture(pkTexture, NiTerrain::TextureType::BLEND_MASK);

                        // Figure out what region it should be using 
                        // (remember y is flipped in textures)
                        float fScale = float(uiCellsPerMaskSide);
                        float fOffsetScale = 1.0f / fScale;
                        NiPoint2 kOffset;
                        kOffset.x = uiCellX * fOffsetScale;
                        kOffset.y = (uiCellsPerMaskSide - uiCellY - 1) * fOffsetScale;
                        pkCell->SetTextureRegion(kOffset, fScale, 
                            NiTerrain::TextureType::BLEND_MASK);
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingTask::FinilizeSectorLoading()
{
    // Update the sector's height map to the one used for loading (possibly the same one)
    m_pkSector->SetHeightMap(m_aspHeightMaps[1][1]);

    // Build the bounds, propertys, and update from parent objects.
    m_pkSector->BuildData(m_iTargetLOD);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingTask::Cleanup()
{   
    // Make sure that the files are closed
    for (NiInt32 iX = -1; iX < 2; ++iX)
    {
        for (NiInt32 iY = -1; iY < 2; ++iY)
        {
            NiTerrainSectorFile* pkFile = m_aspSectorFiles[iX + 1][iY + 1];
            if (pkFile && pkFile->IsReady())
            {
                pkFile->Close();
            }
        }
    }

    // Release resources used when loading
    NiTerrainResourceManager* pkResources = m_pkTerrain->GetResourceManager();

    pkResources->ReleaseBuffer(m_pfHeights);
    pkResources->ReleaseBuffer(m_pkNormals);
    pkResources->ReleaseBuffer(m_pkTangents);
    pkResources->ReleaseBuffer(m_pkBounds);
    pkResources->ReleaseBuffer(m_pkSurfaceIndexes);

    if (m_pkDividedBlendMasks)
    {
        // Don't delete the actual masks because they'll be in use by the textures
        // We'll just check that they have all been used
        for (efd::UInt32 uiIndex = 0; uiIndex < m_uiNumBlendMasksPerSide * m_uiNumBlendMasksPerSide;
            ++uiIndex)
        {
            EE_ASSERT(!m_pkDividedBlendMasks[uiIndex]);
        }
        EE_FREE(m_pkDividedBlendMasks);
    }

    // Free any buffers we needed now (frees them for other tasks)
    m_spDiffuseMap = NULL;
    m_spNormalMap = NULL;
    for (NiUInt32 uiIndex = 0; uiIndex < 9; ++uiIndex)
    {
        m_aspHeightMaps[uiIndex / 3][uiIndex % 3] = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
