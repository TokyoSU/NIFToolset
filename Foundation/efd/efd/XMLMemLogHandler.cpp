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

#include <efd/utf8string.h>
#include <efd/MemTracker.h>
#include <efd/MemManager.h>
#include <efd/File.h>
#include <efd/PathUtils.h>
#include <efd/XMLMemLogHandler.h>
#include <efd/StackUtils.h>
#include <efd/DefaultInitializeMemoryLogHandler.h>

using namespace efd::StackUtils;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::XMLMemLogHandler);

namespace efd
{

//------------------------------------------------------------------------------------------------
XMLMemLogHandler::XMLMemLogHandler() : m_bInitalized(false)
{
}

//------------------------------------------------------------------------------------------------
efd::Bool XMLMemLogHandler::IsInitialized()
{
    return m_bInitalized;
}

//------------------------------------------------------------------------------------------------
const char* XMLMemLogHandler::FormatForXML(const char* pcInString)
{
    static char acOutString[1280];
    EE_MEMASSERT(pcInString != NULL);
    size_t stLen = efd::Strlen(pcInString);

    // If this assert is hit, then the length of the input string is longer
    // than the sanitization buffer (1024 characters including the NULL).
    EE_MEMASSERT(stLen < 1024);

    unsigned int uiIdx = 0;
    for (size_t stSrcIdx = 0;
        (stSrcIdx < stLen) && (uiIdx < EE_ARRAYSIZEOF(acOutString)-1);
        stSrcIdx++)
    {
        acOutString[uiIdx] = pcInString[stSrcIdx];

        if (acOutString[uiIdx] == '<')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'l';
            acOutString[uiIdx++] = 't';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '>')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'g';
            acOutString[uiIdx++] = 't';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '&')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'a';
            acOutString[uiIdx++] = 'm';
            acOutString[uiIdx++] = 'p';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '`')
        {
            acOutString[uiIdx++] = ' ';
        }
        else if (acOutString[uiIdx] == '\'')
        {
            acOutString[uiIdx++] = ' ';
        }
        else
        {
            uiIdx++;
        }
    }

    acOutString[uiIdx] = '\0';
    return acOutString;
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::WriteTextToFile(efd::File* pkFile, const char* pcText, ...)
{
    if (pkFile == 0 || pcText == 0)
    {
        return;
    }

    char acMessage[1280];

    va_list args;
    va_start(args, pcText);
    efd::Vsprintf(acMessage, EE_ARRAYSIZEOF(acMessage), pcText, args);
    va_end(args);

    pkFile->Write(acMessage, efd::Strlen(acMessage));
}

//------------------------------------------------------------------------------------------------
inline void GetSysAndLocalTime(struct tm*& o_pSysTime, struct tm*& o_pLocalTime)
{
#ifdef EE_PLATFORM_PS3
    time_t tCurrentTime = 0;
    o_pSysTime = gmtime(&tCurrentTime);
    o_pLocalTime = gmtime(&tCurrentTime);
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    __time64_t tCurrentTime;
    _time64(&tCurrentTime);
    static struct tm kSysTime;
    o_pSysTime = &kSysTime;
    _gmtime64_s(o_pSysTime, &tCurrentTime);
    static struct tm kLocalTime;
    o_pLocalTime = &kLocalTime;
    _localtime64_s(o_pLocalTime, &tCurrentTime);
#else //#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    time_t tCurrentTime;
    time(&tCurrentTime);
    o_pSysTime = gmtime(&tCurrentTime);
    o_pLocalTime = localtime(&tCurrentTime);
#endif //#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#endif
}

