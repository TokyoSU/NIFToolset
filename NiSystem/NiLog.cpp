// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

// Precompiled Header
#include "NiSystemPCH.h"
#include "NiLog.h"

char NiLogger::ms_acBuffer[MAX_OUTPUT_LENGTH];
NiLogMessageOptions NiLogger::ms_akMessageOptions[NIMESSAGE_MAX_TYPES];
NiLogger::NiLogFile NiLogger::ms_akLogFile[MAX_NUM_LOGFILES];
unsigned int NiLogger::ms_uiLogFileCount = 0;
NiFastCriticalSection NiLogger::ms_kCriticalSection;

//---------------------------------------------------------------------------
NiLogMessageOptions::NiLogMessageOptions() : m_bOutputToDebugWindow(false), 
    m_iLogID(-1), m_bPrependTimestamp (false)
{
}
//---------------------------------------------------------------------------
NiLogger::NiLogger(int iMessageType, const char* pcFormat, ...)
    : m_iMessageType(iMessageType)
{
    if(OkayToOutput() && pcFormat)
    {
        ms_kCriticalSection.Lock();

        va_list kArgList;
        va_start( kArgList, pcFormat );

        FormatOutput(pcFormat, kArgList);

        va_end(kArgList);

        LogBuffer();

        ms_kCriticalSection.Unlock();
    }
}
//---------------------------------------------------------------------------
NiLogger::NiLogger(const char* pcFormat, ...) : m_iMessageType(0)
{
    if(OkayToOutput() && pcFormat)
    {
        ms_kCriticalSection.Lock();

        va_list kArgList;
        va_start( kArgList, pcFormat );

        FormatOutput(pcFormat, kArgList);

        va_end(kArgList);

        LogBuffer();

        ms_kCriticalSection.Unlock();
    }
}//---------------------------------------------------------------------------
NiLoggerDirect::NiLoggerDirect(int iLogID, const char* pcFormat, ...)
{
    ms_kCriticalSection.Lock();

    m_iMessageType = NIMESSAGE_RESERVED_FOR_LOGDIRECT;
    ms_akMessageOptions[m_iMessageType].m_iLogID = iLogID; 

    va_list kArgList;
    va_start( kArgList, pcFormat );

    FormatOutput(pcFormat, kArgList);

    va_end(kArgList);

    LogBuffer();

    ms_kCriticalSection.Unlock();
}
//---------------------------------------------------------------------------
bool NiLogger::OkayToOutput()
{
    return (m_iMessageType >= 0 && m_iMessageType < NIMESSAGE_MAX_TYPES && 
        (ms_akMessageOptions[m_iMessageType].m_bOutputToDebugWindow || 
        ms_akMessageOptions[m_iMessageType].m_iLogID != -1)); 
}
//---------------------------------------------------------------------------
void NiLogger::FormatOutput(const char* pcFormat, va_list kArgList)
{
    char* pcBuffer = ms_acBuffer;
    unsigned int uiMaxOutputLength = MAX_OUTPUT_LENGTH;

    if (ms_akMessageOptions[m_iMessageType].m_bPrependTimestamp)
    {
        // prepend optional timestamp
        NiSprintf(pcBuffer, MAX_OUTPUT_LENGTH, "%f: ", NiGetCurrentTimeInSec());
        size_t stTimestampStrlen = strlen(pcBuffer);
        uiMaxOutputLength = (unsigned int)
            (uiMaxOutputLength - stTimestampStrlen);
        pcBuffer += stTimestampStrlen;

    }

    NiVsprintf(pcBuffer, uiMaxOutputLength, pcFormat, kArgList);
}
//---------------------------------------------------------------------------
void NiLogger::LogBuffer()
{
    if (ms_akMessageOptions[m_iMessageType].m_bOutputToDebugWindow)
    {
#if defined(WIN32) || defined(_XENON)
        OutputDebugStringA(ms_acBuffer);
#elif defined (_PS3)
        printf("%s", ms_acBuffer);
#elif defined(_WII)
        OSReport(ms_acBuffer);
#endif
    }

    if (ms_akMessageOptions[m_iMessageType].m_iLogID >= 0)
    {
        NiLogger::NiLogFile* pkLogFile =
            &ms_akLogFile[ms_akMessageOptions[m_iMessageType].m_iLogID];

        if (pkLogFile->IsActive())
            pkLogFile->Log(ms_acBuffer);
    }
}

//---------------------------------------------------------------------------
void NiLogger::_SDMInit()
{
    for (unsigned int uiLogFile = 0; uiLogFile < MAX_NUM_LOGFILES; 
        uiLogFile++)
    {
        ms_akLogFile[uiLogFile].Free();        
    }

    NiLogBehavior::Get()->Initialize();

}
//---------------------------------------------------------------------------
void NiLogger::_SDMShutdown()
{
    CloseAllLogs();
}
//---------------------------------------------------------------------------

