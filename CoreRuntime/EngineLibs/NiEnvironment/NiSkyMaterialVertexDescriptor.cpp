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

#include "NiEnvironmentPCH.h"

#include "NiSkyMaterialVertexDescriptor.h"
#include "NiSkyMaterial.h"

//---------------------------------------------------------------------------
NiString NiSkyMaterialVertexDescriptor::ToString()
{
    NiString kResult;

    ToStringATMOSPHERE_CALC_MODE(kResult, false);

    return kResult;
}

//---------------------------------------------------------------------------
bool NiSkyMaterialVertexDescriptor::OutputNormals()
{
    bool bResult = false;

    bResult |= GetUSES_NORMALS() == 1;

    return bResult;
}

//---------------------------------------------------------------------------
bool NiSkyMaterialVertexDescriptor::OutputWorldView()
{
    bool bResult = false;
    bResult |= GetATMOSPHERE_CALC_MODE() != 
        NiSkyMaterial::AtmosphericCalcMode::NONE;

    return bResult;
}

//---------------------------------------------------------------------------