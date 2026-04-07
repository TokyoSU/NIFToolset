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
//--------------------------------------------------------------------------------------------------
// Precompiled Header
#include "NiD3DXEffectShaderLibPCH.h"

#include "NiD3DXEffectShaderLibrary.h"
#include "NiD3DXEffectFactory.h"
#include "NiD3DXEffectFile.h"
#include "NiD3DXEffectLoader.h"
#include "NiD3DXEffectTechnique.h"
#include "NiD3DXEffectTechniqueSet.h"
#include <NiShaderLibraryDesc.h>
#include <NiD3DUtility.h>

#if defined(WIN32)
#include <TCHAR.h>
#endif //#if defined(WIN32)

NiD3DXEffectShaderLibrary* NiD3DXEffectShaderLibrary::ms_pkLibrary = 0;

//--------------------------------------------------------------------------------------------------
NiD3DXEffectShaderLibrary::NiD3DXEffectShaderLibrary() :
    NiD3DShaderLibrary("NiD3DXEffectShaderLib")
{
    ms_pkLibrary = this;
}

//--------------------------------------------------------------------------------------------------
NiD3DXEffectShaderLibrary::~NiD3DXEffectShaderLibrary()
{
    NiOutputDebugString("Releasing D3DXEffect Shaders!\n");

    NiTListIterator kIter = m_kD3DXEffectFileList.GetHeadPos();
    while (kIter)
    {
        NiD3DXEffectFile* pkFile = m_kD3DXEffectFileList.RemovePos(kIter);
        NiDelete pkFile;
    }

    ms_pkLibrary = NULL;
}

//--------------------------------------------------------------------------------------------------
NiD3DXEffectShaderLibrary* NiD3DXEffectShaderLibrary::Create(
    NiRenderer* pkRenderer, int iDirectoryCount,
    const char* apcDirectories[], bool bRecurseSubFolders)
{
    NiD3DRenderer* pkD3DRenderer = NiDynamicCast(NiD3DRenderer, pkRenderer);

    if (pkD3DRenderer && pkD3DRenderer->GetRendererID() !=
#if defined(WIN32)
        NiSystemDesc::RENDERER_DX9)
#elif defined(_XENON)
        NiSystemDesc::RENDERER_XENON)
#else
#error Unsupported Platform
#endif
    {
        pkD3DRenderer = NULL;
    }

#if defined(WIN32)
    if (pkD3DRenderer == NULL &&
        NiD3DXEffectFactory::GetApplicationInstance() == NULL)
    {
        return NULL;
    }
#endif //#if defined(WIN32)

    NiD3DXEffectShaderLibrary* pkShaderLibrary = NULL;
    if (ms_pkLibrary)
        pkShaderLibrary = ms_pkLibrary;
    else
        pkShaderLibrary = NiNew NiD3DXEffectShaderLibrary();

    if (pkShaderLibrary)
    {
        pkShaderLibrary->SetRenderer(pkD3DRenderer);

        for (int i = 0; i < iDirectoryCount; i++)
        {
            bool bSuccess = pkShaderLibrary->LoadD3DXEffectFiles(
                apcDirectories[i], bRecurseSubFolders);
            if (!bSuccess)
            {
                NiD3DUtility::Log("LoadD3DXEffectFiles failed on %s\n",
                    apcDirectories[i]);
            }
        }
    }

    return pkShaderLibrary;
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLibrary::Shutdown()
{ /* */ }

