#pragma once
#ifndef NIF_NATIVE_PORTAL_H
#define NIF_NATIVE_PORTAL_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY void NIF_Portal_Destroy(NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY NIF_PortalHandle NIF_Portal_Create(unsigned int vertexCount, const NIF_Vec3* vertices, NIF_AVObjectHandle adjoiner, int active);
NIFTOOLSET_NATIVE_ENTRY int NIF_Portal_AsAVObject(NIF_PortalHandle portal, NIF_AVObjectHandle* outObject);
NIFTOOLSET_NATIVE_ENTRY void NIF_Portal_SetAdjoiner(NIF_PortalHandle portal, NIF_AVObjectHandle adjoiner);
NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_Portal_GetAdjoiner(NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY void NIF_Portal_SetActive(NIF_PortalHandle portal, int active);
NIFTOOLSET_NATIVE_ENTRY int NIF_Portal_GetActive(NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Portal_GetVertexCount(NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY int NIF_Portal_GetVertex(NIF_PortalHandle portal, unsigned int vertexIndex, NIF_Vec3* vertex);
NIFTOOLSET_NATIVE_ENTRY int NIF_Portal_SetVertex(NIF_PortalHandle portal, unsigned int vertexIndex, NIF_Vec3 vertex);
NIFTOOLSET_NATIVE_ENTRY void NIF_Portal_SetModelBound(NIF_PortalHandle portal, NIF_Bound bound);
NIFTOOLSET_NATIVE_ENTRY NIF_Bound NIF_Portal_GetModelBound(NIF_PortalHandle portal);

NIFTOOLSET_NATIVE_ENTRY void NIF_Room_Destroy(NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomHandle NIF_Room_Create(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_Room_AsNode(NIF_RoomHandle room, NIF_NodeHandle* outNode);
NIFTOOLSET_NATIVE_ENTRY void NIF_Room_AttachOutgoingPortal(NIF_RoomHandle room, NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY int NIF_Room_DetachOutgoingPortal(NIF_RoomHandle room, NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Room_GetOutgoingPortalCount(NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY NIF_PortalHandle NIF_Room_GetOutgoingPortalAt(NIF_RoomHandle room, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY void NIF_Room_AttachIncomingPortal(NIF_RoomHandle room, NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY int NIF_Room_DetachIncomingPortal(NIF_RoomHandle room, NIF_PortalHandle portal);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Room_GetIncomingPortalCount(NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY NIF_PortalHandle NIF_Room_GetIncomingPortalAt(NIF_RoomHandle room, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY void NIF_Room_AttachFixture(NIF_RoomHandle room, NIF_AVObjectHandle fixture);
NIFTOOLSET_NATIVE_ENTRY int NIF_Room_DetachFixture(NIF_RoomHandle room, NIF_AVObjectHandle fixture);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Room_GetFixtureCount(NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_Room_GetFixtureAt(NIF_RoomHandle room, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY int NIF_Room_ContainsPoint(NIF_RoomHandle room, NIF_Vec3 point);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Room_GetLastRenderedTimestamp(NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY void NIF_Room_SetCurrentTimestamp(unsigned int timestamp);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Room_GetCurrentTimestamp(void);

NIFTOOLSET_NATIVE_ENTRY void NIF_RoomGroup_Destroy(NIF_RoomGroupHandle roomGroup);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomGroupHandle NIF_RoomGroup_Create(void);
NIFTOOLSET_NATIVE_ENTRY int NIF_RoomGroup_AsNode(NIF_RoomGroupHandle roomGroup, NIF_NodeHandle* outNode);
NIFTOOLSET_NATIVE_ENTRY void NIF_RoomGroup_AttachShell(NIF_RoomGroupHandle roomGroup, NIF_AVObjectHandle shell);
NIFTOOLSET_NATIVE_ENTRY NIF_AVObjectHandle NIF_RoomGroup_GetShell(NIF_RoomGroupHandle roomGroup);
NIFTOOLSET_NATIVE_ENTRY int NIF_RoomGroup_DetachShell(NIF_RoomGroupHandle roomGroup, NIF_AVObjectHandle* outShell);
NIFTOOLSET_NATIVE_ENTRY void NIF_RoomGroup_AttachRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY int NIF_RoomGroup_DetachRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_RoomGroup_GetRoomCount(NIF_RoomGroupHandle roomGroup);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomHandle NIF_RoomGroup_GetRoomAt(NIF_RoomGroupHandle roomGroup, unsigned int index);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomHandle NIF_RoomGroup_WhichRoom(NIF_RoomGroupHandle roomGroup, NIF_Vec3 point);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomHandle NIF_RoomGroup_WhichRoomFrom(NIF_RoomGroupHandle roomGroup, NIF_Vec3 point, NIF_RoomHandle lastRoom);
NIFTOOLSET_NATIVE_ENTRY NIF_RoomHandle NIF_RoomGroup_GetLastRoom(NIF_RoomGroupHandle roomGroup);
NIFTOOLSET_NATIVE_ENTRY void NIF_RoomGroup_SetLastRoom(NIF_RoomGroupHandle roomGroup, NIF_RoomHandle room);
NIFTOOLSET_NATIVE_ENTRY int NIF_RoomGroup_GetPortallingDisabled(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_RoomGroup_SetPortallingDisabled(int disabled);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_PORTAL_H
