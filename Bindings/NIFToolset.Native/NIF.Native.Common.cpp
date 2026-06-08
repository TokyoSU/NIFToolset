#include "NIF.Native.Common.h"
#include "NIF.Native.Internal.h"

#include <NiFixedString.h>

namespace
{
	constexpr unsigned int kVersionMajor = 0;
	constexpr unsigned int kVersionMinor = 1;
	constexpr unsigned int kVersionPatch = 0;
	constexpr char kVersionString[] = "0.1.0";
}

NIF_Vec3 NIF_MakeVec3(const NiPoint3& value)
{
	return { value.x, value.y, value.z };
}

NiPoint3 NIF_MakePoint3(const NIF_Vec3& value)
{
	return NiPoint3(value.x, value.y, value.z);
}

NIF_Mat3 NIF_MakeMat3(const NiMatrix3& value)
{
	NIF_Mat3 result = {};
	NiPoint3 row;
	value.GetRow(0, row);
	result.m[0][0] = row.x;
	result.m[0][1] = row.y;
	result.m[0][2] = row.z;
	value.GetRow(1, row);
	result.m[1][0] = row.x;
	result.m[1][1] = row.y;
	result.m[1][2] = row.z;
	value.GetRow(2, row);
	result.m[2][0] = row.x;
	result.m[2][1] = row.y;
	result.m[2][2] = row.z;
	return result;
}

NiMatrix3 NIF_MakeMatrix3(const NIF_Mat3& value)
{
	NiMatrix3 result;
	result.SetRow(0, NiPoint3(value.m[0][0], value.m[0][1], value.m[0][2]));
	result.SetRow(1, NiPoint3(value.m[1][0], value.m[1][1], value.m[1][2]));
	result.SetRow(2, NiPoint3(value.m[2][0], value.m[2][1], value.m[2][2]));
	return result;
}

NIF_Transform NIF_MakeTransform(const NiTransform& value)
{
	return { NIF_MakeMat3(value.m_Rotate), NIF_MakeVec3(value.m_Translate), value.m_fScale };
}

NIF_Rect NIF_MakeRect(const NiRect<float>& value)
{
	return { value.m_left, value.m_right, value.m_top, value.m_bottom };
}

NiRect<float> NIF_MakeRect(const NIF_Rect& value)
{
	return NiRect<float>(value.left, value.right, value.top, value.bottom);
}

NIF_Frustum NIF_MakeFrustum(const NiFrustum& value)
{
	return {
		value.m_fLeft,
		value.m_fRight,
		value.m_fTop,
		value.m_fBottom,
		value.m_fNear,
		value.m_fFar,
		value.m_bOrtho ? 1 : 0
	};
}

NiFrustum NIF_MakeFrustum(const NIF_Frustum& value)
{
	return NiFrustum(
		value.left,
		value.right,
		value.top,
		value.bottom,
		value.nearPlane,
		value.farPlane,
		value.isOrtho != 0);
}

NIF_Color NIF_MakeColor(const NiColor& value)
{
	return { value.r, value.g, value.b };
}

NiColor NIF_MakeColor(const NIF_Color& value)
{
	return NiColor(value.r, value.g, value.b);
}

NIF_ColorA NIF_MakeColorA(const NiColorA& value)
{
	return { value.r, value.g, value.b, value.a };
}

NiColorA NIF_MakeColorA(const NIF_ColorA& value)
{
	return NiColorA(value.r, value.g, value.b, value.a);
}

const char* NIF_GetObjectName(const NiObject* pkObject)
{
	NiObjectNET* pkObjectNet = NiDynamicCast(NiObjectNET, const_cast<NiObject*>(pkObject));
	return pkObjectNet ? pkObjectNet->GetName() : nullptr;
}

