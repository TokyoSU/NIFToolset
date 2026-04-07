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

#include <egf/BlockIdentification.h>
#include <efd/Serialize.h>

using namespace egf;

//------------------------------------------------------------------------------------------------
bool BlockIdentification::operator == (const BlockIdentification& other) const
{
    return m_instance == other.m_instance && m_blockAsset == other.m_blockAsset;
}

//------------------------------------------------------------------------------------------------
bool BlockIdentification::operator != (const BlockIdentification& other) const
{
    return m_instance != other.m_instance || m_blockAsset != other.m_blockAsset;
}

//------------------------------------------------------------------------------------------------
bool BlockIdentification::operator > (const BlockIdentification& other) const
{
    if (m_blockAsset == other.m_blockAsset)
        return m_instance > other.m_instance;
    return m_blockAsset > other.m_blockAsset;
}

//------------------------------------------------------------------------------------------------
inline bool BlockIdentification::operator <= (const BlockIdentification& other) const
{
    if (m_blockAsset == other.m_blockAsset)
        return m_instance <= other.m_instance;
    return m_blockAsset <= other.m_blockAsset;
}

//------------------------------------------------------------------------------------------------
inline bool BlockIdentification::operator >= (const BlockIdentification& other) const
{
    if (m_blockAsset == other.m_blockAsset)
        return m_instance >= other.m_instance;
    return m_blockAsset >= other.m_blockAsset;
}

//------------------------------------------------------------------------------------------------
void BlockIdentification::Serialize(efd::Archive& io_ar)
{
    efd::Serializer::SerializeObject(m_blockAsset, io_ar);
    efd::Serializer::SerializeObject(m_instance, io_ar);
}

//------------------------------------------------------------------------------------------------
