#pragma once
#ifndef NIF_NATIVE_COMMON_H
#define NIF_NATIVE_COMMON_H

#include "NIFToolsetNativeLibType.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum NIF_Result
{
	NIF_RESULT_OK = 0,
	NIF_RESULT_INVALID_HANDLE = 1,
	NIF_RESULT_INVALID_ARGUMENT = 2,
	NIF_RESULT_OUT_OF_RANGE = 3,
	NIF_RESULT_INVALID_TYPE = 4,
	NIF_RESULT_ENGINE_ERROR = 5,
	NIF_RESULT_OUT_OF_MEMORY = 6,
	NIF_RESULT_EXCEPTION = 7,
	NIF_RESULT_NOT_SUPPORTED = 8
} NIF_Result;

typedef enum NIF_NodeType
{
	NIF_NODE_TYPE_UNKNOWN = 0,
	NIF_NODE_TYPE_NODE = 1,
	NIF_NODE_TYPE_BSP_NODE = 2,
	NIF_NODE_TYPE_BILLBOARD_NODE = 3,
	NIF_NODE_TYPE_SWITCH_NODE = 4,
	NIF_NODE_TYPE_LOD_NODE = 5,
	NIF_NODE_TYPE_SORT_ADJUST_NODE = 6,
	NIF_NODE_TYPE_ROOM = 7,
	NIF_NODE_TYPE_ROOM_GROUP = 8,
	NIF_NODE_TYPE_TERRAIN = 9,
	NIF_NODE_TYPE_TERRAIN_CELL = 10,
	NIF_NODE_TYPE_TERRAIN_CELL_NODE = 11,
	NIF_NODE_TYPE_TERRAIN_CELL_LEAF = 12,
	NIF_NODE_TYPE_TERRAIN_SECTOR = 13,
	NIF_NODE_TYPE_ATMOSPHERE = 14,
	NIF_NODE_TYPE_ENVIRONMENT = 15,
	NIF_NODE_TYPE_SKY = 16,
	NIF_NODE_TYPE_SKY_DOME = 17,
	NIF_NODE_TYPE_DECORATION_FIELD = 18,
	NIF_NODE_TYPE_DECORATION_LAYER = 19,
	NIF_NODE_TYPE_DECORATION_PLANE = 20,
	NIF_NODE_TYPE_OLD_WALL = 21,
} NIF_NodeType;

typedef struct NIF_Vec2
{
	float x;
	float y;
} NIF_Vec2;

typedef struct NIF_Vec3
{
	float x;
	float y;
	float z;
} NIF_Vec3;

typedef struct NIF_Vec4
{
	float x;
	float y;
	float z;
	float w;
} NIF_Vec4;

typedef struct NIF_Mat3
{
	float m[3][3];
} NIF_Mat3;

typedef struct NIF_Transform
{
	NIF_Mat3 rotate;
	NIF_Vec3 translate;
	float scale;
} NIF_Transform;

typedef struct NIF_Rect
{
	float left;
	float right;
	float top;
	float bottom;
} NIF_Rect;

typedef struct NIF_Frustum
{
	float left;
	float right;
	float top;
	float bottom;
	float nearPlane;
	float farPlane;
	int isOrtho;
} NIF_Frustum;

typedef struct NIF_Bound
{
	NIF_Vec3 center;
	float radius;
} NIF_Bound;

typedef struct NIF_Color
{
	float r;
	float g;
	float b;
} NIF_Color;

typedef struct NIF_ColorA
{
	float r;
	float g;
	float b;
	float a;
} NIF_ColorA;

