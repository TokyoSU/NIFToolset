#include "NIF.Native.Main.Scene.h"
#include "NIF.Native.Internal.h"

extern "C"
{

NIF_NodeHandle NIF_Node_Create(void)
{
	return NIF_CreateNodeHandle(NiNew NiNode());
}

void NIF_Node_Destroy(NIF_NodeHandle node)
{
	delete static_cast<NIF_NodeHandle_t*>(node);
}

unsigned int NIF_Node_GetChildCount(NIF_NodeHandle node)
{
	NIF_NodeHandle_t* pNodeHandle = static_cast<NIF_NodeHandle_t*>(node);
	if (!pNodeHandle || !pNodeHandle->spObject)
	{
		return 0;
	}

	return pNodeHandle->spObject->GetChildCount();
}

NIF_AVObjectHandle NIF_Node_GetChildAt(NIF_NodeHandle node, unsigned int index)
{
	NIF_NodeHandle_t* pNodeHandle = static_cast<NIF_NodeHandle_t*>(node);
	if (!pNodeHandle || !pNodeHandle->spObject)
	{
		return nullptr;
	}

	return NIF_CreateAVObjectHandle(pNodeHandle->spObject->GetAt(index));
}

int NIF_Node_AttachChild(NIF_NodeHandle node, NIF_AVObjectHandle child)
{
	NIF_NodeHandle_t* pNodeHandle = static_cast<NIF_NodeHandle_t*>(node);
	NIF_AVObjectHandle_t* pChildHandle = static_cast<NIF_AVObjectHandle_t*>(child);
	if (!pNodeHandle || !pNodeHandle->spObject || !pChildHandle || !pChildHandle->spObject)
	{
		return 0;
	}

	pNodeHandle->spObject->AttachChild(pChildHandle->spObject);
	return 1;
}

int NIF_Node_DetachChild(NIF_NodeHandle node, NIF_AVObjectHandle child)
{
	NIF_NodeHandle_t* pNodeHandle = static_cast<NIF_NodeHandle_t*>(node);
	NIF_AVObjectHandle_t* pChildHandle = static_cast<NIF_AVObjectHandle_t*>(child);
	if (!pNodeHandle || !pNodeHandle->spObject || !pChildHandle || !pChildHandle->spObject)
	{
		return 0;
	}

	return pNodeHandle->spObject->DetachChild(pChildHandle->spObject) ? 1 : 0;
}

void NIF_Node_RemoveAllChildren(NIF_NodeHandle node)
{
	NIF_NodeHandle_t* pNodeHandle = static_cast<NIF_NodeHandle_t*>(node);
	if (!pNodeHandle || !pNodeHandle->spObject)
	{
		return;
	}

	pNodeHandle->spObject->RemoveAllChildren();
}

void NIF_AVObject_Destroy(NIF_AVObjectHandle object)
{
	delete static_cast<NIF_AVObjectHandle_t*>(object);
}

const char* NIF_AVObject_GetName(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return nullptr;
	}

	return pObjectHandle->spObject->GetName();
}

void NIF_AVObject_SetName(NIF_AVObjectHandle object, const char* name)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->SetName(NiFixedString(name));
}

void NIF_AVObject_Update(NIF_AVObjectHandle object, float time)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->Update(time);
}

void NIF_AVObject_UpdateProperties(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->UpdateProperties();
}

void NIF_AVObject_UpdateEffects(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->UpdateEffects();
}

void NIF_AVObject_SetTranslate(NIF_AVObjectHandle object, NIF_Vec3 translate)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->SetTranslate(NIF_MakePoint3(translate));
}

NIF_Vec3 NIF_AVObject_GetTranslate(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pObjectHandle->spObject->GetTranslate());
}

NIF_Vec3 NIF_AVObject_GetWorldTranslate(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pObjectHandle->spObject->GetWorldTranslate());
}

void NIF_AVObject_SetRotate(NIF_AVObjectHandle object, NIF_Mat3 rotation)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->SetRotate(NIF_MakeMatrix3(rotation));
}

NIF_Mat3 NIF_AVObject_GetRotate(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return NIF_MakeIdentityMat3();
	}

	return NIF_MakeMat3(pObjectHandle->spObject->GetRotate());
}

NIF_Mat3 NIF_AVObject_GetWorldRotate(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return NIF_MakeIdentityMat3();
	}

	return NIF_MakeMat3(pObjectHandle->spObject->GetWorldRotate());
}

void NIF_AVObject_SetScale(NIF_AVObjectHandle object, float scale)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return;
	}

	pObjectHandle->spObject->SetScale(scale);
}

float NIF_AVObject_GetScale(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return 0.0f;
	}

	return pObjectHandle->spObject->GetScale();
}

float NIF_AVObject_GetWorldScale(NIF_AVObjectHandle object)
{
	NIF_AVObjectHandle_t* pObjectHandle = static_cast<NIF_AVObjectHandle_t*>(object);
	if (!pObjectHandle || !pObjectHandle->spObject)
	{
		return 0.0f;
	}

	return pObjectHandle->spObject->GetWorldScale();
}

}
