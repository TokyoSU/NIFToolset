#include "NIF.Native.Common.h"
#include "NIF.Native.Internal.h"

#include <NiFixedString.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <new>
#include <string>

namespace
{
	constexpr unsigned int kVersionMajor = 0;
	constexpr unsigned int kVersionMinor = 3;
	constexpr unsigned int kVersionPatch = 0;
	constexpr char kVersionString[] = "0.3.0";

	thread_local NIF_Result g_lastErrorCode = NIF_RESULT_OK;
	thread_local std::string g_lastErrorMessage;
}

static_assert(sizeof(NIF_Vec2) == 8, "NIF_Vec2 ABI changed");
static_assert(sizeof(NIF_Vec3) == 12, "NIF_Vec3 ABI changed");
static_assert(sizeof(NIF_Vec4) == 16, "NIF_Vec4 ABI changed");
static_assert(sizeof(NIF_Mat3) == 36, "NIF_Mat3 ABI changed");
static_assert(sizeof(NIF_Transform) == 52, "NIF_Transform ABI changed");
static_assert(sizeof(NIF_Rect) == 16, "NIF_Rect ABI changed");
static_assert(sizeof(NIF_Frustum) == 28, "NIF_Frustum ABI changed");
static_assert(sizeof(NIF_Bound) == 16, "NIF_Bound ABI changed");
static_assert(sizeof(NIF_Color) == 12, "NIF_Color ABI changed");
static_assert(sizeof(NIF_ColorA) == 16, "NIF_ColorA ABI changed");

void NIF_SetLastError(NIF_Result result, const char* message)
{
	g_lastErrorCode = result;
	g_lastErrorMessage = message ? message : "";
}

void NIF_SetLastError(NIF_Result result, const std::string& message)
{
	g_lastErrorCode = result;
	g_lastErrorMessage = message;
}

void NIF_SetLastErrorFromCurrentException(const char* context)
{
	try
	{
		throw;
	}
	catch (const std::exception& exception)
	{
		std::string message = context ? context : "Native binding call failed";
		message += ": ";
		message += exception.what();
		NIF_SetLastError(NIF_RESULT_EXCEPTION, message);
	}
	catch (...)
	{
		NIF_SetLastError(NIF_RESULT_EXCEPTION, context ? context : "Unknown native exception");
	}
}

size_t NIF_CopyStringInternal(const char* source, char* destination, size_t destinationSize)
{
	const char* safeSource = source ? source : "";
	const size_t requiredSize = std::strlen(safeSource) + 1;
	if (destination && destinationSize > 0)
	{
		const size_t copyLength = std::min(requiredSize - 1, destinationSize - 1);
		if (copyLength > 0)
		{
			std::memcpy(destination, safeSource, copyLength);
		}
		destination[copyLength] = '\0';
	}
	return requiredSize;
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

	NIF_ObjectHandle_t* handle = new (std::nothrow) NIF_ObjectHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_ObjectHandle>(handle);
}

NIF_AVObjectHandle NIF_CreateAVObjectHandle(NiAVObject* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_AVObjectHandle_t* handle = new (std::nothrow) NIF_AVObjectHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_AVObjectHandle>(handle);
}

NIF_MeshHandle NIF_CreateMeshHandle(NiMesh* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_MeshHandle_t* handle = new (std::nothrow) NIF_MeshHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate mesh handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_MeshHandle>(handle);
}

NIF_NodeHandle NIF_CreateNodeHandle(NiNode* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_NodeHandle_t* handle = new (std::nothrow) NIF_NodeHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_NodeHandle>(handle);
}

NIF_CameraHandle NIF_CreateCameraHandle(NiCamera* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_CameraHandle_t* handle = new (std::nothrow) NIF_CameraHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_CameraHandle>(handle);
}

NIF_DataStreamHandle NIF_CreateDataStreamHandle(NiDataStream* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DataStreamHandle_t* handle = new (std::nothrow) NIF_DataStreamHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_DataStreamHandle>(handle);
}

NIF_DataStreamRefHandle NIF_CreateDataStreamRefHandle(NiDataStreamRef* pkObject, NiObject* pkOwner)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DataStreamRefHandle_t* handle = new (std::nothrow) NIF_DataStreamRefHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
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

	NIF_ControllerSequenceHandle_t* handle = new (std::nothrow) NIF_ControllerSequenceHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = static_cast<NiObject*>(pkObject);
	return static_cast<NIF_ControllerSequenceHandle>(handle);
}

