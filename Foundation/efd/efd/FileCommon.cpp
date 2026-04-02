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
#include "efdPCH.h"

#include <efd/FileCommon.h>

namespace efd
{

//--------------------------------------------------------------------------------------------------
FileCommon::FileCommon()
    : m_bGood(false)
{
}

//--------------------------------------------------------------------------------------------------
FileCommon::~FileCommon()
{
}

//--------------------------------------------------------------------------------------------------
bool FileCommon::eof()
{
    if (!IsGood()) return false;
    return GetPosition() == GetFileSize();
}

//--------------------------------------------------------------------------------------------------
bool FileCommon::IsGood()
{
    return m_bGood;
}

//--------------------------------------------------------------------------------------------------
FileCommon::operator bool() const
{
    return m_bGood;
}

} // end namespace efd

