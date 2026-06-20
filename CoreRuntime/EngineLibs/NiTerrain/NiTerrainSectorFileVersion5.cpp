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
#include "NiTerrainSectorFileVersion5.h"

#include <efd/File.h>
#include <NiBound.h>

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiITerrainSectorFileVersion5, NiTerrainFileInterface, NiTypeMask::NiITerrainSectorFileVersion5);
//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::DetectFileVersion(FileIdentifier kID)
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
   
    return eVersion == 5;
}

//--------------------------------------------------------------------------------------------------
NiString NiTerrainSectorFileVersion5::GenerateSectorFileName(const char* pcTerrainArchive, 
    NiInt32 iSectorX, NiInt32 iSectorY)
{
    NiString kSectorPath;
    kSectorPath.Format("%s\\Sector_%d_%d", pcTerrainArchive, iSectorX, iSectorY);
    kSectorPath += NiString("\\quadtree.dof");
    return kSectorPath;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)m_kSectorFile);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::GenerateMinMaxElevation()
{
    // Skip straight to the beginning of the highest level of detail
    efd::UInt32 uiNumLOD = m_spPrevVersion->GetNumLOD();
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PushBlock(0);
    }

    // Iterate over the terrain looking for the highest and lowest points
    m_fMinElevation = FLT_MAX;
    m_fMaxElevation = FLT_MIN;
    for (efd::UInt32 uiLeafID = 0; uiLeafID < m_kSectorSpaceMap.size(); ++uiLeafID)
    {
        // Extract the height data from this leaf
        efd::UInt32 uiDataLength = 0;
        float* pfHeights = (float*)m_spPrevVersion->GetStreamData(
            NiITerrainSectorFileVersion5::STREAM_HEIGHT, 
            uiDataLength);
        uiDataLength /= sizeof(float);
        EE_ASSERT(pfHeights);

        // Feed this data into the final data set
        for (efd::UInt32 uiDataIndex = 0; uiDataIndex < uiDataLength; ++uiDataIndex)
        {
            // Sample the height and min/max it
            float fHeight = pfHeights[uiDataIndex];
            m_fMinElevation = efd::Min(m_fMinElevation, fHeight);
            m_fMaxElevation = efd::Max(m_fMaxElevation, fHeight);
        }

        // Move onto the next leaf
        if (uiLeafID + 1 < m_kSectorSpaceMap.size())
            m_spPrevVersion->NextBlock();
    }

    // don't let the min/max fall inside these values 
    // (otherwise it'll be a ridiculously flat terrain)
    m_fMinElevation = efd::Min(m_fMinElevation, -100.0f);
    m_fMaxElevation = efd::Max(m_fMaxElevation, 100.0f);

    // Pop back to the lowest detail
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PopBlock();
    }
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::DetectTextureSizes()
{
    // Calculate low detail texture size:
    // -------------------------------------
    // Read the data from the interface
    NiUInt32 uiDataLength;
    NiUInt8* pucDataStream = (NiUInt8*)m_spPrevVersion->GetStreamData(
        NiITerrainSectorFileVersion5::STREAM_LOW_DETAIL_DIFFUSE_TEXTURE, 
        uiDataLength);

    // Calculate the size of this texture
    m_uiLowDetailTextureSize = NiUInt32(NiSqrt(float(uiDataLength / 4)));
    EE_ASSERT(m_uiLowDetailTextureSize * m_uiLowDetailTextureSize * 4 == uiDataLength);
        
    // Calculate the blend mask texture size:
    // -----------------------------------------
    // Jump to the high detail blocks
    efd::UInt32 uiNumLOD = m_spPrevVersion->GetNumLOD();
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)  
        m_spPrevVersion->PushBlock(0);

    // Read the size of one blend mask:
    uiDataLength = 0;
    pucDataStream = (NiUInt8*)m_spPrevVersion->GetStreamData(
        NiITerrainSectorFileVersion5::STREAM_BLEND_MASK,
        uiDataLength);

    // Calculate the size of this texture
    m_uiBlendMaskSize = NiUInt32(NiSqrt(float(uiDataLength / 4)));
    EE_ASSERT(m_uiBlendMaskSize * m_uiBlendMaskSize * 4 == uiDataLength);

    // Scale this blend mask up
    m_uiBlendMaskSize <<= m_spPrevVersion->GetNumLOD();

    // Pop back to the lowest detail
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
        m_spPrevVersion->PopBlock();
}

