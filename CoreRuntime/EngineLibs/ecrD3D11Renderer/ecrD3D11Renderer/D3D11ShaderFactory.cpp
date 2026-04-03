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
#include "ecrD3D11RendererPCH.h"

#include "D3D11ShaderFactory.h"
#include "D3D11ShaderLibrary.h"
#include "D3D11Error.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderProgramFactory.h"

#include <NiShaderLibraryInterface.h>
#include <NiVersion.h>

using namespace ecr;

D3D11ShaderFactory* D3D11ShaderFactory::ms_pD3D11ShaderFactory = NULL;

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::_SDMInit()
{
    D3D11ShaderFactory* pFactory = (D3D11ShaderFactory*)D3D11ShaderFactory::Create();
    if (!pFactory)
    {
        D3D11Renderer::Error("Failed to create shader factory!");
    }

    pFactory->m_pfnClassCreate = pFactory->GetDefaultClassCreateCallback();
    pFactory->m_pfnRunParser = pFactory->GetDefaultRunParserCallback();
    pFactory->m_pfnErrorCallback = pFactory->GetDefaultErrorCallback();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::_SDMShutdown()
{
    D3D11ShaderFactory::Destroy();
}

//------------------------------------------------------------------------------------------------
NiShaderFactory* D3D11ShaderFactory::Create()
{
    if (ms_pD3D11ShaderFactory == 0)
    {
        D3D11ShaderFactory* pFactory = EE_NEW D3D11ShaderFactory();
        EE_ASSERT(pFactory);
        ms_pD3D11ShaderFactory = pFactory;
    }

    // Set the ms_pkShaderFactory variable only if it is not currently set.
    if (ms_pkShaderFactory == NULL)
        ms_pkShaderFactory = ms_pD3D11ShaderFactory;

    return ms_pD3D11ShaderFactory;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::Destroy()
{
    if (ms_pD3D11ShaderFactory)
    {
        efd::Bool releaseFactory = false;
        if (IsActiveFactory())
            releaseFactory = true;

        EE_DELETE ms_pD3D11ShaderFactory;
        ms_pD3D11ShaderFactory = NULL;

        if (releaseFactory)
            ms_pkShaderFactory = NULL;
    }
}

//------------------------------------------------------------------------------------------------
D3D11ShaderFactory::D3D11ShaderFactory() :
    m_shaderMap(59),
    m_libraryMap(37),
#if defined(_USRDLL)
    m_libraryDLLIter(NULL),
#endif //#if defined(_USRDLL)
    m_libraryIter(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShaderFactory::~D3D11ShaderFactory()
{
    // We could call the PurgeAllShaderRendererData() function here to
    // guarantee that all renderer data is released. We will assume that if
    // the renderer has been deleted this will have already occurred, and if
    // it hasn't, the renderer will take care of it.

    RemoveAllShaders();
    RemoveAllLibraries();

#if defined(_USRDLL)
    FreeLibraryDLLs();
#endif //#if defined(_USRDLL)
}

//------------------------------------------------------------------------------------------------
#if defined(_USRDLL)
void D3D11ShaderFactory::FreeLibraryDLLs()
{
    RemoveAllShaders();
    RemoveAllLibraries();

    // Release libraries
    NiTMapIterator iter = m_loadedShaderLibDLLs.GetFirstPos();
    while (iter)
    {
        const efd::Char* pName;
        HMODULE hModule;
        m_loadedShaderLibDLLs.GetNext(iter, pName, hModule);
        if (hModule)
            FreeLibrary(hModule);
    }
    m_loadedShaderLibDLLs.RemoveAll();

    m_libraryDLLIter = NULL;
}

//------------------------------------------------------------------------------------------------
void* D3D11ShaderFactory::GetFirstLibraryDLL(const efd::Char*& pName)
{
    m_libraryDLLIter = m_loadedShaderLibDLLs.GetFirstPos();
    if (m_libraryDLLIter)
    {
        HMODULE hModule;
        NiTMapIterator iter = m_libraryDLLIter;
        m_loadedShaderLibDLLs.GetNext(iter, pName, hModule);
        m_libraryDLLIter = iter;
        if (hModule)
            return hModule;
    }
    pName = NULL;
    return NULL;
}

//------------------------------------------------------------------------------------------------
void* D3D11ShaderFactory::GetNextLibraryDLL(const efd::Char*& pName)
{
    if (m_libraryDLLIter)
    {
        HMODULE hModule;
        NiTMapIterator iter = m_libraryDLLIter;
        m_loadedShaderLibDLLs.GetNext(iter, pName, hModule);
        m_libraryDLLIter = iter;
        if (hModule)
            return hModule;
    }
    pName = NULL;
    return NULL;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::ClearLibraryDLLs()
{
    m_loadedShaderLibDLLs.RemoveAll();

    m_libraryDLLIter = NULL;
}

//------------------------------------------------------------------------------------------------
#endif //#if defined(_USRDLL)

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderFactory::DefaultCreateClass(
    const efd::Char* pLibName,
    NiRenderer* pRenderer, 
    efd::SInt32 directoryCount, 
    const efd::Char* pDirectoryArray[],
    efd::Bool recurseSubFolders, 
    NiShaderLibrary** pReturnLibrary)
{
    if (NiShaderFactory::DefaultCreateClass(
        pLibName, 
        pRenderer,
        directoryCount, 
        pDirectoryArray, 
        recurseSubFolders, 
        pReturnLibrary))
    {
        return true;
    }

#if !defined(_USRDLL)
    return false;
#else
    NISLI_LOADLIBRARY pLoadFunc = NULL;
    HMODULE hLibrary = GetModuleHandle(pLibName);
    if (hLibrary == NULL)
    {
        // Load the interface library first by explicit name, then
        // by the default Gamebryo name.
        hLibrary = LoadLibrary(pLibName);
        if (hLibrary == NULL)
        {
            efd::Char extendedLibName[512];
            efd::Sprintf(extendedLibName, sizeof(extendedLibName), "%s" NI_DLL_SUFFIX ".dll", pLibName);
            hLibrary = LoadLibrary(extendedLibName);
        }

        if (hLibrary == NULL)
        {
            ReportError(
                NISHADERERR_UNKNOWN, 
                false, 
                "LoadLibrary failed on %s\n"
                "Get last error returned 0x%08X.\n", 
                pLibName,
                GetLastError());
            return false;
        }

        // Ensure library was built with the same compiler
        NISLI_GETCOMPILERVERSIONFUNCTION pGetCompilerVersionFunc =
            (NISLI_GETCOMPILERVERSIONFUNCTION)GetProcAddress(hLibrary, "GetCompilerVersion");
        if (pGetCompilerVersionFunc)
        {
            efd::UInt32 compilerVersion = pGetCompilerVersionFunc();
            if (compilerVersion != (_MSC_VER))
            {
                ReportError(
                    NISHADERERR_UNKNOWN,
                    false,
                    "Shader Library %s\nwas built with different version of Visual Studio.\n\n"
                    "Library compiled with %s.\nThis application compiled with %s.", 
                    pLibName,
                    GetCompilerName(compilerVersion),
                    GetCompilerName(_MSC_VER));
                FreeLibrary(hLibrary);
                return false;
            }
        }
        else
        {
            // Older version - run it anyway
        }

        // Fetch the library creation function
        pLoadFunc = (NISLI_LOADLIBRARY)GetProcAddress(hLibrary, "LoadShaderLibrary");
        if (pLoadFunc == 0)
        {
            ReportError(
                NISHADERERR_UNKNOWN, 
                false,
                "Failed to retrieve 'LoadShaderLibrary' function from %s\n",
                pLibName);
            FreeLibrary(hLibrary);
            return false;
        }

        ms_pD3D11ShaderFactory->m_loadedShaderLibDLLs.SetAt(pLibName, hLibrary);
    }
    else
    {
        // It was found once; it should be findable again
        pLoadFunc = (NISLI_LOADLIBRARY)GetProcAddress(hLibrary, "LoadShaderLibrary");
        EE_ASSERT(pLoadFunc);
    }

    NiShaderLibrary* pLibrary = NULL;
    efd::Bool success = pLoadFunc(
        pRenderer, 
        directoryCount,
        pDirectoryArray, 
        recurseSubFolders, 
        &pLibrary);

    if (!success || pLibrary == NULL)
    {
        ReportError(
            NISHADERERR_UNKNOWN, 
            true,
            "Failed to load library from %s\n", 
            pLibName);
        return false;
    }

    NiShaderLibrary* pExistingLib = ms_pD3D11ShaderFactory->FindLibrary(pLibrary->GetName());
    if (pExistingLib && (pExistingLib != pLibrary))
    {
        D3D11Error::ReportWarning(
            __FUNCTION__
            "> Library %s has been reloaded!\n"
            "REPLACING EXISTING SHADER LIBRARY!\n", 
            pLibrary->GetName());

        ms_pD3D11ShaderFactory->RemoveLibrary(pLibrary->GetName());
        // Library will be released when it is removed from the library;
        // no need to delete it explicitly.
    }

    *pReturnLibrary = pLibrary;

    return true;
#endif
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::DefaultRunParser(
    const efd::Char* pLibName,
    NiRenderer* pRenderer, 
    const efd::Char* pDirectory, 
    efd::Bool recurseSubFolders)
{
    if (!ms_pD3D11ShaderFactory)
        return 0;

    efd::UInt32 libCount = NiShaderFactory::DefaultRunParser(
        pLibName,
        pRenderer,
        pDirectory,
        recurseSubFolders);

    if (libCount)
        return libCount;

#if !defined(_USRDLL)
    return libCount;
#else
    // Load the interface library
    HMODULE hLibrary = LoadLibrary(pLibName);
    if (hLibrary == NULL)
    {
        efd::Char extendedLibName[512];
        efd::Sprintf(extendedLibName, sizeof(extendedLibName), "%s" NI_DLL_SUFFIX ".dll", pLibName);
        hLibrary = LoadLibrary(extendedLibName);
    }

    if (hLibrary == NULL)
    {
        ReportError(
            NISHADERERR_UNKNOWN, 
            false, 
            "LoadLibrary failed on %s\n"
            "Get last error returned 0x%08X.\n", 
            pLibName,
            GetLastError());
        return 0;
    }

    // Ensure library was built with the same compiler
    NISLI_GETCOMPILERVERSIONFUNCTION pGetCompilerVersionFunc = 
        (NISLI_GETCOMPILERVERSIONFUNCTION)GetProcAddress(hLibrary, "GetCompilerVersion");
    if (pGetCompilerVersionFunc)
    {
        efd::UInt32 compilerVersion = pGetCompilerVersionFunc();
        if (compilerVersion != (_MSC_VER))
        {
            ReportError(NISHADERERR_UNKNOWN, 
                false,
                "Shader Library %s\nwas built with different version of Visual Studio.\n\n"
                "Library compiled with %s.\nThis application compiled with %s.", pLibName,
                GetCompilerName(compilerVersion), 
                GetCompilerName(_MSC_VER));
            FreeLibrary(hLibrary);
            return 0;
        }
    }
    else
    {
        // Older version - run it anyway
    }

    // Fetch the library parse function
    NISLI_RUNPARSER pParseFunc = (NISLI_RUNPARSER)GetProcAddress(hLibrary, "RunShaderParser");

    if (pParseFunc == 0)
    {
        ReportError(
            NISHADERERR_UNKNOWN, 
            false,
            "Failed to retrieve 'RunShaderParser' function from %s\n",
            pLibName);
        FreeLibrary(hLibrary);
        return 0;
    }

    libCount = pParseFunc(pDirectory, recurseSubFolders);

    FreeLibrary(hLibrary);

    return libCount;
#endif
}

//------------------------------------------------------------------------------------------------
NiShader* D3D11ShaderFactory::RetrieveShader(
    const efd::Char* pName,
    efd::UInt32 implementation, 
    efd::Bool reportNotFound)
{
    EE_ASSERT(IsActiveFactory());

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
    {
        D3D11Error::ReportWarning("Attempting to RetrieveShader without a valid renderer!\n");
        return NULL;
    }
    if (pName == NULL || pName[0] == '\0')
    {
        D3D11Error::ReportWarning("Attempting to RetrieveShader without a valid shader name!\n");
        return NULL;
    }

    D3D11Renderer::CreationParameters createParameters;
    pRenderer->GetCreationParameters(createParameters);

    if ((createParameters.m_createFlags & D3D11Renderer::CREATE_DEVICE_SINGLETHREADED) != 0 &&
        pRenderer->GetDeviceThreadID() != NiGetCurrentThreadId())
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN, 
            true,
            "RetrieveShader attempted to create shader (%s) on a thread that does not own "
            "the D3DDevice. Shader creation will be deferred until render time.\n This issue "
            "can be resolved by manually 'pre-loading' this shader by calling "
            "NiShaderFactory::RetrieveShader().\n", 
            pName, 
            pName);
        return 0;
    }

    NiShader* pShader = FindShader(pName, implementation);
    if (pShader == NULL)
    {
        // The shader is not currently loaded...
        // Check the libraries for it.

        m_shaderMapLock.Lock();

        // NOTE: Since we iterate over the libraries, if more than one
        // library contains a shader of the same name, the first one
        // found is the one returned. This means ALL SHADERS MUST HAVE
        // UNIQUE NAMES to avoid issues.
        NiShaderLibrary* pLibrary = GetFirstLibrary();

        // Atomically re-verify our assumption w.r.t. other shaders creation.
        pShader = FindShader(pName, implementation);

        if (pShader)
        {
            // Another shader created this resource before we could lock the
            // critical section so we're all set. We'll just short circuit
            // the loop.
            pLibrary = 0;
        }

        while (pLibrary)
        {
            pShader = pLibrary->GetShader(pRenderer, pName, implementation);
            if (pShader)
            {
                EE_ASSERT(NiIsKindOf(D3D11ShaderInterface, pShader));
                // Insert it into the map
                InsertShader(pShader, implementation);

                // Stop
                break;
            }

            pLibrary = GetNextLibrary();
        }

        m_shaderMapLock.Unlock();
    }

    if (reportNotFound && pShader == 0)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN, 
            false,
            "Failed to find shader %s, Implementation %d\n",
            pName, 
            implementation);
    }

    return pShader;

}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::InsertShader(NiShader* pShader, efd::UInt32 implementation)
{
    EE_UNUSED_ARG(implementation);
    EE_ASSERT(IsActiveFactory());

    if (!pShader)
        return;

    EE_ASSERT(implementation == NiShader::DEFAULT_IMPLEMENTATION ||
        implementation == pShader->GetImplementation());

    // We want to allow for the same shader, but different implementations.
    // So, we will tack the implementation number on the end of the shader
    // name when we place it in the list.
    efd::Char fullName[efd::EE_MAX_PATH];
    efd::Sprintf(
        fullName, 
        efd::EE_MAX_PATH, 
        "%s%d", 
        pShader->GetName(), 
        pShader->GetImplementation());

    NiFixedString fullNameString = fullName;
    m_shaderMap.SetAt(fullNameString, pShader);

    // Register the shader as the best implementation as well, if it is.
    D3D11ShaderInterface* pD3D11Shader = NiVerifyStaticCast(D3D11ShaderInterface, pShader);
    if (pD3D11Shader->GetIsBestImplementation())
    {
        efd::Sprintf(
            fullName, 
            efd::EE_MAX_PATH, 
            "%s%d", 
            pShader->GetName(),
            NiShader::DEFAULT_IMPLEMENTATION);
        NiFixedString fullNameStringBest = fullName;
        m_shaderMap.SetAt(fullNameStringBest, pShader);
    }
}

//------------------------------------------------------------------------------------------------
NiShader* D3D11ShaderFactory::FindShader(const efd::Char* pName, efd::UInt32 implementation)
{
    EE_ASSERT(IsActiveFactory());

    efd::Char fullName[efd::EE_MAX_PATH];
    efd::Sprintf(fullName, efd::EE_MAX_PATH, "%s%d", pName, implementation);

    NiShader* pShader = NULL;
    NiFixedString fullNameString = fullName;
    m_shaderMap.GetAt(fullNameString, pShader);

    return pShader;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderFactory::ReleaseShaderFromLibrary(
    const efd::Char* pName,
    efd::UInt32 implementation)
{
    EE_ASSERT(IsActiveFactory());

    if (!pName || pName[0] == '\0')
        return false;

    return ReleaseShaderFromLibrary(FindShader(pName, implementation));
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderFactory::ReleaseShaderFromLibrary(NiShader* pShader)
{
    EE_ASSERT(IsActiveFactory());

    if (pShader == NULL)
        return false;

    NiShader* pTestShader = FindShader(pShader->GetName(), pShader->GetImplementation());

    if (pTestShader == NULL || pShader != pTestShader)
    {
        // Is this a more serious problem?
        return false;
    }

    m_shaderMapLock.Lock();
    NiShaderLibrary* pLibrary = GetFirstLibrary();
    efd::Bool success = false;
    while (pLibrary)
    {
        if (pLibrary->ReleaseShader(pShader))
        {
            success = true;
            break;
        }
        pLibrary = GetNextLibrary();
    }

    m_shaderMapLock.Unlock();

    // Remove shader from factory as well
    RemoveShader(pShader->GetName(), pShader->GetImplementation());

    return success;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveShader(const efd::Char* pName, efd::UInt32 implementation)
{
    EE_ASSERT(IsActiveFactory());

    efd::Char fullName[efd::EE_MAX_PATH];
    efd::Sprintf(fullName, efd::EE_MAX_PATH, "%s%d", pName, implementation);

    NiFixedString fullNameString = fullName;
    m_shaderMap.RemoveAt(fullNameString);

    // Unregister the shader as the best implementation as well, if it is.
    efd::Sprintf(fullName, efd::EE_MAX_PATH, "%s%d", pName, NiShader::DEFAULT_IMPLEMENTATION);
    NiFixedString fullNameStringBest = fullName;
    m_shaderMap.RemoveAt(fullNameStringBest);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveAllShaders()
{
    EE_ASSERT(IsActiveFactory() || m_shaderMap.IsEmpty());

    m_shaderMap.RemoveAll();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::InsertLibrary(NiShaderLibrary* pLibrary)
{
    EE_ASSERT(IsActiveFactory());

    if (!pLibrary)
        return;

    NiFixedString libName = pLibrary->GetName();
    m_libraryMap.SetAt(libName, pLibrary);
}

//------------------------------------------------------------------------------------------------
NiShaderLibrary* D3D11ShaderFactory::FindLibrary(const efd::Char* pName)
{
    EE_ASSERT(IsActiveFactory());

    NiShaderLibraryPtr spLibrary;

    NiFixedString libName = pName;
    m_libraryMap.GetAt(libName, spLibrary);

    return spLibrary;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveLibrary(const efd::Char* pName)
{
    EE_ASSERT(IsActiveFactory());

    NiShaderLibraryPtr spLibrary;

    NiFixedString libName = pName;
    if (m_libraryMap.GetAt(libName, spLibrary))
    {
        if (spLibrary->GetRefCount() == 2)
        {
            m_libraryMap.RemoveAt(libName);

            // This doesn't have to be here, as it will occur when the
            // function exits. For testing purposes, it is here.
            spLibrary = 0;
        }
    }

    m_libraryIter = NULL;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveAllLibraries()
{
    EE_ASSERT(IsActiveFactory() || m_libraryMap.IsEmpty());

    D3D11ShaderLibrary* pLibrary = (D3D11ShaderLibrary*)D3D11ShaderFactory::GetFirstLibrary();
    while (pLibrary)
    {
        NiFixedString libName = pLibrary->GetName();
        m_libraryMap.RemoveAt(libName);

        pLibrary = (D3D11ShaderLibrary*)D3D11ShaderFactory::GetNextLibrary();
    }

    m_libraryMap.RemoveAll();

    m_libraryIter = NULL;
}

//------------------------------------------------------------------------------------------------
NiShaderLibrary* D3D11ShaderFactory::GetFirstLibrary()
{
    EE_ASSERT(IsActiveFactory() || m_libraryMap.IsEmpty());

    m_libraryIter = m_libraryMap.GetFirstPos();
    if (m_libraryIter)
    {
        NiFixedString libName;
        NiShaderLibraryPtr spLibrary;

        // The iterator is a thread local variable; passing it in directly would cause
        // only the temporary to be modified and not stored back
        NiTMapIterator iter = m_libraryIter;
        m_libraryMap.GetNext(iter, libName, spLibrary);
        m_libraryIter = iter;

        return spLibrary;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
NiShaderLibrary* D3D11ShaderFactory::GetNextLibrary()
{
    EE_ASSERT(IsActiveFactory() || m_libraryMap.IsEmpty());

    if (m_libraryIter)
    {
        NiFixedString libName;
        NiShaderLibraryPtr spLibrary;

        // The iterator is a thread local variable; passing it in directly would cause
        // only the temporary to be modified and not stored back
        NiTMapIterator iter = m_libraryIter;
        m_libraryMap.GetNext(iter, libName, spLibrary);
        m_libraryIter = iter;

        return spLibrary;
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::PurgeAllShaderRendererData()
{
    EE_ASSERT(IsActiveFactory());

    // PurgeAllShaderRendererData is called when the application wishes to
    // destroy the renderer, but retain the currently 'loaded' shader list.
    // This will call D3D11Renderer::PurgeAllD3D11Shaders to destroy the
    // renderer specific data contained in them. However, the 'shell' of the
    // shader will still exist to allow for recreation of the data and reuse
    // of the shader.

    NiTMapIterator iter = m_shaderMap.GetFirstPos();
    while (iter)
    {
        NiShader* pShader = 0;
        NiFixedString shaderName;
        m_shaderMap.GetNext(iter, shaderName, pShader);
        if (pShader)
        {
            // Set the renderer pointer
            D3D11ShaderInterface* pD3D11Shader = (D3D11ShaderInterface*)pShader;

            pD3D11Shader->DestroyRendererData();
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RestoreAllShaderRendererData()
{
    EE_ASSERT(IsActiveFactory());

    if (D3D11Renderer::GetRenderer() == NULL)
    {
        return;
    }

    // The shader map will be walked, with each shader being 'recreated'.
    NiTMapIterator iter = m_shaderMap.GetFirstPos();
    while (iter)
    {
        NiShader* pShader = 0;
        NiFixedString shaderName;
        m_shaderMap.GetNext(iter, shaderName, pShader);
        if (pShader)
        {
            // Set the renderer pointer
            D3D11ShaderInterface* pD3D11Shader = (D3D11ShaderInterface*)pShader;
            if (pD3D11Shader->IsInitialized() == false)
            {
                pD3D11Shader->RecreateRendererData();
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderFactory::IsDefaultImplementation(NiShader* pShader)
{
    EE_ASSERT(IsActiveFactory());

    D3D11ShaderInterface* pShaderIf = NiDynamicCast(D3D11ShaderInterface, pShader);

    if (pShaderIf)
    {
        return pShaderIf->GetIsBestImplementation();
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorVertexShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_VERTEX_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorVertexShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_VERTEX_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreateVertexShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_VERTEX_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorHullShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_HULL_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorHullShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_HULL_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreateHullShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_HULL_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorDomainShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_DOMAIN_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorDomainShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_DOMAIN_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreateDomainShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_DOMAIN_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorGeometryShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_GEOMETRY_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorGeometryShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_GEOMETRY_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreateGeometryShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_GEOMETRY_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorComputeShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_COMPUTE_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorComputeShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_COMPUTE_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreateComputeShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_COMPUTE_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMajorPixelShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_PIXEL_SHADER);
    return D3D11_SHVER_GET_MAJOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::GetMinorPixelShaderVersion(const efd::UInt32 version)
{
    EE_ASSERT(D3D11_SHVER_GET_TYPE(version) == D3D11_SHVER_PIXEL_SHADER);
    return D3D11_SHVER_GET_MINOR(version);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderFactory::CreatePixelShaderVersion(
    const efd::UInt32 majorVersion, 
    const efd::UInt32 minorVersion)
{
    return (((D3D11_SHVER_PIXEL_SHADER & 0x0000ffff) << 16) | 
        ((majorVersion & 0x0f) << 4) | 
        ((minorVersion & 0x0f) << 0));
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderFactory::SetAsActiveFactory()
{
    if (IsActiveFactory())
        return false;

    NiTPointerList<NiShaderLibraryPtr> shaderLibrariesToTransfer;

    if (ms_pkShaderFactory)
    {
        // Transfer data from the former active factory to the new one

        // Transfer callback functions, but only if they're not the defaults
        if (ms_pkShaderFactory->GetClassCreateCallback() != 
            ms_pkShaderFactory->GetDefaultClassCreateCallback())
        {
            ms_pD3D11ShaderFactory->m_pfnClassCreate = ms_pkShaderFactory->GetClassCreateCallback();
        }
        if (ms_pkShaderFactory->GetRunParserCallback() != 
            ms_pkShaderFactory->GetDefaultRunParserCallback())
        {
            ms_pD3D11ShaderFactory->m_pfnRunParser = ms_pkShaderFactory->GetRunParserCallback();
        }
        if (ms_pkShaderFactory->GetErrorCallback() != 
            ms_pkShaderFactory->GetDefaultErrorCallback())
        {
            ms_pD3D11ShaderFactory->m_pfnErrorCallback = ms_pkShaderFactory->GetErrorCallback();
        }

        // Transfer global shader constant entries
        //NiTMapIterator iter = NULL;
        //efd::FixedString entryKey;
        //NiGlobalConstantEntry* pEntry =
        //    ms_pkShaderFactory->GetFirstGlobalShaderConstant(kIter, entryKey);
        //while (pEntry)
        //{
        //    EE_ASSERT(entryKey.Exists());
        //    ms_pD3D11ShaderFactory->m_kGlobalConstantMap.SetAt(entryKey, pEntry);
        //    ms_pkShaderFactory->ReleaseGlobalShaderConstant(entryKey);
        //    pEntry = ms_pkShaderFactory->GetNextGlobalShaderConstant(iter, entryKey);
        //}

        // We should not have loaded any actual shaders at this point, so
        // there's no need to transfer them.

        // Transfer shader libraries
        NiShaderLibrary* pLibrary = ms_pkShaderFactory->GetFirstLibrary();
        while (pLibrary)
        {
            NiShaderLibraryPtr spLibrary = pLibrary;
            shaderLibrariesToTransfer.AddTail(spLibrary);
            pLibrary = ms_pkShaderFactory->GetNextLibrary();
        }
        ms_pkShaderFactory->RemoveAllLibraries();

        // Shader program directories should not yet be applied, since
        // no shader program factory should yet exist.

#if defined(_USRDLL)
        // Transfer DLL HMODULEs
        const efd::Char* pDLLName = NULL;
        HMODULE hDLL = (HMODULE)ms_pkShaderFactory->GetFirstLibraryDLL(pDLLName);
        if (hDLL)
        {
            EE_ASSERT(pDLLName);
            ms_pD3D11ShaderFactory->m_loadedShaderLibDLLs.SetAt(pDLLName, hDLL);
            hDLL = (HMODULE)ms_pkShaderFactory->GetNextLibraryDLL(pDLLName);
        }
        ms_pkShaderFactory->ClearLibraryDLLs();
#endif //#if defined(_USRDLL)
    }

    ms_pkShaderFactory = ms_pD3D11ShaderFactory;

    NiTListIterator iter = shaderLibrariesToTransfer.GetHeadPos();
    while (iter)
    {
        NiShaderLibraryPtr spLib = shaderLibrariesToTransfer.GetNext(iter);
        ms_pD3D11ShaderFactory->InsertLibrary(spLib);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
D3D11ShaderFactory* D3D11ShaderFactory::GetD3D11ShaderFactory()
{
    return ms_pD3D11ShaderFactory;
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderFactory::GetFirstProgramDirectory(NiTListIterator& iter)
{
    D3D11ShaderProgramFactory* pSPFactory = D3D11ShaderProgramFactory::GetInstance();
    if (!pSPFactory)
        return 0;

    return pSPFactory->GetFirstProgramDirectory(iter);
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderFactory::GetNextProgramDirectory(NiTListIterator& iter)
{
    D3D11ShaderProgramFactory* pSPFactory = D3D11ShaderProgramFactory::GetInstance();
    if (!pSPFactory)
        return 0;

    return pSPFactory->GetNextProgramDirectory(iter);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::AddProgramDirectory(const efd::Char* pDirectory)
{
    D3D11ShaderProgramFactory* pFactory = D3D11ShaderProgramFactory::GetInstance();
    if (pFactory)
        pFactory->AddProgramDirectory(pDirectory);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveProgramDirectory(const efd::Char* pDirectory)
{
    D3D11ShaderProgramFactory* pFactory = D3D11ShaderProgramFactory::GetInstance();
    if (pFactory)
        pFactory->RemoveProgramDirectory(pDirectory);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderFactory::RemoveAllProgramDirectories()
{
    D3D11ShaderProgramFactory* pFactory = D3D11ShaderProgramFactory::GetInstance();
    if (pFactory)
        pFactory->RemoveAllProgramDirectories();
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderFactory::GetCompilerName(efd::UInt32 version)
{
    if (version == 1310)
        return "VS 7.1";
    else if (version == 1400)
        return "VS 8.0";
    else if (version == 1500)
        return "VS 9.0";
    else
        return "Unknown version";
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderFactory::GetRendererString() const
{
    return "D3D11";
}

//------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_CLASSCREATIONCALLBACK
    D3D11ShaderFactory::GetDefaultClassCreateCallback() const
{
    return &D3D11ShaderFactory::DefaultCreateClass;
}

//------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_RUNPARSERCALLBACK
    D3D11ShaderFactory::GetDefaultRunParserCallback() const
{
    return &D3D11ShaderFactory::DefaultRunParser;
}

//------------------------------------------------------------------------------------------------
NiShaderFactory::NISHADERFACTORY_ERRORCALLBACK D3D11ShaderFactory::GetDefaultErrorCallback() const
{
    return NULL;
}

//------------------------------------------------------------------------------------------------
