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

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

#include "Ni2DRenderView.h"

NiImplementRTTI(Ni2DRenderView, NiRenderView);

//--------------------------------------------------------------------------------------------------
void Ni2DRenderView::SetCameraData(const NiRect<float>&)
{
    // [deprecated]
    EE_FAIL("Ni2DRenderView::CalculatePVGeometry() has been deprecated.");
}

//--------------------------------------------------------------------------------------------------
void Ni2DRenderView::CalculatePVGeometry()
{
    // [deprecated]
    EE_FAIL("Ni2DRenderView::CalculatePVGeometry() has been deprecated.");
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
