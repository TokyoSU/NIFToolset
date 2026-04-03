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

#include "CameraService.h"
#include "RenderService.h"
#include "CoreRuntimeMessages.h"
#include <efd/MessageService.h>
#include <efd/PathUtils.h>
#include <egf/Entity.h>
#include <egf/EntityManager.h>
#include <efd/ServiceManager.h>
#include <NiShadowManager.h>
#include <egf/egfClassIDs.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>

using namespace efd;
using namespace egf;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(CameraService);

EE_HANDLER_WRAP(CameraService, HandleEntityDiscoverMessage, EntityChangeMessage,
                kMSGID_OwnedEntityEnterWorld);

EE_HANDLER_WRAP(CameraService, HandleEntityRemovedMessage, EntityChangeMessage,
                kMSGID_OwnedEntityExitWorld);

EE_HANDLER_WRAP(CameraService, HandleEntityUpdatedMessage, EntityChangeMessage,
                kMSGID_OwnedEntityUpdated);

const float CameraService::ms_kParallelThreshold = 0.99f;
const float CameraService::ms_kInvParallelThreshold = 1.0f - CameraService::ms_kParallelThreshold;
float CameraService::ms_kOrthoZoomSpeed = 0.99f;

//------------------------------------------------------------------------------------------------
CameraService::CameraService()
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 4000;
}

//------------------------------------------------------------------------------------------------
CameraService::~CameraService()
{
    // This method intentionally left blank (all shutdown occurs in OnShutdown)
}

//------------------------------------------------------------------------------------------------
const char* CameraService::GetDisplayName() const
{
    return "CameraService";
}

//------------------------------------------------------------------------------------------------
SyncResult CameraService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<RenderService>();

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<efd::MessageService>();
    EE_ASSERT(m_pMessageService);

    m_pEntityManager = m_pServiceManager->GetSystemServiceAs<egf::EntityManager>();
    EE_ASSERT(m_pEntityManager);

    m_spRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();
    EE_ASSERT(m_spRenderService);

    m_spRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();
    EE_ASSERT(m_spRenderService);

    FlatModelManagerPtr m_spFlatModelManager = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();

    m_spRenderService->AddDelegate(this);

    // Register the Camera Model factory
    m_spFlatModelManager->RegisterBuiltinModelFactory(
        "Camera",
        kFlatModelID_StandardModelLibrary_Camera,
        &(ecr::CameraData::CameraModelFactory));

    RegisterForEntityMessages();

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult CameraService::OnInit()
{
    // Perform initial camera updates
    EntityCameraMap::iterator it = m_entityCameraMap.begin();
    for (; it != m_entityCameraMap.end(); ++it)
    {
        Entity* pEntity = m_pEntityManager->LookupEntity(it->first);
        if (!pEntity)
            continue;

        UpdateCamera(it->second, pEntity);
    }

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult CameraService::OnTick()
{
    float currTime = (float)m_pServiceManager->GetTime(kCLASSID_GameTimeClock);

    // Update all the known cameras.
    EntityCameraMap::iterator itor = m_entityCameraMap.begin();
    while (itor != m_entityCameraMap.end())
    {
        CameraData* pData = itor->second;
        if (pData)
        {
            NiCamera* camera = pData->GetCamera();
            camera->Update(currTime);
        }

        ++itor;
    }

    // Return pending to indicate the service should continue to be ticked.
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
AsyncResult CameraService::OnShutdown()
{
    if (m_pMessageService != NULL)
    {
        // Unsubscribes from all messages
        m_pMessageService->Unsubscribe(this, kCAT_LocalMessage);
    }

    if (m_spRenderService != NULL)
        m_spRenderService->RemoveDelegate(this);

    m_entityCameraMap.clear();

    m_spRenderService = NULL;

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void CameraService::RegisterForEntityMessages()
{
    m_pMessageService->Subscribe(this, kCAT_LocalMessage);
}

//------------------------------------------------------------------------------------------------
void CameraService::HandleEntityDiscoverMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);

    Entity* pEntity = pMessage->GetEntity();
    EE_ASSERT(pEntity);

    // Ignore entities that do not contain a camera entity.
    if (!pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Camera))
        return;


    EntityID entityId = pEntity->GetEntityID();

    CameraDataPtr pData = EE_DYNAMIC_CAST(CameraData,pEntity->FindBuiltinModel(kFlatModelID_StandardModelLibrary_Camera));
    EE_ASSERT(pData);
    pData->SetCamera(NiNew NiCamera());
    pData->SetId(entityId);

    m_entityCameraMap[entityId] = pData;
}

