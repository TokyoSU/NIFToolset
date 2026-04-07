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

#include "NetMetricsDestination.h"

#include <efd/File.h>
#include <efd/StringUtilities.h>

using namespace efd;


const efd::UInt8 kNETMET_NetworkSendBytes = 1;
const efd::UInt8 kNETMET_NetworkReceiveBytes = 2;
const efd::UInt8 kNETMET_NetworkSendMessage = 3;
const efd::UInt8 kNETMET_NetworkReceiveMessage = 4;
const efd::UInt8 kNETMET_NetworkEnvelopeHeader = 5;
const efd::UInt8 kNETMET_NetworkEnvelopeTotal = 6;
const efd::UInt8 kNETMET_ReplicationDiscoverySent = 7;
const efd::UInt8 kNETMET_ReplicationUpdateSent = 8;
const efd::UInt8 kNETMET_ReplicationLossSent = 9;
const efd::UInt8 kNETMET_ReplicationDiscoveryReceived = 10;
const efd::UInt8 kNETMET_ReplicationUpdateReceived = 11;
const efd::UInt8 kNETMET_ReplicationLossReceived = 12;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetMetricsDestination);

//------------------------------------------------------------------------------------------------
NetMetricsDestination::NetMetricsDestination(const efd::utf8string& name,
    const efd::utf8string& fileName)
    : ILogDestination(name)
    , m_startTime(-1.0f)
    , m_finishTime(-1.0f)
    , m_sentBytesTotal(0)
    , m_receivedBytesTotal(0)
    , m_sentMessageCount(0)
    , m_receivedMessageCount(0)
    , m_networkEnvelopeHeader(0)
    , m_networkEnvelopeTotal(0)
    , m_replicationMessagesSent(0)
    , m_replicationMessagesReceived(0)
    , m_fileName(fileName)
{
    m_uniqueIndex.AquireIndex(m_fileName);
}

// Destructor implementation
NetMetricsDestination::~NetMetricsDestination()
{
    // Make the filename unique to the process instance
    IndexedFileName(m_fileName);

    // Write our data to the summary file
    efd::File *pLogFile = efd::File::GetFile(m_fileName.c_str(), efd::File::WRITE_ONLY_TEXT, 1024,
        false);

    TimeType elapsed = m_finishTime - m_startTime;
    char msg[512];

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total sent bytes,%u\n", m_sentBytesTotal);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total received bytes,%u\n", m_receivedBytesTotal);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Elapsed seconds,%.2f\n", elapsed);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Average sent bytes/sec,%.2f\n",
        (m_sentBytesTotal/elapsed));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Average received bytes/sec,%.2f\n",
        (m_receivedBytesTotal/elapsed));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total sent messages,%u\n",
        m_sentMessageCount);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total received messages,%u\n",
        m_receivedMessageCount);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Average sent messages/sec,%.2f\n",
        (m_sentMessageCount/elapsed));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Average received messages/sec,%.2f\n",
        (m_receivedMessageCount/elapsed));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total envelope header size,%u\n",
        m_networkEnvelopeHeader);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total envelope message size,%u\n",
        m_networkEnvelopeTotal);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Envelope overhead,%.2f%%\n",
        (m_networkEnvelopeHeader * 100.0)/m_networkEnvelopeTotal);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total replication messages sent,%u\n",
        (m_replicationMessagesSent));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Replication as %% of overall sent messages,%.2f%%\n",
        (m_replicationMessagesSent * 100.0)/m_sentMessageCount);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE, "Total replication messages received,%u\n",
        (m_replicationMessagesReceived));
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    efd::Snprintf(&msg[0], 512, EE_TRUNCATE,
        "Replication as %% of overall received messages,%.2f%%\n",
        (m_replicationMessagesReceived * 100.0)/m_receivedMessageCount);
    pLogFile->Write(msg, efd::Strlen(&msg[0]));

    pLogFile->Flush();
    delete pLogFile;

    m_uniqueIndex.ReleaseIndex();
}

