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


#include "egfPCH.h"

#include <egf/SchedulerQueue.h>


using namespace egf;


//-------------------------------------------------------------------------------------------------
void SchedulerQueue::push(ScheduledTask* pTask)
{
    // Search the queue for the correct place (execution time) to insert this new behavior
    bool bInserted = false;
    for (efd::list<ScheduledTask*>::reverse_iterator iter = m_priorityQueue.rbegin();
          iter != m_priorityQueue.rend();
          ++iter)
    {
        ScheduledTask* pInsertPend = *iter;
        if (pInsertPend->GetExecuteTime() <= pTask->GetExecuteTime())
        {
            m_priorityQueue.insert(iter.base(), pTask);
            bInserted = true;
            break;
        }
    }
    if (!bInserted)
    {
        m_priorityQueue.push_front(pTask);
    }

    m_eventMap[ pTask->GetEventID() ] = pTask;

    pTask->IncRefCount();
}

//-------------------------------------------------------------------------------------------------
void SchedulerQueue::pop()
{
    ScheduledTask* pFront = m_priorityQueue.front();
    m_eventMap.erase(pFront->GetEventID());
    m_priorityQueue.pop_front();
    pFront->DecRefCount();
}

//-------------------------------------------------------------------------------------------------
ScheduledTask* SchedulerQueue::find(egf::EventID evid)
{
    efd::map<EventID, ScheduledTask*>::iterator iter = m_eventMap.find(evid);
    if (iter != m_eventMap.end())
    {
        return iter->second;
    }
    return NULL;
}

//-------------------------------------------------------------------------------------------------
const ScheduledTask* SchedulerQueue::find(egf::EventID evid) const
{
    efd::map<EventID, ScheduledTask*>::const_iterator iter = m_eventMap.find(evid);
    if (iter != m_eventMap.end())
    {
        return iter->second;
    }
    return NULL;
}

//-------------------------------------------------------------------------------------------------
bool SchedulerQueue::erase(egf::EventID evid)
{
    efd::map<EventID, ScheduledTask*>::iterator iter = m_eventMap.find(evid);
    if (iter != m_eventMap.end())
    {
        ScheduledTask* pTask = iter->second;
        m_priorityQueue.remove(pTask);
        m_eventMap.erase(iter);
        pTask->DecRefCount();
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------
bool SchedulerQueue::erase(egf::EventID i_eventID, ScheduledTaskPtr& o_pTask)
{
    efd::map<EventID, ScheduledTask*>::iterator iter = m_eventMap.find(i_eventID);
    if (iter != m_eventMap.end())
    {
        ScheduledTask* pTask = iter->second;
        m_priorityQueue.remove(pTask);
        m_eventMap.erase(iter);
        o_pTask = pTask;
        pTask->DecRefCount();
        return true;
    }
    o_pTask = NULL;
    return false;
}