//------------------------------------------------------------------------------------------------
void CameraService::HandleEntityRemovedMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);

    m_entityCameraMap.erase(pMessage->GetEntity()->GetEntityID());
}

//------------------------------------------------------------------------------------------------
void CameraService::HandleEntityUpdatedMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    EE_ASSERT(pMessage);

    egf::Entity* pEntity = pMessage->GetEntity();

    CameraDataPtr spCamera;
    if (!m_entityCameraMap.find(pEntity->GetEntityID(), spCamera))
        return;

    UpdateCamera(spCamera, pEntity);
}

//------------------------------------------------------------------------------------------------
CameraData* CameraService::GetActiveCamera(RenderSurface* pSurface)
{
    CameraDataPtr spCamera;

    egf::EntityID cameraId;
    if (m_activeCameraIds.find(pSurface->GetWindowRef(), cameraId))
        if (m_entityCameraMap.find(cameraId, spCamera))
            return spCamera;

    return NULL;
}

//------------------------------------------------------------------------------------------------
void CameraService::SetActiveCamera(egf::EntityID id, efd::WindowRef window)
{
    CameraDataPtr spCamera;

    if (m_entityCameraMap.find(id, spCamera))
    {
        m_activeCameraIds[window] = id;

        Entity* pEntity = m_pEntityManager->LookupEntity(id);
        EE_ASSERT(pEntity);

        UpdateCamera(spCamera, pEntity);
    }
}

