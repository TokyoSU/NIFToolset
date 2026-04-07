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

#include "ShaderService.h"

#include <efd/MessageService.h>
#include <efd/AssetLocatorService.h>
#include <efd/PathUtils.h>
#include <efd/ecrLogIDs.h>
#include <NiShaderParser.h>
#include <NiShaderFactory.h>
#include <efd/ServiceManager.h>
#include "RenderService.h"

using namespace efd;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(ShaderService);

// dwest: NOTE: this will not cause *all* AssetLocatorResponse subclasses to be handled.
EE_HANDLER_WRAP(
    ShaderService,
    HandleAssetLocatorResponse,
    AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

//------------------------------------------------------------------------------------------------
ShaderService::ShaderService(bool createShaderLibs) :
    m_pMessageService(NULL),
    m_state(SS_STATE_INITIAL),
    m_parseCategory(kCAT_INVALID),
    m_loadCategory(kCAT_INVALID),
    m_programCategory(kCAT_INVALID)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2100;
    m_createShaderLibs = createShaderLibs;
    m_maxOnInitTime = 0.01f; // 10 milliseconds

    m_findingProgramsIndex = 0;
    m_programLocateMessageCount = 0;

    // Hardcoded from MimeTagger.ini, as applications can't be guaranteed that this file exists.
    m_shaderProgramTypes.push_back("fx-shader");
    m_shaderProgramTypes.push_back("fx-shader-compiled");
    m_shaderProgramTypes.push_back("fx-shader-d3d10");
    m_shaderProgramTypes.push_back("fx-shader-d3d10-compiled");
    m_shaderProgramTypes.push_back("fx-shader-d3d11");
    m_shaderProgramTypes.push_back("fx-shader-d3d11-compiled");
    m_shaderProgramTypes.push_back("fxl-shader");
    m_shaderProgramTypes.push_back("high-level-shading-language");
    m_shaderProgramTypes.push_back("cg-shader");
    m_shaderProgramTypes.push_back("cgfx-shader");
    m_shaderProgramTypes.push_back("gamebryo-text-shader");
    m_shaderProgramTypes.push_back("gamebryo-binary-shader");
    m_shaderProgramTypes.push_back("assembly-vertex-shader");
    m_shaderProgramTypes.push_back("assembly-pixel-shader");
    m_shaderProgramTypes.push_back("fragment-program-object");
    m_shaderProgramTypes.push_back("vertex-program-object");
}

//------------------------------------------------------------------------------------------------
ShaderService::~ShaderService()
{
}

//------------------------------------------------------------------------------------------------
const char* ShaderService::GetDisplayName() const
{
    return "ShaderService";
}

//------------------------------------------------------------------------------------------------
void ShaderService::SetMaxOnInitTime(float fMaxTimeInSec)
{
    m_maxOnInitTime = fMaxTimeInSec;
}

//------------------------------------------------------------------------------------------------
float ShaderService::GetMaxOnInitTime()
{
    return m_maxOnInitTime;
}

