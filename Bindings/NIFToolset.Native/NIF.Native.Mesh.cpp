#include "NIF.Native.Mesh.h"
#include "NIF.Native.Internal.h"

#include <NiBound.h>
#include <NiDataStream.h>
#include <NiDataStreamElement.h>
#include <NiDataStreamRef.h>
#include <NiFixedString.h>
#include <NiMesh.h>
#include <NiPrimitiveType.h>

static_assert(sizeof(NIF_DataStreamRegion) == 8u, "Unexpected data-stream region size");
static_assert(sizeof(NIF_DataStreamElementDesc) == 32u, "Unexpected data-stream element descriptor size");

namespace
{
	NIF_Bound NIF_MakeBound(const NiBound& bound)
	{
		return { NIF_MakeVec3(bound.GetCenter()), bound.GetRadius() };
	}

	NiBound NIF_MakeBound(const NIF_Bound& bound)
	{
		NiBound result;
		result.SetCenter(NIF_MakePoint3(bound.center));
		result.SetRadius(bound.radius);
		return result;
	}

	NIF_DataStreamRegion NIF_MakeRegion(const NiDataStream::Region& region)
	{
		return { region.GetStartIndex(), region.GetRange() };
	}

	NiDataStream::Region NIF_MakeRegion(const NIF_DataStreamRegion& region)
	{
		return NiDataStream::Region(region.startIndex, region.range);
	}

	int NIF_TryGetMesh(NIF_MeshHandle mesh, NIF_MeshHandle_t*& pHandle)
	{
		pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
		return (pHandle && pHandle->spObject) ? 1 : 0;
	}

	int NIF_TryGetDataStream(NIF_DataStreamHandle dataStream, NIF_DataStreamHandle_t*& pHandle)
	{
		pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
		return (pHandle && pHandle->spObject) ? 1 : 0;
	}

	int NIF_TryGetDataStreamRef(NIF_DataStreamRefHandle streamRef, NIF_DataStreamRefHandle_t*& pHandle)
	{
		pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
		return (pHandle && pHandle->pObject) ? 1 : 0;
	}

	int NIF_FillElementDesc(const NiDataStreamElement& element, NIF_DataStreamElementDesc* outElementDesc)
	{
		if (!outElementDesc)
		{
			return 0;
		}

		outElementDesc->format = static_cast<int>(element.GetFormat());
		outElementDesc->offset = element.GetOffset();
		outElementDesc->sizeInBytes = static_cast<unsigned int>(element.SizeOf());
		outElementDesc->componentCount = element.GetComponentCount();
		outElementDesc->componentSize = element.GetComponentSize();
		outElementDesc->type = static_cast<int>(element.GetType());
		outElementDesc->isNormalized = element.IsNormalized() ? 1 : 0;
		outElementDesc->isPacked = element.IsPacked() ? 1 : 0;
		return 1;
	}
}

extern "C"
{

void NIF_Mesh_Destroy(NIF_MeshHandle mesh)
{
	delete static_cast<NIF_MeshHandle_t*>(mesh);
}

NIF_MeshHandle NIF_Mesh_Create(void)
{
	NiMeshPtr spMesh = NiNew NiMesh();
	return NIF_CreateMeshHandle(spMesh);
}

const char* NIF_Mesh_GetName(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetName() : nullptr;
}

void NIF_Mesh_SetName(NIF_MeshHandle mesh, const char* name)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return;
	}

	pHandle->spObject->SetName(NiFixedString(name));
}

unsigned int NIF_Mesh_GetVertexCount(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetVertexCount() : 0;
}

unsigned int NIF_Mesh_GetPrimitiveCount(NIF_MeshHandle mesh, unsigned int submeshIndex)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid mesh handle");
		return 0;
	}
	if (submeshIndex >= pHandle->spObject->GetSubmeshCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "submeshIndex is out of range");
		return 0;
	}
	return pHandle->spObject->GetPrimitiveCount(submeshIndex);
}

unsigned int NIF_Mesh_GetTotalPrimitiveCount(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetTotalPrimitiveCount() : 0;
}

unsigned int NIF_Mesh_GetSubmeshCount(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetSubmeshCount() : 0;
}

void NIF_Mesh_SetSubmeshCount(NIF_MeshHandle mesh, unsigned int submeshCount)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return;
	}

	pHandle->spObject->SetSubmeshCount(submeshCount);
}

