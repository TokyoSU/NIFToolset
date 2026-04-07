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

#include "efdPCH.h"

#include <efd/Metrics.h>
#include <efd/ServiceManager.h>
#include <efd/ConfigManager.h>
#include <efd/MessageHandlerBase.h>
#include <efd/MessageService.h>
#include <efd/NetMessage.h>
#include <efd/ParseHelper.h>
#include <efd/PathUtils.h>
#include <efd/IAssetServer.h>
#include <efd/AssetLocatorRequest.h>
#include <efd/AssetLocatorResponse.h>
#include <efd/AssetUriReader.h>
#include <efd/AssetConfigService.h>
#include <efd/ReloadManager.h>
#include <efd/AssetLocatorService.h>
#include <efd/efdLogIDs.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetLocatorService);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetLocatorService::AssetLocationMap);

/*static*/ efd::UInt16 AssetLocatorService::m_responseID = 1;

//------------------------------------------------------------------------------------------------
const Category& efd::GetAssetServerServiceCategory()
{
    static const efd::Category kCAT_AssetServerPublicCategory(
        efd::UniversalID::ECU_Any,
        efd::kNetID_ISystemService,
        IAssetServer::CLASS_ID);
    return kCAT_AssetServerPublicCategory;
}

//------------------------------------------------------------------------------------------------
const Category& efd::GetAssetServerProxyCategory()
{
    static const Category s_cat(
        efd::UniversalID::ECU_PrivateChannel,
        kNetID_Any,
        IAssetServer::CLASS_ID);
    return s_cat;
}

//------------------------------------------------------------------------------------------------
const efd::Category& efd::GetAssetClientServiceCategory()
{
    static const efd::Category kCAT_AssetClientPublicCategory(
            efd::UniversalID::ECU_EventChannel,
            kNetID_ISystemService,
            AssetLocatorService::CLASS_ID);

    return kCAT_AssetClientPublicCategory;
}

//------------------------------------------------------------------------------------------------
efd::Category efd::GetAssetClientPrivateCategory(efd::UInt32 virtualProcessID)
{
    return efd::Category(
            efd::UniversalID::ECU_PrivateChannel,
            virtualProcessID,
            AssetLocatorService::CLASS_ID);
}

//------------------------------------------------------------------------------------------------
efd::Category efd::GenerateAssetResponseCategory()
{
    Category responseCategory(
        efd::UniversalID::ECU_Any,
        kNetID_Unassigned,
        AssetLocatorService::CLASS_ID);

    responseCategory.SetNetID((AssetLocatorService::m_responseID++) << 1);
    return responseCategory;
}

EE_HANDLER_WRAP(
    AssetLocatorService,
    HandleConnect,
    StreamMessage,
    kMSGID_NetServiceConnectionEstablished);

EE_HANDLER_WRAP(
    AssetLocatorService,
    HandleDisconnect,
    StreamMessage,
    kMSGID_NetServiceConnectionClosed);

EE_HANDLER_WRAP(
    AssetLocatorService,
    HandleAssetConfigMsg,
    AssetConfigurationMessage,
    kMSGID_AssetConfigResponse);

EE_HANDLER_WRAP(
    AssetLocatorService,
    HandleResponse,
    AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

EE_HANDLER(AssetLocatorService, HandleTagsInfoResponse, AssetTagsInfoResponse);

EE_HANDLER_WRAP(
    AssetLocatorService,
    AssetDataReceived,
    FetchAssetDataResponse,
    kMSGID_AssetFetchDataResponse);

EE_HANDLER_WRAP(
    AssetLocatorService,
    AssetChangeReceived,
    AssetLocatorResponse,
    kMSGID_AssetServerChangeNotify);

//------------------------------------------------------------------------------------------------
AssetLocatorService::AssetLocatorService(
    ServiceManager* pServiceManager,
    const utf8string& globalTagSpace)
    : m_dataState(ASSETS_NONE)
    , m_spMessageService(NULL)
    , m_spToolsMsgService(NULL)
    , m_fetching_from_server(0)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2110;

    // We'll need an AssetConfigService to manage my configuration
    AssetConfigServicePtr pService = EE_NEW AssetConfigService();
    EE_ASSERT(pService != 0);
    pServiceManager->RegisterSystemService(pService);

    URIReader r = URIReader(globalTagSpace);
    utf8string temp;
    r.getTagLiterals(m_globalTags);
    r.getCanonicalLiteral(temp);
    if (temp.length())
    {
        m_globalTags.push_back(temp);
    }

    EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
        ("AssetLocatorService using global tag space %s",
        globalTagSpace.c_str()));
}

