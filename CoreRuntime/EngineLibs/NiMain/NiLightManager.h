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

#pragma once
#ifndef NILIGHTMANAGER_H
#define NILIGHTMANAGER_H

#include "NiMainLibType.h"
#include "NiAVObject.h"
#include "NiNode.h"
#include "NiRenderObject.h"
#include "NiLight.h"
#include "NiTArray.h"
#include "NiTPool.h"
#include "NiTMap.h"

class NIMAIN_ENTRY NiLightManager : public efd::MemObject
{
    NiDeclareRTTI;

public:

    // construction and destruction
    NiLightManager();
    virtual ~NiLightManager();

    static void Initialize();
    static void Shutdown();
    inline static NiLightManager* GetLightManager();

    void Add(NiLight* pkLight);
    void Remove(NiLight* pkLight);
	bool Contains(const NiLight* pkLight) const;

	void RemoveAllLights();
    
	void GetAllLights(NiTPointerList<NiLight *>& kList) const;
    
    // NiLightManager::Priority determines the assignment and order of lights attached to an object.
    // return >=0 if the light should be assigned to this object.
    //     Higher priority values will be sorted toward the beginning of the object's effect list.
    // return <0 if the light should not be assigned to this object.
    virtual float Priority(const NiRenderObject* pkRenderObject, const NiLight* pkLight);

protected:
    // linked list structures for storing light/mesh lists
    struct MappedLight : public MemObject
    {
        NiLightPtr spLight;
    };

    NiTObjectPool<MappedLight> m_kMappedLightPool;
    NiTMap<const NiLight*, MappedLight*> m_kMapLights;
    
    // find mapping entry
    MappedLight* Find(const NiLight* pkLight) const;

public:
    // dumps diagnostic information to debug output
    void Debug_() const;

private:
    static NiLightManager* ms_pkLightManager;
};

NiSmartPointer(NiLightManager);

#include "NiLightManager.inl"

#endif
