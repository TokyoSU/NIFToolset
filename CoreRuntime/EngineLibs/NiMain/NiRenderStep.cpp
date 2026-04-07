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

// Precompiled Header
#include "NiMainPCH.h"

#include "NiRenderer.h"
#include "NiRenderStep.h"

NiImplementRootRTTI(NiRenderStep);

NiFixedString NiRenderStep::ms_kDefaultName;

//--------------------------------------------------------------------------------------------------
void NiRenderStep::ReleaseCaches()
{
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiRenderStep::GetFrameID()
{
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    return pkRenderer->GetFrameID();
}

//--------------------------------------------------------------------------------------------------