//--------------------------------------------------------------------------------------------------
NiShaderLibraryDesc* NiD3DXEffectShaderLibrary::GetShaderLibraryDesc()
{
    return NiD3DShaderLibrary::GetShaderLibraryDesc();
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLibrary::SetShaderLibraryDesc(
    NiShaderLibraryDesc* pkDesc)
{
    NiD3DShaderLibrary::SetShaderLibraryDesc(pkDesc);
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLibrary::SetRenderer(NiD3DRenderer* pkRenderer)
{
    if (m_pkD3DRenderer != pkRenderer)
    {
        NiD3DShaderLibrary::SetRenderer(pkRenderer);

        // this will mark all existing D3DXEffect objects as invalid
        NiD3DXEffectFactory::SetD3DRenderer(pkRenderer);
    }
}

//--------------------------------------------------------------------------------------------------
NiShader* NiD3DXEffectShaderLibrary::GetShader(NiRenderer* pkRenderer,
    const char* pcName, unsigned int uiImplementation)
{
    // Currently, only the DX9 and Xenon renderers are supported by this
    // shader library.
    if (!NiIsKindOf(NiD3DRenderer, pkRenderer))
        return NULL;

    SetRenderer((NiD3DRenderer*)pkRenderer);

    NiD3DXEffectTechniqueSet* pkTechniqueSet = NULL;
    if (!m_kD3DXEffectTechniqueMap.GetAt(pcName, pkTechniqueSet) ||
        pkTechniqueSet == NULL)
    {
        return NULL;
    }

    return (NiShader*)pkTechniqueSet->GetD3DShader((NiD3DRenderer*)pkRenderer,
        uiImplementation);
}

//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLibrary::ReleaseShader(const char* pcName,
    unsigned int uiImplementation)
{
    NiD3DXEffectTechniqueSet* pkTechniqueSet = NULL;
    if (!m_kD3DXEffectTechniqueMap.GetAt(pcName, pkTechniqueSet) ||
        pkTechniqueSet== NULL)
    {
        return false;
    }

    NiD3DXEffectTechnique* pkTechnique =
        pkTechniqueSet->GetTechnique(uiImplementation);
    if (pkTechnique == NULL)
        return false;

    return pkTechnique->ReleaseShader();
}

//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLibrary::ReleaseShader(NiShader* pkShader)
{
    if (pkShader == NULL)
        return false;
    return ReleaseShader(pkShader->GetName(), pkShader->GetImplementation());
}

//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLibrary::InsertD3DXEffectFileIntoList(
    NiD3DXEffectFile* pkD3DXEffectFile)
{
    if (!pkD3DXEffectFile)
        return false;

    NiD3DXEffectFile* pkCheckFile = GetD3DXEffectFile(
        pkD3DXEffectFile->GetName());
    if (pkCheckFile)
    {
        NiD3DUtility::Log("File %s already in list??\n",
            pkD3DXEffectFile->GetName());
        return false;
    }

    m_kD3DXEffectFileList.AddTail(pkD3DXEffectFile);

    // Add it to the description
    if (m_spShaderLibraryDesc == 0)
        m_spShaderLibraryDesc = NiNew NiShaderLibraryDesc();
    EE_ASSERT(m_spShaderLibraryDesc);

    NiShaderDesc* pkDesc = pkD3DXEffectFile->GetFirstShaderDesc();
    while (pkDesc)
    {
        m_spShaderLibraryDesc->AddShaderDesc(pkDesc);

        m_spShaderLibraryDesc->AddPixelShaderVersion(
            pkDesc->GetMinPixelShaderVersion());
        m_spShaderLibraryDesc->AddPixelShaderVersion(
            pkDesc->GetMaxPixelShaderVersion());
        m_spShaderLibraryDesc->AddVertexShaderVersion(
            pkDesc->GetMinVertexShaderVersion());
        m_spShaderLibraryDesc->AddVertexShaderVersion(
            pkDesc->GetMaxVertexShaderVersion());
        m_spShaderLibraryDesc->AddUserDefinedVersion(
            pkDesc->GetMinUserDefinedVersion());
        m_spShaderLibraryDesc->AddUserDefinedVersion(
            pkDesc->GetMaxUserDefinedVersion());
        m_spShaderLibraryDesc->AddPlatformFlags(
            pkDesc->GetPlatformFlags());

        pkDesc = pkD3DXEffectFile->GetNextShaderDesc();
    };

    NiD3DXEffectTechnique* pkTech = pkD3DXEffectFile->GetFirstTechnique();
    while (pkTech)
    {
        NiD3DXEffectTechniqueSet* pkTechniqueSet =
            pkD3DXEffectFile->GetTechniqueSet(pkTech->GetShaderName());
        m_kD3DXEffectTechniqueMap.SetAt(pkTech->GetShaderName(),
            pkTechniqueSet);
        pkTech = pkD3DXEffectFile->GetNextTechnique();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiD3DXEffectShaderLibrary::GetD3DXEffectFileCount()
{
    return m_kD3DXEffectFileList.GetSize();
}

//--------------------------------------------------------------------------------------------------
NiD3DXEffectFile* NiD3DXEffectShaderLibrary::GetD3DXEffectFile(
    const char* pcName)
{
    NiTListIterator kIter = m_kD3DXEffectFileList.GetHeadPos();
    while (kIter)
    {
        NiD3DXEffectFile* pkFile = m_kD3DXEffectFileList.GetNext(kIter);
        if (strcmp(pkFile->GetName(), pcName) == 0)
            return pkFile;
    }
    return NULL;
}

typedef LPDIRECT3D9 (CALLBACK *NILPD3DCREATE) (unsigned int);
//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLibrary::LoadD3DXEffectFiles(const char* pcDirectory,
    bool bRecurseSubFolders)
{
    NiD3DXEffectLoader* pkLoader = NiD3DXEffectLoader::Create();
    EE_ASSERT(pkLoader);
#if defined(WIN32)
    LPDIRECT3DDEVICE9 pkTempDevice = NULL;
    HWND hTempWindow = NULL;
    HINSTANCE hD3DLib = NULL;
    LPDIRECT3D9 pkD3D9 = NULL;
    if (m_pkD3DRenderer == NULL)
    {
        // Create a temp device
        LPCTSTR pcTempClassName = _T("TempFXLoaderWindowClass");

        WNDCLASS kWndClass;
        memset(&kWndClass, 0, sizeof(kWndClass));
        kWndClass.lpfnWndProc = DefWindowProc;
        kWndClass.hInstance = NiD3DXEffectFactory::GetApplicationInstance();
        kWndClass.lpszClassName = pcTempClassName;
        RegisterClass(&kWndClass);

        hTempWindow = CreateWindow(
            pcTempClassName,
            pcTempClassName,
            WS_DISABLED,
            0,
            0,
            10,
            10,
            NULL,
            NULL,
            NiD3DXEffectFactory::GetApplicationInstance(),
            NULL);

        hD3DLib = LoadLibrary("D3D9.DLL");
        NILPD3DCREATE pfnD3DCreate9 = (NILPD3DCREATE)GetProcAddress(hD3DLib, "Direct3DCreate9");

        pkD3D9 = pfnD3DCreate9(D3D_SDK_VERSION);

        D3DPRESENT_PARAMETERS kParams;
        memset(&kParams, 0, sizeof(kParams));
        kParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
        kParams.Windowed = true;

        LPDIRECT3DDEVICE9 pkTempDevice = NULL;
        HRESULT hr = pkD3D9->CreateDevice(
            0,
            D3DDEVTYPE_NULLREF,
            hTempWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
            &kParams,
            &pkTempDevice);

        if (FAILED(hr) && pkTempDevice)
        {
            pkTempDevice->Release();
            pkTempDevice = NULL;
        }

        NiD3DXEffectFactory::SetDevice(pkTempDevice);
    }
#endif //#if defined(WIN32)

    bool bResult = pkLoader->LoadAllD3DXEffectFiles(this, pcDirectory,
        bRecurseSubFolders);
#if defined(WIN32)

    if (pkTempDevice)
    {
        NiD3DXEffectFactory::SetDevice(NULL);
        pkTempDevice->Release();
        DestroyWindow(hTempWindow);

        FreeLibrary(hD3DLib);
    }
#endif //#if defined(WIN32)

    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLibrary::LoadShader(const char* pcFile)
{
    NiD3DXEffectLoader* pkLoader = NiD3DXEffectLoader::Create();
    EE_ASSERT(pkLoader);

    bool bResult = pkLoader->LoadD3DXEffectFile(this, pcFile);
    return bResult;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiD3DXEffectShaderLibrary::GetNumSupportedMimeTypes() const
{
    return NiD3DXEffectFactory::GetFXLSupport() ? 3 : 2;
}

//--------------------------------------------------------------------------------------------------
const char* NiD3DXEffectShaderLibrary::GetSupportedMimeType(
    unsigned int uiIdx) const
{
    EE_ASSERT(uiIdx < 3 || !"Invalid index");

    const char* acMimeTypes[] =
    {
        "fx-shader",
        "fx-shader-compiled",
        "fxl-shader",
    };
    return acMimeTypes[uiIdx];
}

//--------------------------------------------------------------------------------------------------