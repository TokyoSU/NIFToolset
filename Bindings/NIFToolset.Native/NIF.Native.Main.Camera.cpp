#include "NIF.Native.Main.Camera.h"
#include "NIF.Native.Internal.h"

#include <NiFixedString.h>

extern "C"
{

NIF_CameraHandle NIF_Camera_Create(void)
{
	return NIF_CreateCameraHandle(NiNew NiCamera());
}

void NIF_Camera_Destroy(NIF_CameraHandle camera)
{
	delete static_cast<NIF_CameraHandle_t*>(camera);
}

const char* NIF_Camera_GetName(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return nullptr;
	}

	return pCameraHandle->spObject->GetName();
}

void NIF_Camera_SetName(NIF_CameraHandle camera, const char* name)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetName(NiFixedString(name));
}

void NIF_Camera_SetTranslate(NIF_CameraHandle camera, NIF_Vec3 translate)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetTranslate(NIF_MakePoint3(translate));
}

NIF_Vec3 NIF_Camera_GetTranslate(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pCameraHandle->spObject->GetTranslate());
}

NIF_Vec3 NIF_Camera_GetWorldLocation(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pCameraHandle->spObject->GetWorldLocation());
}

NIF_Vec3 NIF_Camera_GetWorldDirection(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pCameraHandle->spObject->GetWorldDirection());
}

NIF_Vec3 NIF_Camera_GetWorldUp(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pCameraHandle->spObject->GetWorldUpVector());
}

NIF_Vec3 NIF_Camera_GetWorldRight(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeVec3(pCameraHandle->spObject->GetWorldRightVector());
}

void NIF_Camera_SetRotate(NIF_CameraHandle camera, NIF_Mat3 rotation)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetRotate(NIF_MakeMatrix3(rotation));
}

NIF_Mat3 NIF_Camera_GetRotate(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return NIF_MakeIdentityMat3();
	}

	return NIF_MakeMat3(pCameraHandle->spObject->GetRotate());
}

void NIF_Camera_SetScale(NIF_CameraHandle camera, float scale)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetScale(scale);
}

float NIF_Camera_GetScale(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return 0.0f;
	}

	return pCameraHandle->spObject->GetScale();
}

void NIF_Camera_Update(NIF_CameraHandle camera, float time)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->Update(time);
}

int NIF_Camera_LookAt(NIF_CameraHandle camera, NIF_Vec3 worldPoint, NIF_Vec3 worldUp)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return 0;
	}

	return pCameraHandle->spObject->LookAtWorldPoint(NIF_MakePoint3(worldPoint), NIF_MakePoint3(worldUp)) ? 1 : 0;
}

void NIF_Camera_SetFrustum(NIF_CameraHandle camera, NIF_Frustum frustum)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetViewFrustum(NIF_MakeFrustum(frustum));
}

NIF_Frustum NIF_Camera_GetFrustum(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0 };
	}

	return NIF_MakeFrustum(pCameraHandle->spObject->GetViewFrustum());
}

void NIF_Camera_SetViewPort(NIF_CameraHandle camera, NIF_Rect rect)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetViewPort(NIF_MakeRect(rect));
}

NIF_Rect NIF_Camera_GetViewPort(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	return NIF_MakeRect(pCameraHandle->spObject->GetViewPort());
}

void NIF_Camera_SetMinNearPlaneDist(NIF_CameraHandle camera, float value)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetMinNearPlaneDist(value);
}

float NIF_Camera_GetMinNearPlaneDist(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return 0.0f;
	}

	return pCameraHandle->spObject->GetMinNearPlaneDist();
}

void NIF_Camera_SetMaxFarNearRatio(NIF_CameraHandle camera, float value)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetMaxFarNearRatio(value);
}

float NIF_Camera_GetMaxFarNearRatio(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return 0.0f;
	}

	return pCameraHandle->spObject->GetMaxFarNearRatio();
}

void NIF_Camera_SetLODAdjust(NIF_CameraHandle camera, float value)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return;
	}

	pCameraHandle->spObject->SetLODAdjust(value);
}

float NIF_Camera_GetLODAdjust(NIF_CameraHandle camera)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (!pCameraHandle || !pCameraHandle->spObject)
	{
		return 0.0f;
	}

	return pCameraHandle->spObject->GetLODAdjust();
}

}
