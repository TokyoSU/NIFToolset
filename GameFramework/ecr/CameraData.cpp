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

#include "CameraData.h"

#include <egf/EntityManager.h>
#include <egf/StandardModelLibraryPropertyIDs.h>

#include "RenderService.h"

using namespace egf;
using namespace efd;
using namespace ecr;


EE_IMPLEMENT_CONCRETE_CLASS_INFO(CameraData);

EE_IMPLEMENT_BUILTINMODEL_PROPERTIES(CameraData);

//--------------------------------------------------------------------------------------------------

CameraData::CameraData()
    :m_zoom(1)
{
    m_spCamera = NULL;
}

//--------------------------------------------------------------------------------------------------
CameraData::~CameraData()
{
    m_spCamera = NULL;
}

//--------------------------------------------------------------------------------------------------
egf::EntityID CameraData::GetId() const
{
    return m_id;
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetId(egf::EntityID id)
{
    m_id = id;
}

//--------------------------------------------------------------------------------------------------
const efd::Point3 CameraData::GetTranslate() const
{
    if(m_spCamera)
    {
        return m_spCamera->GetWorldTranslate();
    }
    else 
    {
        egf::Entity* pEntity = this->m_pOwningEntity;
        EE_ASSERT(pEntity);

        efd::Point3 position;
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position, position);

        return position;
    }
}

//--------------------------------------------------------------------------------------------------
efd::Point3 CameraData::GetRotate() const
{
    if(m_spCamera)
    {
        const NiMatrix3& MatrixRotate = m_spCamera->GetWorldRotate();
        efd::Point3 rotation;
        float x,y,z;
        MatrixRotate.ToEulerAnglesXYZ(x,y,z);
        rotation.x = x * -EE_RADIANS_TO_DEGREES;
        rotation.y = y * -EE_RADIANS_TO_DEGREES;
        rotation.z = z * -EE_RADIANS_TO_DEGREES;
        return rotation;
    }
    else
    {
        egf::Entity* pEntity = this->m_pOwningEntity;
        EE_ASSERT(pEntity);

        efd::Point3 rotation;
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Rotation, rotation);

        return rotation;
    }
}

//--------------------------------------------------------------------------------------------------
efd::Float32 CameraData::GetScale()
{
    if(m_spCamera)
    {
        return m_spCamera->GetWorldScale();
    }
    else
    {
        egf::Entity* pEntity = this->m_pOwningEntity;
        EE_ASSERT(pEntity);

        efd::Float32 scale;
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Scale, scale);

        return scale;
    }
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetTranslate(efd::Point3 translation)
{
    egf::Entity* pEntity = this->m_pOwningEntity;
    EE_ASSERT(pEntity);

    pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_Position, translation);
    if(m_spCamera)
    {
        m_spCamera->SetWorldTranslate(translation);
        m_spCamera->Update(1.0f/60.0f,false);
    }
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetRotate(efd::Point3 rotation)
{
    egf::Entity* pEntity = this->m_pOwningEntity;
    EE_ASSERT(pEntity);

    pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_Rotation, rotation);
    if(m_spCamera)
    {
        NiMatrix3 kXRot, kYRot, kZRot;

        kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS);
        kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS);
        kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS);
        m_spCamera->SetWorldRotate(kXRot * kYRot * kZRot);
        m_spCamera->Update(1.0f/60.0f,false);
    }
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetScale(efd::Float32 scale)
{
    egf::Entity* pEntity = this->m_pOwningEntity;
    EE_ASSERT(pEntity);

    pEntity->SetPropertyValue(kPropertyID_StandardModelLibrary_Scale, scale);
    if(m_spCamera)
    {
        m_spCamera->SetWorldScale(scale);
        m_spCamera->Update(1.0f/60.0f,false);
    }
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetRotate(const NiMatrix3& rotation)
{
    efd::Point3 finalRotation;
    rotation.ToEulerAnglesXYZ(finalRotation.x, finalRotation.y, finalRotation.z);

    finalRotation.x = finalRotation.x * -EE_RADIANS_TO_DEGREES;
    finalRotation.y = finalRotation.y * -EE_RADIANS_TO_DEGREES;
    finalRotation.z = finalRotation.z * -EE_RADIANS_TO_DEGREES;

    SetRotate(finalRotation);
}

//--------------------------------------------------------------------------------------------------

efd::Float32 CameraData::GetZoomFactor()
{
    return m_zoom;
}

//--------------------------------------------------------------------------------------------------
void CameraData::SetZoomFactor(efd::Float32 zoom)
{
    m_zoom = zoom;
}

//--------------------------------------------------------------------------------------------------
egf::IBuiltinModel* ecr::CameraData::CameraModelFactory()
{
    return EE_NEW CameraData();
}

//--------------------------------------------------------------------------------------------------
void CameraData::Update()
{
    EE_ASSERT(m_spCamera);
    {
        // Now that this is a built-in model
        // We might be able to do away with the following code

        efd::Point3 position;
        if ( m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Position) && 
            m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position,
            position) == PropertyResult_OK)
        {
            m_spCamera->SetWorldTranslate(position);
        }

        efd::Point3 rotation;
        if (m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Rotation) && 
            m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Rotation,
            rotation) == PropertyResult_OK)
        {
            NiMatrix3 kXRot, kYRot, kZRot;

            kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS);
            kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS);
            kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS);
            m_spCamera->SetWorldRotate(kXRot * kYRot * kZRot);
        }

        float scale = 1.0f;
        if (m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Scale) && 
            m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Scale,
            scale) == PropertyResult_OK)
        {
            m_spCamera->SetWorldScale(scale);
        }

        // If position rotation or scale changed then we need to update our camera
        if(m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Position) || 
            m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Rotation) || 
            m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_Scale))
        {
            m_spCamera->Update(1.0f/60.0f,false);
        }
    }

    float lodAdjust;
    if (m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_LODAdjust) && 
        m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_LODAdjust,
        lodAdjust) == PropertyResult_OK)
    {
        m_spCamera->SetLODAdjust(lodAdjust);
    }

    float minNearPlane;
    if (m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_MinimumNearPlane) && 
        m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MinimumNearPlane,
        minNearPlane) == PropertyResult_OK)
    {
        m_spCamera->SetMinNearPlaneDist(minNearPlane);
    }

    float maxFarNearRatio;
    if (m_pOwningEntity->IsDirty(kPropertyID_StandardModelLibrary_MaximumFarToNearRatio) && 
        m_pOwningEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaximumFarToNearRatio,
        maxFarNearRatio) == PropertyResult_OK)
    {
        m_spCamera->SetMaxFarNearRatio(maxFarNearRatio);
    }
}

//--------------------------------------------------------------------------------------------------
