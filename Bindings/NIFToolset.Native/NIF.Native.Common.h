#pragma once
#ifndef NIF_NATIVE_COMMON_H
#define NIF_NATIVE_COMMON_H

#include "NIFToolsetNativeLibType.h"

#ifdef __cplusplus
extern "C"
{
#endif

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

typedef void* NIF_StreamHandle;
typedef void* NIF_ObjectHandle;
typedef void* NIF_AVObjectHandle;
typedef void* NIF_MeshHandle;
typedef void* NIF_NodeHandle;
typedef void* NIF_CameraHandle;
typedef void* NIF_DataStreamHandle;
typedef void* NIF_DataStreamRefHandle;
typedef void* NIF_ControllerSequenceHandle;
typedef void* NIF_SequenceDataHandle;
typedef void* NIF_TextKeyExtraDataHandle;
typedef void* NIF_KFMToolHandle;
typedef void* NIF_ActorManagerHandle;
typedef void* NIF_CollisionDataHandle;
typedef void* NIF_CollisionGroupHandle;
typedef void* NIF_PortalHandle;
typedef void* NIF_RoomHandle;
typedef void* NIF_RoomGroupHandle;
typedef void* NIF_RendererHandle;
typedef void* NIF_RenderTargetGroupHandle;
typedef void* NIF_RenderBufferHandle;
typedef void* NIF_DepthStencilBufferHandle;
typedef void* NIF_CullingProcessHandle;
typedef void* NIF_MeshCullingProcessHandle;
typedef void* NIF_AlphaAccumulatorHandle;
typedef void* NIF_RenderListProcessorHandle;
typedef void* NIF_AlphaSortProcessorHandle;
typedef void* NIF_RenderViewHandle;
typedef void* NIF_RenderView3DHandle;
typedef void* NIF_RenderClickHandle;
typedef void* NIF_ViewRenderClickHandle;
typedef void* NIF_RenderStepHandle;
typedef void* NIF_DefaultClickRenderStepHandle;
typedef void* NIF_ParticleSystemHandle;
typedef void* NIF_PSEmitterHandle;

NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionMajor(void);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionMinor(void);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_GetVersionPatch(void);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_GetVersionString(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_IsRuntimeAvailable(void);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_COMMON_H
