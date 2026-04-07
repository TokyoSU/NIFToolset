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

#include "ecrPCH.h"

#include <efd/SerializeRoutines.h>
#include <ecr/AttachNifData.h>

using namespace efd;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(ecr::AttachNifData);

//------------------------------------------------------------------------------------------------
AttachNifData::AttachNifData()
    : m_translation(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f)
{
}

//------------------------------------------------------------------------------------------------
AttachNifData::AttachNifData(const efd::utf8string& attachPoint, const efd::AssetID& nifAsset)
{
    m_attachPoint = attachPoint;
    m_nifAsset = nifAsset;
}

//------------------------------------------------------------------------------------------------
AttachNifData::~AttachNifData()
{
    // This function intentionally left blank
}

//------------------------------------------------------------------------------------------------
bool AttachNifData::FromString(const efd::utf8string& stringValue)
{
    // Can't use Strtok because it considers there to be no content between ';' in ";;"
    efd::Char* stringAsChar = efd::Strdup(stringValue.c_str());
    if (!stringAsChar)
        return false;

    // Find 5 strings and replace ';' with end of string to delineate
    efd::Char* posn = stringAsChar;
    efd::Char* startPoints[5];
    for (efd::UInt32 ui = 0; ui < 5; ++ui)
    {
        // Look for the end of the given portion
        startPoints[ui] = posn;
        while (*posn != ';' && *posn != '\0')
        {
            posn++;
        }

        if (*posn == '\0')
        {
            if (ui != 4)
            {
                EE_FREE(stringAsChar);
                return false;
            }
        }
        else
        {
            *posn = '\0';
            ++posn;
        }
    }

    efd::Point3 newTranslation;
    if (!efd::ParseHelper<efd::Point3>::FromString(startPoints[2], newTranslation))
    {
        EE_FREE(stringAsChar);
        return false;
    }

    efd::Point3 newRotation;
    if (!efd::ParseHelper<efd::Point3>::FromString(startPoints[3], newRotation))
    {
        EE_FREE(stringAsChar);
        return false;
    }

    efd::Float32 newScale;
    if (!efd::ParseHelper<efd::Float32>::FromString(startPoints[4], newScale))
    {
        EE_FREE(stringAsChar);
        return false;
    }

    m_attachPoint = startPoints[0];
    m_nifAsset = startPoints[1];
    m_translation = newTranslation;
    m_rotation = newRotation;
    m_scale = newScale;

    EE_FREE(stringAsChar);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::utf8string AttachNifData::ToString() const
{
    efd::utf8string tStr;
    efd::ParseHelper<efd::Point3>::ToString(m_translation, tStr);
    efd::utf8string rStr;
    efd::ParseHelper<efd::Point3>::ToString(m_rotation, rStr);
    efd::utf8string sStr;
    efd::ParseHelper<efd::Float32>::ToString(m_scale, sStr);
    efd::utf8string str(efd::Formatted, "%s;%s;%s;%s;%s",
        m_attachPoint.c_str(), m_nifAsset.c_str(), tStr.c_str(), rStr.c_str(), sStr.c_str());

    return str;
}

//------------------------------------------------------------------------------------------------
void AttachNifData::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(m_attachPoint, ar);
    Serializer::SerializeObject(m_nifAsset, ar);
    Serializer::SerializeObject(m_translation, ar);
    Serializer::SerializeObject(m_rotation, ar);
    Serializer::SerializeObject(m_scale, ar);
}
