#pragma once
#ifndef NIF_NATIVE_MAIN_CAMERA_H
#define NIF_NATIVE_MAIN_CAMERA_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY NIF_CameraHandle NIF_Camera_Create(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_Destroy(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Camera_GetName(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetName(NIF_CameraHandle camera, const char* name);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetTranslate(NIF_CameraHandle camera, NIF_Vec3 translate);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_Camera_GetTranslate(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_Camera_GetWorldLocation(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_Camera_GetWorldDirection(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_Camera_GetWorldUp(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_Camera_GetWorldRight(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetRotate(NIF_CameraHandle camera, NIF_Mat3 rotation);
NIFTOOLSET_NATIVE_ENTRY NIF_Mat3 NIF_Camera_GetRotate(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetScale(NIF_CameraHandle camera, float scale);
NIFTOOLSET_NATIVE_ENTRY float NIF_Camera_GetScale(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_Update(NIF_CameraHandle camera, float time);
NIFTOOLSET_NATIVE_ENTRY int NIF_Camera_LookAt(NIF_CameraHandle camera, NIF_Vec3 worldPoint, NIF_Vec3 worldUp);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetFrustum(NIF_CameraHandle camera, NIF_Frustum frustum);
NIFTOOLSET_NATIVE_ENTRY NIF_Frustum NIF_Camera_GetFrustum(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetViewPort(NIF_CameraHandle camera, NIF_Rect rect);
NIFTOOLSET_NATIVE_ENTRY NIF_Rect NIF_Camera_GetViewPort(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetMinNearPlaneDist(NIF_CameraHandle camera, float value);
NIFTOOLSET_NATIVE_ENTRY float NIF_Camera_GetMinNearPlaneDist(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetMaxFarNearRatio(NIF_CameraHandle camera, float value);
NIFTOOLSET_NATIVE_ENTRY float NIF_Camera_GetMaxFarNearRatio(NIF_CameraHandle camera);
NIFTOOLSET_NATIVE_ENTRY void NIF_Camera_SetLODAdjust(NIF_CameraHandle camera, float value);
NIFTOOLSET_NATIVE_ENTRY float NIF_Camera_GetLODAdjust(NIF_CameraHandle camera);

NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_Camera_AsAVObject(NIF_CameraHandle camera);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MAIN_CAMERA_H
