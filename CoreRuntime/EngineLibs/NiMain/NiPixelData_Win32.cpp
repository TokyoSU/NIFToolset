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
#include "NiMainPCH.h"

#include "NiPixelData.h"
#include <NiMemHint.h>

//--------------------------------------------------------------------------------------------------
void NiPixelData::AllocateData(unsigned int uiLevels, unsigned int uiFaces,
    unsigned int uiBytes)
{
    EE_ASSERT(uiFaces > 0);
    EE_ASSERT(uiLevels > 0);

    // Rounding up for multiple of 4 for pointer allignment.
    uiBytes = (uiBytes + 3) & ~3;

    // uiLevels * 3     (Levels for each: Width,Height,OffsetInBytes)
    // << 2             (* 4 to make room for unsigned int)
    // + uiBytes        (plus the space allocated for pixels)
    // * uiFaces        (* the number of faces for this pixel data)
    m_pucPixels = NiAlloc2(unsigned char,
        ((uiLevels * 3 + 1) << 2) + uiBytes*uiFaces, NiMemHint::TEXTURE);
    m_puiWidth = (unsigned int*)(m_pucPixels + uiBytes*uiFaces);
    m_puiHeight = m_puiWidth + uiLevels;
    m_puiOffsetInBytes = m_puiHeight + uiLevels;
    EE_ASSERT(m_pucPixels != NULL);
}
//--------------------------------------------------------------------------------------------------
void NiPixelData::FreeData()
{
    NiFree(m_pucPixels);
    m_puiWidth = m_puiHeight = m_puiOffsetInBytes = 0;
    m_pucPixels = 0;
}
//--------------------------------------------------------------------------------------------------