//------------------------------------------------------------------------------------------------
void CameraService::UpdateCamera(CameraData* pCamera, egf::Entity* pEntity)
{
    // Ignore entities that do not contain a camera entity.
    if (!pEntity->GetModel()->ContainsModel(kFlatModelID_StandardModelLibrary_Camera))
        return;

    NiCamera* pControlledCamera = pCamera->GetCamera();

    efd::Point3 position;

    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Position, position) ==
        PropertyResult_OK)
    {
        pControlledCamera->SetTranslate(position.x, position.y, position.z);
    }
    efd::Point3 rotation;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Rotation,
        rotation) == PropertyResult_OK)
    {
        NiMatrix3 kXRot, kYRot, kZRot;

        kXRot.MakeXRotation(rotation.x * -EE_DEGREES_TO_RADIANS);
        kYRot.MakeYRotation(rotation.y * -EE_DEGREES_TO_RADIANS);
        kZRot.MakeZRotation(rotation.z * -EE_DEGREES_TO_RADIANS);
        pControlledCamera->SetRotate(kXRot * kYRot * kZRot);
    }

    float scale;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_Scale, scale)
        == PropertyResult_OK)
    {
        pControlledCamera->SetScale(scale);
    }

    float lodAdjust;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_LODAdjust,
        lodAdjust) == PropertyResult_OK)
    {
        pControlledCamera->SetLODAdjust(lodAdjust);
    }

    float minNearPlane;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MinimumNearPlane, minNearPlane)
        == PropertyResult_OK)
    {
        pControlledCamera->SetMinNearPlaneDist(minNearPlane);
    }

    float maxFarNearRatio;
    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_MaximumFarToNearRatio,
        maxFarNearRatio) == PropertyResult_OK)
    {
        pControlledCamera->SetMaxFarNearRatio(maxFarNearRatio);
    }

    // The frustum settings depend on the particular render surface, so we'll defer
    // setting the frustum until the loop below where we update the surface cameras
    // directly.
    bool orthographic = false;
    float fov = 0.0f;
    float nearPlane = 0.0f;
    float farPlane = 0.0f;
    bool setFrustum = pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_FOV, fov) ==
        PropertyResult_OK &&
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_IsOrthographic, orthographic) ==
        PropertyResult_OK &&
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_NearPlane, nearPlane) ==
        PropertyResult_OK &&
        pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_FarPlane, farPlane) ==
        PropertyResult_OK;

    WindowEntityMap::iterator itor = m_activeCameraIds.begin();
    for (; itor != m_activeCameraIds.end(); itor++)
    {
        if (itor->second != pEntity->GetEntityID())
            continue;

        WindowRef window = itor->first;
        RenderSurface* pSurface = m_spRenderService->GetRenderSurface(window);
        NiCamera* pMainCamera = pSurface->GetCamera();

        pMainCamera->SetTranslate(pControlledCamera->GetTranslate());
        pMainCamera->SetRotate(pControlledCamera->GetRotate());
        pMainCamera->SetScale(pControlledCamera->GetScale());

        pMainCamera->SetLODAdjust(pControlledCamera->GetLODAdjust());
        pMainCamera->SetMinNearPlaneDist(pControlledCamera->GetMinNearPlaneDist());
        pMainCamera->SetMaxFarNearRatio(pControlledCamera->GetMaxFarNearRatio());

        if (!setFrustum)
            continue;

        NiRenderTargetGroup* pRenderTarget = pSurface->GetRenderTargetGroup();
        EE_ASSERT(pRenderTarget);
        if (orthographic)
        {
            float width = (float)pRenderTarget->GetWidth(0);
            float height = (float)pRenderTarget->GetHeight(0);
            float zoom = pCamera->GetZoomFactor();

            NiFrustum frustum(-width * zoom, width * zoom,
                height * zoom, -height * zoom,
                nearPlane, farPlane, orthographic);
            pMainCamera->SetViewFrustum(frustum);
        }
        else
        {
            EE_ASSERT(pRenderTarget);
            efd::Float32 aspectRatio = (efd::Float32)pRenderTarget->GetWidth(0) /
                (efd::Float32)pRenderTarget->GetHeight(0);

            efd::Float32 verticalFieldOfViewRad = EE_DEGREES_TO_RADIANS * fov;
            efd::Float32 viewPlaneHalfHeight = tanf(verticalFieldOfViewRad * 0.5f);
            efd::Float32 viewPlaneHalfWidth = viewPlaneHalfHeight * aspectRatio;

            NiFrustum frustum(-viewPlaneHalfWidth, viewPlaneHalfWidth,
                viewPlaneHalfHeight, -viewPlaneHalfHeight,
                nearPlane, farPlane, orthographic);
            pMainCamera->SetViewFrustum(frustum);
        }
    }
}

//------------------------------------------------------------------------------------------------
NiCamera* CameraService::CreateCamera(const egf::EntityID id, NiRenderTargetGroup* pRenderTarget)
{
    CameraDataPtr spCamera = EE_NEW CameraData();
    spCamera->SetCamera(CreateDefaultCamera(pRenderTarget));
    spCamera->SetId(id);

    m_entityCameraMap[id] = spCamera;

    return spCamera->GetCamera();
}

//------------------------------------------------------------------------------------------------
void CameraService::CreateCamera(
    NiCamera* pExistingCamera,
    const egf::EntityID id)
{
    CameraDataPtr spCamera = EE_NEW CameraData();
    spCamera->SetCamera(pExistingCamera);
    spCamera->SetId(id);

    m_entityCameraMap[id] = spCamera;
}

//------------------------------------------------------------------------------------------------
void CameraService::SetCamera(NiWindowRef windowHandle, const egf::EntityID& id)
{
    CameraData* pCamera = m_entityCameraMap[id];

    if (pCamera != NULL)
    {
        RenderSurface* pSurface = m_spRenderService->GetRenderSurface(windowHandle);

        pSurface->SetCamera(pCamera->GetCamera());
    }
}

