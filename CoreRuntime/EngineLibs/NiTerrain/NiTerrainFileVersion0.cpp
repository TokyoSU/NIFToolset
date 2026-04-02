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
#include "NiTerrainFileVersion0.h"
#include "NiTerrainXMLHelpers.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainFileVersion0, NiTerrainFileVersion1);
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion0::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to access the file
    NiString kSectorPath;
    kSectorPath.Format("%s%s", kID.m_kArchivePath.c_str(), "\\TerrainConfig.xml");

    // Attempt to open the file:
    efd::TiXmlDocument kFile(kSectorPath);
    if (NiTerrainXMLHelpers::LoadXMLFile(&kFile))
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = kFile.FirstChildElement("Terrain");            
        if (!pkCurElement)
            return NULL;

        // Inspect the file version
        const char* pcVersion = pkCurElement->Attribute("Version");
        if (pcVersion)
        {
            NiUInt32 uiFileVersion = 0;
            if (NiString(pcVersion).ToUInt(uiFileVersion))
                eVersion = uiFileVersion;
        }
    }

    return eVersion == 0;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion0::NiTerrainFileVersion0()
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion0::~NiTerrainFileVersion0()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion0::OpenErrorCode NiTerrainFileVersion0::Open(FileIdentifier kID, 
    efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);
    m_kTerrainArchive = (kID.m_kArchivePath);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion0::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainFileVersion3::Initialize();
    if (!bResult)
        return false;

    // Writing of this asset type is no longer supported.
    if (IsWritable())
        return false;

    // The supplied filename will not be correct. Attempt to load using
    // sector 0,0's surface index
    
    // Extract the file path from the supplied filename
    NiString kFilePath = m_kTerrainArchive.c_str();

    // Attempt to read from Sector_0_0
    kFilePath += "\\Sector_0_0";
    if (!efd::File::DirectoryExists(kFilePath))
        return false;

    // Attempt to open the surfaces.xml file
    kFilePath += "\\Surfaces.xml"; 
    m_kFile = efd::TiXmlDocument(kFilePath);
    if (!NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
        return false;

    // Initialize the variables:
    m_kSurfaceNameArray.RemoveAll();
    m_kSurfacePackageArray.RemoveAll();

    // Read the sector index
    if (!ReadSurfaceIndex(m_kFile.FirstChildElement("Block")))
        return false;

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion0::ReadSurfaceIndex(const efd::TiXmlElement* pkDocument)
{
    const efd::TiXmlElement* pkCurrentElement = pkDocument->FirstChildElement();
    if(pkCurrentElement)
    {
        const char* pcPackage = 0;
        const char* pcName = 0;
        const char* pcLayerNum = 0;

        // Loop through all surfaces in the list and collect the information
        while (pkCurrentElement)
        {
            pcPackage = pkCurrentElement->Attribute("package");
            if (pcPackage == 0)
                pcPackage = "";

            pcName = pkCurrentElement->Attribute("name");
            if (pcName == 0)
                pcName = "";

            pcLayerNum = pkCurrentElement->Attribute("position");
            if (pcLayerNum == 0)
                pcLayerNum = "";

            // Convert the layer number into an index
            NiUInt32 uiSurfaceIndex = 0;
            if (NiString(pcLayerNum).ToUInt(uiSurfaceIndex))
            {
                // Add this surface to the list
                m_kSurfacePackageArray.SetAtGrow(uiSurfaceIndex, pcPackage);
                m_kSurfaceNameArray.SetAtGrow(uiSurfaceIndex, pcName);
            }

            // Move onto the next surface
            pkCurrentElement = pkCurrentElement->NextSiblingElement();
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------
