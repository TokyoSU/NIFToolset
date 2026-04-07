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

#include <ecr/RenderService.h>
#include <ecr/IDefaultSurfaceCreator.h>
#include <NiShadowManager.h>
#include <NiImageConverter.h>
#include <NiDevImageConverter.h>
#include <NiSourceTexture.h>
#include <egf/egfClassIDs.h>
#include <egf/egfLogIDs.h>
#include <efd/ServiceManager.h>
#include <efd/IConfigManager.h>
#include <efd/PathUtils.h>

#ifdef EE_PLATFORM_WIN32
#include <efd/Win32/Win32PlatformService.h>
#endif

using namespace egf;
using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(RenderService);
//------------------------------------------------------------------------------------------------
RenderService::RenderService(NiWindowRef parentHandle)
    : m_parentHandle(parentHandle)
    , m_spRenderer(0)
    , m_pActiveSurface(0)
    , m_pDefaultSurfaceCreator(NULL)
    , m_defaultRenderQueue(2000)
    , m_kRenderQueueExtraData("g_RenderQueue")
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 1000;
}

//------------------------------------------------------------------------------------------------
RenderService::~RenderService()
{
    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        m_renderContexts[ui] = 0;
    }

    // Shut down the shadow manager.
    NiShadowManager::Shutdown();

    InternalDestructor();

    m_spRenderer = NULL;
}

//------------------------------------------------------------------------------------------------
const char* ecr::RenderService::GetDisplayName() const
{
    return "RenderService";
}

//------------------------------------------------------------------------------------------------
SyncResult RenderService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_ASSERT(!m_spRenderer);

#ifdef EE_PLATFORM_WIN32
    // If we're on Win32, we need the platform service initialized so that we can get the window
    // handle. Both Win32PlatformService and SDL3PlatformService are accepted.
    pDependencyRegistrar->AddDependency<Win32PlatformService>(sdf_Optional);
#   if defined(EE_PLATFORM_SERVICE_SDL3)
    pDependencyRegistrar->AddDependency(efd::kCLASSID_SDL3PlatformService, sdf_Optional);
#   endif
#endif

    efd::IConfigManager* pConfig = m_pServiceManager->GetSystemServiceAs<efd::IConfigManager>();
    if (pConfig)
    {
        efd::utf8string shaderPath = pConfig->FindValue("RenderService.ShaderCachePath");
        if (!shaderPath.empty())
        {
            if (PathUtils::IsRelativePath(shaderPath))
            {
                shaderPath = PathUtils::ConvertToAbsolute(shaderPath);
            }
            NiMaterial::SetDefaultWorkingDirectory(shaderPath.c_str());
        }
    }
    if (NULL == NiMaterial::GetDefaultWorkingDirectory())
    {
        utf8string shaderPath = PathUtils::PathMakeNative("Data/Shaders/Generated");
        shaderPath = PathUtils::ConvertToAbsolute(shaderPath);
        NiMaterial::SetDefaultWorkingDirectory(shaderPath.c_str());
    }

    NiImageConverter::SetImageConverter(EE_NEW NiDevImageConverter);
    NiTexture::SetMipmapByDefault(true);
    NiSourceTexture::SetUseMipmapping(true);

    // Initialize the shadow manager.  It is initially set to be active.  Also, all entities are
    // initially set to casts and receives shadows.  However, by default, all lights are initially
    // set to not cast shadows.  This approach makes it straightforward to selectively enable
    // shadowing capabilities for individual lights, without having to consider the shadow manager
    // itself or individual cast/receive shadow properties.
    NiShadowManager::Initialize();
    NiShadowManager::SetActive(true);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult RenderService::OnInit()
{
    if (!CreateRenderer())
    {
        NiMessageBox("Unable to create a Renderer.\n", "Renderer Creation Failed");
        return AsyncResult_Failure;
    }

    if (!m_pDefaultSurfaceCreator ||
        !m_pDefaultSurfaceCreator->CreateDefaultSurface(this, m_defaultContextBackgroundColor))
    {
        RenderContext* pRenderContext = EE_NEW RenderContext(this);
        AddRenderContext(pRenderContext);
        pRenderContext->SetBackgroundColor(m_defaultContextBackgroundColor);
        RenderSurfacePtr spSurface = CreateRenderSurface(NULL);
        pRenderContext->AddRenderSurface(spSurface);
        SetActiveRenderSurface(spSurface);
    }

    // Replace EE_ASSERT with a real runtime check — the assert is stripped in Release
    // and allows OnInit() to return Complete with a null active surface.
    if (!GetActiveRenderSurface())
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR0,
            ("RenderService::OnInit — active surface is null after init. "
             "Check that CreateRenderer() succeeded and that any custom "
             "IDefaultSurfaceCreator calls SetActiveRenderSurface()."));
        return AsyncResult_Failure;
    }

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult RenderService::OnTick()
{
    efd::Float32 currTime = (efd::Float32)m_pServiceManager->GetTime(kCLASSID_GameTimeClock);

    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        (m_renderContexts[ui])->Draw(currTime);
    }

    // Return pending to indicate the service should continue to be ticked.
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
AsyncResult RenderService::OnShutdown()
{
    for (DelegateList::iterator i = m_renderServiceDelegates.begin();
         i != m_renderServiceDelegates.end(); ++i)
    {
        (*i)->OnRenderServiceShutdown(this);
    }

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
bool RenderService::OnDeviceLost(void* pvData)
{
    RenderService* pRenderService = (RenderService*)pvData;
    EE_ASSERT(pRenderService);

    EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
        ("RenderService: Device Lost"));

    EE_UNUSED_ARG(pRenderService);

    return true;
}

