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

#include "efdNetworkPCH.h"
#include <efdNetwork/NetVersion.h>

using namespace efd;


//------------------------------------------------------------------------------------------------
NetVersion::NetVersion()
{}

//------------------------------------------------------------------------------------------------
NetVersion::~NetVersion()
{}

//------------------------------------------------------------------------------------------------
// The format MUST be very precise, it must be:
//      32bit numStrings
//      [ 32bit strlen, 8bit * strlen ] * numStrings
// We cannot simply SR_StdVector<>::Serialize this because that uses a more optimized format that
// would confuse down-level clients that might receive this message.
void NetVersion::Serialize(efd::Archive& ar)
{
    efd::UInt32 numStrings = m_versions.size();
    efd::Serializer::SerializeObject(numStrings, ar);

    for (efd::UInt32 i = 0; i < numStrings; ++i)
    {
        efd::UInt32 cch;
        if (ar.IsPacking()) cch = m_versions[i].size();
        efd::Serializer::SerializeObject(cch, ar);
        if (ar.IsPacking())
        {
            efd::Serializer::SerializeRawBytes((efd::UInt8*)m_versions[i].data(), cch, ar);
        }
        else
        {
            // Exact code that old DataStream::Read(utf8string) used:
            efd::Char* cTmp = EE_ALLOC(efd::Char, cch + 1);
            efd::Serializer::SerializeRawBytes((efd::UInt8*)cTmp, cch, ar);
            cTmp[cch] = 0;
            m_versions.push_back(efd::utf8string(cTmp));
            EE_FREE(cTmp);
        }
    }
}

//------------------------------------------------------------------------------------------------