//------------------------------------------------------------------------------------------------
AssetLocatorService::~AssetLocatorService()
{
}

//------------------------------------------------------------------------------------------------
const char* AssetLocatorService::GetDisplayName() const
{
    return "AssetLocator";
}

//------------------------------------------------------------------------------------------------
// Client API functions - connection related
//------------------------------------------------------------------------------------------------
void AssetLocatorService::ConnectionEstablished(efd::Bool isConnected)
{
    // DT32288 The connect API is used to negotiate the per-client protocol that will be used to
    // calculate asset URLs in the response messages.

    // From the server side, we will eventually need to receive a list of physical
    // asset URL prefixes, which will be matched against the protocols supported
    // by the client.  The protocol prefix selected by the client then gets
    // pre-pended to the asset URLs the server returns in responses. The result is
    // the FULL asset URL which is most appropriate for this client to use
    // to load the asset.

    if (isConnected)
    {
        if (m_dataState == ASSETS_NONE)
            m_dataState = ASSETS_REMOTE;
        else if (m_dataState == ASSETS_LOCAL_ONLY)
            m_dataState = ASSETS_LOCAL_AND_REMOTE;

        EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
            ("AssetLocatorService: Local and Remote query ENABLED."));
    }
    else
    {
        if (m_dataState == ASSETS_REMOTE)
            m_dataState = ASSETS_NONE;
        else if (m_dataState == ASSETS_LOCAL_AND_REMOTE)
            m_dataState = ASSETS_LOCAL_ONLY;

        EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
            ("AssetLocatorService: Remote query DISABLED."));
    }
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::AssetChangeReceived(const AssetLocatorResponse* message, efd::Category)
{
    // When an asset is known to have changed, it needs to be added to the local change list with
    // a state set to modified.
    const AssetLocatorResponse::AssetURLMap& assets = message->GetAssetURLMap();
    AssetLocatorResponse::AssetURLMap::const_iterator it = assets.begin();
    for (; it != assets.end(); ++it)
    {
        m_asset_changes[it->first] = ASSET_MODIFIED;

        EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
            ("%s> %s (%s) -> %s", __FUNCTION__,
            it->first.c_str(), it->second.tagset.c_str(), it->second.url.c_str()));
    }

    // Forward a change notification to the ReloadManager Service.
    m_spMessageService->SendLocal(
        message,
        ReloadManager::GetReloadManagerCategory());
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::AssetDataReceived(const FetchAssetDataResponse* pMessage, efd::Category)
{
    // Decrement the outstanding fetch requests.
    EE_ASSERT(m_fetching_from_server > 0);
    --m_fetching_from_server;

    // Figure out the local path to the file being updated.
    utf8string uri = pMessage->GetResponseURI();
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();
    AssetLocatorResponse::AssetURLMap::const_iterator f = assets.find(uri);
    EE_ASSERT(f != assets.end());

    utf8string relativePath = f->second.url;
    EE_ASSERT(relativePath.size() > 0);

    utf8string fullPath;
    m_metastore.assetAbsolutePath(relativePath, fullPath);

    // Make sure that absolute path is platform-ready.
    fullPath = PathUtils::PathMakeNative(fullPath);

    // Make sure the path exists.
    utf8string fullDir = PathUtils::PathRemoveFileName(fullPath);
    if (!File::DirectoryExists(fullDir.c_str()))
    {
        File::CreateDirectoryRecursive(fullDir.c_str());
    }

    // Get the asset data from the message and write it to the local file system.
    const FetchAssetDataResponse* msg = EE_DYNAMIC_CAST(FetchAssetDataResponse, pMessage);
    efd::UInt32 size = msg->GetBufferSize();
    FetchAssetDataResponse::FetchResult result = msg->GetResult();

    if (result == FetchAssetDataResponse::kFETCH_SUCCESS && size > 0)
    {
        const char* buffer = msg->GetAssetData();

        // Write it out to disk.
        File *pkFile = File::GetFile(fullPath.c_str(), File::WRITE_ONLY);
        EE_ASSERT_MESSAGE(
            pkFile != 0,
            ("AssetLocatorService was unable to write an updated asset to disk.\n"
             "Path to asset: %s\n"
             "Verify the directory exists and the file is writable.",
             fullPath.c_str()));

        if (pkFile)
        {
            pkFile->Seek(0, File::SO_BEGIN);
            pkFile->Write(buffer, size);
            EE_DELETE pkFile;
        }
    }
    else if (result != FetchAssetDataResponse::kFETCH_SUCCESS)
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kERR1,
            ("AssetLocatorService unable to fetch updated data for %s",
            fullPath.c_str()));
    }

    // Remove asset from the change list.
    m_asset_changes.erase(uri);

    // If there are no more outstanding fetch requests, then flush all the queued response
    // messages.  All associated assets have been locally updated.
    if (m_fetching_from_server == 0)
    {
        FlushLocalResponseQueue();
    }
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::AssetBrowseInfoReceived(
    const AssetBrowseInfoResponse* pMessage,
    efd::Category)
{
    // Get URI that the response is the answer to
    utf8string uri = pMessage->GetResponseURI();

    // Get the associated request for the response.
    RequestIterator rq = m_PendingRequests.find(uri);
    if (rq != m_PendingRequests.end())
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kLVL3,
            ("Remote Asset BrowseInfo Response for URI '%s'", uri.c_str()));

        // Remove from the pending requests.
        m_PendingRequests.erase(rq);
    }
    else
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kERR1,
            ("Remote Asset BrowseInfo Response for unknown request URI '%s'", uri.c_str()));
    }
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::CheckChangeList(const AssetLocatorResponse::AssetURLMap& url_map)
{
    AssetLocatorResponse::AssetURLMap::const_iterator i;
    for (i = url_map.begin(); i != url_map.end(); ++i)
    {
        AssetChanges::iterator ci = m_asset_changes.find(i->first);
        if (ci != m_asset_changes.end())
        {
            // Asset is changed, need to fetch it from the server
            EE_ASSERT(m_dataState >= ASSETS_REMOTE);

            ci->second = ASSET_LOADING;
            SendAssetFetchRequest(i->first);

            // We are now awaiting server response to 'N' fetch requests
            ++m_fetching_from_server;
        }
    }
}

