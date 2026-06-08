#include "NIF.Native.Portal.h"
#include "NIF.Native.Internal.h"

#include <NiBound.h>
#include <NiPortal.h>
#include <NiRoom.h>
#include <NiRoomGroup.h>

namespace
{
	template <typename TList, typename TItem>
	unsigned int NIF_GetListCount(const TList& list)
	{
		unsigned int count = 0;
		NiTListIterator pos = list.GetHeadPos();
		while (pos)
		{
			list.GetNext(pos);
			++count;
		}
		return count;
	}

	template <typename TList, typename TItem>
	TItem NIF_GetListItemAt(const TList& list, unsigned int index)
	{
		unsigned int currentIndex = 0;
		NiTListIterator pos = list.GetHeadPos();
		while (pos)
		{
			TItem item = list.GetNext(pos);
			if (currentIndex == index)
			{
				return item;
			}

			++currentIndex;
		}

		return nullptr;
	}

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

	NIF_Vec3 NIF_MakeZeroVec3()
	{
		return NIF_Vec3{ 0.0f, 0.0f, 0.0f };
	}

	NiPortal* NIF_GetPortal(NIF_PortalHandle portal)
	{
		NIF_PortalHandle_t* pHandle = static_cast<NIF_PortalHandle_t*>(portal);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRoom* NIF_GetRoom(NIF_RoomHandle room)
	{
		NIF_RoomHandle_t* pHandle = static_cast<NIF_RoomHandle_t*>(room);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRoomGroup* NIF_GetRoomGroup(NIF_RoomGroupHandle roomGroup)
	{
		NIF_RoomGroupHandle_t* pHandle = static_cast<NIF_RoomGroupHandle_t*>(roomGroup);
		return pHandle ? pHandle->spObject : nullptr;
	}
}

NIF_PortalHandle NIF_CreatePortalHandle(NiPortal* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_PortalHandle_t* pHandle = new NIF_PortalHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RoomGroupHandle NIF_CreateRoomGroupHandle(NiRoomGroup* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RoomGroupHandle_t* pHandle = new NIF_RoomGroupHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RoomHandle NIF_CreateRoomHandle(NiRoom* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RoomHandle_t* pHandle = new NIF_RoomHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

extern "C"
{

void NIF_Portal_Destroy(NIF_PortalHandle portal)
{
	delete static_cast<NIF_PortalHandle_t*>(portal);
}

NIF_PortalHandle NIF_Portal_Create(unsigned int vertexCount, const NIF_Vec3* vertices, NIF_AVObjectHandle adjoiner, int active)
{
	NIF_AVObjectHandle_t* pAdjoinerHandle = static_cast<NIF_AVObjectHandle_t*>(adjoiner);
	if (vertexCount > 0xFFFFu || (vertexCount > 0 && !vertices))
	{
		return nullptr;
	}

	NiPoint3 localVertices[32];
	NiPoint3* pVertices = nullptr;
	if (vertexCount > 0)
		{
		if (vertexCount <= 32)
		{
			pVertices = localVertices;
		}
		else
		{
			pVertices = NiNew NiPoint3[vertexCount];
		}

		for (unsigned int i = 0; i < vertexCount; ++i)
		{
			pVertices[i] = NIF_MakePoint3(vertices[i]);
		}
	}

	NiPortal* pkPortal = NiNew NiPortal(static_cast<unsigned short>(vertexCount), pVertices, pAdjoinerHandle ? pAdjoinerHandle->spObject : nullptr, active != 0);
	if (pVertices && pVertices != localVertices)
	{
		NiDelete[] pVertices;
	}

	return NIF_CreatePortalHandle(pkPortal);
}

int NIF_Portal_AsAVObject(NIF_PortalHandle portal, NIF_AVObjectHandle* outObject)
{
	if (outObject)
	{
		*outObject = nullptr;
	}

	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (!pkPortal || !outObject)
	{
		return 0;
	}

	*outObject = NIF_CreateAVObjectHandle(pkPortal);
	return *outObject ? 1 : 0;
}

void NIF_Portal_SetAdjoiner(NIF_PortalHandle portal, NIF_AVObjectHandle adjoiner)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	NIF_AVObjectHandle_t* pAdjoinerHandle = static_cast<NIF_AVObjectHandle_t*>(adjoiner);
	if (pkPortal)
	{
		pkPortal->SetAdjoiner(pAdjoinerHandle ? pAdjoinerHandle->spObject : nullptr);
	}
}

NIF_AVObjectHandle NIF_Portal_GetAdjoiner(NIF_PortalHandle portal)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return pkPortal ? NIF_CreateAVObjectHandle(pkPortal->GetAdjoiner()) : nullptr;
}

void NIF_Portal_SetActive(NIF_PortalHandle portal, int active)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (pkPortal)
	{
		pkPortal->SetActive(active != 0);
	}
}

int NIF_Portal_GetActive(NIF_PortalHandle portal)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return (pkPortal && pkPortal->GetActive()) ? 1 : 0;
}

unsigned int NIF_Portal_GetVertexCount(NIF_PortalHandle portal)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return pkPortal ? pkPortal->GetVertexCount() : 0;
}

int NIF_Portal_GetVertex(NIF_PortalHandle portal, unsigned int vertexIndex, NIF_Vec3* vertex)
{
	if (vertex)
	{
		*vertex = NIF_MakeZeroVec3();
	}

	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (!pkPortal || !vertex || vertexIndex >= pkPortal->GetVertexCount())
	{
		return 0;
	}

	const NiPoint3* pVertices = pkPortal->GetVertices();
	if (!pVertices)
	{
		return 0;
	}

	*vertex = NIF_MakeVec3(pVertices[vertexIndex]);
	return 1;
}

int NIF_Portal_SetVertex(NIF_PortalHandle portal, unsigned int vertexIndex, NIF_Vec3 vertex)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (!pkPortal || vertexIndex >= pkPortal->GetVertexCount())
	{
		return 0;
	}

