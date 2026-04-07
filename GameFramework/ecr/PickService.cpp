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
#include "NiVersion.h"
#include <NiPick.h>

#include <NiPSParticleSystem.h>

#include <efd/ServiceManager.h>
#include <efd/MessageService.h>

#include <egf/EntityManager.h>

#include "PickService.h"
#include "SceneGraphService.h"

using namespace efd;
using namespace egf;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(PickService);

//------------------------------------------------------------------------------------------------
efd::Bool PickService::PickObjectFunctor::operator()(
    const egf::Entity* pEntity,
    const efd::vector<NiObjectPtr>& objects)
{
    EE_ASSERT(objects.size() > 0);
    EE_ASSERT(m_pPickService);
    EE_ASSERT(m_pEntityManager);

    NiPick::PickObjectPolicy* pPickPolicy = NULL;
    if (!m_pPickService->m_pickPolicies.empty())
    {
        efd::map<efd::utf8string, NiPick::PickObjectPolicyPtr>::iterator itor =
            m_pPickService->m_pickPolicies.begin();

        while (itor != m_pPickService->m_pickPolicies.end())
        {
            if (pEntity->GetModel()->ContainsModel(itor->first))
            {
                pPickPolicy = itor->second;
                break;
            }

            ++itor;
        }
    }

    if (!pPickPolicy)
    {
        pPickPolicy = m_pPickService->m_spDefaultPickPolicy;
        NiAVObject* pNode = NiDynamicCast(NiAVObject, objects[0]);
        if (pNode)
        {
            m_pPickService->m_pPicker->SetPickObjectPolicy(pPickPolicy);
            m_pPickService->m_pPicker->SetTarget(pNode);
            m_pPickService->m_pPicker->PickObjects(m_rayOrigin, m_rayDir, true);
        }
    }
    else
    {
        // If a custom pick policy is in place for the current entity, we call it directly instead
        // of going through the picker directly. It is assumed that custom pick policies assigned
        // to entities of a particular model can completely resolve a pick request.
        NiRenderObject* pRenderObj = NiDynamicCast(NiRenderObject, objects[0]);
        pPickPolicy->FindIntersections(m_rayOrigin, m_rayDir, *m_pPickService->m_pPicker,
            pRenderObj);
    }

    return false;
}

//------------------------------------------------------------------------------------------------
PickService::PickService(
    const bool bPickAppCulled,
    const bool bPickFrontOnly,
    const bool bPickClosestOnly) :
    m_pPicker(NULL),
    m_bPickFrontOnly(bPickFrontOnly),
    m_bPickAppCulled(bPickAppCulled),
    m_bPickClosestOnly(bPickClosestOnly)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 1550;
}

//------------------------------------------------------------------------------------------------
PickService::~PickService()
{
    // This method intentionally left blank (all shutdown occurs in OnShutdown)
}

//------------------------------------------------------------------------------------------------
const char* PickService::GetDisplayName() const
{
    return "PickService";
}

