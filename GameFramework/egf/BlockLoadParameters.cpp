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

#include "egfPCH.h"

#include <egf/BlockLoadParameters.h>

using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(BlockLoadParameters);

bool BlockLoadParameters::ms_defaultAutoEnterWorld = true;


//------------------------------------------------------------------------------------------------
BlockLoadParameters::BlockLoadParameters()
    : m_notificationCategory(efd::kCAT_INVALID)
    , m_notificationContext(0)
    , m_notificationBehavior(0)
    , m_callbacks((efd::UInt32)blc_Default)
    , m_flags(0)
    , m_loadThrottleOverride(k_UseDefaultLimit)
    , m_unloadThrottleOverride(k_UseDefaultLimit)
{
    EE_COMPILETIME_ASSERT(sizeof(efd::UInt32) >= sizeof(m_bits));
    m_bits.autoEnterWorld = ms_defaultAutoEnterWorld;
}

//------------------------------------------------------------------------------------------------
BlockLoadParameters::BlockLoadParameters(const efd::Category& cat)
    : m_notificationCategory(cat)
    , m_notificationContext(0)
    , m_notificationBehavior(0)
    , m_callbacks((efd::UInt32)blc_Default)
    , m_flags(0)
    , m_loadThrottleOverride(k_UseDefaultLimit)
    , m_unloadThrottleOverride(k_UseDefaultLimit)
{
    m_bits.autoEnterWorld = ms_defaultAutoEnterWorld;
}

//------------------------------------------------------------------------------------------------
BlockLoadParameters::~BlockLoadParameters()
{
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetDefaultAutoEnterWorld(bool autoEnter)
{
    ms_defaultAutoEnterWorld = autoEnter;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetMessageCallback(const efd::Category& cat, efd::UInt32 context)
{
    m_notificationCategory = cat;
    m_notificationContext = context;
    m_notificationBehavior = 0;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetBehaviorCallback(
    efd::Category cat,
    egf::BehaviorID behavior,
    efd::UInt32 context)
{
    m_notificationCategory = cat;
    m_notificationBehavior = behavior;
    m_notificationContext = context;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetActiveCallbacks(efd::UInt32 activeCallbacks)
{
    m_callbacks = activeCallbacks;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetBlockRotation(const efd::Point3& rotation)
{
    m_rotation = rotation;
    m_bits.rotationSet = true;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::ClearBlockRotation()
{
    m_bits.rotationSet = false;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetBlockOffset(const efd::Point3& offset)
{
    m_offset = offset;
    m_bits.offsetSet = true;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::ClearBlockOffset()
{
    m_bits.offsetSet = false;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetLoadThresholdOverride(efd::UInt32 override)
{
    m_loadThrottleOverride = override;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::SetUnloadThresholdOverride(efd::UInt32 override)
{
    m_unloadThrottleOverride = override;
}

//------------------------------------------------------------------------------------------------
void BlockLoadParameters::Serialize(efd::Archive& io_ar)
{
    efd::Serializer::SerializeObject(m_notificationCategory, io_ar);
    efd::Serializer::SerializeObject(m_notificationContext, io_ar);
    efd::Serializer::SerializeObject(m_notificationBehavior, io_ar);
    efd::Serializer::SerializeObject(m_callbacks, io_ar);
    efd::Serializer::SerializeObject(m_flags, io_ar);
    if (UseRotation())
        efd::Serializer::SerializeObject(m_rotation, io_ar);
    if (UseOffset())
        efd::Serializer::SerializeObject(m_offset, io_ar);
    efd::Serializer::SerializeObject(m_loadThrottleOverride, io_ar);
    efd::Serializer::SerializeObject(m_unloadThrottleOverride, io_ar);
}

//------------------------------------------------------------------------------------------------
