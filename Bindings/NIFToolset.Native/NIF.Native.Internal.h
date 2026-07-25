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
#include <NiMesh.h>
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

#include <cstddef>
#include <string>

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

struct NIF_MeshHandle_t
{
	NiMeshPtr spObject;
};

struct NIF_NodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_BSPNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_BillboardNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_SwitchNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_LODNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_SortAdjustNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_TerrainHandle_t
{
	NiNodePtr spObject;
};

struct NIF_TerrainCellHandle_t
{
	NiNodePtr spObject;
};

struct NIF_TerrainCellNodeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_TerrainCellLeafHandle_t
{
	NiNodePtr spObject;
};

struct NIF_TerrainSectorHandle_t
{
	NiNodePtr spObject;
};

struct NIF_AtmosphereHandle_t
{
	NiNodePtr spObject;
};

struct NIF_EnvironmentHandle_t
{
	NiNodePtr spObject;
};

struct NIF_SkyHandle_t
{
	NiNodePtr spObject;
};

struct NIF_SkyDomeHandle_t
{
	NiNodePtr spObject;
};

struct NIF_DecorationFieldHandle_t
{
	NiNodePtr spObject;
};

struct NIF_DecorationLayerHandle_t
{
	NiNodePtr spObject;
};

struct NIF_DecorationPlaneHandle_t
{
	NiNodePtr spObject;
};

struct NIF_OldWallHandle_t
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
	NIF_RenderStepCallback pPreCallback = nullptr;
	NIF_RenderStepCallback pPostCallback = nullptr;
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
NIF_MeshHandle NIF_CreateMeshHandle(NiMesh* pkObject);
NIF_NodeHandle NIF_CreateNodeHandle(NiNode* pkObject);
NIF_BSPNodeHandle NIF_CreateBSPNodeHandle(NiNode* pkObject);
NIF_BillboardNodeHandle NIF_CreateBillboardNodeHandle(NiNode* pkObject);
NIF_SwitchNodeHandle NIF_CreateSwitchNodeHandle(NiNode* pkObject);
NIF_LODNodeHandle NIF_CreateLODNodeHandle(NiNode* pkObject);
NIF_SortAdjustNodeHandle NIF_CreateSortAdjustNodeHandle(NiNode* pkObject);
NIF_TerrainHandle NIF_CreateTerrainHandle(NiNode* pkObject);
NIF_TerrainCellHandle NIF_CreateTerrainCellHandle(NiNode* pkObject);
NIF_TerrainCellNodeHandle NIF_CreateTerrainCellNodeHandle(NiNode* pkObject);
NIF_TerrainCellLeafHandle NIF_CreateTerrainCellLeafHandle(NiNode* pkObject);
NIF_TerrainSectorHandle NIF_CreateTerrainSectorHandle(NiNode* pkObject);
NIF_AtmosphereHandle NIF_CreateAtmosphereHandle(NiNode* pkObject);
NIF_EnvironmentHandle NIF_CreateEnvironmentHandle(NiNode* pkObject);
NIF_SkyHandle NIF_CreateSkyHandle(NiNode* pkObject);
NIF_SkyDomeHandle NIF_CreateSkyDomeHandle(NiNode* pkObject);
NIF_DecorationFieldHandle NIF_CreateDecorationFieldHandle(NiNode* pkObject);
NIF_DecorationLayerHandle NIF_CreateDecorationLayerHandle(NiNode* pkObject);
NIF_DecorationPlaneHandle NIF_CreateDecorationPlaneHandle(NiNode* pkObject);
NIF_OldWallHandle NIF_CreateOldWallHandle(NiNode* pkObject);
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



void NIF_SetLastError(NIF_Result result, const char* message);
void NIF_SetLastError(NIF_Result result, const std::string& message);
void NIF_SetLastErrorFromCurrentException(const char* context);
size_t NIF_CopyStringInternal(const char* source, char* destination, size_t destinationSize);

#endif // NIF_NATIVE_INTERNAL_H