unsigned int NIF_Mesh_GetStreamRefCount(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetStreamRefCount() : 0;
}

NIF_DataStreamRefHandle NIF_Mesh_GetStreamRefAt(NIF_MeshHandle mesh, unsigned int index)
{
	NIF_MeshHandle_t* pHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pHandle) || index >= pHandle->spObject->GetStreamRefCount())
	{
		return nullptr;
	}

	return NIF_CreateDataStreamRefHandle(pHandle->spObject->GetStreamRefAt(index), pHandle->spObject);
}

NIF_DataStreamRefHandle NIF_Mesh_FindStreamRef(NIF_MeshHandle mesh, const char* semantic, unsigned int semanticIndex, int format)
{
	NIF_MeshHandle_t* pHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pHandle) || !semantic)
	{
		return nullptr;
	}

	NiDataStreamRef* pkRef = pHandle->spObject->FindStreamRef(NiFixedString(semantic), semanticIndex,
		static_cast<NiDataStreamElement::Format>(format));
	return NIF_CreateDataStreamRefHandle(pkRef, pHandle->spObject);
}

NIF_DataStreamRefHandle NIF_Mesh_GetFirstStreamRefByUsage(NIF_MeshHandle mesh, int usage, unsigned int accessMask)
{
	NIF_MeshHandle_t* pHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pHandle))
	{
		return nullptr;
	}

	NiDataStreamRef* pkRef = pHandle->spObject->GetFirstStreamRef(static_cast<NiDataStream::Usage>(usage), accessMask);
	return NIF_CreateDataStreamRefHandle(pkRef, pHandle->spObject);
}

NIF_DataStreamRefHandle NIF_Mesh_AddStream(NIF_MeshHandle mesh, const char* semantic, unsigned int semanticIndex, int format, unsigned int count, unsigned int accessMask, int usage, const void* sourceData, int forceToolDataStream, int createDefaultRegion)
{
	NIF_MeshHandle_t* pHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pHandle) || !semantic)
	{
		return nullptr;
	}

	NiDataStreamRef* pkRef = pHandle->spObject->AddStream(NiFixedString(semantic), semanticIndex,
		static_cast<NiDataStreamElement::Format>(format), count, static_cast<NiUInt8>(accessMask),
		static_cast<NiDataStream::Usage>(usage), sourceData, forceToolDataStream != 0, createDefaultRegion != 0);
	return NIF_CreateDataStreamRefHandle(pkRef, pHandle->spObject);
}

NIF_DataStreamRefHandle NIF_Mesh_AddStreamRef(NIF_MeshHandle mesh, NIF_DataStreamHandle dataStream, const char* semantic, unsigned int semanticIndex, int perInstance)
{
	NIF_MeshHandle_t* pMeshHandle = nullptr;
	NIF_DataStreamHandle_t* pDataStreamHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pMeshHandle) || !NIF_TryGetDataStream(dataStream, pDataStreamHandle) || !semantic)
	{
		return nullptr;
	}

	NiDataStreamRef* pkRef = pMeshHandle->spObject->AddStreamRef(pDataStreamHandle->spObject, NiFixedString(semantic), semanticIndex, perInstance != 0);
	return NIF_CreateDataStreamRefHandle(pkRef, pMeshHandle->spObject);
}

void NIF_Mesh_RemoveStreamRef(NIF_MeshHandle mesh, NIF_DataStreamRefHandle streamRef)
{
	NIF_MeshHandle_t* pMeshHandle = nullptr;
	NIF_DataStreamRefHandle_t* pStreamRefHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pMeshHandle) || !NIF_TryGetDataStreamRef(streamRef, pStreamRefHandle))
	{
		return;
	}

	pMeshHandle->spObject->RemoveStreamRef(pStreamRefHandle->pObject);
	pStreamRefHandle->pObject = nullptr;
	pStreamRefHandle->spOwner = nullptr;
}

void NIF_Mesh_RemoveAllStreamRefs(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = nullptr;
	if (!NIF_TryGetMesh(mesh, pHandle))
	{
		return;
	}

	pHandle->spObject->RemoveAllStreamRefs();
}

int NIF_Mesh_GetPrimitiveType(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? static_cast<int>(pHandle->spObject->GetPrimitiveType()) : 0;
}

