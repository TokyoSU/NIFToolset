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
#include "NiTerrainSectorFileVersion3.h"
#include "NiTerrainFile.h"
#include "NiTerrainUtils.h"
#include "NiIndex.h"
#include "NiTerrainFileVersion2.h"
#include "NiTerrain.h"
#include "NiTerrainXMLHelpers.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainSectorFileVersion3, NiTerrainSectorFileVersion5, NiTypeMask::NiTerrainSectorFileVersion3);
//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion3::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to open the file:
    NiString kSectorFile = kID.m_kSectorFile.c_str();
    efd::File *pkFile = efd::File::GetFile(kSectorFile, efd::File::READ_ONLY);
    if (pkFile)
    {
        bool bPlatformLittle = NiSystemDesc::GetSystemDesc().IsLittleEndian();    
        pkFile->SetEndianSwap(!bPlatformLittle);

        // Read the file header so we can inspect the version.
        FileHeader kFileHeader;
        kFileHeader.LoadBinary(*pkFile);

        eVersion = kFileHeader.m_kVersion;

        // Close the file
        NiDelete pkFile;
    }

    return eVersion == 3;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion3::NiTerrainSectorFileVersion3()
    : m_pucReducedOriginalBlendMask(0)
    , m_pucCurrentBlockBlendMask(0)
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;

    // Initialize the low detail texture stream to a white 4x4 RGBA texture
    for (NiUInt32 uiIndex = 0; uiIndex < sizeof(m_auiDefaultLowDetailTexture);
        ++uiIndex)
    {
        m_auiDefaultLowDetailTexture[uiIndex] = 255;
    }

    // Initialize the surface index stream to point to no surfaces
    for (NiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
    {
        m_aiCurrentSurfaceIndexStream[uiIndex] = -1;
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion3::~NiTerrainSectorFileVersion3()
{
    NiFree(m_pucReducedOriginalBlendMask);
    NiFree(m_pucCurrentBlockBlendMask);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion3::OpenErrorCode NiTerrainSectorFileVersion3::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    OpenErrorCode result = NiTerrainSectorFileVersion5::Open(kID, eAccessMode);
    return result;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion3::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize base file
    bool bResult = NiTerrainSectorFileVersion5::Initialize();
    if (!bResult)
        return false;

    // Extract the folder of the quadtree file
    NiString kSectorPath = m_kSectorFile.GetSubstring(0, 
        m_kSectorFile.FindReverse('\\'));

    // Initialize the blend mask conversion in this version of the file.
    const NiUInt32 uiMaxLayers = 4;
    NiPixelDataPtr spOriginalPixels;
    NiDevImageConverter kImageConverter;
    NiUInt32 uiMaskSize = 0;
    for (NiUInt32 uiLayer = 0; uiLayer < uiMaxLayers; ++uiLayer)
    {
        // Attempt to load the blend mask
        NiString kLayerPath;
        kLayerPath.Format("%s\\%d.tga", (const char*)kSectorPath, uiLayer);
        NiPixelDataPtr spPixelData;
        if (kImageConverter.CanReadImageFile(kLayerPath))
        {
            spPixelData = kImageConverter.ReadImageFile(kLayerPath, 0);
        }
        if (!spPixelData)
        {
            continue;
        }

        // Create a new pixel data if necessary
        if (!spOriginalPixels)
        {
            uiMaskSize = spPixelData->GetWidth();
            spOriginalPixels = NiNew NiPixelData(uiMaskSize, uiMaskSize,
                NiPixelFormat::RGBA32);
        }

        // Copy it's data into the new pixel data
        NiUInt8* pucOrigPixels = spOriginalPixels->GetPixels();
        NiUInt8* pucMaskPixels = spPixelData->GetPixels();
        for (NiUInt32 uiY = 0; uiY < uiMaskSize; ++uiY)
        {
            for (NiUInt32 uiX = 0; uiX < uiMaskSize; ++uiX)
            {
                NiUInt32 uiNewIndex = uiY * uiMaskSize + uiX;
                NiUInt32 uiOldIndex = (uiMaskSize - uiY - 1) * uiMaskSize + uiX;
                pucOrigPixels[uiNewIndex * uiMaxLayers + uiLayer] = 
                    pucMaskPixels[uiOldIndex * spPixelData->GetPixelStride()];
            }
        }
    }

    // Run a filter over this blend mask texture to make sure that the sum of 
    // the channels is normalized to 255 at all points.
    NiUInt8* pucOrigPixels = spOriginalPixels->GetPixels();
    for (NiUInt32 uiY = 0; uiY < uiMaskSize; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiMaskSize; ++uiX)
        {
            NiUInt32 uiNewIndex = (uiY * uiMaskSize + uiX) * uiMaxLayers;
            NiUInt32 uiMaskSum = 0;
            for (NiUInt32 uiLayer = 0; uiLayer < uiMaxLayers; ++uiLayer)
            {
                uiMaskSum += pucOrigPixels[uiNewIndex  + uiLayer];
            }

            float fInvMaskSum = 1.0f;
            if (uiMaskSum > 0)
                fInvMaskSum = 1.0f / (float)uiMaskSum;

            for (NiUInt32 uiLayer = 0; uiLayer < uiMaxLayers; ++uiLayer)
            {
                float fFactor = fInvMaskSum * (float)
                    pucOrigPixels[uiNewIndex  + uiLayer];
                NiUInt8 ucValue = (NiUInt8)(fFactor * 255.0f);

                pucOrigPixels[uiNewIndex  + uiLayer] = ucValue;
            }
        }
    }

    // Don't continue if the mask was never generated.
    if (uiMaskSize == 0)
        return true;

    // Count the number of levels of detail in this file
    NiUInt32 uiNumLevels = 0;
    while (PushBlock(0))
        uiNumLevels++;
    m_uiHighDetailLeafOffset = GetBlockID();
    m_uiNumLOD = uiNumLevels;
    while (PopBlock());

    // Resize the blend mask to an appropriate size if it is too small
    efd::UInt32 uiAppropriateMaskSize = 4 << uiNumLevels;
    if (uiMaskSize < uiAppropriateMaskSize)
    {
        NiPixelData* pkLargerMaskSize = NiNew NiPixelData(uiAppropriateMaskSize, 
            uiAppropriateMaskSize, NiPixelFormat::RGBA32);
        NiTerrainUtils::ResizeImage(spOriginalPixels->GetPixels(), uiMaskSize, 
            pkLargerMaskSize->GetPixels(), uiAppropriateMaskSize, 4);
        uiMaskSize = uiAppropriateMaskSize;
        spOriginalPixels = pkLargerMaskSize;
    }

    // Resize this blend mask to conform to pixel duplication requirements
    NiUInt32 uiCellMaskWidth = uiMaskSize >> uiNumLevels;
    NiUInt32 uiNewWidth = (uiCellMaskWidth - 2) << uiNumLevels;
    m_uiOriginalMaskWidth = uiNewWidth;
    m_uiCellMaskWidth = uiCellMaskWidth;

    m_pucReducedOriginalBlendMask = 
        NiAlloc(NiUInt8, uiNewWidth * uiNewWidth * 4);
    NiTerrainUtils::ResizeImage(spOriginalPixels->GetPixels(), uiMaskSize, 
        m_pucReducedOriginalBlendMask, uiNewWidth, 4);

    // Create a blank stream for delivering block values from.
    m_pucCurrentBlockBlendMask = NiAlloc(NiUInt8, uiCellMaskWidth * 
        uiCellMaskWidth * 4);

    // Detect exactly how many surfaces are used in this file
    NiString kFilePath;
    kFilePath.Format("%s\\surfaces.xml", (const char*)kSectorPath);
    efd::TiXmlDocument kFile((const char*)kFilePath);
    if (!NiTerrainXMLHelpers::LoadXMLFile(&kFile))
        return false;
    efd::TiXmlElement* pkDocument = kFile.FirstChildElement("Block");
    const efd::TiXmlElement* pkCurrentElement = pkDocument->FirstChildElement();
    if (pkCurrentElement)
    {
        const char* pcLayerNum = 0;
        // Loop through all surfaces in the list and collect the information
        while (pkCurrentElement)
        {
            pcLayerNum = pkCurrentElement->Attribute("position");
            if (pcLayerNum == 0)
                pcLayerNum = "";

            // Convert the layer number into an index
            NiUInt32 uiSurfaceIndex = 0;
            if (NiString(pcLayerNum).ToUInt(uiSurfaceIndex))
            {
                m_aiCurrentSurfaceIndexStream[uiSurfaceIndex] = uiSurfaceIndex;
            }

            // Move onto the next surface
            pkCurrentElement = pkCurrentElement->NextSiblingElement();
        }
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion3::NextBlock()
{
    if (!IsReady() || IsWritable())
        return false;

    // Pop the last position on the stack as we are no longer there:
    if (m_kPositionStack.GetSize() != 0)
    {
        m_kPositionStack.RemoveTail();
    }
    // Set the current position on the stack
    m_kPositionStack.AddTail(m_ulFilePosition);

    // Reset the stream information from the previous block (keep the buffers)
    // Read in all of this block's data though!
    NiUInt32 uiStreamNumber;
    for (uiStreamNumber = 0; uiStreamNumber < STREAM_MAX_NUMBER; ++uiStreamNumber)
    {
        m_akStreamHeader[uiStreamNumber].m_kStreamType = STREAM_INVALID;
    }

    // Update the identity information on this block!
    m_iCurrentBlockID++;
    if (m_iCurrentBlockID >= (4 * m_iCurrentBlockLevelIndex + 1))
    {
        m_iCurrentBlockLevelIndex = m_iCurrentBlockID;
        m_iCurrentBlockLevel++;
    }

    // Read in the block header:
    // No need to seek, we are already at the next block!
    LoadLegacyBlockHeaderBinary(m_kCurrentBlockHeader, *m_pkFile);
    m_ulFilePosition += sizeof(m_kCurrentBlockHeader);

    // Read in all the available streams:
    for (uiStreamNumber = 0; uiStreamNumber < STREAM_MAX_NUMBER; ++uiStreamNumber)
    {
        if (!(m_kCurrentBlockHeader.m_kPresentData.m_uiBitField &
            (1 << uiStreamNumber)))
        {
            // This stream is not present
            continue;
        }

        // Read in the stream's header:
        DataStreamHeader kCurrentStreamHeader;
        kCurrentStreamHeader.LoadBinary(*m_pkFile);
        m_ulFilePosition += sizeof(kCurrentStreamHeader);

        // Figure out which stream this is:
        uiStreamNumber = kCurrentStreamHeader.m_kStreamType;
        if (uiStreamNumber > STREAM_MAX_NUMBER)
        {
            // Invalid stream, something wrong with the file
            return false;
        }

        // Make sure the buffer is large enough for this stream:
        if (m_akStreamHeader[uiStreamNumber].m_ulLength != 
            kCurrentStreamHeader.m_ulLength)
        {
            NiFree(m_apvStreamData[uiStreamNumber]);

            m_apvStreamData[uiStreamNumber] = 
                NiAlloc(char, kCurrentStreamHeader.m_ulLength);
        }

        // Translate the stream header into the new meaning
        switch (kCurrentStreamHeader.m_kStreamType)
        {
            case STREAM_NORMAL:
            case STREAM_TANGENT:
            case STREAM_MORPH_NORMAL:
            case STREAM_MORPH_TANGENT:
                kCurrentStreamHeader.m_uiObjectSize /= 2;
                break;
            default:
                break;
        }

        // Assign this stream's header to the correct position
        m_akStreamHeader[uiStreamNumber] = 
            kCurrentStreamHeader;

        // Read in the stream's data
        // Currently, element size is assumed to be sizeof(float) for
        // endian swapping purposes.
        EE_VERIFYEQUALS(m_pkFile->BinaryRead(m_apvStreamData[uiStreamNumber],
            kCurrentStreamHeader.m_ulLength, &kCurrentStreamHeader.m_uiObjectSize),
            kCurrentStreamHeader.m_ulLength);

        // We must flip the textures since they were inverted in previous 
        // versions
        if (uiStreamNumber == STREAM_BLEND_MASK ||
            uiStreamNumber == STREAM_LOW_DETAIL_DIFFUSE_TEXTURE ||
            uiStreamNumber == STREAM_LOW_DETAIL_NORMAL_TEXTURE)
        {
            FlipTextureBuffer(uiStreamNumber, kCurrentStreamHeader);
        }

        m_ulFilePosition += kCurrentStreamHeader.m_ulLength;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion3::LoadLegacyBlockHeaderBinary(
    BlockHeader& kBlockHeader, NiBinaryStream& kStream)
{
    NiStreamLoadBinary(kStream, kBlockHeader.m_ulLength);
    NiStreamLoadBinary(kStream, kBlockHeader.m_ulChildOffset);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kPresentData);

    NiStreamLoadBinary(kStream, kBlockHeader.m_fBoundRadius);

    NiStreamLoadBinary(kStream, kBlockHeader.m_kBoundCenter.x);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kBoundCenter.y);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kBoundCenter.z);

    // Skip over the 3 floats representing the sum of all the points.
    float fTemp;
    NiStreamLoadBinary(kStream, fTemp);
    NiStreamLoadBinary(kStream, fTemp);
    NiStreamLoadBinary(kStream, fTemp);

    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeCenter.x);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeCenter.y);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeCenter.z);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection1.x);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection1.y);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection1.z);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection2.x);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection2.y);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection2.z);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection3.x);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection3.y);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeDirection3.z);

    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeExtent1);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeExtent2);
    NiStreamLoadBinary(kStream, kBlockHeader.m_kVolumeExtent3);
}

