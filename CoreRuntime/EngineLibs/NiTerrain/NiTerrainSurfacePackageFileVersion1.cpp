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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"
#include "NiTerrainSurfacePackageFileVersion1.h"
#include "NiTerrainXMLHelpers.h"
#include "NiTerrainUtils.h"
#include <NiStream.h>
#include <NiIntegerExtraData.h>
#include <efd/ecrLogIDs.h>
#include <NiSourceTexture.h>

static const char* XML_ELEMENT_PACKAGE = "Package";
static const char* XML_ELEMENT_SURFACE = "Surface";
static const char* XML_ELEMENT_METADATA = "MetaData";
static const char* XML_ATTRIBUTE_PACKAGENAME = "name";
static const char* XML_ATTRIBUTE_PACKAGEITERATION = "iteration";
static const char* XML_ATTRIBUTE_PACKAGEVERSION = "version";
static const char* XML_ATTRIBUTE_SURFACENAME = "name";
static const char* XML_ATTRIBUTE_TEXTURETILING = "texture_tiling";
static const char* XML_ATTRIBUTE_DETAILTEXTURETILING = "detail_texturing_tiling";
static const char* XML_ATTRIBUTE_ROTATION = "rotation";
static const char* XML_ATTRIBUTE_PARALLAXSTRENGTH = "parallax_strength";
static const char* XML_ATTRIBUTE_DISTRIBUTIONMASKSTRENGTH = "distribution_mask_strength";
static const char* XML_ATTRIBUTE_SPECULARPOWER = "specular_power";
static const char* XML_ATTRIBUTE_SPECULARINTENSITY = "specular_intensity";
static const char* XML_ATTRIBUTE_TEXTUREASSET = "textureAsset";
static const char* XML_ATTRIBUTE_LASTRELATIVEPATH = "lastRelativePath";
static const char* BINARY_PACKAGE_FILE_EXT = ".tmpkgb";

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiITerrainSurfacePackageFileVersion1, NiTerrainFileInterface);
//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::DetectFileVersion(
    FileIdentifier kID)
{
    // Attempt to open the file:
    efd::TiXmlDocument kFile(kID.m_kPackageFile.c_str());
    if (NiTerrainXMLHelpers::LoadXMLFile(&kFile))
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = kFile.FirstChildElement(XML_ELEMENT_PACKAGE);            
        if (!pkCurElement)
            return false;

        // Inspect the file version
        const char* pcVersion = pkCurElement->Attribute(XML_ATTRIBUTE_PACKAGEVERSION);
        if (pcVersion)
        {
            NiUInt32 uiFileVersion = 0;
            if (NiString(pcVersion).ToUInt(uiFileVersion))
                return uiFileVersion == 1;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion1::NiTerrainSurfacePackageFileVersion1()
    : m_bConfigurationValid(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion1::~NiTerrainSurfacePackageFileVersion1()
{
    // Release the collection of surface information
    efd::vector<SurfaceData*>::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        NiDelete(*kIter);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::GetFilePaths(efd::set<efd::utf8string>& kFilePaths)
{
    kFilePaths.insert(m_kPackageFilename);
    kFilePaths.insert(GenerateCompiledTextureFilename());
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion1::OpenErrorCode NiTerrainSurfacePackageFileVersion1::Open(
    FileIdentifier kID, efd::File::OpenMode eAccessMode)
{
    if (eAccessMode == efd::File::READ_ONLY && !DetectFileVersion(kID))
        return WRONG_VERSION;

    NiTerrainFileInterface::Open(kID.m_pkStoragePolicy, eAccessMode);
    m_kPackageFilename = (kID.m_kPackageFile);

    if (Initialize())
        return SUCCESS;
    else
        return FAIL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::Initialize()
{
    // Begin with the fail condition
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    // Initialize the base file
    bool bResult = NiITerrainSurfacePackageFileVersion1::Initialize();
    if (!bResult)
        return false;

    // Open the file
    m_kFile = efd::TiXmlDocument(m_kPackageFilename.c_str());
    if (!IsWritable()
        && !NiTerrainXMLHelpers::LoadXMLFile(&m_kFile))
    {
        return false;   
    }

    // Initialize the variables:
    m_kSurfaces.clear();

    // If we are reading from the file, then begin setting up:
    if (!IsWritable())
    {
        // Reset to the initial tag
        efd::TiXmlElement* pkCurElement = m_kFile.FirstChildElement(XML_ELEMENT_PACKAGE);            
        if (!pkCurElement)
        {
            return false;   
        }

        // Read the file version
        m_kFileVersion = 0;
        if (!NiString(pkCurElement->Attribute(XML_ATTRIBUTE_PACKAGEVERSION)).ToUInt(m_kFileVersion))
        {
            return false;
        }

        // Read the configuration data
        if (!ReadConfiguration(pkCurElement))
        {
            return false;
        }
    }
    else
    {
        // Must at have access to the file to write
        if (!efd::File::Access(m_kPackageFilename.c_str(), efd::File::WRITE_ONLY))
            return false;
    }

    // Assign success code
    m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::SUCCESS;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::Precache(efd::UInt32 uiDataFields)
{
    if (uiDataFields & DataField::SURFACE_CONFIG)
    {
        // Load the surface configuration
        efd::TiXmlElement* pkCurElement = m_kFile.FirstChildElement(XML_ELEMENT_PACKAGE);            
        if (pkCurElement)
        {
            ReadSurfaces(pkCurElement);
        }
    }

    if (uiDataFields & DataField::COMPILED_TEXTURES)
    {
        // Load the compiled texture index
        ReadCompiledTextures();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::Close()
{
    bool bResult = true;

    // Complete the writing of the file
    if (IsWritable())
    {
        bResult &= WriteFileHeader();
        bResult &= WritePackage();
        bResult &= WriteSurfaces();
        bResult &= WriteCompiledTextures();

        bResult &= m_kFile.SaveFile();
    }

    if (!bResult)
        m_eSuccess = NiTerrainStoragePolicy::IOSuccessCode::FAIL;

    NiITerrainSurfacePackageFileVersion1::Close();
}

//--------------------------------------------------------------------------------------------------
efd::utf8string NiTerrainSurfacePackageFileVersion1::GenerateCompiledTextureFilename()
{
    efd::utf8string kBinaryFilename = efd::PathUtils::PathRemoveFileExtension(m_kPackageFilename);
    kBinaryFilename += BINARY_PACKAGE_FILE_EXT;
    return kBinaryFilename;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::WriteFileHeader()
{
    if (!IsReady() || !IsWritable())
        return false;

    NiTerrainXMLHelpers::WriteXMLHeader(&m_kFile);
    efd::TiXmlElement* pkPackageElement = NiTerrainXMLHelpers::CreateElement(
        XML_ELEMENT_PACKAGE, NULL);
    pkPackageElement->SetAttribute(XML_ATTRIBUTE_PACKAGEVERSION, m_kFileVersion);
    m_kFile.LinkEndChild(pkPackageElement);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::WritePackage()
{
    if (!IsReady() || !IsWritable())
        return false;

    efd::TiXmlElement* pkRootElement = m_kFile.FirstChildElement(XML_ELEMENT_PACKAGE);
    pkRootElement->SetAttribute(XML_ATTRIBUTE_PACKAGENAME, m_kPackageName.c_str());

    efd::utf8string kIteration;
    efd::ParseHelper<efd::UInt32>::ToString(m_uiIteration, kIteration);
    pkRootElement->SetAttribute(XML_ATTRIBUTE_PACKAGEITERATION, kIteration.c_str());

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::WriteSurfaces()
{
    if (!IsReady() || !IsWritable())
        return false;

    // Helper variables
    efd::utf8string kString;
    typedef efd::ParseHelper<efd::Float32> ParseFloat;

    efd::TiXmlElement* pkPackageElement = m_kFile.FirstChildElement(XML_ELEMENT_PACKAGE);
    SurfaceList::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceData* pkSurface = *kIter;
        efd::TiXmlElement* pkSurfaceElement = NiTerrainXMLHelpers::CreateElement(
            XML_ELEMENT_SURFACE, pkPackageElement);

        // Save the surface's data into the element
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_SURFACENAME, pkSurface->m_kName.c_str());
        
        ParseFloat::ToString(pkSurface->m_fTextureTiling, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_TEXTURETILING, kString.c_str()); 
        ParseFloat::ToString(pkSurface->m_fDetailTiling, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_DETAILTEXTURETILING, kString.c_str());
        ParseFloat::ToString(pkSurface->m_fRotation, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_ROTATION, kString.c_str());
        ParseFloat::ToString(pkSurface->m_fParallaxStrength, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_PARALLAXSTRENGTH, kString.c_str());
        ParseFloat::ToString(pkSurface->m_fDistributionMaskStrength, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_DISTRIBUTIONMASKSTRENGTH, kString.c_str());
        ParseFloat::ToString(pkSurface->m_fSpecularPower, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_SPECULARPOWER, kString.c_str());
        ParseFloat::ToString(pkSurface->m_fSpecularIntensity, kString);
        pkSurfaceElement->SetAttribute(XML_ATTRIBUTE_SPECULARINTENSITY, kString.c_str());

        // Save the texture slots to the file
        for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
        {
            if (pkSurface->m_akTextureSlots[uiSlot].m_kAssetID.empty() && 
                pkSurface->m_akTextureSlots[uiSlot].m_kLastRelativePath.empty())
                continue;

            // Fetch the name of the element to make
            const char* pcMapName = NiSurface::GetTextureSlotName((NiSurface::SurfaceMapID)uiSlot);
            EE_ASSERT(pcMapName);
            efd::TiXmlElement* pkSlotElement = NiTerrainXMLHelpers::CreateElement(
                pcMapName, pkSurfaceElement);

            // Set the slot values:
            pkSlotElement->SetAttribute(XML_ATTRIBUTE_TEXTUREASSET,
                pkSurface->m_akTextureSlots[uiSlot].m_kAssetID.c_str());
            pkSlotElement->SetAttribute(XML_ATTRIBUTE_LASTRELATIVEPATH,
                pkSurface->m_akTextureSlots[uiSlot].m_kLastRelativePath.c_str());
        }

        // Save the metadata to the file
        efd::TiXmlElement* pkMetaDataElement = NiTerrainXMLHelpers::CreateElement(
            XML_ELEMENT_METADATA, pkSurfaceElement);
        pkSurface->m_kMetaData.Save(pkMetaDataElement);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::WriteCompiledTextures()
{
    if (!IsReady() || !IsWritable())
        return false;

    efd::utf8string kBinaryFilename = GenerateCompiledTextureFilename();
    
    // Open the file for writing
    efd::File* pkFile = efd::File::GetFile(kBinaryFilename.c_str(), efd::File::WRITE_ONLY);
    if (!pkFile)
        return false;

    // Setup the endianess
    bool bPlatformLittle = NiSystemDesc::GetSystemDesc().IsLittleEndian();
    pkFile->SetEndianSwap(!bPlatformLittle);

    // Write the version, and iteration count to the file
    efd::UInt32 uiVersion = GetFileVersion();
    efd::UInt32 uiIteration = m_uiIteration;
    efd::BinaryStreamSave(*pkFile, &uiVersion);
    efd::BinaryStreamSave(*pkFile, &uiIteration);

    // Setup the stream offset table (reserving data)
    efd::UInt32 uiTableOffset = pkFile->GetPosition();
    m_kCompiledStreamOffsets.clear();
    SurfaceList::iterator kIter;
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceData* pkSurface = *kIter;
        m_kCompiledStreamOffsets[pkSurface->m_kName] = 0;
    }
    WriteCompiledTextureTable(*pkFile, m_kCompiledStreamOffsets);
    
    // Write out all the textures into streams in the file
    for (kIter = m_kSurfaces.begin(); kIter != m_kSurfaces.end(); ++kIter)
    {
        SurfaceData* pkSurface = *kIter;
        
        // Update the offset table with the position of this stream
        pkFile->Seek(0, efd::File::ms_iSeekEnd);
        m_kCompiledStreamOffsets[pkSurface->m_kName] = pkFile->GetPosition();
        
        // Save the surface's info to the stream
        NiStream kStream;
        bool bOriginallyStatic[NiSurface::NUM_SURFACE_TEXTURES];
        for (efd::UInt32 uiTex = 0; uiTex < NiSurface::NUM_SURFACE_TEXTURES; ++uiTex)
        {
            NiTexture* pkTexture = pkSurface->m_aspTextures[uiTex];

            // Disabling static so it doesn't come out loaded as static
            NiSourceTexture* pkSourceTexture = NiDynamicCast(NiSourceTexture, pkTexture);
            if (pkSourceTexture)
            {
                bOriginallyStatic[uiTex] = pkSourceTexture->GetStatic();
                pkSourceTexture->SetStatic(true);
            }

            if (pkTexture)
            {
                
                efd::utf8string kTextureName;
                efd::ParseHelper<efd::UInt32>::ToString(uiTex, kTextureName);
                pkTexture->SetName(kTextureName.c_str());

                kStream.InsertObject(pkTexture);
            }
        }

        // Save the stream
        kStream.Save(pkFile);

        // Now switch all the textures back to non-static
        for (efd::UInt32 uiTex = 0; uiTex < NiSurface::NUM_SURFACE_TEXTURES; ++uiTex)
        {
            NiTexture* pkTexture = pkSurface->m_aspTextures[uiTex];

            // Disabling static so it doesn't come out loaded as static
            NiSourceTexture* pkSourceTexture = NiDynamicCast(NiSourceTexture, pkTexture);
            if (pkSourceTexture)
            {
                pkSourceTexture->SetStatic(bOriginallyStatic[uiTex]);
            }
        }
    }

    // Rewrite the offset table with the correct values
    pkFile->Seek(uiTableOffset, efd::File::ms_iSeekSet);
    WriteCompiledTextureTable(*pkFile, m_kCompiledStreamOffsets);

    // Cleanup
    NiDelete pkFile;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::WriteCompiledTextureTable(efd::BinaryStream& kStream,
    StreamOffsetTable& kOffsetMap)
{
    // Write the size of the table to the stream
    efd::UInt32 uiNumEntries = kOffsetMap.size();
    efd::BinaryStreamSave(kStream, &uiNumEntries);

    StreamOffsetTable::iterator kIter;
    for (kIter = kOffsetMap.begin(); kIter != kOffsetMap.end(); ++kIter)
    {
        const efd::utf8string& kSurfaceName = kIter->first;
        efd::UInt32& uiOffset = kIter->second;

        // Write out the surface name and it's offset
        efd::BinaryStreamSave(kStream, &kSurfaceName);
        efd::BinaryStreamSave(kStream, &uiOffset);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadConfiguration(efd::TiXmlElement* pkRootElement)
{
    // Extract the package name
    const char* pcPackageName = pkRootElement->Attribute(XML_ATTRIBUTE_PACKAGENAME);
    if (pcPackageName == 0)
        pcPackageName = "";
    m_kPackageName = pcPackageName;

    // Extract the iteration count
    const char* pcIteration = pkRootElement->Attribute(XML_ATTRIBUTE_PACKAGEITERATION);
    if (pcIteration == 0)
        pcIteration = "0";
    efd::ParseHelper<efd::UInt32>::FromString(pcIteration, m_uiIteration);

    m_bConfigurationValid = true;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaces(efd::TiXmlElement* pkRootElement)
{
    const efd::TiXmlElement* pkCurElement = pkRootElement->FirstChildElement(XML_ELEMENT_SURFACE);
    if (pkCurElement)
    {
        // Loop through all surfaces in the list and collect the information
        while (pkCurElement)
        {
            SurfaceData kTempSurfaceData;
            if (ReadSurface(pkCurElement, kTempSurfaceData))
            {
                // Add the surface to the list
                SurfaceData* pkSurfaceData = NiNew SurfaceData();
                *pkSurfaceData = kTempSurfaceData;
                m_kSurfaces.push_back(pkSurfaceData);
            }

            // Move onto the next surface
            pkCurElement = pkCurElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurface(const efd::TiXmlElement* pkSurfaceElement, 
    SurfaceData& kTempSurfaceData)
{
    const char* pcName = pkSurfaceElement->Attribute(XML_ATTRIBUTE_SURFACENAME);
    if (!pcName)
        return NULL;
    kTempSurfaceData.m_kName = pcName;

    // Read all of the surface Attributes.
    const char* pcValue = NULL;
    efd::Float32 fAttrib;

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_TEXTURETILING);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fTextureTiling = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_TEXTURETILING));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_DETAILTEXTURETILING);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fDetailTiling = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_DETAILTEXTURETILING));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_ROTATION);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fRotation = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_ROTATION));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_PARALLAXSTRENGTH);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fParallaxStrength = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_PARALLAXSTRENGTH));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_DISTRIBUTIONMASKSTRENGTH);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fDistributionMaskStrength = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_DISTRIBUTIONMASKSTRENGTH));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_SPECULARPOWER);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fSpecularPower = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_SPECULARPOWER));
        }
    }

    pcValue = pkSurfaceElement->Attribute(XML_ATTRIBUTE_SPECULARINTENSITY);
    if (pcValue)
    {
        if (efd::ParseHelper<efd::Float32>::FromString(pcValue, fAttrib))
        {
            kTempSurfaceData.m_fSpecularIntensity = fAttrib;
        }
        else
        {
            EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
                ("NiTerrainSurfacePackageFile: Error reading material Attribute '%s'.",
                XML_ATTRIBUTE_SPECULARINTENSITY));
        }
    }

    // Load in all the meta data for the surface
    const efd::TiXmlElement* pkMetaDataElement = 
        pkSurfaceElement->FirstChildElement(XML_ELEMENT_METADATA);
    if (pkMetaDataElement)
        kTempSurfaceData.m_kMetaData.Load(pkMetaDataElement);

    // Search the surface for textures now
    for (efd::SInt32 iIndex = 0; iIndex < NiSurface::NUM_SURFACE_MAPS; iIndex++)
    {
        const char* pcMapName = NiSurface::GetTextureSlotName((NiSurface::SurfaceMapID)iIndex);
        EE_ASSERT(pcMapName);

        const efd::TiXmlElement* pkElement = pkSurfaceElement->FirstChildElement(pcMapName);
        if (!pkElement || !pkElement->Attribute(XML_ATTRIBUTE_TEXTUREASSET))
            continue;

        const char* pcTextureAssetID = pkElement->Attribute(XML_ATTRIBUTE_TEXTUREASSET);
        const char* pcTextureRelPath = pkElement->Attribute(XML_ATTRIBUTE_LASTRELATIVEPATH);
        
        kTempSurfaceData.m_akTextureSlots[iIndex].m_kAssetID = pcTextureAssetID;
        if (pcTextureRelPath)
        {
            kTempSurfaceData.m_akTextureSlots[iIndex].m_kLastRelativePath = pcTextureRelPath;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadCompiledTextures()
{
    efd::utf8string kBinaryFilename = GenerateCompiledTextureFilename();

    // Open the file for reading
    efd::File* pkFile = efd::File::GetFile(kBinaryFilename.c_str(), efd::File::READ_ONLY);
    if (!pkFile)
        return true;

    // Setup the endianess
    bool bPlatformLittle = NiSystemDesc::GetSystemDesc().IsLittleEndian();
    pkFile->SetEndianSwap(!bPlatformLittle);

    // Read the version, and iteration count from the file
    efd::UInt32 uiVersion;
    efd::UInt32 uiIteration;
    efd::BinaryStreamLoad(*pkFile, &uiVersion);
    efd::BinaryStreamLoad(*pkFile, &uiIteration);

    // Make sure the version and iteration match the expected values
    if (uiVersion != GetFileVersion() ||
        uiIteration != m_uiIteration)
    {
        NiDelete pkFile;
        return true;
    }

    // Read in the stream offset table
    efd::UInt32 uiNumEntries;
    efd::BinaryStreamLoad(*pkFile, &uiNumEntries);
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumEntries; ++uiIndex)
    {
        efd::utf8string kSurfaceName;
        efd::UInt32 uiOffset;

        // Write out the surface name and it's offset
        efd::BinaryStreamLoad(*pkFile, &kSurfaceName);
        efd::BinaryStreamLoad(*pkFile, &uiOffset);

        // Update the map
        m_kCompiledStreamOffsets[kSurfaceName] = uiOffset;
    }
    
    // Thats enough, we'll read in the stream data on demand.
    NiDelete pkFile;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadPackageConfig(efd::utf8string& kPackageName,
    efd::UInt32& uiIteration)
{
    if (!m_bConfigurationValid)
        return false;

    kPackageName = m_kPackageName;
    uiIteration = m_uiIteration;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadNumSurfaces(efd::UInt32& uiNumSurfaces)
{
    if (!m_bConfigurationValid)
        return false;
    
    uiNumSurfaces = m_kSurfaces.size();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    efd::utf8string& kName, 
    efd::Float32& fTextureTiling,
    efd::Float32& fDetailTiling,
    efd::Float32& fRotation,
    efd::Float32& fParallaxStrength,
    efd::Float32& fDistributionMaskStrength,
    efd::Float32& fSpecularPower,
    efd::Float32& fSpecularIntensity,
    efd::UInt32& uiNumDecorationLayers)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    kName = 
        pkSurfaceData->m_kName;
    fTextureTiling = 
        pkSurfaceData->m_fTextureTiling;
    fDetailTiling = 
        pkSurfaceData->m_fDetailTiling;
    fRotation = 
        pkSurfaceData->m_fRotation;
    fParallaxStrength = 
        pkSurfaceData->m_fParallaxStrength;
    fDistributionMaskStrength = 
        pkSurfaceData->m_fDistributionMaskStrength;
    fSpecularPower = 
        pkSurfaceData->m_fSpecularPower;
    fSpecularIntensity = 
        pkSurfaceData->m_fSpecularIntensity;
    uiNumDecorationLayers = 
        pkSurfaceData->m_uiNumDecorationLayers;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID,
    NiTerrainAssetReference* pkReference)
{
    EE_ASSERT(pkReference);
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    if (uiSlotID >= NiSurface::NUM_SURFACE_MAPS)
        return false;

    pkReference->SetReferringAssetLocation(m_kPackageFilename);
    pkReference->SetAssetID(pkSurfaceData->m_akTextureSlots[uiSlotID].m_kAssetID);
    pkReference->SetRelativeAssetLocation(
        pkSurfaceData->m_akTextureSlots[uiSlotID].m_kLastRelativePath);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    NiMetaData& kMetaData)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    kMetaData = pkSurfaceData->m_kMetaData;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return false;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    return ReadSurfaceCompiledTextures(pkSurfaceData->m_kName, aspTextures, uiNumTextures);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfacePackageFileVersion1::ReadSurfaceCompiledTextures(efd::utf8string kSurfaceName, 
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    EE_UNUSED_ARG(uiNumTextures); // Only used in an assertion

    // Find the offset within the compiled texture file
    StreamOffsetTable::iterator kIter = m_kCompiledStreamOffsets.find(kSurfaceName);
    if (kIter == m_kCompiledStreamOffsets.end())
        return false;

    // Open the file and read in the stream    
    efd::utf8string kBinaryFilename = 
        efd::PathUtils::PathRemoveFileExtension(m_kPackageFilename);
    kBinaryFilename += BINARY_PACKAGE_FILE_EXT;

    // Open the file for reading
    efd::File* pkFile = efd::File::GetFile(kBinaryFilename.c_str(), efd::File::READ_ONLY);
    if (!pkFile)
        return false;

    // Setup the endianess
    bool bPlatformLittle = NiSystemDesc::GetSystemDesc().IsLittleEndian();
    pkFile->SetEndianSwap(!bPlatformLittle);

    // Seek to the relevant stream
    pkFile->Seek(kIter->second, efd::File::ms_iSeekSet);
    
    // Load the data
    NiStream kStream;
    if (!kStream.Load(pkFile))
    {
        NiDelete pkFile;
        return false;
    }

    // Read the textures
    efd::UInt32 uiStoredObjectCount = kStream.GetObjectCount();
    EE_ASSERT(uiNumTextures == NiSurface::NUM_SURFACE_TEXTURES);
    for (efd::UInt32 uiTex = 0; uiTex < uiStoredObjectCount; ++uiTex)
    {
        NiTexture* pkTexture = NiDynamicCast(NiTexture, kStream.GetObjectAt(uiTex));

        // Work out which texture this is:
        efd::UInt32 uiTexID;
        if (!efd::ParseHelper<efd::UInt32>::FromString(
            (const char*)pkTexture->GetName(), uiTexID))
            continue;

        aspTextures[uiTexID] = pkTexture;
    }
    
    NiDelete pkFile;    
    return uiStoredObjectCount > 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WritePackageConfig(const efd::utf8string& kPackageName,
    efd::UInt32 uiIteration)
{
    m_kPackageName = kPackageName;
    m_uiIteration = uiIteration;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WriteNumSurfaces(const efd::UInt32& uiNumSurfaces)
{
    // Add a bunch of entries to the surfaces array
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
    {
        SurfaceData* pkSurfaceData = NiNew SurfaceData();
        m_kSurfaces.push_back(pkSurfaceData);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WriteSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    const efd::utf8string& kName, 
    efd::Float32 fTextureTiling,
    efd::Float32 fDetailTiling,
    efd::Float32 fRotation,
    efd::Float32 fParallaxStrength,
    efd::Float32 fDistributionMaskStrength,
    efd::Float32 fSpecularPower,
    efd::Float32 fSpecularIntensity,
    efd::UInt32 uiNumDecorationLayers)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    pkSurfaceData->m_kName = kName;
    pkSurfaceData->m_fTextureTiling = fTextureTiling;
    pkSurfaceData->m_fDetailTiling = fDetailTiling;
    pkSurfaceData->m_fRotation = fRotation;
    pkSurfaceData->m_fParallaxStrength = fParallaxStrength;
    pkSurfaceData->m_fDistributionMaskStrength = fDistributionMaskStrength;
    pkSurfaceData->m_fSpecularPower = fSpecularPower;
    pkSurfaceData->m_fSpecularIntensity = fSpecularIntensity;
    pkSurfaceData->m_uiNumDecorationLayers = uiNumDecorationLayers;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WriteSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID,
    const NiTerrainAssetReference* pkReference)
{
    EE_ASSERT(pkReference);
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];
    if (uiSlotID >= NiSurface::NUM_SURFACE_MAPS)
        return;

    NiTerrainAssetReference kReference;
    kReference = *pkReference;
    kReference.SetReferringAssetLocation(m_kPackageFilename);

    pkSurfaceData->m_akTextureSlots[uiSlotID].m_kAssetID = 
        kReference.GetAssetID();
    pkSurfaceData->m_akTextureSlots[uiSlotID].m_kLastRelativePath = 
        kReference.GetLastRelativeLocation();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WriteSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    const NiMetaData& kMetaData)
{
    if (uiSurfaceIndex >= m_kSurfaces.size())
        return;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    pkSurfaceData->m_kMetaData = kMetaData;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfacePackageFileVersion1::WriteSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    EE_UNUSED_ARG(uiNumTextures); // Only used in an assertion

    if (uiSurfaceIndex >= m_kSurfaces.size())
        return;
    SurfaceData* pkSurfaceData = m_kSurfaces[uiSurfaceIndex];

    EE_ASSERT(uiNumTextures == NiSurface::NUM_SURFACE_TEXTURES);
    for (efd::UInt32 uiTex = 0; uiTex < NiSurface::NUM_SURFACE_TEXTURES; ++uiTex)
    {
        pkSurfaceData->m_aspTextures[uiTex] = aspTextures[uiTex];
    }
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFileVersion1::SurfaceData::SurfaceData()
    : m_kName()
    , m_fTextureTiling(1.0f)
    , m_fDetailTiling(1.0f)
    , m_fRotation(0.0f)
    , m_fParallaxStrength(0.0f)
    , m_fDistributionMaskStrength(0.0f)
    , m_fSpecularPower(0.0f)
    , m_fSpecularIntensity(0.0f)
    , m_uiNumDecorationLayers(0)
{
}

//--------------------------------------------------------------------------------------------------
NiITerrainSurfacePackageFileVersion1::NiITerrainSurfacePackageFileVersion1()
    : NiTerrainFileInterface(ms_InterfaceVersion)
{
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::Precache(efd::UInt32 uiDataFields)
{
    EE_UNUSED_ARG(uiDataFields);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadPackageConfig(efd::utf8string& kPackageName,
    efd::UInt32& uiIteration)
{
    EE_UNUSED_ARG(kPackageName);
    EE_UNUSED_ARG(uiIteration);
    
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WritePackageConfig(const efd::utf8string& kPackageName,
    efd::UInt32 uiIteration)
{
    EE_UNUSED_ARG(kPackageName);
    EE_UNUSED_ARG(uiIteration);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadNumSurfaces(efd::UInt32& uiNumSurfaces)
{
    EE_UNUSED_ARG(uiNumSurfaces);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WriteNumSurfaces(const efd::UInt32& uiNumSurfaces)
{
    EE_UNUSED_ARG(uiNumSurfaces);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    efd::utf8string& kName, 
    efd::Float32& fTextureTiling,
    efd::Float32& fDetailTiling,
    efd::Float32& fRotation,
    efd::Float32& fParallaxStrength,
    efd::Float32& fDistributionMaskStrength,
    efd::Float32& fSpecularPower,
    efd::Float32& fSpecularIntensity,
    efd::UInt32& uiNumDecorationLayers)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(kName);
    EE_UNUSED_ARG(fTextureTiling);
    EE_UNUSED_ARG(fDetailTiling);
    EE_UNUSED_ARG(fRotation);
    EE_UNUSED_ARG(fParallaxStrength);
    EE_UNUSED_ARG(fDistributionMaskStrength);
    EE_UNUSED_ARG(fSpecularPower);
    EE_UNUSED_ARG(fSpecularIntensity);
    EE_UNUSED_ARG(uiNumDecorationLayers);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WriteSurfaceConfig(efd::UInt32 uiSurfaceIndex,
    const efd::utf8string& kName, 
    efd::Float32 fTextureTiling,
    efd::Float32 fDetailTiling,
    efd::Float32 fRotation,
    efd::Float32 fParallaxStrength,
    efd::Float32 fDistributionMaskStrength,
    efd::Float32 fSpecularPower,
    efd::Float32 fSpecularIntensity,
    efd::UInt32 uiNumDecorationLayers)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(kName);
    EE_UNUSED_ARG(fTextureTiling);
    EE_UNUSED_ARG(fDetailTiling);
    EE_UNUSED_ARG(fRotation);
    EE_UNUSED_ARG(fParallaxStrength);
    EE_UNUSED_ARG(fDistributionMaskStrength);
    EE_UNUSED_ARG(fSpecularPower);
    EE_UNUSED_ARG(fSpecularIntensity);
    EE_UNUSED_ARG(uiNumDecorationLayers);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID,
    NiTerrainAssetReference* pkReference)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(uiSlotID);
    EE_UNUSED_ARG(pkReference);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WriteSurfaceSlot(efd::UInt32 uiSurfaceIndex,
    efd::UInt32 uiSlotID,
    const NiTerrainAssetReference* pkReference)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(uiSlotID);
    EE_UNUSED_ARG(pkReference);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    NiMetaData& kMetaData)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(kMetaData);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WriteSurfaceMetadata(efd::UInt32 uiSurfaceIndex,
    const NiMetaData& kMetaData)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(kMetaData);
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(aspTextures);
    EE_UNUSED_ARG(uiNumTextures);

    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiITerrainSurfacePackageFileVersion1::ReadSurfaceCompiledTextures(efd::utf8string kSurfaceName, 
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    EE_UNUSED_ARG(kSurfaceName);
    EE_UNUSED_ARG(aspTextures);
    EE_UNUSED_ARG(uiNumTextures);

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiITerrainSurfacePackageFileVersion1::WriteSurfaceCompiledTextures(efd::UInt32 uiSurfaceIndex,
    NiTexturePtr* aspTextures, efd::UInt32 uiNumTextures)
{
    EE_UNUSED_ARG(uiSurfaceIndex);
    EE_UNUSED_ARG(aspTextures);
    EE_UNUSED_ARG(uiNumTextures);
}

//--------------------------------------------------------------------------------------------------
