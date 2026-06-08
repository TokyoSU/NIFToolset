#include "NIF.Native.Main.Object.h"
#include "NIF.Native.Internal.h"

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

void NIF_Object_SetName(NIF_ObjectHandle object, const char* name)
{
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	NIF_SetObjectName(pObjectHandle->spObject, name);
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
