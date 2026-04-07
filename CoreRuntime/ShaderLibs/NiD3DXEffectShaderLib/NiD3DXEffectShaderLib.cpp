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

#include "NiD3DXEffectShaderLib.h"
#include "NiD3DXEffectShaderLibrary.h"
#include "NiD3DXEffectLoader.h"
#include "NiD3DXEffectFactory.h"

#include <NiD3DShaderLibraryInterface.h>
#include <NiShaderAttributeDesc.h>
#include <NiShaderDesc.h>
#include <NiShaderLibraryDesc.h>

//--------------------------------------------------------------------------------------------------
#if defined(_USRDLL)
#include "NiD3DXEffectShaderSDM.h"
static NiD3DXEffectShaderSDM NiD3DXEffectShaderSDMObject;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID)
{
    NiOutputDebugString("NiD3DXEffectShaderLib> DLLMain CALL - ");

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            //  Initialize anything needed here
            //  If failed, return FALSE
            NiOutputDebugString("PROCESS ATTACH!\n");
            NiStaticDataManager::ProcessAccumulatedLibraries();
            NiD3DXEffectFactory::SetApplicationInstance(hinstDLL);
        }
        break;
    case DLL_THREAD_ATTACH:
        {
            NiOutputDebugString("THREAD ATTACH!\n");
        }
        break;
    case DLL_PROCESS_DETACH:
        {
            //  Shutdown anything needed here
            NiOutputDebugString("PROCESS DETACH!\n");
            NiStaticDataManager::RemoveLibrary("NiD3DXEffectShader");
        }
        break;
    case DLL_THREAD_DETACH:
        {
            NiOutputDebugString("THREAD DETACH!\n");
        }
        break;
    }

    return (TRUE);
}

//--------------------------------------------------------------------------------------------------
NID3DXEFFECTSHADER_ENTRY bool LoadShaderLibrary(NiRenderer* pkRenderer,
    int iDirectoryCount, const char* apcDirectories[], bool bRecurseSubFolders,
    NiShaderLibrary** ppkLibrary)
{
    return NiD3DXEffectShaderLib_LoadShaderLibrary(
        pkRenderer,
        iDirectoryCount,
        apcDirectories,
        bRecurseSubFolders,
        ppkLibrary);
}

//--------------------------------------------------------------------------------------------------
NID3DXEFFECTSHADER_ENTRY unsigned int GetCompilerVersion(void)
{
    return (_MSC_VER);
}
#endif  //#if defined(_USRDLL)

//--------------------------------------------------------------------------------------------------
bool NiD3DXEffectShaderLib_LoadShaderLibrary(NiRenderer* pkRenderer,
    int iDirectoryCount, const char* apcDirectories[], bool bRecurseSubFolders,
    NiShaderLibrary** ppkLibrary)
{
    if (ppkLibrary == NULL)
        return false;

    NiD3DXEffectShaderLibrary* pkLibrary = NiD3DXEffectShaderLibrary::Create(
        (NiD3DRenderer*)pkRenderer, iDirectoryCount, apcDirectories,
        bRecurseSubFolders);

    *ppkLibrary = pkLibrary;
    return (pkLibrary != NULL);
}

//--------------------------------------------------------------------------------------------------
unsigned int NiD3DXEffectShaderLib_GetD3DXEffectCreationFlags()
{
    return NiD3DXEffectFactory::GetD3DXEffectCreationFlags();
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLib_SetD3DXEffectCreationFlags(unsigned int uiFlags)
{
    NiD3DXEffectFactory::SetD3DXEffectCreationFlags(uiFlags);
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLib_SetApplicationInstance(HINSTANCE hInstance)
{
    NiD3DXEffectFactory::SetApplicationInstance(hInstance);
}

//--------------------------------------------------------------------------------------------------
void NiD3DXEffectShaderLib_EnableFXLSupport(bool bEnable)
{
    NiD3DXEffectFactory::SetFXLSupport(bEnable);
}

//--------------------------------------------------------------------------------------------------
