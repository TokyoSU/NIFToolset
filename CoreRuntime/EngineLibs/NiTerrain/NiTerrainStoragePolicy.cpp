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
#include "NiTerrainStoragePolicy.h"

//--------------------------------------------------------------------------------------------------
NiImplementRootRTTI(NiTerrainStoragePolicy);
//--------------------------------------------------------------------------------------------------
NiTerrainStoragePolicy::NiTerrainStoragePolicy()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainStoragePolicy::~NiTerrainStoragePolicy()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStoragePolicy::ResolveAssetReference(NiTerrainAssetReference* pkReference)
{
    NiTerrainAssetResolverBase* pkResolver = GetAssetResolver();
    if (pkResolver)
    {
        pkResolver->ResolveAssetLocation(pkReference);
    }
    else
    {
        pkReference->MarkResolved(true);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStoragePolicy::SetAssetResolver(NiTerrainAssetResolverBase* pkResolver)
{
    m_spAssetResolver = pkResolver;
}

//--------------------------------------------------------------------------------------------------
NiTerrainAssetResolverBase* NiTerrainStoragePolicy::GetAssetResolver()
{
    if (!m_spAssetResolver)
    {
        m_spAssetResolver = NiNew NiTerrainAssetResolverDefault();
    }
    return m_spAssetResolver;
}

//--------------------------------------------------------------------------------------------------
NiTerrainStoragePolicy::OpeningEventArgs::OpeningEventArgs(efd::utf8string kFilename, bool bWrite)
    : m_kFilename(kFilename)
    , m_bWriteAccess(bWrite)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainStoragePolicy::ClosedEventArgs::ClosedEventArgs(efd::utf8string kFilename, 
    IOSuccessCode::Value eSuccess)
    : m_kFilename(kFilename)
    , m_eSuccess(eSuccess)
{
}

//--------------------------------------------------------------------------------------------------