//------------------------------------------------------------------------------------------------
SyncResult PickService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    m_pPicker = EE_NEW NiPick();
    m_pPicker->SetReturnNormal(true);
    m_pPicker->SetIntersectType(NiPick::INTERSECT_TRIANGLE);

    if (m_spDefaultPickPolicy == NULL)
    {
        m_spDefaultPickPolicy = EE_NEW IgnoreParticlePickObjectPolicy();
    }

    m_spSceneGraphService = m_pServiceManager->GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(m_spSceneGraphService);

    m_pickFunctor.m_pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    EE_ASSERT(m_pickFunctor.m_pEntityManager);

    m_pickFunctor.m_pPickService = this;

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult PickService::OnShutdown()
{
    EE_DELETE m_pPicker;

    m_spDefaultPickPolicy = NULL;
    m_pickPolicies.clear();

    m_spSceneGraphService = NULL;

    // Not a smart pointer, but this prevents any inadvertent use
    m_pickFunctor.m_pEntityManager = NULL;

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::Bool PickService::RegisterPickPolicy(const efd::utf8string& modelName,
    NiPick::PickObjectPolicy* pPolicy)
{
    if (!pPolicy || m_pickPolicies.find(modelName) != m_pickPolicies.end())
        return false;

    m_pickPolicies[modelName] = pPolicy;
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool PickService::UnregisterPickPolicy(const efd::utf8string& modelName)
{
    m_pickPolicies.erase(modelName);
    return true;
}

//------------------------------------------------------------------------------------------------
PickService::PickRecord* PickService::PerformPick(const efd::Point3& rayOrigin,
    const efd::Point3& rayDir)
{
    return PerformPick(
        rayOrigin,
        rayDir,
        m_bPickFrontOnly,
        m_bPickAppCulled,
        m_bPickClosestOnly);
}

//------------------------------------------------------------------------------------------------
PickService::PickRecord* PickService::PerformPick(
    const efd::Point3& rayOrigin,
    const efd::Point3& rayDir,
    const bool bPickFrontOnly,
    const bool bPickAppCulled,
    const bool bClosestOnly)
{
    EE_ASSERT(m_pPicker);

    m_pPicker->ClearResultsArray();
    m_pPicker->SetFrontOnly(bPickFrontOnly);
    m_pPicker->SetObserveAppCullFlag(!bPickAppCulled);
    m_pPicker->SetQueryType(bClosestOnly ? NiPick::QUERY_CLOSEST : NiPick::QUERY_ALL);

    m_pickFunctor.m_rayOrigin = rayOrigin;
    m_pickFunctor.m_rayDir = rayDir;
    m_spSceneGraphService->ForEachEntitySceneGraph(m_pickFunctor);

    if (m_pPicker->GetResults().GetSize() != 0)
    {
        PickRecord* pPickRecord = EE_NEW PickRecord(rayOrigin, rayDir);

        pPickRecord->m_rayOrigin = rayOrigin;
        pPickRecord->m_rayDirection = rayDir;

        m_pPicker->GetResults().SortResults();
        pPickRecord->m_pResults = &m_pPicker->GetResults();

        return pPickRecord;
    }
    else
    {
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
PickService::PickRecord* PickService::PerformPick(const efd::Point3& rayOrigin,
    const efd::Point3& rayDir, NiAVObject* testObject)
{
    return PerformPick(
        rayOrigin,
        rayDir,
        testObject,
        m_bPickFrontOnly,
        m_bPickAppCulled,
        m_bPickClosestOnly);
}

//------------------------------------------------------------------------------------------------
PickService::PickRecord* PickService::PerformPick(
    const efd::Point3& rayOrigin,
    const efd::Point3& rayDir,
    NiAVObject* testObject,
    const bool bPickFrontOnly,
    const bool bPickAppCulled,
    const bool bClosestOnly)
{
    EE_ASSERT(m_pPicker);

    m_pPicker->ClearResultsArray();
    m_pPicker->SetFrontOnly(bPickFrontOnly);
    m_pPicker->SetObserveAppCullFlag(!bPickAppCulled);
    m_pPicker->SetQueryType(bClosestOnly ? NiPick::QUERY_CLOSEST : NiPick::QUERY_ALL);

    m_pPicker->SetTarget(testObject);
    m_pPicker->PickObjects(rayOrigin, rayDir, true);

    if (m_pPicker->GetResults().GetSize() != 0)
    {
        PickRecord* pPickRecord = EE_NEW PickRecord(rayOrigin, rayDir);

        pPickRecord->m_rayOrigin = rayOrigin;
        pPickRecord->m_rayDirection = rayDir;
        m_pPicker->GetResults().SortResults();
        pPickRecord->m_pResults = &m_pPicker->GetResults();

        return pPickRecord;
    }
    else
    {
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
bool IgnoreParticlePickObjectPolicy::FindIntersections(const NiPoint3& kOrigin,
    const NiPoint3& kDir, NiPick& kPick, NiRenderObject* pkRenderObj)
{
    if (NiIsKindOf(NiPSParticleSystem, pkRenderObj))
        return false;

    return SimpleSkinPickObjectPolicy::FindIntersections(kOrigin, kDir, kPick,
        pkRenderObj);
}

//------------------------------------------------------------------------------------------------
