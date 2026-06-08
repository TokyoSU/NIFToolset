#pragma once
#ifndef NIF_NATIVE_INTERNAL_H
#define NIF_NATIVE_INTERNAL_H

#include "NIF.Native.Common.h"

#include <NiAVObject.h>
#include <NiCamera.h>
#include <NiCollisionData.h>
#include <NiColor.h>
#include <NiCollisionGroup.h>
#include <NiControllerSequence.h>
#include <NiCullingProcess.h>
#include <NiAlphaAccumulator.h>
#include <NiAlphaSortProcessor.h>
#include <NiActorManager.h>
#include <NiDataStream.h>
#include <NiDataStreamRef.h>
#include <Ni2DBuffer.h>
#include <NiDefaultClickRenderStep.h>
#include <NiDepthStencilBuffer.h>
#include <NiFrustum.h>
#include <NiKFMTool.h>
#include <NiMatrix3.h>
#include <NiNode.h>
#include <NiObjectNET.h>
#include <NiPoint3.h>
#include <NiPortal.h>
#include <NiRect.h>
#include <NiRenderer.h>
#include <NiRenderStep.h>
#include <NiRenderTargetGroup.h>
#include <NiRenderView.h>
#include <NiRenderClick.h>
#include <NiRoom.h>
#include <NiRoomGroup.h>
#include <NiSequenceData.h>
#include <NiStream.h>
#include <NiTextKeyExtraData.h>
#include <NiTransform.h>
#include <NiViewRenderClick.h>
#include <Ni3DRenderView.h>
#include <NiMeshCullingProcess.h>

class NiPSParticleSystem;
class NiPSEmitter;

struct NIF_StreamHandle_t
{
	NiStream* pStream = nullptr;
};

struct NIF_ObjectHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_AVObjectHandle_t
{
	NiAVObjectPtr spObject;
};

struct NIF_NodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_CameraHandle_t
{
	NiCameraPtr spObject;
};

struct NIF_DataStreamHandle_t
{
	NiDataStreamPtr spObject;
};

struct NIF_DataStreamRefHandle_t
{
	NiDataStreamRef* pObject = nullptr;
	NiObjectPtr spOwner;
};

struct NIF_ControllerSequenceHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_SequenceDataHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_TextKeyExtraDataHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_KFMToolHandle_t
{
	NiKFMToolPtr spObject;
};

struct NIF_ActorManagerHandle_t
{
	NiActorManagerPtr spObject;
};

struct NIF_CollisionDataHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_CollisionGroupHandle_t
{
	NiCollisionGroup* pObject = nullptr;
};

struct NIF_PortalHandle_t
{
	NiPortalPtr spObject;
};

struct NIF_RoomHandle_t
{
	NiRoomPtr spObject;
};

struct NIF_RoomGroupHandle_t
{
	NiRoomGroupPtr spObject;
};

struct NIF_RendererHandle_t
{
	NiRendererPtr spObject;
};

struct NIF_RenderTargetGroupHandle_t
{
	NiRenderTargetGroupPtr spObject;
};

struct NIF_RenderBufferHandle_t
{
	Ni2DBufferPtr spObject;
};

struct NIF_DepthStencilBufferHandle_t
{
	NiDepthStencilBufferPtr spObject;
};

struct NIF_CullingProcessHandle_t
{
	NiCullingProcessPtr spObject;
};

struct NIF_MeshCullingProcessHandle_t
{
	NiMeshCullingProcessPtr spObject;
};

struct NIF_AlphaAccumulatorHandle_t
{
	NiAlphaAccumulatorPtr spObject;
};

struct NIF_RenderListProcessorHandle_t
{
	NiRenderListProcessorPtr spObject;
};

struct NIF_AlphaSortProcessorHandle_t
{
	NiAlphaSortProcessorPtr spObject;
};

struct NIF_RenderViewHandle_t
{
	NiRenderViewPtr spObject;
};

struct NIF_RenderView3DHandle_t
{
	Ni3DRenderViewPtr spObject;
};

struct NIF_RenderClickHandle_t
{
	NiRenderClickPtr spObject;
};

struct NIF_ViewRenderClickHandle_t
{
	NiViewRenderClickPtr spObject;
};

struct NIF_RenderStepHandle_t
{
	NiRenderStepPtr spObject;
	void* pPreCallback = nullptr;
	void* pPostCallback = nullptr;
	void* pPreCallbackUserData = nullptr;
	void* pPostCallbackUserData = nullptr;
};

struct NIF_DefaultClickRenderStepHandle_t
{
	NiDefaultClickRenderStepPtr spObject;
};

struct NIF_ParticleSystemHandle_t
{
	NiObjectPtr spObject;
};

struct NIF_PSEmitterHandle_t
{
	NiObjectPtr spObject;
};

