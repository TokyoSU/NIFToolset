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

#include "NiSurfacePackage.h"
#include "NiSurface.h"
#include "NiTerrainXMLHelpers.h"

using namespace efd;

//--------------------------------------------------------------------------------------------------
NiSurfacePackage::NiSurfacePackage() 
    : m_kPackageName(NULL)
    , m_kFileSrc(NULL)
    , m_uiIteration(0)
    , m_bDirty(false)
    , m_pkLibrary(NULL)
{

}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage::NiSurfacePackage(const NiString& kPackageName, NiTerrainSurfaceLibrary* pkLibrary) 
    : m_kPackageName(kPackageName)
    , m_kFileSrc(NULL)
    , m_uiIteration(0)
    , m_bDirty(false)
    , m_pkLibrary(pkLibrary)
{
}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage::~NiSurfacePackage()
{
    UnloadSurfaces();
}

//--------------------------------------------------------------------------------------------------
NiSurfacePackage* NiSurfacePackage::Create(const NiString& kPackageName, 
    NiTerrainSurfaceLibrary* pkLibrary)
{
    return NiNew NiSurfacePackage(kPackageName, pkLibrary);
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::PrecompileSurfaces()
{
    if (!m_kSurfaces.GetCount())
        return false;

    NiTMapIterator kIterator = m_kSurfaces.GetFirstPos();
    NiSurface* pkCurrentSurface;
    const char* pcKey;

    while (kIterator)
    {
        m_kSurfaces.GetNext(kIterator, pcKey, pkCurrentSurface);
        pkCurrentSurface->CompileSurface();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
const char* NiSurfacePackage::GetFilename() const
{
    return m_kFileSrc;
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::SetFileName(const NiString& kFilePath)
{
    m_kFileSrc = kFilePath;
}

//--------------------------------------------------------------------------------------------------
const char* NiSurfacePackage::GetAssetID() const
{
    return m_kAssetID;
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::SetAssetID(const NiString& kAssetID)
{
    m_kAssetID = kAssetID;
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiSurfacePackage::GetIteration() const
{
    return m_uiIteration + (m_bDirty ? 1 : 0);
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::MarkDirty(bool bDirty)
{
    if (m_bDirty && !bDirty)
    {
        // Increment the iteration count of this package
        m_uiIteration++;
    }

    m_bDirty = bDirty;
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::IsDirty() const
{
    return m_bDirty;
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::SetIteration(efd::UInt32 uiIteration)
{
    m_uiIteration = uiIteration;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSurfaceLibrary* NiSurfacePackage::GetLibrary()
{
    return m_pkLibrary;
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::RenameSurface(NiSurface* pkSurface,
        const NiString& kNewSurfaceName)
{
    NiSurface *pkExistingSurface;
    NiString kOldSurfaceName = pkSurface->GetName();

    // Check that no surfaces exist with the new name
    if (GetSurface(kNewSurfaceName, pkExistingSurface))
        return false;

    // Check that the surface is contained within this package to begin with
    GetSurface(pkSurface->GetName(), pkExistingSurface);
    if (pkExistingSurface != pkSurface)
        return false;

    // Map this surface to the new name
    m_kSurfaces.SetAt(kNewSurfaceName, pkSurface);
    pkSurface->SetName(kNewSurfaceName);

    // Remove the surface from the old name
    m_kSurfaces.RemoveAt(kOldSurfaceName);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::ClaimSurface(NiSurface* pkSurface)
{
    EE_ASSERT(pkSurface);

    NiSurfacePackage *pkOldPackage = pkSurface->GetPackage();
    NiSurfacePackage *pkNewPackage = this;

    // Check that these packages aren't't the same ones!
    if (pkOldPackage == pkNewPackage)
        return true;

    // Check that there doesn't already exist a surface by this name...
    NiSurface *pkExistingSurface;
    if (GetSurface(pkSurface->GetName(), pkExistingSurface))
    {
        // A surface by this name already exists in this package
        return false;
    }

    // Add this surface to this package:
    pkNewPackage->m_kSurfaces.SetAt(pkSurface->GetName(), pkSurface);
    pkSurface->SetPackage(pkNewPackage);

    // Remove this surface from the old package:
    if (pkOldPackage)
        pkOldPackage->m_kSurfaces.RemoveAt(pkSurface->GetName());

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::ReleaseSurface(NiSurface* pkSurface)
{
    NiSurface* pkFoundSurface;
    if (GetSurface(pkSurface->GetName(), pkFoundSurface))
    {
        if (pkFoundSurface == pkSurface)
        {
            m_kSurfaces.RemoveAt(pkSurface->GetName());
            pkSurface->SetPackage(0);
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::UnloadSurface(const NiString& kName)
{
    NiSurface* pkSurface;
    if (GetSurface(kName, pkSurface))
    {
        m_kSurfaces.RemoveAt(kName);
        NiDelete pkSurface;
    }
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::UnloadSurfaces()
{
    NiTMapIterator kIterator = m_kSurfaces.GetFirstPos();
    NiSurface* pkCurrentSurface;
    const char* pcKey;

    while (kIterator)
    {
        m_kSurfaces.GetNext(kIterator, pcKey, pkCurrentSurface);
        m_kSurfaces.RemoveAt(pcKey);
        NiDelete pkCurrentSurface;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::CreateSurface(const NiString& kName,
    NiSurface*& pkSurface)
{
    // Make sure that no surfaces already exist by that name
    if (GetSurface(kName, pkSurface))
    {
        pkSurface = 0;
        return false;
    }

    pkSurface = NiNew NiSurface();
    pkSurface->SetName(kName);
    pkSurface->SetPackage(this);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiSurfacePackage::GetSurface(const NiString& kName, NiSurface*& pkSurface) const
{
    if (m_kSurfaces.GetAt(kName, pkSurface))
    {
        return true;
    }
    else
    {
        pkSurface = NULL;
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiSurfacePackage::GetSurfaces(NiTPrimitiveSet<NiSurface*>& kSurfaces) const
{
    NiTMapIterator kIterator = m_kSurfaces.GetFirstPos();
    NiSurface* pkCurrentSurface;
    const char* pcKey;
    
    while (kIterator)
    {
        m_kSurfaces.GetNext(kIterator, pcKey, pkCurrentSurface);
        kSurfaces.Add(pkCurrentSurface);
    }
}

//--------------------------------------------------------------------------------------------------
const NiString& NiSurfacePackage::GetName() const
{
    return m_kPackageName;
}