	NiPoint3* pVertices = pkPortal->GetVertices();
	if (!pVertices)
	{
		return 0;
	}

	pVertices[vertexIndex] = NIF_MakePoint3(vertex);
	pkPortal->SetModelBound(NiBound());
	pkPortal->GetModelBound().ComputeFromData(static_cast<int>(pkPortal->GetVertexCount()), pkPortal->GetVertices());
	return 1;
}

void NIF_Portal_SetModelBound(NIF_PortalHandle portal, NIF_Bound bound)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (pkPortal)
	{
		pkPortal->SetModelBound(NIF_MakeBound(bound));
	}
}

NIF_Bound NIF_Portal_GetModelBound(NIF_PortalHandle portal)
{
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return pkPortal ? NIF_MakeBound(pkPortal->GetModelBound()) : NIF_Bound{ NIF_MakeZeroVec3(), 0.0f };
}

void NIF_Room_Destroy(NIF_RoomHandle room)
{
	delete static_cast<NIF_RoomHandle_t*>(room);
}

NIF_RoomHandle NIF_Room_Create(void)
{
	return NIF_CreateRoomHandle(NiNew NiRoom());
}

int NIF_Room_AsNode(NIF_RoomHandle room, NIF_NodeHandle* outNode)
{
	if (outNode)
	{
		*outNode = nullptr;
	}

	NiRoom* pkRoom = NIF_GetRoom(room);
	if (!pkRoom || !outNode)
	{
		return 0;
	}

	*outNode = NIF_CreateNodeHandle(pkRoom);
	return *outNode ? 1 : 0;
}

void NIF_Room_AttachOutgoingPortal(NIF_RoomHandle room, NIF_PortalHandle portal)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (pkRoom && pkPortal)
	{
		pkRoom->AttachOutgoingPortal(pkPortal);
	}
}

int NIF_Room_DetachOutgoingPortal(NIF_RoomHandle room, NIF_PortalHandle portal)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return (pkRoom && pkPortal && pkRoom->DetachOutgoingPortal(pkPortal)) ? 1 : 0;
}

unsigned int NIF_Room_GetOutgoingPortalCount(NIF_RoomHandle room)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	return pkRoom ? NIF_GetListCount<NiPortalList, NiPortal*>(pkRoom->GetOutgoingPortalList()) : 0;
}

