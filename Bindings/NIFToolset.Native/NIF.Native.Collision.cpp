#include "NIF.Native.Collision.h"
#include "NIF.Native.Internal.h"

#include <NiAVObject.h>
#include <NiBoundingVolume.h>
#include <NiCollisionData.h>
#include <NiCollisionGroup.h>

namespace
{
	NIF_Vec3 NIF_MakeZeroVec3()
	{
		return NIF_Vec3{ 0.0f, 0.0f, 0.0f };
	}

	void NIF_ClearIntersectDesc(NIF_CollisionIntersectDesc* pIntersect)
	{
		if (!pIntersect)
		{
			return;
		}

		pIntersect->root0 = nullptr;
		pIntersect->root1 = nullptr;
		pIntersect->object0 = nullptr;
		pIntersect->object1 = nullptr;
		pIntersect->time = 0.0f;
		pIntersect->point = NIF_MakeZeroVec3();
		pIntersect->normal0 = NIF_MakeZeroVec3();
		pIntersect->normal1 = NIF_MakeZeroVec3();
	}

	void NIF_ClearTriangleDesc(NIF_CollisionTriangleDesc* pTriangle)
	{
		if (!pTriangle)
		{
			return;
		}

		pTriangle->point0 = NIF_MakeZeroVec3();
		pTriangle->point1 = NIF_MakeZeroVec3();
		pTriangle->point2 = NIF_MakeZeroVec3();
	}

	NiBoundingVolume* NIF_GetModelSpaceABV(NiCollisionData* pkCollisionData)
	{
		return pkCollisionData ? pkCollisionData->GetModelSpaceABV() : nullptr;
	}

	NiBoundingVolume* NIF_GetWorldSpaceABV(NiCollisionData* pkCollisionData)
	{
		return pkCollisionData ? pkCollisionData->GetWorldSpaceABV() : nullptr;
	}

	int NIF_GetBoundingVolumeType(NiBoundingVolume* pkBoundingVolume)
	{
		return pkBoundingVolume ? pkBoundingVolume->Type() : static_cast<int>(NiBoundingVolume::BASE_BV);
	}

	unsigned int NIF_GetBoundingVolumeIntersectObjectIndex(NiBoundingVolume* pkBoundingVolume)
	{
		return pkBoundingVolume ? pkBoundingVolume->WhichObjectIntersect() : 0;
	}

	NiCollisionData* NIF_GetCollisionData(NIF_CollisionDataHandle collisionData)
	{
		NIF_CollisionDataHandle_t* pHandle = static_cast<NIF_CollisionDataHandle_t*>(collisionData);
		return pHandle ? NiDynamicCast(NiCollisionData, pHandle->spObject) : nullptr;
	}

	NiCollisionGroup* NIF_GetCollisionGroup(NIF_CollisionGroupHandle collisionGroup)
	{
		NIF_CollisionGroupHandle_t* pHandle = static_cast<NIF_CollisionGroupHandle_t*>(collisionGroup);
		return pHandle ? pHandle->pObject : nullptr;
	}

	NiAVObject* NIF_GetCollisionAVObject(NIF_AVObjectHandle objectHandle)
	{
		NIF_AVObjectHandle_t* pHandle = static_cast<NIF_AVObjectHandle_t*>(objectHandle);
		return pHandle ? pHandle->spObject : nullptr;
	}
}

NIF_CollisionDataHandle NIF_CreateCollisionDataHandle(NiCollisionData* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_CollisionDataHandle_t* pHandle = new NIF_CollisionDataHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_CollisionGroupHandle NIF_CreateCollisionGroupHandle(NiCollisionGroup* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_CollisionGroupHandle_t* pHandle = new NIF_CollisionGroupHandle_t();
	pHandle->pObject = pkObject;
	return pHandle;
}

extern "C"
{

void NIF_Collision_Data_Destroy(NIF_CollisionDataHandle collisionData)
{
	delete static_cast<NIF_CollisionDataHandle_t*>(collisionData);
}

NIF_CollisionDataHandle NIF_Collision_Data_Create(NIF_AVObjectHandle sceneObject)
{
	NiAVObject* pkSceneObject = NIF_GetCollisionAVObject(sceneObject);
	return pkSceneObject ? NIF_CreateCollisionDataHandle(NiNew NiCollisionData(pkSceneObject)) : nullptr;
}

NIF_AVObjectHandle NIF_Collision_Data_GetSceneObject(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? NIF_CreateAVObjectHandle(pkCollisionData->GetSceneGraphObject()) : nullptr;
}

void NIF_Collision_Data_SetPropagationMode(NIF_CollisionDataHandle collisionData, int propagationMode)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->SetPropagationMode(static_cast<NiCollisionData::PropagationMode>(propagationMode));
	}
}

