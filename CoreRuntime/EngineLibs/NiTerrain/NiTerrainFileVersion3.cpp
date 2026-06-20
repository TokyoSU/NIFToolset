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
#include "NiTerrainFileVersion3.h"
#include "NiTerrainFileVersion2.h"
#include "NiTerrainFileVersion1.h"
#include "NiTerrainXMLHelpers.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiITerrainFileVersion3, NiTerrainFileInterface, NiTypeMask::NiITerrainFileVersion3);
NiImplementRTTI(NiTerrainFileVersion3, NiITerrainFileVersion3, NiTypeMask::NiTerrainFileVersion3);
//--------------------------------------------------------------------------------------------------
const char* NiTerrainFileVersion3::ms_pcTerrainConfigFile = "\\root.terrain";
//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::DetectFileVersion(FileIdentifier kID)
{
    FileVersion eVersion = 0;

    // Attempt to access the file
    NiString kSectorPath;
    kSectorPath.Format("%s%s", kID.m_kArchivePath.c_str(), ms_pcTerrainConfigFile);

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

    return eVersion == 3;
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion3::NiTerrainFileVersion3()
    : m_bConfigurationValid(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion3::~NiTerrainFileVersion3()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion3::SurfaceReference::SurfaceReference()
    : m_bValid(false) 
    , m_uiIteration(0)
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion3::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert((const char*)GenerateTerrainConfigFilename());
}

//--------------------------------------------------------------------------------------------------
NiTerrainFileVersion3::OpenErrorCode NiTerrainFileVersion3::Open(FileIdentifier kID, 
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
bool NiTerrainFileVersion3::Initialize()
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
    m_kSurfaceReferences.clear();

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
        
        // Read the surface index
        if (!ReadSurfaceIndex(pkCurElement))
            return false;

        // Read the configuration data
        if (!ReadConfiguration(pkCurElement))
            return false;
    }
    else
    {
        // Must at have access to the file to write
        if (!efd::File::Access(GenerateTerrainConfigFilename(), efd::File::WRITE_ONLY))
            return false;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::ReadConfiguration(efd::TiXmlElement* pkRootElement)
{
    efd::TiXmlElement* pkConfigElement = pkRootElement->FirstChildElement("Configuration");
    if (!pkConfigElement)
        return false;

    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "SectorSize", m_uiSectorSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "NumLOD", m_uiNumLOD);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MaskSize", m_uiMaskSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailTextureSize", m_uiLowDetailSize);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MinElevation", m_fMinElevation);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "MaxElevation", m_fMaxElevation);
    if (!NiTerrainXMLHelpers::ReadElement(pkConfigElement, "VertexSpacing", m_fVertexSpacing))
    {
        // Default value for vertex spacing
        m_fVertexSpacing = 1.0f;
    }
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "NumSurfaces", m_uiSurfaceCount);

    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::ReadElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    m_bConfigurationValid = true;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::ReadSurfaceIndex(efd::TiXmlElement* pkDocument)
{
    const efd::TiXmlElement* pkCurElement = pkDocument->FirstChildElement("SurfaceList");
    pkCurElement = pkCurElement->FirstChildElement("Surface");
    if(pkCurElement)
    {
        const char* pcName = 0;
        const char* pcLayerNum = 0;
        const char* pcPackageIteration = 0;
        const char* pcRelativePath = 0;
        const char* pcAssetID = 0;

        // Loop through all surfaces in the list and collect the information
        while (pkCurElement)
        {
            // Extract the values
            pcRelativePath = pkCurElement->Attribute("LastRelativePath");
            if (pcRelativePath == 0)
                pcRelativePath = "";

            pcAssetID = pkCurElement->Attribute("AssetID");
            if (pcAssetID == 0)
                pcAssetID = "";

            pcName = pkCurElement->Attribute("name");
            if (pcName == 0)
                pcName = "";

            pcLayerNum = pkCurElement->Attribute("position");
            if (pcLayerNum == 0)
                pcLayerNum = "";

            pcPackageIteration = pkCurElement->Attribute("packageIteration");
            if (pcPackageIteration == 0)
                pcPackageIteration = "0";

            // Convert the layer number into an index
            NiUInt32 uiSurfaceIndex = 0;
            NiUInt32 uiIteration = 0;
            if (NiString(pcLayerNum).ToUInt(uiSurfaceIndex) &&
                NiString(pcPackageIteration).ToUInt(uiIteration))
            {
                SurfaceReference kReference;
                kReference.m_kPackageAssetID = pcAssetID;
                kReference.m_kPackageRelativePath = pcRelativePath;
                kReference.m_kSurfaceName = pcName;
                kReference.m_uiIteration = uiIteration;
                kReference.m_bValid = true;

                // Add this surface to the list
                m_kSurfaceReferences[uiSurfaceIndex] = kReference;
            }

            // Move onto the next surface
            pkCurElement = pkCurElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion3::Close()
{
    bool bResult = true;

    // Write the file
    if (IsWritable())
    {
        bResult &= WriteFileHeader();
        bResult &= WriteConfiguration();
        bResult &= WriteSurfaceIndex();

        bResult &= m_kFile.SaveFile();
    }

    if (!bResult)
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    NiITerrainFileVersion3::Close();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::WriteFileHeader()
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
bool NiTerrainFileVersion3::WriteConfiguration()
{
    if (!IsReady() || !IsWritable() || !m_bConfigurationValid)
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement("Terrain");
    efd::TiXmlElement* pkConfigElement = NiTerrainXMLHelpers::CreateElement(
        "Configuration", pkRootElement);

    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "SectorSize", m_uiSectorSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "NumLOD", m_uiNumLOD);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MaskSize", m_uiMaskSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailTextureSize", m_uiLowDetailSize);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MinElevation", m_fMinElevation);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "MaxElevation", m_fMaxElevation);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "VertexSpacing", m_fVertexSpacing);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "NumSurfaces", m_uiSurfaceCount);

    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularPower", 
        m_fLowDetailSpecularPower);
    NiTerrainXMLHelpers::WriteElement(pkConfigElement, "LowDetailSpecularIntensity", 
        m_fLowDetailSpecularIntensity);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::WriteSurfaceIndex()
{
    if (!IsReady() || !IsWritable())
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement("Terrain");
    efd::TiXmlElement* pkSurfaceListElement = NiTerrainXMLHelpers::CreateElement(
        "SurfaceList", pkRootElement);

    // Write each surface into the file
    SurfaceReferenceMap::iterator kIter;
    for (kIter = m_kSurfaceReferences.begin(); kIter != m_kSurfaceReferences.end(); ++kIter)
    {   
        if (!kIter->second.m_bValid)
            continue;

        efd::TiXmlElement* pkSurfaceElement = NiTerrainXMLHelpers::CreateElement(
            "Surface", pkSurfaceListElement);
        
        // Save the surface's data into the attributes of the element
        SurfaceReference kReference = kIter->second;
        pkSurfaceElement->SetAttribute("LastRelativePath", 
            kReference.m_kPackageRelativePath.c_str());
        pkSurfaceElement->SetAttribute("AssetID", kReference.m_kPackageAssetID.c_str());
        pkSurfaceElement->SetAttribute("name", kReference.m_kSurfaceName.c_str());
        pkSurfaceElement->SetAttribute("position", kIter->first);
        pkSurfaceElement->SetAttribute("packageIteration", kReference.m_uiIteration);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity)
{
    efd::UInt32 uiDummyValue;
    return ReadConfiguration(uiSectorSize, uiNumLOD, uiMaskSize, uiLowDetailSize, fMinElevation,
        fMaxElevation, fVertexSpacing, fLowDetailSpecularPower, fLowDetailSpecularIntensity, 
        uiDummyValue);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount)
{
    if (!m_bConfigurationValid)
        return false;

    uiSectorSize = m_uiSectorSize;
    uiNumLOD = m_uiNumLOD;
    uiMaskSize = m_uiMaskSize;
    uiLowDetailSize = m_uiLowDetailSize;
    fMinElevation = m_fMinElevation;
    fMaxElevation = m_fMaxElevation;
    fVertexSpacing = m_fVertexSpacing;
    uiSurfaceCount = m_uiSurfaceCount;
    fLowDetailSpecularPower = m_fLowDetailSpecularPower;
    fLowDetailSpecularIntensity = m_fLowDetailSpecularIntensity;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion3::WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
    efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
    float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
    float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount)
{
    m_bConfigurationValid = true;

    m_uiSectorSize = uiSectorSize;
    m_uiNumLOD = uiNumLOD;
    m_uiMaskSize = uiMaskSize;
    m_uiLowDetailSize = uiLowDetailSize;
    m_fMinElevation = fMinElevation;
    m_fMaxElevation = fMaxElevation;
    m_fVertexSpacing = fVertexSpacing;
    m_uiSurfaceCount = uiSurfaceCount;
    m_fLowDetailSpecularPower = fLowDetailSpecularPower;
    m_fLowDetailSpecularIntensity = fLowDetailSpecularIntensity;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainFileVersion3::ReadSurface(NiUInt32 uiSurfaceIndex, 
    NiTerrainAssetReference* pkPackageRef, NiFixedString& kSurfaceID, efd::UInt32& uiIteration)
{   
    EE_ASSERT(pkPackageRef);

    SurfaceReferenceMap::iterator kIter = m_kSurfaceReferences.find(uiSurfaceIndex);
    if (kIter == m_kSurfaceReferences.end())
        return false;

    SurfaceReference kReference = kIter->second;

    if (!kReference.m_bValid)
        return false;

    pkPackageRef->SetAssetID(kReference.m_kPackageAssetID);
    pkPackageRef->SetRelativeAssetLocation(kReference.m_kPackageRelativePath);
    pkPackageRef->SetReferringAssetLocation((const char*)GenerateTerrainConfigFilename());
    kSurfaceID = kReference.m_kSurfaceName.c_str();
    uiIteration = kReference.m_uiIteration;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainFileVersion3::WriteSurface(NiUInt32 uiSurfaceIndex, 
    NiTerrainAssetReference* pkPackageRef, NiFixedString kSurfaceID, efd::UInt32 uiIteration)
{
    EE_ASSERT(pkPackageRef);

    pkPackageRef->SetReferringAssetLocation((const char*)GenerateTerrainConfigFilename());

    SurfaceReference kReference;
    kReference.m_kPackageAssetID = pkPackageRef->GetAssetID();
    kReference.m_kPackageRelativePath = pkPackageRef->GetLastRelativeLocation();
    kReference.m_kSurfaceName = (const char*)kSurfaceID;
    kReference.m_uiIteration = uiIteration;
    kReference.m_bValid = true;

    m_kSurfaceReferences[uiSurfaceIndex] = kReference;
}

//--------------------------------------------------------------------------------------------------
NiITerrainFileVersion3::NiITerrainFileVersion3()
    : NiTerrainFileInterface(ms_InterfaceVersion)
{
}

//--------------------------------------------------------------------------------------------------
NiFileInterface* NiITerrainFileVersion3::AdaptToNextVersion()
{
    // Previous versions also implement V3 interface, so they just need to report higher
    if (GetInterfaceVersion() < ms_InterfaceVersion)
    {
        m_kInterfaceVersion++;
        return this;
    }
    else
    {
        return EE_NEW NiITerrainFileVersion3To4Adapter(this);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion3::ReadConfiguration(efd::UInt32& uiSectorSize, efd::UInt32& uiNumLOD, 
    efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, float& fMinElevation, 
    float& fMaxElevation, float& fVertexSpacing, float& fLowDetailSpecularPower, 
    float& fLowDetailSpecularIntensity, efd::UInt32& uiSurfaceCount)
{
    EE_UNUSED_ARG(uiSectorSize);
    EE_UNUSED_ARG(uiNumLOD);
    EE_UNUSED_ARG(uiMaskSize); 
    EE_UNUSED_ARG(uiLowDetailSize);
    EE_UNUSED_ARG(fMinElevation); 
    EE_UNUSED_ARG(fMaxElevation);
    EE_UNUSED_ARG(fVertexSpacing); 
    EE_UNUSED_ARG(fLowDetailSpecularPower); 
    EE_UNUSED_ARG(fLowDetailSpecularIntensity); 
    EE_UNUSED_ARG(uiSurfaceCount);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion3::WriteConfiguration(efd::UInt32 uiSectorSize, efd::UInt32 uiNumLOD, 
    efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, float fMinElevation, 
    float fMaxElevation, float fVertexSpacing, float fLowDetailSpecularPower, 
    float fLowDetailSpecularIntensity, efd::UInt32 uiSurfaceCount)
{
    EE_UNUSED_ARG(uiSectorSize);
    EE_UNUSED_ARG(uiNumLOD);
    EE_UNUSED_ARG(uiMaskSize); 
    EE_UNUSED_ARG(uiLowDetailSize);
    EE_UNUSED_ARG(fMinElevation); 
    EE_UNUSED_ARG(fMaxElevation);
    EE_UNUSED_ARG(fVertexSpacing); 
    EE_UNUSED_ARG(fLowDetailSpecularPower); 
    EE_UNUSED_ARG(fLowDetailSpecularIntensity); 
    EE_UNUSED_ARG(uiSurfaceCount);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion3::ReadSurface(NiUInt32 uiSurfaceIndex, 
    NiTerrainAssetReference* pkPackageRef, NiFixedString& kSurfaceID, efd::UInt32& uiIteration)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(pkPackageRef);
    EE_UNUSED_ARG(kSurfaceID);
    EE_UNUSED_ARG(uiIteration);
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion3::WriteSurface(NiUInt32 uiSurfaceIndex, 
    NiTerrainAssetReference* pkPackageRef, NiFixedString kSurfaceID, efd::UInt32 uiIteration)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(pkPackageRef);
    EE_UNUSED_ARG(kSurfaceID);
    EE_UNUSED_ARG(uiIteration);
}

//--------------------------------------------------------------------------------------------------
NiITerrainFileVersion3To4Adapter::NiITerrainFileVersion3To4Adapter(NiITerrainFileVersion3* pBaseVersion)
: NiITerrainFileVersion4()
, m_spPrevVersion(pBaseVersion)
{
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::OpenErrorCode NiITerrainFileVersion3To4Adapter::Open(FileIdentifier kID, 
    efd::File::OpenMode eAccessMode)
{
    return m_spPrevVersion->Open(kID, eAccessMode);
}
//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion3To4Adapter::Close()
{
    return m_spPrevVersion->Close();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainFileVersion3To4Adapter::GetFileVersion() const
{
    return m_spPrevVersion->GetFileVersion();
}

//--------------------------------------------------------------------------------------------------
NiFileInterface::FileVersion NiITerrainFileVersion3To4Adapter::GetInterfaceVersion() const
{
    return ms_InterfaceVersion;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion3To4Adapter::IsReady() const
{
    return m_spPrevVersion->IsReady();
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion3To4Adapter::IsWritable() const
{
    return m_spPrevVersion->IsWritable();
}

//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion3To4Adapter::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    return m_spPrevVersion->GetFilePaths(kFilePaths);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainFileVersion3To4Adapter::ReadConfiguration(efd::UInt32& uiSectorSize, 
    efd::UInt32& uiNumLOD, efd::UInt32& uiMaskSize, efd::UInt32& uiLowDetailSize, 
    float& fMinElevation, float& fMaxElevation, float& fVertexSpacing, 
    float& fLowDetailSpecularPower, float& fLowDetailSpecularIntensity)
{
    efd::UInt32 uiDummyValue;
    bool bResult = m_spPrevVersion->ReadConfiguration(uiSectorSize, uiNumLOD, uiMaskSize, 
        uiLowDetailSize, fMinElevation, fMaxElevation, fVertexSpacing, 
        fLowDetailSpecularPower, fLowDetailSpecularIntensity, uiDummyValue);

    // Force the size of the low detail texture to be at least 256
    if (uiLowDetailSize < 256)
        uiLowDetailSize = 1024;

    return bResult;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainFileVersion3To4Adapter::WriteConfiguration(efd::UInt32 uiSectorSize, 
    efd::UInt32 uiNumLOD, efd::UInt32 uiMaskSize, efd::UInt32 uiLowDetailSize, 
    float fMinElevation, float fMaxElevation, float fVertexSpacing, 
    float fLowDetailSpecularPower, float fLowDetailSpecularIntensity)
{
    return m_spPrevVersion->WriteConfiguration(uiSectorSize, uiNumLOD, uiMaskSize, 
        uiLowDetailSize, fMinElevation, fMaxElevation, fVertexSpacing, 
        fLowDetailSpecularPower, fLowDetailSpecularIntensity, 0);
}

//--------------------------------------------------------------------------------------------------