//--------------------------------------------------------------------------------------------------
void* NiTerrainSectorFileVersion3::GetStreamData(StreamType kStreamID,
    NiUInt32& uiDataLength)
{
    if (!IsReady() || IsWritable()) 
    {
        return 0;
    }

    // Filter out calls to fetch the texture streams
    switch(kStreamID)
    {
        case STREAM_SURFACE_INDEX:
            if (GetBlockID() >= m_uiHighDetailLeafOffset)
            {
                efd::UInt32 uiNumSurfaces = 0;
                for (;uiNumSurfaces < 4; ++uiNumSurfaces)
                {
                    if (m_aiCurrentSurfaceIndexStream[uiNumSurfaces] == -1)
                        break;
                }

                uiDataLength = uiNumSurfaces * sizeof(efd::SInt32);
                return &m_aiCurrentSurfaceIndexStream;
            }
            else
            {
                return NULL;
            }
        case STREAM_BLEND_MASK:
            // Generate the relevant blend mask
            if (GetBlockID() >= m_uiHighDetailLeafOffset &&
                m_pucCurrentBlockBlendMask)
            {
                NiUInt32 uiHalfSectorWidthInVerts = 
                    (m_kFileHeader.m_uiVertsPerBlock << m_uiNumLOD) / 2;
                NiUInt32 uiX = 
                    NiUInt32(m_kCurrentBlockHeader.m_kBoundCenter.x + 
                    uiHalfSectorWidthInVerts) / m_kFileHeader.m_uiVertsPerBlock;
                NiUInt32 uiY = 
                    NiUInt32(m_kCurrentBlockHeader.m_kBoundCenter.y + 
                    uiHalfSectorWidthInVerts) / m_kFileHeader.m_uiVertsPerBlock;
                NiIndex kBlendMaskIndex = NiIndex(uiX,uiY);

                GenerateCellBlendMask(m_pucCurrentBlockBlendMask, 
                    m_uiCellMaskWidth, m_pucReducedOriginalBlendMask, 
                    kBlendMaskIndex, m_uiOriginalMaskWidth);
                uiDataLength = m_uiCellMaskWidth * m_uiCellMaskWidth * 4;
                return m_pucCurrentBlockBlendMask;
            }
            else
            {
                return NULL;
            }
        case STREAM_LOW_DETAIL_DIFFUSE_TEXTURE:
            if (GetBlockID() == 0)
            {
                uiDataLength = sizeof(m_auiDefaultLowDetailTexture);
                return &m_auiDefaultLowDetailTexture;
            }
            else
            {
                return NULL;
            }
        case STREAM_LOW_DETAIL_NORMAL_TEXTURE:
            return NULL;
        default:
            return NiTerrainSectorFileVersion5::GetStreamData(kStreamID, uiDataLength);
    }
}