void NIF_Mesh_SetPrimitiveType(NIF_MeshHandle mesh, int primitiveType)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return;
	}

	if (primitiveType < 0 || primitiveType >= NiPrimitiveType::PRIMITIVE_MAX)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "primitiveType is invalid");
		return;
	}

	pHandle->spObject->SetPrimitiveType(static_cast<NiPrimitiveType::Type>(primitiveType));
}

const char* NIF_Mesh_GetPrimitiveTypeString(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetPrimitiveTypeString() : nullptr;
}

NIF_Bound NIF_Mesh_GetModelBound(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return { { 0.0f, 0.0f, 0.0f }, 0.0f };
	}

	return NIF_MakeBound(pHandle->spObject->GetModelBound());
}

void NIF_Mesh_SetModelBound(NIF_MeshHandle mesh, NIF_Bound bound)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return;
	}

	pHandle->spObject->SetModelBound(NIF_MakeBound(bound));
}

void NIF_Mesh_RecomputeBounds(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		return;
	}

	pHandle->spObject->RecomputeBounds();
}

unsigned int NIF_Mesh_GetModifierCount(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetModifierCount() : 0;
}

NIF_ObjectHandle NIF_Mesh_GetModifierAt(NIF_MeshHandle mesh, unsigned int index)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid mesh handle");
		return nullptr;
	}
	if (index >= pHandle->spObject->GetModifierCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "modifier index is out of range");
		return nullptr;
	}

	return NIF_CreateObjectHandle(pHandle->spObject->GetModifierAt(index));
}

int NIF_Mesh_RemoveModifierAt(NIF_MeshHandle mesh, unsigned int index)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	if (!pHandle || !pHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid mesh handle");
		return 0;
	}
	if (index >= pHandle->spObject->GetModifierCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "modifier index is out of range");
		return 0;
	}
	return pHandle->spObject->RemoveModifierAt(index) ? 1 : 0;
}

int NIF_Mesh_AttachAllModifiers(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject && pHandle->spObject->AttachAllModifiers()) ? 1 : 0;
}

int NIF_Mesh_DetachAllModifiers(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject && pHandle->spObject->DetachAllModifiers()) ? 1 : 0;
}

int NIF_Mesh_ResetModifiers(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject && pHandle->spObject->ResetModifiers()) ? 1 : 0;
}

NIF_AVObjectHandle NIF_Mesh_AsAVObject(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? NIF_CreateAVObjectHandle(pHandle->spObject) : nullptr;
}

NIF_ObjectHandle NIF_Mesh_AsObject(NIF_MeshHandle mesh)
{
	NIF_MeshHandle_t* pHandle = static_cast<NIF_MeshHandle_t*>(mesh);
	return (pHandle && pHandle->spObject) ? NIF_CreateObjectHandle(pHandle->spObject) : nullptr;
}


NIF_ObjectHandle NIF_DataStream_AsObject(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* dataStreamHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	if (!dataStreamHandle || !dataStreamHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid data stream handle");
		return nullptr;
	}

	return NIF_CreateObjectHandle(dataStreamHandle->spObject);
}

void NIF_DataStream_Destroy(NIF_DataStreamHandle dataStream)
{
	delete static_cast<NIF_DataStreamHandle_t*>(dataStream);
}

NIF_DataStreamHandle NIF_DataStream_CreateSingleElement(int format, unsigned int count, unsigned int accessMask, int usage, const void* sourceData, int createRegion0, int forceToolDataStream)
{
	NiDataStream* pkStream = NiDataStream::CreateSingleElementDataStream(static_cast<NiDataStreamElement::Format>(format),
		count, static_cast<NiUInt8>(accessMask), static_cast<NiDataStream::Usage>(usage), sourceData,
		createRegion0 != 0, forceToolDataStream != 0);
	return NIF_CreateDataStreamHandle(pkStream);
}

unsigned int NIF_DataStream_GetStride(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetStride() : 0;
}

unsigned int NIF_DataStream_GetSize(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetSize() : 0;
}

unsigned int NIF_DataStream_GetTotalCount(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetTotalCount() : 0;
}