NIF_TextKeyExtraDataHandle NIF_CreateTextKeyExtraDataHandle(NiTextKeyExtraData* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_TextKeyExtraDataHandle_t* handle = new (std::nothrow) NIF_TextKeyExtraDataHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = static_cast<NiObject*>(pkObject);
	return static_cast<NIF_TextKeyExtraDataHandle>(handle);
}

NIF_RendererHandle NIF_CreateRendererHandle(NiRenderer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RendererHandle_t* handle = new (std::nothrow) NIF_RendererHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_RendererHandle>(handle);
}

NIF_RenderTargetGroupHandle NIF_CreateRenderTargetGroupHandle(NiRenderTargetGroup* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderTargetGroupHandle_t* handle = new (std::nothrow) NIF_RenderTargetGroupHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_RenderTargetGroupHandle>(handle);
}

NIF_RenderBufferHandle NIF_CreateRenderBufferHandle(Ni2DBuffer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderBufferHandle_t* handle = new (std::nothrow) NIF_RenderBufferHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_RenderBufferHandle>(handle);
}

NIF_DepthStencilBufferHandle NIF_CreateDepthStencilBufferHandle(NiDepthStencilBuffer* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DepthStencilBufferHandle_t* handle = new (std::nothrow) NIF_DepthStencilBufferHandle_t();
	if (!handle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	handle->spObject = pkObject;
	return static_cast<NIF_DepthStencilBufferHandle>(handle);
}


#define NIF_DEFINE_NODE_HANDLE_FACTORY(Stem) \
NIF_##Stem##Handle NIF_Create##Stem##Handle(NiNode* pkObject) \
{ \
	if (!pkObject) \
	{ \
		return nullptr; \
	} \
	NIF_##Stem##Handle_t* handle = new (std::nothrow) NIF_##Stem##Handle_t(); \
	if (!handle) \
	{ \
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate " #Stem " handle"); \
		return nullptr; \
	} \
	handle->spObject = pkObject; \
	return static_cast<NIF_##Stem##Handle>(handle); \
}

NIF_DEFINE_NODE_HANDLE_FACTORY(BSPNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(BillboardNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(SwitchNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(LODNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(SortAdjustNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(Terrain)
NIF_DEFINE_NODE_HANDLE_FACTORY(TerrainCell)
NIF_DEFINE_NODE_HANDLE_FACTORY(TerrainCellNode)
NIF_DEFINE_NODE_HANDLE_FACTORY(TerrainCellLeaf)
NIF_DEFINE_NODE_HANDLE_FACTORY(TerrainSector)
NIF_DEFINE_NODE_HANDLE_FACTORY(Atmosphere)
NIF_DEFINE_NODE_HANDLE_FACTORY(Environment)
NIF_DEFINE_NODE_HANDLE_FACTORY(Sky)
NIF_DEFINE_NODE_HANDLE_FACTORY(SkyDome)
NIF_DEFINE_NODE_HANDLE_FACTORY(DecorationField)
NIF_DEFINE_NODE_HANDLE_FACTORY(DecorationLayer)
NIF_DEFINE_NODE_HANDLE_FACTORY(DecorationPlane)
NIF_DEFINE_NODE_HANDLE_FACTORY(OldWall)

#undef NIF_DEFINE_NODE_HANDLE_FACTORY

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

NIF_Result NIF_GetLastErrorCode(void)
{
	return g_lastErrorCode;
}

const char* NIF_GetLastErrorMessage(void)
{
	return g_lastErrorMessage.c_str();
}

size_t NIF_GetLastErrorMessageLength(void)
{
	return g_lastErrorMessage.size();
}

int NIF_CopyLastErrorMessage(char* destination, size_t destinationSize)
{
	const size_t requiredSize = NIF_CopyStringInternal(g_lastErrorMessage.c_str(), destination, destinationSize);
	return destination && destinationSize >= requiredSize ? 1 : 0;
}

void NIF_ClearLastError(void)
{
	g_lastErrorCode = NIF_RESULT_OK;
	g_lastErrorMessage.clear();
}

size_t NIF_CopyString(const char* source, char* destination, size_t destinationSize)
{
	return NIF_CopyStringInternal(source, destination, destinationSize);
}

}