efd::Bool NetMetricsDestination::OnInit()
{
    m_metricsKeyMap["NETWORK.SEND.BYTES"] = kNETMET_NetworkSendBytes;
    m_metricsKeyMap["NETWORK.RECEIVE.BYTES"] = kNETMET_NetworkReceiveBytes;
    m_metricsKeyMap["NETWORK.SEND.MESSAGE"] = kNETMET_NetworkSendMessage;
    m_metricsKeyMap["NETWORK.RECEIVE.MESSAGE"] = kNETMET_NetworkReceiveMessage;
    m_metricsKeyMap["NETWORK.ENVELOPE.HEADER"] = kNETMET_NetworkEnvelopeHeader;
    m_metricsKeyMap["NETWORK.ENVELOPE.TOTAL"] = kNETMET_NetworkEnvelopeTotal;
    m_metricsKeyMap["REPLICATIONSERVICE.SEND.ENTITY_DISCOVER"] = kNETMET_ReplicationDiscoverySent;
    m_metricsKeyMap["REPLICATIONSERVICE.SEND.ENTITY_UPDATE"] = kNETMET_ReplicationUpdateSent;
    m_metricsKeyMap["REPLICATIONSERVICE.SEND.ENTITY_LOSS"] = kNETMET_ReplicationLossSent;
    m_metricsKeyMap["REPLICATIONSERVICE.RECEIVE.ENTITY_DISCOVER"] =
        kNETMET_ReplicationDiscoveryReceived;
    m_metricsKeyMap["REPLICATIONSERVICE.RECEIVE.ENTITY_UPDATE"] = kNETMET_ReplicationUpdateReceived;
    m_metricsKeyMap["REPLICATIONSERVICE.RECEIVE.ENTITY_LOSS"] = kNETMET_ReplicationLossReceived;

    return true;
}

void NetMetricsDestination::LogMessage(
        efd::Bool assert,
        const char* timeStampMachine,
        efd::TimeType timeStampGame,
        const char* pModule,
        const char* pLevel,
        const char* pFile,
        efd::SInt32 line,
        const char* pMsg)
{
    EE_UNUSED_ARG(assert);
    EE_UNUSED_ARG(timeStampMachine);
    EE_UNUSED_ARG(pLevel);
    EE_UNUSED_ARG(pFile);
    EE_UNUSED_ARG(line);

    // We only care about messages sent to the metrics module, which is either "Metrics" or "25"
    // depending on whether the enum manager has been initialized yet.
    utf8string module(pModule);
    if (module != "Metrics" && module != "25")
    {
        return;
    }

    // If it's a message we are interested in, update timestamps appropriately. We start the timer
    // when we receive the first relevant message, in order to eliminate setup time as a factor in
    // our time-based calculations.
    if (m_startTime == -1.0f)
    {
        m_startTime = timeStampGame;
    }

    // Need a max() here because log messages might be sent after the scheduler shuts down & deletes
    // the game time clock
    m_finishTime = timeStampGame > m_finishTime ? timeStampGame : m_finishTime;

    // Extract the key from the message by cutting it down to the third dot, which should remove the
    // "tag" -- the QoS and the connection ID
    efd::utf8string msg(pMsg);
    efd::utf8string::size_type pos = 0;
    efd::UInt8 count = 3;

    while ((count > 0) && (pos != efd::utf8string::npos))
    {
        pos = msg.find('.', pos + 1);
        --count;
    }

    // If the probe name doesn't have three dots in it, use the whole probe name as the key.
    efd::utf8string key;
    if (pos == efd::utf8string::npos)
    {
        pos = msg.find(" = ");

        // The message doesn't match our format expectations, and is thus unlikely to be one we care
        // about. Bail.
        if (pos == efd::utf8string::npos)
        {
            return;
        }
    }

    key = msg.substr(0, pos);

    // See whether the key is one that we care about. If not, exit out
    efd::UInt8 index(0);
    if (!m_metricsKeyMap.find(key, index))
    {
        return;
    }

    // Extract the value by splitting on " = ". If that fails, exit out.
    efd::vector<utf8string> results;
    efd::UInt32 splitCount = msg.split(" = ", results);

    if (splitCount == 0)
    {
        return;
    }

    efd::UInt32 value = efd::strtol(results.back().c_str(), NULL);

    // Depending on which metric this is, we handle the value differently
    switch (index)
    {
        case kNETMET_NetworkSendBytes:
            m_sentBytesTotal += value;
            break;

        case kNETMET_NetworkReceiveBytes:
            m_receivedBytesTotal += value;
            break;

        case kNETMET_NetworkSendMessage:
            ++m_sentMessageCount;
            break;

        case kNETMET_NetworkReceiveMessage:
            ++m_receivedMessageCount;
            break;

        case kNETMET_NetworkEnvelopeHeader:
            m_networkEnvelopeHeader += value;
            break;

        case kNETMET_NetworkEnvelopeTotal:
            m_networkEnvelopeTotal += value;
            break;

        case kNETMET_ReplicationDiscoverySent:
        case kNETMET_ReplicationUpdateSent:
        case kNETMET_ReplicationLossSent:
            ++m_replicationMessagesSent;
            break;

        case kNETMET_ReplicationDiscoveryReceived:
        case kNETMET_ReplicationUpdateReceived:
        case kNETMET_ReplicationLossReceived:
            ++m_replicationMessagesReceived;
            break;

        default:
            EE_FAIL("Unhandled net metrics probe");
    }
}


