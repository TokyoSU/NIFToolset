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

#include "efdPCH.h"

#include <efd/AssetLoadRequest.h>
#include <efd/AssetLoadResponse.h>
#include <efd/SmartCriticalSection.h>
#include <efd/BaseAssetFactory.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
IAssetResponseData::IAssetResponseData()
    : m_spResponse(0)
    , m_isForeground(false)
{
}

//------------------------------------------------------------------------------------------------
BaseAssetFactory::BaseAssetFactory()
    : m_pendingRequestLock()
    , m_pendingRequestMap()
    , m_subscribed(false)
{
}

//------------------------------------------------------------------------------------------------
BaseAssetFactory::~BaseAssetFactory()
{
    // even though ~BaseAssetFactory is pure virtual, we still need to provide an implementation.
    m_pendingRequestMap.clear();
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus BaseAssetFactory::GetResponseData(
    const AssetLoadRequest *pRequest,
    IAssetResponseData*& ppResponse)
{
    IAssetFactory::LoadStatus status = IAssetFactory::LOAD_RUNNING;
    ppResponse = 0;

    SmartCriticalSection cs(m_pendingRequestLock);
    cs.Lock();

    PendingRequestMap::iterator it = m_pendingRequestMap.find(pRequest);
    if (it != m_pendingRequestMap.end())
    {
        ppResponse = it->second;
    }
    else
    {
        // See if a different request is already loading the same asset.
        for (it = m_pendingRequestMap.begin(); it != m_pendingRequestMap.end(); ++it)
        {
            const AssetLoadRequest* pExistingRequest = it->first;

            // If we are already processing a request for this asset, block until the
            // previous request completes to avoid sharing violations.
            if (pExistingRequest->GetAssetPath() == pRequest->GetAssetPath())
            {
                status = IAssetFactory::LOAD_BLOCKED;
            }
        }
        // Seed our request map with the current request. We indicate here that we've seen
        // the request but we haven't started processing it by setting the IAssetResponseData
        // to 0. This is required to avoid sharing violations if the same URN is loaded
        // from different threads. If we did not seed our map, the handler might attempt to
        // load the same asset from disk in each thread simultaneously. An actual value
        // is set for the ResponseData for this request when actually reading data from disk.
        m_pendingRequestMap[pRequest] = 0;
    }

    return status;
}

//------------------------------------------------------------------------------------------------
void BaseAssetFactory::RemoveResponseData(const efd::AssetLoadRequest* pRequest)
{
    SmartCriticalSection cs(m_pendingRequestLock);
    cs.Lock();
    m_pendingRequestMap.erase(pRequest);
}