NIF_PortalHandle NIF_Room_GetOutgoingPortalAt(NIF_RoomHandle room, unsigned int index)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = pkRoom ? NIF_GetListItemAt<NiPortalList, NiPortal*>(pkRoom->GetOutgoingPortalList(), index) : nullptr;
	return NIF_CreatePortalHandle(pkPortal);
}

void NIF_Room_AttachIncomingPortal(NIF_RoomHandle room, NIF_PortalHandle portal)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = NIF_GetPortal(portal);
	if (pkRoom && pkPortal)
	{
		pkRoom->AttachIncomingPortal(pkPortal);
	}
}

int NIF_Room_DetachIncomingPortal(NIF_RoomHandle room, NIF_PortalHandle portal)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = NIF_GetPortal(portal);
	return (pkRoom && pkPortal && pkRoom->DetachIncomingPortal(pkPortal)) ? 1 : 0;
}

unsigned int NIF_Room_GetIncomingPortalCount(NIF_RoomHandle room)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	return pkRoom ? NIF_GetListCount<NiPortalList, NiPortal*>(pkRoom->GetIncomingPortalList()) : 0;
}

NIF_PortalHandle NIF_Room_GetIncomingPortalAt(NIF_RoomHandle room, unsigned int index)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiPortal* pkPortal = pkRoom ? NIF_GetListItemAt<NiPortalList, NiPortal*>(pkRoom->GetIncomingPortalList(), index) : nullptr;
	return NIF_CreatePortalHandle(pkPortal);
}

void NIF_Room_AttachFixture(NIF_RoomHandle room, NIF_AVObjectHandle fixture)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NIF_AVObjectHandle_t* pFixtureHandle = static_cast<NIF_AVObjectHandle_t*>(fixture);
	if (pkRoom && pFixtureHandle && pFixtureHandle->spObject)
	{
		pkRoom->AttachFixture(pFixtureHandle->spObject);
	}
}

int NIF_Room_DetachFixture(NIF_RoomHandle room, NIF_AVObjectHandle fixture)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NIF_AVObjectHandle_t* pFixtureHandle = static_cast<NIF_AVObjectHandle_t*>(fixture);
	return (pkRoom && pFixtureHandle && pFixtureHandle->spObject && pkRoom->DetachFixture(pFixtureHandle->spObject)) ? 1 : 0;
}

unsigned int NIF_Room_GetFixtureCount(NIF_RoomHandle room)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	return pkRoom ? NIF_GetListCount<NiAVObjectList, NiAVObject*>(pkRoom->GetFixtureList()) : 0;
}

NIF_AVObjectHandle NIF_Room_GetFixtureAt(NIF_RoomHandle room, unsigned int index)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	NiAVObject* pkFixture = pkRoom ? NIF_GetListItemAt<NiAVObjectList, NiAVObject*>(pkRoom->GetFixtureList(), index) : nullptr;
	return NIF_CreateAVObjectHandle(pkFixture);
}

int NIF_Room_ContainsPoint(NIF_RoomHandle room, NIF_Vec3 point)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	return (pkRoom && pkRoom->ContainsPoint(NIF_MakePoint3(point))) ? 1 : 0;
}

unsigned int NIF_Room_GetLastRenderedTimestamp(NIF_RoomHandle room)
{
	NiRoom* pkRoom = NIF_GetRoom(room);
	return pkRoom ? pkRoom->GetLastRenderedTimestamp() : 0;
}

void NIF_Room_SetCurrentTimestamp(unsigned int timestamp)
{
	NiRoom::SetCurrentTimestamp(timestamp);
}

unsigned int NIF_Room_GetCurrentTimestamp(void)
{
	return NiRoom::GetCurrentTimestamp();
}

void NIF_RoomGroup_Destroy(NIF_RoomGroupHandle roomGroup)
{
	delete static_cast<NIF_RoomGroupHandle_t*>(roomGroup);
}

NIF_RoomGroupHandle NIF_RoomGroup_Create(void)
{
	return NIF_CreateRoomGroupHandle(NiNew NiRoomGroup());
}

