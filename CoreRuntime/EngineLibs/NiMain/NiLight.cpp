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

#include "NiBool.h"
#include "NiLight.h"
#include "NiNode.h"
#include "NiRenderer.h"
#include "NiStream.h"

NiImplementRTTI(NiLight, NiDynamicEffect, NiTypeMask::NiLight);

//--------------------------------------------------------------------------------------------------
NiLight::NiLight()
{
    m_fRange = 0.0f;
	m_fMaxRange = 100000.0f;
    m_fFalloff = 2.0f;
    m_fDimmer = 1.0f;

    m_kAmb.r = 1.0f;
    m_kAmb.g = 1.0f;
    m_kAmb.b = 1.0f;

    m_kDiff.r = 1.0f;
    m_kDiff.g = 1.0f;
    m_kDiff.b = 1.0f;

    m_kSpec.r = 1.0f;
    m_kSpec.g = 1.0f;
    m_kSpec.b = 1.0f;

    m_uiGroupMask = 0xFF;

#if defined(WIN32) || defined(_XENON)
    m_pvRendererData = 0;
#endif // defined(WIN32) || defined(_XENON)

}

//--------------------------------------------------------------------------------------------------
NiLight::~NiLight()
{
    // This must remain in NiDynamicEffect derived classes rather than in the
    // base class.  Some renderers rely on ability to dynamic cast
    // NiDynamicEffects to determine if they are NiLight derived, and this is
    // not possible if the NiLight portion of the object has been destroyed.
    NiRenderer::RemoveEffect(this);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
void NiLight::CopyMembers(NiLight* pDest,
    NiCloningProcess& kCloning)
{
    NiDynamicEffect::CopyMembers(pDest, kCloning);

    pDest->m_fRange = m_fRange;
	pDest->m_fMaxRange = m_fMaxRange;
    pDest->m_fFalloff = m_fFalloff;
    pDest->m_fDimmer = m_fDimmer;
    pDest->m_kAmb = m_kAmb;
    pDest->m_kDiff = m_kDiff;
    pDest->m_kSpec = m_kSpec;
    pDest->m_uiGroupMask = m_uiGroupMask;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
void NiLight::LoadBinary(NiStream& stream)
{
    NiDynamicEffect::LoadBinary(stream);

    // attributes
    if (stream.GetFileVersion() >= NiStream::GetVersion(30, 3, 0, 5))
    {
        NiStreamLoadBinary(stream, m_fRange);
        NiStreamLoadBinary(stream, m_fMaxRange);
        NiStreamLoadBinary(stream, m_fFalloff);
        NiStreamLoadBinary(stream, m_uiGroupMask);
    }
    NiStreamLoadBinary(stream, m_fDimmer);
    m_kAmb.LoadBinary(stream);
    m_kDiff.LoadBinary(stream);
    m_kSpec.LoadBinary(stream);
}

//--------------------------------------------------------------------------------------------------
void NiLight::LinkObject(NiStream& stream)
{
    NiDynamicEffect::LinkObject(stream);

    // Illuminated nodes will be linked by the nodes themselves.  They
    // should not be linked here (else you get a recursive loop).
}

//--------------------------------------------------------------------------------------------------
bool NiLight::RegisterStreamables(NiStream& stream)
{
    return NiDynamicEffect::RegisterStreamables(stream);

    // Illuminated nodes are already registered by the nodes themselves.
    // They should not be registered here (else you get a recursive loop).
}

//--------------------------------------------------------------------------------------------------
void NiLight::SaveBinary(NiStream& stream)
{
    NiDynamicEffect::SaveBinary(stream);

    // attributes
    NiStreamSaveBinary(stream, m_fRange);
    NiStreamSaveBinary(stream, m_fMaxRange);
    NiStreamSaveBinary(stream, m_fFalloff);
    NiStreamSaveBinary(stream, m_uiGroupMask);
    NiStreamSaveBinary(stream, m_fDimmer);
    m_kAmb.SaveBinary(stream);
    m_kDiff.SaveBinary(stream);
    m_kSpec.SaveBinary(stream);
}

//--------------------------------------------------------------------------------------------------
bool NiLight::IsEqual(NiObject* pObject)
{
    if (!NiDynamicEffect::IsEqual(pObject))
        return false;

    NiLight* pLight = (NiLight*) pObject;

    // attributes
    if (m_fRange != pLight->m_fRange
	||  m_fMaxRange != pLight->m_fMaxRange
    ||  m_fFalloff != pLight->m_fFalloff
    ||  m_fDimmer != pLight->m_fDimmer
    ||  m_kAmb != pLight->m_kAmb
    ||  m_kDiff != pLight->m_kDiff
    ||  m_kSpec != pLight->m_kSpec
    ||  m_uiGroupMask != pLight->m_uiGroupMask)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiLight::GetViewerStrings(NiViewerStringsArray* pStrings)
{
    NiDynamicEffect::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiLight::ms_RTTI.GetName()));

    pStrings->Add(NiGetViewerString("m_fRange",m_fRange));
	pStrings->Add(NiGetViewerString("m_fMaxRange",m_fMaxRange));
    pStrings->Add(NiGetViewerString("m_fFalloff",m_fFalloff));
    pStrings->Add(NiGetViewerString("m_fDimmer",m_fDimmer));

    pStrings->Add(m_kAmb.GetViewerString("m_kAmb"));
    pStrings->Add(m_kDiff.GetViewerString("m_kDiff"));
    pStrings->Add(m_kSpec.GetViewerString("m_kSpec"));

    pStrings->Add(NiGetViewerString("m_uiGroupMask", m_uiGroupMask));
}

//--------------------------------------------------------------------------------------------------

