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
#include "NiTerrainFileVersion1.h"
#include "NiTerrainXMLHelpers.h"

#include "NiTerrainSectorFileVersion5.h"
#include "NiTerrain.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainFileVersion1, NiTerrainFileVersion2);
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::DetectFileVersion(FileIdentifier kID)
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

    return eVersion == 1;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion1::Close()
{
    bool bResult = true;

    // Write the file
    if (IsWritable())
    {
        bResult &= WriteFileHeader();
        bResult &= WriteSurfaceIndex();

        bResult &= m_kFile.SaveFile();
    }

    if (!bResult)
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    NiITerrainFileVersion3::Close();
}
//--------------------------------------------------------------------------------------------------
NiString NiTerrainFileVersion1::GenerateTerrainConfigFilename()
{
    NiString kString = m_kTerrainArchive.c_str();
    kString += "\\TerrainConfig.xml";
    return kString;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion1::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)GenerateTerrainConfigFilename());
}
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount)
{
    NiITerrainSectorFileVersion6::FileIdentifier kID6;
    kID6.m_kArchivePath = m_kTerrainArchive;
    kID6.m_iSectorX = 0;
    kID6.m_iSectorY = 0;
    kID6.m_pkStoragePolicy = m_pkStoragePolicy;

    // Read these values out from Sector 0,0 since there are no other files around
    NiTerrain::StoragePolicy* pkStoragePolicy = (NiTerrain::StoragePolicy*)m_pkStoragePolicy;
    NiITerrainSectorFileVersion6* pkFile = 
        pkStoragePolicy->m_spSectorReadFormat->OpenLegacyFile<NiITerrainSectorFileVersion6>(kID6);
    if (!pkFile)
        return false;

    // Make sure we are dealing with an old terrain asset
    if (pkFile->GetFileVersion() > NiTerrainSectorFile::FileVersion(5))
    {
        pkFile->Close();
        EE_DELETE pkFile;
        return false;
    }
    NiITerrainSectorFileVersion5To6Adapter* pkOldFile = 
        (NiITerrainSectorFileVersion5To6Adapter*)pkFile;

    // Fetch the basic configuration values
    uiNumLOD = pkOldFile->GetNumLOD();
    uiSectorSize = ((pkOldFile->GetBlockWidthInVerts() - 1) << uiNumLOD) + 1;
    
    // Figure out the min/max height of the terrain
    fMinElevation = FLT_MAX;
    fMaxElevation = FLT_MIN;
    pkOldFile->GetMinMaxElevation(fMinElevation, fMaxElevation);
    EE_ASSERT(fMinElevation <= fMaxElevation);

    // Set vertex spacing to default
    fVertexSpacing = 1.0f;

    // Set the low detail specular values to defaults
    fLowDetailSpecularPower = 10.0f;
    fLowDetailSpecularIntensity = 0.5f;

    // Figure out the size of the overall mask
    uiMaskSize = pkOldFile->GetBlendMaskSize();
    // Figure out the size of the low detail diffuse texture
    uiLowDetailSize = pkOldFile->GetLowDetailTextureSize();
    
    // Close off the file
    pkOldFile->Close();
    EE_DELETE(pkOldFile);

    uiSurfaceCount = GetNumSurfaces();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::ReadSurface(NiUInt32 uiSurfaceIndex, NiFixedString& kPackageID, 
    NiFixedString& kSurfaceID)
{   
    if (uiSurfaceIndex < m_kSurfacePackageArray.GetSize())
    {
        kPackageID = GetSurfacePackage(uiSurfaceIndex);
        kSurfaceID = GetSurfaceName(uiSurfaceIndex);
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
//------------------------------------  OLD INTERFACE  ---------------------------------------------
//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion1::NiTerrainFileVersion1()
{
    m_kFileVersion = ms_InterfaceVersion;
    m_kInterfaceVersion = ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion1::~NiTerrainFileVersion1()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion1::OpenErrorCode NiTerrainFileVersion1::Open(FileIdentifier kID, 
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
bool NiTerrainFileVersion1::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainFileVersion3::Initialize();
    if (!bResult)
        return false;

    // Open the file
    m_kFile = efd::TiXmlDocument(GenerateTerrainConfigFilename());
    if (!IsWritable() && !NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
        return false;

    // Initialize the variables:
    m_kSurfaceNameArray.RemoveAll();
    m_kSurfacePackageArray.RemoveAll();

    // If we are reading from the file, then begin setting up:
    if (!IsWritable())
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = m_kFile.FirstChildElement("Terrain");            
        if (!pkCurElement)
            return NULL;

        // Read the file version
        m_kFileVersion = 0;
        if (!NiString(pkCurElement->Attribute("Version")).ToUInt(m_kFileVersion))
            return false;
        
        // Read the sector index
        if (!ReadSurfaceIndex(pkCurElement))
            return false;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::ReadSurfaceIndex(const efd::TiXmlElement* pkDocument)
{
    const efd::TiXmlElement* pkCurElement = pkDocument->FirstChildElement("SurfaceList");
    pkCurElement = pkCurElement->FirstChildElement("Surface");
    if(pkCurElement)
    {
        const char* pcPackage = 0;
        const char* pcName = 0;
        const char* pcLayerNum = 0;

        // Loop through all surfaces in the list and collect the information
        while (pkCurElement)
        {
            pcPackage = pkCurElement->Attribute("package");
            if (pcPackage == 0)
                pcPackage = "";

            pcName = pkCurElement->Attribute("name");
            if (pcName == 0)
                pcName = "";

            pcLayerNum = pkCurElement->Attribute("position");
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
            pkCurElement = pkCurElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::WriteFileHeader()
{
    if (!IsReady() || !IsWritable())
        return false;

    NiTerrainXMLHelpers::WriteXMLHeader(&m_kFile);
    efd::TiXmlElement* pkTerrainElement = NiTerrainXMLHelpers::CreateElement("Terrain", NULL);
    pkTerrainElement->SetAttribute("Version", m_kFileVersion);
    m_kFile.LinkEndChild(pkTerrainElement);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion1::WriteSurfaceIndex()
{
    if (!IsReady() || !IsWritable())
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement("Terrain");
    efd::TiXmlElement* pkSurfaceListElement = NiTerrainXMLHelpers::CreateElement(
        "SurfaceList", pkRootElement);

    // Write each surface into the file
    NiUInt32 uiNumSurfaces = GetNumSurfaces();
    for (NiUInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
    {   
        efd::TiXmlElement* pkSurfaceElement = NiTerrainXMLHelpers::CreateElement(
            "Surface", pkSurfaceListElement);
        
        // Write this surface's data to the file
        pkSurfaceElement->SetAttribute("package", GetSurfacePackage(uiIndex));
        pkSurfaceElement->SetAttribute("name", GetSurfaceName(uiIndex));
        pkSurfaceElement->SetAttribute("position", uiIndex);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
