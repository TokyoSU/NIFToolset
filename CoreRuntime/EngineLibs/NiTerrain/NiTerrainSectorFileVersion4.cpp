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
#include "NiTerrainSectorFileVersion4.h"
#include "NiTerrainUtils.h"
#include "NiIndex.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainSectorFileVersion4, NiTerrainSectorFileVersion5, NiTypeMask::NiTerrainSectorFileVersion4);
//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion4::DetectFileVersion(FileIdentifier kID)
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

    return eVersion == 4;
}
//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion4::NiTerrainSectorFileVersion4()
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion4::~NiTerrainSectorFileVersion4()
{

}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorFileVersion4::OpenErrorCode NiTerrainSectorFileVersion4::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    OpenErrorCode result = NiTerrainSectorFileVersion5::Open(kID, eAccessMode);
    return result;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorFileVersion4::Initialize()
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
        LoadLegacyFileHeaderBinary(m_kFileHeader, *m_pkFile);
        m_ulFilePosition += sizeof(m_kFileHeader) -
            sizeof(m_kFileHeader.m_uiNumLOD);

        // Read the first block's data:
        if (!NextBlock())
            return false;

        // Calculate the number of LOD stored in this file (it is not in
        // the header of this version).
        m_kFileHeader.m_uiNumLOD = 0;
        while (PushBlock(0))
            m_kFileHeader.m_uiNumLOD++;

        while (PopBlock());
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorFileVersion4::LoadLegacyFileHeaderBinary(
    FileHeader& kFileHeader, efd::BinaryStream& kStream)
{
    NiStreamLoadBinary(kStream, kFileHeader.m_kVersion);
    NiStreamLoadBinary(kStream, kFileHeader.m_uiVertsPerBlock);
    kFileHeader.m_uiNumLOD = 0;
}

//--------------------------------------------------------------------------------------------------