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

#include "NiDecorationPCH.h"
#include "NiDecorationPixelProgramDescriptor.h"

#include "NiDecorationMaterial.h"

//------------------------------------------------------------------------------------------------
NiDecorationPixelProgramDescriptor::NiDecorationPixelProgramDescriptor()
{
    m_uiIntCount = NiDecorationMaterial::PIXEL_PROGRAM_DESCRIPTOR_BYTE_COUNT;

    // Allocate bit array
    m_pkBitArray = (unsigned int*)NiMalloc(sizeof(unsigned int) * m_uiIntCount);

    // Clear bit array
    for (unsigned int ui = 0; ui < m_uiIntCount; ui++)
    {
        m_pkBitArray[ui] = 0;
    }
}

//------------------------------------------------------------------------------------------------
NiString NiDecorationPixelProgramDescriptor::ToString()
{
    NiString kString;
    ToStringPROJLIGHTMAPTYPES(kString, true);
    ToStringUVSETFORMAP04(kString, true);
    ToStringPOINTLIGHTCOUNT(kString, true);
    ToStringPSSMSLICECOUNT(kString, true);
    ToStringFADE_NOISE_ENABLED(kString, true);
    return kString;
}

//------------------------------------------------------------------------------------------------