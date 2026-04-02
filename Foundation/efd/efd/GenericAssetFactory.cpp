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

/*
    If you change anything in this code, please check for corresponding changes to the
    Programmer's Guide documentation for adding a new asset type.
*/


#include "efdPCH.h"

#include <efd/File.h>
#include <efd/GenericAssetFactory.h>
#include <efd/AssetLoadRequest.h>
#include <efd/GenericAssetLoadResponse.h>
#include <efd/ServiceManager.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(GenericAssetFactory);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(GenericAssetFactory::ResponseData);

//------------------------------------------------------------------------------------------------
GenericAssetFactory::GenericAssetFactory(const bool appendNULL, const UInt32 bytesPerTick)
    : m_appendNULL(appendNULL)
    , m_bytesPerTick(bytesPerTick)
{
}

//------------------------------------------------------------------------------------------------
GenericAssetFactory::~GenericAssetFactory()
{
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus GenericAssetFactory::LoadAsset(
    AssetFactoryManager* pFactoryManager,
    const AssetLoadRequest* pRequest,
    AssetLoadResponsePtr& pResponse)
{
    /*
        This function is copied verbatim on the doc page titled Creating a New Asset Factory.
        Please keep the two in sync.
    */

    EE_UNUSED_ARG(pFactoryManager);

    EE_ASSERT(pRequest);
    EE_ASSERT(pRequest->GetAssetPath().length());


    ResponseDataPtr spData;
    IAssetFactory::LoadStatus status = GetResponseDataAs<ResponseData>(pRequest, spData);

    if (status != IAssetFactory::LOAD_RUNNING)
    {
        return status;
    }

    if (!spData)
    {
        spData = HandleNewRequest(pRequest);
    }

    if (spData->m_spResponse->GetResult() != AssetLoadResponse::ALR_Success)
    {
        pResponse = spData->m_spResponse;
        return IAssetFactory::LOAD_COMPLETE;
    }

    UInt32 fileSize = spData->m_pFile->GetFileSize();

    UInt32 bytesToRead = fileSize - spData->m_bytesRead;
    if (bytesToRead > m_bytesPerTick)
        bytesToRead = m_bytesPerTick;

    if (bytesToRead > 0)
    {
        UInt32 readSize = spData->m_pFile->Read(spData->m_pBuffer + spData->m_bytesRead, bytesToRead);
        EE_ASSERT(bytesToRead == readSize);
        spData->m_bytesRead += readSize;
    }

    if (fileSize == spData->m_bytesRead)
    {
        pResponse = spData->m_spResponse;

        EE_DELETE spData->m_pFile;

        RemoveResponseData(pRequest);

        if (m_appendNULL)
        {
            spData->m_pBuffer[spData->m_bytesRead] = '\0';
            ++spData->m_bytesRead;
        }

        return IAssetFactory::LOAD_COMPLETE;
    }

    return IAssetFactory::LOAD_RUNNING;
}

//------------------------------------------------------------------------------------------------
GenericAssetFactory::ResponseDataPtr GenericAssetFactory::HandleNewRequest(
    const AssetLoadRequest* pRequest)
{
    GenericAssetFactory::ResponseDataPtr spResult = EE_NEW ResponseData;

    // Try to open the file
    spResult->m_pFile = File::GetFile(pRequest->GetAssetPath().c_str(), File::READ_ONLY);
    if (!spResult->m_pFile)
    {
        EE_LOG(efd::kAssetLoading, ILogger::kERR0,
            ("Could not open file \'%s\' for reading.", pRequest->GetAssetPath().c_str()));

        spResult->m_spResponse = EE_NEW GenericAssetLoadResponse(
            pRequest->GetURN(),
            pRequest->GetResponseCategory(),
            AssetLoadResponse::ALR_FileNotFound,
            pRequest->GetAssetPath(),
            pRequest->GetIsReload());

        return spResult;
    }

    UInt32 fileSize = spResult->m_pFile->GetFileSize();

    UInt32 bufferSize = m_appendNULL ? fileSize + 1 : fileSize;
    spResult->m_pBuffer = EE_ALLOC(char, bufferSize);
    spResult->m_bytesRead = 0;

    spResult->m_spResponse = EE_NEW GenericAssetLoadResponse(
        pRequest->GetURN(),
        pRequest->GetResponseCategory(),
        AssetLoadResponse::ALR_Success,
        pRequest->GetAssetPath(),
        pRequest->GetIsReload(),
        bufferSize,
        spResult->m_pBuffer);

    m_pendingRequestLock.Lock();
    m_pendingRequestMap[pRequest] = spResult;
    m_pendingRequestLock.Unlock();

    return spResult;
}

//------------------------------------------------------------------------------------------------
