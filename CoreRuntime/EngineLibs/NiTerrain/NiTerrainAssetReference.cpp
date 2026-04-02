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
#include "NiTerrainAssetReference.h"

//--------------------------------------------------------------------------------------------------
const char* NiTerrainAssetReference::URN_FORCE_LOOKUP = "FORCE_LOOKUP:";
//--------------------------------------------------------------------------------------------------
NiTerrainAssetReference::NiTerrainAssetReference()
    : m_bIsResolved(false)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainAssetReference::NiTerrainAssetReference(const NiTerrainAssetReference&)
    : NiRefObject()
    , NiTerrainObservable<NiTerrainAssetReference>()
{
    // Do not allow the copy constructor to be invoked
    EE_FAIL("Invalid copy constructor");
}

//--------------------------------------------------------------------------------------------------
NiTerrainAssetReference::~NiTerrainAssetReference()
{
}

//-------------------------------------------------------------------------------------------------
NiTerrainAssetReference& NiTerrainAssetReference::operator=(const NiTerrainAssetReference& kOther)
{
    m_kReferingAssetLocation = kOther.m_kReferingAssetLocation;
    m_kRelativeAssetLocation = kOther.m_kRelativeAssetLocation;
    m_kAssetID = kOther.m_kAssetID;
    m_kResolvedLocation = kOther.m_kResolvedLocation;
    m_bIsResolved = kOther.m_bIsResolved;

    return *this;
}

//-------------------------------------------------------------------------------------------------
efd::utf8string NiTerrainAssetReference::GetLastRelativeLocation() const
{
    // Work out the relative location of this asset
    efd::utf8string kAbsPath = GetResolvedLocation();
    efd::utf8string kRelPath = GetRelativeAssetLocation();

    if (!kAbsPath.empty())
    {
        // Convert to a relative path:
        efd::utf8string kRelTo = efd::PathUtils::PathRemoveFileName(m_kReferingAssetLocation);
        size_t uiBufferSize = kAbsPath.length() * 2;
        efd::Char* pucRelPath = EE_ALLOC(efd::Char, uiBufferSize);
        // NULL terminate the string now incase the conversion fails
        pucRelPath[0] = 0;
        efd::PathUtils::ConvertToRelative(pucRelPath, uiBufferSize, kAbsPath.c_str(), kRelTo.c_str());
        kRelPath = pucRelPath;
        EE_FREE(pucRelPath);
    }

    return kRelPath;
}

//-------------------------------------------------------------------------------------------------
void NiTerrainAssetReference::OnResolved()
{
    RaiseEvent(0);
}

//-------------------------------------------------------------------------------------------------
NiTerrainAssetResolverBase::~NiTerrainAssetResolverBase()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainAssetResolverBase::ResolveAssetLocation(NiTerrainAssetReference* pkReference)
{   
    NiTerrainAssetReferencePtr spReference = pkReference;
    if (spReference->IsResolved())
    {
        spReference->MarkResolved(true);
    }
    else
    {
        InternalResolveReference(spReference);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainAssetResolverDefault::InternalResolveReference(NiTerrainAssetReference* pkReference)
{
    pkReference->SetResolvedLocation(pkReference->GetStoredLocation());
    pkReference->MarkResolved(true);
}
//--------------------------------------------------------------------------------------------------