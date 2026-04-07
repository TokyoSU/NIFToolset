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

#include <stdio.h>
#include <stdarg.h>
#include <efd/FileDestination.h>
#include <efd/PathUtils.h>
#include <efd/ILogger.h>

using namespace efd;

//--------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FileDestination);

//--------------------------------------------------------------------------------------------------
FileDestination::FileDestination(const efd::utf8string& name,
                                  const efd::utf8string& fileName,
                                  FileOption option /*= kFileAppend*/,
                                  efd::Bool fileInfoWithMsg /*= false*/,
                                  efd::Bool fileInfoWithAssert /*= true*/,
                                  efd::Bool flushOnWrite /*= false*/)
    : ILogDestination(name)
    , m_fileName(fileName)
    , m_option(option)
    , m_fileWithMsg(fileInfoWithMsg)
    , m_fileWithAssert(fileInfoWithAssert)
    , m_flushOnWrite(flushOnWrite)
    , m_pLogFile(NULL)
{
    if (!PathUtils::GetPlatformSupportsRelativePaths())
    {
        if (PathUtils::IsRelativePath(fileName))
        {
            m_fileName = PathUtils::ConvertToAbsolute(fileName);
        }
    }
}

//--------------------------------------------------------------------------------------------------
FileDestination::~FileDestination()
{
    // Check for a log file and close it if there is one
    EE_DELETE m_pLogFile;
    m_pLogFile = NULL;

    m_uniqueIndex.ReleaseIndex();
}

//--------------------------------------------------------------------------------------------------
/*virtual*/
efd::Bool FileDestination::OnInit()
{
#if !defined (EE_DISABLE_LOGGING)
    // if the string is 0 length, print and error and abort.
    // the caller is responsible for specifying a filename.
    if (m_fileName.empty())
    {
        fprintf(
            stderr,
            "ERROR: Tried to open FileDestination with empty filename\n");

        return false;
    }

    // Figure out what file options to use
    efd::File::OpenMode eFileOption;
    switch (m_option)
    {
    case kFileAppend:
        eFileOption = efd::File::APPEND_ONLY_TEXT;
        break;

    case kUniqueFileName:
        // Move the current log file out of the way
        UniqueFileName(m_fileName);
        // Fall through on purpose ...
    case kFileOverwrite:
        eFileOption = efd::File::WRITE_ONLY_TEXT;
        break;

    case kIndexedFileName:
        m_uniqueIndex.AquireIndex(m_fileName);
        IndexedFileName(m_fileName);
        eFileOption = efd::File::WRITE_ONLY_TEXT;
        break;

    default:
        fprintf(stderr, "ERROR: Unknown file option\n");
        return false;
        break;
    }
    // try to open the file handle with 1k buffer.
    m_pLogFile = efd::File::GetFile(m_fileName.c_str(), eFileOption, 1024, m_flushOnWrite);

    if ((m_pLogFile != NULL) && (m_option != kFileAppend))
    {
        // If this is a new file write the header
        efd::Char acBuff[EE_MAX_PATH];
        efd::Sprintf(acBuff, EE_MAX_PATH,
            "TimeStamp|Level|Module|File|Line|Message|\n");
        m_pLogFile->Write(acBuff, efd::Strlen(acBuff));
    }

    return (m_pLogFile != NULL);
#else
    return true;
#endif
}

//--------------------------------------------------------------------------------------------------
void FileDestination::Flush()
{
    m_fileLock.Lock();

    if (m_pLogFile)
        m_pLogFile->Flush();

    m_fileLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
void FileDestination::BeginLog(
    efd::Bool assert,
    const char* timeStampMachine,
    efd::TimeType timeStampGame,
    const char* module,
    const char* level,
    const char* file,
    efd::SInt32 line)
{
    EE_UNUSED_ARG(timeStampGame);

    m_fileLock.Lock();

    efd::SInt32 numPrinted = 0;

    // |time|level|module|||msg|
    if ((!assert && !m_fileWithMsg) || (assert && !m_fileWithAssert))
    {
        efd::Char acBuff[1024];
        numPrinted = efd::Sprintf(acBuff, EE_ARRAYSIZEOF(acBuff), "%s|%s|%s|||",
            timeStampMachine, level, module);
        m_pLogFile->Write(acBuff, numPrinted);
    }
    else // |time|level|module|file|line|msg|
    {
        efd::Char acBuff[1024];
        numPrinted = efd::Sprintf(acBuff, EE_ARRAYSIZEOF(acBuff), "%s|%s|%s|%s|%d|",
            timeStampMachine, level, module, file, line);
        m_pLogFile->Write(acBuff, numPrinted);
    }
}

//--------------------------------------------------------------------------------------------------
void FileDestination::ContinueLog(const char* pMsg)
{
    m_pLogFile->Write(pMsg, efd::Strlen(pMsg));
}

//--------------------------------------------------------------------------------------------------
void FileDestination::EndLog()
{
    m_pLogFile->Write("|\n", 2);

    m_fileLock.Unlock();
}


//--------------------------------------------------------------------------------------------------
void FileDestination::UniqueFileName(efd::utf8string& fileName)
{
    // Verify existing file exists
    if (!efd::File::Access(fileName.c_str(), efd::File::READ_ONLY))
        return;

    // Pull the filename apart
    efd::utf8string name;
    efd::utf8string type;

    efd::utf8string::size_type pos = fileName.rfind('.');
    name = fileName.substr(0, pos);
    type = fileName.substr(pos + 1);

    // Create a new filename
    efd::UInt16 index = 1;
    fileName.sprintf("%s-%04i.%s", name.c_str(), index, type.c_str());

    // Check to see if a file exists with the new file name
    while (efd::File::Access(fileName.c_str(), efd::File::READ_ONLY))
    {
        // If failure, Create a new filename and try again
        fileName.sprintf("%s-%04i.%s", name.c_str(), ++index, type.c_str());
    }
}

//--------------------------------------------------------------------------------------------------
efd::SmartPointer<FileDestination> FileDestination::CreateAndOpen(
    ILogger* pLogger,
    const efd::utf8string& name,
    FileOption option,
    efd::Bool fileInfoWithMsg,
    efd::Bool fileInfoWithAssert,
    efd::Bool flushOnWrite)
{
#if !defined(EE_DISABLE_LOGGING)
    EE_ASSERT(pLogger);
    efd::utf8string logFileName = PathUtils::PathAddExtension(name, "log");
    FileDestinationPtr spLogDest =
        EE_NEW FileDestination(
            name,
            logFileName,
            option,
            fileInfoWithMsg,
            fileInfoWithAssert,
            flushOnWrite);
    if (!pLogger->AddDest(spLogDest, true))
    {
        WriteToStdErr("Failed to open log file: ");
        WriteToStdErr(logFileName.c_str());
        WriteToStdErr("\n");
        EE_OUTPUT_DEBUG_STRING("Failed to open log file: ");
        EE_OUTPUT_DEBUG_STRING(logFileName.c_str());
        EE_OUTPUT_DEBUG_STRING("\n");
        return NULL;
    }
    return spLogDest;
#else
    EE_UNUSED_ARG(pLogger);
    EE_UNUSED_ARG(name);
    EE_UNUSED_ARG(option);
    EE_UNUSED_ARG(fileInfoWithMsg);
    EE_UNUSED_ARG(fileInfoWithAssert);
    EE_UNUSED_ARG(flushOnWrite);
    return NULL;
#endif //!defined(EE_DISABLE_LOGGING)
}

