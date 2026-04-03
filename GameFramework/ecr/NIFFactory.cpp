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

#include "ecrPCH.h"

#include <efd/Category.h>
#include <efd/SmartCriticalSection.h>
#include <efd/MessageService.h>
#include <ecr/NIFFactory.h>
#include <ecr/NIFFactoryResponse.h>
#include <efd/AssetFactoryManager.h>
#include <efd/ecrLogIDs.h>
#include <NiStream.h>

using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NIFFactory);

//------------------------------------------------------------------------------------------------
NIFFactory::NIFFactory(NiTexturePalette* pTexturePalette)
    : m_spTexturePalette(pTexturePalette)
{
}

//------------------------------------------------------------------------------------------------
NIFFactory::~NIFFactory()
{
    m_spTexturePalette = 0;
}

//------------------------------------------------------------------------------------------------
efd::IAssetFactory::LoadStatus NIFFactory::LoadAsset(
    efd::AssetFactoryManager*,
    const efd::AssetLoadRequest* pRequest,
    efd::AssetLoadResponsePtr& ppResponse)
{
    NiStream* pStream = EE_NEW NiStream();
    pStream->SetTexturePalette(m_spTexturePalette);
    pStream->ResetLastErrorInfo();
    pStream->Load(pRequest->GetAssetPath().c_str());

    UInt32 err = pStream->GetLastError();
    if (err != NiStream::STREAM_OKAY && err != NiStream::NO_CREATE_FUNCTION)
    {
        AssetLoadResponse::AssetLoadResult result = AssetLoadResponse::ALR_UnknownError;

        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR2,
            ("Failed to load NiStream: %s", pStream->GetLastErrorMessage()));

        switch (err)
        {
        case NiStream::FILE_NOT_LOADED:
            result = AssetLoadResponse::ALR_FileAccessDenied;  // or not found, can't tell
            break;

        case NiStream::NOT_NIF_FILE: // fall through
        case NiStream::OLDER_VERSION: // fall through
        case NiStream::LATER_VERSION: // fall through
        case NiStream::ENDIAN_MISMATCH:
            result = AssetLoadResponse::ALR_ParseError;
            break;

        default:
            break;
        }

        ppResponse = EE_NEW NIFFactoryResponse(
            pRequest->GetURN(),
            pRequest->GetResponseCategory(),
            result,
            pRequest->GetAssetPath(),
            pRequest->GetIsReload());

        EE_DELETE pStream;
    }
    else
    {
        ppResponse = EE_NEW NIFFactoryResponse(
            pRequest->GetURN(),
            pRequest->GetResponseCategory(),
            err == NiStream::NO_CREATE_FUNCTION ?
                AssetLoadResponse::ALR_PartialSuccess :
                AssetLoadResponse::ALR_Success,
            pRequest->GetAssetPath(),
            pRequest->GetIsReload(),
            pStream);
    }

    return IAssetFactory::LOAD_COMPLETE;
}

//------------------------------------------------------------------------------------------------
efd::IAssetFactory::ThreadSafetyDescription NIFFactory::GetThreadSafety() const
{
    return efd::IAssetFactory::LOADER_FG_AND_BG;
}

//------------------------------------------------------------------------------------------------
NIFFactory& NIFFactory::operator=(const NIFFactory&)
{
    EE_ASSERT(false);
    return *this;
}

//------------------------------------------------------------------------------------------------
NIFFactory::NIFFactory(const NIFFactory&)
{
    EE_ASSERT(false);
}
