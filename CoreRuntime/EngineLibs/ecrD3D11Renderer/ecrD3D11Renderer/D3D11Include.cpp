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

#include "D3D11Include.h"

#include <efd/PathUtils.h>
#include <NiFilename.h>

using namespace ecr;

//--------------------------------------------------------------------------------------------------
void D3D11Include::SetBasePath(const efd::Char* pBasePath)
{
    efd::Char absolutePath[efd::EE_MAX_PATH];

    EE_ASSERT(pBasePath);
    // In case pBasePath is relative, we must get an absolute path so
    // m_basePath is a valid absolute path.
    if (efd::PathUtils::IsRelativePath(pBasePath))
    {
        efd::PathUtils::ConvertToAbsolute(absolutePath, efd::EE_MAX_PATH, pBasePath, NULL);
    }
    else
    {
        efd::Strcpy(absolutePath, efd::EE_MAX_PATH, pBasePath);
    }

    NiFilename kFilename(absolutePath);
    efd::Strcpy(m_basePath, efd::EE_MAX_PATH, kFilename.GetDrive());
    efd::Strcat(m_basePath, efd::EE_MAX_PATH, kFilename.GetDir());
}

//--------------------------------------------------------------------------------------------------
HRESULT D3D11Include::Open(
    D3D_INCLUDE_TYPE, 
    LPCSTR pFileName,
    LPCVOID, 
    LPCVOID *ppData, 
    UINT *pBytes)
{
    efd::Char absolutePath[efd::EE_MAX_PATH];

    EE_ASSERT(pFileName);
    if (efd::PathUtils::IsRelativePath(pFileName))
    {
        efd::PathUtils::ConvertToAbsolute(absolutePath, efd::EE_MAX_PATH, pFileName, m_basePath);
    }
    else
    {
        efd::Strcpy(absolutePath, NI_MAX_PATH, pFileName);
    }
    efd::PathUtils::RemoveDotDots(absolutePath);

    efd::File* pFile = efd::File::GetFile(absolutePath, efd::File::READ_ONLY);
    if (pFile == NULL || !(*pFile))
        return E_FAIL;

    UINT dwSize = pFile->GetFileSize();

    CHAR* pFileBuffer =  0;

    if (dwSize != 0)
    {
        pFileBuffer = EE_ALLOC(CHAR, dwSize);
        pFile->Read((void*)pFileBuffer, dwSize);
    }

    EE_DELETE pFile;
    *ppData = (VOID*) pFileBuffer;
    *pBytes = dwSize;

    return S_OK;
}

//--------------------------------------------------------------------------------------------------
HRESULT D3D11Include::Close(LPCVOID pData)
{
    EE_FREE((void*)pData);
    return S_OK;
}

//------------------------------------------------------------------------------------------------
