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
#include "NiEnvironment.h"
#include "NiTTimeOfDayDefaultFunctor.h"
#include "NiTimeOfDay.h"

using namespace efd;

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::NiTimeOfDay():
    m_fDuration(0.0f),
    m_fTimeMultiplier(1.0f),
    m_bActive(false),
    m_fScaledTime(0.0f),
    m_fLastTime(-NI_INFINITY)
{
}

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::~NiTimeOfDay()
{
    NiTMapIterator kIter = m_kProperties.GetFirstPos();
    while (kIter)
    {
        Property* pkValue = NULL;
        NiFixedString kKey;
        m_kProperties.GetNext(kIter, kKey, pkValue);

        if (pkValue)
        {
            delete pkValue;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::DoUpdate(NiUpdateProcess& kUpdate)
{
    float fTime = kUpdate.GetTime();

    // If Animation is not active, then skip updating the controllers
    if (!m_bActive)
    {
        m_fLastTime = fTime;
        return;
    }

    // Initialise the first update
    if (m_fLastTime == -NI_INFINITY)
        m_fLastTime = fTime;
    
    // Calculate how much time has passed
    float fDeltaTime = fTime - m_fLastTime;
    float fScaledTime = m_fScaledTime + m_fTimeMultiplier * fDeltaTime;

    // Wrap the time within bounds
    if (m_fDuration != 0.0f)
    {
        fScaledTime = NiFmod(fScaledTime, m_fDuration);
        if (fScaledTime < 0.0f)
        {
            fScaledTime += m_fDuration;
        }
    }
    else
    {
        fScaledTime = 0.0f;
    }

    // Store the last time for the records
    m_fLastTime = fTime;
    if (m_fScaledTime == fScaledTime) 
        return;
    m_fScaledTime = fScaledTime; 

    UpdateControllers(m_fScaledTime);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{        
    DoUpdate(kUpdate);
    NiAVObject::UpdateDownwardPass(kUpdate); 
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{    
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }

    NiAVObject::UpdateSelectedDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    } 
    
    NiAVObject::UpdateRigidDownwardPass(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdateControllers(float fTime)
{
    // Iterate over the list of properties to update this frame
    NiUInt32 uiNumControllers = m_kControllers.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiNumControllers; ++uiIndex)
    {
        NiTimeOfDayController* pkController = m_kControllers.GetAt(uiIndex);
        if (pkController)
            pkController->Update(fTime);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdatePropertySequence(NiFixedString kPropertyName, efd::UInt32 uiNumKeyframes,
    efd::vector<float> kTimes, efd::vector<efd::utf8string> kValues)
{
    // Fetch the relevant property
    Property* pkProperty = GetProperty(kPropertyName);
    if (!pkProperty)
    {
        EE_FAIL("TimeOfDay Property not defined yet");
        return;
    }
    
    // Generate a sequence builder
    SequenceBuilder* pkSequenceBuilder = 
        GetSequenceBuilder(pkProperty->m_kType, uiNumKeyframes);
    if (!pkSequenceBuilder)
    {
        EE_FAIL("Unrecognised type for TimeOfDay sequence");
        return;
    }

    // Gather all the keyframes:
    for (efd::UInt32 ui = 0; ui < uiNumKeyframes; ++ui)
    {
        pkSequenceBuilder->SetKey(ui, kTimes.at(ui), kValues.at(ui));
    }

    // Finalise the sequence:
    NiInterpolator *pkInterpolator = pkSequenceBuilder->FinaliseSequence(m_fDuration);
    SetInterpolator(pkProperty, pkInterpolator);
    delete pkSequenceBuilder;

    UpdateControllers(m_fScaledTime);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UpdatePropertySequence(NiFixedString kPropertyName, efd::UInt32 uiNumKeyframes,
    efd::vector<efd::utf8string> kTimes, efd::vector<efd::utf8string> kValues)
{
    if (uiNumKeyframes == 0)
        return;

    // Fetch the relevant property
    Property* pkProperty = GetProperty(kPropertyName);
    if (!pkProperty)
    {
        EE_FAIL("TimeOfDay Property not defined yet");
        return;
    }
    
    // Generate a sequence builder
    SequenceBuilder* pkSequenceBuilder = 
        GetSequenceBuilder(pkProperty->m_kType, uiNumKeyframes);
    if (!pkSequenceBuilder)
    {
        EE_FAIL("Unrecognised type for TimeOfDay sequence");
        return;
    }

    // Gather all the keyframes:
    for (efd::UInt32 ui = 0; ui < uiNumKeyframes; ++ui)
    {
        float fTime = GetSecondsFromString(kTimes.at(ui).c_str());
        pkSequenceBuilder->SetKey(ui, fTime , kValues.at(ui));
    }

    // Finalise the sequence:
    NiInterpolator *pkInterpolator = pkSequenceBuilder->FinaliseSequence(m_fDuration);
    SetInterpolator(pkProperty, pkInterpolator);
    delete pkSequenceBuilder;

    UpdateControllers(m_fScaledTime);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::GetPropertySequence(NiFixedString kPropertyName, 
    efd::UInt32& uiNumKeyframes,
    efd::vector<efd::utf8string>& kTimes, 
    efd::vector<efd::utf8string>& kValues)
{
    efd::UInt32 uiNumKeys = 0;
    NiAnimationKey::KeyType eKeyType;
    unsigned char ucSize;
    efd::utf8string kTimeUTF8;
    efd::utf8string kValueUTF8;

    NiTimeOfDay::Property* pkProp = GetProperty(kPropertyName);

    if (!pkProp)
        return;
    
    if (pkProp->m_kType == NiTimeOfDay::TODPT_COLOR)
    {
        NiColorInterpolator* pkColor = NiDynamicCast(NiColorInterpolator, pkProp->m_spInterpolator);

        if (!pkColor)
            return;

        NiColorData* pkColorData = pkColor->GetColorData();

        if (!pkColorData)
            return;
        
        NiColorKey* pkColorKeys = pkColorData->GetAnim(uiNumKeys, eKeyType, ucSize);
        uiNumKeyframes = uiNumKeys;
        for (efd::UInt32 uiKey = 0; uiKey < uiNumKeys; ++uiKey)
        {
            float fKeyTime = pkColorKeys[uiKey].GetTime();
            NiColorA kKeyValue = pkColorKeys[uiKey].GetColor();
            
            kTimeUTF8 = GetStringsFromSeconds(fKeyTime);
            kValueUTF8 = ConvertValueToString(kKeyValue);

            kTimes.push_back(kTimeUTF8);
            kValues.push_back(kValueUTF8);
        }
    }
    else if (pkProp->m_kType == NiTimeOfDay::TODPT_FLOAT)
    {
        NiFloatInterpolator* pkFloats = 
            NiDynamicCast(NiFloatInterpolator, pkProp->m_spInterpolator);

        if (!pkFloats)
            return;

        NiFloatData* pkFloatData = pkFloats->GetFloatData();

        if (!pkFloatData)
            return;
        
        NiFloatKey* pkFloatKeys = pkFloatData->GetAnim(uiNumKeys, eKeyType, ucSize);
        uiNumKeyframes = uiNumKeys;
        for (efd::UInt32 uiKey = 0; uiKey < uiNumKeys; ++uiKey)
        {
            float fKeyTime = pkFloatKeys[uiKey].GetTime();
            float fKeyValue = pkFloatKeys[uiKey].GetValue();

            kTimeUTF8 = GetStringsFromSeconds(fKeyTime);
            kValueUTF8 = ConvertValueToString(fKeyValue);

            kTimes.push_back(kTimeUTF8);
            kValues.push_back(kValueUTF8);
        }
    }    
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::RemoveToDProperties(efd::vector<efd::utf8string> kToRemoveList)
{
    efd::vector<efd::utf8string>::iterator kIter = kToRemoveList.begin();

    while (kIter != kToRemoveList.end())
    {
        Property* pkProp;
        if (m_kProperties.GetAt(kIter->c_str(), pkProp))
        {
            SetInterpolator(pkProp, NULL);
            m_kProperties.RemoveAt(kIter->c_str());
            NiDelete pkProp;
        }
        
        ++kIter;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::GetPropertyNames(NiTObjectSet<NiFixedString>& kPropertyNames) const
{
    NiTMapIterator kIter = m_kProperties.GetFirstPos();
    while (kIter)
    {
        Property* pkValue = NULL;
        NiFixedString kKey;
        m_kProperties.GetNext(kIter, kKey, pkValue);

        kPropertyNames.Add(kKey);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::GetPropertyNamesAndTypes(NiTObjectSet<NiFixedString>& kPropertyNames, 
    NiTObjectSet<NiFixedString>& kPropertyTypes) const
{
    NiTMapIterator kIter = m_kProperties.GetFirstPos();
    while (kIter)
    {
        Property* pkValue = NULL;
        NiFixedString kKey;
        m_kProperties.GetNext(kIter, kKey, pkValue);

        kPropertyNames.Add(kKey);

        NiFixedString kType = GetStringFromPropertyType(pkValue->m_kType);
        kPropertyTypes.Add(kType);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTimeOfDay::IsPropertyBound(const NiFixedString& kPropertyName) const
{
    Property* pkProperty = NULL;
    m_kProperties.GetAt(kPropertyName, pkProperty);

    if (pkProperty && pkProperty->m_spController)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiTimeOfDay::LoadAnimation(const NiString& kFileName)
{
    bool bParseOK = false;

    // Buffer the XML file for parsing by TinyXML.
    TiXmlDocument kAnimationFile(kFileName);
    File* pkFile = efd::File::GetFile(kAnimationFile.Value(), File::READ_ONLY);
    if (pkFile)
    {
        // Buffer the XML file
        unsigned int uiSize = pkFile->GetFileSize();
        char* pcBuffer = EE_ALLOC(char, uiSize + 1);
        pkFile->Read(pcBuffer, uiSize);
        pcBuffer[uiSize] = '\0';

        // Parse the buffer
        bParseOK = (kAnimationFile.LoadBuffer(pcBuffer) != 0);

        // Free resources
        EE_FREE(pcBuffer);
        EE_DELETE(pkFile);
    }

    if (!bParseOK)
    {
        EE_FAIL("TimeOfDay Animation error: Failed to load animation");
        return false;
    }
    

    // Extract the information from the file:
    TiXmlElement* pkAnimationElement = kAnimationFile.RootElement();
    bParseOK = LoadAnimation(pkAnimationElement);

    if (!bParseOK)
    {
        EE_FAIL("TimeOfDay Animation error: Failed to parse animation");
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTimeOfDay::LoadAnimation(TiXmlElement* pkTODAnimElement)
{
    if (!pkTODAnimElement)
        return false;

    // Perform File Version Check
    NiUInt32 uiVersion = 0;
    NiString kAttribute = pkTODAnimElement->Attribute("version");
    kAttribute.ToUInt(uiVersion);

    if (uiVersion != 1)
        return false;

    // Read configuration information, used to decode the rest of the file
    float fTimeScale = 1.0f;
    float fEndTime = 60.0f;
    TiXmlElement* pkConfigElement = pkTODAnimElement->FirstChildElement("Configuration");
    if (pkConfigElement)
    {
        kAttribute = pkConfigElement->Attribute("timeScale");
        kAttribute.ToFloat(fTimeScale);

        kAttribute = pkConfigElement->Attribute("endTime");
        if (!kAttribute.IsEmpty())
            fEndTime = GetSecondsFromString(kAttribute);
    }
    m_fDuration = fEndTime;
    m_fTimeMultiplier = fTimeScale;

    // Read the list of animation tracks that can be controlled
    TiXmlElement* pkPropertySetElement = pkTODAnimElement->FirstChildElement("PropertySet");
    if (pkPropertySetElement)
    {
        TiXmlElement* pkPropertyElement = pkPropertySetElement->FirstChildElement();
        while (pkPropertyElement)
        {
            LoadPropertyDefinition(pkPropertyElement);
            pkPropertyElement = pkPropertyElement->NextSiblingElement();
        }
    }

    // Read the list of animations
    TiXmlElement* pkAnimationElement = pkTODAnimElement->FirstChildElement("Animation");
    if (pkAnimationElement)
    {
        TiXmlElement* pkSequenceElement = pkAnimationElement->FirstChildElement();
        while (pkSequenceElement)
        {
            LoadSequenceDefinition(pkSequenceElement, fEndTime);
            pkSequenceElement = pkSequenceElement->NextSiblingElement();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::LoadPropertyDefinition(TiXmlElement* pkPropertyElement)
{
    NiString kElementName = pkPropertyElement->Value();
    if (kElementName.Compare("Property") != 0)
        return;

    NiFixedString kName = pkPropertyElement->Attribute("name");
    NiFixedString kTypeString = pkPropertyElement->Attribute("dataType");
    PropertyType kType = GetPropertyTypeFromString(kTypeString);
    
    Property* pkProperty = GetProperty(kName);
    // Check that an already existing property has the same type
    EE_ASSERT(pkProperty == NULL || pkProperty->m_kType == kType);

    if (!pkProperty)
    {
        AddProperty(kName, kType);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::LoadSequenceDefinition(TiXmlElement* pkSequenceElement, float fEndTime)
{
    NiString kElementName = pkSequenceElement->Value();
    if (kElementName.Compare("Sequence") != 0)
        return;

    // Fetch the relevant property
    NiFixedString kPropertyName = pkSequenceElement->Attribute("propertyName");
    Property* pkProperty = GetProperty(kPropertyName);
    if (!pkProperty)
    {
        EE_FAIL("TimeOfDay Property not defined yet");
        return;
    }

    // Get the number of keys
    NiUInt32 uiNumKeys = 0;
    NiString kNumKeysString = pkSequenceElement->Attribute("numKeys");
    kNumKeysString.ToUInt(uiNumKeys);

    // Generate a sequence builder
    SequenceBuilder* pkSequenceBuilder = GetSequenceBuilder(pkProperty->m_kType, uiNumKeys);
    if (!pkSequenceBuilder)
    {
        EE_FAIL("Unrecognised type for TimeOfDay sequence");
        return;
    }

    // Gather all the keyframes:
    TiXmlElement* pkCurrentFrame = pkSequenceElement->FirstChildElement();
    NiUInt32 uiCurKey = 0;
    while (pkCurrentFrame)
    {
        // Load Keyframe information
        kElementName = pkCurrentFrame->Value();
        if (kElementName.Compare("KeyFrame") == 0)
        {
            // Calculate the time
            NiString kAttribute = pkCurrentFrame->Attribute("time");
            float fTime = GetSecondsFromString(kAttribute);

            // Load the key value
            EE_ASSERT(uiCurKey < uiNumKeys);
            EE_ASSERT(fTime <= fEndTime);
            pkSequenceBuilder->SetKey(uiCurKey, fTime, pkCurrentFrame);            
            uiCurKey++;
        }

        pkCurrentFrame = pkCurrentFrame->NextSiblingElement();
    }

    // Finalise the sequence:
    NiInterpolator *pkInterpolator = pkSequenceBuilder->FinaliseSequence(fEndTime);
    SetInterpolator(pkProperty, pkInterpolator);
    delete pkSequenceBuilder;
}

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::PropertyType NiTimeOfDay::GetPropertyTypeFromString(NiFixedString kType)
{
    if (kType.EqualsNoCase("float") || kType.EqualsNoCase("decimal"))
        return TODPT_FLOAT;
    else if (kType.EqualsNoCase("color"))
        return TODPT_COLOR;

    return TODPT_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
NiFixedString NiTimeOfDay::GetStringFromPropertyType(NiTimeOfDay::PropertyType  kType) const
{
    if (kType == TODPT_COLOR)
        return "color";
    else if (kType == TODPT_FLOAT)
        return "float";
    
    return "unknown";
}

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::Property* NiTimeOfDay::AddProperty(NiFixedString kName, 
    NiTimeOfDay::PropertyType kType)
{
    Property* pkResult = NiNew Property();
    pkResult->m_kName = kName;
    pkResult->m_kType = kType;
    pkResult->m_spController = NULL;
    pkResult->m_spInterpolator = NULL;

    m_kProperties.SetAt(kName, pkResult);

    return pkResult;
}

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::Property* NiTimeOfDay::GetProperty(NiFixedString kName)
{
    Property* pkResult = NULL;
    m_kProperties.GetAt(kName, pkResult);
    return pkResult;
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::SetInterpolator(Property* pkProperty, NiInterpolator* pkInterpolator)
{
    EE_ASSERT(pkProperty);

    pkProperty->m_spInterpolator = pkInterpolator;
    
    if (pkProperty->m_spController)
    {
        if (pkInterpolator)
        {
            pkProperty->m_spController->SetInterpolator(pkInterpolator);

            if (m_kControllers.Find(pkProperty->m_spController) == UINT_MAX)
                m_kControllers.AddFirstEmpty(pkProperty->m_spController);
        }
        else
        {
            m_kControllers.Remove(pkProperty->m_spController);
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiTimeOfDay::SequenceBuilder* NiTimeOfDay::GetSequenceBuilder(PropertyType kType, 
    NiUInt32 uiNumKeys)
{
    switch (kType)
    {
    case TODPT_FLOAT:
        return NiNew TSequenceBuilder<efd::Float32>(uiNumKeys);
    case TODPT_COLOR:
        return NiNew TSequenceBuilder<efd::ColorA>(uiNumKeys);
    case TODPT_UNKNOWN:
    default:
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
float NiTimeOfDay::GetSecondsFromString(NiString kString)
{
    NiString kSection;
    NiUInt32 uiOffset = 0;

    NiUInt32 uiNumHours = 0;
    NiUInt32 uiNumMinutes = 0;
    NiUInt32 uiNumSeconds = 0;
    NiUInt32 uiNumMilliSeconds = 0;

    kSection = kString.GetSubstring(uiOffset, ":");
    if (kSection.IsEmpty())
        kSection = kString.GetSubstring(uiOffset, kString.Length());
    uiOffset += kSection.Length() + 1;
    kSection.ToUInt(uiNumHours);

    kSection = kString.GetSubstring(uiOffset, ":");
    if (kSection.IsEmpty())
        kSection = kString.GetSubstring(uiOffset, kString.Length());
    uiOffset += kSection.Length() + 1;
    kSection.ToUInt(uiNumMinutes);

    kSection = kString.GetSubstring(uiOffset, ":");
    if (kSection.IsEmpty())
        kSection = kString.GetSubstring(uiOffset, kString.Length());
    uiOffset += kSection.Length() + 1;
    kSection.ToUInt(uiNumSeconds);

    kSection = kString.GetSubstring(uiOffset, ":");
    if (kSection.IsEmpty())
        kSection = kString.GetSubstring(uiOffset, kString.Length());
    uiOffset += kSection.Length() + 1;
    kSection.ToUInt(uiNumMilliSeconds);
    
    float fTime = 0.0f;
    fTime += uiNumHours * 3600.0f;
    fTime += uiNumMinutes * 60.0f;
    fTime += uiNumSeconds;
    fTime += uiNumMilliSeconds * 0.001f;

    return fTime;
}

//--------------------------------------------------------------------------------------------------
NiString NiTimeOfDay::GetStringsFromSeconds(float fTime)
{
    NiUInt32 uiNumHours = (NiUInt32)(fTime / 3600.0f);
    fTime -= uiNumHours * 3600.0f;

    NiUInt32 uiNumMinutes = (NiUInt32)(fTime / 60.0f);
    fTime -= uiNumMinutes * 60.0f;

    NiUInt32 uiNumSeconds = (NiUInt32)(fTime);
    fTime -= uiNumSeconds;

    NiUInt32 uiNumMilliSeconds = (NiUInt32)(fTime / 0.001f);
    fTime -= uiNumMilliSeconds * 0.001f;

    NiString kResult;
    kResult.Format("%d:%d:%d:%d", uiNumHours, uiNumMinutes, uiNumSeconds, uiNumMilliSeconds);

    return kResult;
}

//--------------------------------------------------------------------------------------------------
NiString NiTimeOfDay::ConvertValueToString(float fValue)
{
    efd::utf8string kResult;
    efd::ParseHelper<efd::Float32>::ToString(fValue, kResult);
    return kResult.c_str();
}

//--------------------------------------------------------------------------------------------------
NiString NiTimeOfDay::ConvertValueToString(efd::ColorA kValue)
{
    efd::utf8string kResult;
    efd::ParseHelper<efd::ColorA>::ToString(kValue, kResult);
    return kResult.c_str();
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::RegisterEnvironment(NiEnvironment* pkEnvironment)
{
    if (!pkEnvironment)
        return;

    efd::map<efd::utf8string, efd::utf8string> kPropMap;

    // Register fog property
    efd::utf8string kTemp = (const char*)(pkEnvironment->GetName() + NiString(".FogColor"));
    NiTTimeOfDayDefaultFunctor<NiEnvironment, NiColorA> kColorFunctor(pkEnvironment, 
        &NiEnvironment::SetFogColor);    
    BindProperty<NiTTimeOfDayDefaultFunctor<NiEnvironment, NiColorA>, NiColorA>(
        kTemp.c_str(), kColorFunctor); 
    efd::Color kColor = pkEnvironment->GetFogProperty()->GetFogColor();
    kPropMap[kTemp] = ConvertValueToString(efd::ColorA(kColor.r, kColor.g, kColor.b, 1.0f));

    kTemp = (const char*)(pkEnvironment->GetName() + NiString(".SunElevationAngle"));
    NiTTimeOfDayDefaultFunctor<NiEnvironment, float> kFloatFunctor(pkEnvironment, 
        &NiEnvironment::SetSunElevationAngle);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiEnvironment, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkEnvironment->GetSunElevationAngle());

    kTemp = (const char*)(pkEnvironment->GetName() + NiString(".SunAzimuthAngle"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiEnvironment, float>(pkEnvironment, 
        &NiEnvironment::SetSunAzimuthAngle);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiEnvironment, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkEnvironment->GetSunAzimuthAngle());

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
        UpdatePropertySequence(kIter->first.c_str(),2, kTimes, kValues);
        ++kIter;
    }    

    // Register the components of the environment
    RegisterAtmosphere(pkEnvironment->GetAtmosphere());

    // If the sky is a type of dome, register the dome properties.
    NiSkyDome* pkSkyDome = NiDynamicCast(NiSkyDome, pkEnvironment->GetSky());
    if (pkSkyDome)
        RegisterSkyDome(pkSkyDome);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UnregisterEnvironment(NiEnvironment* pkEnvironment)
{
    efd::vector<efd::utf8string> kToRemoveList;
    efd::utf8string kTemp = (const char*)(pkEnvironment->GetName() + NiString(".FogColor"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkEnvironment->GetName() + NiString(".BeringAngle"));
    kToRemoveList.push_back(kTemp);   
    kTemp = (const char*)(pkEnvironment->GetName() + NiString(".AzimuthAngle"));
    kToRemoveList.push_back(kTemp);

    RemoveToDProperties(kToRemoveList);

    UnregisterAtmosphere(pkEnvironment->GetAtmosphere()); 

    NiSkyDome* pkSkyDome = NiDynamicCast(NiSkyDome, pkEnvironment->GetSky());
    if (pkSkyDome)
        UnregisterSkyDome(pkSkyDome);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::RegisterAtmosphere(NiAtmosphere* pkAtmosphere)
{
    if (!pkAtmosphere)
        return;

    efd::map<efd::utf8string, efd::utf8string> kPropMap;

    efd::utf8string kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".PhaseConstant"));
    NiTTimeOfDayDefaultFunctor<NiAtmosphere, float> kFloatFunctor = 
        NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetPhaseConstant);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetPhaseConstant());

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".SunSize"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetSunSize);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetSunSize());

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".SunIntensity"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetSunIntensity);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetSunIntensity());

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".RayleighConstant"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetRayleighConstant);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetRayleighConstant());

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".MieConstant"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetMieConstant);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetMieConstant());

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".RedWavelength"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetRedWavelength);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    NiPoint3 kRGBWavelength = pkAtmosphere->GetRGBWavelengths();
    kPropMap[kTemp] = ConvertValueToString(kRGBWavelength.x);

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".GreenWavelength"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetGreenWavelength);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(kRGBWavelength.y);

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".BlueWavelength"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetBlueWavelength);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(kRGBWavelength.z);

    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".HDRExposure"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>(pkAtmosphere, 
        &NiAtmosphere::SetHDRExposure);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiAtmosphere, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkAtmosphere->GetHDRExposure());

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
        UpdatePropertySequence(kIter->first.c_str(),2, kTimes, kValues);
        ++kIter;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UnregisterAtmosphere(NiAtmosphere* pkAtmosphere)
{

    efd::vector<efd::utf8string> kToRemoveList;    

    efd::utf8string kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".PhaseConstant"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".SunSize"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".SunIntensity"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".RayleighConstant"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".MieConstant"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".RedWavelength"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".GreenWavelength"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".BlueWavelength"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkAtmosphere->GetName() + NiString(".HDRExposure"));
    kToRemoveList.push_back(kTemp);

    RemoveToDProperties(kToRemoveList);
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::RegisterSkyDome(NiSkyDome* pkSky)
{
    if (!pkSky)
        return;

    efd::map<efd::utf8string, efd::utf8string> kPropMap;
    // Dome settings
    efd::utf8string kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() +
        NiString(".DomeRadius"));
    NiTTimeOfDayDefaultFunctor<NiSkyDome, float> kFloatFunctor = 
        NiTTimeOfDayDefaultFunctor<NiSkyDome, float>(pkSky, 
        &NiSkyDome::SetDomeRadius);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyDome, float>, float>(
        kTemp.c_str(), kFloatFunctor); 
    kPropMap[kTemp] = ConvertValueToString(pkSky->GetDomeRadius());

    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetail"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyDome, float>(pkSky, 
        &NiSkyDome::SetDomeDetail);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyDome, float>, float>(
        kTemp.c_str(), kFloatFunctor);
    kPropMap[kTemp] = ConvertValueToString(pkSky->GetDomeDetail());

    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetailAxisBias"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyDome, float>(pkSky, 
        &NiSkyDome::SetDomeDetailAxisBias);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyDome, float>, float>(
        kTemp.c_str(), kFloatFunctor);
    kPropMap[kTemp] = ConvertValueToString(pkSky->GetDomeDetailAxisBias());

    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetailHorizonBias"));
    kFloatFunctor = NiTTimeOfDayDefaultFunctor<NiSkyDome, float>(pkSky, 
        &NiSkyDome::SetDomeDetailHorizonBias);
    BindProperty<NiTTimeOfDayDefaultFunctor<NiSkyDome, float>, float>(
        kTemp.c_str(), kFloatFunctor);
    kPropMap[kTemp] = ConvertValueToString(pkSky->GetDomeDetailHorizonBias());

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
        UpdatePropertySequence(kIter->first.c_str(),2, kTimes, kValues);
        ++kIter;
    }

    for (NiUInt32 ui = 0; ui < NiSkyDome::NUM_BLEND_STAGES; ++ui)
    {
        if (pkSky->GetBlendStage(ui) == NULL)
            continue;

        pkSky->GetBlendStage(ui)->BindProperties(ui, this);
    }           
}

//--------------------------------------------------------------------------------------------------
void NiTimeOfDay::UnregisterSkyDome(NiSkyDome* pkSky)
{
    efd::vector<efd::utf8string> kToRemoveList;

    efd::utf8string kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() +
        NiString(".DomeRadius"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetail"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetailAxisBias"));
    kToRemoveList.push_back(kTemp);
    kTemp = (const char*)(pkSky->GetAtmosphere()->GetName() + NiString(".DomeDetailHorizonBias"));
    kToRemoveList.push_back(kTemp);

    RemoveToDProperties(kToRemoveList);

    for (NiUInt32 ui = 0; ui < NiSkyDome::NUM_BLEND_STAGES; ++ui)
    {
        if (pkSky->GetBlendStage(ui) == NULL)
            continue;

        pkSky->GetBlendStage(ui)->RemoveProperties(ui, this);
    } 
}
//--------------------------------------------------------------------------------------------------