int NIF_Collision_Data_GetPropagationMode(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? static_cast<int>(pkCollisionData->GetPropagationMode()) : static_cast<int>(NiCollisionData::PROPAGATE_NEVER);
}

void NIF_Collision_Data_SetCollisionMode(NIF_CollisionDataHandle collisionData, int collisionMode)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->SetCollisionMode(static_cast<NiCollisionData::CollisionMode>(collisionMode));
	}
}

int NIF_Collision_Data_GetCollisionMode(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? static_cast<int>(pkCollisionData->GetCollisionMode()) : static_cast<int>(NiCollisionData::NOTEST);
}

void NIF_Collision_Data_SetLocalVelocity(NIF_CollisionDataHandle collisionData, NIF_Vec3 velocity)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->SetLocalVelocity(NIF_MakePoint3(velocity));
	}
}

NIF_Vec3 NIF_Collision_Data_GetLocalVelocity(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? NIF_MakeVec3(pkCollisionData->GetLocalVelocity()) : NIF_Vec3{ 0.0f, 0.0f, 0.0f };
}

void NIF_Collision_Data_SetWorldVelocity(NIF_CollisionDataHandle collisionData, NIF_Vec3 velocity)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->SetWorldVelocity(NIF_MakePoint3(velocity));
	}
}

NIF_Vec3 NIF_Collision_Data_GetWorldVelocity(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? NIF_MakeVec3(pkCollisionData->GetWorldVelocity()) : NIF_Vec3{ 0.0f, 0.0f, 0.0f };
}

void NIF_Collision_Data_SetEnableAuxCallbacks(NIF_CollisionDataHandle collisionData, int enable)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->SetEnableAuxCallbacks(enable != 0);
	}
}

int NIF_Collision_Data_GetEnableAuxCallbacks(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return (pkCollisionData && pkCollisionData->GetEnableAuxCallbacks()) ? 1 : 0;
}

void NIF_Collision_Data_CreateWorldVertices(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->CreateWorldVertices();
	}
}

void NIF_Collision_Data_UpdateWorldVertices(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->UpdateWorldVertices();
	}
}

void NIF_Collision_Data_DestroyWorldVertices(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->DestroyWorldVertices();
	}
}

void NIF_Collision_Data_CreateWorldNormals(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->CreateWorldNormals();
	}
}

void NIF_Collision_Data_UpdateWorldNormals(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->UpdateWorldNormals();
	}
}

void NIF_Collision_Data_DestroyWorldNormals(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->DestroyWorldNormals();
	}
}

void NIF_Collision_Data_MarkVerticesAsChanged(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->MarkVerticesAsChanged();
	}
}

void NIF_Collision_Data_MarkNormalsAsChanged(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	if (pkCollisionData)
	{
		pkCollisionData->MarkNormalsAsChanged();
	}
}

unsigned int NIF_Collision_Data_GetTriangleCount(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return pkCollisionData ? pkCollisionData->GetTriangleCount() : 0;
}

int NIF_Collision_Data_GetWorldTriangle(NIF_CollisionDataHandle collisionData, unsigned int triangleIndex, NIF_CollisionTriangleDesc* triangle)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	NIF_ClearTriangleDesc(triangle);
	if (!pkCollisionData || !triangle || triangleIndex > 0xFFFFu)
	{
		return 0;
	}

	NiPoint3* pkPoint0 = nullptr;
	NiPoint3* pkPoint1 = nullptr;
	NiPoint3* pkPoint2 = nullptr;
	if (!pkCollisionData->GetWorldTriangle(static_cast<unsigned short>(triangleIndex), pkPoint0, pkPoint1, pkPoint2) || !pkPoint0 || !pkPoint1 || !pkPoint2)
	{
		return 0;
	}

	triangle->point0 = NIF_MakeVec3(*pkPoint0);
	triangle->point1 = NIF_MakeVec3(*pkPoint1);
	triangle->point2 = NIF_MakeVec3(*pkPoint2);
	return 1;
}

