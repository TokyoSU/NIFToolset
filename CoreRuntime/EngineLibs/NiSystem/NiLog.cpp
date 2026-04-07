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
#include "NiSystemPCH.h"

#include "NiLog.h"
#include "NiSystem.h"

#include <efd/ecrLogIDs.h>
#include <efd/Logger.h>
#include <efd/FileDestination.h>
#include <efd/SystemLogger.h>
#include <efd/DebugOutDestination.h>

char NiLogger::ms_acBuffer[MAX_OUTPUT_LENGTH];
NiLogMessageOptions NiLogger::ms_akMessageOptions[NIMESSAGE_MAX_TYPES];
NiUInt16 NiLogger::ms_akLogIDMap[NIMESSAGE_MAX_TYPES];

// dwest: extracted from class to reduce include dependencies.
static efd::SmartPointer<efd::FileDestination> ms_akLogFile[NiLogger::MAX_NUM_LOGFILES];
static efd::FileDestination::FileOption ms_akOpenModeMap[NiLogger::OPEN_MODE_MAX];

unsigned int NiLogger::ms_uiLogFileCount = 0;
efd::FastCriticalSection NiLogger::ms_kCriticalSection;

//--------------------------------------------------------------------------------------------------
NiLogMessageOptions::NiLogMessageOptions() : m_bOutputToDebugWindow(false),
    m_iLogID(-1), m_bPrependTimestamp (false)
{
}

//--------------------------------------------------------------------------------------------------
NiLogger::NiLogger(int iMessageType, const char* pcFormat, ...)
    : m_iMessageType(iMessageType)
{
    if (OkayToOutput() && pcFormat)
    {
        ms_kCriticalSection.Lock();

        va_list kArgList;
        va_start(kArgList, pcFormat);

        FormatOutput(pcFormat, kArgList);

        va_end(kArgList);

        EE_LOG(ms_akLogIDMap[m_iMessageType], efd::ILogger::kLVL0, ("%s", ms_acBuffer));

        ms_kCriticalSection.Unlock();
    }

}

//--------------------------------------------------------------------------------------------------
NiLogger::NiLogger(const char* pcFormat, ...) : m_iMessageType(0)
{
    if (OkayToOutput() && pcFormat)
    {
        ms_kCriticalSection.Lock();

        va_list kArgList;
        va_start(kArgList, pcFormat);

        FormatOutput(pcFormat, kArgList);

        va_end(kArgList);

        EE_LOG(ms_akLogIDMap[m_iMessageType], efd::ILogger::kLVL0, ("%s", ms_acBuffer));

        ms_kCriticalSection.Unlock();
    }
}//-------------------------------------------------------------------------------------------------
NiLoggerDirect::NiLoggerDirect(int iLogID, const char* pcFormat, ...)
{
    ms_kCriticalSection.Lock();

    m_iMessageType = NIMESSAGE_RESERVED_FOR_LOGDIRECT;
    ms_akMessageOptions[m_iMessageType].m_iLogID = iLogID;

    va_list kArgList;
    va_start(kArgList, pcFormat);

    FormatOutput(pcFormat, kArgList);

    va_end(kArgList);

    EE_LOG(ms_akLogIDMap[m_iMessageType], efd::ILogger::kLVL0, ("%s", ms_acBuffer));

    ms_kCriticalSection.Unlock();
}

//--------------------------------------------------------------------------------------------------
bool NiLogger::OkayToOutput()
{
    return (m_iMessageType >= 0 && m_iMessageType < NIMESSAGE_MAX_TYPES &&
        (ms_akMessageOptions[m_iMessageType].m_bOutputToDebugWindow ||
        ms_akMessageOptions[m_iMessageType].m_iLogID != -1));
}

//--------------------------------------------------------------------------------------------------
void NiLogger::FormatOutput(const char* pcFormat, va_list kArgList)
{
    char* pcBuffer = ms_acBuffer;
    unsigned int uiMaxOutputLength = MAX_OUTPUT_LENGTH;

    if (ms_akMessageOptions[m_iMessageType].m_bPrependTimestamp)
    {
        // prepend optional timestamp
        NiSprintf(pcBuffer, MAX_OUTPUT_LENGTH, "%f: ",
            NiGetCurrentTimeInSec());
        size_t stTimestampStrlen = strlen(pcBuffer);
        uiMaxOutputLength = (unsigned int)
            (uiMaxOutputLength - stTimestampStrlen);
        pcBuffer += stTimestampStrlen;

    }

    NiVsprintf(pcBuffer, uiMaxOutputLength, pcFormat, kArgList);
}

