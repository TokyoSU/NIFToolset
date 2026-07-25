#include "NIF.Native.Nodes.h"
#include "NIF.Native.Internal.h"

#include <cstring>

namespace
{
	bool NIF_IsKindOfName(const NiObject* object, const char* typeName)
	{
		if (!object || !typeName || !typeName[0])
		{
			return false;
		}
		const NiRTTI* rtti = object->GetRTTI();
		while (rtti)
		{
			if (std::strcmp(rtti->GetName(), typeName) == 0)
			{
				return true;
			}
			rtti = rtti->GetBaseRTTI();
		}
		return false;
	}

	template <class THandle>
	int NIF_CastNamedNode(NIF_ObjectHandle object, THandle* output, THandle (*factory)(NiNode*), const char* expectedType)
	{
		if (output)
		{
			*output = nullptr;
		}
		NIF_ObjectHandle_t* objectHandle = static_cast<NIF_ObjectHandle_t*>(object);
		if (!output)
		{
			NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "Output node handle must not be null");
			return 0;
		}
		if (!objectHandle || !objectHandle->spObject)
		{
			NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid object handle");
			return 0;
		}
		NiNode* node = NiDynamicCast(NiNode, objectHandle->spObject);
		if (!node || !NIF_IsKindOfName(objectHandle->spObject, expectedType))
		{
			NIF_SetLastError(NIF_RESULT_INVALID_TYPE, "Object is not the requested node type");
			return 0;
		}
		*output = factory(node);
		return *output ? 1 : 0;
	}

	template <class THandle>
	NiNode* NIF_GetSpecializedNode(THandle handle)
	{
		if (!handle || !handle->spObject)
		{
			NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid specialized node handle");
			return nullptr;
		}
		return handle->spObject;
	}

	struct NodeTypeEntry
	{
		NIF_NodeType type;
		const char* name;
	};

	constexpr NodeTypeEntry kNodeTypes[] =
	{
		{ NIF_NODE_TYPE_NODE, "NiNode" },
		{ NIF_NODE_TYPE_BSP_NODE, "NiBSPNode" },
		{ NIF_NODE_TYPE_BILLBOARD_NODE, "NiBillboardNode" },
		{ NIF_NODE_TYPE_SWITCH_NODE, "NiSwitchNode" },
		{ NIF_NODE_TYPE_LOD_NODE, "NiLODNode" },
		{ NIF_NODE_TYPE_SORT_ADJUST_NODE, "NiSortAdjustNode" },
		{ NIF_NODE_TYPE_ROOM, "NiRoom" },
		{ NIF_NODE_TYPE_OLD_WALL, "NiOldWall" },
		{ NIF_NODE_TYPE_ROOM_GROUP, "NiRoomGroup" },
		{ NIF_NODE_TYPE_TERRAIN, "NiTerrain" },
		{ NIF_NODE_TYPE_TERRAIN_CELL, "NiTerrainCell" },
		{ NIF_NODE_TYPE_TERRAIN_CELL_NODE, "NiTerrainCellNode" },
		{ NIF_NODE_TYPE_TERRAIN_CELL_LEAF, "NiTerrainCellLeaf" },
		{ NIF_NODE_TYPE_TERRAIN_SECTOR, "NiTerrainSector" },
		{ NIF_NODE_TYPE_ATMOSPHERE, "NiAtmosphere" },
		{ NIF_NODE_TYPE_ENVIRONMENT, "NiEnvironment" },
		{ NIF_NODE_TYPE_SKY, "NiSky" },
		{ NIF_NODE_TYPE_SKY_DOME, "NiSkyDome" },
		{ NIF_NODE_TYPE_DECORATION_FIELD, "NiDecorationField" },
		{ NIF_NODE_TYPE_DECORATION_LAYER, "NiDecorationLayer" },
		{ NIF_NODE_TYPE_DECORATION_PLANE, "NiDecorationPlane" },
	};
}

extern "C"
{

NIF_NodeType NIF_Object_GetNodeType(NIF_ObjectHandle object)
{
	NIF_ObjectHandle_t* objectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!objectHandle || !objectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid object handle");
		return NIF_NODE_TYPE_UNKNOWN;
	}
	NiNode* node = NiDynamicCast(NiNode, objectHandle->spObject);
	if (!node)
	{
		return NIF_NODE_TYPE_UNKNOWN;
	}
	const NiRTTI* rtti = objectHandle->spObject->GetRTTI();
	const char* exactName = rtti ? rtti->GetName() : nullptr;
	if (!exactName)
	{
		return NIF_NODE_TYPE_UNKNOWN;
	}
	for (const NodeTypeEntry& entry : kNodeTypes)
	{
		if (std::strcmp(entry.name, exactName) == 0)
		{
			return entry.type;
		}
	}
	return NIF_NODE_TYPE_UNKNOWN;
}

