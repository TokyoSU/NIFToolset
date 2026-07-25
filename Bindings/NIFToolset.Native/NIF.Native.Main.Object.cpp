#include "NIF.Native.Main.Object.h"
#include "NIF.Native.Internal.h"

#include <NiMesh.h>
#include <NiPSParticleSystem.h>
#include <NiControllerSequence.h>
#include <NiDataStream.h>
#include <NiPSEmitter.h>
#include <NiSequenceData.h>
#include <NiTextKeyExtraData.h>
#include <NiPortal.h>
#include <NiRoom.h>
#include <NiRoomGroup.h>

#include <cstring>

namespace
{
	const NiRTTI* NIF_FindRTTIByName(const NiObject* object, const char* typeName)
	{
		if (!object || !typeName || !typeName[0])
		{
			return nullptr;
		}

		const NiRTTI* rtti = object->GetRTTI();
		while (rtti)
		{
			if (std::strcmp(rtti->GetName(), typeName) == 0)
			{
				return rtti;
			}
			rtti = rtti->GetBaseRTTI();
		}
		return nullptr;
	}
	template <class TObject, class THandle>
	int NIF_CastObjectHandle(
		NIF_ObjectHandle object,
		THandle* output,
		THandle (*factory)(TObject*),
		const char* expectedType)
	{
		if (output)
		{
			*output = nullptr;
		}

		NIF_ObjectHandle_t* objectHandle = static_cast<NIF_ObjectHandle_t*>(object);
		if (!objectHandle || !objectHandle->spObject || !output)
		{
			NIF_SetLastError(
				!output ? NIF_RESULT_INVALID_ARGUMENT : NIF_RESULT_INVALID_HANDLE,
				!output ? "Output handle must not be null" : "Invalid object handle");
			return 0;
		}

		TObject* castObject = NiDynamicCast(TObject, objectHandle->spObject);
		if (!castObject)
		{
			std::string message = "Object is not a ";
			message += expectedType;
			NIF_SetLastError(NIF_RESULT_INVALID_TYPE, message);
			return 0;
		}

		*output = factory(castObject);
		return *output ? 1 : 0;
	}

}

extern "C"
{

void NIF_Object_Destroy(NIF_ObjectHandle object)
{
	delete static_cast<NIF_ObjectHandle_t*>(object);
}

const char* NIF_Object_GetName(NIF_ObjectHandle object)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return nullptr;
	}

	return NIF_GetObjectName(pObjectHandle->spObject);
}

size_t NIF_Object_CopyName(NIF_ObjectHandle object, char* destination, size_t destinationSize)
{
	const char* name = NIF_Object_GetName(object);
	if (!name)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_TYPE, "Object does not expose a name");
	}
	return NIF_CopyStringInternal(name, destination, destinationSize);
}

void NIF_Object_SetName(NIF_ObjectHandle object, const char* name)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	NIF_SetObjectName(pObjectHandle->spObject, name);
}

const char* NIF_Object_GetTypeName(NIF_ObjectHandle object)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid object handle");
		return nullptr;
	}

	const NiRTTI* rtti = pObjectHandle->spObject->GetRTTI();
	return rtti ? rtti->GetName() : nullptr;
}

size_t NIF_Object_CopyTypeName(NIF_ObjectHandle object, char* destination, size_t destinationSize)
{
	return NIF_CopyStringInternal(NIF_Object_GetTypeName(object), destination, destinationSize);
}

int NIF_Object_IsKindOf(NIF_ObjectHandle object, const char* typeName)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid object handle");
		return 0;
	}
	if (!typeName || !typeName[0])
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "typeName must not be empty");
		return 0;
	}

	return NIF_FindRTTIByName(pObjectHandle->spObject, typeName) ? 1 : 0;
}

int NIF_Object_AsAVObject(NIF_ObjectHandle object, NIF_AVObjectHandle* outObject)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (outObject)
	{
		*outObject = nullptr;
	}

	if (!pObjectHandle || !pObjectHandle->spObject || !outObject)
	{
		return 0;
	}

	NiAVObject* pkObject = NiDynamicCast(NiAVObject, pObjectHandle->spObject);
	if (!pkObject)
	{
		return 0;
	}

	*outObject = NIF_CreateAVObjectHandle(pkObject);
	return *outObject ? 1 : 0;
}

int NIF_Object_AsNode(NIF_ObjectHandle object, NIF_NodeHandle* outNode)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (outNode)
	{
		*outNode = nullptr;
	}

	if (!pObjectHandle || !pObjectHandle->spObject || !outNode)
	{
		return 0;
	}

	NiNode* pkObject = NiDynamicCast(NiNode, pObjectHandle->spObject);
	if (!pkObject)
	{
		return 0;
	}

	*outNode = NIF_CreateNodeHandle(pkObject);
	return *outNode ? 1 : 0;
}

int NIF_Object_AsCamera(NIF_ObjectHandle object, NIF_CameraHandle* outCamera)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (outCamera)
	{
		*outCamera = nullptr;
	}

	if (!pObjectHandle || !pObjectHandle->spObject || !outCamera)
	{
		return 0;
	}

	NiCamera* pkObject = NiDynamicCast(NiCamera, pObjectHandle->spObject);
	if (!pkObject)
	{
		return 0;
	}

	*outCamera = NIF_CreateCameraHandle(pkObject);
	return *outCamera ? 1 : 0;
}

