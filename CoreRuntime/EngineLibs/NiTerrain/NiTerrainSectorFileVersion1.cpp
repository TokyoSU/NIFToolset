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
#include "NiTerrainSectorFile.h"
#include "NiTerrainSectorFileVersion1.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainSectorFileVersion1, NiTerrainSectorFileVersion2, NiTypeMask::NiTerrainSectorFileVersion1);
//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::DetectFileVersion(FileIdentifier kID)
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

    return eVersion == 1;
}
//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion1::NiTerrainSectorFileVersion1()
    : m_puiLODOffsets(0)
    , m_puiLookUpTable(0)
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion1::~NiTerrainSectorFileVersion1()
{
    NiFree(m_puiLODOffsets);
    NiFree(m_puiLookUpTable);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion1::OpenErrorCode NiTerrainSectorFileVersion1::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    OpenErrorCode result = NiTerrainSectorFileVersion5::Open(kID, eAccessMode);
    return result;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    if (!NiTerrainSectorFileVersion2::Initialize())
        return false;  

    // Work out the total number of blocks in this file
    m_uiNumBlocks = 0;
    for (NiUInt32 uiLevel = 0; uiLevel <= m_uiNumLOD; ++uiLevel)
    {
        m_uiNumBlocks += NiUInt32(NiPow(4.0f, (float)uiLevel));
    }

    // Initialise the offset array
    m_auiChildOffsetsX[0] = 0;
    m_auiChildOffsetsX[1] = 1;
    m_auiChildOffsetsX[2] = 1;
    m_auiChildOffsetsX[3] = 0;
    m_auiChildOffsetsY[0] = 0;
    m_auiChildOffsetsY[1] = 0;
    m_auiChildOffsetsY[2] = 1;
    m_auiChildOffsetsY[3] = 1;

    // Build the heirachy lookup table
    CreateHierarchyLookUpTable(0);

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion1::CreateHierarchyLookUpTable(
    NiUInt32 uiNewBlockID)
{
    m_puiLODOffsets = NiAlloc(NiUInt32, m_uiNumLOD + 1);
    m_puiLookUpTable = NiAlloc(NiUInt32, m_uiNumBlocks);

    //Initialize the lod offset table
    NiUInt32 uiPreviousOffset = 0;
    for (NiInt32 i = m_uiNumLOD; i >= 0; --i)
    {
        m_puiLODOffsets[i] = uiPreviousOffset;
        uiPreviousOffset += 1 << (i * 2);
    }

    AddBlockToLookUp(uiNewBlockID, 0, 0, 0);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion1::AddBlockToLookUp(
    NiUInt32 uiNewBlockID,
    NiUInt32 uiCurrentLOD,
    NiUInt32 uiBlockX,
    NiUInt32 uiBlockY)
{
    // Add the block to the look up table
    m_puiLookUpTable[uiNewBlockID] = (m_uiNumBlocks - 1) - (uiBlockX +
        uiBlockY * (1 << uiCurrentLOD) + m_puiLODOffsets[uiCurrentLOD]);

    ++uiCurrentLOD;

    // Parse the children of this block if there are any
    if (uiCurrentLOD <= m_uiNumLOD)
    {
        for (NiUInt32 i = 0; i < 4; ++i)
        {
            NiUInt32 uiChildID = uiNewBlockID * 4 + 1 + i;
            NiUInt32 uiChildBlockX = uiBlockX * 2 + m_auiChildOffsetsX[i];
            NiUInt32 uiChildBlockY = uiBlockY * 2 + m_auiChildOffsetsY[i];

            AddBlockToLookUp(uiChildID, uiCurrentLOD,
                uiChildBlockX, uiChildBlockY);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion1::ReadFromBlockIDConverted(NiUInt32 uiBlockID)
{
    // Convert block id to index into the file
    NiUInt32 uiFileIndex = m_puiLookUpTable[uiBlockID];
    m_iCurrentBlockID = -1;
    m_iCurrentBlockLevel = 0;
    m_iCurrentBlockLevelIndex = 0;

    // We now parse the file to find the appropriate position in the file
    m_ulFilePosition = 0;
    m_ulFilePosition += sizeof(m_kFileHeader) - sizeof(m_kFileHeader.m_uiNumLOD);
    m_pkFile->Seek(m_ulFilePosition, efd::File::ms_iSeekSet);
    for (NiUInt32 i = 0; i <= uiFileIndex; ++i)
        OldNextBlock();

    m_iCurrentBlockID = uiBlockID;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::NextBlock()
{
    // Use the original version until the lookup table is complete 
    if (!m_puiLookUpTable)
        return NiTerrainSectorFileVersion2::NextBlock();

    NiInt32 iIndex = m_iCurrentBlockID + 1;
    ReadFromBlockIDConverted(iIndex);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::PushBlock(NiInt32 iChildID)
{
    // Use the original version until the lookup table is complete 
    if (!m_puiLookUpTable)
        return NiTerrainSectorFileVersion2::PushBlock(iChildID);

    NiInt32 iIndex = m_iCurrentBlockID * 4 + 1 + iChildID;
    ReadFromBlockIDConverted(iIndex);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::PopBlock()
{
    // Use the original version until the lookup table is complete 
    if (!m_puiLookUpTable)
        return NiTerrainSectorFileVersion2::PopBlock();

    NiInt32 iIndex = (m_iCurrentBlockID - 1) / 4;
    ReadFromBlockIDConverted(iIndex);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::OldNextBlock()
{
    return NiTerrainSectorFileVersion2::NextBlock();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::OldPopBlock()
{
    return NiTerrainSectorFileVersion2::PopBlock();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion1::OldPushBlock(NiUInt32 uiLOD)
{
    return NiTerrainSectorFileVersion2::PushBlock(uiLOD);
}

//--------------------------------------------------------------------------------------------------
