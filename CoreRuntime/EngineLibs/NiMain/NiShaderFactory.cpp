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

#include "NiShaderFactory.h"
#include "NiShader.h"
#include "NiShaderLibrary.h"
#include "NiRenderObject.h"
#include "NiRenderer.h"
#include "NiShaderParser.h"

//--------------------------------------------------------------------------------------------------
NiShaderFactory* NiShaderFactory::ms_pkShaderFactory = 0;
NiTPrimitiveSet<NiShaderFactory::LibraryCallbackInfo>*
    NiShaderFactory::ms_pkLibraryCallbacks = NULL;
//--------------------------------------------------------------------------------------------------
NiShaderFactory::NiShaderFactory() :
    m_pfnClassCreate(&NiShaderFactory::DefaultCreateClass),
    m_pfnRunParser(&NiShaderFactory::DefaultRunParser),
    m_pfnErrorCallback(0)
{
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::~NiShaderFactory()
{
    ReleaseAllShaders();
    UnregisterAllLibraries();

    // Remove all global constant map entries
    NiTMapIterator kIter = m_kGlobalConstantMap.GetFirstPos();
    while (kIter)
    {
        NiFixedString kName;
        NiGlobalConstantEntry* pkEntry = NULL;
        m_kGlobalConstantMap.GetNext(kIter, kName, pkEntry);
        if (pkEntry)
            ReleaseGlobalShaderConstant(kName);
    }

    if (ms_pkShaderFactory == this)
        ms_pkShaderFactory = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::_SDMInit()
{
    ms_pkLibraryCallbacks = NiNew NiTPrimitiveSet<LibraryCallbackInfo>;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::_SDMShutdown()
{
    NiDelete ms_pkLibraryCallbacks;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::Shutdown()
{
    NiDelete ms_pkShaderFactory;
    ms_pkShaderFactory = 0;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory* NiShaderFactory::GetInstance()
{
    return ms_pkShaderFactory;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::ReleaseAllShaders()
{
    if (!ms_pkShaderFactory)
        return;

    ms_pkShaderFactory->RemoveAllShaders();
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::IsDefaultImplementation(NiShader*)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RegisterLibrary(NiShaderLibrary* pkLibrary)
{
    if (!pkLibrary || !ms_pkShaderFactory)
        return;

    // Make sure it is not already in the map
    NiShaderLibraryPtr spLibrary = ms_pkShaderFactory->FindLibrary(
        pkLibrary->GetName());
    if (spLibrary)
    {
        // It is already in the map...
        if (spLibrary != pkLibrary)
        {
            // They are different!
            EE_FAIL("RegisterLibrary> Two different libraries w/ the "
                "same name!");
        }
    }

    ms_pkShaderFactory->InsertLibrary(pkLibrary);
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::UnregisterLibrary(NiShaderLibrary* pkLibrary)
{
    if (!ms_pkShaderFactory || !pkLibrary)
        return;

    ms_pkShaderFactory->RemoveLibrary(pkLibrary->GetName());
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::UnregisterAllLibraries()
{
    if (!ms_pkShaderFactory)
        return;

    ms_pkShaderFactory->RemoveAllLibraries();
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::RegisterClassCreationCallback(
    NISHADERFACTORY_CLASSCREATIONCALLBACK pfnCallback)
{
    if (!ms_pkShaderFactory)
        return false;

    ms_pkShaderFactory->m_pfnClassCreate = pfnCallback;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::LoadAndRegisterShaderLibrary(const char* pacLibName,
    int iDirectoryCount, const char* pacDirectories[], bool bRecurseSubFolders)
{
    if (!ms_pkShaderFactory)
        return false;

    NiShaderFactory* pkFactory = ms_pkShaderFactory;


    if (pkFactory->m_pfnClassCreate == 0)
    {
        ReportError(NISHADERERR_UNKNOWN, false,
            "LoadAndRegisterShaderLibrary: No valid ClassCreate call\n");
        return false;
    }

    NiShaderLibrary* pkLibrary = NULL;
    bool bSuccess = pkFactory->m_pfnClassCreate(pacLibName,
        NiRenderer::GetRenderer(), iDirectoryCount, pacDirectories,
        bRecurseSubFolders, &pkLibrary);

    if (bSuccess && pkLibrary)
    {
        pkFactory->InsertLibrary(pkLibrary);
    }

    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::RegisterRunParserCallback(
    NISHADERFACTORY_RUNPARSERCALLBACK pfnCallback)
{
    if (!ms_pkShaderFactory)
        return false;

    ms_pkShaderFactory->m_pfnRunParser = pfnCallback;

    return true;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderFactory::LoadAndRunParserLibrary(
    const char* pacLibName, const char* pacDirectory,
    bool bRecurseSubFolders)
{
    if (!ms_pkShaderFactory)
    {
        return 0;
    }

    if (ms_pkShaderFactory->m_pfnRunParser == 0)
    {
        NiShaderFactory::ReportError(NISHADERERR_UNKNOWN, false,
            "LoadAndRegisterShaderLibrary: No valid RunParser call\n");
        return 0;
    }

    unsigned int uiNumParsedFiles = ms_pkShaderFactory->m_pfnRunParser(
        pacLibName, NiRenderer::GetRenderer(), pacDirectory,
        bRecurseSubFolders);

    return uiNumParsedFiles;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::RegisterErrorCallback(
    NISHADERFACTORY_ERRORCALLBACK pfnCallback)
{
    if (!ms_pkShaderFactory)
        return false;

    ms_pkShaderFactory->m_pfnErrorCallback = pfnCallback;

    return true;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderFactory::ReportError(NiShaderError eError,
    bool bRecoverable, const char* pacFmt, ...)
{
    if (!ms_pkShaderFactory)
        return false;

    char acError[2048];
    va_list args;
    va_start(args, pacFmt);
    NiVsprintf(acError, 2048, pacFmt, args);
    va_end(args);

    if (ms_pkShaderFactory->m_pfnErrorCallback)
    {
        return ms_pkShaderFactory->m_pfnErrorCallback(acError, eError,
            bRecoverable);
    }

    // Not defined... just log it via NiOutputDebugString
    NiOutputDebugString("ERROR: ");
    NiOutputDebugString(acError);
    NiOutputDebugString("Error is ");
    if (!bRecoverable)
        NiOutputDebugString("NOT ");
    NiOutputDebugString("recoverable!\n");

    return 0;
}

//--------------------------------------------------------------------------------------------------
NiShader* NiShaderFactory::RetrieveShader(const char*, unsigned int, bool)
{
    // Base does nothing
    return 0;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::InsertShader(NiShader*, unsigned int)
{
    // The base implementation does nothing...
}

//--------------------------------------------------------------------------------------------------
NiShader* NiShaderFactory::FindShader(const char*, unsigned int)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::ReleaseShaderFromLibrary(const char*, unsigned int)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::ReleaseShaderFromLibrary(NiShader*)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveShader(const char*, unsigned int)
{
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveAllShaders()
{
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::InsertLibrary(NiShaderLibrary*)
{
    // The base implementation does nothing...
}

//--------------------------------------------------------------------------------------------------
NiShaderLibrary* NiShaderFactory::FindLibrary(const char*)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveLibrary(const char*)
{
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveAllLibraries()
{
}

//--------------------------------------------------------------------------------------------------
#if defined(_USRDLL)
void NiShaderFactory::FreeLibraryDLLs()
{
}

//--------------------------------------------------------------------------------------------------
void* NiShaderFactory::GetFirstLibraryDLL(const char*&)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
void* NiShaderFactory::GetNextLibraryDLL(const char*&)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::ClearLibraryDLLs()
{
}

//--------------------------------------------------------------------------------------------------
#endif
NiGlobalConstantEntry* NiShaderFactory::GetFirstGlobalShaderConstant(
    NiTMapIterator& kIter, NiFixedString& kName)
{
    kIter = m_kGlobalConstantMap.GetFirstPos();
    if (kIter)
    {
        NiGlobalConstantEntry* pkEntry = NULL;
        m_kGlobalConstantMap.GetNext(kIter, kName, pkEntry);
        if (pkEntry)
            return pkEntry;
    }
    kName = "";
    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiGlobalConstantEntry* NiShaderFactory::GetNextGlobalShaderConstant(
    NiTMapIterator& kIter, NiFixedString& kName)
{
    if (kIter)
    {
        NiGlobalConstantEntry* pkEntry = NULL;
        m_kGlobalConstantMap.GetNext(kIter, kName, pkEntry);
        if (pkEntry)
            return pkEntry;
    }
    kName = "";
    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_CLASSCREATIONCALLBACK
    NiShaderFactory::GetClassCreateCallback() const
{
    return m_pfnClassCreate;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_RUNPARSERCALLBACK
    NiShaderFactory::GetRunParserCallback() const
{
    return m_pfnRunParser;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_ERRORCALLBACK
    NiShaderFactory::GetErrorCallback() const
{
    return m_pfnErrorCallback;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_CLASSCREATIONCALLBACK
    NiShaderFactory::GetDefaultClassCreateCallback() const
{
    return &NiShaderFactory::DefaultCreateClass;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_RUNPARSERCALLBACK
    NiShaderFactory::GetDefaultRunParserCallback() const
{
    return &NiShaderFactory::DefaultRunParser;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_ERRORCALLBACK
    NiShaderFactory::GetDefaultErrorCallback() const
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiShaderLibrary* NiShaderFactory::GetFirstLibrary()
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
NiShaderLibrary* NiShaderFactory::GetNextLibrary()
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
// Global shader constant mappings
//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::RegisterGlobalShaderConstant(
    const NiFixedString& kKey, NiShaderAttributeDesc::AttributeType eType,
    unsigned int uiDataSize, const void* pvInitialData)
{
    if (!ms_pkShaderFactory)
        return false;

    // See if the global is already registered...
    NiGlobalConstantEntry* pkEntry =
        ms_pkShaderFactory->GetGlobalShaderConstantEntry(kKey);

    if (pkEntry)
    {
        // Ensure that it is the same type!
        if (pkEntry->GetType() != eType)
        {
            EE_FAIL("NiRenderer::RegisterGlobalShaderConstant> Entry "
                "type conflict!");
            return false;
        }

        // Inc the ref count manually!
        pkEntry->IncRefCount();
        return true;
    }

    // Wasn't in the map, so create it!
    pkEntry = NiNew NiGlobalConstantEntry(kKey, eType, uiDataSize,
        pvInitialData);
    if (!pkEntry)
    {
        EE_FAIL("NiRenderer::RegisterGlobalShaderConstant> Failed to "
            "create entry!");
        return false;
    }

    // Hand increment the ref count
    pkEntry->IncRefCount();

    // Add it to the map
    NiFixedString kGlobalKey = kKey;
    ms_pkShaderFactory->m_kGlobalConstantMap.SetAt(kGlobalKey,
        pkEntry);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::ReleaseGlobalShaderConstant(const NiFixedString& kKey)
{
    if (!ms_pkShaderFactory)
        return false;

    NiGlobalConstantEntry* pkEntry =
        ms_pkShaderFactory->GetGlobalShaderConstantEntry(kKey);

    if (!pkEntry)
        return false;

    NiFixedString kRemoveKey = kKey;
    if (pkEntry->GetRefCount() == 1)
        ms_pkShaderFactory->m_kGlobalConstantMap.RemoveAt(kRemoveKey);

    pkEntry->DecRefCount();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::RetrieveGlobalShaderConstant(
    const NiFixedString& kKey, unsigned int& uiDataSize, const void*& pvData)
{
    if (!ms_pkShaderFactory)
        return false;

    NiGlobalConstantEntry* pkEntry =
        ms_pkShaderFactory->GetGlobalShaderConstantEntry(kKey);

    if (!pkEntry)
        return false;

    uiDataSize = pkEntry->GetDataSize();
    pvData = pkEntry->GetDataSource();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::UpdateGlobalShaderConstant(const NiFixedString& kKey,
    unsigned int uiDataSize, const void* pvData)
{
    if (!ms_pkShaderFactory)
        return false;

    NiGlobalConstantEntry* pkEntry =
        ms_pkShaderFactory->GetGlobalShaderConstantEntry(kKey);

    if (!pkEntry)
        return false;

    pkEntry->SetData(uiDataSize, pvData);

    return true;
}

//--------------------------------------------------------------------------------------------------
NiGlobalConstantEntry* NiShaderFactory::GetGlobalShaderConstantEntry(
    const NiFixedString& kKey)
{
    NiGlobalConstantEntry* pkEntry;

    if (ms_pkShaderFactory->m_kGlobalConstantMap.GetAt(kKey, pkEntry))
        return pkEntry;

    return 0;
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderFactory::GetFirstShaderProgramFileDirectory(
    NiTListIterator& kIter)
{
    if (ms_pkShaderFactory)
    {
        return ms_pkShaderFactory->GetFirstProgramDirectory(kIter);
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderFactory::GetNextShaderProgramFileDirectory(
    NiTListIterator& kIter)
{
    if (ms_pkShaderFactory)
    {
        return ms_pkShaderFactory->GetNextProgramDirectory(kIter);
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::AddShaderProgramFileDirectory(const char* pacDirectory)
{
    if (ms_pkShaderFactory)
        ms_pkShaderFactory->AddProgramDirectory(pacDirectory);
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveShaderProgramFileDirectory(
    const char* pacDirectory)
{
    if (ms_pkShaderFactory)
        ms_pkShaderFactory->RemoveProgramDirectory(pacDirectory);
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveAllShaderProgramFileDirectories()
{
    if (ms_pkShaderFactory)
        ms_pkShaderFactory->RemoveAllProgramDirectories();
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::AddLibraryCallback(const char* pcName,
    NISHADERLIBRARY_CLASSCREATIONCALLBACK pfnCallback)
{
    unsigned int uiEmpty = ms_pkLibraryCallbacks->GetSize();

    for (unsigned int i = 0; i < ms_pkLibraryCallbacks->GetSize(); i++)
    {
        if (!ms_pkLibraryCallbacks->GetAt(i).pcName &&
            !ms_pkLibraryCallbacks->GetAt(i).pfnCallback)
        {
            uiEmpty = i;
        }

        if (strcmp(ms_pkLibraryCallbacks->GetAt(i).pcName, pcName))
            continue;

        ms_pkLibraryCallbacks->GetAt(i).pfnCallback = pfnCallback;
        return;
    }

    if (uiEmpty >= ms_pkLibraryCallbacks->GetSize())
        uiEmpty = ms_pkLibraryCallbacks->Add(LibraryCallbackInfo());

    ms_pkLibraryCallbacks->GetAt(uiEmpty).pcName = pcName;
    ms_pkLibraryCallbacks->GetAt(uiEmpty).pfnCallback = pfnCallback;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveLibraryCallback(
    NISHADERLIBRARY_CLASSCREATIONCALLBACK pfnCallback)
{
    bool bFound = false;
    unsigned int ui = 0;
    for (ui = 0; ui < ms_pkLibraryCallbacks->GetSize(); ui++)
    {
        if (ms_pkLibraryCallbacks->GetAt(ui).pfnCallback == pfnCallback)
        {
            bFound = true;
            break;
        }
    }

    if (bFound)
        ms_pkLibraryCallbacks->RemoveAt(ui);
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderFactory::GetNumLibraryCallbacks()
{
    return ms_pkLibraryCallbacks->GetSize();
}

//--------------------------------------------------------------------------------------------------
const char* NiShaderFactory::GetLibraryName(unsigned int uiIdx)
{
    return ms_pkLibraryCallbacks->GetAt(uiIdx).pcName;
}

//--------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERLIBRARY_CLASSCREATIONCALLBACK
    NiShaderFactory::GetLibraryCallback(unsigned int uiIdx)
{
    return ms_pkLibraryCallbacks->GetAt(uiIdx).pfnCallback;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiShaderFactory::DefaultRunParser(const char* pcLibName,
    NiRenderer*, const char* pcDirectory, bool bRecurseSubFolders)
{
    if (!ms_pkShaderFactory)
        return 0;

    // Has this parser already been loaded?
    unsigned int uiLoadedLibs = NiShaderParser::GetNumParserCallbacks();
    for (unsigned int i = 0; i < uiLoadedLibs; i++)
    {
        if (strcmp(pcLibName, NiShaderParser::GetParserName(i)))
            continue;

        NiShaderParser::NISHADERPARSER_CLASSCREATIONCALLBACK pfnCallback =
            NiShaderParser::GetParserCallback(i);
        NiShaderParser* pkParser = pfnCallback();
        unsigned int uiCount;
        pkParser->ParseAllFiles(pcDirectory, bRecurseSubFolders, uiCount);
        return uiCount;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::DefaultCreateClass(const char* pcLibName,
    NiRenderer* pkRenderer, int iDirectoryCount, const char* apcDirectories[],
    bool bRecurseSubFolders, NiShaderLibrary** ppkLibrary)
{
    *ppkLibrary = NULL;

    if (!ms_pkShaderFactory)
        return false;

    // Has this library already been loaded?
    unsigned int uiLoadedLibs = NiShaderFactory::GetNumLibraryCallbacks();
    for (unsigned int i = 0; i < uiLoadedLibs; i++)
    {
        if (strcmp(pcLibName, NiShaderFactory::GetLibraryName(i)))
            continue;

        NiShaderFactory::NISHADERLIBRARY_CLASSCREATIONCALLBACK pfnCallback =
            NiShaderFactory::GetLibraryCallback(i);
        return pfnCallback(pkRenderer, iDirectoryCount, apcDirectories,
            bRecurseSubFolders, ppkLibrary);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::SetShaderProgramLocation(const char* pcFile,
    const char* pcPath)
{
    char acBuffer[NI_MAX_PATH];
    for (unsigned int i = 0; i < NI_MAX_PATH; i++)
    {
        acBuffer[i] = (char)tolower(pcFile[i]);
        if (acBuffer[i] == '\0')
        {
            m_kProgramMap.SetAt(acBuffer, pcPath);
            return;
        }
    }

    EE_FAIL("Path too long.");
}

//--------------------------------------------------------------------------------------------------
bool NiShaderFactory::GetShaderProgramLocation(
    const NiFixedString& kFile, NiFixedString& kOutput) const
{
    char acBuffer[NI_MAX_PATH];
    for (unsigned int i = 0; i < NI_MAX_PATH; i++)
    {
        acBuffer[i] = (char)tolower(kFile[i]);
        if (acBuffer[i] == '\0')
            return m_kProgramMap.GetAt(acBuffer, kOutput);
    }

    EE_FAIL("Path too long.");
    return false;
}

//--------------------------------------------------------------------------------------------------
void NiShaderFactory::RemoveAllShaderProgramLocations()
{
    m_kProgramMap.RemoveAll();
}

//--------------------------------------------------------------------------------------------------