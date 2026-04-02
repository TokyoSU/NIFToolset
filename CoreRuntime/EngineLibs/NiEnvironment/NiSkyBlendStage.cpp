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
#include "NiSkyBlendStage.h"
#include "NiTimeOfDay.h"
#include "NiTTimeOfDayDefaultFunctor.h"

//---------------------------------------------------------------------------
NiImplementRootRTTI(NiSkyBlendStage);
NiImplementRTTI(NiSkyFogBlendStage, NiSkyBlendStage);
NiImplementRTTI(NiSkyGradientBlendStage, NiSkyBlendStage);
NiImplementRTTI(NiSkySkyboxBlendStage, NiSkyBlendStage);

//---------------------------------------------------------------------------
NiSkyBlendStage::NiSkyBlendStage(
    NiSkyMaterial::ColorMap::Value eColorMap) :
    m_eColorMap(eColorMap),
    m_eModifierSource(NiSkyMaterial::ModifierSource::DEFAULT),
    m_eBlendMethod(NiSkyMaterial::BlendMethod::MULTIPLY),
    m_fBlendConstant(0.0f),
    m_fBlendHorizonBias(0.0f),
    m_fBlendBiasExponent(0.0f),
    m_bEnabled(true),
    m_bHasSettingChanged(true)
{
}

//---------------------------------------------------------------------------
NiSkySkyboxBlendStage::NiSkySkyboxBlendStage() :
    NiSkyBlendStage(NiSkyMaterial::ColorMap::SKYBOX),
    m_kOrientation(NiMatrix3::IDENTITY),
    m_spTexture(NULL)
{
    SetModifierSource(NiSkyMaterial::ModifierSource::CONSTANT);
    SetBlendMethod(NiSkyMaterial::BlendMethod::INTERPOLATE);
}

//---------------------------------------------------------------------------
NiSkyFogBlendStage::NiSkyFogBlendStage() :
    NiSkyBlendStage(NiSkyMaterial::ColorMap::FOG)
{
    SetBlendConstant(1.0f);
    SetModifierSource(NiSkyMaterial::ModifierSource::HORIZONBIAS);
    SetBlendMethod(NiSkyMaterial::BlendMethod::INTERPOLATE);
    SetBlendBiasExponent(20.0f);
    SetBlendHorizonBias(1.0f);
}

//---------------------------------------------------------------------------
NiSkyGradientBlendStage::NiSkyGradientBlendStage() :
    NiSkyBlendStage(NiSkyMaterial::ColorMap::GRADIENT),
    m_kZenithColor(NiColorA(0.0f, 0.0f, 0.1f, 1.0f)),
    m_kHorizonColor(NiColorA(0.0f, 0.0f, 0.1f, 1.0f)),
    m_fGradientHorizonBias(0.0f),
    m_fGradientBiasExponent(3.0f)
{
    SetModifierSource(NiSkyMaterial::ModifierSource::CONSTANT);
    SetBlendMethod(NiSkyMaterial::BlendMethod::ADD);
}

//---------------------------------------------------------------------------
void NiSkyBlendStage::ConfigureObject(NiAVObject* pkObject, 
    NiUInt32 uiStage, NiSkyMaterial* pkMaterial)
{
    pkMaterial->SetBlendStageConfiguration(pkObject, 
        uiStage, GetColorMapType(), GetModifierSource(), GetBlendMethod());

    switch (GetModifierSource())
    {
    case NiSkyMaterial::ModifierSource::CONSTANT:
        pkMaterial->SetModifierValue(pkObject, uiStage, GetBlendConstant());
        break;

    case NiSkyMaterial::ModifierSource::HORIZONBIAS:
        pkMaterial->SetModifierHorizonBiasValues(pkObject, 
            uiStage, m_fBlendBiasExponent, m_fBlendHorizonBias);
        break;

    default:
        break;
    }

    MarkPropertyChanged(false);
}

//---------------------------------------------------------------------------
void NiSkyGradientBlendStage::ConfigureObject(NiAVObject* pkObject, 
    NiUInt32 uiStage, NiSkyMaterial* pkMaterial)
{
    NiSkyBlendStage::ConfigureObject(pkObject, uiStage, pkMaterial);

    pkMaterial->SetGradientValues(pkObject, uiStage, 
        m_fGradientBiasExponent, m_fGradientHorizonBias,
        m_kHorizonColor, m_kZenithColor);
}