//------------------------------------------------------------------------------------------------
SyncResult ShaderService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<AssetLocatorService>();
    pDependencyRegistrar->AddDependency<ecr::RenderService>();

    m_pAssetLocator = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    EE_ASSERT(m_pAssetLocator);

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_pMessageService);

    if (!m_pAssetLocator || !m_pMessageService)
        return SyncResult_Failure;

    if (!NiShaderFactory::GetInstance())
        return SyncResult_Failure;

    m_parseCategory = GenerateAssetResponseCategory();
    m_pMessageService->Subscribe(this, m_parseCategory);

    m_loadCategory = GenerateAssetResponseCategory();
    m_pMessageService->Subscribe(this, m_loadCategory);

    m_programCategory = GenerateAssetResponseCategory();
    m_pMessageService->Subscribe(this, m_programCategory);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult ShaderService::OnInit()
{
    // Poll the current time so we can measure how long we've been parsing/loading shaders.
    float startTime = NiGetCurrentTimeInSec();

    switch (m_state)
    {
    case SS_STATE_INITIAL:
        CreateShaderParsers();

        if (m_createShaderLibs)
        {
            CreateShaderLibraries();
        }
        else
        {
            // Since we are not creating shader libraries we need to manually
            // populate the m_libraries list with the set of all the loaded
            // shader libraries.
            NiShaderFactory* pkFactory = NiShaderFactory::GetInstance();
            if (pkFactory)
            {
                NiShaderLibrary* pkShaderLib = pkFactory->GetFirstLibrary();
                while (pkShaderLib)
                {
                    m_libraries.Add(pkShaderLib);
                    pkShaderLib = pkFactory->GetNextLibrary();
                }
            }
        }

        if (!NiShaderParser::GetNumParserCallbacks() &&
            !NiShaderFactory::GetNumLibraryCallbacks())
        {
            m_state = SS_STATE_READY;
            return AsyncResult_Complete;
        }
        else
        {
            AddInitialFiles();
            m_state = SS_STATE_FINDING_PROGRAMS;
        }

        m_findingProgramsIndex = 0;
        m_programLocateMessageCount = 0;

        // Early out if we've spent to much time in OnInit()
        if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
            return AsyncResult_Pending;
        // fall through

    case SS_STATE_FINDING_PROGRAMS:

        // If we have fired off all the AssetLocates for the program category and we have
        // processed all the associated asset locate response messages for this category
        // then we can proceed to the next state
        if (m_findingProgramsIndex == m_shaderProgramTypes.size() &&
            m_programLocateMessageCount >= m_shaderProgramTypes.size())
        {
            m_state = SS_STATE_FOUND_PROGRAMS;
        }

        // Note: due to the way overrides work in the asset controller, we cannot search
        // for urn:shader, because if we have two files "name.ext1" and "name.ext2", it will
        // only return one of them.  Therefore, we must search per extension explicitly so that
        // files are only overridden by name within a given mime type.
        for (; m_findingProgramsIndex < m_shaderProgramTypes.size();
            m_findingProgramsIndex++)
        {
            utf8string mimeType = "urn:" + m_shaderProgramTypes[m_findingProgramsIndex];
            m_pAssetLocator->AssetLocate(mimeType, m_programCategory);

            // Early out if we've spent to much time in OnInit()
            if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
            {
                m_findingProgramsIndex++;
                return AsyncResult_Pending;
            }
        }
        return AsyncResult_Pending;

    case SS_STATE_FOUND_PROGRAMS:
    {
        if (m_pendingParseLocate.size() == 0)
        {
            if (m_pendingLoadLocate.size() > 0)
            {
                m_state = SS_STATE_PARSING;
            }
            else
            {
                m_state = SS_STATE_LOADING;
            }
            return AsyncResult_Pending;
        }

        // Fire off asset locate messages for all shaders that need to be parsed.
        for (; m_parsingIterator != m_pendingParseLocate.end();
            ++m_parsingIterator)
        {
            m_pAssetLocator->AssetLocate(m_parsingIterator->first, m_parseCategory);

            // Early out if we've spent to much time in OnInit()
            if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
            {
                ++m_parsingIterator;
                return AsyncResult_Pending;
            }
        }

        m_state = SS_STATE_PARSING;
        return AsyncResult_Pending;
    }
    case SS_STATE_PARSING:
    {
        efd::list<PendingWorkPtr>::iterator itParseWork = m_pendingParse.begin();

        // Iterate through the list of pending shaders to parse.
        bool bDone = m_pendingParse.size() == 0;
        while (!bDone)
        {
            // Early out if we've spent to much time in OnInit()
            if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
                return AsyncResult_Pending;

            PendingWork* pPendingWork = *itParseWork;
            ParseShader(pPendingWork);
            m_pendingParse.remove(pPendingWork);

            if (m_pendingParse.begin() == m_pendingParse.end())
            {
                bDone = true;
                continue;
            }

            itParseWork = m_pendingParse.begin();
        }

        // Fire off asset locate messages for all shaders that need to be loaded.
        for (; m_loadingIterator != m_pendingLoadLocate.end();
            ++m_loadingIterator)
        {
            m_pAssetLocator->AssetLocate(m_loadingIterator->first, m_loadCategory);

            // Early out if we've spent to much time in OnInit()
            if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
            {
                ++m_loadingIterator;
                return AsyncResult_Pending;
            }
        }

        // State set to SS_STATE_LOADING in HandleAssetLocatorResponse
        return AsyncResult_Pending;
    }
    case SS_STATE_LOADING:
    {
        efd::list<PendingWorkPtr>::iterator itParseWork = m_pendingLoad.begin();

        // Iterate through the list of pending shaders to load.
        bool bDone = m_pendingLoad.size() == 0;
        while (!bDone)
        {
            // Early out if we've spent to much time in OnInit()
            if (NiGetCurrentTimeInSec() - startTime >= m_maxOnInitTime)
                return AsyncResult_Pending;

            PendingWork* pPendingWork = *itParseWork;
            LoadShader(pPendingWork);
            m_pendingLoad.remove(pPendingWork);

            if (m_pendingLoad.begin() == m_pendingLoad.end())
            {
                bDone = true;
                continue;
            }
            itParseWork = m_pendingLoad.begin();
        }

        m_state = SS_STATE_READY;
        // fall through
    }
    default:
    case SS_STATE_READY:
        // No - op
        // Fall through to the return
        break;
    }

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult ShaderService::OnTick()
{
    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult ShaderService::OnShutdown()
{
    m_parsers.RemoveAll();
    m_libraries.RemoveAll();

    m_pMessageService->Unsubscribe(this, m_parseCategory);
    m_pMessageService->Unsubscribe(this, m_loadCategory);
    m_pMessageService->Unsubscribe(this, m_programCategory);

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void ShaderService::CreateShaderParsers()
{
    UInt32 numParsers = NiShaderParser::GetNumParserCallbacks();
    for (UInt32 i = 0; i < numParsers; i++)
    {
        NiShaderParser* pParser = (*NiShaderParser::GetParserCallback(i))();
        if (!pParser)
            continue;

        EE_ASSERT(pParser);
        m_parsers.Add(pParser);
    }
}

//------------------------------------------------------------------------------------------------
void ShaderService::CreateShaderLibraries()
{
    NiShaderFactory* pFactory = NiShaderFactory::GetInstance();
    EE_ASSERT(pFactory);
    if (!pFactory)
        return;

    UInt32 numLibraries = NiShaderFactory::GetNumLibraryCallbacks();
    for (UInt32 i = 0; i < numLibraries; i++)
    {
        NiShaderLibrary* pLibrary;
        const char* dirs[] = {""};
        bool success = (*NiShaderFactory::GetLibraryCallback(i))(
            NiRenderer::GetRenderer(),
            0,
            dirs,
            false,
            &pLibrary);

        if (!success)
            continue;

        EE_ASSERT(pLibrary);
        m_libraries.Add(pLibrary);
        pFactory->InsertLibrary(pLibrary);
    }
}

//------------------------------------------------------------------------------------------------
void ShaderService::AddInitialFiles()
{
    // Add files to parse
    {
        for (UInt32 i = 0; i < m_parsers.GetSize(); i++)
        {
            NiShaderParser* pParser = m_parsers.GetAt(i);
            UInt32 numMimeTypes = pParser->GetNumSupportedMimeTypes();
            for (UInt32 m = 0; m < numMimeTypes; m++)
            {
                const char* type = pParser->GetSupportedMimeType(m);
                utf8string str = "urn:";
                str += type;
                m_pendingParseLocate[str] = pParser;
            }
        }
        // Point the iterator to the begining of the map.
        m_parsingIterator = m_pendingParseLocate.begin();
    }

    // Add files to load
    {
        for (UInt32 i = 0; i < m_libraries.GetSize(); i++)
        {
            NiShaderLibrary* pLibrary = m_libraries.GetAt(i);
            UInt32 numMimeTypes = pLibrary->GetNumSupportedMimeTypes();
            for (UInt32 m = 0; m < numMimeTypes; m++)
            {
                const char* type = pLibrary->GetSupportedMimeType(m);
                utf8string str = "urn:";
                str += type;
                m_pendingLoadLocate[str] = pLibrary;
            }
        }
        // Point the iterator to the begining of the map.
        m_loadingIterator = m_pendingLoadLocate.begin();
    }
}

//------------------------------------------------------------------------------------------------
void ShaderService::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    utf8string uri = pMessage->GetResponseURI();

    if (targetChannel == m_programCategory)
    {
        CreateProgramMap(pMessage);
        m_programLocateMessageCount++;
        return;
    }
    else if (targetChannel == m_parseCategory)
    {
        NiShaderParser* pParser;
        if (m_pendingParseLocate.find(uri, pParser))
        {
            // Find shader library with matching mime type for this parser.
            const char* outputMimeType = pParser->GetOutputMimeType();
            NiShaderLibrary* outputLibrary = FindShaderLibForMimeType(outputMimeType);

            if (!outputLibrary)
            {
                EE_LOG(efd::kGamebryoGeneral0,
                    efd::ILogger::kERR2,
                    ("ShaderService::ParseShader: "
                    "No shader library supports mime type '%s'.  Parsing files anyway.",
                    outputMimeType));
            }

            const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();
            AssetLocatorResponse::AssetURLMap::const_iterator it;

            // Add all shaders that require parsing for this mime type to the list
            // of shaders that need to be parsed. These shaders will be parsed over
            // multiple OnInit() calls.
            for (it = assets.begin(); it != assets.end(); ++it)
            {
                utf8string path = it->second.url;

                PendingWorkPtr spPendingParse = EE_NEW PendingWork();
                spPendingParse->m_pLibrary = outputLibrary;
                spPendingParse->m_pParser = pParser;
                spPendingParse->m_pFileName = path.c_str();

                m_pendingParse.push_back(spPendingParse);
            }

            return;
        }
    }
    else if (targetChannel == m_loadCategory)
    {
        if (m_loadingIterator == m_pendingLoadLocate.end())
            m_state = SS_STATE_LOADING;

        NiShaderLibrary* pLibrary;
        if (m_pendingLoadLocate.find(uri, pLibrary))
        {
            const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();
            AssetLocatorResponse::AssetURLMap::const_iterator it;

            // Add all shaders that require loading for this mime type to the list
            // of shaders that need to be loaded. These shaders will be loaded over
            // multiple OnInit() calls.
            for (it = assets.begin(); it != assets.end(); ++it)
            {
                FixedString path = it->second.url.c_str();

                PendingWorkPtr spPendingLoad = NiNew PendingWork();
                spPendingLoad->m_pLibrary = pLibrary;
                spPendingLoad->m_pFileName = path;

                m_pendingLoad.push_back(spPendingLoad);
            }

            return;
        }
    }

    EE_LOG(efd::kGamebryoGeneral0,
        efd::ILogger::kERR2,
        ("ShaderService::HandleAssetLocatorResponse: "
        "Unknown uri: '%s'",
        uri.c_str()));
}

//------------------------------------------------------------------------------------------------
NiShaderLibrary* ShaderService::FindShaderLibForMimeType(const char* pMimeType)
{
    // Find shader library with matching mime type for this parser.
    NiShaderLibrary* outputLibrary = NULL;
    for (UInt32 l = 0; l < m_libraries.GetSize(); l++)
    {
        NiShaderLibrary* library = m_libraries.GetAt(l);
        UInt32 numMimeTypes = library->GetNumSupportedMimeTypes();
        for (UInt32 m = 0; m < numMimeTypes; m++)
        {
            if (strcmp(library->GetSupportedMimeType(m), pMimeType))
                continue;

            outputLibrary = library;
            break;
        }

        if (outputLibrary)
            break;
    }

    return outputLibrary;
}

//------------------------------------------------------------------------------------------------
void ShaderService::ParseShader(PendingWork* pPendingParse)
{
    NiTObjectArray<NiFixedString> generated;
    UInt32 count;

    EE_ASSERT(pPendingParse->m_pParser);

    // Parse the shader in the provided PendingWork object.
    if (pPendingParse->m_pParser->ParseFile(pPendingParse->m_pFileName.c_str(), count, &generated))
    {
        if (pPendingParse->m_pLibrary)
        {
            // If a library exists for the shader type, then add the shader to the
            // set of shaders to load.
            for (UInt32 i = 0; i < generated.GetSize(); i++)
            {
                PendingWorkPtr pPendingLoad = EE_NEW PendingWork();

                pPendingLoad->m_pLibrary = pPendingParse->m_pLibrary;
                pPendingLoad->m_pFileName = generated.GetAt(i);

                m_pendingLoad.push_back(pPendingLoad);
            }
        }
    }
    generated.RemoveAll();

}

//------------------------------------------------------------------------------------------------
bool ShaderService::LoadShader(PendingWork* pPendingWork)
{
    EE_ASSERT(pPendingWork && pPendingWork->m_pLibrary);
    bool success = pPendingWork->m_pLibrary->LoadShader(pPendingWork->m_pFileName.c_str());

    if (!success)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR2,
            ("ShaderService::LoadShader: "
            "Failed loading shader '%s'.",
            pPendingWork->m_pFileName.c_str()));
    }

    return success;
}

//------------------------------------------------------------------------------------------------
void ShaderService::CreateProgramMap(const efd::AssetLocatorResponse* msg)
{
    NiShaderFactory* pFactory = NiShaderFactory::GetInstance();
    EE_ASSERT(pFactory);
    if (!pFactory)
        return;

    const AssetLocatorResponse::AssetURLMap& assets = msg->GetAssetURLMap();
    AssetLocatorResponse::AssetURLMap::const_iterator it;
    for (it = assets.begin(); it != assets.end(); ++it)
    {
        utf8string path = it->second.url;
        const utf8string& file = PathUtils::PathGetFileName(path);
        pFactory->SetShaderProgramLocation(file.c_str(), path.c_str());
    }
}

//------------------------------------------------------------------------------------------------
void ShaderService::AddShaderProgramType(const char* pMimeType)
{
    m_shaderProgramTypes.push_back(pMimeType);
}

//------------------------------------------------------------------------------------------------