//--------------------------------------------------------------------------------------------------
NiUInt8* NiTerrainSectorFileVersion3::ResizeImage(
    NiUInt8* pucOriginalPixels, NiUInt32 uiOriginalWidth, NiUInt32 uiNewWidth)
{
    const NiUInt32 uiPixelStride = 4;
    NiUInt32 uiNewImageDataLength = (uiNewWidth * uiNewWidth) * uiPixelStride;
    NiUInt8* pucNewPixels = NiAlloc(NiUInt8, uiNewImageDataLength);

    // Use Bilinear filtering to generate the new image
    for (NiUInt32 uiY = 0; uiY < uiNewWidth; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiNewWidth; ++uiX)
        {
            // Calculate UV point to sample on original image
            float fU = float(uiX) / float(uiNewWidth);
            float fV = float(uiY) / float(uiNewWidth);

            // Calculate the relevant pixels at this point
            fU *= float(uiOriginalWidth);
            fV *= float(uiOriginalWidth);
            int iOrigX = NiUInt32(NiFloor(fU));
            int iOrigY = NiUInt32(NiFloor(fV));
            float fURatio = fU - iOrigX;
            float fVRatio = fV - iOrigY;
            float fUOpposite = 1 - fURatio;
            float fVOpposite = 1 - fVRatio;

            // Sampling Indexes
            NiUInt32 uiTopLeft = 
                (iOrigY) * uiOriginalWidth + (iOrigX);
            NiUInt32 uiTopRight = 
                (iOrigY) * uiOriginalWidth + (iOrigX + 1);
            NiUInt32 uiBottomLeft = 
                (iOrigY + 1) * uiOriginalWidth + (iOrigX);
            NiUInt32 uiBottomRight = 
                (iOrigY + 1) * uiOriginalWidth + (iOrigX + 1);
            NiUInt32 uiNewIndex = uiY * uiNewWidth + uiX;

            // Adjust to account for the pixel stride
            uiTopLeft *= uiPixelStride;
            uiTopRight *= uiPixelStride;
            uiBottomLeft *= uiPixelStride;
            uiBottomRight *= uiPixelStride;
            uiNewIndex *= uiPixelStride;

            // Loop through all the components:
            for (NiUInt32 uiComponent = 0; uiComponent < uiPixelStride; 
                ++uiComponent)
            {
                float fTopLeftValue = 
                    pucOriginalPixels[uiTopLeft + uiComponent];
                float fTopRightValue = 
                    pucOriginalPixels[uiTopRight + uiComponent];
                float fBottomLeftValue = 
                    pucOriginalPixels[uiBottomLeft + uiComponent];
                float fBottomRightValue = 
                    pucOriginalPixels[uiBottomRight + uiComponent];

                float fValue = 
                    (fTopLeftValue * fUOpposite + 
                    fTopRightValue * fURatio) * fVOpposite + 
                    (fBottomLeftValue * fUOpposite + 
                    fBottomRightValue * fURatio) * fVRatio;

                pucNewPixels[uiNewIndex + uiComponent] = NiUInt8(fValue);
            }
        }
    }

    return pucNewPixels;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion3::GenerateCellBlendMask(
    NiUInt8* pucDestination, NiUInt32 uiMaskWidth, 
    NiUInt8* pucSource, NiIndex kLocation, NiUInt32 uiOriginalWidth)
{
    const NiUInt32 uiPixelStride = 4;
    
    // Fetch pixels the appropriate block and return those values.
    NiUInt32 uiOriginalMaskWidth = uiMaskWidth - 2;
    NiInt32 iXOffset = (uiOriginalMaskWidth * kLocation.x) - 1;
    NiInt32 iYOffset = (uiOriginalMaskWidth * kLocation.y) - 1;

    // Iterate over the new values and fetch them from the original
    for (NiUInt32 uiY = 0; uiY < uiMaskWidth; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiMaskWidth; ++uiX)
        {
            NiInt32 iNewIndex = (uiMaskWidth - 1 - uiY) * uiMaskWidth + uiX;
            NiInt32 iOldIndex = (uiY + iYOffset) * uiOriginalWidth + 
                uiX + iXOffset;

            iNewIndex *= uiPixelStride;
            iOldIndex *= uiPixelStride;

            // Loop through all the components:
            for (NiUInt32 uiComponent = 0; uiComponent < uiPixelStride; 
                ++uiComponent)
            {
                NiUInt8 ucValue;
                if ((uiY + iYOffset) >= uiOriginalWidth ||
                    (uiX + iXOffset) >= uiOriginalWidth)
                {
                    ucValue = 0;
                }
                else
                {
                    ucValue = pucSource[iOldIndex + uiComponent];
                }

                pucDestination[iNewIndex + uiComponent] = ucValue;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion3::FlipTextureBuffer(
    NiUInt32 uiStreamNumber, DataStreamHeader kCurrentStreamHeader)
{
    // All textures use the same number of components
    NiUInt32 uiObjectSize = 4;
    // First get the buffer
    char* pucBuffer = (char*) m_apvStreamData[uiStreamNumber];
    char* pucFlippedBuffer = NiAlloc(char, kCurrentStreamHeader.m_ulLength);

    NiUInt32 uiWidth = (NiUInt32)NiSqrt((float)
        (kCurrentStreamHeader.m_ulLength) / (float)(uiObjectSize));

    for (NiUInt32 uiY = 0; uiY < uiWidth; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiWidth; ++uiX)
        {
            NiUInt32 uiNewIndex = ((uiWidth - 1 - uiY) * uiWidth * 
                uiObjectSize) + (uiX * uiObjectSize);
            NiUInt32 uiOldIndex = (uiY * uiWidth * uiObjectSize) + 
                (uiX * uiObjectSize);

            for (NiUInt32 uiC = 0; uiC < uiObjectSize; ++uiC)
            {
                pucFlippedBuffer[uiNewIndex + uiC] = 
                    pucBuffer[uiOldIndex + uiC];
            }
        }
    }

    m_apvStreamData[uiStreamNumber] = pucFlippedBuffer;
    NiFree(pucBuffer);
}

//--------------------------------------------------------------------------------------------------