//---------------------------------------------------------------------------
void NiSkyFogBlendStage::ConfigureObject(NiAVObject* pkObject, 
    NiUInt32 uiStage, NiSkyMaterial* pkMaterial)
{
    NiSkyBlendStage::ConfigureObject(pkObject, uiStage, pkMaterial);
}

//---------------------------------------------------------------------------
void NiSkySkyboxBlendStage::ConfigureObject(NiAVObject* pkObject, 
    NiUInt32 uiStage, NiSkyMaterial* pkMaterial)
{
    NiSkyBlendStage::ConfigureObject(pkObject, uiStage, pkMaterial);

    // Fetch the texturing property for this object
    NiTexturingProperty* pkTexProp = NiDynamicCast(NiTexturingProperty, 
        pkObject->GetProperty(NiTexturingProperty::GetType()));
    if (!pkTexProp)
    {
        pkTexProp = NiNew NiTexturingProperty();
        pkObject->AttachProperty(pkTexProp);
    }
    EE_ASSERT(pkTexProp);

    // Fetch the shader map for this blend stage
    NiTexturingProperty::ShaderMap* pkMap = pkTexProp->GetShaderMap(uiStage);
    if (!pkMap)
    {
        pkMap = NiNew NiTexturingProperty::ShaderMap(m_spTexture, uiStage);
        pkTexProp->SetShaderMap(uiStage, pkMap);
    }
    else
        pkMap->SetTexture(m_spTexture);

    // Update the orientation of the skybox
    if (IsOriented())
    {
        pkMaterial->SetOrientedSkyboxValues(pkObject, uiStage, m_kOrientation);
    }

    // Update the object properties
    pkObject->UpdateProperties();
}

//---------------------------------------------------------------------------
NiFixedString NiSkyBlendStage::CreatePropertyName(
    const NiFixedString& kCategory, const NiFixedString& kName, 
    NiUInt32 uiStage)
{
    EE_ASSERT(NiSkyMaterial::NUM_BLEND_STAGES <= 9);

    NiUInt32 uiStrLen = kCategory.GetLength() + 1 + kName.GetLength() + 7;
    char* pcNewString = NiAlloc(char, uiStrLen + 1);

    NiSprintf(pcNewString, uiStrLen + 1, "%s.Stage%d.%s", 
        (const char*)kCategory, uiStage, (const char*)kName);

    NiFixedString kReturn = NiFixedString(pcNewString);
    NiFree(pcNewString);

    return kReturn;
}

//---------------------------------------------------------------------------
void NiSkyBlendStage::BindProperties(NiUInt32 uiStage, NiTimeOfDay* pkTimeOfDay)
{
    efd::map<efd::utf8string, efd::utf8string> kPropMap;

    efd::utf8string kTemp = 
        (const char*)CreatePropertyName("Environment", "BlendConstant", uiStage);
    NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float> kFloatFunctor = 
        NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>(this, 
        &NiSkyBlendStage::SetBlendConstant);
    pkTimeOfDay->BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetBlendConstant());

    kTemp = (const char*)CreatePropertyName("Environment", "BlendHorizonBias", uiStage);
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>(this, 
        &NiSkyBlendStage::SetBlendHorizonBias);
    pkTimeOfDay->BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetBlendHorizonBias());

    kTemp = (const char*)CreatePropertyName("Environment", "BlendBiasExponent", uiStage);
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>(this, 
        &NiSkyBlendStage::SetBlendBiasExponent);
    pkTimeOfDay->BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyBlendStage, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetBlendBiasExponent());

    // Update the initial keyframes    
    efd::vector<float> kTimes; 
    kTimes.push_back(0.0f);
    kTimes.push_back(86400.0f);
    efd::vector<efd::utf8string> kValues;

    efd::map<efd::utf8string, efd::utf8string>::iterator kIter = kPropMap.begin();

    while (kIter != kPropMap.end())
    {
        kValues.clear();
        kValues.push_back(kIter->second);
        kValues.push_back(kIter->second);
        pkTimeOfDay->UpdatePropertySequence(kIter->first.c_str(),2, kTimes, kValues);
        ++kIter;
    }
}