typedef struct NIF_StreamHandle_t* NIF_StreamHandle;
typedef struct NIF_ObjectHandle_t* NIF_ObjectHandle;
typedef struct NIF_AVObjectHandle_t* NIF_AVObjectHandle;
typedef struct NIF_MeshHandle_t* NIF_MeshHandle;
typedef struct NIF_NodeHandle_t* NIF_NodeHandle;
typedef struct NIF_BSPNodeHandle_t* NIF_BSPNodeHandle;
typedef struct NIF_BillboardNodeHandle_t* NIF_BillboardNodeHandle;
typedef struct NIF_SwitchNodeHandle_t* NIF_SwitchNodeHandle;
typedef struct NIF_LODNodeHandle_t* NIF_LODNodeHandle;
typedef struct NIF_SortAdjustNodeHandle_t* NIF_SortAdjustNodeHandle;
typedef struct NIF_TerrainHandle_t* NIF_TerrainHandle;
typedef struct NIF_TerrainCellHandle_t* NIF_TerrainCellHandle;
typedef struct NIF_TerrainCellNodeHandle_t* NIF_TerrainCellNodeHandle;
typedef struct NIF_TerrainCellLeafHandle_t* NIF_TerrainCellLeafHandle;
typedef struct NIF_TerrainSectorHandle_t* NIF_TerrainSectorHandle;
typedef struct NIF_AtmosphereHandle_t* NIF_AtmosphereHandle;
typedef struct NIF_EnvironmentHandle_t* NIF_EnvironmentHandle;
typedef struct NIF_SkyHandle_t* NIF_SkyHandle;
typedef struct NIF_SkyDomeHandle_t* NIF_SkyDomeHandle;
typedef struct NIF_DecorationFieldHandle_t* NIF_DecorationFieldHandle;
typedef struct NIF_DecorationLayerHandle_t* NIF_DecorationLayerHandle;
typedef struct NIF_DecorationPlaneHandle_t* NIF_DecorationPlaneHandle;
typedef struct NIF_CameraHandle_t* NIF_CameraHandle;
typedef struct NIF_DataStreamHandle_t* NIF_DataStreamHandle;
typedef struct NIF_DataStreamRefHandle_t* NIF_DataStreamRefHandle;
typedef struct NIF_ControllerSequenceHandle_t* NIF_ControllerSequenceHandle;
typedef struct NIF_SequenceDataHandle_t* NIF_SequenceDataHandle;
typedef struct NIF_TextKeyExtraDataHandle_t* NIF_TextKeyExtraDataHandle;
typedef struct NIF_KFMToolHandle_t* NIF_KFMToolHandle;
typedef struct NIF_ActorManagerHandle_t* NIF_ActorManagerHandle;
typedef struct NIF_CollisionDataHandle_t* NIF_CollisionDataHandle;
typedef struct NIF_CollisionGroupHandle_t* NIF_CollisionGroupHandle;
typedef struct NIF_PortalHandle_t* NIF_PortalHandle;
typedef struct NIF_RoomHandle_t* NIF_RoomHandle;
typedef struct NIF_OldWallHandle_t* NIF_OldWallHandle;
typedef struct NIF_RoomGroupHandle_t* NIF_RoomGroupHandle;
typedef struct NIF_RendererHandle_t* NIF_RendererHandle;
typedef struct NIF_RenderTargetGroupHandle_t* NIF_RenderTargetGroupHandle;
typedef struct NIF_RenderBufferHandle_t* NIF_RenderBufferHandle;
typedef struct NIF_DepthStencilBufferHandle_t* NIF_DepthStencilBufferHandle;
typedef struct NIF_CullingProcessHandle_t* NIF_CullingProcessHandle;
typedef struct NIF_MeshCullingProcessHandle_t* NIF_MeshCullingProcessHandle;
typedef struct NIF_AlphaAccumulatorHandle_t* NIF_AlphaAccumulatorHandle;
typedef struct NIF_RenderListProcessorHandle_t* NIF_RenderListProcessorHandle;
typedef struct NIF_AlphaSortProcessorHandle_t* NIF_AlphaSortProcessorHandle;
typedef struct NIF_RenderViewHandle_t* NIF_RenderViewHandle;
typedef struct NIF_RenderView3DHandle_t* NIF_RenderView3DHandle;
typedef struct NIF_RenderClickHandle_t* NIF_RenderClickHandle;
typedef struct NIF_ViewRenderClickHandle_t* NIF_ViewRenderClickHandle;
typedef struct NIF_RenderStepHandle_t* NIF_RenderStepHandle;
typedef struct NIF_DefaultClickRenderStepHandle_t* NIF_DefaultClickRenderStepHandle;
typedef struct NIF_ParticleSystemHandle_t* NIF_ParticleSystemHandle;
typedef struct NIF_PSEmitterHandle_t* NIF_PSEmitterHandle;

// Callbacks use cdecl so they can be declared explicitly in C# with
// UnmanagedFunctionPointer(CallingConvention.Cdecl).
typedef int (NIFTOOLSET_NATIVE_CALL *NIF_RenderStepCallback)(NIF_RenderStepHandle renderStep, void* userData);

NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionMajor(void);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionMinor(void);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionPatch(void);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_GetVersionString(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_IsRuntimeAvailable(void);

// Thread-local error state. Any returned message is UTF-8 and remains valid
// until the next binding call on the same thread that changes the error.
NIFTOOLSET_NATIVE_ENTRY NIF_Result NIF_GetLastErrorCode(void);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_GetLastErrorMessage(void);
NIFTOOLSET_NATIVE_ENTRY size_t NIF_GetLastErrorMessageLength(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_CopyLastErrorMessage(char* destination, size_t destinationSize);
NIFTOOLSET_NATIVE_ENTRY void NIF_ClearLastError(void);

// Copies a UTF-8 null-terminated string. Returns the required buffer size,
// including the null terminator. A null destination can be used to query size.
NIFTOOLSET_NATIVE_ENTRY size_t NIF_CopyString(const char* source, char* destination, size_t destinationSize);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_COMMON_H
