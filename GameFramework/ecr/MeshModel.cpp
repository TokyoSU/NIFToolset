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

// Pre-compiled header
#include "ecrPCH.h"

#include <ecr/SceneGraphService.h>
#include <ecr/RenderService.h>

#include <ecr/MeshModel.h>

using namespace efd;
using namespace egf;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(MeshModel);

EE_IMPLEMENT_BUILTINMODEL_PROPERTIES(MeshModel);

//--------------------------------------------------------------------------------------------------
IBuiltinModel* MeshModel::MeshModelFactory()
{
    return EE_NEW MeshModel();
}

//--------------------------------------------------------------------------------------------------
MeshModel::MeshModel()
    : m_nifAsset()
    , m_isNifAssetShared(true)
{
}
    
//--------------------------------------------------------------------------------------------------
MeshModel::~MeshModel()
{
}

//--------------------------------------------------------------------------------------------------
bool MeshModel::Initialize(egf::Entity* pOwner, const egf::PropertyDescriptorList& defaults)
{
    IBuiltinModel::Initialize(pOwner, defaults);

    for (PropertyDescriptorList::const_iterator iter = defaults.begin();
        iter != defaults.end();
        ++iter)
    {
        switch ((*iter)->GetPropertyID())
        {
            case kPropertyID_StandardModelLibrary_NifAsset:
                (*iter)->GetDefaultProperty()->GetValue(
                    kPropertyID_StandardModelLibrary_NifAsset,
                    &m_nifAsset);
                break;

            case kPropertyID_StandardModelLibrary_IsNifAssetShared:
                (*iter)->GetDefaultProperty()->GetValue(
                    kPropertyID_StandardModelLibrary_IsNifAssetShared,
                    &m_isNifAssetShared);
                break;

            case kPropertyID_StandardModelLibrary_AttachedObjects:
                IAttachedObjectsProperty::SetAttachedObjectsProperty((*iter));
                break;

            default:;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool MeshModel::ResetProperty(const egf::PropertyDescriptor* pDefault)
{
    switch (pDefault->GetPropertyID())
    {
        case kPropertyID_StandardModelLibrary_NifAsset:
            pDefault->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_NifAsset,
                &m_nifAsset);
            break;

        case kPropertyID_StandardModelLibrary_IsNifAssetShared:
            pDefault->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_IsNifAssetShared,
                &m_isNifAssetShared);
            break;

        case kPropertyID_StandardModelLibrary_AttachedObjects:
            IAttachedObjectsProperty::SetAttachedObjectsProperty(pDefault);
            break;

        default:;
    }
    
    return true;
}

//--------------------------------------------------------------------------------------------------
void MeshModel::OnAdded()
{
    SceneGraphService* pSceneGraphService =
        GetServiceManager()->GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(pSceneGraphService);

    pSceneGraphService->AddEntityWithMeshModel(m_pOwningEntity);
}

//--------------------------------------------------------------------------------------------------
void MeshModel::OnRemoved()
{
    // If OnRemoved is called after EntityManager shutdown this could be NULL
    const efd::ServiceManager* pSrvMgr = GetServiceManager();
    if (pSrvMgr)
    {
        // If OnRemoved is called after SceneGraphService shutdown this could be NULL
        SceneGraphService* pSceneGraphService = pSrvMgr->GetSystemServiceAs<SceneGraphService>();
        if (pSceneGraphService)
        {
            pSceneGraphService->RemoveEntityWithMeshModel(m_pOwningEntity);
        }
    }

    IBuiltinModel::OnRemoved();
}

//--------------------------------------------------------------------------------------------------
bool MeshModel::operator==(const IProperty& other) const
{
    if (!IBuiltinModel::operator ==(other))
        return false;
        
    const MeshModel* otherClass = static_cast<const MeshModel *>(&other);

    if (m_nifAsset != otherClass->m_nifAsset ||
        m_isNifAssetShared != otherClass->m_isNifAssetShared)
    {
        return false;
    }

    return IAttachedObjectsProperty::operator ==(*otherClass);
}

//--------------------------------------------------------------------------------------------------