int NIF_Collision_Data_HasModelSpaceABV(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return (pkCollisionData && pkCollisionData->GetModelSpaceABV()) ? 1 : 0;
}

int NIF_Collision_Data_HasWorldSpaceABV(NIF_CollisionDataHandle collisionData)
{
	NiCollisionData* pkCollisionData = NIF_GetCollisionData(collisionData);
	return (pkCollisionData && pkCollisionData->GetWorldSpaceABV()) ? 1 : 0;
}

int NIF_Collision_Data_GetModelSpaceABVType(NIF_CollisionDataHandle collisionData)
{
	return NIF_GetBoundingVolumeType(NIF_GetModelSpaceABV(NIF_GetCollisionData(collisionData)));
}

int NIF_Collision_Data_GetWorldSpaceABVType(NIF_CollisionDataHandle collisionData)
{
	return NIF_GetBoundingVolumeType(NIF_GetWorldSpaceABV(NIF_GetCollisionData(collisionData)));
}

unsigned int NIF_Collision_Data_GetModelSpaceABVIntersectObjectIndex(NIF_CollisionDataHandle collisionData)
{
	return NIF_GetBoundingVolumeIntersectObjectIndex(NIF_GetModelSpaceABV(NIF_GetCollisionData(collisionData)));
}

unsigned int NIF_Collision_Data_GetWorldSpaceABVIntersectObjectIndex(NIF_CollisionDataHandle collisionData)
{
	return NIF_GetBoundingVolumeIntersectObjectIndex(NIF_GetWorldSpaceABV(NIF_GetCollisionData(collisionData)));
}

int NIF_Collision_Data_TestABVIntersect(NIF_CollisionDataHandle collisionData0, NIF_CollisionDataHandle collisionData1, float deltaTime)
{
	NiCollisionData* pkCollisionData0 = NIF_GetCollisionData(collisionData0);
	NiCollisionData* pkCollisionData1 = NIF_GetCollisionData(collisionData1);
	NiBoundingVolume* pkBoundingVolume0 = NIF_GetWorldSpaceABV(pkCollisionData0);
	NiBoundingVolume* pkBoundingVolume1 = NIF_GetWorldSpaceABV(pkCollisionData1);
	return (pkBoundingVolume0 && pkBoundingVolume1 && NiBoundingVolume::TestIntersect(deltaTime, *pkBoundingVolume0, pkCollisionData0->GetWorldVelocity(), *pkBoundingVolume1, pkCollisionData1->GetWorldVelocity())) ? 1 : 0;
}

int NIF_Collision_Data_FindABVIntersect(NIF_CollisionDataHandle collisionData0, NIF_CollisionDataHandle collisionData1, float deltaTime, int calculateNormals, NIF_CollisionIntersectDesc* intersect)
{
	NiCollisionData* pkCollisionData0 = NIF_GetCollisionData(collisionData0);
	NiCollisionData* pkCollisionData1 = NIF_GetCollisionData(collisionData1);
	NiBoundingVolume* pkBoundingVolume0 = NIF_GetWorldSpaceABV(pkCollisionData0);
	NiBoundingVolume* pkBoundingVolume1 = NIF_GetWorldSpaceABV(pkCollisionData1);
	NIF_ClearIntersectDesc(intersect);
	if (!pkCollisionData0 || !pkCollisionData1 || !pkBoundingVolume0 || !pkBoundingVolume1 || !intersect)
	{
		return 0;
	}

	float intersectionTime = 0.0f;
	NiPoint3 intersectionPoint;
	NiPoint3 normal0;
	NiPoint3 normal1;
	if (!NiBoundingVolume::FindIntersect(deltaTime, *pkBoundingVolume0, pkCollisionData0->GetWorldVelocity(), *pkBoundingVolume1, pkCollisionData1->GetWorldVelocity(), intersectionTime, intersectionPoint, calculateNormals != 0, normal0, normal1))
	{
		return 0;
	}

	intersect->root0 = NIF_CreateAVObjectHandle(pkCollisionData0->GetSceneGraphObject());
	intersect->root1 = NIF_CreateAVObjectHandle(pkCollisionData1->GetSceneGraphObject());
	intersect->object0 = intersect->root0;
	intersect->object1 = intersect->root1;
	intersect->time = intersectionTime;
	intersect->point = NIF_MakeVec3(intersectionPoint);
	intersect->normal0 = NIF_MakeVec3(normal0);
	intersect->normal1 = NIF_MakeVec3(normal1);
	return 1;
}