//--------------------------------------------------------------------------------------------------
void NiLogger::_SDMInit()
{
    // Ensure a logger instance exists.
    EE_VERIFY(efd::LoggerSingleton::Instance());

    for (unsigned int uiLogFile = 0; uiLogFile < MAX_NUM_LOGFILES;
        uiLogFile++)
    {
        ms_akLogFile[uiLogFile] = NULL;
    }

    ms_akLogIDMap[NIMESSAGE_GENERAL_0] = efd::kGamebryoGeneral0;
    ms_akLogIDMap[NIMESSAGE_GENERAL_1] = efd::kGamebryoGeneral1;
    ms_akLogIDMap[NIMESSAGE_GENERAL_2] = efd::kGamebryoGeneral2;

    ms_akLogIDMap[NIMESSAGE_MEMORY_0] = efd::kGamebryoMemory0;
    ms_akLogIDMap[NIMESSAGE_MEMORY_1] = efd::kGamebryoMemory1;
    ms_akLogIDMap[NIMESSAGE_MEMORY_2] = efd::kGamebryoMemory2;

    ms_akLogIDMap[NIMESSAGE_MEMORY_TIMESTAMP] = efd::kGamebryoMemoryTimeStamp;

    ms_akLogIDMap[NIMESSAGE_RESERVED_FOR_LOGDIRECT] = efd::kGamebryoReserved;

    ms_akOpenModeMap[OPEN_APPEND] = efd::FileDestination::kFileAppend;
    ms_akOpenModeMap[OPEN_OVERWRITE] = efd::FileDestination::kFileOverwrite;
    ms_akOpenModeMap[OPEN_UNIQUENAME] = efd::FileDestination::kUniqueFileName;

    NiLogBehavior::Get()->Initialize();

}

//--------------------------------------------------------------------------------------------------
void NiLogger::_SDMShutdown()
{
    CloseAllLogs();
}

//--------------------------------------------------------------------------------------------------
int NiLogger::OpenLog(const char* pcFilename, OpenMode eOpenMode,
    bool bFlushOnWrite, bool bCommitToDisk)
{
    EE_UNUSED_ARG(bFlushOnWrite);
    EE_UNUSED_ARG(bCommitToDisk);
    efd::LoggerSingleton* pkLogger = efd::LoggerSingleton::Instance();

    if (!pkLogger || !pkLogger->GetLogger())
        return -1;


    if (ms_uiLogFileCount == NiLogger::MAX_NUM_LOGFILES)
    {
        // No available slots.
        return -1;
    }

    // Find an open slot
    int iSlot = 0;

    while (iSlot < NiLogger::MAX_NUM_LOGFILES)
    {
        if (!ms_akLogFile[iSlot])
        {
            // Found one.
            break;
        }
        iSlot++;
    }

    // Make sure we found one
    if (iSlot == NiLogger::MAX_NUM_LOGFILES)
    {
        // there are no slots available
        EE_FAIL("NiLogger::OpenLog - no slots available.");
        return -1;
    }

    // Create the log file instance
    ms_akLogFile[iSlot] =
        efd::FileDestination::CreateAndOpen(pkLogger->GetLogger(), pcFilename,
        ms_akOpenModeMap[(NiUInt32)eOpenMode]);

    if (!ms_akLogFile[iSlot])
    {
        // Failed to create the file!
        return -1;
    }

    NiLogger::ms_uiLogFileCount++;

    return iSlot;
}

//--------------------------------------------------------------------------------------------------
void NiLogger::CloseLog(int iLogID)
{
    if (iLogID >= 0 && iLogID < NiLogger::MAX_NUM_LOGFILES &&
        ms_akLogFile[iLogID])
    {
        ms_akLogFile[iLogID] = NULL;
        NiLogger::ms_uiLogFileCount--;
    }
}

//--------------------------------------------------------------------------------------------------
void NiLogger::CloseAllLogs()
{
    for (unsigned int uiLogID = 0; uiLogID < NiLogger::MAX_NUM_LOGFILES;
        uiLogID++)
    {
        ms_akLogFile[uiLogID] = NULL;
    }
    NiLogger::ms_uiLogFileCount = 0;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiLogger::UnixToDos(char* pcString, unsigned int uiStringLen)
{
    // printf formatting may be unix or dos depending on platform.
    // This function may be used to convert the string to dos
    // format.

    // It reformats in a temporary buffer and then copies the temporary
    // buffer over the original one.
    char acTempBuffer[MAX_OUTPUT_LENGTH];

    const char cLF = 0x0A;
    const char cCR = 0x0D;

    char cPrevChar = '\0';
    char* pcChar = &pcString[0];

    unsigned int uiNewLength = 0;
    unsigned int uiCurPoint = 0;

    while (*pcChar != '\0' && uiCurPoint < uiStringLen)
    {
        if ((*pcChar == cLF) && (cPrevChar != cCR) &&
            uiNewLength < MAX_OUTPUT_LENGTH-3)
        {
            acTempBuffer[uiNewLength++] = cCR;
            acTempBuffer[uiNewLength++] = cLF;
        }
        else if (uiNewLength < MAX_OUTPUT_LENGTH-2)
        {
            acTempBuffer[uiNewLength++] = *pcChar;
        }
        cPrevChar = *pcChar;
        pcChar++;
        uiCurPoint++;
    }
    acTempBuffer[uiNewLength] = '\0';
    EE_ASSERT(uiNewLength <= uiStringLen);

    NiStrcpy(pcString, uiStringLen, acTempBuffer);

    return uiNewLength;
}

//--------------------------------------------------------------------------------------------------
void NiLogger::FlushAllLogs()
{
    for (NiUInt32 ui = 0; ui < MAX_NUM_LOGFILES; ui++)
    {
        if (ms_akLogFile[ui])
            ms_akLogFile[ui]->Flush();
    }

}

//--------------------------------------------------------------------------------------------------

void NiLogger::CreateDefaultDebugOutDestination()
{
    efd::ILogger* pkLogger = efd::LoggerSingleton::Instance()->GetLogger();

    efd::ILogDestination* pkDefault = EE_NEW efd::DebugOutDestination("CRDebugOut");
    pkLogger->AddDest(pkDefault, true);
}