//------------------------------------------------------------------------------------------------
NiCamera* CameraService::CreateDefaultCamera(NiRenderTargetGroup* pRenderTarget)
{
    EE_ASSERT(pRenderTarget);
    efd::Float32 aspectRatio = (efd::Float32)pRenderTarget->GetWidth(0) /
        (efd::Float32)pRenderTarget->GetHeight(0);

    // Setup the camera frustum and viewport
    efd::Float32 verticalFieldOfViewDegrees = 60.0f;
    efd::Float32 verticalFieldOfViewRad = verticalFieldOfViewDegrees * EE_DEGREES_TO_RADIANS;
    efd::Float32 viewPlaneHalfHeight = tanf(verticalFieldOfViewRad * 0.5f);
    efd::Float32 viewPlaneHalfWidth = viewPlaneHalfHeight * aspectRatio;

    NiFrustum viewFrustum = NiFrustum(
        -viewPlaneHalfWidth, viewPlaneHalfWidth,
        viewPlaneHalfHeight, -viewPlaneHalfHeight,
        1.0f, 10000.0f);

    NiRect<efd::Float32> kPort(0.0f, 1.0f, 1.0f, 0.0f);
    NiCamera* pCamera = NiNew NiCamera;

    pCamera->SetViewFrustum(viewFrustum);
    pCamera->SetViewPort(kPort);

    pCamera->LookAtWorldPoint(NiPoint3(0.0f, 10.0f, 0.0f),
        NiPoint3(0.0f, 0.0f, 1.0f));

    pCamera->SetTranslate(0.0f, 0.0f, 96.0f);

    pCamera->Update(0.0f);

    return pCamera;
}

//------------------------------------------------------------------------------------------------
void CameraService::OnSurfaceRemoved(RenderService* pService, RenderSurface* pSurface)
{
    EE_UNUSED_ARG(pService);
    m_activeCameraIds.erase(pSurface->GetWindowRef());
}

//------------------------------------------------------------------------------------------------
NiPoint3 CameraService::Pan(
    const float dX,
    const float dY,
    const NiPoint3& inputPoint,
    const NiMatrix3& inputRotation)
{
    NiPoint3 newTranslation = NiPoint3(0.0f, dY, -dX);
    newTranslation = inputRotation * newTranslation;
    return (inputPoint + newTranslation);
}

//------------------------------------------------------------------------------------------------
NiMatrix3 CameraService::Look(
    const float dX,
    const float dY,
    const NiMatrix3& inputRotation,
    const NiPoint3& up)
{
    NiPoint3 look;
    inputRotation.GetCol(0, look);

    // prevent from looking straight up/down; causes rapid orientation changes
    float deltaY = dY;
    if (((up.Dot(look) > ms_kParallelThreshold) && (dY < 0.0f)) ||
        ((up.Dot(look) < -ms_kParallelThreshold) && (dY > 0.0f)))
    {
        deltaY = 0.0f;
    }

    NiPoint3 lookOffset = NiPoint3(0.0f, -deltaY, dX);
    lookOffset = inputRotation * lookOffset;
    look += lookOffset;
    look.Unitize();
    NiPoint3 lookTangent = look.Cross(up);
    lookTangent.Unitize();
    NiPoint3 lookBiTangent = lookTangent.Cross(look);

    return NiMatrix3(look, lookBiTangent, lookTangent);
}

