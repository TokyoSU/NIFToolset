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

#include <efd/StdStreamFile.h>

namespace efd
{

//--------------------------------------------------------------------------------------------------
StdStreamFile* StdStreamFile::GetFile(FILE* stream)
{
    return EE_NEW StdStreamFile(stream);
}

//--------------------------------------------------------------------------------------------------
StdStreamFile::StdStreamFile(FILE* stream)
        : m_stream(stream)
{
}

//--------------------------------------------------------------------------------------------------
StdStreamFile::~StdStreamFile()
{
    this->m_bGood = false;
}

//--------------------------------------------------------------------------------------------------
unsigned int StdStreamFile::Read(void *pvBuffer, unsigned int uiBytes)
{
    EE_ASSERT(m_stream == stdin);
    fgets((char *)pvBuffer, uiBytes, m_stream);
    return uiBytes;
}

//--------------------------------------------------------------------------------------------------
unsigned int StdStreamFile::Write(const void *pvBuffer, unsigned int uiBytes)
{
    EE_ASSERT(m_stream == stdout || m_stream == stderr);
    EE_UNUSED_ARG(uiBytes);
    return fprintf(m_stream, "%s", (char *)pvBuffer);
}

//--------------------------------------------------------------------------------------------------
bool StdStreamFile::Flush()
{
    fflush(m_stream);
    return true;
}

//--------------------------------------------------------------------------------------------------
StdStreamFile::operator bool() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool StdStreamFile::Seek(int iNumBytes)
{
    /* noop */
    EE_UNUSED_ARG(iNumBytes);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool StdStreamFile::Seek(int iOffset, SeekOrigin iWhence)
{
    /* noop */
    EE_UNUSED_ARG(iOffset);
    EE_UNUSED_ARG(iWhence);
    return true;
}

//--------------------------------------------------------------------------------------------------
unsigned int StdStreamFile::GetFileSize() const
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
bool StdStreamFile::eof()
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool StdStreamFile::IsGood()
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void StdStreamFile::SetEndianSwap(bool bDoSwap)
{
    EE_UNUSED_ARG(bDoSwap);
    EE_FAIL("StdStreamFile::SetEndianSwap - not implemented.");
}

} // end namespace efd