int NiLogger::OpenLog(const char* pcFilename, OpenMode eOpenMode, 
    bool bFlushOnWrite, bool bCommitToDisk)
{
    if (ms_uiLogFileCount == NiLogger::MAX_NUM_LOGFILES)
    {
        // No available slots.
        return -1;
    }

    // Find an open slot
    int iSlot = 0;
    
    while (iSlot < NiLogger::MAX_NUM_LOGFILES)
    {
        if (!NiLogger::ms_akLogFile[iSlot].IsActive())
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
        NIASSERT(!"NiLogger::OpenLog - no slots available.");
        return -1;
    }

    // Create the log file instance
    NiLogger::ms_akLogFile[iSlot].Init(pcFilename, eOpenMode, 
        bFlushOnWrite, bCommitToDisk);
    if (!NiLogger::ms_akLogFile[iSlot].IsActive())
    {
        // Failed to create the file!
        return -1;
    }

    NiLogger::ms_uiLogFileCount++;

    return iSlot;
}
//---------------------------------------------------------------------------
void NiLogger::CloseLog(int iLogID)
{
    if (iLogID >= 0 && iLogID < NiLogger::MAX_NUM_LOGFILES && 
        NiLogger::ms_akLogFile[iLogID].IsActive())
    {
        NiLogger::ms_akLogFile[iLogID].Free();
        NiLogger::ms_uiLogFileCount--;
    }
}
//---------------------------------------------------------------------------
void NiLogger::CloseAllLogs()
{
    for (unsigned int uiLogID = 0; uiLogID < NiLogger::MAX_NUM_LOGFILES; 
        uiLogID++)
    {
        NiLogger::ms_akLogFile[uiLogID].Free();
    }
    NiLogger::ms_uiLogFileCount = 0;
}
//---------------------------------------------------------------------------
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
        if (( *pcChar == cLF) && ( cPrevChar != cCR) && 
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
    NIASSERT(uiNewLength <= uiStringLen);
    
    NiStrcpy(pcString, uiStringLen, acTempBuffer);

    return uiNewLength;
}


//---------------------------------------------------------------------------
NiLogger::NiLogFile::NiLogFile()
{
    m_acName[0] = '\0';
}
//---------------------------------------------------------------------------
NiLogger::NiLogFile::~NiLogFile()
{
    CloseFile();
}
//---------------------------------------------------------------------------
void NiLogger::NiLogFile::CloseFile()
{
    if (m_pkFile)
    {
        fclose(m_pkFile);
    }
    m_pkFile = 0;
}
//---------------------------------------------------------------------------
bool NiLogger::NiLogFile::OpenFile(OpenMode eOpenMode, bool bCommitToDisk)
{
    char acOpenMode[5];

    switch (eOpenMode)
    {
    case OPEN_APPEND:
        NiStrcpy(acOpenMode, 5, "a+t");
        break;
    case OPEN_OVERWRITE:
        NiStrcpy(acOpenMode, 5, "wt");
        break;
    default:
        NIASSERT(!"NiLogger::OPEN_UNIQUENAME Not yet supported");
        m_pkFile = 0;
        return false;
    }

    if (bCommitToDisk)
        NiStrcat(acOpenMode, 5, "c");

#if _MSC_VER >= 1400
    if (fopen_s(&m_pkFile, m_acName, acOpenMode) != 0 || !m_pkFile)
        return false;
#else //#if _MSC_VER >= 1400
    m_pkFile = fopen(m_acName, acOpenMode);

    if (!m_pkFile)
        return false;
#endif //#if _MSC_VER >= 1400



    return true;
}
//---------------------------------------------------------------------------
void NiLogger::NiLogFile::Init(const char* pcName, OpenMode eOpenMode,
    bool bFlushOnWrite, bool bCommitToDisk)
{
    NIASSERT(pcName);

    NiStrcpy(m_acName, 256, pcName);
    m_bFlushOnWrite = bFlushOnWrite;

    if (!OpenFile(eOpenMode, bCommitToDisk))
    {
        NiOutputDebugString(
            "NiLogger::NiLogFile::Init Error "
            "- Failed to open the following file:");
        NiOutputDebugString(pcName);
        m_acName[0] = '\0';
        m_pkFile = 0;
    }
}
//---------------------------------------------------------------------------
void NiLogger::NiLogFile::Free()
{
    CloseFile();
    m_acName[0] = '\0';
}
//---------------------------------------------------------------------------
bool NiLogger::NiLogFile::IsActive()
{
    return m_acName[0] != '\0';
}
//---------------------------------------------------------------------------
void NiLogger::NiLogFile::Log(char* pcMessage)
{
    if (!m_pkFile)
        return;

    fwrite(pcMessage, sizeof(char), strlen(pcMessage), m_pkFile);
    if (m_bFlushOnWrite)
        fflush(m_pkFile);
}
//---------------------------------------------------------------------------
void NiLogger::FlushAllLogs()
{
    for (unsigned int uiLogID = 0; uiLogID < NiLogger::MAX_NUM_LOGFILES;
        uiLogID++)
    {
        if (NiLogger::ms_akLogFile[uiLogID].IsActive())
            fflush(NiLogger::ms_akLogFile[uiLogID].m_pkFile);
    }
}
//---------------------------------------------------------------------------
