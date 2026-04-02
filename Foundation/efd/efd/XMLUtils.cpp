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

#include "efdPCH.h"

#include <efd/TinyXML.h>

using namespace efd;


//--------------------------------------------------------------------------------------------------
bool XMLUtils::GetAttribute(const TiXmlAttributeSet& attrs, const EE_TIXML_STRING& name,
                             efd::utf8string& o_result)
{
    bool retval = false;

    const TiXmlAttribute* attrib = attrs.Find (name.c_str());
    if (attrib)
    {
        o_result = attrib->Value();
        retval = true;
    }

    return retval;
}