unsigned int NIF_DataStream_GetCount(NIF_DataStreamHandle dataStream, unsigned int regionIndex)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || regionIndex >= pHandle->spObject->GetRegionCount())
	{
		return 0;
	}

	return pHandle->spObject->GetCount(regionIndex);
}

unsigned int NIF_DataStream_GetAccessMask(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetAccessMask() : 0;
}

int NIF_DataStream_GetUsage(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? static_cast<int>(pHandle->spObject->GetUsage()) : 0;
}

unsigned int NIF_DataStream_GetRegionCount(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetRegionCount() : 0;
}

NIF_DataStreamRegion NIF_DataStream_GetRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || regionIndex >= pHandle->spObject->GetRegionCount())
	{
		return { 0, 0 };
	}

	return NIF_MakeRegion(pHandle->spObject->GetRegion(regionIndex));
}

void NIF_DataStream_SetRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex, NIF_DataStreamRegion region)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || regionIndex >= pHandle->spObject->GetRegionCount())
	{
		return;
	}

	pHandle->spObject->SetRegion(NIF_MakeRegion(region), regionIndex);
}

unsigned int NIF_DataStream_AddRegion(NIF_DataStreamHandle dataStream, NIF_DataStreamRegion region)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle))
	{
		return 0;
	}

	return pHandle->spObject->AddRegion(NIF_MakeRegion(region));
}

void NIF_DataStream_RemoveRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || regionIndex >= pHandle->spObject->GetRegionCount())
	{
		return;
	}

	pHandle->spObject->RemoveRegion(regionIndex);
}

void NIF_DataStream_RemoveAllRegions(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle))
	{
		return;
	}

	pHandle->spObject->RemoveAllRegions();
}

unsigned int NIF_DataStream_GetElementDescCount(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject) ? pHandle->spObject->GetElementDescCount() : 0;
}

int NIF_DataStream_GetElementDesc(NIF_DataStreamHandle dataStream, unsigned int index, NIF_DataStreamElementDesc* outElementDesc)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || index >= pHandle->spObject->GetElementDescCount())
	{
		return 0;
	}

	return NIF_FillElementDesc(pHandle->spObject->GetElementDescAt(index), outElementDesc);
}

int NIF_DataStream_GetStreamable(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject && pHandle->spObject->GetStreamable()) ? 1 : 0;
}

void NIF_DataStream_SetStreamable(NIF_DataStreamHandle dataStream, int streamable)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle))
	{
		return;
	}

	pHandle->spObject->SetStreamable(streamable != 0);
}

int NIF_DataStream_GetLocked(NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamHandle_t* pHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	return (pHandle && pHandle->spObject && pHandle->spObject->GetLocked()) ? 1 : 0;
}

void* NIF_DataStream_Lock(NIF_DataStreamHandle dataStream, unsigned int lockMask)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle))
	{
		return nullptr;
	}

	return pHandle->spObject->Lock(static_cast<NiUInt8>(lockMask));
}

void* NIF_DataStream_LockRegion(NIF_DataStreamHandle dataStream, unsigned int regionIndex, unsigned int lockMask)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle) || regionIndex >= pHandle->spObject->GetRegionCount())
	{
		return nullptr;
	}

	return pHandle->spObject->LockRegion(regionIndex, static_cast<NiUInt8>(lockMask));
}

void NIF_DataStream_Unlock(NIF_DataStreamHandle dataStream, unsigned int lockMask)
{
	NIF_DataStreamHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStream(dataStream, pHandle))
	{
		return;
	}

	pHandle->spObject->Unlock(static_cast<NiUInt8>(lockMask));
}

void NIF_DataStreamRef_Destroy(NIF_DataStreamRefHandle streamRef)
{
	delete static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
}

NIF_DataStreamHandle NIF_DataStreamRef_GetDataStream(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle))
	{
		return nullptr;
	}

	return NIF_CreateDataStreamHandle(pHandle->pObject->GetDataStream());
}

void NIF_DataStreamRef_SetDataStream(NIF_DataStreamRefHandle streamRef, NIF_DataStreamHandle dataStream)
{
	NIF_DataStreamRefHandle_t* pRefHandle = nullptr;
	NIF_DataStreamHandle_t* pDataStreamHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pRefHandle) || !NIF_TryGetDataStream(dataStream, pDataStreamHandle))
	{
		return;
	}

	pRefHandle->pObject->SetDataStream(pDataStreamHandle->spObject);
}

