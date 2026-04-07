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

#include "D3D11ShaderLibrary.h"

using namespace ecr;

efd::CriticalSection D3D11ShaderLibrary::ms_criticalSection;
efd::Char* D3D11ShaderLibrary::ms_pDirectory = 0;
efd::UInt32 D3D11ShaderLibrary::ms_platform = 0;

//------------------------------------------------------------------------------------------------
D3D11ShaderLibrary::D3D11ShaderLibrary(const efd::Char* pName) :
    NiShaderLibrary(pName)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShaderLibrary::~D3D11ShaderLibrary()
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::SystemDesc::RendererID D3D11ShaderLibrary::GetRendererID() const
{
    return efd::SystemDesc::RENDERER_D3D11;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderLibrary::SetDirectoryInfo(const efd::Char* pDirectory)
{
    ms_criticalSection.Lock();
    EE_DELETE[] ms_pDirectory;
    if (pDirectory && pDirectory[0] != '\0')
    {
        efd::UInt32 stringLen = efd::Strlen(pDirectory) + 1;
        ms_pDirectory = EE_ALLOC(efd::Char, stringLen);
        EE_ASSERT(ms_pDirectory);
        efd::Strcpy(ms_pDirectory, stringLen, pDirectory);
    }
    ms_criticalSection.Unlock();
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderLibrary::GetDirectory()
{
    return ms_pDirectory;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderLibrary::SetPlatform(efd::UInt32 platform)
{
    ms_platform = platform;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderLibrary::GetPlatform()
{
    return ms_platform;
}

//------------------------------------------------------------------------------------------------
NiShaderLibraryDesc* D3D11ShaderLibrary::GetShaderLibraryDesc()
{
    return m_spShaderLibraryDesc;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderLibrary::SetShaderLibraryDesc(NiShaderLibraryDesc* pLibDesc)
{
    m_spShaderLibraryDesc = pLibDesc;
}

//------------------------------------------------------------------------------------------------