int NIF_RoomGroup_AsNode(NIF_RoomGroupHandle roomGroup, NIF_NodeHandle* outNode)
{
	if (outNode)
	{
		*outNode = nullptr;
	}

	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	if (!pkRoomGroup || !outNode)
	{
		return 0;
	}

	*outNode = NIF_CreateNodeHandle(pkRoomGroup);
	return *outNode ? 1 : 0;
}

void NIF_RoomGroup_AttachShell(NIF_RoomGroupHandle roomGroup, NIF_AVObjectHandle shell)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NIF_AVObjectHandle_t* pShellHandle = static_cast<NIF_AVObjectHandle_t*>(shell);
	if (pkRoomGroup)
	{
		pkRoomGroup->AttachShell(pShellHandle ? pShellHandle->spObject : nullptr);
	}
}

NIF_AVObjectHandle NIF_RoomGroup_GetShell(NIF_RoomGroupHandle roomGroup)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	return pkRoomGroup ? NIF_CreateAVObjectHandle(pkRoomGroup->GetShell()) : nullptr;
}

int NIF_RoomGroup_DetachShell(NIF_RoomGroupHandle roomGroup, NIF_AVObjectHandle* outShell)
{
	if (outShell)
	{
		*outShell = nullptr;
	}

	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	if (!pkRoomGroup || !outShell)
	{
		return 0;
	}

	*outShell = NIF_CreateAVObjectHandle(pkRoomGroup->DetachShell());
	return *outShell ? 1 : 0;
}

void NIF_RoomGroup_AttachRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NiRoom* pkRoom = NIF_GetRoom(room);
	if (pkRoomGroup && pkRoom)
	{
		pkRoomGroup->AttachRoom(pkRoom);
	}
}

int NIF_RoomGroup_DetachRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NiRoom* pkRoom = NIF_GetRoom(room);
	return (pkRoomGroup && pkRoom && pkRoomGroup->DetachRoom(pkRoom)) ? 1 : 0;
}

unsigned int NIF_RoomGroup_GetRoomCount(NIF_RoomGroupHandle roomGroup)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	return pkRoomGroup ? NIF_GetListCount<NiRoomList, NiRoom*>(pkRoomGroup->GetRoomList()) : 0;
}

NIF_RoomHandle NIF_RoomGroup_GetRoomAt(NIF_RoomGroupHandle roomGroup, unsigned int index)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NiRoom* pkRoom = pkRoomGroup ? NIF_GetListItemAt<NiRoomList, NiRoom*>(pkRoomGroup->GetRoomList(), index) : nullptr;
	return NIF_CreateRoomHandle(pkRoom);
}

NIF_RoomHandle NIF_RoomGroup_WhichRoom(NIF_RoomGroupHandle roomGroup, NIF_Vec3 point)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	return pkRoomGroup ? NIF_CreateRoomHandle(pkRoomGroup->WhichRoom(NIF_MakePoint3(point))) : nullptr;
}

NIF_RoomHandle NIF_RoomGroup_WhichRoomFrom(NIF_RoomGroupHandle roomGroup, NIF_Vec3 point, NIF_RoomHandle lastRoom)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NiRoom* pkLastRoom = NIF_GetRoom(lastRoom);
	return pkRoomGroup ? NIF_CreateRoomHandle(pkRoomGroup->WhichRoom(NIF_MakePoint3(point), pkLastRoom)) : nullptr;
}

NIF_RoomHandle NIF_RoomGroup_GetLastRoom(NIF_RoomGroupHandle roomGroup)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	return pkRoomGroup ? NIF_CreateRoomHandle(pkRoomGroup->GetLastRoom()) : nullptr;
}

void NIF_RoomGroup_SetLastRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room)
{
	NiRoomGroup* pkRoomGroup = NIF_GetRoomGroup(roomGroup);
	NiRoom* pkRoom = NIF_GetRoom(room);
	if (pkRoomGroup)
	{
		pkRoomGroup->SetLastRoom(pkRoom);
	}
}

int NIF_RoomGroup_GetPortallingDisabled(void)
{
	return NiRoomGroup::GetPortallingDisabled() ? 1 : 0;
}

void NIF_RoomGroup_SetPortallingDisabled(int disabled)
{
	NiRoomGroup::SetPortallingDisabled(disabled != 0);
}

}