//------------------------------------------------------------------------------------------------
bool RenderService::OnDeviceReset(bool bBeforeReset, void* pvData)
{
    RenderService* pRenderService = (RenderService*)pvData;

    EE_ASSERT(pRenderService);

    if (bBeforeReset)
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL1,
            ("RenderService: Device Reset (Before)"));
    }
    else
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL1,
            ("RenderService: Device Reset (After)"));

        efd::UInt32 contexts = static_cast<efd::UInt32>(pRenderService->m_renderContexts.size());
        for (efd::UInt32 ui = 0; ui < contexts; ui++)
        {
            RenderContext* pContext = pRenderService->m_renderContexts[ui];

            efd::UInt32 surfaces = pContext->GetRenderSurfaceCount();
            for (efd::UInt32 n = 0; n < surfaces; n++)
            {
                RenderSurface* pSurface = pContext->GetRenderSurfaceAt(n);
                pRenderService->RecreateRenderSurface(pSurface);
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
RenderSurface* RenderService::GetRenderSurface(efd::WindowRef window) const
{
    /// Find the surface
    RenderSurface* pSurface = 0;
    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        pSurface = m_renderContexts[ui]->GetRenderSurface(window);
        if (pSurface)
            return pSurface;
    }

    return (RenderSurface*)0;
}

//------------------------------------------------------------------------------------------------
void RenderService::AddRenderedEntity(egf::Entity* pEntity, NiAVObject* pAVObject)
{
    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        (m_renderContexts[ui])->AddRenderedEntity(pEntity, pAVObject);
    }

    // Alert delegates that a new entity was added
    for (DelegateList::iterator i = m_renderServiceDelegates.begin();
         i != m_renderServiceDelegates.end(); ++i)
    {
        (*i)->OnRenderedEntityAdded(this, pEntity, pAVObject);
    }
}

