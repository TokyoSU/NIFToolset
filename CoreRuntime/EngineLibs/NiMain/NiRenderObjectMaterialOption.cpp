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

#include "NiRenderObjectMaterialOption.h"

//--------------------------------------------------------------------------------------------------
NiFixedString NiRenderObjectMaterialOption::ms_kStrings[];

//--------------------------------------------------------------------------------------------------
void NiRenderObjectMaterialOption::_SDMInit()
{
    unsigned int uiCount = 1;
    ms_kStrings[E__Invalid] = NULL;

#define Macro(string) ms_kStrings[E_##string] = #string; uiCount++;
    Macro(TRANSFORM_SKINNED);
    Macro(TRANSFORM_INSTANCED);
    Macro(MORPHING);
#undef Macro

    EE_ASSERT(uiCount == E__EnumerationCount);
}

//--------------------------------------------------------------------------------------------------
void NiRenderObjectMaterialOption::_SDMShutdown()
{
    unsigned int uiCount = 1;
    ms_kStrings[E__Invalid] = NULL;

#define Macro(string) ms_kStrings[E_##string] = NULL; uiCount++;
    Macro(TRANSFORM_SKINNED);
    Macro(TRANSFORM_INSTANCED);
    Macro(MORPHING);
#undef Macro

    EE_ASSERT(uiCount == E__EnumerationCount);
}

//--------------------------------------------------------------------------------------------------
