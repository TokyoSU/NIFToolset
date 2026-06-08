#pragma once
#ifndef NIF_NATIVE_MESH_H
#define NIF_NATIVE_MESH_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct NIF_DataStreamRegion
{
	unsigned int startIndex;
	unsigned int range;
} NIF_DataStreamRegion;

typedef struct NIF_DataStreamElementDesc
{
	int format;
	unsigned int offset;
	unsigned int sizeInBytes;
	unsigned int componentCount;
	unsigned int componentSize;
	int type;
	int isNormalized;
	int isPacked;
} NIF_DataStreamElementDesc;

NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_Destroy(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_MeshHandle NIF_Mesh_Create(void);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Mesh_GetName(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_SetName(NIF_MeshHandle mesh, const char* name);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetVertexCount(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetPrimitiveCount(NIF_MeshHandle mesh, unsigned int submeshIndex);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetTotalPrimitiveCount(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetSubmeshCount(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_SetSubmeshCount(NIF_MeshHandle mesh, unsigned int submeshCount);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetStreamRefCount(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRefHandle NIF_Mesh_GetStreamRefAt(NIF_MeshHandle mesh, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRefHandle NIF_Mesh_FindStreamRef(NIF_MeshHandle mesh, const char* semantic, unsigned int semanticIndex, int format);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRefHandle NIF_Mesh_GetFirstStreamRefByUsage(NIF_MeshHandle mesh, int usage, unsigned int accessMask);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRefHandle NIF_Mesh_AddStream(NIF_MeshHandle mesh, const char* semantic, unsigned int semanticIndex, int format, unsigned int count, unsigned int accessMask, int usage, const void* sourceData, int forceToolDataStream, int createDefaultRegion);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRefHandle NIF_Mesh_AddStreamRef(NIF_MeshHandle mesh, NIF_DataStreamHandle dataStream, const char* semantic, unsigned int semanticIndex, int perInstance);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_RemoveStreamRef(NIF_MeshHandle mesh, NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_RemoveAllStreamRefs(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY int NIF_Mesh_GetPrimitiveType(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_SetPrimitiveType(NIF_MeshHandle mesh, int primitiveType);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Mesh_GetPrimitiveTypeString(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_Bound NIF_Mesh_GetModelBound(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_SetModelBound(NIF_MeshHandle mesh, NIF_Bound bound);
NIFTOOLSET_NATIVE_ENTRY void NIF_Mesh_RecomputeBounds(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Mesh_GetModifierCount(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_ObjectHandle NIF_Mesh_GetModifierAt(NIF_MeshHandle mesh, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY int NIF_Mesh_RemoveModifierAt(NIF_MeshHandle mesh, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY int NIF_Mesh_AttachAllModifiers(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY int NIF_Mesh_DetachAllModifiers(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY int NIF_Mesh_ResetModifiers(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_Mesh_AsAVObject(NIF_MeshHandle mesh);
NIFTOOLSET_NATIVE_ENTRY NIF_ObjectHandle NIF_Mesh_AsObject(NIF_MeshHandle mesh);

NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_Destroy(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamHandle NIF_DataStream_CreateSingleElement(int format, unsigned int count, unsigned int accessMask, int usage, const void* sourceData, int createRegion0, int forceToolDataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetStride(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetSize(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetTotalCount(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetCount(NIF_DataStreamHandle dataStream, unsigned int regionIndex);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetAccessMask(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStream_GetUsage(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetRegionCount(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRegion NIF_DataStream_GetRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_SetRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex, NIF_DataStreamRegion region);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_AddRegion(NIF_DataStreamHandle dataStream, NIF_DataStreamRegion region);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_RemoveRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_RemoveAllRegions(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStream_GetElementDescCount(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStream_GetElementDesc(NIF_DataStreamHandle dataStream, unsigned int index, NIF_DataStreamElementDesc* outElementDesc);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStream_GetStreamable(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_SetStreamable(NIF_DataStreamHandle dataStream, int streamable);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStream_GetLocked(NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY void* NIF_DataStream_Lock(NIF_DataStreamHandle dataStream, unsigned int lockMask);
NIFTOOLSET_NATIVE_ENTRY void* NIF_DataStream_LockRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex, unsigned int lockMask);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStream_Unlock(NIF_DataStreamHandle dataStream, unsigned int lockMask);

NIFTOOLSET_NATIVE_ENTRY void NIF_DataStreamRef_Destroy(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamHandle NIF_DataStreamRef_GetDataStream(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStreamRef_SetDataStream(NIF_DataStreamRefHandle streamRef, NIF_DataStreamHandle dataStream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetStride(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetSize(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetTotalCount(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetCount(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetAccessMask(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStreamRef_GetUsage(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetElementDescCount(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStreamRef_GetElementDesc(NIF_DataStreamRefHandle streamRef, unsigned int index, NIF_DataStreamElementDesc* outElementDesc);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_DataStreamRef_GetSemanticName(NIF_DataStreamRefHandle streamRef, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetSemanticIndex(NIF_DataStreamRefHandle streamRef, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY int NIF_DataStreamRef_IsPerInstance(NIF_DataStreamRefHandle streamRef);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStreamRef_SetPerInstance(NIF_DataStreamRefHandle streamRef, int perInstance);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_DataStreamRef_GetSubmeshRegionIndex(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex);
NIFTOOLSET_NATIVE_ENTRY NIF_DataStreamRegion NIF_DataStreamRef_GetRegionForSubmesh(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStreamRef_BindRegionToSubmesh(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex, unsigned int regionIndex);
NIFTOOLSET_NATIVE_ENTRY void NIF_DataStreamRef_SetActiveCount(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex, unsigned int count);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MESH_H