//------------------------------------------------------------------------------------------------
void RenderService::RemoveRenderedEntity(egf::Entity* pEntity, NiAVObject* pAVObject)
{
    // Alert delegates that a new entity is about to be removed
    for (DelegateList::iterator i = m_renderServiceDelegates.begin();
         i != m_renderServiceDelegates.end(); ++i)
    {
        (*i)->OnRenderedEntityRemoved(this, pEntity, pAVObject);
    }

    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        (m_renderContexts[ui])->RemoveRenderedEntity(pEntity, pAVObject);
    }
}

//------------------------------------------------------------------------------------------------
void RenderService::UpdateRenderedEntity(egf::Entity* pEntity, NiAVObject* pAVObject)
{
	RemoveRenderedEntity(pEntity, pAVObject);
	AddRenderedEntity(pEntity, pAVObject);
}

//------------------------------------------------------------------------------------------------
void RenderService::AddRenderedObject(SceneGraphService::SceneGraphHandle handle,
    NiAVObject* pAVObject)
{
    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        (m_renderContexts[ui])->AddRenderedObject(handle, pAVObject);
    }
}

//------------------------------------------------------------------------------------------------
void RenderService::RemoveRenderedObject(SceneGraphService::SceneGraphHandle handle,
    NiAVObject* pAVObject)
{
    efd::UInt32 size = GetRenderContextCount();
    for (efd::UInt32 ui = 0; ui < size; ++ui)
    {
        (m_renderContexts[ui])->RemoveRenderedObject(handle, pAVObject);
    }
}

//------------------------------------------------------------------------------------------------
void RenderService::SetActiveRenderSurface(efd::WindowRef window)
{
    RenderSurface* pSurface = GetRenderSurface(window);

    SetActiveRenderSurface(pSurface);
}

//------------------------------------------------------------------------------------------------
bool RenderService::SetDefaultSurfaceCreator(IDefaultSurfaceCreator* pCreator)
{
    if (NULL == GetActiveRenderSurface())
    {
        m_pDefaultSurfaceCreator = pCreator;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool RenderService::SetDefaultSurfaceBackgroundColor(const efd::ColorA& color)
{
    if (NULL == GetActiveRenderSurface())
    {
        m_defaultContextBackgroundColor = color;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 RenderService::GetRenderQueueID(egf::Entity* pEntity, NiAVObject* pScene)
{
    // Default the queueID to the default render queue
    efd::UInt32 queueID = m_defaultRenderQueue;

    // See if the model is Renderable
    if (pEntity)
    {
        const FlatModel* pModel = pEntity->GetModel();
        if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_Renderable))
        {
            // Get the queue property from the Entity, if the property cannot be found,
            // then the default render queue is used
            pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Queue, queueID);

            return queueID;
        }
    }

    NiIntegerExtraData* pExtraData = NiDynamicCast(NiIntegerExtraData, 
        pScene->GetExtraData(m_kRenderQueueExtraData));
    if (pExtraData)
    {
        queueID = pExtraData->GetValue();
    }
    
    return queueID;
}

//--------------------------------------------------------------------------------------------------
void RenderService::SetRenderQueue(NiAVObject* pScene, efd::UInt32 renderQueue)
{
    NiIntegerExtraData* pExtraData = NiDynamicCast(NiIntegerExtraData, 
        pScene->GetExtraData(m_kRenderQueueExtraData));
    if (!pExtraData)
    {
        pExtraData = EE_NEW NiIntegerExtraData(0);
        pScene->AddExtraData(m_kRenderQueueExtraData, pExtraData);
    }
    pExtraData->SetValue(renderQueue);
}

//--------------------------------------------------------------------------------------------------
void RenderService::SetRenderQueue(egf::Entity* pEntity, efd::UInt32 renderQueue)
{
    // See if the model is Renderable
    const FlatModel* pModel = pEntity->GetModel();
    if (pModel->ContainsModel(kFlatModelID_StandardModelLibrary_Renderable))
    {
        // Get the queue property from the Entity, if the property cannot be found,
        // then the default render queue is used
        pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_Queue, renderQueue);
    }
}

//--------------------------------------------------------------------------------------------------