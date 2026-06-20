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

#include "NiPointLight.h"
#include "NiStream.h"

NiImplementRTTI(NiPointLight, NiLight, NiTypeMask::NiPointLight);

//--------------------------------------------------------------------------------------------------
NiPointLight::NiPointLight()
{
    m_ucEffectType = NiDynamicEffect::POINT_LIGHT;
}

//--------------------------------------------------------------------------------------------------
void NiPointLight::UpdateWorldData()
{
    NiLight::UpdateWorldData();
    IncRevisionID();
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPointLight);

//--------------------------------------------------------------------------------------------------
void NiPointLight::CopyMembers(NiPointLight* pDest,
    NiCloningProcess& kCloning)
{
    NiLight::CopyMembers(pDest, kCloning);

    // The list m_illuminatedNodeList is not processed.  The application
    // has the responsibility for cloning the relationships between the
    // lights and nodes.
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPointLight);

//--------------------------------------------------------------------------------------------------
void NiPointLight::LoadBinary(NiStream& stream)
{
    NiLight::LoadBinary(stream);

    if (stream.GetFileVersion() < NiStream::GetVersion(30, 3, 0, 5))
    {
        float fAtten0;
        float fAtten1;
        float fAtten2;
        NiStreamLoadBinary(stream, fAtten0);
        NiStreamLoadBinary(stream, fAtten1);
        NiStreamLoadBinary(stream, fAtten2);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPointLight::LinkObject(NiStream& stream)
{
    NiLight::LinkObject(stream);

    // Illuminated nodes will be linked by the nodes themselves.  They
    // should not be linked here (else you get a recursive loop).
}

//--------------------------------------------------------------------------------------------------
bool NiPointLight::RegisterStreamables(NiStream& stream)
{
    return NiLight::RegisterStreamables(stream);

    // Illuminated nodes are already registered by the nodes themselves.
    // They should not be registered here (else you get a recursive loop).
}

//--------------------------------------------------------------------------------------------------
void NiPointLight::SaveBinary(NiStream& stream)
{
    NiLight::SaveBinary(stream);

    // No extra properties on a point light, attenuation is stored on the NiLight
}

//--------------------------------------------------------------------------------------------------
bool NiPointLight::IsEqual(NiObject* pObject)
{
    if (!NiLight::IsEqual(pObject))
        return false;

    // No extra properties on a point light, attenuation is stored on the NiLight

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPointLight::GetViewerStrings(NiViewerStringsArray* pStrings)
{
    NiLight::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiPointLight::ms_RTTI.GetName()));
}

//--------------------------------------------------------------------------------------------------