const char* NIF_NodeType_GetName(NIF_NodeType type)
{
	if (type == NIF_NODE_TYPE_UNKNOWN)
	{
		return "Unknown";
	}
	for (const NodeTypeEntry& entry : kNodeTypes)
	{
		if (entry.type == type)
		{
			return entry.name;
		}
	}
	NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "Unknown NIF_NodeType value");
	return "Unknown";
}

int NIF_Object_AsBSPNode(NIF_ObjectHandle object, NIF_BSPNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateBSPNodeHandle, "NiBSPNode");
}

void NIF_BSPNode_Destroy(NIF_BSPNodeHandle node)
{
	delete static_cast<NIF_BSPNodeHandle_t*>(node);
}

NIF_NodeHandle NIF_BSPNode_AsNode(NIF_BSPNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsBillboardNode(NIF_ObjectHandle object, NIF_BillboardNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateBillboardNodeHandle, "NiBillboardNode");
}

void NIF_BillboardNode_Destroy(NIF_BillboardNodeHandle node)
{
	delete static_cast<NIF_BillboardNodeHandle_t*>(node);
}

NIF_NodeHandle NIF_BillboardNode_AsNode(NIF_BillboardNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsSwitchNode(NIF_ObjectHandle object, NIF_SwitchNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateSwitchNodeHandle, "NiSwitchNode");
}

void NIF_SwitchNode_Destroy(NIF_SwitchNodeHandle node)
{
	delete static_cast<NIF_SwitchNodeHandle_t*>(node);
}

NIF_NodeHandle NIF_SwitchNode_AsNode(NIF_SwitchNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsLODNode(NIF_ObjectHandle object, NIF_LODNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateLODNodeHandle, "NiLODNode");
}

void NIF_LODNode_Destroy(NIF_LODNodeHandle node)
{
	delete static_cast<NIF_LODNodeHandle_t*>(node);
}

NIF_SwitchNodeHandle NIF_LODNode_AsSwitchNode(NIF_LODNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateSwitchNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsSortAdjustNode(NIF_ObjectHandle object, NIF_SortAdjustNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateSortAdjustNodeHandle, "NiSortAdjustNode");
}

void NIF_SortAdjustNode_Destroy(NIF_SortAdjustNodeHandle node)
{
	delete static_cast<NIF_SortAdjustNodeHandle_t*>(node);
}

NIF_NodeHandle NIF_SortAdjustNode_AsNode(NIF_SortAdjustNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsOldWall(NIF_ObjectHandle object, NIF_OldWallHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateOldWallHandle, "NiOldWall");
}

void NIF_OldWall_Destroy(NIF_OldWallHandle node)
{
	delete static_cast<NIF_OldWallHandle_t*>(node);
}

NIF_NodeHandle NIF_OldWall_AsNode(NIF_OldWallHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsTerrain(NIF_ObjectHandle object, NIF_TerrainHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateTerrainHandle, "NiTerrain");
}

void NIF_Terrain_Destroy(NIF_TerrainHandle node)
{
	delete static_cast<NIF_TerrainHandle_t*>(node);
}

NIF_NodeHandle NIF_Terrain_AsNode(NIF_TerrainHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsTerrainCell(NIF_ObjectHandle object, NIF_TerrainCellHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateTerrainCellHandle, "NiTerrainCell");
}

void NIF_TerrainCell_Destroy(NIF_TerrainCellHandle node)
{
	delete static_cast<NIF_TerrainCellHandle_t*>(node);
}

NIF_NodeHandle NIF_TerrainCell_AsNode(NIF_TerrainCellHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsTerrainCellNode(NIF_ObjectHandle object, NIF_TerrainCellNodeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateTerrainCellNodeHandle, "NiTerrainCellNode");
}

void NIF_TerrainCellNode_Destroy(NIF_TerrainCellNodeHandle node)
{
	delete static_cast<NIF_TerrainCellNodeHandle_t*>(node);
}