int NIF_Collision_Data_GetCollisionTestType(NIF_AVObjectHandle object0, NIF_AVObjectHandle object1)
{
	NiAVObject* pkObject0 = NIF_GetCollisionAVObject(object0);
	NiAVObject* pkObject1 = NIF_GetCollisionAVObject(object1);
	return (pkObject0 && pkObject1) ? static_cast<int>(NiCollisionData::GetCollisionTestType(pkObject0, pkObject1)) : static_cast<int>(NiCollisionData::NOTEST_NOTEST);
}

int NIF_Collision_Data_ValidateForCollision(NIF_AVObjectHandle objectHandle, int collisionMode)
{
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	return (pkObject && NiCollisionData::ValidateForCollision(pkObject, static_cast<NiCollisionData::CollisionMode>(collisionMode))) ? 1 : 0;
}

void NIF_Collision_Data_SetEnableVelocity(int enable)
{
	NiCollisionData::SetEnableVelocity(enable != 0);
}

int NIF_Collision_Data_GetEnableVelocity(void)
{
	return NiCollisionData::GetEnableVelocity() ? 1 : 0;
}

void NIF_Collision_Group_Destroy(NIF_CollisionGroupHandle collisionGroup)
{
	delete NIF_GetCollisionGroup(collisionGroup);
	delete static_cast<NIF_CollisionGroupHandle_t*>(collisionGroup);
}

NIF_CollisionGroupHandle NIF_Collision_Group_Create(void)
{
	return NIF_CreateCollisionGroupHandle(NiNew NiCollisionGroup());
}

void NIF_Collision_Group_AddCollider(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle, int createCollisionData, int maxDepth, int binSize)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	if (pkCollisionGroup && pkObject)
	{
		pkCollisionGroup->AddCollider(pkObject, createCollisionData != 0, maxDepth, binSize);
	}
}

void NIF_Collision_Group_AddCollidee(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle, int createCollisionData, int maxDepth, int binSize)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	if (pkCollisionGroup && pkObject)
	{
		pkCollisionGroup->AddCollidee(pkObject, createCollisionData != 0, maxDepth, binSize);
	}
}

void NIF_Collision_Group_RemoveCollider(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	if (pkCollisionGroup && pkObject)
	{
		pkCollisionGroup->RemoveCollider(pkObject);
	}
}

void NIF_Collision_Group_RemoveCollidee(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	if (pkCollisionGroup && pkObject)
	{
		pkCollisionGroup->RemoveCollidee(pkObject);
	}
}

void NIF_Collision_Group_Remove(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	if (pkCollisionGroup && pkObject)
	{
		pkCollisionGroup->Remove(pkObject);
	}
}

void NIF_Collision_Group_RemoveAll(NIF_CollisionGroupHandle collisionGroup)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	if (pkCollisionGroup)
	{
		pkCollisionGroup->RemoveAll();
	}
}

int NIF_Collision_Group_IsCollider(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	return (pkCollisionGroup && pkObject && pkCollisionGroup->IsCollider(pkObject)) ? 1 : 0;
}

int NIF_Collision_Group_IsCollidee(NIF_CollisionGroupHandle collisionGroup, NIF_AVObjectHandle objectHandle)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	NiAVObject* pkObject = NIF_GetCollisionAVObject(objectHandle);
	return (pkCollisionGroup && pkObject && pkCollisionGroup->IsCollidee(pkObject)) ? 1 : 0;
}

void NIF_Collision_Group_UpdateWorldData(NIF_CollisionGroupHandle collisionGroup)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	if (pkCollisionGroup)
	{
		pkCollisionGroup->UpdateWorldData();
	}
}

int NIF_Collision_Group_TestCollisions(NIF_CollisionGroupHandle collisionGroup, float deltaTime)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	return (pkCollisionGroup && pkCollisionGroup->TestCollisions(deltaTime)) ? 1 : 0;
}

int NIF_Collision_Group_FindCollisions(NIF_CollisionGroupHandle collisionGroup, float deltaTime)
{
	NiCollisionGroup* pkCollisionGroup = NIF_GetCollisionGroup(collisionGroup);
	if (!pkCollisionGroup)
	{
		return 0;
	}

	pkCollisionGroup->FindCollisions(deltaTime);
	return 1;
}

}
