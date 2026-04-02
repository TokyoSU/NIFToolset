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

#include <NiTPointerList.h>
#include <NiImageConverter.h>
#include <NiFilename.h>

#include "NiTerrain.h"
#include "NiTerrainSector.h"
#include "NiTerrainSectorFile.h"
#include "NiTerrainXMLHelpers.h"
#include "NiTerrainStreamLocks.h"
#include "NiTerrainStreamingManager.h"

using namespace efd;

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::CreateBlankGeometry()
{
    // Delegate the generation of a blank sector to the streaming manager
    m_pkTerrain->GetStreamingManager()->RequestCreateBlankSector(this, true);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSector::Save(const char* pcArchive, NiUInt32* puiErrorCode)
{
    // Set the default archive path if required
    if (!pcArchive)
        pcArchive = m_pkTerrain->GetArchivePath();

    EE_ASSERT(pcArchive);
    EE_ASSERT(puiErrorCode);
    
    // Make sure the prerequisites are available
    if (!NiTerrain::InToolMode())
        return false;
    if (!m_pkSectorData->IsLODLoaded(m_pkTerrain->GetNumLOD()))
        return false;

    // Attempt to open the file to write
    NiInt16 iSectorX = m_pkSectorData->GetSectorIndexX();
    NiInt16 iSectorY = m_pkSectorData->GetSectorIndexY();
    NiTerrainSectorFile* pkFile = m_pkTerrain->OpenSectorFile(iSectorX, iSectorY, true, pcArchive);
    if (!pkFile)
    {
        *puiErrorCode |= NiTerrain::EC_SECTOR_IS_READ_ONLY;
        return false;
    }

    // Initialize a buffer to store the normals/tangents and heights
    NiUInt32 uiSectorWidthInVerts = m_pkSectorData->GetSectorWidthInVerts();
    NiUInt32 uiStreamWidth = uiSectorWidthInVerts;
    NiUInt32 uiNumDataPoints = uiStreamWidth * uiStreamWidth;

    // Initialize the file header
    pkFile->WriteSectorConfig(uiSectorWidthInVerts, m_pkTerrain->GetNumLOD());

    // Setup stream locks
    NiTerrainStreamLocks kLock;
    NiTerrainNormalRandomAccessIterator kCellNormals;
    NiTerrainTangentRandomAccessIterator kCellTangents;
    NiUInt8 ucMask = NiDataStream::LOCK_READ;

    // Setup stream information
    efd::UInt32 uiNumLOD = m_pkTerrain->GetNumLOD();
    efd::UInt32 uiBeginIndex = GetCellOffset(uiNumLOD);
    efd::UInt32 uiEndIndex = m_kCellArray.GetSize();
    efd::UInt32 uiCellWidthInVerts = m_pkSectorData->GetCellWidthInVerts();
    efd::UInt32 uiNumCellsAcross = (1 << uiNumLOD);
    efd::UInt32 uiNumPixelsPerCell = m_pkTerrain->GetMaskSize() / uiNumCellsAcross;

    // Initialize the required streams
    NiTStridedRandomAccessIterator<efd::Point2> kNormalIterator;
    NiTStridedRandomAccessIterator<efd::Point2> kTangentIterator;
    efd::Point2* pkNormals = EE_ALLOC(efd::Point2, uiNumDataPoints);
    kNormalIterator = NiTStridedRandomAccessIterator<efd::Point2>(pkNormals, 
        sizeof(efd::Point2));
    efd::Point2* pkTangents = EE_ALLOC(efd::Point2, uiNumDataPoints);
    kTangentIterator = NiTStridedRandomAccessIterator<efd::Point2>(pkTangents, 
        sizeof(efd::Point2));
    NiTerrainSectorFile::LeafData* pkLeafData = 
        EE_ALLOC(NiTerrainSectorFile::LeafData, uiEndIndex - uiBeginIndex);
    NiPixelDataPtr spBlendMask = EE_NEW NiPixelData(m_pkTerrain->GetMaskSize(), 
        m_pkTerrain->GetMaskSize(), NiPixelFormat::RGBA32);
    memset(spBlendMask->GetPixels(), 0, spBlendMask->GetSizeInBytes());
    NiTerrainSectorFile::SurfaceReferenceMap kSurfaceReferences;

    // Extract the height geometry from the sector
    efd::UInt16* pusHeights = m_spHeightMap->Lock(NiTerrainSector::HeightMap::LockType::READ);
    
    // Extract the lighting geometry from all the leafs
    for (efd::UInt32 uiRegion = uiBeginIndex; uiRegion < uiEndIndex; ++uiRegion)
    {
        NiTerrainCellLeaf* pkLeaf = NiDynamicCast(NiTerrainCellLeaf, GetCellByRegion(uiRegion));
        EE_ASSERT(pkLeaf);

        // Figure out where this leaf belongs in the scheme of things
        NiIndex kBottomLeft;
        pkLeaf->GetBottomLeftIndex(kBottomLeft);

        // Lock this cell's streams for read
        kLock.GetNormalIterator(pkLeaf, ucMask, kCellNormals);
        kLock.GetTangentIterator(pkLeaf, ucMask, kCellTangents);

        // Extract the geometry
        for (efd::UInt32 uiY = 0; uiY < uiCellWidthInVerts; ++uiY)
        {
            for (efd::UInt32 uiX = 0; uiX < uiCellWidthInVerts; ++uiX)
            {
                efd::UInt32 uiVertIndex = uiX + uiY * uiCellWidthInVerts;

                // Extract the normal from this vertex
                NiPoint2 kNormal;
                kCellNormals.GetHighDetail(uiVertIndex, kNormal);
                
                // Extract the tangent from this vertex
                NiPoint2 kTangent;
                kCellTangents.GetHighDetail(uiVertIndex, kTangent);

                // Figure out where this data belongs in the final set of streams
                efd::UInt32 uiSectorIndex = 
                    (uiX + kBottomLeft.x) + (uiY + kBottomLeft.y) * uiStreamWidth;
                kNormalIterator[uiSectorIndex] = kNormal;
                kTangentIterator[uiSectorIndex] = kTangent;
            }
        }

        // Get surface information
        efd::UInt32& uiNumSurfaces = pkLeafData[uiRegion - uiBeginIndex].m_uiNumSurfaces;
        efd::UInt32* puiSurfaceIndex = pkLeafData[uiRegion - uiBeginIndex].m_auiSurfaceIndex;
        uiNumSurfaces = pkLeaf->GetSurfaceCount();
        for (NiUInt32 uiSlot = 0; uiSlot < NiTerrainCellLeaf::MAX_NUM_SURFACES; ++uiSlot)
        {
            if (uiSlot < uiNumSurfaces)
            {
                efd::SInt32 iSurfaceIndex = pkLeaf->GetSurfaceIndex(uiSlot);
                puiSurfaceIndex[uiSlot] = iSurfaceIndex;

                // Make sure this index is stored in the sector wide surface references
                if (kSurfaceReferences.find(iSurfaceIndex) == kSurfaceReferences.end())
                    kSurfaceReferences[iSurfaceIndex] = NiTerrainSectorFile::SurfaceReference();
            }
            else
                puiSurfaceIndex[uiSlot] = NULL;
        }

        // Extract the blend mask
        NiPixelData* pkPixelData = NiTerrainUtils::ExtractPixelData(
            pkLeaf->GetTexture(NiTerrainCell::TextureType::BLEND_MASK));
        NiTextureRegion kRegion = pkLeaf->GetTextureRegion(NiTerrainCell::TextureType::BLEND_MASK);
        if (pkPixelData)
        {
            // Figure out the position of the blend mask on the sector wide blend mask
            efd::UInt32 uiIndex = pkLeaf->GetCellID() - GetCellOffset(uiNumLOD);
            efd::UInt32 uiCellX = uiIndex % uiNumCellsAcross;
            efd::UInt32 uiCellY = uiIndex / uiNumCellsAcross;
            NiIndex kBufferOffset;
            kBufferOffset.x = uiCellX * uiNumPixelsPerCell;
            kBufferOffset.y = (uiNumCellsAcross - uiCellY - 1) * uiNumPixelsPerCell;

            // Figure out the segment of the blend mask that the leaf is using
            NiIndex kTextureStart = kRegion.GetStartPixelIndex() - NiIndex(1,1);
            NiIndex kTextureEnd = kRegion.GetEndPixelIndex() + NiIndex(1,1);
            NiIndex kTextureRange = kTextureEnd - kTextureStart;
            EE_ASSERT(kTextureRange.x == uiNumPixelsPerCell);
            EE_ASSERT(kTextureRange.y == uiNumPixelsPerCell);

            // Figure out the length in bytes of each scanline
            efd::UInt32 uiStride = pkPixelData->GetPixelStride();
            efd::UInt32 uiScanlineLength = uiNumPixelsPerCell * uiStride;

            // Copy the segment of this texture that the leaf is using into the overall buffer
            for (efd::UInt32 uiY = 0; uiY < kTextureRange.y; ++uiY)
            {
                // Figure out the beginning of the buffer scanline
                efd::UInt32 uiBufferBeginIndex = 
                    ((uiY + kBufferOffset.y) * spBlendMask->GetWidth()) + kBufferOffset.x;
                void* pvDest = &spBlendMask->GetPixels()[uiBufferBeginIndex * uiStride];

                // Figure out the beginning of the texture scanline
                efd::UInt32 uiTextureBeginIndex = 
                    ((uiY + kTextureStart.y) * pkPixelData->GetWidth()) + kTextureStart.x;
                void* pvSrc = &pkPixelData->GetPixels()[uiTextureBeginIndex * uiStride];

                // Execute scanline copy
                NiMemcpy(pvDest, pvSrc, uiScanlineLength);
            }
        }
    }

    // Go over the surface references and collect the information required to store them
    NiTerrainSectorFile::SurfaceReferenceMap::iterator kIter;
    for (kIter = kSurfaceReferences.begin(); kIter != kSurfaceReferences.end(); ++kIter)
    {
        efd::utf8string kPackageID;
        efd::utf8string kPackageLocation;
        efd::utf8string kSurfaceID;
        efd::UInt32 uiPackageIteration;
        m_pkTerrain->GetSurfaceEntry(kIter->first, kPackageID, kPackageLocation, kSurfaceID, 
            uiPackageIteration);

        NiTerrainSectorFile::SurfaceReference& kRef = kIter->second;
        kRef.m_iSurfaceIndex = kIter->first;
        kRef.m_kSurfaceName = kSurfaceID;
        kRef.m_uiPackageIteration = uiPackageIteration;
        kRef.m_spPackageReference = NiNew NiTerrainAssetReference();
        kRef.m_spPackageReference->SetAssetID(kPackageID);

        if (efd::PathUtils::IsAbsolutePath(kPackageLocation))
        {
            kRef.m_spPackageReference->SetResolvedLocation(kPackageLocation);
        }
        else
        {
            kRef.m_spPackageReference->SetRelativeAssetLocation(kPackageLocation);
        }
    }

    // Output the geometry
    pkFile->WriteHeights(pusHeights, uiNumDataPoints);
    pkFile->WriteNormals(pkNormals, uiNumDataPoints);
    pkFile->WriteTangents(pkTangents, uiNumDataPoints);
    pkFile->WriteCellSurfaceIndex(0, uiEndIndex - uiBeginIndex, pkLeafData);
    pkFile->WriteSurfaceReferences(kSurfaceReferences);

    // Cleanup Geometry output
    m_spHeightMap->Unlock(NiTerrainSector::HeightMap::LockType::READ);
    EE_FREE(pkNormals);
    EE_FREE(pkTangents);
    EE_FREE(pkLeafData);

    // Output the cell details
    NiTerrainSectorFile::CellData* pkCellData = 
        EE_ALLOC(NiTerrainSectorFile::CellData, uiEndIndex);
    for (NiUInt32 uiRegion = 0; uiRegion < uiEndIndex; ++uiRegion)
    {
        NiTerrainCell* pkCell = GetCellByRegion(uiRegion);

        // Get bounding information
        pkCellData[uiRegion].m_kBound = pkCell->GetLocalBound();
        pkCellData[uiRegion].m_kBox = pkCell->GetLocalBoxBound();
    }
    pkFile->WriteCellBoundData(0, uiEndIndex, pkCellData);
    EE_FREE(pkCellData);

    // Output the textures
    {
        NiPixelDataPtr spPixData;

        // Low detail normal map
        if (m_pkLowDetailNormalTexture)
        {
            spPixData = NiTerrainUtils::ExtractPixelData(m_pkLowDetailNormalTexture);
            pkFile->WriteLowDetailNormalMap(spPixData);
        }

        // Low detail diffuse map
        if (m_pkLowDetailDiffuseTexture)
        {
            spPixData = NiTerrainUtils::ExtractPixelData(m_pkLowDetailDiffuseTexture);
            if (!spPixData && IsReadyToPaint())
            {
                spPixData = 
                    NiTerrainUtils::ExtractPixelData(m_pkPaintingData->m_spLowDetailDiffuseRenderClick);
            }
            pkFile->WriteLowDetailDiffuseMap(spPixData);
        }

        // Output blend mask
        pkFile->WriteBlendMask(spBlendMask);
    }

    // Output custom data
    NiTerrain::CustomDataPolicy* pkCustomDataPolicy = m_pkTerrain->GetCustomDataPolicy();
    if (pkCustomDataPolicy)
    {
        if (!pkCustomDataPolicy->SaveCustomData(this, pkFile))
            *puiErrorCode |= NiTerrain::EC_SECTOR_SAVE_CUSTOM_DATA_FAILED;
    }

    // Close the file
    pkFile->Close();
    EE_DELETE pkFile;

    *puiErrorCode |= NiTerrain::EC_SECTOR_SAVED;
    return true;
}

//--------------------------------------------------------------------------------------------------
NiSourceTexture* NiTerrainSector::CreateBlendTexture(NiUInt32 uiTextureSize)
{
    NiSourceTexture* pkBlendTexture = NULL;

    // Cases where the creator didn't care about the size of the texture
    if (uiTextureSize == 0)
        uiTextureSize = m_pkTerrain->GetMaskSize() >> m_pkTerrain->GetNumLOD();

    // The texture must be large enough to include borders, so 4x4 is the 
    // minimum size of a blend texture.
    EE_ASSERT(uiTextureSize >= 4);
    NiPixelDataPtr spPixelData = NiNew NiPixelData(uiTextureSize, uiTextureSize, 
        NiPixelFormat::RGBA32);

    efd::UInt8* pucPixels = spPixelData->GetPixels();
    NiUInt32 uiWidth = spPixelData->GetWidth();
    // Clear the buffer
    memset(pucPixels, 0, uiWidth * uiWidth * 4);
    
    // Create the appropriate texture out of the generated pixel data
    pkBlendTexture = GetResourceManager()->CreateTexture(
        NiTerrain::TextureType::BLEND_MASK, spPixelData);
    spPixelData = 0;

    return pkBlendTexture;
}

//--------------------------------------------------------------------------------------------------