//------------------------------------------------------------------------------------------------
void CameraService::Orbit(
    const float dX,
    const float dY,
    const NiPoint3& inputPoint,
    const NiMatrix3& inputRotation,
    const NiPoint3& center,
    const NiPoint3& up,
    NiPoint3& returnPoint,
    NiMatrix3& returnRotation)
{
    // figure out how far our look direction is from the orbit center
    NiPoint3 currentLook;
    NiPoint3 currentUp;
    inputRotation.GetCol(0, currentLook);
    inputRotation.GetCol(1, currentUp);
    NiPoint3 centerOffset = ((center - inputPoint).Dot(currentLook) *
        currentLook) - (center - inputPoint);

    float upDotLook = up.Dot(currentLook);

    // prevent from looking straight up/down
    float deltaY = dY;

    if (((upDotLook > ms_kParallelThreshold) && (dY < 0.0f)) ||
        ((upDotLook < -ms_kParallelThreshold) && (dY > 0.0f)))
    {
        deltaY = 0.0f;
    }

    float distance = (inputPoint - center - centerOffset).Length();
    NiPoint3 translationOffset = NiPoint3(distance, 0.0f, 0.0f);
    centerOffset = inputRotation.Transpose() * centerOffset;

    NiMatrix3 yRotation;
    yRotation.MakeZRotation(deltaY);
    translationOffset = inputRotation * (yRotation * translationOffset);
    centerOffset = inputRotation * (yRotation * centerOffset);

    NiMatrix3 xRotation;
    xRotation.MakeZRotation(dX);
    translationOffset = xRotation * translationOffset;
    centerOffset = xRotation * centerOffset;

    returnPoint = (center - translationOffset + centerOffset);

    NiPoint3 look = translationOffset;
    look.Unitize();

    NiPoint3 lookTangent = look.Cross(up);
    lookTangent.Unitize();

    // If LOOK x UP produces the zero vector then we need to resolve the edge case that the new
    // look direction is parallel to the up vector.  This can happen if the starting rotation is
    // looking straight up or down the up axis.
    if (lookTangent == NiPoint3::ZERO)
    {
        lookTangent = look.Cross(currentUp);
        lookTangent.Unitize();
    }

    NiPoint3 lookBiTangent = lookTangent.Cross(look);
    returnRotation.SetCol(0, look);
    returnRotation.SetCol(1, lookBiTangent);
    returnRotation.SetCol(2, lookTangent);
}

//------------------------------------------------------------------------------------------------
NiPoint3 CameraService::Dolly(
    const float dZ,
    const NiPoint3& inputPoint,
    const NiMatrix3& inputRotation)
{
    NiPoint3 direction;
    inputRotation.GetCol(0, direction);
    return (inputPoint + direction * dZ);
}

//------------------------------------------------------------------------------------------------
NiFrustum CameraService::OrthoZoom(const float dZ, const NiFrustum& inputFrustum)
{
    NiFrustum returnFrustum = inputFrustum;
    float fScaleFactor = pow(ms_kOrthoZoomSpeed, dZ);
    returnFrustum.m_fLeft *= fScaleFactor;
    returnFrustum.m_fRight *= fScaleFactor;
    returnFrustum.m_fTop *= fScaleFactor;
    returnFrustum.m_fBottom *= fScaleFactor;
    return returnFrustum;
}

//------------------------------------------------------------------------------------------------
NiMatrix3 CameraService::LookAt(
    const NiPoint3& focus,
    const NiPoint3& source,
    const NiPoint3& up)
{
    NiPoint3 look = focus - source;
    look.Unitize();
    NiPoint3 lookTangent = look.Cross(up);
    lookTangent.Unitize();
    NiPoint3 lookBiTangent = lookTangent.Cross(look);

    return NiMatrix3(look, lookBiTangent, lookTangent);
}

//------------------------------------------------------------------------------------------------
NiPoint3 CameraService::PanTo(
    const NiBound& focus,
    const NiMatrix3& currentRotation,
    const NiFrustum& frustum)
{
    NiPoint3 look;
    currentRotation.GetCol(0, look);
    float frustumEdge = (frustum.m_fRight > frustum.m_fTop) ?
        frustum.m_fTop : frustum.m_fRight;
    float distanceToCenter = 2.0f * focus.GetRadius() / frustumEdge;

    return (focus.GetCenter() - distanceToCenter * look);
}

