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

#pragma once
#ifndef EE_CAMERADATA_H
#define EE_CAMERADATA_H

#include "ecrLibType.h"

#include <NiCamera.h>
#include <egf/Entity.h>
#include <efd/SmartPointer.h>
#include <egf/EntityManager.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>
#include <egf/BuiltinModelHelper.h>

#include <ecr/ecrClassIDs.h>

namespace ecr
{

/**
    CameraData holds per camera data needed by the CameraService. This data includes
    an NiCamera, a reference to the camera entity, and a zoom factor.
*/
class EE_ECR_ENTRY CameraData : public egf::IBuiltinModelImpl
{
    /// @cond EMERGENT_INTERNAL
    EE_DECLARE_CLASS1(
        CameraData,
        efd::kCLASSID_CameraData,
        egf::IBuiltinModelImpl);
    EE_DECLARE_CONCRETE_REFCOUNT;

    EE_DECLARE_BUILTINMODEL_PROPERTIES
    
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_FOV, efd::Float32, CameraData, GetFOV, SetFOV)
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_NearPlane, efd::Float32, CameraData, GetNearPlane, SetNearPlane)
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_FarPlane, efd::Float32, CameraData, GetFarPlane, SetFarPlane)
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_MinimumNearPlane, efd::Float32, CameraData, GetMinimumNearPlane, SetMinimumNearPlane)
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_MaximumFarToNearRatio, efd::Float32, CameraData, GetMaximumFarToNearRatio, SetMaximumFarToNearRatio)

        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_IsOrthographic, efd::Bool, CameraData, GetIsOrthographic, SetIsOrthographic)
        EE_BUILTINMODELPROPERTY_ACCESSOR(CameraData, egf::kPropertyID_StandardModelLibrary_LODAdjust, efd::SInt32, CameraData, GetLODAdjust, SetLODAdjust)

    EE_END_BUILTINMODEL_PROPERTIES

public:

    inline void SetFOV(const efd::Float32& FOV);
    inline efd::Float32 GetFOV()const;

    inline void SetNearPlane(const efd::Float32& NearPlane);
    inline efd::Float32 GetNearPlane()const;

    inline void SetFarPlane(const efd::Float32& FarPlane);
    inline efd::Float32 GetFarPlane()const;

    inline void SetMinimumNearPlane(const efd::Float32& MinNearPlane);
    inline efd::Float32 GetMinimumNearPlane()const;

    inline void SetMaximumFarToNearRatio(const efd::Float32& MaxFtoN);
    inline efd::Float32 GetMaximumFarToNearRatio()const;

    inline void SetIsOrthographic(const efd::Bool& IsOrthographic);
    inline efd::Bool GetIsOrthographic()const;

    inline void SetLODAdjust(const efd::SInt32& LODAdjust);
    inline efd::SInt32 GetLODAdjust()const;

    inline void SetCamera(NiCamera* camera);
    inline NiCamera* GetCamera()const;
     /// @endcond

public:
    /**
    The factory class for the CameraData built-in model.

    It is public to allow the egf::FlatModelManager to register the factory in its
    PreInit method.
    */
    static egf::IBuiltinModel* CameraModelFactory();

    /// Fully qualified constructor. The zoom is set to 1.
    CameraData();
    virtual ~CameraData();

    /// Get the entity ID of the camera.
    egf::EntityID GetId() const;

    /// Set the entity ID of the camera.
    void SetId(egf::EntityID id);

    /**
        Set the rotation for the camera on the entity. Note that this will not directly
        change the NiCamera object's rotation. That is handled by the CameraService when
        it receives an entity update message. Since rotation on entities is stored as
        a triple of Euler angles, this matrix form of the set method will extract the angles
        from the rotation matrix.
        @param rotation Camera rotation stored as a 3x3 rotation matrix.
    */
    void SetRotate(const NiMatrix3& rotation);

    /**
        Get the translation of the camera entity.
    */
    const efd::Point3 GetTranslate()const;

    /**
        Get the rotation of the camera entity as a triple of Euler angles.
    */
    efd::Point3 GetRotate()const;

    /**
        Get the scale of the camera entity.
    */
    efd::Float32 GetScale();

    /**
        Get the zoom of the camera entity.
    */
    efd::Float32 GetZoomFactor();

    /**
        Set the translation for the camera on the entity. Note that this will not directly
        change the NiCamera object's rotation. That is handled by the CameraService when
        it receives an entity update message.
    */
    void SetTranslate(efd::Point3 translation);

    /**
        Set the rotation for the camera on the entity. Note that this will not directly
        change the NiCamera object's rotation. That is handled by the CameraService when
        it receives an entity update message.
        @param rotation Camera rotation stored as an Euler angle triple.
    */
    void SetRotate(efd::Point3 rotation);

    /**
        Set the scale for the camera on the entity. Note that this will not directly
        change the NiCamera object's rotation. That is handled by the CameraService when
        it receives an entity update message.
    */
    void SetScale(efd::Float32 scale);

    /**
        Set the zoom for the camera on the entity. Note that this will not directly
        change the NiCamera object's rotation. That is handled by the CameraService when
        it receives an entity update message.
    */
    void SetZoomFactor(efd::Float32 zoom);

    /**
        Update this camera.
    */
    virtual void Update();

protected:

    egf::EntityID m_id;

    NiCameraPtr m_spCamera;

private:

    efd::SInt32 m_LODAdjust;
    
    efd::Bool m_IsOrthographic;

    efd::Float32 m_MaxFtoN;
    efd::Float32 m_MinNearPlane;
    efd::Float32 m_FarPlane;
    efd::Float32 m_NearPlane;
    efd::Float32 m_Fov;

public:

    /// Current zoom of the camera.
    efd::Float32 m_zoom;
};

typedef efd::SmartPointer<CameraData> CameraDataPtr;

}; // namespace

#include "CameraData.inl"

#endif // EE_CAMERADATA_H