//--------------------------------------------------------------------------------------------------
//----------------------------------OLD INTERFACE IMPLEMENTATION------------------------------------
//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainSectorFileVersion5::GetNumLOD() const
{
    return m_kFileHeader.m_uiNumLOD;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::IsReady() const
{
    return NiITerrainSectorFileVersion5::IsReady() && m_pkFile != 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::SetBlockWidthInVerts(NiUInt32 uiVertsPerBlock)
{
    m_kFileHeader.m_uiVertsPerBlock = uiVertsPerBlock;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::SetNumLOD(NiUInt32 uiNumLods)
{
    m_kFileHeader.m_uiNumLOD = uiNumLods;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::WriteFileHeader()
{
    if (!IsWritable() || !IsReady())
        return;

    // Write the file header to the file:
    m_kFileHeader.SaveBinary(*m_pkFile);
    m_ulFilePosition += sizeof(m_kFileHeader);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::WriteBlock()
{
    if (!IsWritable() || !IsReady())
        return;

    // Update the identity information on this block!
    m_iCurrentBlockID++;
    if (m_iCurrentBlockID >= (4*m_iCurrentBlockLevelIndex + 1))
    {
        m_iCurrentBlockLevelIndex = m_iCurrentBlockID;
        m_iCurrentBlockLevel++;
    }

    // Update this block's parent's header to point to this set of children
    if ((m_iCurrentBlockID % 4) - 1 == 0)
    {
        NiUInt32 uiParentPosition = m_kPositionStack.RemoveHead();
        NiInt32 iChildOffsetOffset = (NiInt32)(
            (char*)&(m_kCurrentBlockHeader.m_ulChildOffset) -
            (char*)&(m_kCurrentBlockHeader));

        // Seek to the parent and write the new child's address:
        m_pkFile->Seek(uiParentPosition, efd::File::ms_iSeekSet);
        m_pkFile->Seek(iChildOffsetOffset, efd::File::ms_iSeekCur);

        // Write the new child's address:
        NiStreamSaveBinary(*m_pkFile, m_ulFilePosition);

        // Seek back to the current block's position:
        m_pkFile->Seek(m_ulFilePosition, efd::File::ms_iSeekSet);
    }

    // Add the position of this block to the list so it's
    // children offset can be set later:
    m_kPositionStack.AddTail(m_ulFilePosition);

    // Finalize the information in this block's header:
    m_kCurrentBlockHeader.m_ulLength = sizeof(m_kCurrentBlockHeader);
    m_kCurrentBlockHeader.m_kPresentData.m_uiBitField = 0;
    NiUInt32 kStreamID;
    for (kStreamID = 0; kStreamID < STREAM_MAX_NUMBER; ++kStreamID)
    {
        m_kCurrentBlockHeader.m_ulLength +=
            m_akStreamHeader[kStreamID].m_ulLength;

        // Check if this stream is present:
        if (m_akStreamHeader[kStreamID].m_ulLength != 0)
        {
            m_kCurrentBlockHeader.m_kPresentData.m_uiBitField |=
                (1 << kStreamID);
            m_kCurrentBlockHeader.m_ulLength += sizeof(DataStreamHeader);
        }
    }

    // Write the block to the file:
    // First the header:
    m_kCurrentBlockHeader.SaveBinary(*m_pkFile);
    m_ulFilePosition += sizeof(m_kCurrentBlockHeader);

    // Now write all the streams:
    for (kStreamID = 0; kStreamID < STREAM_MAX_NUMBER; ++kStreamID)
    {
        if (m_akStreamHeader[kStreamID].m_ulLength == 0)
        {
            continue;
        }

        // Write this stream to the file:
        m_akStreamHeader[kStreamID].SaveBinary(*m_pkFile);
        m_ulFilePosition += sizeof(DataStreamHeader);

        // Write the data to the file:
        // Currently, element size is assumed to be sizeof(float) for
        // endian swapping purposes.
        EE_VERIFYEQUALS(m_pkFile->BinaryWrite(m_apvStreamData[kStreamID], 
            m_akStreamHeader[kStreamID].m_ulLength, &m_akStreamHeader[kStreamID].m_uiObjectSize),
            m_akStreamHeader[kStreamID].m_ulLength);

        m_ulFilePosition += m_akStreamHeader[kStreamID].m_ulLength;

        // Reset this stream's info as it has now been written:
        m_akStreamHeader[kStreamID].m_ulLength = 0;
        m_apvStreamData[kStreamID] = 0;
    }

    // Reset this stream's information:
    m_kCurrentBlockHeader.m_ulChildOffset = 0;
    m_kCurrentBlockHeader.m_kPresentData.m_uiBitField = 0;
    m_kCurrentBlockHeader.m_ulLength = 0;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainSectorFileVersion5::GetBlockWidthInVerts() const
{
    if (!IsReady())
    {
        return 0;
    }

    return m_kFileHeader.m_uiVertsPerBlock;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::NextBlock()
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
    for (uiStreamNumber = 0;
        uiStreamNumber < STREAM_MAX_NUMBER;
        ++uiStreamNumber)
    {
        m_akStreamHeader[uiStreamNumber].m_kStreamType = STREAM_INVALID;
    }

    // Update the identity information on this block!
    m_iCurrentBlockID++;
    if (m_iCurrentBlockID >= (4*m_iCurrentBlockLevelIndex + 1))
    {
        m_iCurrentBlockLevelIndex = m_iCurrentBlockID;
        m_iCurrentBlockLevel++;
    }

    // Read in the block header:
    // No need to seek, we are already at the next block!
    m_kCurrentBlockHeader.LoadBinary(*m_pkFile);
    m_ulFilePosition += sizeof(m_kCurrentBlockHeader);

    // Read in all the available streams:
    for (uiStreamNumber = 0;
        uiStreamNumber < STREAM_MAX_NUMBER;
        ++uiStreamNumber)
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

        // Assign this stream's header to the correct position
        m_akStreamHeader[uiStreamNumber] =
            kCurrentStreamHeader;

        // Read in the stream's data
        // Currently, element size is assumed to be sizeof(float) for
        // endian swapping purposes.
        EE_VERIFYEQUALS(m_pkFile->BinaryRead(m_apvStreamData[uiStreamNumber],
            kCurrentStreamHeader.m_ulLength, &kCurrentStreamHeader.m_uiObjectSize),
            kCurrentStreamHeader.m_ulLength);

        m_ulFilePosition += kCurrentStreamHeader.m_ulLength;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::PushBlock(int iChildID)
{
    if (!IsReady() || IsWritable())
    {
        return false;
    }

    // Check the bounds of the parameter:
    if (iChildID < 0 || iChildID > 3)
    {
        return false;
    }

    // Figure out where the children start
    NiUInt32 ulSeekOffset = m_kCurrentBlockHeader.m_ulChildOffset;
    if (ulSeekOffset == 0)
    {
        // No children for this block!
        return false;
    }

    // Update the identity information on this block!
    m_iCurrentBlockLevelIndex = 4 * m_iCurrentBlockLevelIndex + 1;
    m_iCurrentBlockLevel++;
    m_iCurrentBlockID = 4*m_iCurrentBlockID; // (add 1 in NextBlock)

    // Seek to the first child block:
    m_pkFile->Seek(ulSeekOffset, efd::File::ms_iSeekSet);
    m_ulFilePosition = ulSeekOffset;

    // Seek to the requested child:
    for (NiUInt32 uiChild = 0; uiChild < (NiUInt32)iChildID; ++uiChild)
    {
        // read this child's header in (to see how far we need to skip)
        m_kCurrentBlockHeader.LoadBinary(*m_pkFile);
        m_ulFilePosition += sizeof(m_kCurrentBlockHeader);

        // Seek past this block to get to its sibling:
        ulSeekOffset =
            m_kCurrentBlockHeader.m_ulLength - sizeof(m_kCurrentBlockHeader);
        m_pkFile->Seek(ulSeekOffset, efd::File::ms_iSeekCur);
        m_ulFilePosition += ulSeekOffset;

        m_iCurrentBlockID++;
    }

    // Put this block's position on the stack:
    m_kPositionStack.AddTail(m_ulFilePosition);

    // Read in this block's details:
    bool bResult = NextBlock();
    if (!bResult)
    {
        PopBlock();
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::PopBlock()
{
    // Check that we haven't reached the root of the tree:
    if (m_kPositionStack.GetSize() == 1)
    {
        return false;
    }

    // Update the identity information on this block!
    m_iCurrentBlockLevelIndex = (m_iCurrentBlockLevelIndex - 1) / 4;
    m_iCurrentBlockLevel--;
    m_iCurrentBlockID = (m_iCurrentBlockID - 1) / 4 - 1;

    // The lowest item on the stack is where we currently are.
    // Pop that off and seek to where the previous item specifies:
    m_kPositionStack.RemoveTail();

    // Seek to the previous item: (parent)
    NiUInt32 ulSeekPosition = m_kPositionStack.GetTail();
    m_pkFile->Seek(ulSeekPosition, efd::File::ms_iSeekSet);
    m_ulFilePosition = ulSeekPosition;

    // Read in the parent block's data:
    return NextBlock();
}

//--------------------------------------------------------------------------------------------------
NiBound NiTerrainSectorFileVersion5::GetBlockBound() const
{
    NiBound result;
    result.SetCenter(m_kCurrentBlockHeader.m_kBoundCenter);
    result.SetRadius(m_kCurrentBlockHeader.m_fBoundRadius);

    return result;
}
//---------------------------------------------------------------------------
NiBoxBV NiTerrainSectorFileVersion5::GetBlockBoundVolume() const
{
    NiBoxBV result;
    result.SetCenter(m_kCurrentBlockHeader.m_kVolumeCenter);
    result.SetAxis(0, m_kCurrentBlockHeader.m_kVolumeDirection1);
    result.SetAxis(1, m_kCurrentBlockHeader.m_kVolumeDirection2);
    result.SetAxis(2, m_kCurrentBlockHeader.m_kVolumeDirection3);
    result.SetExtent(0, m_kCurrentBlockHeader.m_kVolumeExtent1);
    result.SetExtent(1, m_kCurrentBlockHeader.m_kVolumeExtent2);
    result.SetExtent(2, m_kCurrentBlockHeader.m_kVolumeExtent3);

    return result;
}

//---------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::SetBlockBounds(NiBound kBound, NiBoxBV kVolume)
{
    // Bound Information:
    m_kCurrentBlockHeader.m_kBoundCenter = kBound.GetCenter();
    m_kCurrentBlockHeader.m_fBoundRadius = kBound.GetRadius();

    // Bounding Volume Information:
    m_kCurrentBlockHeader.m_kVolumeCenter = kVolume.GetCenter();
    m_kCurrentBlockHeader.m_kVolumeDirection1 = kVolume.GetAxis(0);
    m_kCurrentBlockHeader.m_kVolumeDirection2 = kVolume.GetAxis(1);
    m_kCurrentBlockHeader.m_kVolumeDirection3 = kVolume.GetAxis(2);
    m_kCurrentBlockHeader.m_kVolumeExtent1 = kVolume.GetExtent(0);
    m_kCurrentBlockHeader.m_kVolumeExtent2 = kVolume.GetExtent(1);
    m_kCurrentBlockHeader.m_kVolumeExtent3 = kVolume.GetExtent(2);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::SetStreamData(StreamType kStreamID,
    NiUInt32 uiObjectSize, void* pvData, NiUInt32 uiDataLength)
{
    EE_ASSERT(uiObjectSize);
    EE_ASSERT(uiDataLength == 0 || pvData != 0);

    if (!IsReady() || !IsWritable())
    {
        return;
    }

    m_akStreamHeader[kStreamID].m_kStreamType = kStreamID;
    m_akStreamHeader[kStreamID].m_ulLength = uiDataLength;
    m_akStreamHeader[kStreamID].m_uiObjectSize = uiObjectSize;

    m_apvStreamData[kStreamID] = (char*)pvData;
}

//--------------------------------------------------------------------------------------------------
void* NiTerrainSectorFileVersion5::GetStreamData(StreamType kStreamID,
    NiUInt32& uiDataLength)
{
    if (!IsReady() || IsWritable())
    {
        return 0;
    }

    if (m_akStreamHeader[kStreamID].m_kStreamType == kStreamID)
    {
        uiDataLength = m_akStreamHeader[kStreamID].m_ulLength;
        return m_apvStreamData[kStreamID];
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion5::NiTerrainSectorFileVersion5()
    : m_pkFile(0)
    , m_ulFilePosition(0)
    , m_kSectorFile(0)
    , m_iCurrentBlockID(0)
    , m_iCurrentBlockLevel(0)
{
    // Initialize all the stream pointers to 0
    NiUInt32 uiStreamID;
    for (uiStreamID = 0; uiStreamID < STREAM_MAX_NUMBER; ++uiStreamID)
    {
        m_apvStreamData[uiStreamID] = 0;
        m_akStreamHeader[uiStreamID].m_ulLength = 0;
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion5::~NiTerrainSectorFileVersion5()
{
    NiDelete m_pkFile;

    // Release the stream buffers:
    if (!IsWritable())
    {
        NiUInt32 uiStreamID;
        for (uiStreamID = 0; uiStreamID < STREAM_MAX_NUMBER; ++uiStreamID)
        {
            if (m_apvStreamData[uiStreamID])
            {
                NiFree(m_apvStreamData[uiStreamID]);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion5::OpenErrorCode NiTerrainSectorFileVersion5::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);

    // Allocate a temporary buffer for "Standardizing" the file path.
    const char* pcSectorFile = kID.m_kSectorFile.c_str();
    size_t stLen = strlen(pcSectorFile);
    char* pcNonStandardSectorFile = NiStackAlloc(char, stLen+1);

    // Standardize the file path.
    NiMemcpy(pcNonStandardSectorFile, pcSectorFile, stLen+1);
    NiPath::Standardize(pcNonStandardSectorFile);

    m_kSectorFile = pcNonStandardSectorFile;

    // Free the temporary buffer.
    NiStackFree(pcNonStandardSectorFile);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion5::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainSectorFileVersion5::Initialize();
    if (!bResult)
        return false;

    // Attempt to gain access to the file in this mode!
    m_pkFile = efd::File::GetFile(m_kSectorFile, m_eAccessMode);
    if (!m_pkFile)
        return false;

    bool bPlatformLittle = NiSystemDesc::GetSystemDesc().IsLittleEndian();
    m_pkFile->SetEndianSwap(!bPlatformLittle);

    // Return null if it was not possible
    if (!m_pkFile)
    {
        return false;
    }

    // Initialize the variables:
    m_ulFilePosition = 0;
    m_iCurrentBlockID = -1;
    m_iCurrentBlockLevel = 0;
    m_iCurrentBlockLevelIndex = 0;
    m_kPositionStack.RemoveAll();

    // If we are reading from the file, then begin setting up:
    if (!IsWritable())
    {
        // Read the file header
        m_kFileHeader.LoadBinary(*m_pkFile);
        m_ulFilePosition += sizeof(m_kFileHeader);

        // Read the first block's data:
        return NextBlock();
    }
    else
    {
        m_kFileHeader.m_kVersion = ms_InterfaceVersion;
        m_kFileHeader.m_uiVertsPerBlock = 32; // Default
        m_kFileHeader.m_uiNumLOD = 5; // Default
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::FileHeader::LoadBinary(efd::BinaryStream& kStream)
{
    NiStreamLoadBinary(kStream, m_kVersion);
    NiStreamLoadBinary(kStream, m_uiVertsPerBlock);
    NiStreamLoadBinary(kStream, m_uiNumLOD);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::FileHeader::SaveBinary(efd::BinaryStream& kStream)
{
    NiStreamSaveBinary(kStream, m_kVersion);
    NiStreamSaveBinary(kStream, m_uiVertsPerBlock);
    NiStreamSaveBinary(kStream, m_uiNumLOD);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::BlockHeader::LoadBinary(efd::BinaryStream& kStream)
{
    NiStreamLoadBinary(kStream, m_ulLength);
    NiStreamLoadBinary(kStream, m_ulChildOffset);
    NiStreamLoadBinary(kStream, m_kPresentData);

    NiStreamLoadBinary(kStream, m_fBoundRadius);

    NiStreamLoadBinary(kStream, m_kBoundCenter.x);
    NiStreamLoadBinary(kStream, m_kBoundCenter.y);
    NiStreamLoadBinary(kStream, m_kBoundCenter.z);

    NiStreamLoadBinary(kStream, m_kVolumeCenter.x);
    NiStreamLoadBinary(kStream, m_kVolumeCenter.y);
    NiStreamLoadBinary(kStream, m_kVolumeCenter.z);
    NiStreamLoadBinary(kStream, m_kVolumeDirection1.x);
    NiStreamLoadBinary(kStream, m_kVolumeDirection1.y);
    NiStreamLoadBinary(kStream, m_kVolumeDirection1.z);
    NiStreamLoadBinary(kStream, m_kVolumeDirection2.x);
    NiStreamLoadBinary(kStream, m_kVolumeDirection2.y);
    NiStreamLoadBinary(kStream, m_kVolumeDirection2.z);
    NiStreamLoadBinary(kStream, m_kVolumeDirection3.x);
    NiStreamLoadBinary(kStream, m_kVolumeDirection3.y);
    NiStreamLoadBinary(kStream, m_kVolumeDirection3.z);

    NiStreamLoadBinary(kStream, m_kVolumeExtent1);
    NiStreamLoadBinary(kStream, m_kVolumeExtent2);
    NiStreamLoadBinary(kStream, m_kVolumeExtent3);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::BlockHeader::SaveBinary(efd::BinaryStream& kStream)
{
    NiStreamSaveBinary(kStream, m_ulLength);
    NiStreamSaveBinary(kStream, m_ulChildOffset);
    NiStreamSaveBinary(kStream, m_kPresentData);

    NiStreamSaveBinary(kStream, m_fBoundRadius);

    NiStreamSaveBinary(kStream, m_kBoundCenter.x);
    NiStreamSaveBinary(kStream, m_kBoundCenter.y);
    NiStreamSaveBinary(kStream, m_kBoundCenter.z);

    NiStreamSaveBinary(kStream, m_kVolumeCenter.x);
    NiStreamSaveBinary(kStream, m_kVolumeCenter.y);
    NiStreamSaveBinary(kStream, m_kVolumeCenter.z);
    NiStreamSaveBinary(kStream, m_kVolumeDirection1.x);
    NiStreamSaveBinary(kStream, m_kVolumeDirection1.y);
    NiStreamSaveBinary(kStream, m_kVolumeDirection1.z);
    NiStreamSaveBinary(kStream, m_kVolumeDirection2.x);
    NiStreamSaveBinary(kStream, m_kVolumeDirection2.y);
    NiStreamSaveBinary(kStream, m_kVolumeDirection2.z);
    NiStreamSaveBinary(kStream, m_kVolumeDirection3.x);
    NiStreamSaveBinary(kStream, m_kVolumeDirection3.y);
    NiStreamSaveBinary(kStream, m_kVolumeDirection3.z);

    NiStreamSaveBinary(kStream, m_kVolumeExtent1);
    NiStreamSaveBinary(kStream, m_kVolumeExtent2);
    NiStreamSaveBinary(kStream, m_kVolumeExtent3);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::DataStreamHeader::LoadBinary(efd::BinaryStream& kStream)
{
    NiStreamLoadBinary(kStream, m_ulLength);
    NiStreamLoadBinary(kStream, m_kStreamType);
    NiStreamLoadBinary(kStream, m_uiObjectSize);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion5::DataStreamHeader::SaveBinary(efd::BinaryStream& kStream)
{
    NiStreamSaveBinary(kStream, m_ulLength);
    NiStreamSaveBinary(kStream, m_kStreamType);
    NiStreamSaveBinary(kStream, m_uiObjectSize);
}

//--------------------------------------------------------------------------------------------------
NiITerrainSectorFileVersion5::NiITerrainSectorFileVersion5()
    : NiTerrainFileInterface(ms_InterfaceVersion)
{
}

//--------------------------------------------------------------------------------------------------
NiFileInterface* NiITerrainSectorFileVersion5::AdaptToNextVersion()
{
    // Previous versions also implement V5 interface, so they just need to report higher
    if (GetInterfaceVersion() < ms_InterfaceVersion)
    {
        m_kInterfaceVersion++;
        return this;
    }
    else
    {
        return EE_NEW NiITerrainSectorFileVersion5To6Adapter(this);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::Precache(efd::UInt32 uiBeginLevel,
    efd::UInt32 uiEndLevel, efd::UInt32 uiData)
{
    // Don't include anything that we can't actually load in this format
    efd::UInt32 uiExcludedData = 
        DataField::BOUNDS | DataField::LOWDETAIL_NORMALS | DataField::NORMALS | DataField::TANGENTS;
    uiData = uiData & ~uiExcludedData;

    m_eCachedData = uiData;
    EE_UNUSED_ARG(uiBeginLevel);
    EE_UNUSED_ARG(uiEndLevel);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::GenerateSectorSpaceMap(
    efd::map<efd::UInt32, NiIndex>& kSectorSpaceMap, efd::UInt32 uiLeafID,
    efd::UInt32 uiSubDivisions, NiIndex kOffset)
{
    if (uiSubDivisions == 0)
    {
        // Make sure an entry for this leaf doesn't already exist
        EE_ASSERT(kSectorSpaceMap.find(uiLeafID) == kSectorSpaceMap.end());
        // Make sure an entry for this offset doesn't already exist
        efd::map<efd::UInt32, NiIndex>::iterator kIter;
        for (kIter = kSectorSpaceMap.begin(); kIter != kSectorSpaceMap.end(); ++kIter)
        {
            EE_ASSERT(kIter->second != kOffset);
        }

        // Map the leaf to the appropriate space
        kSectorSpaceMap[uiLeafID] = kOffset;
    }
    else
    {
        // Child offset map
        NiIndex akChildOffsets[4] = {
            NiIndex(0,0), 
            NiIndex(1,0), 
            NiIndex(1,1), 
            NiIndex(0,1)
        };

        // Recurse into this region to calculate the leafs
        for (efd::UInt32 uiChild = 0; uiChild < 4; ++uiChild)
        {
            GenerateSectorSpaceMap(kSectorSpaceMap, uiLeafID * 4 + uiChild + 1, uiSubDivisions - 1,
                kOffset + (akChildOffsets[uiChild] * (1 << (uiSubDivisions - 1))));
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadSectorConfig(
    efd::UInt32& uiSectorWidthInVerts, efd::UInt32& uiNumLOD)
{
    uiNumLOD = m_spPrevVersion->GetNumLOD();
    uiSectorWidthInVerts = ((m_spPrevVersion->GetBlockWidthInVerts() - 1) << uiNumLOD) + 1;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadHeights(efd::UInt16* pusHeights, 
    efd::UInt32 uiDataLength)
{
    // Initialize the buffer
    memset(pusHeights, 0, uiDataLength * sizeof(efd::UInt16));
    // Figure out the width of the sector in verts
    efd::UInt32 uiBlockWidthInVerts = m_spPrevVersion->GetBlockWidthInVerts();
    efd::UInt32 uiBlockWidth = uiBlockWidthInVerts - 1;
    efd::UInt32 uiSectorWidthInVerts = ((uiBlockWidth) << m_spPrevVersion->GetNumLOD()) + 1;
    efd::UInt32 uiNumLOD = m_spPrevVersion->GetNumLOD();

    // Skip straight to the beginning of the highest level of detail
    efd::UInt32 uiNumNodes = 0;
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PushBlock(0);

        // Calculate the number of nodes before the leafs
        uiNumNodes += (1 << uiLevel) * (1 << uiLevel);
    }

    // Extract the actual values for backwards compatible heights
    for (efd::UInt32 uiLeafID = 0; uiLeafID < m_kSectorSpaceMap.size(); ++uiLeafID)
    {
        // Extract the height data from this leaf
        efd::UInt32 uiHeightDataLength = 0;
        float* pfHeights = (float*)m_spPrevVersion->GetStreamData(
            NiITerrainSectorFileVersion5::STREAM_HEIGHT, 
            uiHeightDataLength);
        uiHeightDataLength /= sizeof(float);
        EE_ASSERT(pfHeights);

        // Work out the position of this leaf in sector space
        efd::UInt32 uiLeafRegionID = uiLeafID + uiNumNodes;
        EE_ASSERT(m_kSectorSpaceMap.find(uiLeafRegionID) != m_kSectorSpaceMap.end());
        NiIndex kLeafOffset = m_kSectorSpaceMap[uiLeafRegionID] * uiBlockWidth;

        // Feed this data into the final data set
        for (efd::UInt32 uiDataIndex = 0; uiDataIndex < uiHeightDataLength; ++uiDataIndex)
        {
            // Sample the height and convert it to a 16bit 
            float fHeight = pfHeights[uiDataIndex];
            efd::UInt16 usSample = efd::UInt16(
                ((fHeight - m_fMinElevation) / (m_fMaxElevation - m_fMinElevation)) * USHRT_MAX);

            // Work out the index of this leaf height, in sector space
            NiIndex kLeafIndex(
                uiDataIndex % uiBlockWidthInVerts, uiDataIndex / uiBlockWidthInVerts);
            NiIndex kSectorIndex = kLeafIndex + kLeafOffset;

            // Apply this height to the sector space map
            efd::UInt32 uiSectorSpaceIndex = kSectorIndex.y * uiSectorWidthInVerts + kSectorIndex.x;
            EE_ASSERT(uiSectorSpaceIndex < uiDataLength);
            pusHeights[uiSectorSpaceIndex] = usSample;
        }

        // Move onto the next leaf
        if (uiLeafID + 1 < m_kSectorSpaceMap.size())
            m_spPrevVersion->NextBlock();
    }

    // Pop back to the lowest detail
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PopBlock();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadBlendMask(NiPixelData*& pkBlendMask)
{
    // Create a blend mask large enough to cover the terrain
    efd::UInt32 uiSectorMaskWidth = m_uiBlendMaskSize;
    efd::UInt32 uiCellMaskWidth = uiSectorMaskWidth >> m_spPrevVersion->GetNumLOD();
    efd::UInt32 uiNumCellsPerAxis = 1 << m_spPrevVersion->GetNumLOD();
    efd::UInt32 uiNumLOD = m_spPrevVersion->GetNumLOD();

    // Allocate the blend mask based on this size
    pkBlendMask = EE_NEW NiPixelData(uiSectorMaskWidth, uiSectorMaskWidth, NiPixelFormat::RGBA32);
    efd::UInt8* pucSectorMask = pkBlendMask->GetPixels();

    // Skip straight to the beginning of the highest level of detail
    efd::UInt32 uiNumNodes = 0;
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PushBlock(0);

        // Calculate the number of nodes before the leafs
        uiNumNodes += (1 << uiLevel) * (1 << uiLevel);
    }

    // Extract blend mask for backwards compatible blend masks
    for (efd::UInt32 uiLeafID = 0; uiLeafID < m_kSectorSpaceMap.size(); ++uiLeafID)
    {
        // Extract the mask data from this leaf
        efd::UInt32 uiMaskDataLength = 0;
        efd::UInt8* pucMask = (NiUInt8*)m_spPrevVersion->GetStreamData(
            NiITerrainSectorFileVersion5::STREAM_BLEND_MASK, 
            uiMaskDataLength);
        uiMaskDataLength /= (sizeof(efd::UInt8) * 4); // 4 channels
        EE_ASSERT(pucMask);

        // Work out the position of this leaf in sector space
        efd::UInt32 uiLeafRegionID = uiLeafID + uiNumNodes;
        EE_ASSERT(m_kSectorSpaceMap.find(uiLeafRegionID) != m_kSectorSpaceMap.end());
        NiIndex kLeafOffset = m_kSectorSpaceMap[uiLeafRegionID];
        // Flip the y offset
        kLeafOffset.y = uiNumCellsPerAxis - kLeafOffset.y - 1; 
        kLeafOffset *= uiCellMaskWidth;

        // Feed this data into the final data set
        for (efd::UInt32 uiDataIndex = 0; uiDataIndex < uiMaskDataLength; ++uiDataIndex)
        {
            // Work out the index of this leaf mask value, in sector space
            NiIndex kLeafIndex(uiDataIndex % uiCellMaskWidth, uiDataIndex / uiCellMaskWidth);
            NiIndex kSectorIndex = kLeafIndex + kLeafOffset;
            efd::UInt32 uiSectorSpaceIndex = kSectorIndex.y * uiSectorMaskWidth + kSectorIndex.x;
            uiSectorSpaceIndex *= 4;

            // Do this for all 4 channels
            efd::UInt32 uiMaskSum = 0;
            for (efd::UInt32 uiChannel = 0; uiChannel < 4; ++uiChannel)
            {
                // Sample the mask value
                efd::UInt8 ucSample = pucMask[uiDataIndex * 4 + uiChannel];

                // Enforce the maximum sum of 255 for the mask channels
                uiMaskSum += ucSample;
                if (uiMaskSum > 255)
                {
                    efd::SInt32 uiOverflow = uiMaskSum - 255;
                    ucSample -= efd::Int32ToUInt8(uiOverflow);
                    uiMaskSum -= uiOverflow;
                }

                // Apply this value to the sector space mask
                EE_ASSERT(uiSectorSpaceIndex + uiChannel < pkBlendMask->GetSizeInBytes());
                pucSectorMask[uiSectorSpaceIndex + uiChannel] = ucSample;
            }

            EE_ASSERT(uiMaskSum < 256);
        }

        // Move onto the next leaf
        if (uiLeafID + 1 < m_kSectorSpaceMap.size())
            m_spPrevVersion->NextBlock();
    }

    // Pop back to the lowest detail
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PopBlock();
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadLowDetailDiffuseMap(
    NiPixelData*& pkLowDetailDiffuse)
{
    if (GetFileVersion() < ms_InterfaceVersion)
    {
        // Returning false here will force low detail textures to be re-rendered when the scene 
        // loads in the tools as all older versions didn't have valid low detail maps anyway
        return false;
    }

    // Read the data from the interface
    NiUInt32 uiDataLength;
    NiUInt8* pucDataStream = (NiUInt8*)m_spPrevVersion->GetStreamData(
        NiITerrainSectorFileVersion5::STREAM_LOW_DETAIL_DIFFUSE_TEXTURE, 
        uiDataLength);
    if (!pucDataStream || uiDataLength == 0)
        return false;

    // Calculate the size of this texture
    NiUInt32 uiTexSize = m_uiLowDetailTextureSize;
    if (uiTexSize * uiTexSize * 4 != uiDataLength)
        return false;

    // Generate an appropriate pixel data object to create the texture from
    NiPixelData* pkPixelData = EE_NEW NiPixelData(uiTexSize, uiTexSize, NiPixelFormat::RGBA32);
    NiUInt8* pucPixelData = pkPixelData->GetPixels();
    for (NiUInt32 uiIndex = 0; uiIndex < uiDataLength; ++uiIndex)
    {
        // Make sure the alpha channel = 0 (it now stores specularity information)
        if (uiIndex % 4 == 3)
        {
            pucPixelData[uiIndex] = 0;
        }
        else
        {
            pucPixelData[uiIndex] = pucDataStream[uiIndex];
        }
    }

    pkLowDetailDiffuse = pkPixelData;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadCellSurfaceIndex(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, LeafData* pkLeafData)
{
    EE_ASSERT(pkLeafData);

    // Skip straight to the beginning of the highest level of detail
    efd::UInt32 uiNumLOD = m_spPrevVersion->GetNumLOD();
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PushBlock(0);
    }

    // Loop through each cell and populate it's buffer entry
    for (efd::UInt32 uiIndex = 0; uiIndex < uiCellRegionID + uiNumCells; ++uiIndex)
    {
        if (uiIndex >= uiCellRegionID)
        {
            LeafData& kLeafData = pkLeafData[uiIndex - uiCellRegionID];

            NiUInt32 uiDataLength;
            NiUInt32* uiSurfaceIndex = (NiUInt32*)m_spPrevVersion->GetStreamData(
                NiITerrainSectorFileVersion5::STREAM_SURFACE_INDEX, 
                uiDataLength);
            if (uiSurfaceIndex)
            {
                // Copy the data
                kLeafData.m_uiNumSurfaces = uiDataLength / sizeof(NiUInt32);
                for (efd::UInt32 uiSlot = 0; uiSlot < kLeafData.m_uiNumSurfaces; ++uiSlot)
                    kLeafData.m_auiSurfaceIndex[uiSlot] = uiSurfaceIndex[uiSlot];
            }
            else
            {
                kLeafData.m_uiNumSurfaces = 0;
            }

            // Zero out the rest of the entries
            for (efd::UInt32 uiSlot = kLeafData.m_uiNumSurfaces; 
                uiSlot < NiTerrainCellLeaf::MAX_NUM_SURFACES; ++uiSlot)
            {
                kLeafData.m_auiSurfaceIndex[uiSlot] = 0;
            }
        }

        if (uiIndex + 1 < uiCellRegionID + uiNumCells)
        {
            // Move onto the next block
            m_spPrevVersion->NextBlock();
        }
    }

    // Pop back to the lowest detail
    for (efd::UInt32 uiLevel = 0; uiLevel < uiNumLOD; ++uiLevel)
    {       
        m_spPrevVersion->PopBlock();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiITerrainSectorFileVersion5To6Adapter::GetNumLOD()
{
    return m_spPrevVersion->GetNumLOD();
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiITerrainSectorFileVersion5To6Adapter::GetBlockWidthInVerts()
{
    return m_spPrevVersion->GetBlockWidthInVerts();
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiITerrainSectorFileVersion5To6Adapter::GetBlendMaskSize()
{
    return m_uiBlendMaskSize;
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiITerrainSectorFileVersion5To6Adapter::GetLowDetailTextureSize()
{
    return m_uiLowDetailTextureSize;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::GetMinMaxElevation(float& fMin, float& fMax)
{
    fMin = m_fMinElevation;
    fMax = m_fMaxElevation;
}

//--------------------------------------------------------------------------------------------------
NiITerrainSectorFileVersion5To6Adapter::NiITerrainSectorFileVersion5To6Adapter(
    NiITerrainSectorFileVersion5* pBaseVersion)
    : NiITerrainSectorFileVersion6()
    , m_spPrevVersion(pBaseVersion)
{
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::OpenErrorCode NiITerrainSectorFileVersion5To6Adapter::Open(
    FileIdentifier kIdentifier, efd::File::OpenMode eAccessMode)
{
    NiITerrainSectorFileVersion5::FileIdentifier kID;

    NiString kSectorPath;
    kSectorPath.Format("%s\\Sector_%d_%d", 
        kIdentifier.m_kArchivePath.c_str(),
        kIdentifier.m_iSectorX, 
        kIdentifier.m_iSectorY);
    kSectorPath += NiString("\\quadtree.dof");

    kID.m_kSectorFile = (const char*)kSectorPath;
    kID.m_pkStoragePolicy = kIdentifier.m_pkStoragePolicy;

    OpenErrorCode result = m_spPrevVersion->Open(kID, eAccessMode);

    if (result == SUCCESS)
    {
        // Calculate a map to direct where particular leafs translate to the sector space
        GenerateSectorSpaceMap(m_kSectorSpaceMap, 0, m_spPrevVersion->GetNumLOD(), NiIndex(0, 0));

        // Calculate the highest/lowest elevations on the terrain
        GenerateMinMaxElevation();

        // Detect the texture sizes:
        DetectTextureSizes();
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::Close()
{
    return m_spPrevVersion->Close();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainSectorFileVersion5To6Adapter::GetFileVersion() const
{
    return m_spPrevVersion->GetFileVersion();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainSectorFileVersion5To6Adapter::GetInterfaceVersion() const
{
    return ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::IsReady() const
{
    return m_spPrevVersion->IsReady();
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::IsWritable() const
{
    return m_spPrevVersion->IsWritable();
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    return m_spPrevVersion->GetFilePaths(kFilePaths);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::IsDataReady(DataField::Value eDataField)
{
    return (m_eCachedData & eDataField) != 0;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteSectorConfig(efd::UInt32 uiSectorWidthInVerts, 
    efd::UInt32 uiNumLOD)
{
    EE_UNUSED_ARG(uiSectorWidthInVerts);
    EE_UNUSED_ARG(uiNumLOD);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteHeights(efd::UInt16* pusHeights,
    efd::UInt32 uiDataLength)
{
    EE_UNUSED_ARG(pusHeights);
    EE_UNUSED_ARG(uiDataLength);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadNormals(efd::Point2* pkNormals, 
    efd::UInt32 uiDataLength)
{
    EE_UNUSED_ARG(pkNormals);
    EE_UNUSED_ARG(uiDataLength);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteNormals(efd::Point2* pkNormals, 
    efd::UInt32 uiDataLength)
{
    EE_UNUSED_ARG(pkNormals);
    EE_UNUSED_ARG(uiDataLength);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadTangents(efd::Point2* pkTangents, 
    efd::UInt32 uiDataLength)
{
    EE_UNUSED_ARG(pkTangents);
    EE_UNUSED_ARG(uiDataLength);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteTangents(efd::Point2* pkTangents, 
    efd::UInt32 uiDataLength)
{
    EE_UNUSED_ARG(pkTangents);
    EE_UNUSED_ARG(uiDataLength);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteBlendMask(NiPixelData* pkBlendMask)
{
    EE_UNUSED_ARG(pkBlendMask);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteLowDetailDiffuseMap(
    NiPixelData* pkLowDetailDiffuse)
{
    EE_UNUSED_ARG(pkLowDetailDiffuse);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadLowDetailNormalMap(NiPixelData*& pkLowDetailNormal)
{
    EE_UNUSED_ARG(pkLowDetailNormal);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteLowDetailNormalMap(NiPixelData* pkLowDetailNormal)
{
    EE_UNUSED_ARG(pkLowDetailNormal);
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteCellSurfaceIndex(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, LeafData* pkLeafData)
{
    EE_UNUSED_ARG(uiCellRegionID);
    EE_UNUSED_ARG(uiNumCells);
    EE_UNUSED_ARG(pkLeafData);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadCellBoundData(efd::UInt32 uiCellRegionID,
    efd::UInt32 uiNumCells, CellData* pkCellData)
{
    EE_UNUSED_ARG(uiCellRegionID);
    EE_UNUSED_ARG(uiNumCells);
    EE_UNUSED_ARG(pkCellData);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteCellBoundData(efd::UInt32 uiCellRegionID, 
    efd::UInt32 uiNumCells, CellData* pkCellData)
{
    EE_UNUSED_ARG(uiCellRegionID);
    EE_UNUSED_ARG(uiNumCells);
    EE_UNUSED_ARG(pkCellData);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSectorFileVersion5To6Adapter::ReadTerrainSectorPhysXData(
    efd::map<efd::UInt32, NiPhysXMaterialMetaData>& kMaterialMap, 
    NiTerrainSectorPhysXData*& pkSampleData)
{
    EE_UNUSED_ARG(kMaterialMap);
    EE_UNUSED_ARG(pkSampleData);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSectorFileVersion5To6Adapter::WriteTerrainSectorPhysXData(
    efd::map<efd::UInt32, NiPhysXMaterialMetaData> kMaterialMap, 
    NiTerrainSectorPhysXData* pkSampleData)
{
    EE_UNUSED_ARG(kMaterialMap);
    EE_UNUSED_ARG(pkSampleData);
}

//--------------------------------------------------------------------------------------------------