//------------------------------------------------------------------------------------------------
void CameraService::MouseToRay(
    const float x,
    const float y,
    const unsigned int appWidth,
    const unsigned int appHeight,
    const NiCamera* pCamera,
    NiPoint3& origin,
    NiPoint3& direction)
{
    float unitizedX = (x / appWidth) * 2.0f - 1.0f;
    float unitizedY = ((appHeight - y) / appHeight) * 2.0f - 1.0f;
    unitizedX *= pCamera->GetViewFrustum().m_fRight;
    unitizedY *= pCamera->GetViewFrustum().m_fTop;

    NiMatrix3 rotation = pCamera->GetRotate();
    NiPoint3 look, lookUp, lookRight;
    rotation.GetCol(0, look);
    rotation.GetCol(1, lookUp);
    rotation.GetCol(2, lookRight);

    if (pCamera->GetViewFrustum().m_bOrtho)
    {
        origin = pCamera->GetWorldTranslate() + lookRight * unitizedX +
            lookUp * unitizedY;
        direction = look;
    }
    else
    {
        origin = pCamera->GetWorldTranslate();
        direction = look + lookUp * unitizedY + lookRight * unitizedX;
        direction.Unitize();
    }
}

//------------------------------------------------------------------------------------------------
NiPoint3 CameraService::TranslateOnAxis(
    const NiPoint3& startingPoint,
    const NiPoint3& axis,
    const NiPoint3& inputOrigin,
    const NiPoint3& inputDirection)
{
    // figure out the closest point between two 3d lines
    // one line is (entitylocation + m_eAxis * x)
    // the other is (origin + dir * y)
    // the closest pt on line 1 should be the new location of the entity
    NiPoint3 p1, p2;  // line origins
    NiPoint3 d1, d2;  // line directions
    NiPoint3 delta;    // line between origins

    p1 = startingPoint;
    p2 = inputOrigin;
    d1 = axis;
    d2 = inputDirection;
    delta = p1 - p2;

    float denominator = d1.SqrLength() * d2.SqrLength() -
        d2.Dot(d1) * d2.Dot(d1);
    float numerator = delta.Dot(d2) * d2.Dot(d1) - delta.Dot(d1) *
        d2.Dot(d2);
    // fSolution is the number of D1 to get to the closest point from P1
    float solution = numerator / denominator;

    return (d1 * solution);
}

//------------------------------------------------------------------------------------------------
NiPoint3 CameraService::TranslateOnPlane(
    const NiPoint3& startingPoint,
    const NiPoint3& normal,
    const NiPoint3& inputOrigin,
    const NiPoint3& inputDirection)
{
    // project the ray on to the plain, use the resulting point
    // find the component of delta in the direction of the plane
    float distance = (startingPoint - inputOrigin).Dot(normal);
    // component of the direction vector with respect to normal
    float compDirN = inputDirection.Dot(normal);
    NiPoint3 projectedPt = inputOrigin + inputDirection *
        (distance / compDirN);
    // find the difference between starting location and projected pt
    return (projectedPt - startingPoint);
}

//------------------------------------------------------------------------------------------------
float CameraService::RotateAboutAxis(
    const NiPoint3& startingPoint,
    const NiPoint3& axis,
    const NiPoint3& tangent,
    const NiPoint3& biTangent,
    const NiPoint3& inputOrigin,
    const NiPoint3& inputDirection)
{
    // return the number of radians from the tangent in the direction of
    // the bi-tangent

    // determine if the input is above the horizon by making sure input
    // origin and direction are in different directions relative to the axis
    float aDotO = axis.Dot(inputOrigin);
    float aDotD = axis.Dot(inputDirection);
    float multiplier = (aDotO * aDotD >= 0.0f) ? -1.0f : 1.0f;
    // project input onto a plane
    // make a vector from the projected pt - plane origin
    NiPoint3 offSet = TranslateOnPlane(startingPoint, axis, inputOrigin,
        inputDirection);
    // if input is looking away from the horizon, invert offset
    offSet *= multiplier;
    // unitize this vector
    offSet.Unitize();
    // use its components to figure the radians
    float tangentComponent = offSet.Dot(tangent);
    float biTangentComponent = offSet.Dot(biTangent);
    // fReturn value is the radian rotation about the given axis
    // that the input is different from the tangent vector
    float returnValue = NiACos(tangentComponent);
    if (biTangentComponent <= 0.0f)
    {
        returnValue = efd::EE_TWO_PI - returnValue;
    }
    return returnValue;
}

//------------------------------------------------------------------------------------------------