NIF_Vec3 NIF_MakeVec3(const NiPoint3& value);
NiPoint3 NIF_MakePoint3(const NIF_Vec3& value);
NIF_Mat3 NIF_MakeMat3(const NiMatrix3& value);
NiMatrix3 NIF_MakeMatrix3(const NIF_Mat3& value);
NIF_Transform NIF_MakeTransform(const NiTransform& value);
NIF_Rect NIF_MakeRect(const NiRect<float>& value);
NiRect<float> NIF_MakeRect(const NIF_Rect& value);
NIF_Frustum NIF_MakeFrustum(const NiFrustum& value);
NiFrustum NIF_MakeFrustum(const NIF_Frustum& value);
NIF_Color NIF_MakeColor(const NiColor& value);
NiColor NIF_MakeColor(const NIF_Color& value);
NIF_ColorA NIF_MakeColorA(const NiColorA& value);
NiColorA NIF_MakeColorA(const NIF_ColorA& value);
const char* NIF_GetObjectName(const NiObject* pkObject);
void NIF_SetObjectName(NiObject* pkObject, const char* name);
NIF_ObjectHandle NIF_CreateObjectHandle(NiObject* pkObject);
NIF_AVObjectHandle NIF_CreateAVObjectHandle(NiAVObject* pkObject);
NIF_NodeHandle NIF_CreateNodeHandle(NiNode* pkObject);
NIF_CameraHandle NIF_CreateCameraHandle(NiCamera* pkObject);
NIF_DataStreamHandle NIF_CreateDataStreamHandle(NiDataStream* pkObject);
NIF_DataStreamRefHandle NIF_CreateDataStreamRefHandle(NiDataStreamRef* pkObject, NiObject* pkOwner);
NIF_ControllerSequenceHandle NIF_CreateControllerSequenceHandle(NiControllerSequence* pkObject);
NIF_SequenceDataHandle NIF_CreateSequenceDataHandle(NiSequenceData* pkObject);
NIF_TextKeyExtraDataHandle NIF_CreateTextKeyExtraDataHandle(NiTextKeyExtraData* pkObject);
NIF_KFMToolHandle NIF_CreateKFMToolHandle(NiKFMTool* pkObject);
NIF_ActorManagerHandle NIF_CreateActorManagerHandle(NiActorManager* pkObject);
NIF_CollisionDataHandle NIF_CreateCollisionDataHandle(NiCollisionData* pkObject);
NIF_CollisionGroupHandle NIF_CreateCollisionGroupHandle(NiCollisionGroup* pkObject);
NIF_PortalHandle NIF_CreatePortalHandle(NiPortal* pkObject);
NIF_RoomHandle NIF_CreateRoomHandle(NiRoom* pkObject);
NIF_RoomGroupHandle NIF_CreateRoomGroupHandle(NiRoomGroup* pkObject);
NIF_RendererHandle NIF_CreateRendererHandle(NiRenderer* pkObject);
NIF_RenderTargetGroupHandle NIF_CreateRenderTargetGroupHandle(NiRenderTargetGroup* pkObject);
NIF_RenderBufferHandle NIF_CreateRenderBufferHandle(Ni2DBuffer* pkObject);
NIF_DepthStencilBufferHandle NIF_CreateDepthStencilBufferHandle(NiDepthStencilBuffer* pkObject);
NIF_CullingProcessHandle NIF_CreateCullingProcessHandle(NiCullingProcess* pkObject);
NIF_MeshCullingProcessHandle NIF_CreateMeshCullingProcessHandle(NiMeshCullingProcess* pkObject);
NIF_AlphaAccumulatorHandle NIF_CreateAlphaAccumulatorHandle(NiAlphaAccumulator* pkObject);
NIF_RenderListProcessorHandle NIF_CreateRenderListProcessorHandle(NiRenderListProcessor* pkObject);
NIF_AlphaSortProcessorHandle NIF_CreateAlphaSortProcessorHandle(NiAlphaSortProcessor* pkObject);
NIF_RenderViewHandle NIF_CreateRenderViewHandle(NiRenderView* pkObject);
NIF_RenderView3DHandle NIF_CreateRenderView3DHandle(Ni3DRenderView* pkObject);
NIF_RenderClickHandle NIF_CreateRenderClickHandle(NiRenderClick* pkObject);
NIF_ViewRenderClickHandle NIF_CreateViewRenderClickHandle(NiViewRenderClick* pkObject);
NIF_RenderStepHandle NIF_CreateRenderStepHandle(NiRenderStep* pkObject);
NIF_DefaultClickRenderStepHandle NIF_CreateDefaultClickRenderStepHandle(NiDefaultClickRenderStep* pkObject);
// Keep phase 3 particle handle factory declarations in the shared internal header.
NIF_ParticleSystemHandle NIF_CreateParticleSystemHandle(NiPSParticleSystem* pkObject);
NIF_PSEmitterHandle NIF_CreatePSEmitterHandle(NiPSEmitter* pkObject);
NIF_Mat3 NIF_MakeIdentityMat3(void);

typedef int (*NIF_RenderStepCallback)(NIF_RenderStepHandle renderStep, void* userData);

#endif // NIF_NATIVE_INTERNAL_H