void NIF_SetObjectName(NiObject* pkObject, const char* name)
{
	NiObjectNET* pkObjectNet = NiDynamicCast(NiObjectNET, pkObject);
	if (pkObjectNet)
	{
		pkObjectNet->SetName(NiFixedString(name));
	}
}

NIF_ObjectHandle NIF_CreateObjectHandle(NiObject* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_ObjectHandle_t* handle = new NIF_ObjectHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_ObjectHandle>(handle);
}

NIF_AVObjectHandle NIF_CreateAVObjectHandle(NiAVObject* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_AVObjectHandle_t* handle = new NIF_AVObjectHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_AVObjectHandle>(handle);
}

NIF_NodeHandle NIF_CreateNodeHandle(NiNode* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_NodeHandle_t* handle = new NIF_NodeHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_NodeHandle>(handle);
}

NIF_CameraHandle NIF_CreateCameraHandle(NiCamera* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_CameraHandle_t* handle = new NIF_CameraHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_CameraHandle>(handle);
}

NIF_DataStreamHandle NIF_CreateDataStreamHandle(NiDataStream* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DataStreamHandle_t* handle = new NIF_DataStreamHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_DataStreamHandle>(handle);
}

NIF_DataStreamRefHandle NIF_CreateDataStreamRefHandle(NiDataStreamRef* pkObject, NiObject* pkOwner)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DataStreamRefHandle_t* handle = new NIF_DataStreamRefHandle_t();
	handle->pObject = pkObject;
	handle->spOwner = pkOwner;
	return static_cast<NIF_DataStreamRefHandle>(handle);
}

NIF_ControllerSequenceHandle NIF_CreateControllerSequenceHandle(NiControllerSequence* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_ControllerSequenceHandle_t* handle = new NIF_ControllerSequenceHandle_t();
	handle->spObject = static_cast<NiObject*>(pkObject);
	return static_cast<NIF_ControllerSequenceHandle>(handle);
}

NIF_TextKeyExtraDataHandle NIF_CreateTextKeyExtraDataHandle(NiTextKeyExtraData* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_TextKeyExtraDataHandle_t* handle = new NIF_TextKeyExtraDataHandle_t();
	handle->spObject = static_cast<NiObject*>(pkObject);
	return static_cast<NIF_TextKeyExtraDataHandle>(handle);
}

NIF_RendererHandle NIF_CreateRendererHandle(NiRenderer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RendererHandle_t* handle = new NIF_RendererHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_RendererHandle>(handle);
}

NIF_RenderTargetGroupHandle NIF_CreateRenderTargetGroupHandle(NiRenderTargetGroup* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderTargetGroupHandle_t* handle = new NIF_RenderTargetGroupHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_RenderTargetGroupHandle>(handle);
}

NIF_RenderBufferHandle NIF_CreateRenderBufferHandle(Ni2DBuffer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderBufferHandle_t* handle = new NIF_RenderBufferHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_RenderBufferHandle>(handle);
}

NIF_DepthStencilBufferHandle NIF_CreateDepthStencilBufferHandle(NiDepthStencilBuffer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DepthStencilBufferHandle_t* handle = new NIF_DepthStencilBufferHandle_t();
	handle->spObject = pkObject;
	return static_cast<NIF_DepthStencilBufferHandle>(handle);
}

NIF_Mat3 NIF_MakeIdentityMat3(void)
{
	NIF_Mat3 identity = {};
	identity.m[0][0] = 1.0f;
	identity.m[1][1] = 1.0f;
	identity.m[2][2] = 1.0f;
	return identity;
}

extern "C"
{

unsigned int NIF_GetVersionMajor(void)
{
	return kVersionMajor;
}

unsigned int NIF_GetVersionMinor(void)
{
	return kVersionMinor;
}

unsigned int NIF_GetVersionPatch(void)
{
	return kVersionPatch;
}

const char* NIF_GetVersionString(void)
{
	return kVersionString;
}

int NIF_IsRuntimeAvailable(void)
{
	return 1;
}

}