int NIF_Object_AsMesh(NIF_ObjectHandle object, NIF_MeshHandle* outMesh)
{
	if (outMesh)
	{
		*outMesh = nullptr;
	}
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject || !outMesh)
	{
		NIF_SetLastError(!outMesh ? NIF_RESULT_INVALID_ARGUMENT : NIF_RESULT_INVALID_HANDLE,
			!outMesh ? "outMesh must not be null" : "Invalid object handle");
		return 0;
	}

	NiMesh* mesh = NiDynamicCast(NiMesh, pObjectHandle->spObject);
	if (!mesh)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_TYPE, "Object is not a NiMesh");
		return 0;
	}

	*outMesh = NIF_CreateMeshHandle(mesh);
	return *outMesh ? 1 : 0;
}

int NIF_Object_AsParticleSystem(NIF_ObjectHandle object, NIF_ParticleSystemHandle* outParticleSystem)
{
	if (outParticleSystem)
	{
		*outParticleSystem = nullptr;
	}
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject || !outParticleSystem)
	{
		NIF_SetLastError(!outParticleSystem ? NIF_RESULT_INVALID_ARGUMENT : NIF_RESULT_INVALID_HANDLE,
			!outParticleSystem ? "outParticleSystem must not be null" : "Invalid object handle");
		return 0;
	}

	NiPSParticleSystem* particleSystem = NiDynamicCast(NiPSParticleSystem, pObjectHandle->spObject);
	if (!particleSystem)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_TYPE, "Object is not a NiPSParticleSystem");
		return 0;
	}

	*outParticleSystem = NIF_CreateParticleSystemHandle(particleSystem);
	return *outParticleSystem ? 1 : 0;
}


int NIF_Object_AsDataStream(NIF_ObjectHandle object, NIF_DataStreamHandle* outDataStream)
{
	return NIF_CastObjectHandle<NiDataStream>(
		object, outDataStream, &NIF_CreateDataStreamHandle, "NiDataStream");
}

int NIF_Object_AsControllerSequence(NIF_ObjectHandle object, NIF_ControllerSequenceHandle* outSequence)
{
	return NIF_CastObjectHandle<NiControllerSequence>(
		object, outSequence, &NIF_CreateControllerSequenceHandle, "NiControllerSequence");
}

int NIF_Object_AsSequenceData(NIF_ObjectHandle object, NIF_SequenceDataHandle* outSequenceData)
{
	return NIF_CastObjectHandle<NiSequenceData>(
		object, outSequenceData, &NIF_CreateSequenceDataHandle, "NiSequenceData");
}

int NIF_Object_AsTextKeyExtraData(NIF_ObjectHandle object, NIF_TextKeyExtraDataHandle* outTextKeys)
{
	return NIF_CastObjectHandle<NiTextKeyExtraData>(
		object, outTextKeys, &NIF_CreateTextKeyExtraDataHandle, "NiTextKeyExtraData");
}

int NIF_Object_AsCollisionData(NIF_ObjectHandle object, NIF_CollisionDataHandle* outCollisionData)
{
	return NIF_CastObjectHandle<NiCollisionData>(
		object, outCollisionData, &NIF_CreateCollisionDataHandle, "NiCollisionData");
}

int NIF_Object_AsParticleEmitter(NIF_ObjectHandle object, NIF_PSEmitterHandle* outEmitter)
{
	return NIF_CastObjectHandle<NiPSEmitter>(
		object, outEmitter, &NIF_CreatePSEmitterHandle, "NiPSEmitter");
}

int NIF_Object_AsPortal(NIF_ObjectHandle object, NIF_PortalHandle* outPortal)
{
	return NIF_CastObjectHandle<NiPortal>(
		object, outPortal, &NIF_CreatePortalHandle, "NiPortal");
}

int NIF_Object_AsRoom(NIF_ObjectHandle object, NIF_RoomHandle* outRoom)
{
	return NIF_CastObjectHandle<NiRoom>(
		object, outRoom, &NIF_CreateRoomHandle, "NiRoom");
}

int NIF_Object_AsRoomGroup(NIF_ObjectHandle object, NIF_RoomGroupHandle* outRoomGroup)
{
	return NIF_CastObjectHandle<NiRoomGroup>(
		object, outRoomGroup, &NIF_CreateRoomGroupHandle, "NiRoomGroup");
}

NIF_ObjectHandle NIF_AVObject_AsObject(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* objectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!objectHandle || !objectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid AV object handle");
		return nullptr;
	}

	return NIF_CreateObjectHandle(objectHandle->spObject);
}

int NIF_AVObject_GetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle* outCollisionData)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (outCollisionData)
	{
		*outCollisionData = nullptr;
	}

	if (!pObjectHandle || !pObjectHandle->spObject || !outCollisionData)
	{
		return 0;
	}

	NiCollisionObject* pkCollisionObject = pObjectHandle->spObject->GetCollisionObject();
	NiCollisionData* pkCollisionData = NiDynamicCast(NiCollisionData, pkCollisionObject);
	if (!pkCollisionData)
	{
		return 0;
	}

	*outCollisionData = NIF_CreateCollisionDataHandle(pkCollisionData);
	return *outCollisionData ? 1 : 0;
}

int NIF_AVObject_SetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle collisionData)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	NIF_CollisionDataHandle_t* pCollisionHandle = static_cast<NIF_CollisionDataHandle_t*>(collisionData);
	if (!pObjectHandle || !pObjectHandle->spObject || !pCollisionHandle)
	{
		return 0;
	}

	NiCollisionData* pkCollisionData = NiDynamicCast(NiCollisionData, pCollisionHandle->spObject);
	if (!pkCollisionData)
	{
		return 0;
	}

	pObjectHandle->spObject->SetCollisionObject(pkCollisionData);
	return 1;
}

void NIF_AVObject_ClearCollisionData(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->SetCollisionObject(nullptr);
}

}