//------------------------------------------------------------------------------------------------
efd::Bool XMLMemLogHandler::OnInit()
{
    m_pOverviewXMLFile = NULL;
    m_pLeakXMLFile = NULL;
    m_pSummaryXMLFile = NULL;

    char acLogPath[efd::EE_MAX_PATH];
    EE_VERIFY(PathUtils::GetDefaultLogDirectory(acLogPath, efd::EE_MAX_PATH));

    char acOverviewPath[efd::EE_MAX_PATH];
    char acLeakPath[efd::EE_MAX_PATH];

    efd::Sprintf(acOverviewPath, EE_MAX_PATH, "%sMemory-Overview.xml", acLogPath);
    efd::Sprintf(acLeakPath, EE_MAX_PATH, "%sMemory-Leaks.xml", acLogPath);

    m_pOverviewXMLFile = efd::File::GetFile(acOverviewPath, efd::File::WRITE_ONLY);
    m_pLeakXMLFile = efd::File::GetFile(acLeakPath, efd::File::WRITE_ONLY);

    struct tm* pSysTime = NULL;
    struct tm* pLocalTime = NULL;
    GetSysAndLocalTime(pSysTime, pLocalTime);
    EE_MEMASSERT(pSysTime && pLocalTime);

    if (m_pOverviewXMLFile)
    {
        WriteLogHeader(m_pOverviewXMLFile, "memoryreport.xsl", pSysTime, pLocalTime);
        WriteTextToFile(m_pOverviewXMLFile, "<memory_dump timestamp='%f' >\n",
            GetCurrentTimeInSec());
    }

    if (m_pLeakXMLFile)
    {
        WriteLogHeader(m_pLeakXMLFile, "memoryleak.xsl", pSysTime, pLocalTime);
    }

    CreateXSLForLeaks();
    CreateXSLForSnapshot();
    CreateXSLForFreeReport();

    m_bInitalized = true;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool XMLMemLogHandler::OnShutdown()
{
    if (m_pOverviewXMLFile)
        WriteTextToFile(m_pOverviewXMLFile, "</memory_dump>\n");

    LogMemoryReport();
    LogSummaryStats();

    if (m_pOverviewXMLFile)
        WriteTextToFile(m_pOverviewXMLFile, "</memory_log>\n");

    if (m_pLeakXMLFile)
        WriteTextToFile(m_pLeakXMLFile, "</memory_log>\n");

    // Note: m_bInitalized is explicitly disabled after the log summaries are reported but
    // before the XML files are deleted. This ensures that the summaries are still written to
    // XML files. It also prevents the IMemLogHandler from attempting to log the deleting of the
    // XML files. Attempting to write to file X, that file X is being deleted obviously causes
    // problems.
    m_bInitalized = false;

    EE_DELETE m_pOverviewXMLFile;
    EE_DELETE m_pLeakXMLFile;

    return true;
}

//------------------------------------------------------------------------------------------------
bool IsDeletion(MemEventType eDeallocType)
{
    switch (eDeallocType)
    {
    case EE_MET_DELETE:
    case EE_MET_DELETE_ARRAY:
    case EE_MET_FREE:
    case EE_MET_ALIGNEDFREE:
    case EE_MET_EXTERNALFREE:
        return true;

    default:
        return false;
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::WriteAllocUnitXMLBlock(
    efd::File* pkFile,
    const AllocUnit *pkUnit,
    const char* pcPrefix,
    bool includeStack,
    MemEventType eDeallocType,
    double fDeallocTime,
    unsigned long ulDeallocThreadId,
    size_t stSizeUnused) const
{
    EE_MEMASSERT(pcPrefix != NULL);
    EE_MEMASSERT(pkFile != NULL);

    WriteTextToFile(pkFile, (char*)pcPrefix);
    WriteTextToFile(pkFile, "<alloc_unit ");
    WriteTextToFile(pkFile, "id='%d' ", pkUnit->m_stAllocationID);
    WriteTextToFile(pkFile, "alloc_thread_id='%lX' ", pkUnit->m_ulAllocThreadId);
    WriteTextToFile(pkFile, "dealloc_thread_id='%lX' ", ulDeallocThreadId);
    WriteTextToFile(pkFile, "alloc_time='%08f' ", pkUnit->m_fAllocTime);
    WriteTextToFile(pkFile, "dealloc_time='%08f' ", fDeallocTime);
    WriteTextToFile(pkFile, "life_span='%08f' ", fDeallocTime - pkUnit->m_fAllocTime);
    WriteTextToFile(pkFile, "addr='0x%p' ", pkUnit->m_pvMem);
    WriteTextToFile(pkFile, "size='%d' ", pkUnit->m_stSizeAllocated);
    WriteTextToFile(pkFile, "size_requested='%d' ", pkUnit->m_stSizeRequested);
    WriteTextToFile(pkFile, "size_unused='%d' ", stSizeUnused);
    WriteTextToFile(pkFile, "alignment='%d' ", pkUnit->m_stAlignment);
    WriteTextToFile(pkFile, "alloc_type='%s' ",
        MemManager::MemEventTypeToString(pkUnit->m_eAllocType));
    WriteTextToFile(pkFile, "dealloc_type='%s' ",
        MemManager::MemEventTypeToString(eDeallocType));
    WriteTextToFile(pkFile, "mem_hint='%d' ", pkUnit->m_kAllocHint.GetRaw());
    WriteTextToFile(pkFile, "file='%s' ", pkUnit->m_kFLF.SourceFileStripper());
    WriteTextToFile(pkFile, "line='%d' ", pkUnit->m_kFLF.m_uiLine);
    WriteTextToFile(pkFile, "func='%s' ", FormatForXML(pkUnit->m_kFLF.m_pcFunc));
    WriteTextToFile(pkFile, "long_file='%s' ", pkUnit->m_kFLF.m_pcFile);
#if defined(EE_MEMTRACKER_DETAILEDREPORTING)
    // If this is being deleted then we have already overwritten the memory with 0xDDDDDDDD so
    // we can no longer generate a detailed report.
    if (!IsDeletion(eDeallocType))
    {
        char objectDetails[1024];
        pkUnit->GetDetailedReport(objectDetails, EE_ARRAYSIZEOF(objectDetails));
        if (objectDetails[0])
        {
            WriteTextToFile(pkFile, "detail='%s' ", FormatForXML(objectDetails));
        }
    }
#endif
    WriteTextToFile(pkFile, ">");
#if defined(EE_MEMTRACKER_STACKTRACE)
    if (includeStack)
    {
        char stack[1024];
        ResolveSymbolNames(pkUnit->m_stack, pkUnit->m_stackSize, stack, EE_ARRAYSIZEOF(stack));
        if (stack[0])
        {
            WriteTextToFile(pkFile, "\n%s\t<stack>\n", pcPrefix);
            WriteTextToFile(pkFile, "%s", FormatForXML(stack));
            WriteTextToFile(pkFile, "</stack>\n");
            WriteTextToFile(pkFile, (char*)pcPrefix);
        }
    }
#else
    EE_UNUSED_ARG(includeStack);
#endif
    WriteTextToFile(pkFile, "</alloc_unit>\n");
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::LogAllocUnit(
    const AllocUnit *pkUnit,
    const char* pcPrefix,
    MemEventType eDeallocType,
    double fDeallocTime,
    unsigned long ulDeallocThreadId,
    size_t stSizeUnused) const
{
    if (m_pOverviewXMLFile)
    {
        if (eDeallocType != EE_MET_UNKNOWN)
        {
            // Only log deallocations
            WriteAllocUnitXMLBlock(m_pOverviewXMLFile, pkUnit, pcPrefix, false,
                eDeallocType, fDeallocTime, ulDeallocThreadId, stSizeUnused);
        }
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::LogAllocUnitLeak(const AllocUnit *pkUnit, const char* pcPrefix) const
{
    if (m_pLeakXMLFile)
    {
        WriteAllocUnitXMLBlock(m_pLeakXMLFile, pkUnit, pcPrefix, true);
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::LogMemoryReport() const
{
    efd::MemTracker* pkMemTracker = efd::MemTracker::Get();
    EE_MEMASSERT(pkMemTracker);

    if (m_pLeakXMLFile)
    {
        WriteTextToFile(m_pLeakXMLFile, "<active_memory_dump timestamp='%f' >\n",
            GetCurrentTimeInSec());

        pkMemTracker->ReportActiveAllocations(
            (efd::MemTracker::ReportActiveAllocUnit)XMLMemLogHandler::ReportMemoryLeaks);
        WriteTextToFile(m_pLeakXMLFile, "</active_memory_dump>\n");
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::ReportMemoryLeaks(AllocUnit* pkActiveAllocUnit, IMemLogHandler* pLogHandler)
{
    pLogHandler->LogAllocUnitLeak(pkActiveAllocUnit, "\t");
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::LogSummaryStats() const
{
    if (m_pOverviewXMLFile)
    {
        efd::MemTracker* pkMemTracker = efd::MemTracker::Get();
        EE_MEMASSERT(pkMemTracker);

        WriteTextToFile(m_pOverviewXMLFile, "\t<memory_summary timestamp='%f' ",
            GetCurrentTimeInSec());
        WriteTextToFile(m_pOverviewXMLFile, "TotalActiveSize='%d' ",
            pkMemTracker->m_stActiveMemory);
        WriteTextToFile(m_pOverviewXMLFile, "PeakActiveSize='%d' ",
            pkMemTracker->m_stPeakMemory);
        WriteTextToFile(m_pOverviewXMLFile, "AccumulatedSize='%d' ",
            pkMemTracker->m_stAccumulatedMemory);
        WriteTextToFile(m_pOverviewXMLFile, "AllocatedButUnusedSize='%d' ",
            pkMemTracker->m_stUnusedButAllocatedMemory);
        WriteTextToFile(m_pOverviewXMLFile, "ActiveAllocCount='%d' ",
            pkMemTracker->m_stActiveAllocationCount);
        WriteTextToFile(m_pOverviewXMLFile, "PeakActiveAllocCount='%d' ",
            pkMemTracker->m_stPeakAllocationCount);
        WriteTextToFile(m_pOverviewXMLFile, "TotalAllocCount='%d' ",
            pkMemTracker->m_stAccumulatedAllocationCount);

        WriteTextToFile(m_pOverviewXMLFile, "TotalActiveExternalSize='%d' ",
            pkMemTracker->m_stActiveExternalMemory);
        WriteTextToFile(m_pOverviewXMLFile, "PeakActiveExternalSize='%d' ",
            pkMemTracker->m_stPeakExternalMemory);
        WriteTextToFile(m_pOverviewXMLFile, "AccumulatedExternalSize='%d' ",
            pkMemTracker->m_stAccumulatedExternalMemory);
        WriteTextToFile(m_pOverviewXMLFile, "ActiveExternalAllocCount='%d' ",
            pkMemTracker->m_stActiveExternalAllocationCount);
        WriteTextToFile(m_pOverviewXMLFile, "PeakExternalActiveAllocCount='%d' ",
            pkMemTracker->m_stPeakExternalAllocationCount);
        WriteTextToFile(m_pOverviewXMLFile, "TotalExternalAllocCount='%d' ",
            pkMemTracker->m_stAccumulatedExternalAllocationCount);

        WriteTextToFile(m_pOverviewXMLFile, "TotalTrackerOverhead='%d' ",
            pkMemTracker->m_stActiveTrackerOverhead);
        WriteTextToFile(m_pOverviewXMLFile, "PeakTrackerOverhead='%d' ",
            pkMemTracker->m_stPeakTrackerOverhead);
        WriteTextToFile(m_pOverviewXMLFile, "AccumulatedTrackerOverhead='%d' ",
            pkMemTracker->m_stAccumulatedTrackerOverhead);

        WriteTextToFile(m_pOverviewXMLFile, "></memory_summary>\n");
    }
}

//------------------------------------------------------------------------------------------------
bool XMLMemLogHandler::SetMarker(const char* pcMarkerType,
    const char* pcClassifier, const char* pcString)
{
    EE_UNUSED_ARG(pcClassifier);
    EE_UNUSED_ARG(pcMarkerType);
    EE_UNUSED_ARG(pcString);

#if !defined(EE_EFD_CONFIG_SHIPPING)

    if (!m_pOverviewXMLFile)
        return false;

    const char* pcPrefix =  "\t";

    // Write out the marker to the log
    WriteTextToFile(m_pOverviewXMLFile, (char*)pcPrefix);
    WriteTextToFile(m_pOverviewXMLFile, "<alloc_marker ");

    // the marker may not be XML-safe, so we need to auto-format it
    const char* pcFormattedString = FormatForXML(pcMarkerType);
    WriteTextToFile(m_pOverviewXMLFile, "marker_type='%s' ", pcFormattedString);

    // the marker may not be XML-safe, so we need to auto-format it
    pcFormattedString = FormatForXML(pcClassifier);
    WriteTextToFile(m_pOverviewXMLFile, "classifier='%s' ", pcFormattedString);

    // the marker may not be XML-safe, so we need to auto-format it
    pcFormattedString = FormatForXML(pcString);
    WriteTextToFile(m_pOverviewXMLFile, "marker='%s' ", pcFormattedString);

    efd::MemTracker* pMemTracker =  efd::MemTracker::Get();
    EE_MEMASSERT(pMemTracker);

    WriteTextToFile(m_pOverviewXMLFile, "next_id='%d' ", pMemTracker->GetCurrentAllocationID());
    WriteTextToFile(m_pOverviewXMLFile, "thread_id='%lX' ", GetCurrentThreadId());
    WriteTextToFile(m_pOverviewXMLFile, "time='%08f' ", GetCurrentTimeInSec());
    WriteTextToFile(m_pOverviewXMLFile, "/>\n");

#endif
    return true;
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::CreateXSLForLeaks()
{
    const char* pcXSLFile =
        "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
        "\n"
        "<xsl:stylesheet\n"
        "version=\"1.0\"\n"
        "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
        "<xsl:template match=\"/\">\n"
        "        \n"
        "<html>\n"
        "<head>\n"
        "        <title>Memory Leak Report</title>\n"
        "</head>\n"
        "\n"
        "<body>\n"
        "        <h1 align=\"center\">Memory Leak Report</h1>\n"
        "        <h2 align=\"center\">Date: <xsl:value-of select=\""
        "memory_log/@date\"/></h2>\n"
        "        <h3 align=\"center\">Summary:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\""
        " style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "               <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Statistic</th><th>Value</th>    \n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td >Total Active Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "sum(memory_log/active_memory_dump/alloc_unit/@size)\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Active Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "count(memory_log/active_memory_dump/alloc_unit)\"/></td>\n"
        "                </tr>\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Leaked Allocations:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "               <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Allocation #</th>\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Birth Time</th>\n"
        "                    <th>File</th>\n"
        "                    <th>Line</th>\n"
        "                    <th>Function</th>\n"
        "                    <th>Long Filename</th>\n"
        "<xsl:if test=\"count(memory_log/active_memory_dump/alloc_unit[@detail])\">\n"
        "                    <th>Details</th>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"count(memory_log/active_memory_dump/alloc_unit/stack)\">\n"
        "                    <th>Stack Trace</th>\n"
        "</xsl:if>\n"
        "                </tr>\n"
        "                <xsl:for-each select=\"memory_log/active_memory_dump/alloc_unit\">\n"
        "                <xsl:sort select=\"@id\" data-type=\"number\" order=\"ascending\"/>\n"
        "                    <tr>\n"
        "                        <td><xsl:value-of select=\"@id\"/></td>\n"
        "                        <td><xsl:value-of select=\"@size\"/></td>\n"
        "                        <td><xsl:value-of select=\"@alloc_time\"/></td>\n"
        "                        <td><xsl:value-of select=\"@file\"/></td>\n"
        "                        <td><xsl:value-of select=\"@line\"/></td>\n"
        "                        <td><xsl:value-of select=\"@func\"/></td>\n"
        "                        <td><a><xsl:attribute name=\"href\">file:///<xsl:value-of select=\"@long_file\"/>\n"
        "                            </xsl:attribute><xsl:value-of select=\"@long_file\"/></a></td>\n"
        "<xsl:if test=\"count(/memory_log/active_memory_dump/alloc_unit[@detail])\">\n"
        "<xsl:if test=\"@detail\">\n"
        "                       <td><pre><xsl:value-of select=\"@detail\"/></pre></td>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"not(@detail)\">\n"
        "                       <td><xsl:text disable-output-escaping=\"yes\">&amp;nbsp;</xsl:text></td>\n"
        "</xsl:if>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"count(/memory_log/active_memory_dump/alloc_unit/stack)\">\n"
        "<xsl:if test=\"stack\">\n"
        "                        <td><pre><xsl:value-of select=\"stack\"/></pre></td>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"not(stack)\">\n"
        "                       <td><xsl:text disable-output-escaping=\"yes\">&amp;nbsp;</xsl:text></td>\n"
        "</xsl:if>\n"
        "</xsl:if>\n"
        "                    </tr>\n"
        "                </xsl:for-each>\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "</body>\n"
        "\n"
        "</html>\n"
        "\n"
        "</xsl:template>\n"
        "</xsl:stylesheet>\n";

    char acLogPath[efd::EE_MAX_PATH];
    EE_VERIFY(efd::PathUtils::GetDefaultLogDirectory(acLogPath, efd::EE_MAX_PATH));

    char acFilename[efd::EE_MAX_PATH];

    efd::Sprintf(acFilename, efd::EE_MAX_PATH, "%smemoryleak.xsl", acLogPath);

    // Only write the XSL file if it doesn't already exist
    if (!efd::File::Access(acFilename, efd::File::READ_ONLY))
    {
        efd::File* pkFile = efd::File::GetFile(acFilename, efd::File::WRITE_ONLY);
        if (pkFile)
        {
            pkFile->PutS(pcXSLFile);
            EE_DELETE pkFile;
        }
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::CreateXSLForSnapshot()
{
#if defined(EE_MEMTRACKER_SNAPSHOT)
    const char* pcXSLFile =
        "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
        "\n"
        "<xsl:stylesheet\n"
        "version=\"1.0\"\n"
        "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
        "<xsl:template match=\"/\">\n"
        "        \n"
        "<html>\n"
        "<head>\n"
        "        <title>Memory Snapshot Report</title>\n"
        "</head>\n"
        "\n"
        "<body>\n"
        "        <h1 align=\"center\">Memory Snapshot Report</h1>\n"
        "        <h2 align=\"center\">Date: <xsl:value-of select=\""
        "memory_log/@date\"/></h2>\n"
        "        <h3 align=\"center\">Summary:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\""
        " style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "               <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Statistic</th><th>Value</th>    \n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td >Total Active Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "sum(memory_log/active_memory_dump/alloc_unit/@size)\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Active Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "count(memory_log/active_memory_dump/alloc_unit)\"/></td>\n"
        "                </tr>\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Outstanding Allocations:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "               <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Allocation #</th>\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Birth Time</th>\n"
        "                    <th>File</th>\n"
        "                    <th>Line</th>\n"
        "                    <th>Function</th>\n"
        "                    <th>Long Filename</th>\n"
        "<xsl:if test=\"count(memory_log/active_memory_dump/alloc_unit[@detail])\">\n"
        "                    <th>Details</th>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"count(memory_log/active_memory_dump/alloc_unit/stack)\">\n"
        "                    <th>Stack Trace</th>\n"
        "</xsl:if>\n"
        "                </tr>\n"
        "                <xsl:for-each select=\"memory_log/active_memory_dump/alloc_unit\">\n"
        "                <xsl:sort select=\"@id\" data-type=\"number\" order=\"ascending\"/>\n"
        "                    <tr>\n"
        "                        <td><xsl:value-of select=\"@id\"/></td>\n"
        "                        <td><xsl:value-of select=\"@size\"/></td>\n"
        "                        <td><xsl:value-of select=\"@alloc_time\"/></td>\n"
        "                        <td><xsl:value-of select=\"@file\"/></td>\n"
        "                        <td><xsl:value-of select=\"@line\"/></td>\n"
        "                        <td><xsl:value-of select=\"@func\"/></td>\n"
        "                        <td><a><xsl:attribute name=\"href\">file:///<xsl:value-of select=\"@long_file\"/>\n"
        "                            </xsl:attribute><xsl:value-of select=\"@long_file\"/></a></td>\n"
        "<xsl:if test=\"count(/memory_log/active_memory_dump/alloc_unit[@detail])\">\n"
        "<xsl:if test=\"@detail\">\n"
        "                       <td><pre><xsl:value-of select=\"@detail\"/></pre></td>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"not(@detail)\">\n"
        "                       <td><xsl:text disable-output-escaping=\"yes\">&amp;nbsp;</xsl:text></td>\n"
        "</xsl:if>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"count(/memory_log/active_memory_dump/alloc_unit/stack)\">\n"
        "<xsl:if test=\"stack\">\n"
        "                        <td><pre><xsl:value-of select=\"stack\"/></pre></td>\n"
        "</xsl:if>\n"
        "<xsl:if test=\"not(stack)\">\n"
        "                       <td><xsl:text disable-output-escaping=\"yes\">&amp;nbsp;</xsl:text></td>\n"
        "</xsl:if>\n"
        "</xsl:if>\n"
        "                    </tr>\n"
        "                </xsl:for-each>\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "</body>\n"
        "\n"
        "</html>\n"
        "\n"
        "</xsl:template>\n"
        "</xsl:stylesheet>\n";

    char acLogPath[efd::EE_MAX_PATH];
    EE_VERIFY(efd::PathUtils::GetDefaultLogDirectory(acLogPath, efd::EE_MAX_PATH));

    char acFilename[efd::EE_MAX_PATH];

    efd::Sprintf(acFilename, efd::EE_MAX_PATH, "%smemorysnapshot.xsl", acLogPath);

    // Only write the XSL file if it doesn't already exist
    if (!efd::File::Access(acFilename, efd::File::READ_ONLY))
    {
        efd::File* pkFile = efd::File::GetFile(acFilename, efd::File::WRITE_ONLY);
        if (pkFile)
        {
            pkFile->PutS(pcXSLFile);
            EE_DELETE pkFile;
        }
    }
#endif // defined(EE_MEMTRACKER_SNAPSHOT)
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::CreateXSLForFreeReport()
{
    const char* pcXSLFile =
        "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
        "\n"
        "<xsl:stylesheet\n"
        "version=\"1.0\"\n"
        "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
        "<xsl:key name=\"sizes\" match=\"alloc_unit\" use=\"@size\"/>\n"
        "<xsl:key name=\"files\" match=\"alloc_unit\" use=\"@long_file\"/>\n"
        "<xsl:key name=\"funcs\" match=\"alloc_unit\" use=\"@func\"/>\n"
        "<xsl:template match=\"/\">\n"
        "        \n"
        "<html>\n"
        "<head>\n"
        "        <title>Memory Report</title>\n"
        "</head>\n"
        "\n"
        "<body>\n"
        "        <h1 align=\"center\">Memory Report</h1>\n"
        "        <h2 align=\"center\">Date: <xsl:value-of select=\"memory_log/"
        "@date\"/></h2>\n"
        "        <h3 align=\"center\">Summary:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Statistic</th><th>Value</th>   \n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td >Application Lifetime</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@timestamp\"/> seconds</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td >Total Active Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@TotalActiveSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Peak Active Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@PeakActiveSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Accumulated Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@AccumulatedSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Allocated But Unused Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@AllocatedButUnusedSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Percent of Allocations Unused</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "format-number(memory_log/memory_summary/@AllocatedButUnusedSize div "
        "memory_log/memory_summary/@AccumulatedSize * 100.0, '#.##')\"/>%</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Active Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@ActiveAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Peak Active Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@PeakActiveAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Total Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@TotalAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Total Tracker Overhead</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@TotalTrackerOverhead\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Peak Tracker Overhead</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@PeakTrackerOverhead\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Accumulated Overhead</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@AccumulatedTrackerOverhead\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td >Total Active External Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@TotalActiveExternalSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Peak Active External Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@PeakActiveExternalSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Accumulated External Size</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@AccumulatedExternalSize\"/> bytes</td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Active External Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@ActiveExternalAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Peak Active External Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@PeakExternalActiveAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Total External Alloc Count</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "memory_log/memory_summary/@TotalExternalAllocCount\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Allocation Sizes &lt; 1024 bytes</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "count(memory_log/memory_dump/alloc_unit[@size &lt; 1024])\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Allocation Life Spans &lt; 1 second</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "count(memory_log/memory_dump/alloc_unit[@life_span &lt; 1.0])\"/></td>\n"
        "                </tr>\n"
        "                <tr>\n"
        "                    <td>Percent Life Spans &lt; 1 second</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "format-number(count(memory_log/memory_dump/alloc_unit"
        "[@life_span &lt; 1.0]) div memory_log/memory_summary/@TotalAllocCount"
        " * 100.0, '#.##')\"/>%</td>\n"
        "                </tr>\n"
        "                 <tr>\n"
        "                    <td>Percent Sizes &lt; 1024 bytes</td>\n"
        "                    <td align=\"center\"> <xsl:value-of select=\""
        "format-number(count(memory_log/memory_dump/alloc_unit[@size &lt; 1024])"
        " div memory_log/memory_summary/@TotalAllocCount * 100.0, '#.##')\"/>%</td>\n"
        "                </tr>\n"
        "                \n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n\n"
        "        <h4 align=\"center\">(The sections below are usually empty, since per-allocation "
        "memory logging is disabled by default.)</h4>\n"
        "        <h4 align=\"center\">(To enable per-allocation memory logging, see \"Memory "
        "Tracking and Debugging\" in \"Programming for Memory Usage/Custom Allocation\".)</h4>\n"
        "        \n\n"
        "        <h3 align=\"center\">Top 50 Largest Allocations:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Allocation #</th>\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Life Span</th>\n"
        "                    <th>File</th>\n"
        "                    <th>Line</th>\n"
        "                    <th>Function</th>\n"
        "<xsl:if test=\"count(memory_log/memory_dump/alloc_unit/stack)\">\n"
        "                    <th>Stack Trace</th>\n"
        "</xsl:if>\n"
        "                </tr>\n"
        "                    <xsl:for-each select=\"memory_log/memory_dump/alloc_unit\">\n"
        "                    <xsl:sort select=\"@size\" data-type=\"number\" "
        "order=\"descending\"/>\n"
        "                        <xsl:if test=\"position() &lt; 50\">\n"
        "                        <tr>\n"
        "                            <td><xsl:value-of select=\"@id\"/></td>\n"
        "                            <td><xsl:value-of select=\"@size\"/></td>\n"
        "                            <td><xsl:value-of select=\"@life_span\"/></td>\n"
        "                            <td><xsl:value-of select=\"@file\"/></td>\n"
        "                            <td><xsl:value-of select=\"@line\"/></td>\n"
        "                            <td><xsl:value-of select=\"@func\"/></td>\n"
        "<xsl:if test=\"stack\">\n"
        "                            <td><pre><xsl:value-of select=\"stack\"/></pre></td>\n"
        "</xsl:if>\n"
        "                        </tr>\n"
        "                        </xsl:if>\n"
        "                    </xsl:for-each>\n"
        "                \n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Top 50 Smallest Allocations:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Allocation #</th>\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Life Span</th>\n"
        "                    <th>File</th>\n"
        "                    <th>Line</th>\n"
        "                    <th>Function</th>\n"
        "<xsl:if test=\"count(memory_log/memory_dump/alloc_unit/stack)\">\n"
        "                    <th>Stack Trace</th>\n"
        "</xsl:if>\n"
        "                </tr>\n"
        "                    <xsl:for-each select=\"memory_log/memory_dump/alloc_unit\">\n"
        "                    <xsl:sort select=\"@size\" data-type=\"number\" "
        "order=\"ascending\"/>\n"
        "                        <xsl:if test=\"position() &lt; 50\">\n"
        "                        <tr>\n"
        "                            <td><xsl:value-of select=\"@id\"/></td>\n"
        "                            <td><xsl:value-of select=\"@size\"/></td>\n"
        "                            <td><xsl:value-of select=\"@life_span\"/></td>\n"
        "                            <td><xsl:value-of select=\"@file\"/></td>\n"
        "                            <td><xsl:value-of select=\"@line\"/></td>\n"
        "                            <td><xsl:value-of select=\"@func\"/></td>\n"
        "<xsl:if test=\"stack\">\n"
        "                            <td><pre><xsl:value-of select=\"stack\"/></pre></td>\n"
        "</xsl:if>\n"
        "                        </tr>\n"
        "                        </xsl:if>\n"
        "                    </xsl:for-each>\n"
        "                \n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Top 50 Shortest-lived Allocations:</h3>\n"
        "        \n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Allocation #</th>\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Life Span</th>\n"
        "                    <th>File</th>\n"
        "                    <th>Line</th>\n"
        "                    <th>Function</th>\n"
        "<xsl:if test=\"count(memory_log/memory_dump/alloc_unit/stack)\">\n"
        "                    <th>Stack Trace</th>\n"
        "</xsl:if>\n"
        "                </tr>\n"
        "                    <xsl:for-each select=\"memory_log/memory_dump/alloc_unit\">\n"
        "                    <xsl:sort select=\"@life_span\" data-type=\"number\" "
        "order=\"ascending\"/>\n"
        "                        <xsl:if test=\"position() &lt; 50\">\n"
        "                        <tr>\n"
        "                            <td><xsl:value-of select=\"@id\"/></td>\n"
        "                            <td><xsl:value-of select=\"@size\"/></td>\n"
        "                            <td><xsl:value-of select=\"@life_span\"/></td>\n"
        "                            <td><xsl:value-of select=\"@file\"/></td>\n"
        "                            <td><xsl:value-of select=\"@line\"/></td>\n"
        "                            <td><xsl:value-of select=\"@func\"/></td>\n"
        "<xsl:if test=\"stack\">\n"
        "                            <td><pre><xsl:value-of select=\"stack\"/></pre></td>\n"
        "</xsl:if>\n"
        "                        </tr>\n"
        "                        </xsl:if>\n"
        "                    </xsl:for-each>\n"
        "                \n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Number of Allocations by File</h3>\n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" "
        "style=\"font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Filename</th>\n"
        "                    <th>Count</th>\n"
        "                </tr>\n"
        "                <xsl:for-each select=\"memory_log/memory_dump/"
        "alloc_unit[generate-id() = generate-id(key('files',@long_file)[1])]\">\n"
        "                <xsl:sort select=\"count(../alloc_unit[@long_file="
        "current()/@long_file])\" data-type=\"number\" order=\"descending\"/>\n"
        "                <tr>\n"
        "<!--                   <td><a><xsl:attribute name=\"href\">file:///"
        "<xsl:value-of select=\"@long_file\"/></xsl:attribute><xsl:value-of "
        "select=\"@long_file\"/></a>\n"
        "                    </td>-></!-->\n"
        "                    <td><xsl:value-of select=\"@long_file\"/></td>\n"
        "                    <td><xsl:value-of select=\"count(../alloc_unit["
        "@long_file=current()/@long_file])\"/></td>\n"
        "                </tr>\n"
        "                </xsl:for-each>\n"
        "\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Number of Allocations by Function</h3>\n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" style=\""
        "font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Function</th>\n"
        "                    <th>Count</th>\n"
        "                </tr>\n"
        "                <xsl:for-each select=\"memory_log/memory_dump/alloc_unit["
        "generate-id() = generate-id(key('funcs',@func)[1])]\">\n"
        "                <xsl:sort select=\"count(../alloc_unit[@func=current()/"
        "@func])\" data-type=\"number\" order=\"descending\"/>\n"
        "                <tr>\n"
        "                    <td><xsl:value-of select=\"@func\"/></td>\n"
        "                    <td><xsl:value-of select=\"count(../alloc_unit["
        "@func=current()/@func])\"/></td>\n"
        "                </tr>\n"
        "                </xsl:for-each>\n"
        "\n"
        "            </tbody>\n"
        "        </table>\n"
        "        \n"
        "        <h3 align=\"center\">Number of Allocations by Size</h3>\n"
        "        <table cellpadding=\"5\" border=\"2\" align=\"center\" style=\""
        "font-family:Arial,sans-serif; font-size:10pt\">\n"
        "            <tbody>\n"
        "                <tr style=\"color:black; background-color:#e9d6e7\">\n"
        "                    <th>Size in bytes</th>\n"
        "                    <th>Count</th>\n"
        "                </tr>\n"
        "                <xsl:for-each select=\"memory_log/memory_dump/alloc_unit"
        "[generate-id() = generate-id(key('sizes',@size)[1])]\">\n"
        "                <xsl:sort select=\"@size\" data-type=\"number\"/>\n"
        "                <tr>\n"
        "                    <td><xsl:value-of select=\"@size\"/></td>\n"
        "                    <td><xsl:value-of select=\"count(../alloc_unit["
        "@size=current()/@size])\"/></td>\n"
        "                </tr>\n"
        "                </xsl:for-each>\n"
        "\n"
        "            </tbody>\n"
        "        </table>\n"
        "\n"
        "</body>\n"
        "\n"
        "</html>\n"
        "\n"
        "</xsl:template>\n"
        "\n"
        "</xsl:stylesheet>\n";

    char acLogPath[efd::EE_MAX_PATH];
    EE_VERIFY(efd::PathUtils::GetDefaultLogDirectory(acLogPath, efd::EE_MAX_PATH));

    char acFilename[efd::EE_MAX_PATH];

    efd::Sprintf(acFilename, efd::EE_MAX_PATH, "%smemoryreport.xsl", acLogPath);

    // Only write the XSL file if it doesn't already exist
    if (!efd::File::Access(acFilename, efd::File::READ_ONLY))
    {
        efd::File* pkFile = efd::File::GetFile(acFilename, efd::File::WRITE_ONLY);
        if (pkFile)
        {
            pkFile->PutS(pcXSLFile);
            EE_DELETE pkFile;
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::IMemLogHandler* CreateDefaultMemoryLogHandler()
{
    efd::IMemLogHandler* pkLogHandler = EE_NEW efd::XMLMemLogHandler();

    return pkLogHandler;
}

#if defined(EE_MEMTRACKER_SNAPSHOT)

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::BeginSnapshotLog(const char* pszSuffix)
{
    if (!m_pSummaryXMLFile)
    {
        char acLogPath[efd::EE_MAX_PATH];
        EE_VERIFY(PathUtils::GetDefaultLogDirectory(acLogPath, efd::EE_MAX_PATH));

        char acSummaryPath[efd::EE_MAX_PATH];
        efd::Sprintf(acSummaryPath, EE_MAX_PATH, "%sMemory-Snapshot-%s.xml", acLogPath, pszSuffix);

        m_pSummaryXMLFile = efd::File::GetFile(acSummaryPath, efd::File::WRITE_ONLY);
        if (m_pSummaryXMLFile)
        {
            struct tm* pkSysTime = NULL;
            struct tm* pkLocalTime = NULL;
            GetSysAndLocalTime(pkSysTime, pkLocalTime);
            EE_MEMASSERT(pkSysTime && pkLocalTime);
            WriteLogHeader(m_pSummaryXMLFile, "memorysnapshot.xsl", pkSysTime, pkLocalTime);
            WriteTextToFile(m_pSummaryXMLFile, "<active_memory_dump timestamp='%f' >\n",
                GetCurrentTimeInSec());
        }
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::LogAllocUnitSnapshot(const AllocUnit *pkUnit)
{
    if (m_pSummaryXMLFile)
    {
        WriteAllocUnitXMLBlock(m_pSummaryXMLFile, pkUnit, "\t", true);
    }
}

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::EndSnapshotLog()
{
    if (m_pSummaryXMLFile)
    {
        WriteTextToFile(m_pSummaryXMLFile, "</active_memory_dump>\n</memory_log>\n");
        m_pSummaryXMLFile->Flush();
        EE_DELETE m_pSummaryXMLFile;
        m_pSummaryXMLFile = NULL;
    }
}
#endif // defined(EE_MEMTRACKER_SNAPSHOT)

//------------------------------------------------------------------------------------------------
void XMLMemLogHandler::WriteLogHeader(
    efd::File* pFile,
    const char* pszXslFile,
    struct tm* pkSysTime,
    struct tm* pkLocalTime)
{
    WriteTextToFile(pFile,
        "<?xml version=\"1.0\"?>\n"
        "<?xml-stylesheet type=\"text/xsl\" href=\"%s\"?>\n"
        "<memory_log date=\"%02d/%02d/%04d - %2d:%02d:%02d UTC (%2d:%02d:%02d local)\">\n",
        pszXslFile,
        pkSysTime->tm_mon+1, pkSysTime->tm_mday, 1900+pkSysTime->tm_year,
        pkSysTime->tm_hour, pkSysTime->tm_min, pkSysTime->tm_sec,
        pkLocalTime->tm_hour, pkLocalTime->tm_min, pkLocalTime->tm_sec);
}

//------------------------------------------------------------------------------------------------
} // end namespace efd
