// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

// Precompiled Header
#include "NiMainPCH.h"

#include "NiLightManager.h"

#include "NiLight.h"
#include "NiAmbientLight.h"
#include "NiPointLight.h"
#include "NiSpotLight.h"
#include "NiDirectionalLight.h"

//--------------------------------------------------------------------------------------------------
// Singleton
//--------------------------------------------------------------------------------------------------
NiLightManager* NiLightManager::ms_pkLightManager = NULL;
//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLightManager, NiAVObject, NiTypeMask::NiLightManager);
//--------------------------------------------------------------------------------------------------
// Public interface
//--------------------------------------------------------------------------------------------------
NiLightManager::NiLightManager()
{
}

//--------------------------------------------------------------------------------------------------
NiLightManager::~NiLightManager()
{
    RemoveAllLights();
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::Initialize()
{
    EE_ASSERT(!ms_pkLightManager);
    ms_pkLightManager = NiNew NiLightManager();
    EE_ASSERT(ms_pkLightManager);
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::Shutdown()
{
    NiDelete ms_pkLightManager;
    ms_pkLightManager = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::Add(NiLight* pkLight)
{
    EE_ASSERT(pkLight);
    if ( !Contains(pkLight) )
    {
        MappedLight* pkMappedLight = m_kMappedLightPool.GetFreeObject();
        pkMappedLight->spLight = pkLight;
        m_kMapLights.SetAt(pkLight, pkMappedLight);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiLightManager::Contains(const NiLight* pkLight) const
{
    return NULL != Find(pkLight);
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::Remove(NiLight* pkLight)
{
    MappedLight* pkMappedLight = Find(pkLight);
    if (pkMappedLight)
    {
        m_kMapLights.RemoveAt(pkLight);
        pkMappedLight->spLight = NULL;
        m_kMappedLightPool.ReleaseObject(pkMappedLight);
    }
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::RemoveAllLights()
{
    NiTMapIterator kPos = m_kMapLights.GetFirstPos();
    while (kPos)
    {
        const NiLight* pkLight;
        MappedLight* pkMappedLight;
        m_kMapLights.GetNext(kPos, pkLight, pkMappedLight);

        pkMappedLight->spLight = NULL;
        m_kMappedLightPool.ReleaseObject(pkMappedLight);
    }
    m_kMapLights.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
// Virtual interface
//--------------------------------------------------------------------------------------------------
float NiLightManager::Priority(const NiRenderObject* pkRenderObject, const NiLight* pkLight)
{
    // Only assign light to object if in common light group
    if (0 == (pkRenderObject->GetLightGroupMask() & pkLight->GetGroupMask()))
    {
        return -1.0f;
    }

    // Select light based on range parameter

    float fRange = pkLight->GetRange();

    const NiBound& kBound = pkRenderObject->GetWorldBound();

    efd::Point3 kDist = pkLight->GetWorldTranslate() - kBound.GetCenter();
    float fDist = kDist.Length();

    if (fRange > 0.0f && fDist > (fRange + kBound.GetRadius())) // NOTE fRange <= 0 is infinite range
    {
        return -1.0f; // reject lights outside of range
    }

    const float MIN_DIST = 1.0e-25f; // minimum sensitivity to prevent division by too small a value
    float fPriority = 1.0f / efd::Max(fDist, MIN_DIST);

    // TODO factor in attenuation or dimmer, spotlight hemisphere, etc.
    // > fPriority *= dimmer * attenuation ?

    return fPriority;
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::GetAllLights(NiTPointerList<NiLight *>& kList) const
{
    const NiLight* kKey;
    MappedLight* kVal;
    NiTMapIterator pkIter = m_kMapLights.GetFirstPos();
    while (pkIter)
    {
        m_kMapLights.GetNext(pkIter, kKey, kVal);
        if (kVal)
        {
            kList.AddTail(kVal->spLight);
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Protected interface
//--------------------------------------------------------------------------------------------------
NiLightManager::MappedLight* NiLightManager::Find(const NiLight* pkLight) const
{
    MappedLight* pkMappedLight = NULL;
    m_kMapLights.GetAt(pkLight, pkMappedLight);
    EE_ASSERT(pkMappedLight == NULL || pkMappedLight->spLight == pkLight);
    return pkMappedLight;
}

//--------------------------------------------------------------------------------------------------
void NiLightManager::Debug_() const
{
    //GB_DEBUG_MSG("NiLightManager::Debug_()\n");

    NiTMapIterator kPos = m_kMapLights.GetFirstPos();
    while (kPos)
    {
        const NiLight* pkLight;
        MappedLight* pkMappedLight;
        m_kMapLights.GetNext(kPos, pkLight, pkMappedLight);

        const char * pcName = "NiLight";
        switch (pkLight->GetEffectType())
        {
            case NiDynamicEffect::AMBIENT_LIGHT:
                pcName = "AmbientLight";
                break;
            case NiDynamicEffect::POINT_LIGHT:
            case NiDynamicEffect::SHADOWPOINT_LIGHT:
                pcName = "PointLight";
                break;
            case NiDynamicEffect::DIR_LIGHT:
            case NiDynamicEffect::SHADOWDIR_LIGHT:
                pcName = "DirectionalLight";
                break;
            case NiDynamicEffect::SPOT_LIGHT:
            case NiDynamicEffect::SHADOWSPOT_LIGHT:
                pcName = "SpotLight";
                break;
            default:
                break;
        }

        //GB_DEBUG_MSG("%s %08p/%08p [%02X]> ", pcName, pkLight, pkMappedLight, 
	}
        
}


//--------------------------------------------------------------------------------------------------
