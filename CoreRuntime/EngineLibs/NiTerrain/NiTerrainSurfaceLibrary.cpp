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
#include "NiTerrainSurfaceLibrary.h"
#include "NiSurfacePackage.h"
#include "NiSurface.h"
#include "NiTerrainSurfacePackageFile.h"

#include "NiTerrainSurfacePackageFileVersion0.h"

#include <efd/ecrLogIDs.h>

//--------------------------------------------------------------------------------------------------
NiTerrainSurfaceLibrary::NiTerrainSurfaceLibrary()
{
}
//--------------------------------------------------------------------------------------------------
NiSurfacePackage* NiTerrainSurfaceLibrary::FindPackageByName(efd::utf8string kPackageName)
{
    PackageMap::iterator kIter = m_kNameToPackage.find(kPackageName);
    if (kIter != m_kNameToPackage.end())
        return kIter->second;
    else
        return NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage* NiTerrainSurfaceLibrary::FindPackageByAsset(efd::utf8string kPackageAssetID)
{
    PackageMap::iterator kIter = m_kAssetIDToPackage.find(kPackageAssetID);
    if (kIter != m_kAssetIDToPackage.end())
    {
        return kIter->second;
    }
    else
    {
        efd::utf8string redirectedAssetID;
        if (m_kAssetRedirectionsOrigToCurrent.find(kPackageAssetID, redirectedAssetID))
        {
            kIter = m_kAssetIDToPackage.find(redirectedAssetID);
            if (kIter != m_kAssetIDToPackage.end())
            {
                return kIter->second;
            }
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage* NiTerrainSurfaceLibrary::FindPackageByFile(efd::utf8string kPackageFilename)
{
    PackageMap::iterator kIter = m_kFilenameToPackage.find(kPackageFilename);
    if (kIter != m_kFilenameToPackage.end())
        return kIter->second;
    else
        return NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiTerrainSurfaceLibrary::FindSurfaceByName(efd::utf8string kPackageName, 
    efd::utf8string kSurfaceName)
{
    NiSurface* pkSurface = NULL;

    NiSurfacePackage* pkPackage = FindPackageByName(kPackageName);
    if (pkPackage && pkPackage->GetSurface(kSurfaceName.c_str(), pkSurface))
        return pkSurface;

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiTerrainSurfaceLibrary::FindSurfaceByAsset(efd::utf8string kPackageAssetID,
    efd::utf8string kSurfaceName)
{
    NiSurface* pkSurface = NULL;

    NiSurfacePackage* pkPackage = FindPackageByAsset(kPackageAssetID);
    if (pkPackage && pkPackage->GetSurface(kSurfaceName.c_str(), pkSurface))
        return pkSurface;

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiTerrainSurfaceLibrary::FindSurfaceByFile(efd::utf8string kPackageFilename, 
    efd::utf8string kSurfaceName)
{
    NiSurface* pkSurface = NULL;

    NiSurfacePackage* pkPackage = FindPackageByFile(kPackageFilename);
    if (pkPackage && pkPackage->GetSurface(kSurfaceName.c_str(), pkSurface))
        return pkSurface;

    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::AddPackage(NiSurfacePackage* pkPackage)
{
    // Check if the package is already registered
    NiSurfacePackagePtr spPackage = pkPackage;
    if (m_kPackages.find(spPackage) != m_kPackages.end())
        return;

    // Add to the package set
    m_kPackages.insert(spPackage);

    // Make sure the package has an asset ID, (set it to the package name if not)
    if (spPackage->GetAssetID() == efd::utf8string(""))
        spPackage->SetAssetID((const char*)pkPackage->GetName());

    // Check for existing entries in the maps for these packages
    // Use a package smart pointer to make sure that the original packages stay valid until after 
    // the new package has been put in place.
    NiSurfacePackagePtr pkOriginalAsset = FindPackageByName((const char*)spPackage->GetName());
    NiSurfacePackagePtr pkOriginalName = FindPackageByAsset((const char*)spPackage->GetAssetID());
    NiSurfacePackagePtr pkOriginalFile = FindPackageByFile((const char*)spPackage->GetFilename());
    EE_ASSERT(pkOriginalAsset == pkOriginalName);
    EE_ASSERT(pkOriginalAsset == pkOriginalFile);

    // Remove the original asset
    if (pkOriginalAsset)
        RemovePackage(pkOriginalAsset);

    // Register to all the different indexes:
    m_kAssetIDToPackage[(const char*)spPackage->GetAssetID()] = spPackage;
    m_kNameToPackage[(const char*)spPackage->GetName()] = spPackage;
    m_kFilenameToPackage[(const char*)spPackage->GetFilename()] = spPackage;

    RaisePackageUpdated(spPackage->GetAssetID());
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RemovePackage(NiSurfacePackage* pkPackage)
{
    m_kAssetIDToPackage.erase((const char*)pkPackage->GetAssetID());
    m_kNameToPackage.erase((const char*)pkPackage->GetName());
    m_kFilenameToPackage.erase((const char*)pkPackage->GetFilename());
        
    m_kPackages.erase(pkPackage);
}

//-------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::FlushLoadedPackages()
{
    m_kAssetIDToPackage.clear();
    m_kNameToPackage.clear();
    m_kFilenameToPackage.clear();

    m_kPackages.clear();
}

//-------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::InjectReference(const NiTerrainAssetReference& kReference)
{
    RegisterReference_Internal(kReference, NULL);
}

//-------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RegisterReference_Internal(const NiTerrainAssetReference& kReference, 
    EventType::HandlerType* pkCallback)
{ 
    bool bAttachHandler = true;

    const efd::utf8string& kAssetID = kReference.GetAssetID();
    if (pkCallback)
    {
        NiPointer<EventType>& pkEvent = m_kReferences[kAssetID];
        if (!pkEvent)
            pkEvent = NiNew EventType();
        
        bAttachHandler = pkEvent->AttachHandler(pkCallback);
    }

    // Attempt to find the package
    NiSurfacePackage* pkPackage = FindPackageByAsset(kAssetID);
    if (pkPackage)
    {
        // Send the event to say it is already loaded
        if (pkCallback)
            pkCallback->HandleEvent(pkPackage, kAssetID);
    }
    else if (IsPackageLoading(kAssetID))
    {
        // The package may already be attempting to load?
        // Do nothing
    }
    else
    {
        // Insert a request to load the referenced package
        RequestLoadPackage(kReference);
    }

    if (!bAttachHandler)
        NiDelete pkCallback;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::DeregisterReference_Internal(
    const NiTerrainAssetReference& kReference, EventType::HandlerType* kCallback)
{
    const efd::utf8string& kAssetID = kReference.GetAssetID();
    ReferenceMap::iterator kIter = m_kReferences.find(kAssetID);
    if (kIter != m_kReferences.end())
    {
        EventType* pkEvent = kIter->second;
        pkEvent->DetachHandler(kCallback);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RequestReload(NiSurfacePackage* pkPackage)
{
    // Generate an appropriate reference object
    NiTerrainAssetReference kReference;
    kReference.SetAssetID(pkPackage->GetAssetID());
    kReference.SetRelativeAssetLocation(pkPackage->GetFilename());

    // Ask to load this package
    RequestLoadPackage(kReference);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RequestLoadPackage(const NiTerrainAssetReference& kReference)
{
    // Add to the pending list
    NiTerrainAssetReferencePtr spPending = NiNew NiTerrainAssetReference();
    *spPending = kReference;
    spPending->AttachObserver(this, &NiTerrainSurfaceLibrary::OnPackageResolved);
    m_kPendingReferences.insert(spPending);

    // Now request that the reference be resolved
    StoragePolicy* pkStoragePolicy = GetStoragePolicy();
    pkStoragePolicy->ResolveAssetReference(spPending);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfaceLibrary::IsPackageLoading(const efd::utf8string& kAssetID)
{
    // Check for this package being resolved
    {
        PendingPackageSet::iterator kIter;
        for (kIter = m_kPendingReferences.begin(); kIter != m_kPendingReferences.end(); ++kIter)
        {
            efd::utf8string curAssetID = (*kIter)->GetAssetID();
            if (curAssetID == kAssetID)
                return true;
        }
    }

    // Check for this package's surfaces being loaded
    {
        PendingTextureMap::iterator kIter;
        for (kIter = m_kPendingTextureReferences.begin(); 
            kIter != m_kPendingTextureReferences.end();
            ++kIter)
        {
            NiSurface* pkSurface = kIter->second;
            if (!pkSurface)
                continue;

            NiSurfacePackage* pkPackage = pkSurface->GetPackage();
            if (!pkPackage)
                continue;

            efd::utf8string curAssetID = pkPackage->GetAssetID();
            if (curAssetID == kAssetID)
                return true;
        }
    }

    // Check against redirections too
    efd::utf8string redirectedAssetID;
    if (m_kAssetRedirectionsOrigToCurrent.find(kAssetID, redirectedAssetID))
    {
        if (IsPackageLoading(redirectedAssetID))
            return true;
    }
    
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfaceLibrary::IsPackageLoading()
{
    if (m_kPendingReferences.size())
        return true;

    if (m_kPendingTextureReferences.size())
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::OnPackageResolved(NiTerrainAssetReference* pkReference, 
    efd::UInt32)
{
    // Lets see if the asset was resolved...
    if (pkReference->GetResolvedLocation().empty())
    {
        efd::utf8string kLastKnown = pkReference->GetStoredLocation();
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL2,
            ("NiTerrainSurfaceLibrary::OnPackageResolved: "
            "Failed to resolve package reference. (%s). The last known location was (%s)", 
            pkReference->GetAssetID().c_str(), pkReference->GetStoredLocation().c_str()));

        RaisePackageUpdated(pkReference->GetAssetID());
        m_kPendingReferences.erase(pkReference);
        return;
    }

    // Register any asset redirections that were detected:
    efd::utf8string originalAssetID = pkReference->GetAssetID();
    efd::utf8string resolvedAssetID = pkReference->GetResolvedAssetID();
    if (originalAssetID != resolvedAssetID)
    {
        m_kAssetRedirectionsCurrentToOrig[resolvedAssetID] = originalAssetID;
        m_kAssetRedirectionsOrigToCurrent[originalAssetID] = resolvedAssetID;

        NiSurfacePackage* pkPackage = FindPackageByAsset(resolvedAssetID);
        if (pkPackage)
        {
            RaisePackageUpdated(resolvedAssetID);
            m_kPendingReferences.erase(pkReference);
            return;
        }
    }

    // Asset should be resolved, so lets attempt to load it.
    LoadPackage(resolvedAssetID, pkReference->GetResolvedLocation());

    // Delete the reference
    m_kPendingReferences.erase(pkReference);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RaisePackageUpdated(const efd::utf8string& kAssetID)
{
    // Look up the package
    NiSurfacePackage* pkPackage = FindPackageByAsset(kAssetID);

    // Look up the event to raise
    ReferenceMap::iterator kIter = m_kReferences.find(kAssetID);
    if (kIter != m_kReferences.end())
    {
        EventType* pkEvent = kIter->second;
        pkEvent->Raise(pkPackage, kAssetID);
    }

    // Look up any redirected events to raise
    efd::utf8string redirectedAssetID;
    if (m_kAssetRedirectionsCurrentToOrig.find(kAssetID, redirectedAssetID))
    {
        kIter = m_kReferences.find(redirectedAssetID);
        if (kIter != m_kReferences.end())
        {
            EventType* pkEvent = kIter->second;
            pkEvent->Raise(pkPackage, redirectedAssetID);
        }
    }
    
    // Now raise the global event for when a package is updated
    RaiseEvent(pkPackage);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSurfaceLibrary::SavePackage(NiSurfacePackage* pkPackage)
{
    if (!pkPackage)
        return false;

    // Open the file
    NiTerrainSurfacePackageFile* pkFile = OpenPackageFile(pkPackage->GetFilename(), true);
    if (!pkFile)
        return false;
    pkFile->WritePackageConfig((const char*)pkPackage->GetName(), pkPackage->GetIteration());

    // Fetch the set of surfaces
    NiTPrimitiveSet<NiSurface*> kSurfaces;
    pkPackage->GetSurfaces(kSurfaces);

    // Write the number of surfaces
    efd::UInt32 uiNumSurfaces = kSurfaces.GetSize();
    pkFile->WriteNumSurfaces(uiNumSurfaces);

    // Iterate over all the surfaces 
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
    {
        NiSurface* pkSurface = kSurfaces.GetAt(uiIndex);
        // Write out the attributes
        pkFile->WriteSurfaceConfig(uiIndex, 
            (const char*)pkSurface->GetName(),
            pkSurface->GetTextureTiling(),
            pkSurface->GetDetailTextureTiling(),
            pkSurface->GetRotation(),
            pkSurface->GetParallaxStrength(),
            pkSurface->GetDistributionMaskStrength(),
            pkSurface->GetSpecularPower(),
            pkSurface->GetSpecularIntensity(),
            0);

        // Write out the texture slots
        for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
        {
            NiSurface::TextureSlotEntry* pkSlot = pkSurface->GetTextureSlotEntry(
                NiSurface::SurfaceMapID(uiSlot));
            if (!pkSlot)
                continue;

            NiTerrainAssetReferencePtr spReference = NiNew NiTerrainAssetReference();
            spReference->SetAssetID(pkSlot->m_kAssetId);
            spReference->SetResolvedLocation((const char*)pkSlot->m_kFilePath);
            
            pkFile->WriteSurfaceSlot(uiIndex, uiSlot, spReference);
        }

        // Write out the metadata
        pkFile->WriteSurfaceMetadata(uiIndex, pkSurface->GetMetaData());

        // Write out the compiled textures for the surface
        NiTexturePtr aspTextures[NiSurface::NUM_SURFACE_TEXTURES];
        NiTexturingPropertyPtr spTexProp = pkSurface->GenerateSurfaceTextures();
        if (spTexProp)
        {
            // Extract the textures from the shader maps
            efd::UInt32 uiNumMaps = spTexProp->GetShaderMapCount();
            uiNumMaps = efd::Min(uiNumMaps, efd::UInt32(NiSurface::NUM_SURFACE_TEXTURES));
            for (efd::UInt32 uiTex = 0; uiTex < uiNumMaps; ++uiTex)
            {
                NiTexturingProperty::ShaderMap* pkMap = spTexProp->GetShaderMap(uiTex);
                aspTextures[uiTex] = pkMap->GetTexture();
            }

            pkFile->WriteSurfaceCompiledTextures(uiIndex, 
                aspTextures, 
                NiSurface::NUM_SURFACE_TEXTURES);
        }
    }

    pkPackage->MarkDirty(false);

    pkFile->Close();
    NiDelete pkFile;

    return true;
}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage* NiTerrainSurfaceLibrary::ReadPackage(efd::utf8string kFilename)
{
    NiSurfacePackage* pkPackage = NULL;

    // Open the file
    NiTerrainSurfacePackageFile* pkFile = OpenPackageFile(kFilename, false);
    efd::utf8string kPackageName;
    efd::UInt32 uiIteration;
    if (pkFile)
    {
        // Precache the required data from disk
        pkFile->Precache(NiTerrainSurfacePackageFile::DataField::SURFACE_CONFIG);

        // Read the package's name and iteration count
        pkFile->ReadPackageConfig(kPackageName, uiIteration);

        // Create the package:
        pkPackage = NiSurfacePackage::Create(kPackageName.c_str(), this);
        pkPackage->SetFileName(kFilename.c_str());
        pkPackage->SetIteration(uiIteration);

        // Read in the surfaces:
        efd::UInt32 uiNumSurfaces = 0;
        pkFile->ReadNumSurfaces(uiNumSurfaces);
        for (efd::UInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
        {
            // Read in the attributes
            efd::utf8string kName;
            efd::Float32 fTextureTiling;
            efd::Float32 fDetailTiling;
            efd::Float32 fRotation;
            efd::Float32 fParallaxStrength;
            efd::Float32 fDistributionMaskStrength;
            efd::Float32 fSpecularPower;
            efd::Float32 fSpecularIntensity;
            efd::UInt32 uiNumDecorationLayers;
            pkFile->ReadSurfaceConfig(uiIndex, 
                kName,
                fTextureTiling,
                fDetailTiling,
                fRotation,
                fParallaxStrength,
                fDistributionMaskStrength,
                fSpecularPower,
                fSpecularIntensity,
                uiNumDecorationLayers);

            NiSurface* pkSurface = NULL;
            pkPackage->CreateSurface(kName.c_str(), pkSurface);
            if (!pkSurface)
                continue;

            // Push the attributes to the surface
            pkSurface->SetTextureTiling(fTextureTiling);
            pkSurface->SetDetailTextureTiling(fDetailTiling);
            pkSurface->SetRotation(fRotation);
            pkSurface->SetParallaxStrength(fParallaxStrength);
            pkSurface->SetDistributionMaskStrength(fDistributionMaskStrength);
            pkSurface->SetSpecularPower(fSpecularPower);
            pkSurface->SetSpecularIntensity(fSpecularIntensity);

            // Read in all the texture maps
            for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
            {
                NiTerrainAssetReference kReference;
                pkFile->ReadSurfaceSlot(uiIndex, uiSlot, &kReference);
                if (kReference.GetAssetID().empty() && kReference.GetRelativeAssetLocation().empty())
                    continue;

                NiSurface::TextureSlotEntry* pSlot = EE_NEW NiSurface::TextureSlotEntry();
                pSlot->m_kFilePath = kReference.GetRelativeAssetLocation().c_str();
                pSlot->m_kAssetId = kReference.GetAssetID();
                pkSurface->SetTextureSlotEntry((NiSurface::SurfaceMapID)uiSlot, pSlot);
            }

            // Read in the metadata
            pkFile->ReadSurfaceMetadata(uiIndex, pkSurface->GetMetaData());
        }
    }

    // Clean up the file
    if (pkFile)
    {
        pkFile->Close();
        NiDelete pkFile;
    }

    return pkPackage;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::LoadPackage(efd::utf8string kAssetID, 
    efd::utf8string kFilename)
{
    NiSurfacePackage* pkPackage = ReadPackage(kFilename);
    if (pkPackage)
    {
        // Package loaded successfully
        pkPackage->SetAssetID(kAssetID.c_str());
        RequestLoadSurfaces(pkPackage);
    }
    else
    {
        // Failed to load the package
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR0,
            ("NiTerrainSurfaceLibrary::LoadPackage: Failed to load package file. (%s:%s).", 
            kAssetID.c_str(), kFilename.c_str()));

        RaisePackageUpdated(kAssetID);
    }
}   

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::RequestLoadSurfaces(NiSurfacePackage* pkPackage)
{
    StoragePolicy* pkStoragePolicy = GetStoragePolicy();

    // Fetch the set of surfaces
    NiTPrimitiveSet<NiSurface*> kSurfaces;
    pkPackage->GetSurfaces(kSurfaces);

    // Iterate over all the surfaces and resolve their textures
    bool bPackageResolved = true;
    efd::UInt32 uiNumSurfaces = kSurfaces.GetSize();
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumSurfaces; ++uiIndex)
    {
        NiSurface* pkSurface = kSurfaces.GetAt(uiIndex);

        // Iterate over all the texture slots:
        typedef efd::set<NiTerrainAssetReference*> ReferenceSet;
        ReferenceSet kPendingReferences;
        for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
        {
            NiSurface::TextureSlotEntry* pkSlot = pkSurface->GetTextureSlotEntry(
                NiSurface::SurfaceMapID(uiSlot));
            if (!pkSlot)
                continue;
            
            // Setup a reference to this texture
            NiTerrainAssetReferencePtr pkPending = NiNew NiTerrainAssetReference();
            pkPending->SetReferringAssetLocation(pkPackage->GetFilename());
            pkPending->SetRelativeAssetLocation((const char*)pkSlot->m_kFilePath);
            pkPending->SetAssetID(pkSlot->m_kAssetId);
            pkPending->AttachObserver(this, &NiTerrainSurfaceLibrary::OnTextureResolved);

            // Clear out the filename in the texture slot
            pkSlot->m_kFilePath = "";

            m_kPendingTextureReferences[pkPending] = pkSurface;
            kPendingReferences.insert(pkPending);
        }

        // Work out if this surface has been resolved
        bool bResolved = kPendingReferences.size() == 0;
        pkSurface->MarkResolved(bResolved);
        bPackageResolved &= bResolved;

        // Now request that these references be resolved
        ReferenceSet::iterator kIter;
        for (kIter = kPendingReferences.begin(); kIter != kPendingReferences.end(); ++kIter)
        {
            pkStoragePolicy->ResolveAssetReference(*kIter);
        }
    }

    if (bPackageResolved)
    {
        // Package is already completely resolved
        AddPackage(pkPackage);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSurfaceLibrary::OnTextureResolved(NiTerrainAssetReference* pkReference,
    efd::UInt32)
{
    // Find the surface that this reference was meant for
    NiSurface* pkSurface = m_kPendingTextureReferences[pkReference];
    
    // Lets see if the asset was resolved...
    efd::utf8string kResolvedLocation = pkReference->GetResolvedLocation();
    if (kResolvedLocation.empty())
    {
        efd::utf8string kLastKnown = pkReference->GetStoredLocation();
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL2,
            ("NiTerrainSurfaceLibrary::OnTextureResolved: "
            "Failed to resolve texture reference for surface '(%s)' (%s). "
            "The last known location was (%s)", (const char*)pkSurface->GetName(),
            pkReference->GetAssetID().c_str(), kLastKnown.c_str()));
        kResolvedLocation = kLastKnown;
    }
    
    if (kResolvedLocation.empty())
        kResolvedLocation = NiSurface::MISSING_TEXTURE_PATH;

    // Lets find the texture slot on this surface
    efd::utf8string kAssetID = pkReference->GetAssetID();
    for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
    {
        NiSurface::TextureSlotEntry* pkSlot = 
            pkSurface->GetTextureSlotEntry((NiSurface::SurfaceMapID)uiSlot);
        if (!pkSlot)
            continue;

        // First, match on asset llid. Otherwise, use the uri that we requested.
        if (kAssetID == pkSlot->m_kAssetId)
        {
            // Update the filepath of this texture with the resolved one
            pkSlot->m_kFilePath = kResolvedLocation.c_str();
            // Update the assetID of this texture with the resolved one
            pkSlot->m_kAssetId = pkReference->GetResolvedAssetID();
        }
    }

    // Check the surface to see if it is resolved
    bool bResolved = true;
    for (efd::UInt32 uiSlot = 0; uiSlot < NiSurface::NUM_SURFACE_MAPS; uiSlot++)
    {
        const NiSurface::TextureSlotEntry* pkSlot = pkSurface->GetTextureSlotEntry(
            (NiSurface::SurfaceMapID)uiSlot);
        if (!pkSlot)
            continue;

        if (pkSlot->m_kFilePath.IsEmpty())
        {
            bResolved = false;
            break;
        }
    }

    if (bResolved)
    {
        // Mark the surface with it's resolution status
        pkSurface->MarkResolved(true);

        // Check the package to see if it has been completely resolved
        NiSurfacePackage* pkPackage = pkSurface->GetPackage();

        // Check all the surfaces for missing textures
        bool bPackageResolved = true;
        NiTPrimitiveSet<NiSurface*> kSurfaces;
        pkPackage->GetSurfaces(kSurfaces);
        efd::UInt32 uiNumSurfaces = kSurfaces.GetSize();
        for (efd::UInt32 uiIndex = 0; uiIndex < uiNumSurfaces; uiIndex++)
        {
            NiSurface* pkCurSurface = kSurfaces.GetAt(uiIndex);
            EE_ASSERT(pkCurSurface);

            // If any surface is not resolved, the package cannot be resolved, so our work is 
            // done.
            if (!pkCurSurface->IsResolved())
            {
                bPackageResolved = false;
                break;
            }
        }

        if (bPackageResolved)
        {
            // Loading of the package is done. 
            // Add it to the library
            AddPackage(pkPackage);
        }
    }

    m_kPendingTextureReferences.erase(pkReference);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFile* NiTerrainSurfaceLibrary::OpenPackageFile(
    const efd::utf8string& kFilename, bool bWrite)
{
    // Figure out which callback to execute
    StoragePolicy* pkStoragePolicy = GetStoragePolicy();
    return pkStoragePolicy->OpenPackageFile(kFilename, bWrite);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfaceLibrary::StoragePolicy::StoragePolicy()
{
    m_spPackageReadFormat = EE_NEW NiTerrainSurfacePackageFileFormatRegistry();
    m_spPackageWriteFormat = m_spPackageReadFormat;

    m_spPackageReadFormat->AddFileFormat<NiTerrainSurfacePackageFileVersion0>(0);
    m_spPackageReadFormat->AddFileFormat<NiTerrainSurfacePackageFileVersion1>(1);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfacePackageFile* NiTerrainSurfaceLibrary::StoragePolicy::OpenPackageFile(
    const efd::utf8string& kFilename, bool bWriteAccess)
{
    NiTerrainSurfacePackageFileFormatRegistry* pkFormat = 
        (bWriteAccess) ? (m_spPackageWriteFormat) : (m_spPackageReadFormat);
    EE_ASSERT(pkFormat);

    NiTerrainSurfacePackageFile::FileIdentifier kFileID;
    kFileID.m_kPackageFile = kFilename;
    kFileID.m_pkStoragePolicy = this;
    return pkFormat->OpenFile(kFileID, bWriteAccess);
}

//--------------------------------------------------------------------------------------------------