unsigned int NIF_DataStreamRef_GetStride(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? pHandle->pObject->GetStride() : 0;
}

unsigned int NIF_DataStreamRef_GetSize(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? pHandle->pObject->GetSize() : 0;
}

unsigned int NIF_DataStreamRef_GetTotalCount(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? pHandle->pObject->GetTotalCount() : 0;
}

unsigned int NIF_DataStreamRef_GetCount(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || submeshIndex >= pHandle->pObject->GetSubmeshRemapCount())
	{
		return 0;
	}

	return pHandle->pObject->GetCount(submeshIndex);
}

unsigned int NIF_DataStreamRef_GetAccessMask(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? pHandle->pObject->GetAccessMask() : 0;
}

int NIF_DataStreamRef_GetUsage(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? static_cast<int>(pHandle->pObject->GetUsage()) : 0;
}

unsigned int NIF_DataStreamRef_GetElementDescCount(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject) ? pHandle->pObject->GetElementDescCount() : 0;
}

int NIF_DataStreamRef_GetElementDesc(NIF_DataStreamRefHandle streamRef, unsigned int index, NIF_DataStreamElementDesc* outElementDesc)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || index >= pHandle->pObject->GetElementDescCount())
	{
		return 0;
	}

	return NIF_FillElementDesc(pHandle->pObject->GetElementDescAt(index), outElementDesc);
}

const char* NIF_DataStreamRef_GetSemanticName(NIF_DataStreamRefHandle streamRef, unsigned int index)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || index >= pHandle->pObject->GetElementDescCount())
	{
		return nullptr;
	}

	return static_cast<const char*>(pHandle->pObject->GetSemanticNameAt(index));
}

unsigned int NIF_DataStreamRef_GetSemanticIndex(NIF_DataStreamRefHandle streamRef, unsigned int index)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || index >= pHandle->pObject->GetElementDescCount())
	{
		return 0;
	}

	return pHandle->pObject->GetSemanticIndexAt(index);
}

int NIF_DataStreamRef_IsPerInstance(NIF_DataStreamRefHandle streamRef)
{
	NIF_DataStreamRefHandle_t* pHandle = static_cast<NIF_DataStreamRefHandle_t*>(streamRef);
	return (pHandle && pHandle->pObject && pHandle->pObject->IsPerInstance()) ? 1 : 0;
}

void NIF_DataStreamRef_SetPerInstance(NIF_DataStreamRefHandle streamRef, int perInstance)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle))
	{
		return;
	}

	pHandle->pObject->SetPerInstance(perInstance != 0);
}

unsigned int NIF_DataStreamRef_GetSubmeshRegionIndex(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || submeshIndex >= pHandle->pObject->GetSubmeshRemapCount())
	{
		return 0;
	}

	return pHandle->pObject->GetRegionIndexForSubmesh(submeshIndex);
}

NIF_DataStreamRegion NIF_DataStreamRef_GetRegionForSubmesh(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || submeshIndex >= pHandle->pObject->GetSubmeshRemapCount())
	{
		return { 0, 0 };
	}

	return NIF_MakeRegion(pHandle->pObject->GetRegionForSubmesh(submeshIndex));
}

void NIF_DataStreamRef_BindRegionToSubmesh(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex, unsigned int regionIndex)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle))
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid data-stream-reference handle");
		return;
	}
	if (submeshIndex >= pHandle->pObject->GetSubmeshRemapCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "submeshIndex is out of range");
		return;
	}
	NiDataStream* dataStream = pHandle->pObject->GetDataStream();
	if (!dataStream || regionIndex >= dataStream->GetRegionCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "regionIndex is out of range");
		return;
	}

	pHandle->pObject->BindRegionToSubmesh(submeshIndex, regionIndex);
}

void NIF_DataStreamRef_SetActiveCount(NIF_DataStreamRefHandle streamRef, unsigned int submeshIndex, unsigned int count)
{
	NIF_DataStreamRefHandle_t* pHandle = nullptr;
	if (!NIF_TryGetDataStreamRef(streamRef, pHandle) || submeshIndex >= pHandle->pObject->GetSubmeshRemapCount())
	{
		return;
	}

	pHandle->pObject->SetActiveCount(submeshIndex, count);
}

}
