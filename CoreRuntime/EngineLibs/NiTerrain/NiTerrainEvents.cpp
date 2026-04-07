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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"
#include "NiTerrainEvents.h"

//-------------------------------------------------------------------------------------------------
efd::UInt32 NiTerrainEventDelegateTypeID::ms_uiNextValue = 0;
//-------------------------------------------------------------------------------------------------
NiTerrainEventDelegateTypeID::NiTerrainEventDelegateTypeID()
{
    m_uiTypeID = ms_uiNextValue;
    ms_uiNextValue++;
}

//-------------------------------------------------------------------------------------------------
bool NiTerrainEventDelegateTypeID::operator==(const NiTerrainEventDelegateTypeID& kOther) const
{
    return m_uiTypeID == kOther.m_uiTypeID;
}

//-------------------------------------------------------------------------------------------------