NIF_TerrainCellHandle NIF_TerrainCellNode_AsTerrainCell(NIF_TerrainCellNodeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateTerrainCellHandle(nativeNode) : nullptr;
}

int NIF_Object_AsTerrainCellLeaf(NIF_ObjectHandle object, NIF_TerrainCellLeafHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateTerrainCellLeafHandle, "NiTerrainCellLeaf");
}

void NIF_TerrainCellLeaf_Destroy(NIF_TerrainCellLeafHandle node)
{
	delete static_cast<NIF_TerrainCellLeafHandle_t*>(node);
}

NIF_TerrainCellHandle NIF_TerrainCellLeaf_AsTerrainCell(NIF_TerrainCellLeafHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateTerrainCellHandle(nativeNode) : nullptr;
}

int NIF_Object_AsTerrainSector(NIF_ObjectHandle object, NIF_TerrainSectorHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateTerrainSectorHandle, "NiTerrainSector");
}

void NIF_TerrainSector_Destroy(NIF_TerrainSectorHandle node)
{
	delete static_cast<NIF_TerrainSectorHandle_t*>(node);
}

NIF_NodeHandle NIF_TerrainSector_AsNode(NIF_TerrainSectorHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsAtmosphere(NIF_ObjectHandle object, NIF_AtmosphereHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateAtmosphereHandle, "NiAtmosphere");
}

void NIF_Atmosphere_Destroy(NIF_AtmosphereHandle node)
{
	delete static_cast<NIF_AtmosphereHandle_t*>(node);
}

NIF_NodeHandle NIF_Atmosphere_AsNode(NIF_AtmosphereHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsEnvironment(NIF_ObjectHandle object, NIF_EnvironmentHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateEnvironmentHandle, "NiEnvironment");
}

void NIF_Environment_Destroy(NIF_EnvironmentHandle node)
{
	delete static_cast<NIF_EnvironmentHandle_t*>(node);
}

NIF_NodeHandle NIF_Environment_AsNode(NIF_EnvironmentHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsSky(NIF_ObjectHandle object, NIF_SkyHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateSkyHandle, "NiSky");
}

void NIF_Sky_Destroy(NIF_SkyHandle node)
{
	delete static_cast<NIF_SkyHandle_t*>(node);
}

NIF_NodeHandle NIF_Sky_AsNode(NIF_SkyHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsSkyDome(NIF_ObjectHandle object, NIF_SkyDomeHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateSkyDomeHandle, "NiSkyDome");
}

void NIF_SkyDome_Destroy(NIF_SkyDomeHandle node)
{
	delete static_cast<NIF_SkyDomeHandle_t*>(node);
}

NIF_SkyHandle NIF_SkyDome_AsSky(NIF_SkyDomeHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateSkyHandle(nativeNode) : nullptr;
}

int NIF_Object_AsDecorationField(NIF_ObjectHandle object, NIF_DecorationFieldHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateDecorationFieldHandle, "NiDecorationField");
}

void NIF_DecorationField_Destroy(NIF_DecorationFieldHandle node)
{
	delete static_cast<NIF_DecorationFieldHandle_t*>(node);
}

NIF_NodeHandle NIF_DecorationField_AsNode(NIF_DecorationFieldHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsDecorationLayer(NIF_ObjectHandle object, NIF_DecorationLayerHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateDecorationLayerHandle, "NiDecorationLayer");
}

void NIF_DecorationLayer_Destroy(NIF_DecorationLayerHandle node)
{
	delete static_cast<NIF_DecorationLayerHandle_t*>(node);
}

NIF_NodeHandle NIF_DecorationLayer_AsNode(NIF_DecorationLayerHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

int NIF_Object_AsDecorationPlane(NIF_ObjectHandle object, NIF_DecorationPlaneHandle* outNode)
{
	return NIF_CastNamedNode(object, outNode, &NIF_CreateDecorationPlaneHandle, "NiDecorationPlane");
}

void NIF_DecorationPlane_Destroy(NIF_DecorationPlaneHandle node)
{
	delete static_cast<NIF_DecorationPlaneHandle_t*>(node);
}

NIF_NodeHandle NIF_DecorationPlane_AsNode(NIF_DecorationPlaneHandle node)
{
	NiNode* nativeNode = NIF_GetSpecializedNode(node);
	return nativeNode ? NIF_CreateNodeHandle(nativeNode) : nullptr;
}

}
