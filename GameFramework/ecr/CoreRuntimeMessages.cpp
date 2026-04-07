// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "ecrPCH.h"
#include "CoreRuntimeMessages.h"

EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::SceneGraphMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::SceneGraphAddedMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::SceneGraphRemovedMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::SceneGraphsUpdatedMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::AttachmentMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::AttachmentMadeMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::AttachmentBrokenMessage);

//------------------------------------------------------------------------------------------------
void ecr::SceneGraphMessage::Serialize(efd::Archive&)
{
    EE_FAIL("Message only valid for local delivery");
}

//------------------------------------------------------------------------------------------------
void ecr::AttachmentMessage::Serialize(efd::Archive&)
{
    EE_FAIL("Message only valid for local delivery");
}

//------------------------------------------------------------------------------------------------