//--------------------------------------------------------------------------------------------------
void NiSkyBlendStage::RemoveProperties(NiUInt32 uiStage, NiTimeOfDay* pkTimeOfDay)
{
    efd::vector<efd::utf8string> kToRemoveList;
    

    efd::utf8string kTemp = (const char*)CreatePropertyName("Environment", "BlendConstant", uiStage);
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)CreatePropertyName("Environment", "BlendHorizonBias", uiStage);
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)CreatePropertyName("Environment", "BlendBiasExponent", uiStage);
    kToRemoveList.push_back(kTemp);

    pkTimeOfDay->RemoveToDProperties(kToRemoveList);
}

//---------------------------------------------------------------------------
void NiSkyGradientBlendStage::BindProperties(NiUInt32 uiStage, 
    NiTimeOfDay* pkTimeOfDay)
{
    NiSkyBlendStage::BindProperties(uiStage, pkTimeOfDay);

    efd::map<efd::utf8string, efd::utf8string> kPropMap;

    efd::utf8string kTemp = 
        (const char*)CreatePropertyName("Environment", "ZenithColor", uiStage);
    NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, NiColorA> kColorFunctor = 
        NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, NiColorA>(this, 
        &NiSkyGradientBlendStage::SetZenithColor);
    pkTimeOfDay->BindProperty<
        NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, NiColorA>, NiColorA>(
        kTemp.c_str(), kColorFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetZenithColor());

    kTemp = (const char*)CreatePropertyName("Environment", "HorizonColor", uiStage);
    kColorFunctor = NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, NiColorA>(this, 
        &NiSkyGradientBlendStage::SetHorizonColor);
    pkTimeOfDay->BindProperty<
        NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, NiColorA>, NiColorA>(
        kTemp.c_str(), kColorFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetHorizonColor());

    kTemp = (const char*)CreatePropertyName("Environment", "GradientHorizonBias", uiStage);
    NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, float> kFloatFunctor = 
        NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, float>(this, 
        &NiSkyGradientBlendStage::SetGradientHorizonBias);
    pkTimeOfDay->BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetGradientHorizonBias());

    kTemp = (const char*)CreatePropertyName("Environment", "GradientBiasExponent", uiStage);
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, float>(this, 
        &NiSkyGradientBlendStage::SetGradientBiasExponent);
    pkTimeOfDay->BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyGradientBlendStage, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = pkTimeOfDay->ConvertValueToString(GetGradientBiasExponent());

    // Update the initial keyframes    
    efd::vector<float> kTimes; 
    kTimes.push_back(0.0f);
    kTimes.push_back(86400.0f);
    efd::vector<efd::utf8string> kValues;

    efd::map<efd::utf8string, efd::utf8string>::iterator kIter = kPropMap.begin();

    while (kIter != kPropMap.end())
    {
        kValues.clear();
        kValues.push_back(kIter->second);
        kValues.push_back(kIter->second);
        pkTimeOfDay->UpdatePropertySequence(kIter->first.c_str(), 2, kTimes, kValues);
        ++kIter;
    }

}

//--------------------------------------------------------------------------------------------------
void NiSkyGradientBlendStage::RemoveProperties(NiUInt32 uiStage, NiTimeOfDay* pkTimeOfDay)
{
    efd::vector<efd::utf8string> kToRemoveList;

    efd::utf8string kTemp = (const char*)CreatePropertyName("Environment", "ZenithColor", uiStage);
    kToRemoveList.push_back(kTemp);

    kTemp = (const char*)CreatePropertyName("Environment", "HorizonColor", uiStage);
    kToRemoveList.push_back(kTemp);

    kTemp = (const char*)CreatePropertyName("Environment", "GradientHorizonBias", uiStage);
    kToRemoveList.push_back(kTemp);

    kTemp = (const char*)CreatePropertyName("Environment", "GradientBiasExponent", uiStage);
    kToRemoveList.push_back(kTemp);

    pkTimeOfDay->RemoveToDProperties(kToRemoveList);
}

//---------------------------------------------------------------------------
void NiSkySkyboxBlendStage::BindProperties(NiUInt32 uiStage, 
    NiTimeOfDay* pkTimeOfDay)
{
    NiSkyBlendStage::BindProperties(uiStage, pkTimeOfDay);
}

//---------------------------------------------------------------------------
void NiSkySkyboxBlendStage::RemoveProperties(NiUInt32 uiStage, NiTimeOfDay* pkTimeOfDay)
{
    NiSkyBlendStage::RemoveProperties(uiStage, pkTimeOfDay);
}
