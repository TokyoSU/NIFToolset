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

#ifndef NIFILE_H
#define NIFILE_H

#include "NiRTLib.h"
#include "NiBinaryStream.h"

#if !defined(WIN32)
#error One of these macros must be defined: WIN32
#endif

class NISYSTEM_ENTRY NiFile : public NiBinaryStream
{
     NiDeclareDerivedBinaryStream();

public:
    typedef enum
    {
        READ_ONLY,
        WRITE_ONLY,
        APPEND_ONLY
    } OpenMode;
    
    NiFile(const char* pcName, OpenMode eMode,
        unsigned int uiBufferSize = 32768);
    virtual ~NiFile();

    // GetFile is used throughout Gamebryo to create NiFile objects. Use 
    // SetFileCreateFunc to override its behavior (to create an instance of 
    // an NiFile-derived class rather than an NiFile).
    static NiFile* GetFile(const char *pcName, OpenMode eMode,
        unsigned int uiBufferSize = 32768);

    typedef NiFile* (*FILECREATEFUNC)(const char *pcName, 
        OpenMode eMode, unsigned int uiBufferSize);

    // Set the file creation function or restore to default
    // if pfnFunc is NULL.
    static void SetFileCreateFunc(FILECREATEFUNC pfnFunc);
    
    // Check if a file exists with permissions defined by eMode.
    static bool Access(const char* pcName, OpenMode eMode);

    // Override the behavior of NiFile::Access in the same way that
    // NiFile::SetFileCreateFunc overrides NiFile::GetFile:
    typedef bool (*FILEACCESSFUNC)(const char* pcName, OpenMode eMode);
    static void SetFileAccessFunc(FILEACCESSFUNC pfnFunc);

    // Create a single directory
    static bool CreateDirectory(const char* pcDirName);
    static bool DirectoryExists(const char* pcDirName);

    // Override the behavior of NiFile::CreateDirectory in the same way that
    // NiFile::SetFileCreateFunc overrides NiFile::GetFile:
    typedef bool (*CREATEDIRFUNC)(const char* pcName);
    static void SetCreateDirectoryFunc(CREATEDIRFUNC pfnFunc);

    // Override the behavior of NiFile::DirectoryExists in the same way that
    // NiFile::SetFileCreateFunc overrides NiFile::GetFile:
    typedef bool (*DIREXISTSFUNC)(const char* pcName);
    static void SetDirectoryExistsFunc(DIREXISTSFUNC pfnFunc);

    // Recursively check to see if all directories exist in the path. If not,
    // create them one at a time.
    static bool CreateDirectoryRecursive(const char* pcFullPath);

    virtual operator bool() const;

    virtual void Seek(int iNumBytes);
    virtual void Seek(int iOffset, int iWhence);
    static const int ms_iSeekSet;
    static const int ms_iSeekCur;
    static const int ms_iSeekEnd;

    virtual unsigned int GetFileSize() const;

    virtual void SetEndianSwap(bool bDoSwap);

protected:
    NiFile();
    bool Flush();
    unsigned int DiskWrite(const void* pvBuffer, unsigned int uiBytes);

    static FILECREATEFUNC ms_pfnFileCreateFunc;
    static FILEACCESSFUNC ms_pfnFileAccessFunc;
    static CREATEDIRFUNC  ms_pfnCreateDirFunc;
    static DIREXISTSFUNC  ms_pfnDirExistsFunc;

    static NiFile* DefaultFileCreateFunc(const char *pcName, OpenMode eMode, unsigned int uiBufferSize);
    static bool DefaultFileAccessFunc(const char *pcName, OpenMode eMode);
    static bool DefaultCreateDirectoryFunc(const char* pcDir);
    static bool DefaultDirectoryExistsFunc(const char* pcDir);

    unsigned int m_uiBufferAllocSize;
    unsigned int m_uiBufferReadSize;
    unsigned int m_uiPos;

    unsigned int DiskRead(void* pvBuffer, unsigned int uiBytes);

    char* m_pBuffer;
    FILE* m_pFile;
    OpenMode m_eMode;
    bool m_bGood;

    // Read or write a chunk of data from a file (no endian swapping)
    unsigned int FileRead(void* pvBuffer, unsigned int uiBytes);
    unsigned int FileWrite(const void* pvBuffer, unsigned int uiBytes);
};

#endif // NIFILE_H

