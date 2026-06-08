#pragma once
#ifndef NIF_NATIVE_MAIN_SCENE_H
#define NIF_NATIVE_MAIN_SCENE_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY NIF_NodeHandle NIF_Node_Create(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_Node_Destroy(NIF_NodeHandle node);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Node_GetChildCount(NIF_NodeHandle node);
NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_Node_GetChildAt(NIF_NodeHandle node, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY int NIF_Node_AttachChild(NIF_NodeHandle node, NIF_AVObjectHandle child);
NIFTOOLSET_NATIVE_ENTRY int NIF_Node_DetachChild(NIF_NodeHandle node, NIF_AVObjectHandle child);
NIFTOOLSET_NATIVE_ENTRY void NIF_Node_RemoveAllChildren(NIF_NodeHandle node);

NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_Destroy(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_AVObject_GetName(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_SetName(NIF_AVObjectHandle object, const char* name);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_Update(NIF_AVObjectHandle object, float time);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_UpdateProperties(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_UpdateEffects(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_SetTranslate(NIF_AVObjectHandle object, NIF_Vec3 translate);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_AVObject_GetTranslate(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY NIF_Vec3 NIF_AVObject_GetWorldTranslate(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_SetRotate(NIF_AVObjectHandle object, NIF_Mat3 rotation);
NIFTOOLSET_NATIVE_ENTRY NIF_Mat3 NIF_AVObject_GetRotate(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY NIF_Mat3 NIF_AVObject_GetWorldRotate(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_SetScale(NIF_AVObjectHandle object, float scale);
NIFTOOLSET_NATIVE_ENTRY float NIF_AVObject_GetScale(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY float NIF_AVObject_GetWorldScale(NIF_AVObjectHandle object);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MAIN_SCENE_H
