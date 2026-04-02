// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

// Precompiled Header
#include "NiMainPCH.h"

#include "NiSwitchNode.h"
#include "NiDefaultShadowVisitor.h"

NiImplementRTTI(NiDefaultShadowVisitor, NiShadowVisitor);

//--------------------------------------------------------------------------------------------------
NiDefaultShadowVisitor::~NiDefaultShadowVisitor()
{
}

//--------------------------------------------------------------------------------------------------
NiShadowVisitor* NiDefaultShadowVisitor::CreateDefaultShadowVisitor()
{
    return NiNew NiDefaultShadowVisitor();
}

//--------------------------------------------------------------------------------------------------
bool NiDefaultShadowVisitor::ShouldProcessNodeChildren(NiNode* pkNode)
{
    if (NiIsKindOf(NiSwitchNode, pkNode))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------