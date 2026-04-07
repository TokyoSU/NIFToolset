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

#include "NiSkyMaterialPixelDescriptor.h"
#include "NiSkyMaterial.h"

typedef NiSkyMaterial::ColorMap ColorMap;
typedef NiSkyMaterial::ModifierSource ModifierSource;
typedef NiSkyMaterial::BlendMethod BlendMethod;

//---------------------------------------------------------------------------
NiString NiSkyMaterialPixelDescriptor::ToString()
{
    NiString kResult;

    ToStringATMOSPHERE_CALC_MODE(kResult, false);    
    
    return kResult;
}

//---------------------------------------------------------------------------
bool NiSkyMaterialPixelDescriptor::InputNormals()
{
    bool bResult = false;

    bResult |= GetSTAGE0_COLORMAP_SELECTION() == ColorMap::SKYBOX;
    bResult |= GetSTAGE1_COLORMAP_SELECTION() == ColorMap::SKYBOX;
    bResult |= GetSTAGE2_COLORMAP_SELECTION() == ColorMap::SKYBOX;
    bResult |= GetSTAGE3_COLORMAP_SELECTION() == ColorMap::SKYBOX;
    bResult |= GetSTAGE4_COLORMAP_SELECTION() == ColorMap::SKYBOX;
    bResult |= GetSTAGE0_MODIFIER_SELECTION() == ModifierSource::HORIZONBIAS;
    bResult |= GetSTAGE1_MODIFIER_SELECTION() == ModifierSource::HORIZONBIAS;
    bResult |= GetSTAGE2_MODIFIER_SELECTION() == ModifierSource::HORIZONBIAS;
    bResult |= GetSTAGE3_MODIFIER_SELECTION() == ModifierSource::HORIZONBIAS;
    bResult |= GetSTAGE4_MODIFIER_SELECTION() == ModifierSource::HORIZONBIAS;
    bResult |= GetSTAGE0_COLORMAP_SELECTION() == ColorMap::GRADIENT;
    bResult |= GetSTAGE1_COLORMAP_SELECTION() == ColorMap::GRADIENT;
    bResult |= GetSTAGE2_COLORMAP_SELECTION() == ColorMap::GRADIENT;
    bResult |= GetSTAGE3_COLORMAP_SELECTION() == ColorMap::GRADIENT;
    bResult |= GetSTAGE4_COLORMAP_SELECTION() == ColorMap::GRADIENT;

    return bResult;
}

//---------------------------------------------------------------------------
bool NiSkyMaterialPixelDescriptor::InputWorldView()
{
    bool bResult = false;
    bResult |= GetATMOSPHERE_CALC_MODE() != 
        NiSkyMaterial::AtmosphericCalcMode::NONE;

    return bResult;
}

//---------------------------------------------------------------------------
void NiSkyMaterialPixelDescriptor::GetStageConfiguration(NiUInt32 uiStageIndex, 
    NiUInt32& kColorMap, NiUInt32& kModifierSource, NiUInt32& kBlendMethod)
{
    switch (uiStageIndex)
    {
    case 0:
        kModifierSource = GetSTAGE0_MODIFIER_SELECTION();
        kColorMap = GetSTAGE0_COLORMAP_SELECTION();
        kBlendMethod = GetSTAGE0_BLENDMODE_SELECTION();
        break;
    case 1:
        kModifierSource = GetSTAGE1_MODIFIER_SELECTION();
        kColorMap = GetSTAGE1_COLORMAP_SELECTION();
        kBlendMethod = GetSTAGE1_BLENDMODE_SELECTION();
        break;
    case 2:
        kModifierSource = GetSTAGE2_MODIFIER_SELECTION();
        kColorMap = GetSTAGE2_COLORMAP_SELECTION();
        kBlendMethod = GetSTAGE2_BLENDMODE_SELECTION();
        break;
    case 3:
        kModifierSource = GetSTAGE3_MODIFIER_SELECTION();
        kColorMap = GetSTAGE3_COLORMAP_SELECTION();
        kBlendMethod = GetSTAGE3_BLENDMODE_SELECTION();
        break;
    case 4:
        kModifierSource = GetSTAGE4_MODIFIER_SELECTION();
        kColorMap = GetSTAGE4_COLORMAP_SELECTION();
        kBlendMethod = GetSTAGE4_BLENDMODE_SELECTION();
        break;
    default:
        kModifierSource = ModifierSource::DISABLED;
        kColorMap = ColorMap::CUSTOM;
        kBlendMethod = BlendMethod::CUSTOM;

        EE_FAIL("Requested config of an unsupported blend stage");
    }
}

//---------------------------------------------------------------------------