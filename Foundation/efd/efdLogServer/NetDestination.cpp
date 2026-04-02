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

#include "efdLogServicePCH.h"

#include <stdio.h>
#include <stdarg.h>
#include <efdLogServer/NetDestination.h>
#include <efdLogServer/LogServiceMessages.h>
#include <efd/MessageService.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetDestination);


//------------------------------------------------------------------------------------------------
NetDestination::NetDestination(const efd::utf8string& name,
                               const efd::Category& category,
                               MessageService* pMessageService,
                               efd::UInt32 highwatermark,
                               bool fileWithMsg,
                               bool fileWithAssert)
    : ILogDestination(name)
    , m_category(category)
    , m_fileWithMsg(fileWithMsg)
    , m_fileWithAssert(fileWithAssert)
    , m_pid(0)
    , m_spMessageService(pMessageService)
{
    m_buffer.reserve(highwatermark);
    EE_ASSERT(m_spMessageService);
}


//------------------------------------------------------------------------------------------------
NetDestination::~NetDestination()
{
}


//------------------------------------------------------------------------------------------------
bool NetDestination::OnInit()
{
    m_pid = efd::GetPid();
    return true;
}


//------------------------------------------------------------------------------------------------
void NetDestination::Publish()
{
    if (m_category.IsValid())
    {
        m_bufferLock.Lock();

        // don't send out too much data per message. There's a limit on the size of an outgoing
        // message. Chunk up the response accordingly. For now, limit each message to 7168 bytes to
        // avoid the size limits, which are around 8k.
        int totalBytes = 0;
        LogMemBuffer chunk;
        for (LogMemBuffer::const_iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
        {
            if (totalBytes > 7168)
            {
                totalBytes = 0;
                IMessagePtr response = EE_NEW LogEntriesMessage(m_spMessageService->GetNetID(),
                    m_pid, chunk);
                m_spMessageService->Send(response, m_category);
                chunk.clear();
            }
            chunk.push_back(*it);
            totalBytes += (int)it->size() * sizeof(char);
        }

        // send anything left over...
        if (totalBytes > 0)
        {
            IMessagePtr response = EE_NEW LogEntriesMessage(m_spMessageService->GetNetID(), m_pid,
                chunk);
            m_spMessageService->Send(response, m_category);
        }

        m_buffer.clear();

        m_bufferLock.Unlock();
    }
}


//------------------------------------------------------------------------------------------------
void NetDestination::BeginLog(bool assert,
                              const char* timeStampMachine,
                              efd::TimeType timeStampGame,
                              const char* module,
                              const char* level,
                              const char *file,
                              efd::SInt32 line)
{
    m_bufferLock.Lock();

    // |time|level|module|file|line|msg|
    efd::utf8string entry = "|";

    entry.append(timeStampMachine);
    entry.append("|");
    entry.append(level);
    entry.append("|");
    entry.append(module);
    entry.append("|");

    if ((assert && m_fileWithAssert) || (!assert && m_fileWithMsg))
    {
        entry.append(file);
        entry.append("|");
        entry.append(line);
        entry.append("|");
    }
    else
    {
        entry.append("||");
    }

    m_buffer.push_back(entry);

    EE_UNUSED_ARG(timeStampGame);
}

//------------------------------------------------------------------------------------------------
void NetDestination::ContinueLog(const char* pMsg)
{
    utf8string& currentLog = m_buffer.back();
    currentLog.append(pMsg);
}

//------------------------------------------------------------------------------------------------
void NetDestination::EndLog()
{
    utf8string& currentLog = m_buffer.back();
    currentLog.append("|");

    m_bufferLock.Unlock();
}

//------------------------------------------------------------------------------------------------
void NetDestination::Flush()
{
    Publish();
}
