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
#include "NiD3D10RendererPCH.h"

#include "NiD3D10Include.h"

//--------------------------------------------------------------------------------------------------
void NiD3D10Include::SetBasePath(const char* pcBasePath)
{
    char acAbsolutePath[NI_MAX_PATH];

    EE_ASSERT(pcBasePath);
    // In case pcBasePath is relative, we must get an absolute path so
    // m_acBasePath is a valid absolute path.
    if (NiPath::IsRelative(pcBasePath))
    {
        NiPath::ConvertToAbsolute(acAbsolutePath, NI_MAX_PATH, pcBasePath,
            NULL);
    }
    else
    {
        NiStrcpy(acAbsolutePath, NI_MAX_PATH, pcBasePath);
    }

    NiFilename kFilename(acAbsolutePath);
    NiStrcpy(m_acBasePath, NI_MAX_PATH, kFilename.GetDrive());
    NiStrcat(m_acBasePath, NI_MAX_PATH, kFilename.GetDir());
}

//--------------------------------------------------------------------------------------------------
HRESULT NiD3D10Include::Open(D3D10_INCLUDE_TYPE, LPCSTR pFileName,
    LPCVOID, LPCVOID *ppData, UINT *pBytes)
{
    char acAbsolutePath[NI_MAX_PATH];

    EE_ASSERT(pFileName);
    if (NiPath::IsRelative(pFileName))
    {
        NiPath::ConvertToAbsolute(acAbsolutePath, NI_MAX_PATH, pFileName,
            m_acBasePath);
    }
    else
    {
        NiStrcpy(acAbsolutePath, NI_MAX_PATH, pFileName);
    }
    NiPath::RemoveDotDots(acAbsolutePath);

    UINT dwSize = 0;

    efd::File *pkFile = efd::File::GetFile(acAbsolutePath,efd::File::READ_ONLY);
    if (pkFile==NULL || !(*pkFile))
        return E_FAIL;

    dwSize = pkFile->GetFileSize();

    CHAR* pcFileBuffer =  0;

    if (dwSize != 0)
    {
        pcFileBuffer = NiAlloc(CHAR, dwSize);
        pkFile->Read((void*)pcFileBuffer, dwSize);
    }

    NiDelete pkFile;
    *ppData = (VOID*) pcFileBuffer;
    *pBytes = dwSize;

    return S_OK;
}

//--------------------------------------------------------------------------------------------------
HRESULT NiD3D10Include::Close(LPCVOID pData)
{
    NiFree((void*)pData);
    return S_OK;
}

//--------------------------------------------------------------------------------------------------