//------------------------------------------------------------------------------------------------
bool AssetLocatorService::IsConnected()
{
    return m_dataState >= ASSETS_REMOTE;
}

//------------------------------------------------------------------------------------------------
bool AssetLocatorService::IsReady()
{
    return m_dataState >= ASSETS_LOCAL_ONLY;
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::LocalLookup(const utf8string& uri, efd::Category& callback)
{
    EE_ASSERT(IsReady());

    EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
        ("LocalLookup for URI '%s'",uri.c_str()));

    // perform search of the Asset Web
    TripleSet triples;
    AssetLocatorResponse::AssetURLMap url_map;
    m_metastore.findAllAssets(uri, triples);
    MakeAssetURLMap(triples, url_map);

    if (triples.size() > 0)
    {
        EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
            ("Asset Locator Service found %d assets that matched URI '%s'",
            triples.size(), uri.c_str()));
    }
    else
    {
        EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
            ("Asset Locator Service found no assets that matched URI '%s'", uri.c_str()));
    }

    SendLocalResponse(uri, callback, url_map);
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::MakeAssetURLMap(
    const TripleSet& triples,
    AssetLocatorResponse::AssetURLMap& url_map)
{
    for (TripleSetIterator i = triples.begin(); i != triples.end(); i++)
    {
        utf8string name, tagset, relative, llid_part;

        m_metastore.assetPropertyValue(i->subject, URIReader::GlobalNamespace_relpath, relative);
        m_metastore.assetPropertyValue(i->subject, URIReader::GlobalNamespace_name, name);
        m_metastore.assetAllTags(i->subject, tagset);
        m_metastore.assetPropertyValue(i->subject, URIReader::GlobalNamespace_llid, llid_part);

        utf8string llid = "urn:llid:" + llid_part;

        AssetLocatorResponse::AssetLoc loc = {name, tagset, "", relative, llid};
        url_map[i->subject] = loc;
    }
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::SendAssetFetchRequest(const efd::utf8string& uri)
{
    FetchAssetDataRequestPtr req = EE_NEW FetchAssetDataRequest;

    req->SetURI(uri);

    // If our app is sharing the asset web with the asset server they will also share the asset
    // web instance id. If the asset server is restarted it generates a new instance id. Force the
    // asset web to reload the instance file each time we request the instance id for fetch
    // requests. This allows us to restart toolbench / Asset server while the application is
    // running while avoiding the problem where the app writes the fetch asset to disk, triggering
    // an asset change in the server, resulting in the app doing another fetch and getting into
    // an endless loop.
    m_metastore.closeGraph("awebinst");

    // Server will use our asset web instance Id to determine how to process the fetch request.
    utf8string id;
    m_metastore.getInstanceId(id);
    req->SetAssetWebInstanceId(id);

    // pull the entire asset in one request.
    req->SetFullFetch(true);

    // set the appropriate category for the server to respond on
    req->CallbackCategory(m_assetLocateChannel);

    // send the message
    SendRequest(req, GetAssetServerServiceCategory());
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::SendLocalResponse(
    const efd::utf8string& uri,
    const efd::Category& callback,
    const AssetLocatorResponse::AssetURLMap& url_map)
{
    // remove any global tags from the search URI
    vector<utf8string>::iterator iter;
    utf8string tempURI = uri;
    for (iter = m_globalTags.begin(); iter != m_globalTags.end(); ++iter)
    {
        utf8string temp = ":" + *iter;

        utf8string::size_type sp = uri.rfind(temp);
        if (sp != utf8string::npos)
        {
            utf8string::size_type tagLength = temp.length();

            utf8string::size_type ep = uri.find(":", sp + tagLength);
            if (ep == utf8string::npos || ep == (sp + tagLength + 1))
            {
                tempURI.replace(sp, ep, "");
            }
        }
    }

    // create response message
    AssetLocatorResponsePtr resp =
        EE_NEW MessageWrapper<AssetLocatorResponse, kMSGID_AssetLocatorResponse>;

    resp->SetResponseURI (tempURI);
    resp->ClientCategory(callback);

    AssetLocatorResponse::AssetURLMap::const_iterator i;
    for (i = url_map.begin(); i != url_map.end(); i++)
    {
        // ensure asset URL's contain local absolute paths
        AssetLocatorResponse::AssetLoc loc = i->second;
        utf8string absolute;
        m_metastore.assetAbsolutePath(loc.url, absolute);

        // make sure that absolute path is platform-ready
        absolute = PathUtils::PathMakeNative(absolute);
        loc.url = absolute;

        resp->SetAssetLoc(i->first, loc);
    }

    // Send the response locally. Measure the time it takes to process so that we can provide
    // a warning about slow asset loaders.
    METRICS_ONLY(TimeType howlong = GetCurrentTimeInSec());
    m_spMessageService->SendLocal(resp, callback);

    METRICS_ONLY(
    howlong = GetCurrentTimeInSec()-howlong;
    if (howlong > 0.1)  // 100 ms
    {
        EE_LOG(
            efd::kAssets,
            efd::ILogger::kLVL1,
            ("SLOW LOAD: Asset loaders took %f ms to load '%s'",
            howlong,
            uri.c_str()));
    })
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::QueueLocalResponse(
    const efd::utf8string& uri,
    const efd::Category& callback,
    const AssetLocatorResponse::AssetURLMap& url_map)
{
    LocalResponseQueueEntry entry;
    entry.uri = uri;
    entry.callback = callback;
    entry.urls = url_map;

    m_local_response_queue.push_back(entry);
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::FlushLocalResponseQueue()
{
    vector<LocalResponseQueueEntry>::iterator i;

    for (i = m_local_response_queue.begin(); i != m_local_response_queue.end(); ++i)
    {
        SendLocalResponse(i->uri, i->callback, i->urls);
    }
    m_local_response_queue.clear();
}

//------------------------------------------------------------------------------------------------
efd::Bool AssetLocatorService::AssetLocate(
    const efd::utf8string& uri,
    efd::Category callback,
    AssetLookupScope scope)
{
    // append any global tags to the search URI
    utf8string qualifiedURI(uri);

    vector<utf8string>::iterator iter;
    for (iter = m_globalTags.begin(); iter != m_globalTags.end(); ++iter)
    {
        qualifiedURI = qualifiedURI + ":";
        qualifiedURI = qualifiedURI + *iter;
    }

    // Determine which type of lookup - local or remote
    bool remoteLookup = (m_dataState >= ASSETS_REMOTE) && (scope != ALS_LOCAL_ONLY);

    if (remoteLookup)
    {
        // setup the request message
        AssetLocatorRequestPtr assetRequest =
            EE_NEW MessageWrapper<AssetLocatorRequest, kMSGID_AssetLocatorRequest>();
        assetRequest->SetURI(qualifiedURI);
        assetRequest->CallbackCategory(m_assetLocateChannel);
        assetRequest->ClientCategory(callback);

        // send the message
        SendRequest(assetRequest, GetAssetServerServiceCategory());

        // keep track of outstanding queries
        m_PendingRequests.push_back(qualifiedURI);

        return true;
    }
    else if (m_dataState != ASSETS_NONE)
    {
        // Do the local lookup
        LocalLookup(qualifiedURI, callback);

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool AssetLocatorService::AssetBrowseInfo(
    const efd::utf8string& uri,
    const efd::utf8string& filePathHint /* = efd::utf8string::NullString() */)
{
    // Browse Info can only be retrieved from an Asset Server. It is not supported to compute it
    // locally.
    EE_ASSERT (m_dataState >= ASSETS_REMOTE);

    if (m_dataState < ASSETS_REMOTE)
        return false;

    // setup the request message
    AssetLocatorRequestPtr assetRequest = AssetLocatorRequest::CreateTypedMessage(
        kMSGID_AssetBrowseInfoRequest,
        m_spToolsMsgService);
    assetRequest->SetURI(uri);
    assetRequest->SetPathHint(filePathHint);

    // send the message
    SendRequest(assetRequest, GetAssetServerServiceCategory());

    // keep track of outstanding queries
    m_PendingRequests.push_back(uri);

    return true;
}

//------------------------------------------------------------------------------------------------
bool AssetLocatorService::AssetTagsInfo(AssetTagCategory category)
{
    // Tag Info can only be retrieved from an Asset Server. It is not supported to compute it
    // locally.
    EE_ASSERT(m_dataState >= ASSETS_REMOTE);

    // setup the request message
    AssetTagsInfoRequestPtr assetRequest =
        EE_NEW MessageWrapper<AssetTagsInfoRequest, kMSGID_AssetTagsInfoRequest>();
    assetRequest->SetCategory (category);

    // send the message
    m_spToolsMsgService->Send(assetRequest, GetAssetServerServiceCategory());

    return true;
}

//------------------------------------------------------------------------------------------------
AssetLocatorService::AssetLocationMapPtr
AssetLocatorService::AssetLocationsFromResponseMsg(const efd::IMessage* pMessage)
{
    const AssetLocatorResponse* pResponseMessage = EE_DYNAMIC_CAST(AssetLocatorResponse, pMessage);

    AssetLocationMapPtr retval = EE_NEW AssetLocationMap(
        pResponseMessage->GetResponseURI(),
        pResponseMessage->GetAssetURLMap());
    return retval;
}

//------------------------------------------------------------------------------------------------
AssetLocatorService::AssetLocationMap::AssetLocationMap(
    const efd::utf8string& uri,
    const AssetLocatorResponse::AssetURLMap& loc)
{
    m_searchURI = uri;
    AssetLocatorResponse::AssetURLMap::const_iterator iurl;
    for (iurl = loc.begin(); iurl != loc.end(); iurl++)
    {
        AssetQueryResult result;
        result.assetID = iurl->first;
        result.assetName = iurl->second.name;
        result.assetURL = iurl->second.url;
        m_assetResults.push_back(result);
    }
}

//------------------------------------------------------------------------------------------------
UInt32 AssetLocatorService::AssetLocationMap::Count() const
{
    return (UInt32)m_assetResults.size();
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorService::AssetLocationMap::Query() const
{
    return m_searchURI;
}

//------------------------------------------------------------------------------------------------
Bool AssetLocatorService::AssetLocationMap::HasTag(const utf8string& tag) const
{
    URIReader parse = URIReader(m_searchURI);
    return parse.hasTag(tag);
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorService::AssetLocationMap::GetID(UInt32 index) const
{
    EE_ASSERT(index < Count());
    return m_assetResults[index].assetID;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorService::AssetLocationMap::GetName(UInt32 index) const
{
    EE_ASSERT(index < Count());
    return m_assetResults[index].assetName;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorService::AssetLocationMap::GetURL(UInt32 index) const
{
    EE_ASSERT(index < Count());
    return m_assetResults[index].assetURL;
}

//------------------------------------------------------------------------------------------------
efd::Bool AssetLocatorService::AssetWebOpen(const efd::utf8string& web_root)
{
    Bool retval = false;
    if (web_root.length())
    {
        m_metastore.closeWeb();

        EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
            ("AssetLocatorService::Open asset web at %s",
            web_root.c_str()));

        AssetConfigService* pConfig = m_pServiceManager->GetSystemServiceAs<AssetConfigService>();
        EE_ASSERT(pConfig);
        m_metastore.SetPreloadGraphs(pConfig->GetPreloadGraphs());

        retval = m_metastore.openWeb(web_root, AssetWeb::AWEB_READ_ONLY);

        if (m_dataState == ASSETS_NONE)
        {
            m_dataState = ASSETS_LOCAL_ONLY;
            EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
                ("AssetLocatorService Remote query DISABLED."));
        }
        else if (m_dataState == ASSETS_REMOTE)
        {
            m_dataState = ASSETS_LOCAL_AND_REMOTE;
            EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
                ("AssetLocatorService Remote query ENABLED."));
        }
    }

    return retval;
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::GetAssetWebAuthorId(efd::utf8string& guid)
{
    // On Windows, getAuthorId() will get the user's author id directly off the system. However,
    // this may not be what you want if you are trying to run the game on a second PC apart from
    // the one you are running the Asset Ctrl on.  We allow this configuration by supporting a
    // configuration setting of AssetWeb.OverrideSystemId=true.
    AssetConfigService* pConfig = m_pServiceManager->GetSystemServiceAs<AssetConfigService>();
    EE_ASSERT (pConfig);

    utf8string value_str;
    pConfig->GetConfigValue("OverrideSystemId", value_str);
    Bool bValue = false;
    ParseHelper<Bool>::FromString(value_str, bValue);

    if (bValue)
    {
        m_metastore.getWebAuthorId(guid);
    }
    else
    {
        m_metastore.getAuthorId(guid);
    }
}

//------------------------------------------------------------------------------------------------
efd::SyncResult AssetLocatorService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<AssetConfigService>();
    // We need the MessageService to be ready for sending response messages before this service
    // can do anything meaningful.
    pDependencyRegistrar->AddDependency<MessageService>();

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);

    RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetServerConnectNotify>(
        m_spMessageService);
    RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetLocatorResponse>(
        m_spMessageService);
    RegisterMessageWrapperFactory<AssetBrowseInfoResponse, kMSGID_AssetBrowseInfoResponse>(
        m_spMessageService);
    RegisterMessageFactory<AssetTagsInfoResponse>(m_spMessageService);

    RegisterMessageWrapperFactory<AssetLocatorRequest, kMSGID_AssetLocatorRequest>(
        m_spMessageService);
    RegisterMessageWrapperFactory<AssetLocatorRequest, kMSGID_AssetBrowseInfoRequest>(
        m_spMessageService);
    RegisterMessageWrapperFactory<AssetTagsInfoRequest, kMSGID_AssetTagsInfoRequest>(
        m_spMessageService);
    RegisterMessageFactory<FetchAssetDataRequest>(m_spMessageService);

    // Register the response message class factory for change notifications
    RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetServerChangeNotify>(
        m_spMessageService);
    RegisterMessageWrapperFactory<FetchAssetDataResponse,kMSGID_AssetFetchDataResponse>(
        m_spMessageService);

    m_spMessageService->Subscribe(this, kCAT_LocalMessage);

    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetLocatorService::OnInit()
{
    m_spToolsMsgService =
        m_pServiceManager->GetSystemServiceAs<MessageService>(kCLASSID_ToolsMessageService);
    if (!m_spToolsMsgService)
    {
        m_spToolsMsgService = m_spMessageService;
    }
    else
    {
        RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetServerConnectNotify>(
            m_spToolsMsgService);
        RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetLocatorResponse>(
            m_spToolsMsgService);
        RegisterMessageWrapperFactory<AssetBrowseInfoResponse, kMSGID_AssetBrowseInfoResponse>(
            m_spToolsMsgService);
        RegisterMessageFactory<AssetTagsInfoResponse>(m_spToolsMsgService);
        RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetServerChangeNotify>(
            m_spToolsMsgService);
        RegisterMessageWrapperFactory<FetchAssetDataResponse,kMSGID_AssetFetchDataResponse>(
            m_spToolsMsgService);
    }

    m_spToolsMsgService->Subscribe(this, kCAT_LocalMessage);
    m_spToolsMsgService->Subscribe(this, GetAssetClientServiceCategory());

    m_assetLocateChannel = GetAssetClientPrivateCategory(m_spToolsMsgService->GetNetID());
    m_spToolsMsgService->Subscribe(this, m_assetLocateChannel);

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetLocatorService::OnShutdown()
{
    if (m_spMessageService)
    {
        m_spMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_spMessageService = NULL;
    }

    if (m_spToolsMsgService)
    {
        m_spToolsMsgService->Unsubscribe(this, GetAssetClientServiceCategory());
        m_spToolsMsgService->Unsubscribe(this, kCAT_LocalMessage);
        m_spToolsMsgService->Unsubscribe(this, m_assetLocateChannel);

        m_spToolsMsgService = NULL;
    }

    m_PendingRequests.clear();

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::HandleConnect(const efd::StreamMessage* pStreamMessage, efd::Category)
{
    EE_UNUSED_ARG(pStreamMessage);
    EE_ASSERT(kMSGID_NetServiceConnectionEstablished == pStreamMessage->GetClassID());
    EE_LOG(efd::kAssets, efd::ILogger::kLVL1,
        ("%s> NetServiceConnectionEstablished message received",
        __FUNCTION__));

    // We now know we have a valid VirtualProcessID. Subscribe for asset fetch responses on our
    // private category.
    efd::Category newCat = GetAssetClientPrivateCategory(m_spToolsMsgService->GetNetID());
    if (newCat != m_assetLocateChannel)
    {
        m_spToolsMsgService->Unsubscribe(this, m_assetLocateChannel);
        m_assetLocateChannel = newCat;
        m_spToolsMsgService->Subscribe(this, m_assetLocateChannel);
    }

    AssetLocatorResponsePtr resp =
        EE_NEW MessageWrapper<AssetLocatorResponse, kMSGID_AssetServerConnectNotify>();
    m_spToolsMsgService->Send(resp, GetAssetServerServiceCategory());
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::HandleDisconnect(const efd::StreamMessage* pStreamMessage, efd::Category)
{
    EE_UNUSED_ARG(pStreamMessage);
    EE_ASSERT(kMSGID_NetServiceConnectionClosed == pStreamMessage->GetClassID());
    EE_LOG(efd::kAssets, efd::ILogger::kLVL1,
        ("%s> NetServiceConnectionClosed message received",
        __FUNCTION__));

    ConnectionEstablished(false);
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::HandleAssetConfigMsg(
    const efd::AssetConfigurationMessage* pMessage,
    efd::Category)
{
    // Ignore any local configuration changes to the asset service.
    if (!pMessage->IsRemoteConfig())
        return;

    utf8string newAssetWebRoot = pMessage->GetAssetWebPath();

    // This is less than perfect in that we are assuming that if the remote Asset Controller has
    // opened any asset web (as indicated by a path value), then our application should use the
    // Asset Controller for asset lookups.
    if (!newAssetWebRoot.empty())
    {
        EE_LOG(efd::kAssets, efd::ILogger::kLVL1,
            ("Rapid Iteration enabled. <ContentRootDirectory = '%s'>", newAssetWebRoot.c_str()));

        ConnectionEstablished(true);
    }
    else
    {
        EE_LOG(efd::kAssets, efd::ILogger::kLVL1, ("Rapid Iteration disabled."));

        ConnectionEstablished(false);
    }
}

//------------------------------------------------------------------------------------------------
/* virtual */
void AssetLocatorService::HandleResponse(const efd::AssetLocatorResponse* pResponseMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pResponseMessage);

    // Get the class of the response message.

    EE_ASSERT(pResponseMessage->GetClassID() != kMSGID_AssetServerChangeNotify);
    EE_ASSERT(pResponseMessage->GetClassID() != kMSGID_AssetFetchDataResponse);
    EE_ASSERT(pResponseMessage->GetClassID() != kMSGID_AssetBrowseInfoResponse);

    // Handle remote Asset Location responses.
    {
        // Get URI that the response is the answer to.
        utf8string uri = pResponseMessage->GetResponseURI();

        // Get the associated request for the response.
        RequestIterator rq = m_PendingRequests.find(uri);
        if (rq != m_PendingRequests.end())
        {
            EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
                ("Remote Asset Locate Response for URI '%s'", uri.c_str()));

            // Remove from the pending requests.
            m_PendingRequests.erase (rq);
        }
        else
        {
            EE_LOG(efd::kAssets, efd::ILogger::kERR1,
                ("Remote Asset Locate Response for unknown request URI '%s'", uri.c_str()));
        }

        // Check if any of the assets are in the change list.
        const AssetLocatorResponse::AssetURLMap& url_map = pResponseMessage->GetAssetURLMap();
        CheckChangeList(url_map);

        // Log count of assets in the response.  It *may* indicate a problem if a request returns
        // zero matching assets.
        if (url_map.size() > 0)
        {
            EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
                ("Asset Locator Service found %d assets that matched URI '%s'",
                url_map.size(),
                uri.c_str()));
        }
        else
        {
            EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
                ("Asset Locator Service found no assets that matched URI '%s'. "\
                 "For troubleshooting docs, see Programmer Level>Foundation>"\
                 "Asset Runtime Service>Troubleshooting",
                uri.c_str()));
        }

        // Either queue or send the response to local handlers, depending on if we are awaiting
        // the outcome of fetch requests.
        if (m_fetching_from_server > 0)
            QueueLocalResponse(uri, pResponseMessage->ClientCategory(), url_map);
        else
            SendLocalResponse(uri, pResponseMessage->ClientCategory(), url_map);
    }
}

//------------------------------------------------------------------------------------------------
void AssetLocatorService::HandleTagsInfoResponse(
    const efd::AssetTagsInfoResponse* pResponseMessage,
    efd::Category)
{
    EE_UNUSED_ARG(pResponseMessage);
    EE_ASSERT(pResponseMessage);
    EE_LOG(efd::kAssets, efd::ILogger::kLVL3,
        ("Remote Asset Server sent TagsInfo Response"));
}

//------------------------------------------------------------------------------------------------
/* virtual */
void AssetLocatorService::SendRequest(AssetLocatorRequest* pMessage, const Category &category)
{
    // log some message statistics
    EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
        ("AssetLocatorService::SendRequest sending new request for %s",
        pMessage->GetURI().c_str()));

    // send the message through the net service to the Asset Server
    m_spToolsMsgService->Send(pMessage, category);
}

//------------------------------------------------------------------------------------------------
