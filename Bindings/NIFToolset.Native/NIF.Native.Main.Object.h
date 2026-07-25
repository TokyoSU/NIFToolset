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
NIFTOOLSET_NATIVE_ENTRY size_t NIF_Object_CopyName(NIF_ObjectHandle object, char* destination, size_t destinationSize);
NIFTOOLSET_NATIVE_ENTRY void NIF_Object_SetName(NIF_ObjectHandle object, const char* name);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Object_GetTypeName(NIF_ObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY size_t NIF_Object_CopyTypeName(NIF_ObjectHandle object, char* destination, size_t destinationSize);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_IsKindOf(NIF_ObjectHandle object, const char* typeName);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsAVObject(NIF_ObjectHandle object, NIF_AVObjectHandle* outObject);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsNode(NIF_ObjectHandle object, NIF_NodeHandle* outNode);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsCamera(NIF_ObjectHandle object, NIF_CameraHandle* outCamera);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsMesh(NIF_ObjectHandle object, NIF_MeshHandle* outMesh);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsParticleSystem(NIF_ObjectHandle object, NIF_ParticleSystemHandle* outParticleSystem);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsDataStream(NIF_ObjectHandle object, NIF_DataStreamHandle* outDataStream);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsControllerSequence(NIF_ObjectHandle object, NIF_ControllerSequenceHandle* outSequence);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsSequenceData(NIF_ObjectHandle object, NIF_SequenceDataHandle* outSequenceData);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsTextKeyExtraData(NIF_ObjectHandle object, NIF_TextKeyExtraDataHandle* outTextKeys);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsCollisionData(NIF_ObjectHandle object, NIF_CollisionDataHandle* outCollisionData);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsParticleEmitter(NIF_ObjectHandle object, NIF_PSEmitterHandle* outEmitter);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsPortal(NIF_ObjectHandle object, NIF_PortalHandle* outPortal);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsRoom(NIF_ObjectHandle object, NIF_RoomHandle* outRoom);
NIFTOOLSET_NATIVE_ENTRY int NIF_Object_AsRoomGroup(NIF_ObjectHandle object, NIF_RoomGroupHandle* outRoomGroup);
NIFTOOLSET_NATIVE_ENTRY NIF_ObjectHandle NIF_AVObject_AsObject(NIF_AVObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY int NIF_AVObject_GetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle* outCollisionData);
NIFTOOLSET_NATIVE_ENTRY int NIF_AVObject_SetCollisionData(NIF_AVObjectHandle object, NIF_CollisionDataHandle collisionData);
NIFTOOLSET_NATIVE_ENTRY void NIF_AVObject_ClearCollisionData(NIF_AVObjectHandle object);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MAIN_OBJECT_H
