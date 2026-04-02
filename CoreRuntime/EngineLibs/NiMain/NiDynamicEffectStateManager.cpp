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

// Precompiled Header
#include "NiMainPCH.h"

#include "NiDynamicEffectStateManager.h"
#include "NiDynamicEffectState.h"

NiDynamicEffectStateManager*
    NiDynamicEffectStateManager::ms_pkDynamicEffectStateManager = NULL;

//--------------------------------------------------------------------------------------------------
void NiDynamicEffectStateManager::_SDMInit()
{
    ms_pkDynamicEffectStateManager = NiNew NiDynamicEffectStateManager();
}

//--------------------------------------------------------------------------------------------------
void NiDynamicEffectStateManager::_SDMShutdown()
{
    NiDelete ms_pkDynamicEffectStateManager;
}

//--------------------------------------------------------------------------------------------------
NiDynamicEffectStateManager::NiDynamicEffectStateManager()
{
    m_pkEffectStateMap =
        NiNew NiTPointerMap<NiUInt32, ListEntry*>(DEFAULT_MAP_SIZE);
    EE_ASSERT(m_pkEffectStateMap);

    m_pkListEntryPool =
        NiNew NiTObjectPool<ListEntry>(DEFAULT_LIST_ENTRY_POOL_SIZE);
    EE_ASSERT(m_pkListEntryPool);
}

//--------------------------------------------------------------------------------------------------
NiDynamicEffectStateManager::~NiDynamicEffectStateManager()
{
    NiDelete m_pkEffectStateMap;

    m_pkListEntryPool->PurgeAllObjects();
    NiDelete m_pkListEntryPool;
}

//--------------------------------------------------------------------------------------------------
void NiDynamicEffectStateManager::LockManager()
{
    m_kEffectStateCriticalSection.Lock();
}

//--------------------------------------------------------------------------------------------------
void NiDynamicEffectStateManager::UnlockManager()
{
    m_kEffectStateCriticalSection.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiDynamicEffectStateManager::AddDynamicEffectState(NiDynamicEffectStatePtr& spEffectState)
{
    if (!spEffectState)
        return;

    NiUInt32 uiHashKey = spEffectState->GetHashKey();
    ListEntry* pkEntry = NULL;

    // Note it is important that all operations with spEffectState that may result in
    // smart pointer delete operations must occur inside the critical section lock. This
    // prevents potential effect state double deletions that can occur from a different thread.
    m_kEffectStateCriticalSection.Lock();

    // Check to see if an entry already exists with the same key.
    m_pkEffectStateMap->GetAt(uiHashKey, pkEntry);

    if (!pkEntry)
    {
        // No other entry contain the same key.
        pkEntry = m_pkListEntryPool->GetFreeObject();
        EE_ASSERT(pkEntry);

        pkEntry->m_pkNext = NULL;
        pkEntry->m_pkEffectState = spEffectState;
        m_pkEffectStateMap->SetAt(uiHashKey, pkEntry);

        m_kEffectStateCriticalSection.Unlock();
        return;
    }

    // An set of entries was found with the same key. Iterate through
    // the list to deterimine if any of those entries are an exact match.
    while (pkEntry)
    {
        if (pkEntry->m_pkEffectState->Equal(spEffectState))
        {
            // An identical effect state already exists.
            spEffectState = pkEntry->m_pkEffectState;
            m_kEffectStateCriticalSection.Unlock();
            return;
        }

        if (!pkEntry->m_pkNext)
            break;

        pkEntry = pkEntry->m_pkNext;
    }

    // The provided NiDynamicEffectState does not already exist in the map,
    // so add it to the map.
    pkEntry->m_pkNext = m_pkListEntryPool->GetFreeObject();
    pkEntry = pkEntry->m_pkNext;

    pkEntry->m_pkNext = NULL;
    pkEntry->m_pkEffectState = spEffectState;

    m_kEffectStateCriticalSection.Unlock();
}

//--------------------------------------------------------------------------------------------------
bool NiDynamicEffectStateManager::RemoveDynamicEffectState(
    NiDynamicEffectState* pkEffectState)
{
    if (!pkEffectState)
        return false;

    NiUInt32 uiHashKey = pkEffectState->GetHashKey();

    m_kEffectStateCriticalSection.Lock();

    ListEntry* pkPrevEntry = NULL;
    ListEntry* pkEntry = NULL;
    m_pkEffectStateMap->GetAt(uiHashKey, pkEntry);

    while (pkEntry)
    {
        if (pkEntry->m_pkEffectState == pkEffectState)
        {
            if (pkPrevEntry)
            {
                pkPrevEntry->m_pkNext = pkEntry->m_pkNext;
            }
            else if (!pkEntry->m_pkNext)
            {
                // Handle case were pkEntry is the only entry in the list.
                m_pkEffectStateMap->RemoveAt(uiHashKey);         
            }
            else
            {
                // Handle case were pkEntry is the first entry in list.
                m_pkEffectStateMap->SetAt(uiHashKey, pkEntry->m_pkNext);
            }

            m_pkListEntryPool->ReleaseObject(pkEntry);

            m_kEffectStateCriticalSection.Unlock();
            return true;
        }

        pkPrevEntry = pkEntry;
        pkEntry = pkEntry->m_pkNext;
    }

    // Effect State not found.
    m_kEffectStateCriticalSection.Unlock();
    return false;
}

//--------------------------------------------------------------------------------------------------
