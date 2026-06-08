#pragma once
#ifndef NIF_NATIVE_MAIN_OBJECT_H
#define NIF_NATIVE_MAIN_OBJECT_H

#include "NIF.Native.Common.h"
#include "NIF.Native.Collision.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY void NIF_Object_Destroy(NIF_ObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Object_GetName(NIF_ObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_Object_SetName(NIF_ObjectHandle object, const char* name);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsAVObject(NIF_ObjectHandle object, NIF_AVObjectHandle* outObject);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsNode(NIF_ObjectHandle object, NIF_NodeHandle* outNode);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsCamera(NIF_ObjectHandle object, NIF_CameraHandle* outCamera);
NIFTOOLSET_NATIVE_ENTRY int NIF_AVObject_GetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle* outCollisionData);
NIFTOOLSET_NATIVE_ENTRY int NIF_AVObject_SetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle collisionData);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_ClearCollisionData(NIF_AVObjectHandle object);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MAIN_OBJECT_H
